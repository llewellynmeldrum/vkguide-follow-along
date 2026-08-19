#pragma once 
#include "Types.hpp"
#include "vk_types.hpp"
#include <vulkan/vulkan_core.h>
namespace vk_init{

inline constexpr auto command_pool_create_info(u32 family, VkCommandPoolCreateFlags flags=0){
    return VkCommandPoolCreateInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        // we expect to be able to reset individual command buffers from this pool
        .flags = flags,
        .queueFamilyIndex = family,
    };
}
inline constexpr auto command_buffer_alloc_info(VkCommandPool pool, u32 count=1){
    return VkCommandBufferAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = count
    };
}

inline constexpr auto fence_create_info(VkFenceCreateFlags flags=0){
    return VkFenceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags,
    };
}
inline constexpr auto semaphore_create_info(VkSemaphoreCreateFlags flags=0){
    return VkSemaphoreCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags,
    };
}

inline constexpr auto command_buffer_begin_info(VkCommandBufferUsageFlags flags=0){
    return VkCommandBufferBeginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = flags,
        .pInheritanceInfo = nullptr,
    };
}

inline constexpr auto image_subresource_range(VkImageAspectFlags aspectMask=0){
    return VkImageSubresourceRange{
        .aspectMask = aspectMask,
        .baseMipLevel =0, 
        .levelCount = VK_REMAINING_MIP_LEVELS,
        .baseArrayLayer = 0,
        .layerCount = VK_REMAINING_ARRAY_LAYERS,
    };
}

inline constexpr auto semaphore_submit_info(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore){
    return VkSemaphoreSubmitInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .pNext = nullptr,
        .semaphore = semaphore,
        .value = 1,
        .stageMask = stageMask, 
        .deviceIndex = 0,
    };
}

inline constexpr auto command_buffer_submit_info(VkCommandBuffer cmdBuf){
    return VkCommandBufferSubmitInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .pNext = nullptr,
        .commandBuffer = cmdBuf,
        .deviceMask = 0,
    };
}

inline constexpr auto submit_info(VkCommandBufferSubmitInfo* cmdBufInfo, VkSemaphoreSubmitInfo* signalSemInfo, VkSemaphoreSubmitInfo* waitSemInfo){
    return VkSubmitInfo2{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .pNext = nullptr,

        .waitSemaphoreInfoCount = waitSemInfo ? 1U : 0U,
        .pWaitSemaphoreInfos = waitSemInfo, 

        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = cmdBufInfo, 

        .signalSemaphoreInfoCount = signalSemInfo ? 1U : 0U,
        .pSignalSemaphoreInfos = signalSemInfo,
    };
}


} // namespace vk_init
