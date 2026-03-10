#pragma once

#include "battle_types.hpp"

int enemy_small_ship_tile(int visual_index);
int player_ship_tile_center(int ship_index);

const std::vector<LevelDef>& levels();
void seed_default_weapons(BattleState& battle);
