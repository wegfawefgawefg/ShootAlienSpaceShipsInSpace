#include "pickup_defs.hpp"

#include <array>
#include <cstdlib>

namespace {

float random_unit() {
    return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
}

Weapon make_weapon(WeaponType type, WeaponFixture fixture, bool automatic, int projectile_tile,
                   int projectile_count, float cooldown, float projectile_speed,
                   float projectile_life, float projectile_radius, float damage,
                   float spread_degrees, const char* name) {
    Weapon weapon{};
    weapon.name = name;
    weapon.type = type;
    weapon.fixture = fixture;
    weapon.automatic = automatic;
    weapon.projectile_tile = projectile_tile;
    weapon.projectile_count = projectile_count;
    weapon.cooldown = cooldown;
    weapon.cooldown_timer = 0.0f;
    weapon.projectile_speed = projectile_speed;
    weapon.projectile_life = projectile_life;
    weapon.projectile_radius = projectile_radius;
    weapon.damage = damage;
    weapon.spread_degrees = spread_degrees;
    weapon.attached_pickups.clear();
    return weapon;
}

const std::vector<PickupDef> kPickupDefs = {
    {
        "twin_blaster",
        "Twin Blaster",
        "Adds a second center gun.",
        1,
        PickupTier::Common,
        PickupEffectType::GrantWeapon,
        make_weapon(WeaponType::Basic, WeaponFixture::Center, false, 1, 1, 0.16f, 1500.0f, 0.5f,
                    1.25f, 1.0f, 0.0f, "Twin Blaster"),
    },
    {
        "needle_cannon",
        "Needle Cannon",
        "Fast automatic shots with lighter damage.",
        3,
        PickupTier::Uncommon,
        PickupEffectType::GrantWeapon,
        make_weapon(WeaponType::Basic, WeaponFixture::Center, true, 3, 1, 0.06f, 1750.0f, 0.4f,
                    1.0f, 0.55f, 0.0f, "Needle Cannon"),
    },
    {
        "sidecar_spread",
        "Sidecar Spread",
        "Evenly spread side mount with light projectiles.",
        5,
        PickupTier::Common,
        PickupEffectType::GrantWeapon,
        make_weapon(WeaponType::Basic, WeaponFixture::EvenlySpread, false, 5, 3, 0.20f, 1450.0f,
                    0.52f, 1.15f, 0.6f, 0.0f, "Sidecar Spread"),
    },
    {
        "missile_pod",
        "Missile Pod",
        "Heavy homing rockets with splash damage.",
        15,
        PickupTier::Rare,
        PickupEffectType::GrantWeapon,
        make_weapon(WeaponType::Missile, WeaponFixture::Center, false, 15, 1, 0.45f, 920.0f, 0.9f,
                    1.75f, 2.8f, 0.0f, "Missile Pod"),
    },
    {
        "prism_fan",
        "Prism Fan",
        "A wide splayed burst.",
        9,
        PickupTier::Uncommon,
        PickupEffectType::GrantWeapon,
        make_weapon(WeaponType::Basic, WeaponFixture::Splayed, false, 9, 3, 0.28f, 1480.0f, 0.52f,
                    1.15f, 0.7f, 12.0f, "Prism Fan"),
    },
    {
        "heavy_lance",
        "Heavy Lance",
        "A slower but harder-hitting center shot.",
        12,
        PickupTier::Uncommon,
        PickupEffectType::GrantWeapon,
        make_weapon(WeaponType::Basic, WeaponFixture::Center, false, 12, 1, 0.32f, 1700.0f, 0.6f,
                    1.45f, 2.0f, 0.0f, "Heavy Lance"),
    },
    {
        "rail_shot",
        "Rail Shot",
        "Ultra-fast piercing line shot.",
        10,
        PickupTier::Rare,
        PickupEffectType::GrantWeapon,
        make_weapon(WeaponType::Rail, WeaponFixture::Center, false, 10, 1, 0.34f, 2400.0f, 0.65f,
                    1.0f, 1.8f, 0.0f, "Rail Shot"),
    },
    {
        "arc_caster",
        "Arc Caster",
        "A hard-swaying shot that weaves through space.",
        11,
        PickupTier::Uncommon,
        PickupEffectType::GrantWeapon,
        make_weapon(WeaponType::Arc, WeaponFixture::Center, false, 11, 1, 0.18f, 1200.0f, 0.75f,
                    1.2f, 0.9f, 0.0f, "Arc Caster"),
    },
    {
        "proximity_mine",
        "Proximity Mine",
        "A parked mine that detonates on approach.",
        14,
        PickupTier::Rare,
        PickupEffectType::GrantWeapon,
        make_weapon(WeaponType::Mine, WeaponFixture::Center, false, 14, 1, 0.42f, 560.0f, 1.4f,
                    1.8f, 2.4f, 0.0f, "Proximity Mine"),
    },
    {
        "swarm_rack",
        "Swarm Rack",
        "Twin missiles with weaker damage but wider lane coverage.",
        16,
        PickupTier::Rare,
        PickupEffectType::GrantWeapon,
        make_weapon(WeaponType::Missile, WeaponFixture::EvenlySpread, false, 16, 2, 0.58f, 860.0f,
                    1.0f, 1.4f, 1.45f, 0.0f, "Swarm Rack"),
    },
    {
        "wave_splitter",
        "Wave Splitter",
        "A two-shot arc fan that bends across the screen.",
        17,
        PickupTier::Uncommon,
        PickupEffectType::GrantWeapon,
        make_weapon(WeaponType::Arc, WeaponFixture::Splayed, false, 17, 2, 0.22f, 1180.0f, 0.85f,
                    1.1f, 0.72f, 16.0f, "Wave Splitter"),
    },
    {
        "mine_net",
        "Mine Net",
        "Side-mounted mines that lock down a wider area.",
        18,
        PickupTier::Rare,
        PickupEffectType::GrantWeapon,
        make_weapon(WeaponType::Mine, WeaponFixture::EvenlySpread, false, 18, 2, 0.55f, 520.0f,
                    1.5f, 1.55f, 1.55f, 0.0f, "Mine Net"),
    },
    {
        "pierce_repeater",
        "Pierce Repeater",
        "Automatic rail bursts with lighter per-shot damage.",
        19,
        PickupTier::Rare,
        PickupEffectType::GrantWeapon,
        make_weapon(WeaponType::Rail, WeaponFixture::Center, true, 19, 1, 0.11f, 2200.0f, 0.6f,
                    0.95f, 0.72f, 0.0f, "Pierce Repeater"),
    },
    {
        "chain_gun",
        "Chain Gun",
        "An automatic stream with weaker rounds.",
        2,
        PickupTier::Uncommon,
        PickupEffectType::GrantWeapon,
        make_weapon(WeaponType::Basic, WeaponFixture::Center, true, 2, 1, 0.045f, 1650.0f, 0.42f,
                    1.0f, 0.45f, 0.0f, "Chain Gun"),
    },
    {
        "hair_trigger",
        "Hair Trigger",
        "Faster cooldown, lower damage.",
        24,
        PickupTier::Common,
        PickupEffectType::UpgradeRandomRate,
        {},
        0.72f,
        0.86f,
    },
    {
        "heavy_slugs",
        "Heavy Slugs",
        "Higher damage, slower cadence.",
        25,
        PickupTier::Common,
        PickupEffectType::UpgradeRandomDamage,
        {},
        1.35f,
        1.18f,
    },
    {
        "auto_retrofit",
        "Auto Retrofit",
        "Makes a random weapon automatic with a damage penalty.",
        26,
        PickupTier::Uncommon,
        PickupEffectType::UpgradeRandomAuto,
        {},
        0.84f,
    },
    {
        "scatter_caps",
        "Scatter Caps",
        "More pellets, less damage per pellet.",
        27,
        PickupTier::Uncommon,
        PickupEffectType::UpgradeRandomSpread,
        {},
        0.74f,
        6.0f,
    },
    {
        "hull_patch",
        "Hull Patch",
        "Gain one extra life.",
        31,
        PickupTier::Rare,
        PickupEffectType::ExtraLife,
        {},
        1.0f,
    },
    {
        "magnet_core",
        "Magnet Core",
        "Pull pickups from farther away.",
        28,
        PickupTier::Common,
        PickupEffectType::PickupMagnet,
        {},
        18.0f,
    },
    {
        "glass_reactor",
        "Glass Reactor",
        "All guns hit harder, but your hurtbox grows.",
        29,
        PickupTier::Rare,
        PickupEffectType::GlassReactor,
        {},
        1.18f,
        0.4f,
    },
    {
        "stability_fins",
        "Stability Fins",
        "Calmer impact shake, slightly slower projectiles.",
        30,
        PickupTier::Common,
        PickupEffectType::StabilityFins,
        {},
        0.78f,
        0.94f,
    },
};

const std::array<PickupTier, 5> kCommonRolls = {
    PickupTier::Common,   PickupTier::Common, PickupTier::Common,
    PickupTier::Uncommon, PickupTier::Rare,
};

const std::array<PickupTier, 6> kBossRolls = {
    PickupTier::Boss,     PickupTier::Rare, PickupTier::Rare,
    PickupTier::Uncommon, PickupTier::Boss, PickupTier::Rare,
};

} // namespace

const std::vector<PickupDef>& pickup_defs() {
    return kPickupDefs;
}

const PickupDef& pickup_def(int index) {
    return kPickupDefs.at(static_cast<std::size_t>(index));
}

const char* pickup_tier_name(PickupTier tier) {
    switch (tier) {
    case PickupTier::Common:
        return "common";
    case PickupTier::Uncommon:
        return "uncommon";
    case PickupTier::Rare:
        return "rare";
    case PickupTier::Boss:
        return "boss";
    case PickupTier::Cursed:
        return "cursed";
    }
    return "?";
}

int roll_pickup_drop(const Enemy& enemy) {
    const float chance =
        enemy.is_boss ? 1.0f : (0.15f + std::min(0.20f, (enemy.max_hp - 1.0f) * 0.05f));
    if (!enemy.is_boss && random_unit() > chance) {
        return -1;
    }

    const PickupTier desired =
        enemy.is_boss ? kBossRolls[static_cast<std::size_t>(std::rand()) % kBossRolls.size()]
                      : kCommonRolls[static_cast<std::size_t>(std::rand()) % kCommonRolls.size()];

    std::vector<int> matches;
    for (std::size_t i = 0; i < kPickupDefs.size(); ++i) {
        const PickupDef& def = kPickupDefs[i];
        if (def.tier == desired || (enemy.is_boss && def.tier == PickupTier::Rare)) {
            matches.push_back(static_cast<int>(i));
        }
    }

    if (matches.empty()) {
        return -1;
    }
    return matches[static_cast<std::size_t>(std::rand()) % matches.size()];
}
