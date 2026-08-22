#pragma once
#include "vk_types.hpp"
struct SwapchainSettings {
    vk::raii::PhysicalDevice const& physical_device;
    vk::raii::Device const& device;
    vk::raii::SurfaceKHR const& surface;
    vk::Extent2D extents{};
    vk::Format format{vk::Format::eB8G8R8A8Unorm};
    vk::ColorSpaceKHR colorSpace{vk::ColorSpaceKHR::eSrgbNonlinear};
    vk::PresentModeKHR present_mode{vk::PresentModeKHR::eFifo};
    vk::ImageUsageFlags image_usage_flags{vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eColorAttachment};
};

struct Swapchain {
    vk::raii::SwapchainKHR descriptor{nullptr};
    std::vector<vk::raii::Semaphore> renderSemaphores{};
    vk::Format imageFormat{};
    vk::Extent2D extent{};
    std::vector<vk::Image> images;
    std::vector<vk::raii::ImageView> imageViews{};
};

// clang-format on
Swapchain make_swapchain(SwapchainSettings const& s);
