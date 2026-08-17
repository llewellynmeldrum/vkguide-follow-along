#include <print>

#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_video.h"
#include "glm/vec2.hpp"

#include "platform.hpp"
#include "imgui.h"
#include "ui.hpp"

void PlatformContext::handle_key_down(SDL_KeyboardEvent const& ev) {
    if (ui_wants_keyboard()) return; // ignore?
    switch (ev.key){
        case SDLK_ESCAPE:{
            m_should_close = true; 
            return;
            break;
        }
        default:{
            break;
        }
    }
}

void PlatformContext::handle_events() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
//        ui.process_event(&event);
        if (event.type == SDL_EVENT_QUIT){
            m_should_close = true; 
            return;
        } else if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED){
            if (event.window.windowID == SDL_GetWindowID(m_window)){
                m_should_close = true; 
                return;
            }
        }
        switch (event.type){
            case SDL_EVENT_KEY_DOWN: 
                handle_key_down(event.key);
            break;

            default: 
            break;
        }
    }

}
bool PlatformContext::should_close()const{
    return m_should_close;
}
bool PlatformContext::ui_wants_mouse()const {
    ImGuiIO& io = ImGui::GetIO();
    return io.WantCaptureMouse;
}
bool PlatformContext::ui_wants_keyboard()const {
    ImGuiIO& io = ImGui::GetIO();
    return io.WantCaptureKeyboard;
}
bool PlatformContext::is_minimized()const {
    return SDL_GetWindowFlags(m_window) & SDL_WINDOW_MINIMIZED;
}
void PlatformContext::setup(){
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std ::println("Error: {}() -> {}", "SDL_Init", SDL_GetError());
        std ::exit(1);
    };
    auto const* mode = SDL_GetDesktopDisplayMode(SDL_GetPrimaryDisplay());
    m_win_w_px = cfg.window_scale01.x * mode->w;
    m_win_h_px = cfg.window_scale01.y * mode->h;
    m_main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    m_window = SDL_CreateWindow("window", m_win_w_px, m_win_h_px, 
                              SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!m_window) {
        std::println("Error: SDL_CreateWindow() -> {}",SDL_GetError());
        std::exit(EXIT_FAILURE);
    }
    f32 win_x_px = cfg.window_pos01.x * m_win_w_px;
    f32 win_y_px = cfg.window_pos01.y * m_win_h_px;

    // Create SDL window graphics context
    if (!SDL_SetWindowPosition(m_window, win_x_px, win_y_px)) {
        std ::println("Error: {}() -> {}", "SDL_SetWindowPosition", SDL_GetError());
        std ::exit(1);
    };
    m_gpu_device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL | SDL_GPU_SHADERFORMAT_METALLIB, true, nullptr);
    if (!m_gpu_device ) {
        std::println("Error: SDL_CreateGPUDevice(): {}", SDL_GetError());
        std ::exit(1);
    }
    if (!SDL_ShowWindow(m_window)) {
        std ::println("Error: {}() -> {}", "SDL_ShowWindow", SDL_GetError());
        std ::exit(1);
    };
    // Claim window for GPU Device
    if (!SDL_ClaimWindowForGPUDevice(m_gpu_device, m_window))
    {
        std::println("Error: SDL_ClaimWindowFOrGPUDevice(): {}", SDL_GetError());
        std ::exit(1);
    }
    SDL_SetGPUSwapchainParameters(m_gpu_device, m_window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC);
}

void PlatformContext::cleanup(){ 
    SDL_ReleaseWindowFromGPUDevice(m_gpu_device, m_window);
    SDL_DestroyGPUDevice(m_gpu_device);
    SDL_DestroyWindow(m_window);
    SDL_Quit();
}
