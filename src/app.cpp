#include "app.hpp"

#include "battle_render.hpp"
#include "level_flow.hpp"

#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>

namespace {

constexpr float TARGET_FRAME_SECONDS = 1.0f / 144.0f;
constexpr float FIXED_STEP_SECONDS = 1.0f / 144.0f;

float frame_dt(Uint64 previous, Uint64 current) {
    const Uint64 freq = SDL_GetPerformanceFrequency();
    return static_cast<float>(current - previous) / static_cast<float>(freq);
}

bool create_scene_target(App& app) {
    app.scene_target = SDL_CreateTexture(app.renderer, SDL_PIXELFORMAT_RGBA8888,
                                         SDL_TEXTUREACCESS_TARGET, GAME_WIDTH, GAME_HEIGHT);
    if (!app.scene_target) {
        SDL_Log("SDL_CreateTexture(scene_target) failed: %s", SDL_GetError());
        return false;
    }
    SDL_SetTextureBlendMode(app.scene_target, SDL_BLENDMODE_BLEND);
    return true;
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

    app.renderer = SDL_CreateRenderer(app.window, -1, SDL_RENDERER_ACCELERATED);
    if (!app.renderer) {
        app.renderer = SDL_CreateRenderer(app.window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!app.renderer) {
        SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
        return false;
    }

    SDL_SetWindowPosition(app.window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowCursor(SDL_DISABLE);

    if (!create_scene_target(app)) {
        return false;
    }

    if (!assets_load(app.assets, app.renderer)) {
        return false;
    }

    session_init(app.session, app.assets);
    app.last_counter = SDL_GetPerformanceCounter();
    return true;
}

void app_run(App& app) {
    float accumulator = 0.0f;
    while (app.running) {
        const Uint64 frame_start = SDL_GetPerformanceCounter();
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            session_handle_event(app.session, app.assets, event, app.running);
        }

        const Uint64 current_counter = SDL_GetPerformanceCounter();
        const float dt = std::min(0.1f, frame_dt(app.last_counter, current_counter));
        app.last_counter = current_counter;
        accumulator += dt;

        while (accumulator >= FIXED_STEP_SECONDS) {
            session_update(app.session, app.assets, FIXED_STEP_SECONDS);
            accumulator -= FIXED_STEP_SECONDS;
        }

        int window_width = WINDOW_WIDTH;
        int window_height = WINDOW_HEIGHT;
        SDL_GetRendererOutputSize(app.renderer, &window_width, &window_height);
        const ScreenLayout layout = make_screen_layout(window_width, window_height);

        SDL_SetRenderTarget(app.renderer, app.scene_target);
        session_render_scene(app.session, app.assets, app.renderer, SDL_GetTicks64());

        SDL_SetRenderTarget(app.renderer, nullptr);
        SDL_SetRenderDrawColor(app.renderer, 0, 0, 0, 255);
        SDL_RenderClear(app.renderer);
        SDL_RenderCopyF(app.renderer, app.scene_target, nullptr, &layout.scene_rect);
        session_render_overlay(app.session, app.assets, app.renderer, layout, SDL_GetTicks64());
        SDL_RenderPresent(app.renderer);

        const float frame_time = frame_dt(frame_start, SDL_GetPerformanceCounter());
        const float remaining = TARGET_FRAME_SECONDS - frame_time;
        if (remaining > 0.0f) {
            SDL_Delay(static_cast<Uint32>(remaining * 1000.0f));
        }
    }
}

void app_shutdown(App& app) {
    assets_unload(app.assets);

    if (app.scene_target) {
        SDL_DestroyTexture(app.scene_target);
        app.scene_target = nullptr;
    }
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
