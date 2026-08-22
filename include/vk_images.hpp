#pragma once

#include "vk_types.hpp"
#include "vkinit.hpp"
#include "vulkan/vulkan.hpp"
#include <vulkan/vulkan_core.h>
namespace vk_util {inline constexpr auto image_subresource_range(vk::ImageAspectFlags aspect) {
    return vk::ImageSubresourceRange{
        .aspectMask = aspect,
        .baseMipLevel = 0,
        .levelCount = vk::RemainingMipLevels,
        .baseArrayLayer = 0,
        .layerCount = vk::RemainingArrayLayers,
    };
}
inline void transition_image(vk::raii::CommandBuffer const& cmd, vk::Image img,
                             vk::ImageLayout old, vk::ImageLayout next) {

    auto dst_access_mask = vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead;
    if (next == vk::ImageLayout::ePresentSrcKHR){
        dst_access_mask = vk::AccessFlagBits2::eNone;
    }
    auto aspectMask =
        (next == vk::ImageLayout::eDepthAttachmentOptimal)
            ? vk::ImageAspectFlagBits::eDepth 
            : vk::ImageAspectFlagBits::eColor;

    auto imgBarrier = vk::ImageMemoryBarrier2KHR{
        .srcStageMask = vk::PipelineStageFlagBits2::eAllCommands,
        .srcAccessMask = vk::AccessFlagBits2::eMemoryWrite,

        .dstStageMask = vk::PipelineStageFlagBits2::eAllCommands,
        .dstAccessMask = dst_access_mask,
           

        .oldLayout = old,
        .newLayout = next,

        .srcQueueFamilyIndex = vk::QueueFamilyIgnored, 
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored, 

        .image = img,
        .subresourceRange = vk_util::image_subresource_range(aspectMask)
    };

    cmd.pipelineBarrier2(
        vk::DependencyInfo{
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &imgBarrier
        }
    );

}

} // namespace vk_util
