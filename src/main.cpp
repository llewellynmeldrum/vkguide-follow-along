#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <cpptrace/utils.hpp>
#include <mach/mig.h>
#include <print>
#include <vulkan/vulkan_core.h>
#include "VkBootstrap.h"
#include "vk_types.hpp"
#include "SDL3/SDL_video.h"
#include "Types.hpp"
#include "shared.hpp"
#include "logger.hpp"
#include "vulkan/vulkan.hpp"
// TODO: draw a triangle with vulkan
//
//
const char* APP_NAME = "Test Window";
FWD_DECL_STRUCT(SDL_Window);
struct VkEngine{
public:
    VkEngine(){init();}
    ~VkEngine(){cleanup();}

    VkEngine*                  m_loadedEngine  {};
    size_t                     m_frameCount    {};
    bool                       m_shouldStop    {};
    SDL_Window*                m_window        {};

    VkInstance               m_vkInstance    {};
    VkExtent2D               m_windowExtent  {800,600};
    VkDebugUtilsMessengerEXT m_vkDebugMessenger  {};
    VkPhysicalDevice         m_vkPhysicalDevice   {};
    VkDevice                 m_vkDevice           {};
    VkSurfaceKHR             m_vkSurface       {};

    struct SwapchainContext{
        VkSwapchainKHR   descriptor;
        VkFormat         imageFormat;

        std::vector<VkImage> images;
        std::vector<VkImageView> imageViews;
        VkExtent2D extent;
    }m_vkSwapchain;
    void init_swapchain();


    constexpr static bool m_useValidationLayers{};

    VkEngine& get();

    bool is_initialized();

    void init();
    void cleanup();

    void run();
    void draw();

private:
    void init_window();
    void init_vulkan();
    void cleanup_swapchain();
    void init_commands();
    void init_sync_structures();
};

void VkEngine::init_window(){
    SDL_Init(SDL_INIT_VIDEO);
    m_window = SDL_CreateWindow(
        APP_NAME,
        m_windowExtent.width,
        m_windowExtent.height,
        SDL_WINDOW_VULKAN
    );
    if (!m_window){ LOG_ERROR("Failed to init window"); LOG_EXIT(1); }
}

void VkEngine::init_vulkan(){
    
    u32 ext_count{};
    auto extensions = SDL_Vulkan_GetInstanceExtensions(&ext_count);
    vkb::InstanceBuilder builder;
    auto vkb_instance = builder
        .set_app_name(APP_NAME)
        .request_validation_layers(m_useValidationLayers)
        .enable_extensions(ext_count, extensions)
        .require_api_version(1,3,0)
        .build()
        .value();

    m_vkInstance = vkb_instance.instance;
    m_vkDebugMessenger = vkb_instance.debug_messenger;

    assert(m_window);
    if(!SDL_Vulkan_CreateSurface(m_window, m_vkInstance, nullptr, &m_vkSurface)){
        LOG_ERROR("Failure in {}(): {}", "SDL_Vulkan_CreateSurface", SDL_GetError());
        LOG_EXIT(1);
    }
    auto vkb_physical_gpu = vkb::PhysicalDeviceSelector{vkb_instance}
        .set_minimum_version(1,3)
        .set_required_features_12({
            .sType =VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
            .descriptorIndexing=true,
            .bufferDeviceAddress=true,
        })
        .set_required_features_13({
            .sType =VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
            .synchronization2 = true,
            .dynamicRendering = true,
        })
        .set_surface(m_vkSurface)
        .select()
        .value();
    m_vkPhysicalDevice = vkb_physical_gpu.physical_device;
    
    auto vkb_gpu = vkb::DeviceBuilder{vkb_physical_gpu}.build().value();

    m_vkDevice = vkb_gpu.device;



}
struct SwapchainSetup{
    VkPhysicalDevice physical_device;
    VkDevice device;
    VkSurfaceKHR  surface;
    VkExtent2D extents;
    VkFormat format=VK_FORMAT_B8G8R8A8_UNORM;
    VkColorSpaceKHR colorSpace{VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    VkPresentModeKHR present_mode{VK_PRESENT_MODE_FIFO_KHR};
    VkImageUsageFlagBits image_usage_flags{VK_IMAGE_USAGE_TRANSFER_DST_BIT};
    constexpr inline auto make_swapchain(){
        auto vkb_swapchain = vkb::SwapchainBuilder{physical_device,device, surface}
            .set_desired_format({
                .format = format,
                .colorSpace = colorSpace,
            })
            .set_desired_present_mode(present_mode)
            .set_desired_extent(extents.width,extents.height)
            .add_image_usage_flags(image_usage_flags)
            .build()
            .value();

        return VkEngine::SwapchainContext{
            .descriptor = vkb_swapchain.swapchain,
            .imageFormat = vkb_swapchain.image_format,
            .images = vkb_swapchain.get_images().value(),
            .imageViews= vkb_swapchain.get_image_views().value(),
            .extent = vkb_swapchain.extent,
        };
    }
};

void VkEngine::init_swapchain(){
    m_vkSwapchain = SwapchainSetup{
        .physical_device = m_vkPhysicalDevice,
        .device = m_vkDevice,
        .surface = m_vkSurface,
        .extents = m_windowExtent,
    }.make_swapchain();
}
void VkEngine::cleanup_swapchain(){
    vkDestroySwapchainKHR(m_vkDevice, m_vkSwapchain.descriptor, nullptr);
    for (auto image_view: m_vkSwapchain.imageViews){
        vkDestroyImageView(m_vkDevice,image_view,nullptr);
    }
}

void VkEngine::init_commands(){

}
void VkEngine::init_sync_structures(){

}

void VkEngine::init(){
    std::println(stderr, "INITIALIZING ENGINE ({})", static_cast<void*>(this));
    ASSERT(m_loadedEngine == nullptr);
    m_loadedEngine = this;
    init_window();
    init_vulkan();
    init_swapchain();
    init_commands();
    init_sync_structures();
}
void VkEngine::draw(){
    // TODO: implement
}
void VkEngine::run(){
    // TODO: implement
}
void VkEngine::cleanup(){
    std::println(stderr, "DESTROYING ENGINE ({})", static_cast<void*>(this));
    if (is_initialized()){
        cleanup_swapchain();
        vkDestroySurfaceKHR(m_vkInstance, m_vkSurface,nullptr);
        vkDestroyDevice(m_vkDevice,nullptr);
        vkb::destroy_debug_utils_messenger(m_vkInstance,m_vkDebugMessenger);
        vkDestroyInstance(m_vkInstance,nullptr);
        SDL_DestroyWindow(m_window);
    }
    m_loadedEngine = nullptr;
}
VkEngine& VkEngine::get(){
    return *m_loadedEngine;
}
int main() {
    cpptrace::register_terminate_handler();
    VkEngine engine{};
    engine.run();
}
bool VkEngine::is_initialized(){return m_window;}
