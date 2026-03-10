#include "app.hpp"

#include "battle_render.hpp"
#include "level_flow.hpp"

#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>

namespace {

float frame_dt(Uint64 previous, Uint64 current) {
    const Uint64 freq = SDL_GetPerformanceFrequency();
    return static_cast<float>(current - previous) / static_cast<float>(freq);
}

} // namespace

bool app_init(App& app) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return false;
    }

    if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) == 0) {
        SDL_Log("IMG_Init failed: %s", IMG_GetError());
        return false;
    }

    if (TTF_Init() != 0) {
        SDL_Log("TTF_Init failed: %s", TTF_GetError());
        return false;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) != 0) {
        SDL_Log("Mix_OpenAudio failed: %s", Mix_GetError());
        return false;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
    app.window = SDL_CreateWindow("Shoot Alien Space Ships In Space", SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT,
                                  SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_UTILITY);
    if (!app.window) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        return false;
    }

    app.renderer =
        SDL_CreateRenderer(app.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!app.renderer) {
        app.renderer = SDL_CreateRenderer(app.window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!app.renderer) {
        SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
        return false;
    }

    SDL_SetWindowPosition(app.window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_RenderSetLogicalSize(app.renderer, GAME_WIDTH, GAME_HEIGHT);
    SDL_ShowCursor(SDL_DISABLE);

    if (!assets_load(app.assets, app.renderer)) {
        return false;
    }

    session_init(app.session, app.assets);
    app.last_counter = SDL_GetPerformanceCounter();
    return true;
}

void app_run(App& app) {
    while (app.running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            session_handle_event(app.session, app.assets, event, app.running);
        }

        const Uint64 current_counter = SDL_GetPerformanceCounter();
        const float dt = frame_dt(app.last_counter, current_counter);
        app.last_counter = current_counter;

        session_update(app.session, app.assets, dt);
        session_render(app.session, app.assets, app.renderer, SDL_GetTicks64());
        SDL_RenderPresent(app.renderer);
    }
}

void app_shutdown(App& app) {
    assets_unload(app.assets);

    if (app.renderer) {
        SDL_DestroyRenderer(app.renderer);
        app.renderer = nullptr;
    }
    if (app.window) {
        SDL_DestroyWindow(app.window);
        app.window = nullptr;
    }

    Mix_CloseAudio();
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
}
