#pragma once 
#include "Types.hpp"
#include "shared.hpp"
#include "glm/vec2.hpp"
struct PlatformConfig{
    glm::vec2 window_scale01;
    glm::vec2 window_pos01;
};
FWD_DECL_STRUCT(SDL_GPUDevice);
FWD_DECL_STRUCT(SDL_Window);
FWD_DECL_STRUCT(SDL_KeyboardEvent);
FWD_DECL_STRUCT(UI);
// Handles the window, gpu state 
struct PlatformContext{

    PlatformContext(PlatformConfig const &cfg) :cfg(cfg) {setup();}
    ~PlatformContext(){cleanup();}
    PlatformConfig const& cfg;

    void setup();
    void cleanup();



    bool is_minimized()const;
    bool should_close()const;


    void handle_events() ;




    bool m_should_close = false;
    float m_main_scale;
    SDL_Window* m_window;
    SDL_GPUDevice* m_gpu_device;
    f32 m_win_w_px;
    f32 m_win_h_px;
private:
    void handle_key_down(SDL_KeyboardEvent const& ev);
    bool ui_wants_mouse() const;
    bool ui_wants_keyboard() const;
};

