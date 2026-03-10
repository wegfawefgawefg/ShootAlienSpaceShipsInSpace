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
    weapon.pierce = 0;
    weapon.ricochet = 0;
    weapon.homing_turn = 0.0f;
    weapon.wave_amplitude = 0.0f;
    weapon.wave_frequency = 0.0f;
    weapon.stop_age = 0.0f;
    weapon.explosion_radius = 0.0f;
    weapon.burst_count = 1;
    weapon.burst_interval = 0.0f;
    weapon.burst_remaining = 0;
    weapon.burst_timer = 0.0f;
    weapon.beam_duration = 0.0f;
    weapon.beam_width = 0.0f;
    weapon.beam_length = 0.0f;
    weapon.beam_tick_interval = 0.0f;
    weapon.orbital_count = 0;
    weapon.orbital_radius = 0.0f;
    weapon.orbital_speed = 0.0f;
    weapon.orbital_duration = 0.0f;
    switch (type) {
    case WeaponType::Missile:
        weapon.homing_turn = 12.0f;
        weapon.explosion_radius = 13.0f;
        break;
    case WeaponType::Rail:
        weapon.pierce = 4;
        break;
    case WeaponType::Arc:
        weapon.wave_amplitude = 250.0f;
        weapon.wave_frequency = 16.0f;
        break;
    case WeaponType::Mine:
        weapon.stop_age = 0.16f;
        weapon.explosion_radius = 18.0f;
        break;
    case WeaponType::Burst:
        weapon.burst_count = 3;
        weapon.burst_interval = 0.045f;
        break;
    case WeaponType::Beam:
        weapon.beam_duration = 0.16f;
        weapon.beam_width = 6.0f;
        weapon.beam_length = 180.0f;
        weapon.beam_tick_interval = 0.04f;
        break;
    case WeaponType::Orbital:
        weapon.orbital_count = 2;
        weapon.orbital_radius = 18.0f;
        weapon.orbital_speed = 160.0f;
        weapon.orbital_duration = 4.0f;
        break;
    case WeaponType::Basic:
        break;
    }
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
        "ricochet_blaster",
        "Ricochet Blaster",
        "Fast rounds that bank off side walls.",
        20,
        PickupTier::Uncommon,
        PickupEffectType::GrantWeapon,
        [] {
            Weapon weapon =
                make_weapon(WeaponType::Basic, WeaponFixture::Center, false, 20, 1, 0.16f, 1580.0f,
                            0.8f, 1.1f, 0.82f, 0.0f, "Ricochet Blaster");
            weapon.ricochet = 2;
            return weapon;
        }(),
    },
    {
        "hunter_rounds",
        "Hunter Rounds",
        "A basic gun with slight target tracking.",
        21,
        PickupTier::Uncommon,
        PickupEffectType::GrantWeapon,
        [] {
            Weapon weapon = make_weapon(WeaponType::Basic, WeaponFixture::Center, true, 21, 1,
                                        0.10f, 1460.0f, 0.55f, 1.0f, 0.62f, 0.0f, "Hunter Rounds");
            weapon.homing_turn = 4.5f;
            return weapon;
        }(),
    },
    {
        "bombard_shot",
        "Bombard Shot",
        "Slow explosive slugs with no spread.",
        22,
        PickupTier::Rare,
        PickupEffectType::GrantWeapon,
        [] {
            Weapon weapon = make_weapon(WeaponType::Basic, WeaponFixture::Center, false, 22, 1,
                                        0.36f, 980.0f, 0.9f, 1.5f, 1.8f, 0.0f, "Bombard Shot");
            weapon.explosion_radius = 11.0f;
            return weapon;
        }(),
    },
    {
        "pulse_drill",
        "Pulse Drill",
        "Three piercing slugs with a tight fan.",
        23,
        PickupTier::Rare,
        PickupEffectType::GrantWeapon,
        [] {
            Weapon weapon = make_weapon(WeaponType::Basic, WeaponFixture::Splayed, false, 23, 3,
                                        0.24f, 1720.0f, 0.55f, 1.0f, 0.85f, 7.0f, "Pulse Drill");
            weapon.pierce = 1;
            return weapon;
        }(),
    },
    {
        "seeker_fan",
        "Seeker Fan",
        "A three-missile fan that tracks into the lane.",
        24,
        PickupTier::Rare,
        PickupEffectType::GrantWeapon,
        make_weapon(WeaponType::Missile, WeaponFixture::Splayed, false, 24, 3, 0.62f, 820.0f, 1.1f,
                    1.2f, 1.0f, 12.0f, "Seeker Fan"),
    },
    {
        "skipshot_array",
        "Skipshot Array",
        "Three side-spaced bullets that bank off walls.",
        25,
        PickupTier::Uncommon,
        PickupEffectType::GrantWeapon,
        [] {
            Weapon weapon = make_weapon(WeaponType::Basic, WeaponFixture::EvenlySpread, true, 25, 3,
                                        0.14f, 1480.0f, 0.9f, 0.95f, 0.48f, 0.0f, "Skipshot Array");
            weapon.ricochet = 1;
            return weapon;
        }(),
    },
    {
        "tunnel_lance",
        "Tunnel Lance",
        "A slow drill shot that bores through targets.",
        26,
        PickupTier::Rare,
        PickupEffectType::GrantWeapon,
        [] {
            Weapon weapon = make_weapon(WeaponType::Basic, WeaponFixture::Center, false, 26, 1,
                                        0.30f, 1400.0f, 0.7f, 1.5f, 2.3f, 0.0f, "Tunnel Lance");
            weapon.pierce = 3;
            return weapon;
        }(),
    },
    {
        "mine_halo",
        "Mine Halo",
        "A splayed three-mine curtain for area denial.",
        27,
        PickupTier::Rare,
        PickupEffectType::GrantWeapon,
        make_weapon(WeaponType::Mine, WeaponFixture::Splayed, false, 27, 3, 0.70f, 500.0f, 1.6f,
                    1.5f, 1.3f, 14.0f, "Mine Halo"),
    },
    {
        "siege_arc",
        "Siege Arc",
        "Twin arcs that sway wide and stay alive longer.",
        28,
        PickupTier::Rare,
        PickupEffectType::GrantWeapon,
        make_weapon(WeaponType::Arc, WeaponFixture::EvenlySpread, false, 28, 2, 0.26f, 1100.0f,
                    1.1f, 1.15f, 0.84f, 0.0f, "Siege Arc"),
    },
    {
        "blast_fan",
        "Blast Fan",
        "Explosive shells in a short heavy spread.",
        29,
        PickupTier::Rare,
        PickupEffectType::GrantWeapon,
        [] {
            Weapon weapon = make_weapon(WeaponType::Basic, WeaponFixture::Splayed, false, 29, 3,
                                        0.34f, 1080.0f, 0.8f, 1.25f, 1.0f, 10.0f, "Blast Fan");
            weapon.explosion_radius = 8.0f;
            return weapon;
        }(),
    },
    {
        "burst_carbine",
        "Burst Carbine",
        "A clean three-round burst on each trigger pull.",
        30,
        PickupTier::Uncommon,
        PickupEffectType::GrantWeapon,
        [] {
            Weapon weapon = make_weapon(WeaponType::Burst, WeaponFixture::Center, false, 30, 1,
                                        0.26f, 1650.0f, 0.55f, 1.0f, 0.78f, 0.0f, "Burst Carbine");
            weapon.burst_count = 3;
            weapon.burst_interval = 0.045f;
            return weapon;
        }(),
    },
    {
        "scatter_burst",
        "Scatter Burst",
        "Short bursts with a wider spread fan.",
        31,
        PickupTier::Rare,
        PickupEffectType::GrantWeapon,
        [] {
            Weapon weapon = make_weapon(WeaponType::Burst, WeaponFixture::Splayed, false, 31, 3,
                                        0.34f, 1500.0f, 0.5f, 1.0f, 0.52f, 10.0f, "Scatter Burst");
            weapon.burst_count = 2;
            weapon.burst_interval = 0.06f;
            return weapon;
        }(),
    },
    {
        "ion_beam",
        "Ion Beam",
        "A narrow cutting beam with quick refresh.",
        32,
        PickupTier::Rare,
        PickupEffectType::GrantWeapon,
        [] {
            Weapon weapon = make_weapon(WeaponType::Beam, WeaponFixture::Center, true, 32, 1, 0.30f,
                                        0.0f, 0.0f, 0.0f, 0.52f, 0.0f, "Ion Beam");
            weapon.beam_duration = 0.18f;
            weapon.beam_width = 5.0f;
            weapon.beam_length = 185.0f;
            weapon.beam_tick_interval = 0.04f;
            return weapon;
        }(),
    },
    {
        "prism_beam",
        "Prism Beam",
        "A heavier beam with more width and a slower cycle.",
        33,
        PickupTier::Boss,
        PickupEffectType::GrantWeapon,
        [] {
            Weapon weapon = make_weapon(WeaponType::Beam, WeaponFixture::Center, false, 33, 1,
                                        0.55f, 0.0f, 0.0f, 0.0f, 0.95f, 0.0f, "Prism Beam");
            weapon.beam_duration = 0.28f;
            weapon.beam_width = 9.0f;
            weapon.beam_length = 210.0f;
            weapon.beam_tick_interval = 0.05f;
            return weapon;
        }(),
    },
    {
        "orbit_ring",
        "Orbit Ring",
        "Two orbitals that grind through nearby ships.",
        34,
        PickupTier::Rare,
        PickupEffectType::GrantWeapon,
        [] {
            Weapon weapon = make_weapon(WeaponType::Orbital, WeaponFixture::Center, true, 34, 1,
                                        0.65f, 0.0f, 0.0f, 1.3f, 0.72f, 0.0f, "Orbit Ring");
            weapon.orbital_count = 2;
            weapon.orbital_radius = 18.0f;
            weapon.orbital_speed = 170.0f;
            weapon.orbital_duration = 4.2f;
            return weapon;
        }(),
    },
    {
        "saw_halo",
        "Saw Halo",
        "A heavier three-orbital halo with shorter life.",
        35,
        PickupTier::Boss,
        PickupEffectType::GrantWeapon,
        [] {
            Weapon weapon = make_weapon(WeaponType::Orbital, WeaponFixture::Center, false, 35, 1,
                                        0.95f, 0.0f, 0.0f, 1.45f, 1.0f, 0.0f, "Saw Halo");
            weapon.orbital_count = 3;
            weapon.orbital_radius = 24.0f;
            weapon.orbital_speed = 220.0f;
            weapon.orbital_duration = 3.6f;
            return weapon;
        }(),
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
        "shatter_tips",
        "Shatter Tips",
        "Piercing rounds, but lower raw damage.",
        6,
        PickupTier::Uncommon,
        PickupEffectType::UpgradeRandomPierce,
        {},
        0.9f,
        1.0f,
    },
    {
        "wall_skippers",
        "Wall Skippers",
        "Adds side ricochet with a slower cadence.",
        7,
        PickupTier::Uncommon,
        PickupEffectType::UpgradeRandomRicochet,
        {},
        1.14f,
    },
    {
        "tracking_package",
        "Tracking Package",
        "Adds target seeking but trims shot speed.",
        8,
        PickupTier::Rare,
        PickupEffectType::UpgradeRandomTracking,
        {},
        4.0f,
        0.9f,
    },
    {
        "volatile_payload",
        "Volatile Payload",
        "Explosive impact, slower reload.",
        13,
        PickupTier::Rare,
        PickupEffectType::UpgradeRandomBlast,
        {},
        10.0f,
        1.16f,
    },
    {
        "overpressure_coils",
        "Overpressure Coils",
        "Faster shot speed and a little more damage.",
        4,
        PickupTier::Common,
        PickupEffectType::UpgradeRandomVelocity,
        {},
        1.2f,
        1.08f,
    },
    {
        "ghost_barrel",
        "Ghost Barrel",
        "Slightly faster projectiles, less raw damage.",
        34,
        PickupTier::Common,
        PickupEffectType::UpgradeRandomVelocity,
        {},
        1.08f,
        0.95f,
    },
    {
        "seeker_glass",
        "Seeker Glass",
        "A stronger tracking package with a sharper speed penalty.",
        35,
        PickupTier::Rare,
        PickupEffectType::UpgradeRandomTracking,
        {},
        7.0f,
        0.78f,
    },
    {
        "payload_baffles",
        "Payload Baffles",
        "Bigger blast radius with even slower cadence.",
        14,
        PickupTier::Rare,
        PickupEffectType::UpgradeRandomBlast,
        {},
        14.0f,
        1.28f,
    },
    {
        "burst_retrofit",
        "Burst Retrofit",
        "Converts a random weapon to burst fire.",
        8,
        PickupTier::Uncommon,
        PickupEffectType::UpgradeRandomBurstMode,
        {},
        3.0f,
        0.05f,
    },
    {
        "beam_lens",
        "Beam Lens",
        "Converts a random weapon into a narrow beam.",
        9,
        PickupTier::Rare,
        PickupEffectType::UpgradeRandomBeamLens,
        {},
        0.08f,
        2.0f,
    },
    {
        "burst_amplifier",
        "Burst Amplifier",
        "Adds an extra shot to a burst sequence.",
        10,
        PickupTier::Uncommon,
        PickupEffectType::UpgradeRandomBurstCount,
        {},
        1.0f,
        0.92f,
    },
    {
        "rapid_burst_link",
        "Rapid Burst Link",
        "Tightens burst spacing and trims cooldown slightly.",
        11,
        PickupTier::Uncommon,
        PickupEffectType::UpgradeRandomBurstCadence,
        {},
        0.75f,
        0.94f,
    },
    {
        "wide_lens",
        "Wide Lens",
        "Makes a beam wider but less damaging per tick.",
        12,
        PickupTier::Rare,
        PickupEffectType::UpgradeRandomBeamWidth,
        {},
        2.5f,
        0.88f,
    },
    {
        "focal_array",
        "Focal Array",
        "Extends beam reach and duration.",
        13,
        PickupTier::Rare,
        PickupEffectType::UpgradeRandomBeamLength,
        {},
        32.0f,
        0.05f,
    },
    {
        "melter_core",
        "Melter Core",
        "A beam upgrade that pushes tick damage hard.",
        36,
        PickupTier::Boss,
        PickupEffectType::UpgradeRandomBeamDamage,
        {},
        1.35f,
        1.10f,
    },
    {
        "orbital_forge",
        "Orbital Forge",
        "Adds another orbital and widens the ring.",
        37,
        PickupTier::Rare,
        PickupEffectType::UpgradeRandomOrbitalCount,
        {},
        1.0f,
        4.0f,
    },
    {
        "drill_jacket",
        "Drill Jacket",
        "A second point of pierce but reduced damage.",
        15,
        PickupTier::Uncommon,
        PickupEffectType::UpgradeRandomPierce,
        {},
        0.84f,
        0.0f,
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
        "aux_mount",
        "Aux Mount",
        "Gain one active weapon slot.",
        32,
        PickupTier::Rare,
        PickupEffectType::ExtraWeaponSlot,
        {},
        1.0f,
    },
    {
        "cargo_rack",
        "Cargo Rack",
        "Gain one stash slot.",
        33,
        PickupTier::Common,
        PickupEffectType::ExtraStashSlot,
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
