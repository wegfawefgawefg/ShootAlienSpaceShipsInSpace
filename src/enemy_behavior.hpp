#pragma once

#include "battle_types.hpp"

void start_level(BattleState& battle, int level_index);
bool spawn_next_wave(BattleState& battle);
bool update_enemy_intro(BattleState& battle, float dt);
void update_enemy_wave(BattleState& battle, float dt);
