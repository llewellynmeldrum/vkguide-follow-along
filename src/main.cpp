
#include <format>
#include <print>
#include <thread>
#include <type_traits>

#include "SDL3/SDL_video.h"
#include "Types.hpp"
#include "VkBootstrap.h"
#include "logger.hpp"
#include "shared.hpp"
#include "vk_images.hpp"
#include "vk_types.hpp"
#include "vkinit.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <cpptrace/utils.hpp>
#include <vulkan/vulkan_core.h>

#include "Timer.hpp"

const char* APP_NAME = "Test Window";

//*******************************************************************************************
// swapchain.hpp
struct Swapchain {
    VkSwapchainKHR descriptor;
    VkFormat imageFormat;
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    VkExtent2D extent;
};
struct SwapchainSettings {
    VkPhysicalDevice physical_device{};
    VkDevice device{};
    VkSurfaceKHR surface{};
    VkExtent2D extents{};
    VkFormat format{VK_FORMAT_B8G8R8A8_UNORM};
    VkColorSpaceKHR colorSpace{VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    VkPresentModeKHR present_mode{VK_PRESENT_MODE_FIFO_KHR};
    VkImageUsageFlagBits image_usage_flags{VK_IMAGE_USAGE_TRANSFER_DST_BIT};
};

constexpr inline auto make_swapchain(SwapchainSettings const& s) {
    auto vkb_swapchain =
        vkb::SwapchainBuilder{s.physical_device, s.device, s.surface}
            .set_desired_format({
                .format = s.format,
                .colorSpace = s.colorSpace,
            })
            .set_desired_present_mode(s.present_mode)
            .set_desired_extent(s.extents.width, s.extents.height)
            .add_image_usage_flags(s.image_usage_flags)
            .build()
            .value();
    if (!vkb_swapchain)
        LOG_FATAL("Failed to build swapchain.");

    auto sw = Swapchain{
        .descriptor = vkb_swapchain.swapchain,
        .imageFormat = vkb_swapchain.image_format,
        .images = vkb_swapchain.get_images().value(),
        .imageViews = vkb_swapchain.get_image_views().value(),
        .extent = vkb_swapchain.extent,
    };
    std::println("{}", static_cast<void const*>(sw.descriptor));
    std::println("{}", static_cast<void const*>(sw.images.data()));
    ASSERT(sw.images.size() > 0);
    return sw;
}
//*******************************************************************************************

struct FrameData {
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    // sync:
    VkFence fence;
    VkSemaphore swapchainSemaphore;
    VkSemaphore renderSemaphore;
};

FWD_DECL_STRUCT(SDL_Window);
struct VkEngine {
  public:
    template <typename T, size_t S> using array = std::array<T, S>;
    static constexpr u32 N_FSYNC = 2;
    VkEngine() { init(); }
    ~VkEngine() { cleanup(); }

    VkInstance m_vkInstance{};
    VkExtent2D m_windowExtent{800, 600};
    VkDebugUtilsMessengerEXT m_vkDebugMessenger{};
    VkPhysicalDevice m_vkPhysicalDevice{};
    VkDevice m_vkDevice{};
    VkSurfaceKHR m_vkSurface{};

    Swapchain m_Swapchain{};

    VkEngine* m_loadedEngine{};
    size_t m_frameCount{};
    bool m_shouldStopRendering{};
    SDL_Window* m_window{};

    array<FrameData, N_FSYNC> m_inflightFrames{};
    VkQueue m_vkQueue{};
    u32 m_vkQueueFamily{};

    FrameData& get_current_frame();

    constexpr static bool m_useValidationLayers{true};

    VkEngine& get();

    bool is_initialized();

    void init();
    void cleanup();

    void run();
    void draw();

  private:
    void init_window();
    void init_vulkan();
    void init_swapchain();
    void init_commands();
    void init_sync_structures();

    void cleanup_swapchain();
};

void VkEngine::init_window() {
    SDL_Init(SDL_INIT_VIDEO);
    m_window = SDL_CreateWindow(APP_NAME, m_windowExtent.width,
                                m_windowExtent.height, SDL_WINDOW_VULKAN);
    if (!m_window) {
        LOG_ERROR("Failed to init window");
        LOG_EXIT(1);
    }
}

void VkEngine::init_vulkan() {

    // 1. query SDL for the extensions of this instance
    u32 ext_count{};
    auto extensions = SDL_Vulkan_GetInstanceExtensions(&ext_count);

    // 2. Create the instance+debug messenger with a vkb::InstanceBuilder
    auto vkb_builder_ret = vkb::InstanceBuilder{}
                               .set_app_name(APP_NAME)
                               .request_validation_layers(m_useValidationLayers)
                               .enable_extensions(ext_count, extensions)
                               .require_api_version(1, 3, 0)
                               .build();
    if (!vkb_builder_ret)
        LOG_FATAL("Failed to build vulkan instance.");
    auto vkb_instance = vkb_builder_ret.value();

    m_vkInstance = vkb_instance.instance;
    m_vkDebugMessenger = vkb_instance.debug_messenger;

    // 3. Create the VkSurfaceKHR to render to
    assert(m_window);
    if (!SDL_Vulkan_CreateSurface(m_window, m_vkInstance, nullptr,
                                  &m_vkSurface)) {
        LOG_ERROR("Failure in {}(): {}", "SDL_Vulkan_CreateSurface",
                  SDL_GetError());
        LOG_EXIT(1);
    }

    // 4. Select a physical device with certain capabilities
    auto vkb_physical_device_res =
        vkb::PhysicalDeviceSelector{vkb_instance}
            .set_minimum_version(1, 3)
            .set_required_features_12({
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
                .descriptorIndexing = true,
                .bufferDeviceAddress = true,
            })
            .set_required_features_13({
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
                .synchronization2 = true,
                .dynamicRendering = true,
            })
            .set_surface(m_vkSurface)
            .select();
    if (!vkb_physical_device_res)
        LOG_FATAL("Failed to select physical device.");
    auto vkb_physical_device = vkb_physical_device_res.value();
    m_vkPhysicalDevice = vkb_physical_device.physical_device;

    // 5. Create the logical device from the physical one
    auto vkb_device_res = vkb::DeviceBuilder{vkb_physical_device}.build();
    if (!vkb_device_res)
        LOG_FATAL("Failed to select physical device.");
    auto vkb_device = vkb_device_res.value();

    m_vkDevice = vkb_device.device;

    m_vkQueue = vkb_device.get_queue(vkb::QueueType::graphics).value();
    m_vkQueueFamily =
        vkb_device.get_queue_index(vkb::QueueType::graphics).value();
}

void VkEngine::init_swapchain() {
    m_Swapchain = make_swapchain(SwapchainSettings{
        .physical_device = m_vkPhysicalDevice,
        .device = m_vkDevice,
        .surface = m_vkSurface,
        .extents = m_windowExtent,
    });
}

void VkEngine::cleanup_swapchain() {
    for (auto image_view : m_Swapchain.imageViews) {
        vkDestroyImageView(m_vkDevice, image_view, nullptr);
    }
    vkDestroySwapchainKHR(m_vkDevice, m_Swapchain.descriptor, nullptr);
}

void VkEngine::init_commands() {
    auto cmdPoolInfo = vk_init::command_pool_create_info(
        m_vkQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    for (auto& frame : m_inflightFrames) {
        vk_check(vkCreateCommandPool(m_vkDevice, &cmdPoolInfo, nullptr,
                                     &frame.commandPool));

        auto allocInfo =
            vk_init::command_buffer_alloc_info(frame.commandPool, 1);

        vk_check(vkAllocateCommandBuffers(m_vkDevice, &allocInfo,
                                          &frame.commandBuffer));
    }
}

void VkEngine::init_sync_structures() {
    auto fenceCreateInfo =
        vk_init::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
    auto semaphoreCreateInfo = vk_init::semaphore_create_info();
    for (auto& frame : m_inflightFrames) {
        vk_check(
            vkCreateFence(m_vkDevice, &fenceCreateInfo, nullptr, &frame.fence));
        vk_check(vkCreateSemaphore(m_vkDevice, &semaphoreCreateInfo, nullptr,
                                   &frame.renderSemaphore));
        vk_check(vkCreateSemaphore(m_vkDevice, &semaphoreCreateInfo, nullptr,
                                   &frame.swapchainSemaphore));
    }
}

void VkEngine::init() {
    LOG_INFO("INITIALIZING ENGINE ({})", static_cast<void*>(this));
    ASSERT(m_loadedEngine == nullptr);
    m_loadedEngine = this;
    init_window();
    init_vulkan();
    init_swapchain();
    init_commands();
    init_sync_structures();
}
template <typename T>
    requires std::floating_point<T> || std::integral<T>
u64 stons(T sec) {
    return static_cast<u64>(sec * 1'000'000'000);
}
void VkEngine::draw() {
    auto& curFrame = get_current_frame();
    vk_check(vkWaitForFences(m_vkDevice, 1, &curFrame.fence, true, stons(1)));
    vk_check(vkResetFences(m_vkDevice, 1, &curFrame.fence));

    // request image from the swapchain
    // If the swapchain has no image ready, it will just block the thread
    // for `timeout`
    u32 swapchainImageIdx{};
    vk_check(vkAcquireNextImageKHR(m_vkDevice, m_Swapchain.descriptor, stons(1),
                                   curFrame.swapchainSemaphore, VK_NULL_HANDLE,
                                   &swapchainImageIdx));
    auto cmd_buf = curFrame.commandBuffer;
    vk_check(vkResetCommandBuffer(cmd_buf, 0));

    auto commandBufferBeginInfo = vk_init::command_buffer_begin_info(
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    vk_check(vkBeginCommandBuffer(cmd_buf, &commandBufferBeginInfo));

    auto swap_img = m_Swapchain.images.at(swapchainImageIdx);
    vk_util::transition_image(cmd_buf, swap_img, VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_GENERAL);

    auto clearColor = VkClearColorValue{{
        std::abs(std::sin(m_frameCount / 120.0f)),
        0.0f,
        std::abs(std::sin(m_frameCount / 120.0f)),
        1.0f,
    }};

    auto clearRange =
        vk_init::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);

    vkCmdClearColorImage(cmd_buf, swap_img, VK_IMAGE_LAYOUT_GENERAL,
                         &clearColor, 1, &clearRange);

    vk_util::transition_image(cmd_buf, swap_img, VK_IMAGE_LAYOUT_GENERAL,
                              VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    vk_check(vkEndCommandBuffer(cmd_buf));

    auto submit_info = vk_init::command_buffer_submit_info(cmd_buf);
    auto wait_info = vk_init::semaphore_submit_info(
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
        curFrame.swapchainSemaphore);
    auto signal_info = vk_init::semaphore_submit_info(
        VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, curFrame.renderSemaphore);

    auto submission =
        vk_init::submit_info(&submit_info, &signal_info, &wait_info);

    vk_check(vkQueueSubmit2(m_vkQueue, 1, &submission,
                            curFrame.fence) // SEGFAULT IN HERE
    );

    auto present_info =
        VkPresentInfoKHR{.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                         .pNext = nullptr,

                         .waitSemaphoreCount = 1,
                         .pWaitSemaphores = &curFrame.renderSemaphore,

                         .swapchainCount = 1,
                         .pSwapchains = &m_Swapchain.descriptor,

                         .pImageIndices = &swapchainImageIdx};
    vk_check(vkQueuePresentKHR(m_vkQueue, &present_info));
    m_frameCount++;
}
void VkEngine::run() {
    using namespace std::chrono_literals;
    SDL_Event e{};
    bool should_stop{false};
    while (!should_stop) {
        while ((SDL_PollEvent(&e)) != 0) {
            if (e.type == SDL_EVENT_QUIT) {
                should_stop = true;
            }
            if (e.type == SDL_EVENT_WINDOW_MINIMIZED) {
                m_shouldStopRendering = true;
            }
            if (e.type == SDL_EVENT_WINDOW_RESTORED) {
                m_shouldStopRendering = false;
            }
        }
        if (m_shouldStopRendering) {
            std::this_thread::sleep_for(100ms);
            continue;
        }
        draw();
        std::println("fps:{:4.2f}",
                     m_frameCount / timer::get_seconds(timer::since_epoch()));
    }
    // TODO: implement
}
void VkEngine::cleanup() {
    LOG_INFO("DESTROYING ENGINE ({})", static_cast<void*>(this));
    if (is_initialized()) {
        vkDeviceWaitIdle(m_vkDevice);
        for (auto& frame : m_inflightFrames) {
            vkDestroyCommandPool(m_vkDevice, frame.commandPool, nullptr);

            vkDestroyFence(m_vkDevice, frame.fence, nullptr);
            vkDestroySemaphore(m_vkDevice, frame.renderSemaphore, nullptr);
            vkDestroySemaphore(m_vkDevice, frame.swapchainSemaphore, nullptr);
        }

        cleanup_swapchain();
        vkDestroySurfaceKHR(m_vkInstance, m_vkSurface, nullptr);
        vkDestroyDevice(m_vkDevice, nullptr);
        vkb::destroy_debug_utils_messenger(m_vkInstance, m_vkDebugMessenger);
        vkDestroyInstance(m_vkInstance, nullptr);
        SDL_DestroyWindow(m_window);
    }
    m_loadedEngine = nullptr;
}
VkEngine& VkEngine::get() { return *m_loadedEngine; }
int main() {
    timer::set_prog_epoch();
    cpptrace::register_terminate_handler();
    {
        VkEngine engine{};
        engine.run();
    }
    LOG_EXIT(EXIT_SUCCESS);
}
bool VkEngine::is_initialized() { return m_window; }
FrameData& VkEngine::get_current_frame() {
    return m_inflightFrames.at(m_frameCount % N_FSYNC);
}
