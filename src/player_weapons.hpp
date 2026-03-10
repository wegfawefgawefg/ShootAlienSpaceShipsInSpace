#pragma once

#include "battle_types.hpp"

void fire_player_weapons(BattleState& battle, Assets& assets, bool trigger_pressed,
                         bool trigger_held);
void update_player_bullets(BattleState& battle, float dt);
