#include "vkgs/core/module.h"

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <sstream>

#include "vkgs/core/gaussian_splats.h"
#include "vkgs/core/rendered_image.h"

#include "generated/parse_ply.h"
#include "generated/splat_vert.h"
#include "generated/splat_frag.h"
#include "buffer.h"
#include "image.h"
#include "device.h"
#include "sorter.h"
#include "buffer.h"
#include "semaphore.h"
#include "fence.h"
#include "queue.h"
#include "command.h"
#include "task_monitor.h"
#include "pipeline_layout.h"
#include "compute_pipeline.h"
#include "graphics_pipeline.h"

namespace {

auto WorkgroupSize(size_t count, uint32_t local_size) { return (count + local_size - 1) / local_size; }

void cmdPushDescriptorSet(VkCommandBuffer cb, VkPipelineBindPoint bind_point, VkPipelineLayout pipeline_layout,
                          const std::vector<VkBuffer>& buffers) {
  std::vector<VkDescriptorBufferInfo> buffer_infos(buffers.size());
  std::vector<VkWriteDescriptorSet> writes(buffers.size());
  for (int i = 0; i < buffers.size(); ++i) {
    buffer_infos[i] = {buffers[i], 0, VK_WHOLE_SIZE};
    writes[i] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    writes[i].dstBinding = i;
    writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[i].descriptorCount = 1;
    writes[i].pBufferInfo = &buffer_infos[i];
  }
  vkCmdPushDescriptorSet(cb, bind_point, pipeline_layout, 0, writes.size(), writes.data());
}

}  // namespace

namespace vkgs {
namespace core {

Module::Module() {
  device_ = std::make_shared<Device>();
  task_monitor_ = std::make_shared<TaskMonitor>();
  sorter_ = std::make_shared<Sorter>(*device_, device_->physical_device(), device_->allocator());

  parse_ply_pipeline_layout_ =
      PipelineLayout::Create(*device_,
                             {
                                 {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
                                 {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
                                 {2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
                                 {3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
                                 {4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
                             },
                             {{VK_SHADER_STAGE_COMPUTE_BIT, 0, 4}});

  parse_ply_pipeline_ = ComputePipeline::Create(*device_, *parse_ply_pipeline_layout_, parse_ply);

  splat_pipeline_layout_ =
      PipelineLayout::Create(*device_, {{0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT}});

  splat_pipeline_ =
      GraphicsPipeline::Create(*device_, *splat_pipeline_layout_, splat_vert, splat_frag, VK_FORMAT_R8G8B8A8_UNORM);
}

Module::~Module() = default;

const std::string& Module::device_name() const noexcept { return device_->device_name(); }
uint32_t Module::graphics_queue_index() const noexcept { return device_->graphics_queue_index(); }
uint32_t Module::compute_queue_index() const noexcept { return device_->compute_queue_index(); }
uint32_t Module::transfer_queue_index() const noexcept { return device_->transfer_queue_index(); }

std::shared_ptr<GaussianSplats> Module::load_from_ply(const std::string& path) {
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
  for (int i = 0; i < 15; ++i) {
    ply_offsets[10 + 1 + i] = offsets["f_rest_" + std::to_string(i)] / 4;
    ply_offsets[10 + 17 + i] = offsets["f_rest_" + std::to_string(15 + i)] / 4;
    ply_offsets[10 + 33 + i] = offsets["f_rest_" + std::to_string(30 + i)] / 4;
  }
  ply_offsets[58] = offsets["opacity"] / 4;
  ply_offsets[59] = offset / 4;

  std::cout << "offset: " << offset << std::endl;
  std::cout << "point_count: " << point_count << std::endl;

  std::vector<char> buffer(offset * point_count);
  in.read(buffer.data(), buffer.size());

  // allocate buffers
  auto buffer_size = buffer.size() + 60 * sizeof(uint32_t);
  auto ply_stage =
      Buffer::Create(device_, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, buffer_size, true);
  auto ply_buffer =
      Buffer::Create(device_, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, buffer_size);

  auto position = Buffer::Create(device_, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, point_count * 3 * sizeof(float));
  auto cov3d = Buffer::Create(device_, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, point_count * 6 * sizeof(float));
  auto sh = Buffer::Create(device_, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, point_count * 48 * 2);
  auto opacity = Buffer::Create(device_, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, point_count * sizeof(float));

  std::memcpy(ply_stage->data(), ply_offsets.data(), ply_offsets.size() * sizeof(uint32_t));
  std::memcpy(ply_stage->data<char>() + ply_offsets.size() * sizeof(uint32_t), buffer.data(), buffer.size());

  auto sem = device_->AllocateSemaphore();
  {
    auto command0 = device_->transfer_queue()->AllocateCommandBuffer();
    auto cb0 = command0->command_buffer();
    auto fence0 = device_->AllocateFence();

    VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cb0, &begin_info);

    VkBufferCopy region;
    region.srcOffset = 0;
    region.dstOffset = 0;
    region.size = buffer_size;
    vkCmdCopyBuffer(cb0, *ply_stage, *ply_buffer, 1, &region);

    // Release barrier
    VkBufferMemoryBarrier2 release_barrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
    release_barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    release_barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    release_barrier.srcQueueFamilyIndex = device_->transfer_queue()->family_index();
    release_barrier.dstQueueFamilyIndex = device_->compute_queue()->family_index();
    release_barrier.buffer = *ply_buffer;
    release_barrier.offset = 0;
    release_barrier.size = VK_WHOLE_SIZE;
    VkDependencyInfo release_dependency_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    release_dependency_info.bufferMemoryBarrierCount = 1;
    release_dependency_info.pBufferMemoryBarriers = &release_barrier;
    vkCmdPipelineBarrier2(cb0, &release_dependency_info);

    vkEndCommandBuffer(cb0);

    VkCommandBufferSubmitInfo command_buffer_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
    command_buffer_info.commandBuffer = cb0;

    VkSemaphoreSubmitInfo signal_semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
    signal_semaphore_info.semaphore = sem->semaphore();
    signal_semaphore_info.value = sem->value() + 1;
    signal_semaphore_info.stageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;

    VkSubmitInfo2 submit = {VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
    submit.commandBufferInfoCount = 1;
    submit.pCommandBufferInfos = &command_buffer_info;
    submit.signalSemaphoreInfoCount = 1;
    submit.pSignalSemaphoreInfos = &signal_semaphore_info;

    vkQueueSubmit2(device_->transfer_queue()->queue(), 1, &submit, fence0->fence());
    task_monitor_->Add(fence0, {command0, sem, ply_stage, ply_buffer});
  }

  {
    auto command1 = device_->compute_queue()->AllocateCommandBuffer();
    auto cb1 = command1->command_buffer();
    auto fence1 = device_->AllocateFence();

    VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cb1, &begin_info);

    // Acquire barrier
    VkBufferMemoryBarrier2 acquire_barrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
    acquire_barrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    acquire_barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
    acquire_barrier.srcQueueFamilyIndex = device_->transfer_queue()->family_index();
    acquire_barrier.dstQueueFamilyIndex = device_->compute_queue()->family_index();
    acquire_barrier.buffer = *ply_buffer;
    acquire_barrier.offset = 0;
    acquire_barrier.size = VK_WHOLE_SIZE;
    VkDependencyInfo acquire_dependency_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    acquire_dependency_info.bufferMemoryBarrierCount = 1;
    acquire_dependency_info.pBufferMemoryBarriers = &acquire_barrier;
    vkCmdPipelineBarrier2(cb1, &acquire_dependency_info);

    // ply_buffer -> gaussian_splats
    vkCmdPushConstants(cb1, *parse_ply_pipeline_layout_, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(point_count),
                       &point_count);
    cmdPushDescriptorSet(cb1, VK_PIPELINE_BIND_POINT_COMPUTE, *parse_ply_pipeline_layout_,
                         {*ply_buffer, *position, *cov3d, *opacity, *sh});

    vkCmdBindPipeline(cb1, VK_PIPELINE_BIND_POINT_COMPUTE, *parse_ply_pipeline_);
    vkCmdDispatch(cb1, WorkgroupSize(point_count, 256), 1, 1);

    // Visibility barrier
    VkMemoryBarrier2 visibility_barrier = {VK_STRUCTURE_TYPE_MEMORY_BARRIER_2};
    visibility_barrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    visibility_barrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
    visibility_barrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    visibility_barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
    VkDependencyInfo visibility_dependency_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    visibility_dependency_info.memoryBarrierCount = 1;
    visibility_dependency_info.pMemoryBarriers = &visibility_barrier;
    vkCmdPipelineBarrier2(cb1, &visibility_dependency_info);

    vkEndCommandBuffer(cb1);

    // Acquire
    VkSemaphoreSubmitInfo wait_semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
    wait_semaphore_info.semaphore = sem->semaphore();
    wait_semaphore_info.value = sem->value() + 1;
    wait_semaphore_info.stageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;

    VkCommandBufferSubmitInfo command_buffer_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
    command_buffer_info.commandBuffer = cb1;

    VkSubmitInfo2 submit = {VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
    submit.waitSemaphoreInfoCount = 1;
    submit.pWaitSemaphoreInfos = &wait_semaphore_info;
    submit.commandBufferInfoCount = 1;
    submit.pCommandBufferInfos = &command_buffer_info;

    vkQueueSubmit2(device_->compute_queue()->queue(), 1, &submit, fence1->fence());
    task_monitor_->Add(fence1, {command1, sem, parse_ply_pipeline_, ply_buffer, position, cov3d, sh, opacity});
  }

  sem->Increment();

  return std::make_shared<GaussianSplats>(point_count, position, cov3d, sh, opacity);
}

std::shared_ptr<RenderedImage> Module::draw(std::shared_ptr<GaussianSplats> splats) {
  constexpr uint32_t width = 1024;
  constexpr uint32_t height = 1024;

  auto image = Image::Create(device_, VK_FORMAT_R8G8B8A8_UNORM, width, height,
                             VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
  auto sem = device_->AllocateSemaphore();

  // Graphics queue
  {
    // TODO: calculate instances in compute queue
    float s = 0.2f;
    std::vector<float> instances_data = {
        0.0f, 0.0f, 0.f, 1.f, s, 0.f, 0.f, s, 1.f, 0.f, 0.f, 1.f,  //
        0.0f, 0.5f, 0.f, 1.f, s, 0.f, 0.f, s, 0.f, 1.f, 0.f, 1.f,  //
        0.5f, 0.0f, 0.f, 1.f, s, 0.f, 0.f, s, 0.f, 0.f, 1.f, 1.f,  //
    };
    auto instances_stage =
        Buffer::Create(device_, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, instances_data.size() * sizeof(float), true);
    auto instances = Buffer::Create(device_, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                    instances_data.size() * sizeof(float));

    std::vector<uint32_t> index_data = {
        0, 1, 2,  2,  1, 3,   //
        4, 5, 6,  6,  5, 7,   //
        8, 9, 10, 10, 9, 11,  //
    };
    auto index_stage =
        Buffer::Create(device_, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, index_data.size() * sizeof(uint32_t), true);
    auto index = Buffer::Create(device_, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                index_data.size() * sizeof(uint32_t));

    std::memcpy(instances_stage->data(), instances_data.data(), instances_data.size() * sizeof(float));
    std::memcpy(index_stage->data(), index_data.data(), index_data.size() * sizeof(uint32_t));

    auto fence = device_->AllocateFence();
    auto command = device_->graphics_queue()->AllocateCommandBuffer();
    auto cb = command->command_buffer();

    VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cb, &begin_info);

    VkBufferCopy region = {0, 0, instances_data.size() * sizeof(float)};
    vkCmdCopyBuffer(cb, *instances_stage, *instances, 1, &region);

    region = {0, 0, index_data.size() * sizeof(uint32_t)};
    vkCmdCopyBuffer(cb, *index_stage, *index, 1, &region);

    VkMemoryBarrier2 memory_barrier = {VK_STRUCTURE_TYPE_MEMORY_BARRIER_2};
    memory_barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    memory_barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    memory_barrier.dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT;
    memory_barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_INDEX_READ_BIT;
    VkDependencyInfo dependency_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependency_info.memoryBarrierCount = 1;
    dependency_info.pMemoryBarriers = &memory_barrier;
    vkCmdPipelineBarrier2(cb, &dependency_info);

    // Layout transition to color attachment
    VkImageMemoryBarrier2 image_memory_barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    image_memory_barrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    image_memory_barrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    image_memory_barrier.image = *image;
    image_memory_barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    dependency_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependency_info.imageMemoryBarrierCount = 1;
    dependency_info.pImageMemoryBarriers = &image_memory_barrier;
    vkCmdPipelineBarrier2(cb, &dependency_info);

    // Rendering
    VkRenderingAttachmentInfo color_attachment = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    color_attachment.imageView = image->image_view();
    color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.clearValue.color = {0.f, 0.f, 0.f, 1.f};
    VkRenderingInfo rendering_info = {VK_STRUCTURE_TYPE_RENDERING_INFO};
    rendering_info.renderArea.offset = {0, 0};
    rendering_info.renderArea.extent = {width, height};
    rendering_info.layerCount = 1;
    rendering_info.colorAttachmentCount = 1;
    rendering_info.pColorAttachments = &color_attachment;
    vkCmdBeginRendering(cb, &rendering_info);

    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, *splat_pipeline_);
    cmdPushDescriptorSet(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, *splat_pipeline_layout_, {*instances});

    VkViewport viewport = {0.f, 0.f, static_cast<float>(width), static_cast<float>(height), 0.f, 1.f};
    vkCmdSetViewport(cb, 0, 1, &viewport);
    VkRect2D scissor = {0, 0, width, height};
    vkCmdSetScissor(cb, 0, 1, &scissor);

    vkCmdBindIndexBuffer(cb, *index, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(cb, index_data.size(), 1, 0, 0, 0);

    vkCmdEndRendering(cb);

    // Layout transition to transfer src, and release
    image_memory_barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    image_memory_barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    image_memory_barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    image_memory_barrier.srcQueueFamilyIndex = device_->graphics_queue()->family_index();
    image_memory_barrier.dstQueueFamilyIndex = device_->transfer_queue()->family_index();
    image_memory_barrier.image = *image;
    image_memory_barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    dependency_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependency_info.imageMemoryBarrierCount = 1;
    dependency_info.pImageMemoryBarriers = &image_memory_barrier;
    vkCmdPipelineBarrier2(cb, &dependency_info);

    vkEndCommandBuffer(cb);

    // Submit
    VkCommandBufferSubmitInfo command_buffer_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
    command_buffer_info.commandBuffer = cb;

    VkSemaphoreSubmitInfo signal_semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
    signal_semaphore_info.semaphore = sem->semaphore();
    signal_semaphore_info.value = sem->value() + 1;
    signal_semaphore_info.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo2 submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
    submit_info.commandBufferInfoCount = 1;
    submit_info.pCommandBufferInfos = &command_buffer_info;
    submit_info.signalSemaphoreInfoCount = 1;
    submit_info.pSignalSemaphoreInfos = &signal_semaphore_info;

    vkQueueSubmit2(device_->graphics_queue()->queue(), 1, &submit_info, fence->fence());
    task_monitor_->Add(fence, {command, image, instances_stage, instances, index_stage, index, sem});
  }

  auto buffer = Buffer::Create(device_, VK_BUFFER_USAGE_TRANSFER_DST_BIT, width * height * 4, true);
  std::vector<uint8_t> image_buffer(width * height * 4);
  {
    auto fence = device_->AllocateFence();
    auto command = device_->transfer_queue()->AllocateCommandBuffer();
    auto cb = command->command_buffer();

    VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cb, &begin_info);

    // Acquire
    VkImageMemoryBarrier2 image_memory_barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    image_memory_barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    image_memory_barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
    image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    image_memory_barrier.srcQueueFamilyIndex = device_->graphics_queue()->family_index();
    image_memory_barrier.dstQueueFamilyIndex = device_->transfer_queue()->family_index();
    image_memory_barrier.image = *image;
    image_memory_barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    VkDependencyInfo dependency_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependency_info.imageMemoryBarrierCount = 1;
    dependency_info.pImageMemoryBarriers = &image_memory_barrier;
    vkCmdPipelineBarrier2(cb, &dependency_info);

    // Image to buffer
    VkBufferImageCopy region;
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};
    vkCmdCopyImageToBuffer(cb, *image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *buffer, 1, &region);

    vkEndCommandBuffer(cb);

    // Submit
    VkCommandBufferSubmitInfo command_buffer_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
    command_buffer_info.commandBuffer = cb;

    VkSemaphoreSubmitInfo wait_semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
    wait_semaphore_info.semaphore = sem->semaphore();
    wait_semaphore_info.value = sem->value() + 1;
    wait_semaphore_info.stageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;

    VkSubmitInfo2 submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
    submit_info.waitSemaphoreInfoCount = 1;
    submit_info.pWaitSemaphoreInfos = &wait_semaphore_info;
    submit_info.commandBufferInfoCount = 1;
    submit_info.pCommandBufferInfos = &command_buffer_info;

    vkQueueSubmit2(device_->transfer_queue()->queue(), 1, &submit_info, fence->fence());
    task_monitor_->Add(fence, {command, image, buffer, sem});

    fence->Wait();

    // TODO: wait for transfer
    std::memcpy(image_buffer.data(), buffer->data(), buffer->size());
  }

  sem->Increment();

  return std::make_shared<RenderedImage>(width, height, std::move(image_buffer));
}

}  // namespace core
}  // namespace vkgs
