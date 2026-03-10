#pragma once

#include "battle_types.hpp"

void maybe_spawn_enemy_drop(BattleState& battle, const Enemy& enemy);
void maybe_spawn_enemy_gold(BattleState& battle, const Enemy& enemy, bool on_death);
bool apply_pickup_by_index(BattleState& battle, int def_index, bool from_shop);
void update_pickups(BattleState& battle, Assets& assets, float dt);
