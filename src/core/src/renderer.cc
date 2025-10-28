#include "vkgs/core/renderer.h"

#include <cstring>
#include <fstream>
#include <unordered_map>
#include <sstream>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "volk.h"

#include "vkgs/gpu/cmd/barrier.h"
#include "vkgs/gpu/cmd/pipeline.h"
#include "vkgs/gpu/cmd/queue_submission.h"
#include "vkgs/gpu/buffer.h"
#include "vkgs/gpu/image.h"
#include "vkgs/gpu/device.h"
#include "vkgs/gpu/semaphore.h"
#include "vkgs/gpu/fence.h"
#include "vkgs/gpu/queue.h"
#include "vkgs/gpu/command.h"
#include "vkgs/gpu/task_monitor.h"
#include "vkgs/gpu/pipeline_layout.h"
#include "vkgs/gpu/compute_pipeline.h"
#include "vkgs/gpu/graphics_pipeline.h"

#include "vkgs/core/gaussian_splats.h"
#include "vkgs/core/rendering_task.h"
#include "generated/parse_ply.h"
#include "generated/parse_data.h"
#include "generated/rank.h"
#include "generated/inverse_index.h"
#include "generated/projection.h"
#include "generated/splat_vert.h"
#include "generated/splat_frag.h"
#include "generated/splat_background_vert.h"
#include "generated/splat_background_frag.h"
#include "sorter.h"
#include "compute_storage.h"
#include "graphics_storage.h"
#include "struct.h"

namespace {

auto WorkgroupSize(size_t count, uint32_t local_size) { return (count + local_size - 1) / local_size; }

}  // namespace

namespace vkgs {
namespace core {

Renderer::Renderer() {
  device_ = std::make_shared<gpu::Device>();
  task_monitor_ = std::make_shared<gpu::TaskMonitor>();
  sorter_ = std::make_shared<Sorter>(*device_, device_->physical_device());

  for (int i = 0; i < 2; ++i) {
    auto& double_buffer = double_buffer_[i];
    double_buffer.compute_storage = std::make_shared<ComputeStorage>(device_);
    double_buffer.graphics_storage = std::make_shared<GraphicsStorage>(device_);
    double_buffer.compute_semaphore = device_->AllocateSemaphore();
    double_buffer.graphics_semaphore = device_->AllocateSemaphore();
    double_buffer.transfer_semaphore = device_->AllocateSemaphore();
  }

  parse_pipeline_layout_ =
      gpu::PipelineLayout::Create(*device_,
                                  {
                                      {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
                                      {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
                                      {2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
                                      {3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
                                      {4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
                                  },
                                  {{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ParsePushConstants)}});
  parse_ply_pipeline_ = gpu::ComputePipeline::Create(*device_, *parse_pipeline_layout_, parse_ply);
  parse_data_pipeline_ = gpu::ComputePipeline::Create(*device_, *parse_pipeline_layout_, parse_data);

  compute_pipeline_layout_ =
      gpu::PipelineLayout::Create(*device_,
                                  {
                                      {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
                                      {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
                                      {2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
                                      {3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
                                      {4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
                                      {5, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
                                      {6, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
                                      {7, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
                                      {8, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
                                  },
                                  {{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants)}});
  rank_pipeline_ = gpu::ComputePipeline::Create(*device_, *compute_pipeline_layout_, rank);
  inverse_index_pipeline_ = gpu::ComputePipeline::Create(*device_, *compute_pipeline_layout_, inverse_index);
  projection_pipeline_ = gpu::ComputePipeline::Create(*device_, *compute_pipeline_layout_, projection);

  graphics_pipeline_layout_ =
      gpu::PipelineLayout::Create(*device_, {{0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT}},
                                  {{VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(GraphicsPushConstants)}});
  splat_pipeline_ = gpu::GraphicsPipeline::Create(*device_, *graphics_pipeline_layout_, splat_vert, splat_frag,
                                                  VK_FORMAT_R16G16B16A16_SFLOAT);
  splat_background_pipeline_ =
      gpu::GraphicsPipeline::Create(*device_, *graphics_pipeline_layout_, splat_background_vert, splat_background_frag,
                                    VK_FORMAT_R16G16B16A16_SFLOAT);
}

Renderer::~Renderer() = default;

const std::string& Renderer::device_name() const noexcept { return device_->device_name(); }
uint32_t Renderer::graphics_queue_index() const noexcept { return device_->graphics_queue_index(); }
uint32_t Renderer::compute_queue_index() const noexcept { return device_->compute_queue_index(); }
uint32_t Renderer::transfer_queue_index() const noexcept { return device_->transfer_queue_index(); }

std::shared_ptr<GaussianSplats> Renderer::CreateGaussianSplats(size_t size, const float* means_ptr,
                                                               const float* quats_ptr, const float* scales_ptr,
                                                               const float* opacities_ptr, const uint16_t* colors_ptr,
                                                               int sh_degree) {
  std::vector<uint32_t> index_data;
  index_data.reserve(6 * size);
  for (int i = 0; i < size; ++i) {
    index_data.push_back(4 * i + 0);
    index_data.push_back(4 * i + 1);
    index_data.push_back(4 * i + 2);
    index_data.push_back(4 * i + 2);
    index_data.push_back(4 * i + 1);
    index_data.push_back(4 * i + 3);
  }

  int colors_size = 0;
  int sh_packed_size = 0;
  switch (sh_degree) {
    case 0:
      colors_size = 1;
      sh_packed_size = 1;
      break;
    case 1:
      colors_size = 4;
      sh_packed_size = 3;
      break;
    case 2:
      colors_size = 9;
      sh_packed_size = 7;
      break;
    case 3:
      colors_size = 16;
      sh_packed_size = 12;
      break;
    default:
      throw std::runtime_error("Unsupported SH degree: " + std::to_string(sh_degree));
  }

  auto position_stage = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, size * 3 * sizeof(float), true);
  auto quats_stage = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, size * 4 * sizeof(float), true);
  auto scales_stage = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, size * 3 * sizeof(float), true);
  auto colors_stage =
      gpu::Buffer::Create(device_, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, size * colors_size * 3 * sizeof(uint16_t), true);
  auto opacity_stage = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, size * sizeof(float), true);
  auto index_stage = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, size * 6 * sizeof(uint32_t), true);

  auto position = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                      size * 3 * sizeof(float));
  auto quats = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                   size * 4 * sizeof(float));
  auto scales = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                    size * 3 * sizeof(float));
  auto colors = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                    size * colors_size * 3 * sizeof(uint16_t));
  auto opacity = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                     size * sizeof(float));

  auto cov3d = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, size * 6 * sizeof(float));
  auto sh =
      gpu::Buffer::Create(device_, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, size * sh_packed_size * 4 * sizeof(uint16_t));
  auto index_buffer = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                          size * 6 * sizeof(uint32_t));

  std::memcpy(position_stage->data(), means_ptr, position_stage->size());
  std::memcpy(quats_stage->data(), quats_ptr, quats_stage->size());
  std::memcpy(scales_stage->data(), scales_ptr, scales_stage->size());
  std::memcpy(opacity_stage->data(), opacities_ptr, opacity_stage->size());
  std::memcpy(colors_stage->data(), colors_ptr, colors_stage->size());
  std::memcpy(index_stage->data(), index_data.data(), index_stage->size());

  ParsePushConstants parse_data_push_constants = {};
  parse_data_push_constants.point_count = size;
  parse_data_push_constants.sh_degree = sh_degree;

  auto sem = device_->AllocateSemaphore();
  auto tq = device_->transfer_queue();
  auto cq = device_->compute_queue();
  auto gq = device_->graphics_queue();

  std::shared_ptr<gpu::Task> task;

  // Transfer queue: stage to buffers
  {
    auto cb = tq->AllocateCommandBuffer();
    auto fence = device_->AllocateFence();

    VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(*cb, &begin_info);

    VkBufferCopy region = {0, 0, position_stage->size()};
    vkCmdCopyBuffer(*cb, *position_stage, *position, 1, &region);
    region = {0, 0, quats_stage->size()};
    vkCmdCopyBuffer(*cb, *quats_stage, *quats, 1, &region);
    region = {0, 0, scales_stage->size()};
    vkCmdCopyBuffer(*cb, *scales_stage, *scales, 1, &region);
    region = {0, 0, colors_stage->size()};
    vkCmdCopyBuffer(*cb, *colors_stage, *colors, 1, &region);
    region = {0, 0, opacity_stage->size()};
    vkCmdCopyBuffer(*cb, *opacity_stage, *opacity, 1, &region);
    region = {0, 0, index_stage->size()};
    vkCmdCopyBuffer(*cb, *index_stage, *index_buffer, 1, &region);

    gpu::cmd::Barrier()
        .Release(VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, *tq, *cq, *position)
        .Release(VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, *tq, *cq, *quats)
        .Release(VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, *tq, *cq, *scales)
        .Release(VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, *tq, *cq, *colors)
        .Release(VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, *tq, *cq, *opacity)
        .Release(VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, *tq, *gq, *index_buffer)
        .Commit(*cb);

    vkEndCommandBuffer(*cb);

    gpu::cmd::QueueSubmission()
        .Command(*cb)
        .Signal(*sem, sem->value() + 1, VK_PIPELINE_STAGE_2_TRANSFER_BIT)
        .Submit(*tq, *fence);

    task_monitor_->Add(fence, {cb, position_stage, quats_stage, scales_stage, colors_stage, opacity_stage, index_stage,
                               position, quats, scales, colors, opacity, index_buffer});
  }

  // Compute queue: parse data
  {
    auto cb = cq->AllocateCommandBuffer();
    auto fence = device_->AllocateFence();

    VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(*cb, &begin_info);

    gpu::cmd::Barrier()
        .Acquire(VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, *tq, *cq, *position)
        .Acquire(VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, *tq, *cq, *quats)
        .Acquire(VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, *tq, *cq, *scales)
        .Acquire(VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, *tq, *cq, *colors)
        .Acquire(VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, *tq, *cq, *opacity)
        .Commit(*cb);

    gpu::cmd::Pipeline(VK_PIPELINE_BIND_POINT_COMPUTE, *parse_pipeline_layout_)
        .Storage(0, *quats)
        .Storage(1, *scales)
        .Storage(2, *cov3d)
        .Storage(3, *colors)
        .Storage(4, *sh)
        .PushConstant(VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(parse_data_push_constants), &parse_data_push_constants)
        .Bind(*parse_data_pipeline_)
        .Commit(*cb);
    vkCmdDispatch(*cb, WorkgroupSize(size, 256), 1, 1);

    gpu::cmd::Barrier()
        .Memory(VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT)
        .Commit(*cb);

    vkEndCommandBuffer(*cb);

    gpu::cmd::QueueSubmission()
        .Wait(*sem, sem->value() + 1, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT)
        .Command(*cb)
        .Submit(*cq, *fence);

    task = task_monitor_->Add(fence, {cb, sem, position, quats, scales, cov3d, colors, sh, opacity});
  }

  // Graphics queue: make visible
  {
    auto cb = gq->AllocateCommandBuffer();
    auto fence = device_->AllocateFence();

    VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(*cb, &begin_info);

    gpu::cmd::Barrier()
        .Acquire(VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT, VK_ACCESS_2_INDEX_READ_BIT, *tq, *gq, *index_buffer)
        .Commit(*cb);

    vkEndCommandBuffer(*cb);

    gpu::cmd::QueueSubmission()
        .Wait(*sem, sem->value() + 1, VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT)
        .Command(*cb)
        .Submit(*gq, *fence);

    task_monitor_->Add(fence, {cb, sem, index_buffer});
  }

  sem->Increment();

  return std::make_shared<GaussianSplats>(size, sh_degree, position, cov3d, sh, opacity, index_buffer, task);
}

std::shared_ptr<GaussianSplats> Renderer::LoadFromPly(const std::string& path, int sh_degree) {
  std::ifstream in(path, std::ios::binary);

  // parse header
  std::unordered_map<std::string, int> offsets;
  int offset = 0;
  uint32_t point_count = 0;
  std::string line;
  while (std::getline(in, line)) {
    if (line == "end_header") break;

    std::istringstream iss(line);
    std::string word;
    iss >> word;
    if (word == "property") {
      int size = 0;
      std::string type, property;
      iss >> type >> property;
      if (type == "float") {
        size = 4;
      }
      offsets[property] = offset;
      offset += size;
    } else if (word == "element") {
      std::string type;
      size_t count;
      iss >> type >> count;
      if (type == "vertex") {
        point_count = count;
      }
    }
  }

  int K = 0;
  for (const auto& [key, _] : offsets) {
    if (key.find("f_rest_") != std::string::npos) {
      K = std::max(K, std::stoi(key.substr(7)));
    }
  }
  K = K + 1;

  int sh_degree_data = 0;  // [0, 1, 2, 3], sh degree
  int sh_packed_size = 0;  // [1, 3, 7, 12], storage dimension for packing with f16vec4.
  switch (K) {
    case 0:  // no f_rest
      sh_degree_data = 0;
      sh_packed_size = 1;
      break;
    case 9:  // f_rest_[0..9)
      sh_degree_data = 1;
      sh_packed_size = 3;
      break;
    case 24:  // f_rest_[0..24)
      sh_degree_data = 2;
      sh_packed_size = 7;
      break;
    case 45:  // f_rest_[0..45)
      sh_degree_data = 3;
      sh_packed_size = 12;
      break;
    default:
      throw std::runtime_error("Unsupported SH degree for having f_rest_[0.." + std::to_string(K - 1) + "]");
  }
  K /= 3;

  if (sh_degree == -1) sh_degree = sh_degree_data;
  if (sh_degree > sh_degree_data) {
    throw std::runtime_error("SH degree for drawing is greater than the maximum degree of the data");
  }

  std::vector<uint32_t> ply_offsets(60);
  ply_offsets[0] = offsets["x"] / 4;
  ply_offsets[1] = offsets["y"] / 4;
  ply_offsets[2] = offsets["z"] / 4;
  ply_offsets[3] = offsets["scale_0"] / 4;
  ply_offsets[4] = offsets["scale_1"] / 4;
  ply_offsets[5] = offsets["scale_2"] / 4;
  ply_offsets[6] = offsets["rot_1"] / 4;  // qx
  ply_offsets[7] = offsets["rot_2"] / 4;  // qy
  ply_offsets[8] = offsets["rot_3"] / 4;  // qz
  ply_offsets[9] = offsets["rot_0"] / 4;  // qw
  ply_offsets[10 + 0] = offsets["f_dc_0"] / 4;
  ply_offsets[10 + 16] = offsets["f_dc_1"] / 4;
  ply_offsets[10 + 32] = offsets["f_dc_2"] / 4;
  for (int i = 0; i < K; ++i) {
    ply_offsets[10 + 1 + i] = offsets["f_rest_" + std::to_string(K * 0 + i)] / 4;
    ply_offsets[10 + 17 + i] = offsets["f_rest_" + std::to_string(K * 1 + i)] / 4;
    ply_offsets[10 + 33 + i] = offsets["f_rest_" + std::to_string(K * 2 + i)] / 4;
  }
  ply_offsets[58] = offsets["opacity"] / 4;
  ply_offsets[59] = offset / 4;

  std::vector<char> buffer(offset * point_count);
  in.read(buffer.data(), buffer.size());

  std::vector<uint32_t> index_data;
  index_data.reserve(6 * point_count);
  for (int i = 0; i < point_count; ++i) {
    index_data.push_back(4 * i + 0);
    index_data.push_back(4 * i + 1);
    index_data.push_back(4 * i + 2);
    index_data.push_back(4 * i + 2);
    index_data.push_back(4 * i + 1);
    index_data.push_back(4 * i + 3);
  }

  ParsePushConstants parse_ply_push_constants = {};
  parse_ply_push_constants.point_count = point_count;
  parse_ply_push_constants.sh_degree = sh_degree;

  // allocate buffers
  auto buffer_size = buffer.size() + 60 * sizeof(uint32_t);
  auto ply_stage = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                       buffer_size, true);
  auto ply_buffer =
      gpu::Buffer::Create(device_, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, buffer_size);

  auto position = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, point_count * 3 * sizeof(float));
  auto cov3d = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, point_count * 6 * sizeof(float));
  auto sh = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                point_count * sh_packed_size * 4 * sizeof(uint16_t));
  auto opacity = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, point_count * sizeof(float));

  auto index_stage =
      gpu::Buffer::Create(device_, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, point_count * 6 * sizeof(uint32_t), true);
  auto index_buffer = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                          point_count * 6 * sizeof(uint32_t));

  std::memcpy(ply_stage->data(), ply_offsets.data(), ply_offsets.size() * sizeof(uint32_t));
  std::memcpy(ply_stage->data<char>() + ply_offsets.size() * sizeof(uint32_t), buffer.data(), buffer.size());
  std::memcpy(index_stage->data(), index_data.data(), index_data.size() * sizeof(uint32_t));

  auto sem = device_->AllocateSemaphore();

  auto cq = device_->compute_queue();
  auto gq = device_->graphics_queue();
  auto tq = device_->transfer_queue();

  std::shared_ptr<gpu::Task> task;

  // Transfer queue: stage to buffers
  {
    auto cb = tq->AllocateCommandBuffer();
    auto fence = device_->AllocateFence();

    VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(*cb, &begin_info);

    VkBufferCopy region = {0, 0, buffer_size};
    vkCmdCopyBuffer(*cb, *ply_stage, *ply_buffer, 1, &region);

    region = {0, 0, index_stage->size()};
    vkCmdCopyBuffer(*cb, *index_stage, *index_buffer, 1, &region);

    gpu::cmd::Barrier()
        .Release(VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, *tq, *cq, *ply_buffer)
        .Release(VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, *tq, *gq, *index_buffer)
        .Commit(*cb);

    vkEndCommandBuffer(*cb);

    gpu::cmd::QueueSubmission()
        .Command(*cb)
        .Signal(*sem, sem->value() + 1, VK_PIPELINE_STAGE_2_TRANSFER_BIT)
        .Submit(*tq, *fence);

    task_monitor_->Add(fence, {cb, sem, ply_stage, ply_buffer, index_stage, index_buffer});
  }

  // Compute queue: parse ply
  {
    auto cb = cq->AllocateCommandBuffer();
    auto fence = device_->AllocateFence();

    VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(*cb, &begin_info);

    gpu::cmd::Barrier()
        .Acquire(VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, *tq, *cq, *ply_buffer)
        .Commit(*cb);

    // ply_buffer -> gaussian_splats
    gpu::cmd::Pipeline(VK_PIPELINE_BIND_POINT_COMPUTE, *parse_pipeline_layout_)
        .Storage(0, *ply_buffer)
        .Storage(1, *position)
        .Storage(2, *cov3d)
        .Storage(3, *opacity)
        .Storage(4, *sh)
        .PushConstant(VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(parse_ply_push_constants), &parse_ply_push_constants)
        .Bind(*parse_ply_pipeline_)
        .Commit(*cb);
    vkCmdDispatch(*cb, WorkgroupSize(point_count, 256), 1, 1);

    gpu::cmd::Barrier()
        .Memory(VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT)
        .Commit(*cb);

    vkEndCommandBuffer(*cb);

    // Submit
    gpu::cmd::QueueSubmission()
        .Wait(*sem, sem->value() + 1, VK_PIPELINE_STAGE_2_TRANSFER_BIT)
        .Command(*cb)
        .Submit(*cq, *fence);

    task = task_monitor_->Add(fence, {cb, sem, parse_ply_pipeline_, ply_buffer, position, cov3d, sh, opacity});
  }

  // Graphics queue: acquire index buffer
  {
    auto cb = gq->AllocateCommandBuffer();
    auto fence = device_->AllocateFence();

    VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(*cb, &begin_info);

    gpu::cmd::Barrier()
        .Acquire(VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT, VK_ACCESS_2_INDEX_READ_BIT, *tq, *gq, *index_buffer)
        .Commit(*cb);

    vkEndCommandBuffer(*cb);

    // Submit
    gpu::cmd::QueueSubmission()
        .Wait(*sem, sem->value() + 1, VK_PIPELINE_STAGE_2_TRANSFER_BIT)
        .Command(*cb)
        .Submit(*gq, *fence);

    task_monitor_->Add(fence, {cb, sem, index_buffer});
  }
  sem->Increment();

  return std::make_shared<GaussianSplats>(point_count, sh_degree, position, cov3d, sh, opacity, index_buffer, task);
}

std::shared_ptr<RenderingTask> Renderer::Draw(std::shared_ptr<GaussianSplats> splats,
                                              const std::vector<DrawOptions>& batch_draw_options, uint32_t width,
                                              uint32_t height, uint8_t* dst) {
  std::shared_ptr<RenderingTask> rendering_task;
  std::vector<std::shared_ptr<gpu::Fence>> fences;

  auto cq = device_->compute_queue();
  auto gq = device_->graphics_queue();
  auto tq = device_->transfer_queue();

  auto N = splats->size();
  auto position = splats->position();
  auto cov3d = splats->cov3d();
  auto sh = splats->sh();
  auto opacity = splats->opacity();
  auto index_buffer = splats->index_buffer();

  uint32_t B = batch_draw_options.size();
  auto image_stage = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_TRANSFER_DST_BIT, B * width * height * 4, true);
  auto camera_stage = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, B * sizeof(Camera), true);
  auto camera = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                    B * sizeof(Camera));

  std::vector<Camera> camera_data(B);
  for (int i = 0; i < B; ++i) {
    camera_data[i].projection = batch_draw_options[i].projection;
    camera_data[i].view = batch_draw_options[i].view;
    camera_data[i].camera_position = glm::inverse(batch_draw_options[i].view)[3];
    camera_data[i].screen_size = glm::uvec2(width, height);
  }
  std::memcpy(camera_stage->data(), camera_data.data(), B * sizeof(Camera));

  // Compute queue: copy camera data to buffer
  {
    auto fence = device_->AllocateFence();
    auto cb = cq->AllocateCommandBuffer();

    VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(*cb, &begin_info);

    VkBufferCopy region = {0, 0, B * sizeof(Camera)};
    vkCmdCopyBuffer(*cb, *camera_stage, *camera, 1, &region);

    gpu::cmd::Barrier()
        .Memory(VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT)
        .Commit(*cb);

    vkEndCommandBuffer(*cb);

    gpu::cmd::QueueSubmission().Command(*cb).Submit(*cq, *fence);
    task_monitor_->Add(fence, {cb, camera_stage, camera});
  }

  for (int i = 0; i < B; ++i) {
    const auto& draw_options = batch_draw_options[i];

    // Update storages
    const auto& double_buffer = double_buffer_[frame_index_ % 2];
    auto compute_storage = double_buffer.compute_storage;
    auto graphics_storage = double_buffer.graphics_storage;
    auto csem = double_buffer.compute_semaphore;
    auto cval = csem->value();
    auto gsem = double_buffer.graphics_semaphore;
    auto gval = gsem->value();
    auto tsem = double_buffer.transfer_semaphore;
    auto tval = tsem->value();

    compute_storage->Update(N, sorter_->GetStorageRequirements(N));
    graphics_storage->Update(width, height);

    auto visible_point_count = compute_storage->visible_point_count();
    auto key = compute_storage->key();
    auto index = compute_storage->index();
    auto sort_storage = compute_storage->sort_storage();
    auto inverse_index = compute_storage->inverse_index();
    auto draw_indirect = compute_storage->draw_indirect();
    auto instances = compute_storage->instances();

    // Compute queue
    {
      ComputePushConstants compute_push_constants = {};
      compute_push_constants.model = glm::mat4(1.f);
      compute_push_constants.point_count = N;
      compute_push_constants.eps2d = draw_options.eps2d;
      compute_push_constants.sh_degree_data = splats->sh_degree();
      compute_push_constants.sh_degree_draw =
          draw_options.sh_degree == -1 ? splats->sh_degree() : draw_options.sh_degree;

      auto fence = device_->AllocateFence();
      auto cb = cq->AllocateCommandBuffer();

      VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
      begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
      vkBeginCommandBuffer(*cb, &begin_info);

      vkCmdFillBuffer(*cb, *visible_point_count, 0, sizeof(uint32_t), 0);
      vkCmdFillBuffer(*cb, *inverse_index, 0, N * sizeof(uint32_t), -1);

      gpu::cmd::Barrier()
          .Memory(VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
                  VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT)
          .Commit(*cb);

      // Rank
      gpu::cmd::Pipeline pipeline(VK_PIPELINE_BIND_POINT_COMPUTE, *compute_pipeline_layout_);
      pipeline.Storage(0, *camera, i * sizeof(Camera), sizeof(Camera))
          .Storage(1, *position)
          .Storage(2, *visible_point_count)
          .Storage(3, *key)
          .Storage(4, *index)
          .PushConstant(VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(compute_push_constants), &compute_push_constants)
          .Bind(*rank_pipeline_)
          .Commit(*cb);
      vkCmdDispatch(*cb, WorkgroupSize(N, 256), 1, 1);

      // Sort
      gpu::cmd::Barrier()
          .Memory(VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
                  VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                  VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_TRANSFER_READ_BIT)
          .Commit(*cb);

      sorter_->SortKeyValueIndirect(*cb, N, *visible_point_count, *key, *index, *sort_storage);

      // Inverse index
      gpu::cmd::Barrier()
          .Memory(VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
                  VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT)
          .Commit(*cb);

      pipeline.Storage(0, *visible_point_count)
          .Storage(1, *index)
          .Storage(2, *inverse_index)
          .PushConstant(VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(compute_push_constants), &compute_push_constants)
          .Bind(*inverse_index_pipeline_)
          .Commit(*cb);
      vkCmdDispatch(*cb, WorkgroupSize(N, 256), 1, 1);

      // Projection
      gpu::cmd::Barrier()
          .Memory(VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
                  VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT)
          .Commit(*cb);

      pipeline.Storage(0, *camera, i * sizeof(Camera), sizeof(Camera))
          .Storage(1, *position)
          .Storage(2, *cov3d)
          .Storage(3, *opacity)
          .Storage(4, *sh)
          .Storage(5, *visible_point_count)
          .Storage(6, *inverse_index)
          .Storage(7, *draw_indirect)
          .Storage(8, *instances)
          .Bind(*projection_pipeline_)
          .Commit(*cb);
      vkCmdDispatch(*cb, WorkgroupSize(N, 256), 1, 1);

      // Release
      gpu::cmd::Barrier()
          .Release(VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, *cq, *gq, *instances)
          .Release(VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, *cq, *gq, *draw_indirect)
          .Commit(*cb);

      vkEndCommandBuffer(*cb);

      // Submit
      gpu::cmd::QueueSubmission submission;
      if (frame_index_ >= 2) {
        // G[i-2].read before C[i].comp
        submission.Wait(*gsem, gval - 2 + 1, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_TRANSFER_BIT);
      }
      submission.Command(*cb).Signal(*csem, cval + 1, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT).Submit(*cq, *fence);

      task_monitor_->Add(fence, {cb, csem, camera, position, cov3d, opacity, sh, visible_point_count, key, index,
                                 sort_storage, inverse_index, draw_indirect, instances});
    }

    auto image = graphics_storage->image();
    auto image_u8 = graphics_storage->image_u8();

    // Graphics queue
    {
      GraphicsPushConstants graphics_push_constants = {};
      graphics_push_constants.background = glm::vec4(draw_options.background, 1.f);

      auto fence = device_->AllocateFence();
      auto cb = gq->AllocateCommandBuffer();

      VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
      begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
      vkBeginCommandBuffer(*cb, &begin_info);

      gpu::cmd::Barrier()
          // Acquire
          .Acquire(VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, *cq, *gq, *instances)
          .Acquire(VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT, *cq, *gq,
                   *draw_indirect)
          // Layout transition to color attachment
          .Image(0, 0, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                 VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, *image)
          .Commit(*cb);

      // Rendering
      VkRenderingAttachmentInfo color_attachment = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
      color_attachment.imageView = image->image_view();
      color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      color_attachment.clearValue.color = {0.f, 0.f, 0.f, 0.f};
      VkRenderingInfo rendering_info = {VK_STRUCTURE_TYPE_RENDERING_INFO};
      rendering_info.renderArea.offset = {0, 0};
      rendering_info.renderArea.extent = {width, height};
      rendering_info.layerCount = 1;
      rendering_info.colorAttachmentCount = 1;
      rendering_info.pColorAttachments = &color_attachment;
      vkCmdBeginRendering(*cb, &rendering_info);

      gpu::cmd::Pipeline pipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, *graphics_pipeline_layout_);
      pipeline.Storage(0, *instances)
          .PushConstant(VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(graphics_push_constants), &graphics_push_constants)
          .Bind(*splat_pipeline_)
          .Commit(*cb);

      VkViewport viewport = {0.f, 0.f, static_cast<float>(width), static_cast<float>(height), 0.f, 1.f};
      vkCmdSetViewport(*cb, 0, 1, &viewport);
      VkRect2D scissor = {0, 0, width, height};
      vkCmdSetScissor(*cb, 0, 1, &scissor);

      vkCmdBindIndexBuffer(*cb, *index_buffer, 0, VK_INDEX_TYPE_UINT32);
      vkCmdDrawIndexedIndirect(*cb, *draw_indirect, 0, 1, 0);

      pipeline.Bind(*splat_background_pipeline_).Commit(*cb);
      vkCmdDraw(*cb, 3, 1, 0, 0);

      vkCmdEndRendering(*cb);

      // float -> uint8
      gpu::cmd::Barrier()
          .Image(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                 VK_PIPELINE_STAGE_2_BLIT_BIT, VK_ACCESS_2_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *image)
          .Image(0, 0, VK_PIPELINE_STAGE_2_BLIT_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, *image_u8)
          .Commit(*cb);

      VkImageBlit image_region = {};
      image_region.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
      image_region.srcOffsets[0] = {0, 0, 0};
      image_region.srcOffsets[1] = {static_cast<int>(width), static_cast<int>(height), 1};
      image_region.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
      image_region.dstOffsets[0] = {0, 0, 0};
      image_region.dstOffsets[1] = {static_cast<int>(width), static_cast<int>(height), 1};
      vkCmdBlitImage(*cb, *image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *image_u8, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                     1, &image_region, VK_FILTER_NEAREST);

      // Layout transition to transfer src, and release
      gpu::cmd::Barrier()
          .Release(VK_PIPELINE_STAGE_2_BLIT_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *gq, *tq, *image_u8)
          .Commit(*cb);

      vkEndCommandBuffer(*cb);

      // Submit
      gpu::cmd::QueueSubmission submission;
      // C[i].comp before G[i].read
      submission.Wait(*csem, cval + 1,
                      VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT |
                          VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT);

      if (frame_index_ >= 2) {
        // T[i-2].xfer before G[i].output
        submission.Wait(*tsem, tval - 1 + 1, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
      }

      submission
          .Command(*cb)
          // G[i].read
          .Signal(*gsem, gval + 1,
                  VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT |
                      VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT)
          // G[i].blit
          .Signal(*gsem, gval + 2, VK_PIPELINE_STAGE_2_BLIT_BIT)
          .Submit(*gq, *fence);

      task_monitor_->Add(fence, {cb, image, instances, index_buffer, draw_indirect, gsem});
    }

    // Transfer queue
    {
      auto fence = device_->AllocateFence();
      auto cb = tq->AllocateCommandBuffer();

      VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
      begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
      vkBeginCommandBuffer(*cb, &begin_info);

      gpu::cmd::Barrier()
          .Acquire(VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_READ_BIT,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *gq, *tq, *image_u8)
          .Commit(*cb);

      // Image to buffer
      VkBufferImageCopy region;
      region.bufferOffset = i * width * height * 4;
      region.bufferRowLength = 0;
      region.bufferImageHeight = 0;
      region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
      region.imageOffset = {0, 0, 0};
      region.imageExtent = {width, height, 1};
      vkCmdCopyImageToBuffer(*cb, *image_u8, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *image_stage, 1, &region);

      vkEndCommandBuffer(*cb);

      // Submit
      gpu::cmd::QueueSubmission()
          // G[i].blit before T[i].xfer
          .Wait(*gsem, gval + 2, VK_PIPELINE_STAGE_2_TRANSFER_BIT)
          .Command(*cb)
          // T[i].xfer
          .Signal(*tsem, tval + 1, VK_PIPELINE_STAGE_2_TRANSFER_BIT)
          .Submit(*tq, *fence);

      task_monitor_->Add(fence, {cb, image_u8, image_stage, tsem});
      fences.push_back(fence);
    }

    csem->Increment();
    gsem->Increment();
    gsem->Increment();
    tsem->Increment();
    frame_index_++;
  }

  return std::make_shared<RenderingTask>(std::move(fences), image_stage, B * width * height * 4, dst);
}

}  // namespace core
}  // namespace vkgs
