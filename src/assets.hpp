#pragma once

#include "math.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <array>
#include <vector>

struct TextureAtlas {
    SDL_Texture* texture{nullptr};
    int columns{1};
    int rows{1};
    int tile_width{8};
    int tile_height{8};
    std::vector<Vec2> tile_offsets{};
};

struct Assets {
    TTF_Font* ui_font_small{nullptr};
    TTF_Font* ui_font_large{nullptr};
    SDL_Texture* menu_background{nullptr};
    SDL_Texture* menu_title{nullptr};
    TextureAtlas ships{};
    TextureAtlas particles{};

    std::array<Mix_Chunk*, 3> laser_sounds{};
    std::array<Mix_Chunk*, 3> rock_hit_sounds{};
    Mix_Chunk* warp_start{nullptr};
    Mix_Chunk* warp_stop{nullptr};
    Mix_Chunk* menu_music{nullptr};
    std::array<Mix_Chunk*, 2> battle_music{};
};

bool assets_load(Assets& assets, SDL_Renderer* renderer);
void assets_unload(Assets& assets);

SDL_Rect atlas_tile_rect(const TextureAtlas& atlas, int tile_index);
Vec2 atlas_tile_offset(const TextureAtlas& atlas, int tile_index);
