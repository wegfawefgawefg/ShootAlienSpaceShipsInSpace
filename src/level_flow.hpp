#pragma once

#include "battle_types.hpp"

void session_init(SessionState& session, Assets& assets);
void session_handle_event(SessionState& session, Assets& assets, const SDL_Event& event,
                          bool& running);
void session_update(SessionState& session, Assets& assets, float dt);
