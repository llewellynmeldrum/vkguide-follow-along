#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlgpu3.h"


#include "ui.hpp"
#include "platform.hpp"
#include "shared.hpp"
namespace IG = ImGui;

UI::UI(PlatformContext const& _ctx) : ctx(_ctx) { 
    setup(); 
}
UI::~UI(){
    cleanup();
}

void UI::process_event(SDL_Event const* event)const{
    ImGui_ImplSDL3_ProcessEvent(event);
}
void UI::setup(){
    IMGUI_CHECKVERSION();
    IG::CreateContext();
    ImGuiIO& io = IG::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    IG::StyleColorsDark();

    // Setup scaling
    ImGuiStyle& style = IG::GetStyle();
    style.ScaleAllSizes(ctx.m_main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = ctx.m_main_scale;        // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically overrides this for every window depending on the current monitor)

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForSDLGPU(ctx.m_window);
    ImGui_ImplSDLGPU3_InitInfo init_info = {};
    init_info.Device = ctx.m_gpu_device;
    init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(ctx.m_gpu_device, ctx.m_window);
    init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;                      // Only used in multi-viewports mode.
    init_info.SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR;  // Only used in multi-viewports mode.
    init_info.PresentMode = SDL_GPU_PRESENTMODE_VSYNC;
    ImGui_ImplSDLGPU3_Init(&init_info);
}
void UI::cleanup(){
    SDL_WaitForGPUIdle(ctx.m_gpu_device);
    ImGui_ImplSDL3_Shutdown();
    ImGui_ImplSDLGPU3_Shutdown();
    IG::DestroyContext();
}
void UI::draw(){
    ImGui_ImplSDLGPU3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    IG::NewFrame();
    IG::ShowDemoWindow();
}
void UI::render(){
    // Rendering
    IG::Render();
    ImDrawData* draw_data = IG::GetDrawData();
    const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

    SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(ctx.m_gpu_device); // Acquire a GPU command buffer

    SDL_GPUTexture* swapchain_texture{};
    SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, ctx.m_window, &swapchain_texture, nullptr, nullptr);

    if (swapchain_texture && !is_minimized) {
        // This is mandatory: call ImGui_ImplSDLGPU3_PrepareDrawData() to upload the vertex/index buffer!
        ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, command_buffer);

        // Setup and start a render pass
        SDL_GPUColorTargetInfo target_info = {};
        target_info.texture = swapchain_texture;
        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
        target_info.clear_color = SDL_FColor { clear_color.x, clear_color.y, clear_color.z, clear_color.w };
        target_info.load_op = SDL_GPU_LOADOP_CLEAR;
        target_info.store_op = SDL_GPU_STOREOP_STORE;
        target_info.mip_level = 0;
        target_info.layer_or_depth_plane = 0;
        target_info.cycle = false;

        SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(command_buffer, &target_info, 1, nullptr);

        // Render ImGui
        ImGui_ImplSDLGPU3_RenderDrawData(draw_data, command_buffer, render_pass);

        SDL_EndGPURenderPass(render_pass);
    }

    // Submit the command buffer
    SDL_SubmitGPUCommandBuffer(command_buffer);
}
