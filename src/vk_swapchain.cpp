#include <algorithm>

#include "vk_swapchain.hpp"
#include "vk_types.hpp"

#include "logger.hpp"
#include "types.hpp"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_raii.hpp"
static constexpr auto UNDEFINED_EXTENT {numeric_max<u32>};
Swapchain make_swapchain(SwapchainSettings const& s) {
    auto fmts = s.physical_device.getSurfaceFormatsKHR(s.surface);
    ASSERT(fmts.size() > 0);

    auto preferred = vk::SurfaceFormatKHR{s.format,s.colorSpace};
    auto surface_supports_fmt = std::ranges::contains(fmts, preferred) ;
    auto image_fmt = surface_supports_fmt
        ? *(std::ranges::find(fmts,preferred))
        : fmts.front();
    if (!surface_supports_fmt){
        LOG_WARN("Surface does not support desired format ({}/{}), falling back to {}/{}.",
                 vk::to_string(s.format),vk::to_string(s.format),
                 vk::to_string(image_fmt.format),vk::to_string(image_fmt.format)
                 );
    }

    auto modes = s.physical_device.getSurfacePresentModesKHR(s.surface);
    bool surface_supports_present_mode = std::ranges::contains(modes,s.present_mode);
    auto present_mode = surface_supports_present_mode
            ? s.present_mode
            : vk::PresentModeKHR::eFifo; // only one guaranteed by the spec
    if (!surface_supports_present_mode){
        LOG_WARN("Surface does not support desired present mode({}), falling back to {}." ,
                 vk::to_string(s.present_mode),
                 vk::to_string(vk::PresentModeKHR::eFifo));
    }

    auto caps = s.physical_device.getSurfaceCapabilitiesKHR(s.surface);
    auto extent = caps.currentExtent;
    if (extent.width == UNDEFINED_EXTENT){
        // clamp the extent to the capabilities
        auto const max = caps.maxImageExtent;
        auto const min = caps.minImageExtent;
        extent.width = std::clamp(extent.width, min.width, max.width);
        extent.height = std::clamp(extent.height, min.height, max.height);
    }
    auto image_count = caps.minImageCount+1;
    if (caps.maxImageCount > 0) { // 0 means "no upper bound" apparently
        image_count = std::min(image_count, caps.maxImageCount);
    }
    // next is checking usage flags
    if ((caps.supportedUsageFlags & s.image_usage_flags) != s.image_usage_flags){
        LOG_FATAL("Surface does not support desired image usage {} (supports {})",
                  vk::to_string(s.image_usage_flags),
                  vk::to_string(caps.supportedUsageFlags)
                  );
    }
    auto swapchain = Swapchain{
        .descriptor = vk::raii::SwapchainKHR{s.device,
            vk::SwapchainCreateInfoKHR{
                .surface = *s.surface,
                .minImageCount = image_count,
                .imageFormat = image_fmt.format,
                .imageColorSpace = image_fmt.colorSpace,
                .imageExtent = extent,
                .imageArrayLayers = 1,
                .imageUsage = s.image_usage_flags,
                .imageSharingMode = vk::SharingMode::eExclusive,
                .preTransform = caps.currentTransform,
                .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
                .presentMode = present_mode,
                .clipped = vk::True,
            },
        },
        .imageFormat = image_fmt.format,
        .extent = extent,
    };
//////////////////VK_IMAGE_USAGE_2_TRANSFER_DST_BIT_KHR
//    vk::ImageUsageFlagBits2KHR::eTransferDst;
    swapchain.images = swapchain.descriptor.getImages();
    ASSERT(swapchain.images.size() > 0);

    swapchain.imageViews.reserve(swapchain.images.size());
    for (auto const& image: swapchain.images){
        swapchain.imageViews.emplace_back(
            s.device,
            vk::ImageViewCreateInfo{
                .image = image,
                .viewType = vk::ImageViewType::e2D,
                .format = swapchain.imageFormat,
                .subresourceRange = {
                    .aspectMask = vk::ImageAspectFlagBits::eColor,
                    .levelCount = 1,
                    .layerCount = 1,
                },
            }
        );
        swapchain.renderSemaphores.emplace_back(
            s.device,
            vk::SemaphoreCreateInfo{}
        );
    }
    return swapchain;
};
//    auto vkb_swapchain =
//        vkb::SwapchainBuilder{s.physical_device, s.device, s.surface}
//            .set_desired_format({
//                .format = s.format,
//                .colorSpace = s.colorSpace,
//            })
//            .set_desired_present_mode(s.present_mode)
//            .set_desired_extent(s.extents.width, s.extents.height)
//            .add_image_usage_flags(s.image_usage_flags)
//            .build()
//            .value();
//    if (!vkb_swapchain)
//        LOG_FATAL("Failed to build swapchain.");
//
//    descriptor = vkb_swapchain.swapchain;
//    imageFormat = vkb_swapchain.image_format;
//    images = vkb_swapchain.get_images().value();
//    imageViews = vkb_swapchain.get_image_views().value();
//    extent = vkb_swapchain.extent;
//    ASSERT(images.size() > 0);
