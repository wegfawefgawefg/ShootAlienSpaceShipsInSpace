#pragma once

#include "battle_types.hpp"

void session_render(const SessionState& session, const Assets& assets, SDL_Renderer* renderer,
                    Uint64 ticks);
