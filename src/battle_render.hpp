#pragma once

#include "battle_types.hpp"

struct ScreenLayout {
    SDL_FRect scene_rect{};
    SDL_FRect left_panel{};
    SDL_FRect right_panel{};
};

ScreenLayout make_screen_layout(int window_width, int window_height);
int inventory_selectable_index_at(const BattleState& battle, const ScreenLayout& layout, float x,
                                  float y, int current_selection);
int shop_offer_index_at(const BattleState& battle, const ScreenLayout& layout, float x, float y);
void session_render_scene(const SessionState& session, const Assets& assets, SDL_Renderer* renderer,
                          Uint64 ticks);
void session_render_overlay(const SessionState& session, const Assets& assets,
                            SDL_Renderer* renderer, const ScreenLayout& layout, Uint64 ticks);
