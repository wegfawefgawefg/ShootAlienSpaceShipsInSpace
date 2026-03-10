#pragma once

#include "battle_types.hpp"

struct Assets;

void enter_shop(BattleState& battle);
void reroll_shop(BattleState& battle);
bool buy_selected_shop_offer(BattleState& battle);
bool move_selected_inventory_item(BattleState& battle);
void sell_selected_inventory_item(BattleState& battle);
bool advance_from_shop(BattleState& battle, Assets& assets);
