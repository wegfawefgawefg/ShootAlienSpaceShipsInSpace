#pragma once

#include "battle_types.hpp"

#include <string>
#include <vector>

enum class PickupTier {
    Common,
    Uncommon,
    Rare,
    Boss,
    Cursed,
};

enum class PickupEffectType {
    GrantWeapon,
    UpgradeRandomRate,
    UpgradeRandomDamage,
    UpgradeRandomAuto,
    UpgradeRandomSpread,
    ExtraLife,
    PickupMagnet,
    GlassReactor,
    StabilityFins,
};

struct PickupDef {
    std::string id{};
    std::string name{};
    std::string desc{};
    int icon_tile{0};
    PickupTier tier{PickupTier::Common};
    PickupEffectType effect{PickupEffectType::GrantWeapon};
    Weapon weapon{};
    float scalar_a{0.0f};
    float scalar_b{0.0f};
};

const std::vector<PickupDef>& pickup_defs();
const PickupDef& pickup_def(int index);
const char* pickup_tier_name(PickupTier tier);
int roll_pickup_drop(const Enemy& enemy);
