#include "shop.hpp"

#include "enemy_behavior.hpp"
#include "level_data.hpp"
#include "pickup_defs.hpp"
#include "pickups.hpp"

#include <SDL2/SDL_mixer.h>
#include <algorithm>
#include <cstdlib>

namespace {

int base_price_for_tier(PickupTier tier) {
    switch (tier) {
    case PickupTier::Common:
        return 8;
    case PickupTier::Uncommon:
        return 15;
    case PickupTier::Rare:
        return 26;
    case PickupTier::Boss:
        return 40;
    case PickupTier::Cursed:
        return 22;
    }
    return 10;
}

int price_for_pickup(const PickupDef& def) {
    int price = base_price_for_tier(def.tier);
    if (def.effect == PickupEffectType::GrantWeapon) {
        price += 6;
    }
    return price;
}

bool is_weapon_offer(const PickupDef& def) {
    return def.effect == PickupEffectType::GrantWeapon;
}

bool is_support_offer(const PickupDef& def) {
    return def.effect != PickupEffectType::GrantWeapon;
}

PickupTier roll_shop_tier(int level_index, bool premium_slot) {
    const int roll = std::rand() % 100;
    if (premium_slot) {
        if (level_index >= 2) {
            if (roll < 18) {
                return PickupTier::Boss;
            }
            if (roll < 58) {
                return PickupTier::Rare;
            }
            if (roll < 88) {
                return PickupTier::Uncommon;
            }
            return PickupTier::Common;
        }
        if (roll < 35) {
            return PickupTier::Rare;
        }
        if (roll < 80) {
            return PickupTier::Uncommon;
        }
        return PickupTier::Common;
    }

    if (level_index <= 0) {
        if (roll < 58) {
            return PickupTier::Common;
        }
        if (roll < 90) {
            return PickupTier::Uncommon;
        }
        return PickupTier::Rare;
    }
    if (level_index == 1) {
        if (roll < 38) {
            return PickupTier::Common;
        }
        if (roll < 78) {
            return PickupTier::Uncommon;
        }
        return PickupTier::Rare;
    }
    if (roll < 24) {
        return PickupTier::Common;
    }
    if (roll < 62) {
        return PickupTier::Uncommon;
    }
    if (roll < 94) {
        return PickupTier::Rare;
    }
    return PickupTier::Boss;
}

int choose_shop_pickup_index(const BattleState& battle, bool want_weapon, bool premium_slot,
                             const std::vector<int>& used_defs) {
    const auto& defs = pickup_defs();
    const PickupTier desired_tier = roll_shop_tier(battle.current_level_index, premium_slot);

    auto not_used = [&](int def_index) {
        return std::find(used_defs.begin(), used_defs.end(), def_index) == used_defs.end();
    };
    auto matches_lane = [&](const PickupDef& def) {
        return want_weapon ? is_weapon_offer(def) : is_support_offer(def);
    };

    std::vector<int> preferred;
    std::vector<int> relaxed_tier;
    std::vector<int> fallback;
    for (int i = 0; i < static_cast<int>(defs.size()); ++i) {
        const PickupDef& def = defs[static_cast<std::size_t>(i)];
        if (!not_used(i)) {
            continue;
        }
        if (!matches_lane(def)) {
            continue;
        }
        fallback.push_back(i);
        if (def.tier == desired_tier) {
            preferred.push_back(i);
        } else if (def.tier == PickupTier::Uncommon || def.tier == PickupTier::Rare ||
                   def.tier == PickupTier::Common) {
            relaxed_tier.push_back(i);
        }
    }

    const std::vector<int>* bucket = nullptr;
    if (!preferred.empty()) {
        bucket = &preferred;
    } else if (!relaxed_tier.empty()) {
        bucket = &relaxed_tier;
    } else if (!fallback.empty()) {
        bucket = &fallback;
    } else {
        return -1;
    }

    return (*bucket)[static_cast<std::size_t>(std::rand()) % bucket->size()];
}

void clamp_selection(BattleState& battle) {
    const int max_index = std::max(0, static_cast<int>(battle.shop_offers.size()) - 1);
    battle.shop_selection = std::clamp(battle.shop_selection, 0, max_index);
}

void refill_active_from_stash(BattleState& battle) {
    if (battle.weapons.size() < static_cast<std::size_t>(battle.weapon_slots) &&
        !battle.weapon_stash.empty()) {
        battle.weapons.push_back(battle.weapon_stash.front());
        battle.weapon_stash.erase(battle.weapon_stash.begin());
    }
}

} // namespace

void reroll_shop(BattleState& battle) {
    battle.shop_offers.clear();
    battle.shop_offers.reserve(5);
    std::vector<int> used_defs;
    used_defs.reserve(5);
    const std::array<bool, 5> weapon_slots = {true, false, true, false, true};
    const std::array<bool, 5> premium_slots = {false, false, false, false, true};
    for (int i = 0; i < 5; ++i) {
        const int def_index =
            choose_shop_pickup_index(battle, weapon_slots[static_cast<std::size_t>(i)],
                                     premium_slots[static_cast<std::size_t>(i)], used_defs);
        if (def_index < 0) {
            continue;
        }
        used_defs.push_back(def_index);
        battle.shop_offers.push_back({def_index, price_for_pickup(pickup_def(def_index)), false});
    }
    clamp_selection(battle);
}

void enter_shop(BattleState& battle) {
    battle.phase = BattlePhase::Shop;
    battle.inventory_open = false;
    battle.shop_selection = 0;
    battle.shop_reroll_cost = std::max(6, 8 + battle.current_level_index * 3);
    if (battle.shop_offers.empty()) {
        reroll_shop(battle);
    }
}

bool buy_selected_shop_offer(BattleState& battle) {
    clamp_selection(battle);
    if (battle.shop_offers.empty()) {
        return false;
    }
    ShopOffer& offer = battle.shop_offers[static_cast<std::size_t>(battle.shop_selection)];
    if (offer.sold || battle.gold < offer.price) {
        return false;
    }
    if (!apply_pickup_by_index(battle, offer.pickup_def_index, true)) {
        return false;
    }
    battle.gold -= offer.price;
    offer.sold = true;
    return true;
}

bool move_selected_inventory_item(BattleState& battle) {
    if (battle.inventory_selection < 0) {
        return false;
    }

    const int active_count = static_cast<int>(battle.weapons.size());
    const int idx = battle.inventory_selection;
    if (idx < active_count) {
        if (battle.weapon_stash.size() >= static_cast<std::size_t>(battle.weapon_stash_slots)) {
            return false;
        }
        battle.weapon_stash.push_back(battle.weapons[static_cast<std::size_t>(idx)]);
        battle.weapons.erase(battle.weapons.begin() + idx);
        battle.inventory_selection =
            std::max(0, std::min(battle.inventory_selection, active_count - 2));
        return true;
    }

    const int stash_index = idx - active_count;
    if (stash_index >= 0 && stash_index < static_cast<int>(battle.weapon_stash.size()) &&
        battle.weapons.size() < static_cast<std::size_t>(battle.weapon_slots)) {
        battle.weapons.push_back(battle.weapon_stash[static_cast<std::size_t>(stash_index)]);
        battle.weapon_stash.erase(battle.weapon_stash.begin() + stash_index);
        return true;
    }

    return false;
}

void sell_selected_inventory_item(BattleState& battle) {
    if (battle.inventory_selection < 0) {
        return;
    }
    const int idx = battle.inventory_selection;
    if (idx < static_cast<int>(battle.weapons.size())) {
        const Weapon& weapon = battle.weapons[static_cast<std::size_t>(idx)];
        battle.gold += 6 + static_cast<int>(weapon.attached_pickups.size()) * 3;
        battle.weapons.erase(battle.weapons.begin() + idx);
        refill_active_from_stash(battle);
        return;
    }

    const int stash_index = idx - static_cast<int>(battle.weapons.size());
    if (stash_index >= 0 && stash_index < static_cast<int>(battle.weapon_stash.size())) {
        const Weapon& weapon = battle.weapon_stash[static_cast<std::size_t>(stash_index)];
        battle.gold += 6 + static_cast<int>(weapon.attached_pickups.size()) * 3;
        battle.weapon_stash.erase(battle.weapon_stash.begin() + stash_index);
    }
}

bool advance_from_shop(BattleState& battle, Assets& assets) {
    battle.shop_offers.clear();
    battle.phase = BattlePhase::LevelIntro;
    battle.shop_selection = 0;
    battle.inventory_open = false;
    start_level(battle, (battle.current_level_index + 1) % static_cast<int>(levels().size()));
    Mix_PlayChannel(-1, assets.warp_start, 0);
    return true;
}
