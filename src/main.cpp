#include <vulkan/vulkan.hpp>
#include "vk_types.hpp"
#include "range/v3/view/enumerate.hpp"
// stdlib
#include <format>
#include <print>
#include <thread>
#include <type_traits>

// Libraries
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <cpptrace/utils.hpp>
#include <SDL3/SDL_video.h>
#include <VkBootstrap.h>

// Project headers
#include "types.hpp"
#include "vk_images.hpp"
#include "vk_swapchain.hpp"
#include "logger.hpp"
#include "shared.hpp"
#include "vkinit.hpp"
#include "timer.hpp"
#include "vulkan/vulkan.hpp"
//#include "vulkan/vulkan.hpp"


char const* APP_NAME = "Test Window";

struct FrameData {
    vk::raii::CommandPool commandPool{nullptr};
    vk::raii::CommandBuffer commandBuffer{nullptr};
    vk::raii::Fence fence{nullptr};
    vk::raii::Semaphore swapchainSemaphore{nullptr};
};

FWD_DECL_STRUCT(SDL_Window);
struct VkEngine {
  public:
    template <typename T, size_t S> using array = std::array<T, S>;
    static constexpr u32 N_FSYNC = 2;
    VkEngine() { init(); }
    ~VkEngine() { cleanup(); }
    VkEngine* m_loadedEngine{};

    size_t m_frameCount{};
    bool m_shouldStopRendering{};
    SDL_Window* m_window{};
    static constexpr vk::Extent2D m_windowExtent{800, 600};
    static constexpr bool m_useValidationLayers{true};

    vk::raii::Context m_vkContext{};
    vk::raii::Instance m_vkInstance{nullptr};
    vk::raii::DebugUtilsMessengerEXT m_vkDebugMessenger{nullptr};
    vk::raii::SurfaceKHR m_vkSurface{nullptr};
    vk::raii::PhysicalDevice m_vkPhysicalDevice{nullptr};
    vk::raii::Device m_vkDevice{nullptr};
    vk::raii::Queue m_vkQueue{nullptr};
    u32 m_vkQueueFamily{};


    Swapchain m_Swapchain{};

    array<FrameData, N_FSYNC> m_inflightFrames{};

    FrameData& get_current_frame();


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
VKAPI_ATTR vk::Bool32 VKAPI_CALL vk_debug_callback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
    vk::DebugUtilsMessageTypeFlagsEXT types,
    vk::DebugUtilsMessengerCallbackDataEXT const* data,
    void* _
) {
    using Sev = vk::DebugUtilsMessageSeverityFlagBitsEXT;
    auto const kind = vk::to_string(types);
    if (severity >= Sev::eError) {
        LOG_ERROR("[vulkan {}] {}", kind, data->pMessage);
    } else if (severity >= Sev::eWarning) {
        LOG_WARN("[vulkan {}] {}", kind, data->pMessage);
    } else {
        LOG_INFO("[vulkan {}] {}", kind, data->pMessage);
    }
    ASSERT(false);
    return vk::False;
}

bool supports_extension(std::span<vk::ExtensionProperties const> haystack, std::string_view needle) {
    return std::ranges::any_of(
        haystack, 
        [needle](auto const& ext) {
            return std::string_view{ext.extensionName} == needle;
        }
    );
}
template<typename Pred>
auto get_pd_queue_family(vk::raii::PhysicalDevice physical_device, Pred&& pred) -> std::optional<std::size_t>{
    auto families = physical_device.getQueueFamilyProperties();
    for (auto const& [idx, family] : ranges::views::enumerate(families)){
        if (!(family.queueFlags & vk::QueueFlagBits::eGraphics)){
            continue;
        }
        if (std::invoke(std::forward<Pred>(pred),idx)){
            return idx;
        }
    }
    return std::nullopt;
}
constexpr auto pd_has_required_features(vk::raii::PhysicalDevice const& pd) {

    auto const features = pd.getFeatures2<
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan12Features,
        vk::PhysicalDeviceVulkan13Features
    >();

    auto const& f12 = features.get<vk::PhysicalDeviceVulkan12Features>();
    auto const& f13 = features.get<vk::PhysicalDeviceVulkan13Features>();

    bool f12_compliant = 
        f12.descriptorIndexing 
        && f12.bufferDeviceAddress;

    bool f13_compliant = 
        f13.synchronization2 
        && f13.dynamicRendering;

    return f12_compliant && f13_compliant;
};
void VkEngine::init_vulkan() try {
    static constexpr auto API_VER = vk::ApiVersion13;
    constexpr auto EXT_MOLTENVK_FIX = "VK_KHR_portability_subset";
    constexpr auto VALIDATION_LAYER = "VK_LAYER_KHRONOS_validation";
    // sdl requires some extensions for its windowing stuff
    
    u32 ext_count{};
    auto const* sdl_extensions = SDL_Vulkan_GetInstanceExtensions(&ext_count);
    if (!sdl_extensions){
        LOG_FATAL("Failed to query extensions from SDL. SDL_Error:{}",SDL_GetError());
    }
    auto instanceExtensions = std::vector<char const*>(sdl_extensions, sdl_extensions+ext_count);
    auto appLayers = std::vector<char const*>{};
    auto instanceFlags = vk::InstanceCreateFlags{};


    struct InstanceExtension{
        const char* name;
        vk::InstanceCreateFlagBits flag;
    };
    static constexpr InstanceExtension portabilityKHR{
        .name = vk::KHRPortabilityEnumerationExtensionName,
        .flag = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR,
    };

    auto const inst_supported_extensions = m_vkContext.enumerateInstanceExtensionProperties();
    // this is required to enable moltenVK support, as it is a non conformant driver
    if (supports_extension(inst_supported_extensions, portabilityKHR.name)){
        instanceExtensions.push_back(portabilityKHR.name);
        instanceFlags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
    }

    if (m_useValidationLayers){
        appLayers.push_back(VALIDATION_LAYER);
        instanceExtensions.push_back(vk::EXTDebugUtilsExtensionName);
    }

 
    auto appInfo = vk::ApplicationInfo{
        .pApplicationName = APP_NAME,
        .applicationVersion = vk::makeApiVersion(0, 0, 1, 0),
        .pEngineName = "No Engine",
        .engineVersion = vk::makeApiVersion(0, 0, 1, 0),
        .apiVersion = API_VER,
    };

    using Severity = vk::DebugUtilsMessageSeverityFlagBitsEXT;
    using Type = vk::DebugUtilsMessageTypeFlagBitsEXT;
    auto const instanceDebugInfo = vk::DebugUtilsMessengerCreateInfoEXT{
        .messageSeverity = Severity::eWarning | Severity::eError,
        .messageType = Type::eGeneral | Type::eValidation | Type::ePerformance,
        .pfnUserCallback = vk_debug_callback,
    };

    m_vkInstance = vk::raii::Instance{
        m_vkContext,
        vk::InstanceCreateInfo{
            // without this pnext, vulkan cannot report errors to the debug extension during instance creation
            .pNext = m_useValidationLayers ? &instanceDebugInfo : nullptr,
            .flags = instanceFlags,
            .pApplicationInfo = &appInfo,

            .enabledLayerCount = static_cast<u32>(appLayers.size()),
            .ppEnabledLayerNames = appLayers.data(),

            .enabledExtensionCount = static_cast<u32>(instanceExtensions.size()),
            .ppEnabledExtensionNames = instanceExtensions.data(),

        },
    };

    if (m_useValidationLayers){
        m_vkDebugMessenger = vk::raii::DebugUtilsMessengerEXT{m_vkInstance, instanceDebugInfo};
    }
    ASSERT(m_window);

    // Have SDL create the surface given our instance we just setup 
    auto raw_surface = VkSurfaceKHR{};
    if (!SDL_Vulkan_CreateSurface(m_window, get_c_handle(m_vkInstance), nullptr,
                                  &raw_surface)) {
        LOG_FATAL("Failure in {}(): {}", "SDL_Vulkan_CreateSurface", SDL_GetError());
    }
    m_vkSurface = vk::raii::SurfaceKHR{m_vkInstance, raw_surface};

    // NOTE: Instance extensions modify global behaviour BEFORE a device is selected,
    // whereas DEVICE extensions modify the behaviour of a specific vk::Device.
    // Configure Device extensions
    auto device_extensions = std::vector<char const*>{};

    // iterate over all physical devices listed by driver
    for (auto const& physical_device : m_vkInstance.enumeratePhysicalDevices()){
        // skip if device doesnt support expected api version
        auto device_api_ver = physical_device.getProperties().apiVersion;
        if (device_api_ver < API_VER) continue;

        // skip if device doesnt support khr swapchain
        auto const supported_extensions = physical_device.enumerateDeviceExtensionProperties();
        if (!supports_extension(supported_extensions,vk::KHRSwapchainExtensionName)){
            continue;
        }

        auto family = get_pd_queue_family(
            physical_device, 
            [physical_device, this](u32 idx){
                return physical_device.getSurfaceSupportKHR(idx,m_vkSurface) == vk::True;
            }
        );
        if (!family) continue; // no matching queue family on the device
        
        if (!pd_has_required_features(physical_device)) continue;

        device_extensions.push_back(vk::KHRSwapchainExtensionName); 

        if (supports_extension(supported_extensions,EXT_MOLTENVK_FIX)){
            device_extensions.push_back(EXT_MOLTENVK_FIX);
        }
        m_vkPhysicalDevice = std::move(physical_device);
        m_vkQueueFamily = *family;
        break;
    }
    if (!*m_vkPhysicalDevice){
        LOG_FATAL("Unable to select a physical device! Driver listed {}, none matched",//
                  m_vkInstance.enumeratePhysicalDevices().size());
    }else{
        LOG_INFO("SELECTED GPU: {}",std::string_view{m_vkPhysicalDevice.getProperties().deviceName});
    }

    auto queue_prio = 1.0f;
    auto queue_info = vk::DeviceQueueCreateInfo{
        .queueFamilyIndex = m_vkQueueFamily,
        .queueCount = 1,
        .pQueuePriorities = &queue_prio,
    };

    auto const required_features_12 = vk::PhysicalDeviceVulkan12Features{
        .descriptorIndexing = vk::True,
        .bufferDeviceAddress = vk::True,
    };
    auto const required_features_13 = vk::PhysicalDeviceVulkan13Features{
        .synchronization2 = vk::True,
        .dynamicRendering = vk::True,
    };

    auto enabled_features = vk::StructureChain{
        vk::PhysicalDeviceFeatures2{},
        required_features_12,
        required_features_13,
    };
    m_vkDevice = vk::raii::Device{
        m_vkPhysicalDevice,
        vk::DeviceCreateInfo{
            .pNext = &enabled_features.get<vk::PhysicalDeviceFeatures2>(),
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &queue_info,
            .enabledExtensionCount = static_cast<u32>(device_extensions.size()),
            .ppEnabledExtensionNames = device_extensions.data(),
        },
    };
    m_vkQueue = m_vkDevice.getQueue(m_vkQueueFamily, 0);
} catch(vk::SystemError const& e){
    LOG_FATAL("Failed to initialize vulkan: {}", e.what());
}


void VkEngine::init_swapchain() {
    m_Swapchain = make_swapchain(
        SwapchainSettings{
            .physical_device = m_vkPhysicalDevice,
            .device = m_vkDevice,
            .surface = m_vkSurface,
            .extents = m_windowExtent,
        }
    );
}

void VkEngine::init_commands() {
    for (auto& frame : m_inflightFrames){
        frame.commandPool = vk::raii::CommandPool{
            m_vkDevice,
            vk::CommandPoolCreateInfo{
                // without this flag, we would not be able to individually reset command buffers,
                // but rather only the whole pool
                .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                .queueFamilyIndex = m_vkQueueFamily,
            },
        };
        
        auto buffers = m_vkDevice.allocateCommandBuffers(
            vk::CommandBufferAllocateInfo{
                .commandPool = *frame.commandPool,
                .level = vk::CommandBufferLevel::ePrimary,
                .commandBufferCount = 1,
            }
        );
        ASSERT(buffers.size() == 1);
        frame.commandBuffer = std::move(buffers.at(0));
    }


}

void VkEngine::init_sync_structures() {
    for (auto& frame : m_inflightFrames) {
        // Fences are cpu<->gpu. 
        frame.fence = vk::raii::Fence{
            m_vkDevice,
            vk::FenceCreateInfo{
                .flags = vk::FenceCreateFlagBits::eSignaled,
            },
        };

        frame.swapchainSemaphore = vk::raii::Semaphore{
            m_vkDevice,
            vk::SemaphoreCreateInfo{}
        };
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
    auto& frame = get_current_frame();
    auto fenceWaitRV = m_vkDevice.waitForFences(*frame.fence, vk::True, stons(1));
    if (fenceWaitRV != vk::Result::eSuccess){
        LOG_FATAL("Failed to wait for fence");
    }
    u32 imageIndex{};
    try{
        auto [res, idx] = m_Swapchain.descriptor.acquireNextImage(stons(1),*frame.swapchainSemaphore);
        imageIndex = idx;
        if (res == vk::Result::eSuboptimalKHR){
            LOG_WARN("Suboptimal swapchain acquire (wtv the fuck that means)");
        }
    } catch(vk::OutOfDateKHRError const&){
        // recreate swapchain
        PANIC("Unimplemented");
    }
    m_vkDevice.resetFences(*frame.fence);

    auto & cmdBuf = frame.commandBuffer;

    cmdBuf.reset();

    cmdBuf.begin(
        vk::CommandBufferBeginInfo{
            .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
        }
    );
    auto const swapchainImage = m_Swapchain.images.at(imageIndex);

    vk_util::transition_image(cmdBuf, swapchainImage, 
                              vk::ImageLayout::eUndefined,
                              vk::ImageLayout::eGeneral);

    auto const clearColor = vk::ClearColorValue{.float32 = {{
        std::abs(std::sin(m_frameCount / 120.0f)),
        0.0f,
        std::abs(std::sin(m_frameCount / 120.0f)),
        1.0f
        }}
    };

    cmdBuf.clearColorImage(
            swapchainImage, 
            vk::ImageLayout::eGeneral,
            clearColor,
            vk_util::image_subresource_range(vk::ImageAspectFlagBits::eColor)
        );

    vk_util::transition_image(cmdBuf, swapchainImage, 
                              vk::ImageLayout::eGeneral,
                                vk::ImageLayout::ePresentSrcKHR);
    cmdBuf.end();

    auto const cmdInfo = vk::CommandBufferSubmitInfo{
        .commandBuffer = *cmdBuf,
    };
    auto const waitInfo = vk::SemaphoreSubmitInfo{
        .semaphore = *frame.swapchainSemaphore,
        .value = 1,
        .stageMask = vk::PipelineStageFlagBits2::eAllCommands,
    };

    auto const signalInfo = vk::SemaphoreSubmitInfo{
        .semaphore = *m_Swapchain.renderSemaphores[imageIndex],
        .value = 1,
        .stageMask = vk::PipelineStageFlagBits2::eAllCommands,
    };
    auto submit_info = vk::SubmitInfo2{};
    submit_info.setCommandBufferInfos(cmdInfo);
    submit_info.setWaitSemaphoreInfos(waitInfo);
    submit_info.setSignalSemaphoreInfos(signalInfo);
    m_vkQueue.submit2(
        submit_info,
        *frame.fence
    );
    try {
        auto const res = m_vkQueue.presentKHR(
            vk::PresentInfoKHR{
                .waitSemaphoreCount = 1,
                .pWaitSemaphores = &*m_Swapchain.renderSemaphores[imageIndex],
                .swapchainCount = 1,
                .pSwapchains = &*m_Swapchain.descriptor,
                .pImageIndices = &imageIndex,
            }
        );
        if (res == vk::Result::eSuboptimalKHR){
            // recreate swapchain
            PANIC("Unimplemented");
        }

    } catch(vk::OutOfDateKHRError const&){
        // recreate swapchain
        PANIC("Unimplemented");
    }
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
    if (is_initialized()) {
        m_vkDevice.waitIdle();

        m_Swapchain = Swapchain{};
        for (auto& frame: m_inflightFrames){
            frame = FrameData{};
        }
        m_vkQueue.clear();
        m_vkDevice.clear();
        m_vkPhysicalDevice.clear();
        m_vkSurface.clear();
        // this cant be done here, shouldnt it happen after destruction of raii stuff?
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
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
