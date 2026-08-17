#pragma once 
#include "shared.hpp"
FWD_DECL_STRUCT(PlatformContext);
FWD_DECL_UNION(SDL_Event);
struct UI{
public:
    UI(PlatformContext const& _ctx);
    ~UI();


    PlatformContext const& ctx;
    void process_event(SDL_Event const* ev)const;
    void setup();
    void draw();
    void cleanup();
    void render();
};

