#pragma once

#include "vk_types.hpp"
#include "vkinit.hpp"
#include <vulkan/vulkan_core.h>
namespace vk_util {
inline void transition_image(VkCommandBuffer cmd, VkImage img,
                             VkImageLayout old, VkImageLayout next) {

    VkImageAspectFlags aspectMask =
        (next == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
            ? VK_IMAGE_ASPECT_DEPTH_BIT
            : VK_IMAGE_ASPECT_COLOR_BIT;

    auto imgBarrier = VkImageMemoryBarrier2{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext = nullptr,

        .srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        .srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,

        .dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        .dstAccessMask =
            VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT,

        .oldLayout = old,
        .newLayout = next,

        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,

        .image = img,
        .subresourceRange = vk_init::image_subresource_range(aspectMask)};
    auto depInfo = VkDependencyInfo{.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                                    .pNext = nullptr,
                                    .imageMemoryBarrierCount = 1,
                                    .pImageMemoryBarriers = &imgBarrier};
    vkCmdPipelineBarrier2(cmd, &depInfo);
}

} // namespace vk_util
