#include "assets.hpp"

#include <array>
#include <filesystem>
#include <string>

namespace {

SDL_Texture* load_texture(SDL_Renderer* renderer, const char* path) {
    SDL_Surface* surface = IMG_Load(path);
    if (!surface) {
        SDL_Log("IMG_Load failed for %s: %s", path, IMG_GetError());
        return nullptr;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (!texture) {
        SDL_Log("SDL_CreateTextureFromSurface failed for %s: %s", path, SDL_GetError());
    }
    return texture;
}

Mix_Chunk* load_sound(const char* path) {
    Mix_Chunk* sound = Mix_LoadWAV(path);
    if (!sound) {
        SDL_Log("Mix_LoadWAV failed for %s: %s", path, Mix_GetError());
    }
    return sound;
}

TTF_Font* load_font() {
    static const std::array<const char*, 4> candidate_paths = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
    };

    for (const char* path : candidate_paths) {
        if (!std::filesystem::exists(path)) {
            continue;
        }
        TTF_Font* font = TTF_OpenFont(path, 20);
        if (font) {
            return font;
        }
    }

    SDL_Log("Unable to load a system font: %s", TTF_GetError());
    return nullptr;
}

bool load_sound_array(std::array<Mix_Chunk*, 3>& sounds, const std::array<const char*, 3>& paths) {
    for (std::size_t i = 0; i < sounds.size(); ++i) {
        sounds[i] = load_sound(paths[i]);
        if (!sounds[i]) {
            return false;
        }
    }
    return true;
}

bool load_sound_array(std::array<Mix_Chunk*, 2>& sounds, const std::array<const char*, 2>& paths) {
    for (std::size_t i = 0; i < sounds.size(); ++i) {
        sounds[i] = load_sound(paths[i]);
        if (!sounds[i]) {
            return false;
        }
    }
    return true;
}

} // namespace

bool assets_load(Assets& assets, SDL_Renderer* renderer) {
    assets.ui_font = load_font();
    assets.menu_background = load_texture(renderer, "assets/cover.png");
    assets.menu_title = load_texture(renderer, "assets/title.png");
    assets.ships.texture = load_texture(renderer, "assets/ships.png");
    assets.ships.columns = 10;
    assets.ships.rows = 10;
    assets.ships.tile_width = 8;
    assets.ships.tile_height = 8;

    assets.particles.texture = load_texture(renderer, "assets/particles.png");
    assets.particles.columns = 6;
    assets.particles.rows = 10;
    assets.particles.tile_width = 8;
    assets.particles.tile_height = 8;

    const bool textures_ok = assets.ui_font && assets.menu_background && assets.menu_title &&
                             assets.ships.texture && assets.particles.texture;
    if (!textures_ok) {
        return false;
    }

    const std::array<const char*, 3> laser_paths = {
        "assets/laserShoot1.wav",
        "assets/laserShoot2.wav",
        "assets/laserShoot3.wav",
    };
    const std::array<const char*, 3> rock_paths = {
        "assets/rockhit.wav",
        "assets/rockhit2.wav",
        "assets/rockhit3.wav",
    };
    const std::array<const char*, 2> battle_music_paths = {
        "assets/loading.wav",
        "assets/menu.wav",
    };

    if (!load_sound_array(assets.laser_sounds, laser_paths)) {
        return false;
    }
    if (!load_sound_array(assets.rock_hit_sounds, rock_paths)) {
        return false;
    }

    assets.warp_start = load_sound("assets/start_warping.wav");
    assets.warp_stop = load_sound("assets/stop_warping.wav");
    assets.menu_music = load_sound("assets/mainmenumusic.wav");
    if (!assets.warp_start || !assets.warp_stop || !assets.menu_music) {
        return false;
    }
    return load_sound_array(assets.battle_music, battle_music_paths);
}

void assets_unload(Assets& assets) {
    auto destroy_texture = [](SDL_Texture*& texture) {
        if (texture) {
            SDL_DestroyTexture(texture);
            texture = nullptr;
        }
    };

    auto free_chunk = [](Mix_Chunk*& chunk) {
        if (chunk) {
            Mix_FreeChunk(chunk);
            chunk = nullptr;
        }
    };

    destroy_texture(assets.menu_background);
    destroy_texture(assets.menu_title);
    destroy_texture(assets.ships.texture);
    destroy_texture(assets.particles.texture);
    if (assets.ui_font) {
        TTF_CloseFont(assets.ui_font);
        assets.ui_font = nullptr;
    }

    for (Mix_Chunk*& sound : assets.laser_sounds) {
        free_chunk(sound);
    }
    for (Mix_Chunk*& sound : assets.rock_hit_sounds) {
        free_chunk(sound);
    }
    for (Mix_Chunk*& sound : assets.battle_music) {
        free_chunk(sound);
    }

    free_chunk(assets.warp_start);
    free_chunk(assets.warp_stop);
    free_chunk(assets.menu_music);
}

SDL_Rect atlas_tile_rect(const TextureAtlas& atlas, int tile_index) {
    const int wrapped = tile_index % (atlas.columns * atlas.rows);
    const int tile_x = wrapped % atlas.columns;
    const int tile_y = wrapped / atlas.columns;
    return {
        tile_x * atlas.tile_width,
        tile_y * atlas.tile_height,
        atlas.tile_width,
        atlas.tile_height,
    };
}
