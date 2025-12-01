#include "vkgs/core/parser.h"

#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <cstring>

#include "vkgs/gpu/gpu.h"
#include "vkgs/gpu/device.h"
#include "vkgs/gpu/compute_pipeline.h"
#include "vkgs/gpu/buffer.h"
#include "vkgs/gpu/task.h"
#include "vkgs/gpu/semaphore.h"
#include "vkgs/gpu/cmd/barrier.h"
#include "vkgs/gpu/cmd/pipeline.h"

#include "vkgs/core/gaussian_splats.h"

#include "generated/parse_ply.h"
#include "generated/parse_data.h"
#include "struct.h"

namespace {

auto WorkgroupSize(size_t count, uint32_t local_size) { return (count + local_size - 1) / local_size; }

auto GetIndexData(int size) {
  std::vector<uint32_t> index_data;
  index_data.reserve(6 * size);
  for (int i = 0; i < size; ++i) {
    index_data.push_back(4 * i + 0);
    index_data.push_back(4 * i + 1);
    index_data.push_back(4 * i + 2);
    index_data.push_back(4 * i + 0);
    index_data.push_back(4 * i + 2);
    index_data.push_back(4 * i + 3);
  }
  return index_data;
}

}  // namespace

namespace vkgs {
namespace core {

Parser::Parser() {
  parse_pipeline_layout_ = gpu::PipelineLayout::Create({
      .bindings =
          {
              {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
              {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
              {2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
              {3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
              {4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
          },
      .push_constants = {{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ParsePushConstants)}},
  });
  parse_ply_pipeline_ = gpu::ComputePipeline::Create(parse_pipeline_layout_, parse_ply);
  parse_data_pipeline_ = gpu::ComputePipeline::Create(parse_pipeline_layout_, parse_data);
}

Parser::~Parser() = default;

std::shared_ptr<GaussianSplats> Parser::CreateGaussianSplats(size_t size, const float* means_ptr,
                                                             const float* quats_ptr, const float* scales_ptr,
                                                             const float* opacities_ptr, const uint16_t* colors_ptr,
                                                             int sh_degree) {
  std::vector<uint32_t> index_data = GetIndexData(size);

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

  auto position_stage = gpu::Buffer::Create(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, size * 3 * sizeof(float), true);
  auto quats_stage = gpu::Buffer::Create(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, size * 4 * sizeof(float), true);
  auto scales_stage = gpu::Buffer::Create(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, size * 3 * sizeof(float), true);
  auto colors_stage =
      gpu::Buffer::Create(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, size * colors_size * 3 * sizeof(uint16_t), true);
  auto opacity_stage = gpu::Buffer::Create(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, size * sizeof(float), true);
  auto index_stage = gpu::Buffer::Create(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, index_data.size() * sizeof(uint32_t), true);

  auto position = gpu::Buffer::Create(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                      size * 3 * sizeof(float));
  auto quats = gpu::Buffer::Create(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                   size * 4 * sizeof(float));
  auto scales = gpu::Buffer::Create(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                    size * 3 * sizeof(float));
  auto colors = gpu::Buffer::Create(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                    size * colors_size * 3 * sizeof(uint16_t));
  auto opacity =
      gpu::Buffer::Create(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, size * sizeof(float));

  auto cov3d = gpu::Buffer::Create(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, size * 6 * sizeof(float));
  auto sh = gpu::Buffer::Create(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, size * sh_packed_size * 4 * sizeof(uint16_t));
  auto index_buffer = gpu::Buffer::Create(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                          index_data.size() * sizeof(uint32_t));

  std::memcpy(position_stage->data(), means_ptr, position_stage->size());
  std::memcpy(quats_stage->data(), quats_ptr, quats_stage->size());
  std::memcpy(scales_stage->data(), scales_ptr, scales_stage->size());
  std::memcpy(opacity_stage->data(), opacities_ptr, opacity_stage->size());
  std::memcpy(colors_stage->data(), colors_ptr, colors_stage->size());
  std::memcpy(index_stage->data(), index_data.data(), index_stage->size());

  ParsePushConstants parse_data_push_constants = {
      .point_count = static_cast<uint32_t>(size),
      .sh_degree = static_cast<uint32_t>(sh_degree),
  };

  auto device = gpu::GetDevice();
  auto sem = device->AllocateSemaphore();
  auto tq = device->transfer_queue_index();
  auto cq = device->compute_queue_index();
  auto gq = device->graphics_queue_index();

  // Transfer queue: stage to buffers
  {
    gpu::TransferTask task;
    auto cb = task.command_buffer();

    VkBufferCopy region = {0, 0, position_stage->size()};
    vkCmdCopyBuffer(cb, position_stage, position, 1, &region);
    region = {0, 0, quats_stage->size()};
    vkCmdCopyBuffer(cb, quats_stage, quats, 1, &region);
    region = {0, 0, scales_stage->size()};
    vkCmdCopyBuffer(cb, scales_stage, scales, 1, &region);
    region = {0, 0, colors_stage->size()};
    vkCmdCopyBuffer(cb, colors_stage, colors, 1, &region);
    region = {0, 0, opacity_stage->size()};
    vkCmdCopyBuffer(cb, opacity_stage, opacity, 1, &region);
    region = {0, 0, index_stage->size()};
    vkCmdCopyBuffer(cb, index_stage, index_buffer, 1, &region);

    gpu::cmd::Barrier()
        .Release(VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, tq, cq, position)
        .Release(VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, tq, cq, quats)
        .Release(VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, tq, cq, scales)
        .Release(VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, tq, cq, colors)
        .Release(VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, tq, cq, opacity)
        .Release(VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, tq, gq, index_buffer)
        .Commit(cb);

    task.Signal(sem, sem->value() + 1, VK_PIPELINE_STAGE_2_TRANSFER_BIT);

    position_stage->Keep();
    quats_stage->Keep();
    scales_stage->Keep();
    colors_stage->Keep();
    opacity_stage->Keep();
    index_stage->Keep();
  }

  // Compute queue: parse data
  gpu::QueueTask queue_task;
  {
    gpu::ComputeTask task;
    auto cb = task.command_buffer();

    gpu::cmd::Barrier()
        .Acquire(VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, tq, cq, position)
        .Acquire(VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, tq, cq, quats)
        .Acquire(VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, tq, cq, scales)
        .Acquire(VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, tq, cq, colors)
        .Acquire(VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, tq, cq, opacity)
        .Commit(cb);

    gpu::cmd::Pipeline(VK_PIPELINE_BIND_POINT_COMPUTE, parse_pipeline_layout_)
        .Storage(0, quats)
        .Storage(1, scales)
        .Storage(2, cov3d)
        .Storage(3, colors)
        .Storage(4, sh)
        .PushConstant(VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(parse_data_push_constants), &parse_data_push_constants)
        .Bind(parse_data_pipeline_)
        .Commit(cb);
    vkCmdDispatch(cb, WorkgroupSize(size, 256), 1, 1);

    gpu::cmd::Barrier()
        .Memory(VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT)
        .Commit(cb);

    quats->Keep();
    scales->Keep();
    colors->Keep();

    task.Wait(sem, sem->value() + 1, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
    queue_task = task.Submit();
  }

  // Graphics queue: make visible
  {
    gpu::GraphicsTask task;
    auto cb = task.command_buffer();

    gpu::cmd::Barrier()
        .Acquire(VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT, VK_ACCESS_2_INDEX_READ_BIT, tq, gq, index_buffer)
        .Commit(cb);

    task.Wait(sem, sem->value() + 1, VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT);
  }

  sem->Increment();

  return std::make_shared<GaussianSplats>(size, sh_degree, position, cov3d, sh, opacity, index_buffer, queue_task);
}

std::shared_ptr<GaussianSplats> Parser::LoadFromPly(const std::string& path, int sh_degree) {
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

  std::vector<uint32_t> index_data = GetIndexData(point_count);

  ParsePushConstants parse_ply_push_constants = {
      .point_count = point_count,
      .sh_degree = static_cast<uint32_t>(sh_degree),
  };

  // allocate buffers
  auto buffer_size = buffer.size() + 60 * sizeof(uint32_t);
  auto ply_stage =
      gpu::Buffer::Create(VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, buffer_size, true);
  auto ply_buffer =
      gpu::Buffer::Create(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, buffer_size);

  auto position = gpu::Buffer::Create(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, point_count * 3 * sizeof(float));
  auto cov3d = gpu::Buffer::Create(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, point_count * 6 * sizeof(float));
  auto sh =
      gpu::Buffer::Create(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, point_count * sh_packed_size * 4 * sizeof(uint16_t));
  auto opacity = gpu::Buffer::Create(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, point_count * sizeof(float));

  auto index_stage = gpu::Buffer::Create(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, index_data.size() * sizeof(uint32_t), true);
  auto index_buffer = gpu::Buffer::Create(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                          index_data.size() * sizeof(uint32_t));

  std::memcpy(ply_stage->data(), ply_offsets.data(), ply_offsets.size() * sizeof(uint32_t));
  std::memcpy(ply_stage->data<char>() + ply_offsets.size() * sizeof(uint32_t), buffer.data(), buffer.size());
  std::memcpy(index_stage->data(), index_data.data(), index_data.size() * sizeof(uint32_t));

  auto device = gpu::GetDevice();
  auto sem = device->AllocateSemaphore();
  auto tq = device->transfer_queue_index();
  auto cq = device->compute_queue_index();
  auto gq = device->graphics_queue_index();

  // Transfer queue: stage to buffers
  {
    gpu::TransferTask task;
    auto cb = task.command_buffer();

    VkBufferCopy region = {0, 0, buffer_size};
    vkCmdCopyBuffer(cb, ply_stage, ply_buffer, 1, &region);

    region = {0, 0, index_stage->size()};
    vkCmdCopyBuffer(cb, index_stage, index_buffer, 1, &region);

    gpu::cmd::Barrier()
        .Release(VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, tq, cq, ply_buffer)
        .Release(VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, tq, gq, index_buffer)
        .Commit(cb);

    task.Signal(sem, sem->value() + 1, VK_PIPELINE_STAGE_2_TRANSFER_BIT);

    ply_stage->Keep();
    index_stage->Keep();
  }

  // Compute queue: parse ply
  gpu::QueueTask queue_task;
  {
    gpu::ComputeTask task;
    auto cb = task.command_buffer();

    gpu::cmd::Barrier()
        .Acquire(VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, tq, cq, ply_buffer)
        .Commit(cb);

    // ply_buffer -> gaussian_splats
    gpu::cmd::Pipeline(VK_PIPELINE_BIND_POINT_COMPUTE, parse_pipeline_layout_)
        .Storage(0, ply_buffer)
        .Storage(1, position)
        .Storage(2, cov3d)
        .Storage(3, opacity)
        .Storage(4, sh)
        .PushConstant(VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(parse_ply_push_constants), &parse_ply_push_constants)
        .Bind(parse_ply_pipeline_)
        .Commit(cb);
    vkCmdDispatch(cb, WorkgroupSize(point_count, 256), 1, 1);

    gpu::cmd::Barrier()
        .Memory(VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT)
        .Commit(cb);

    ply_buffer->Keep();

    task.Wait(sem, sem->value() + 1, VK_PIPELINE_STAGE_2_TRANSFER_BIT);
    queue_task = task.Submit();
  }

  // Graphics queue: acquire index buffer
  {
    gpu::GraphicsTask task;
    auto cb = task.command_buffer();

    gpu::cmd::Barrier()
        .Acquire(VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT, VK_ACCESS_2_INDEX_READ_BIT, tq, gq, index_buffer)
        .Commit(cb);

    task.Wait(sem, sem->value() + 1, VK_PIPELINE_STAGE_2_TRANSFER_BIT);
  }

  sem->Increment();

  return std::make_shared<GaussianSplats>(point_count, sh_degree, position, cov3d, sh, opacity, index_buffer,
                                          queue_task);
}

}  // namespace core
}  // namespace vkgs
