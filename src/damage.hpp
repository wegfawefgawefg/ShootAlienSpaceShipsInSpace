#pragma once

#include "battle_types.hpp"

void update_enemy_bullets(BattleState& battle, float dt);
void update_particles(BattleState& battle, float dt);
void update_player_respawn(BattleState& battle, float dt);
void resolve_damage(BattleState& battle);
