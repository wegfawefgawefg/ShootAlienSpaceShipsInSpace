#pragma once

#include "battle_types.hpp"

struct ScreenLayout;

void session_init(SessionState& session, Assets& assets);
void session_handle_event(SessionState& session, Assets& assets, const SDL_Event& event,
                          bool& running);
void session_handle_pointer(SessionState& session, Assets& assets, const ScreenLayout& layout,
                            float x, float y, bool left_pressed);
void session_update(SessionState& session, Assets& assets, float dt);
