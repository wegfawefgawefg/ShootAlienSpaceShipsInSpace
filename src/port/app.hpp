#pragma once

#include "port/assets.hpp"
#include "port/gameplay.hpp"

#include <SDL2/SDL.h>

struct App {
    SDL_Window* window{nullptr};
    SDL_Renderer* renderer{nullptr};
    Assets assets{};
    GameplayState gameplay{};
    bool running{true};
    Uint64 last_counter{0};
};

bool app_init(App& app);
void app_run(App& app);
void app_shutdown(App& app);
