#include "pickups.hpp"

#include "pickup_defs.hpp"

#include <SDL2/SDL_mixer.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace {

constexpr float PICKUP_MAX_AGE = 18.0f;
constexpr float GOLD_MAX_AGE = 16.0f;

float random_unit() {
    return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
}

float random_range(float min_value, float max_value) {
    return min_value + (max_value - min_value) * random_unit();
}

int random_index(int count) {
    return std::rand() % count;
}

bool add_weapon_to_loadout(BattleState& battle, Weapon weapon) {
    if (battle.weapons.size() < static_cast<std::size_t>(battle.weapon_slots)) {
        battle.weapons.push_back(std::move(weapon));
        return true;
    }
    if (battle.weapon_stash.size() < static_cast<std::size_t>(battle.weapon_stash_slots)) {
        battle.weapon_stash.push_back(std::move(weapon));
        return true;
    }
    return false;
}

bool is_pickup_in_safe_band(Vec2 pos) {
    return pos.x >= 24.0f && pos.x <= GAME_WIDTH - 24.0f && pos.y >= 24.0f &&
           pos.y <= GAME_HEIGHT - 42.0f;
}

Vec2 pickup_safe_target(Vec2 pos) {
    return {clampf(pos.x, 28.0f, GAME_WIDTH - 28.0f), clampf(pos.y, 28.0f, GAME_HEIGHT * 0.7f)};
}

int pick_random_weapon_index(const BattleState& battle, bool prefer_non_auto) {
    std::vector<int> candidates;
    for (std::size_t i = 0; i < battle.weapons.size(); ++i) {
        if (!prefer_non_auto || !battle.weapons[i].automatic) {
            candidates.push_back(static_cast<int>(i));
        }
    }
    if (candidates.empty()) {
        for (std::size_t i = 0; i < battle.weapons.size(); ++i) {
            candidates.push_back(static_cast<int>(i));
        }
    }
    if (candidates.empty()) {
        return -1;
    }
    return candidates[static_cast<std::size_t>(std::rand()) % candidates.size()];
}

void remember_pickup(BattleState& battle, int def_index) {
    battle.owned_pickups.push_back(def_index);
    battle.recent_pickups.push_back(def_index);
    if (battle.recent_pickups.size() > 10) {
        battle.recent_pickups.erase(battle.recent_pickups.begin());
    }
}

void attach_pickup_to_weapon(Weapon& weapon, int def_index) {
    weapon.attached_pickups.push_back(def_index);
}

} // namespace

bool apply_pickup_by_index(BattleState& battle, int def_index, bool from_shop) {
    const PickupDef& def = pickup_def(def_index);
    switch (def.effect) {
    case PickupEffectType::GrantWeapon: {
        Weapon weapon = def.weapon;
        if (!add_weapon_to_loadout(battle, weapon)) {
            if (from_shop) {
                return false;
            }
            battle.gold += 4;
            battle.gold_gain_flash = 1.0f;
        }
        break;
    }
    case PickupEffectType::UpgradeRandomRate: {
        const int index = pick_random_weapon_index(battle, false);
        if (index >= 0) {
            Weapon& weapon = battle.weapons[static_cast<std::size_t>(index)];
            weapon.cooldown = std::max(0.03f, weapon.cooldown * def.scalar_a);
            weapon.damage *= def.scalar_b;
            attach_pickup_to_weapon(weapon, def_index);
        }
        break;
    }
    case PickupEffectType::UpgradeRandomDamage: {
        const int index = pick_random_weapon_index(battle, false);
        if (index >= 0) {
            Weapon& weapon = battle.weapons[static_cast<std::size_t>(index)];
            weapon.damage *= def.scalar_a;
            weapon.cooldown *= def.scalar_b;
            attach_pickup_to_weapon(weapon, def_index);
        }
        break;
    }
    case PickupEffectType::UpgradeRandomAuto: {
        const int index = pick_random_weapon_index(battle, true);
        if (index >= 0) {
            Weapon& weapon = battle.weapons[static_cast<std::size_t>(index)];
            weapon.automatic = true;
            weapon.damage *= def.scalar_a;
            weapon.cooldown = std::max(0.04f, weapon.cooldown * 0.88f);
            attach_pickup_to_weapon(weapon, def_index);
        }
        break;
    }
    case PickupEffectType::UpgradeRandomSpread: {
        const int index = pick_random_weapon_index(battle, false);
        if (index >= 0) {
            Weapon& weapon = battle.weapons[static_cast<std::size_t>(index)];
            weapon.projectile_count = std::min(6, weapon.projectile_count + 2);
            weapon.damage *= def.scalar_a;
            weapon.spread_degrees += def.scalar_b;
            if (weapon.fixture == WeaponFixture::Center) {
                weapon.fixture = WeaponFixture::Splayed;
            }
            attach_pickup_to_weapon(weapon, def_index);
        }
        break;
    }
    case PickupEffectType::UpgradeRandomPierce: {
        const int index = pick_random_weapon_index(battle, false);
        if (index >= 0) {
            Weapon& weapon = battle.weapons[static_cast<std::size_t>(index)];
            weapon.pierce += 1;
            weapon.damage *= def.scalar_a;
            attach_pickup_to_weapon(weapon, def_index);
        }
        break;
    }
    case PickupEffectType::UpgradeRandomRicochet: {
        const int index = pick_random_weapon_index(battle, false);
        if (index >= 0) {
            Weapon& weapon = battle.weapons[static_cast<std::size_t>(index)];
            weapon.ricochet += 1;
            weapon.cooldown *= def.scalar_a;
            attach_pickup_to_weapon(weapon, def_index);
        }
        break;
    }
    case PickupEffectType::UpgradeRandomTracking: {
        const int index = pick_random_weapon_index(battle, false);
        if (index >= 0) {
            Weapon& weapon = battle.weapons[static_cast<std::size_t>(index)];
            weapon.homing_turn = std::max(weapon.homing_turn, 0.0f) + def.scalar_a;
            weapon.projectile_speed *= def.scalar_b;
            attach_pickup_to_weapon(weapon, def_index);
        }
        break;
    }
    case PickupEffectType::UpgradeRandomBlast: {
        const int index = pick_random_weapon_index(battle, false);
        if (index >= 0) {
            Weapon& weapon = battle.weapons[static_cast<std::size_t>(index)];
            weapon.explosion_radius = std::max(weapon.explosion_radius, 0.0f) + def.scalar_a;
            weapon.cooldown *= def.scalar_b;
            attach_pickup_to_weapon(weapon, def_index);
        }
        break;
    }
    case PickupEffectType::UpgradeRandomVelocity: {
        const int index = pick_random_weapon_index(battle, false);
        if (index >= 0) {
            Weapon& weapon = battle.weapons[static_cast<std::size_t>(index)];
            weapon.projectile_speed *= def.scalar_a;
            weapon.damage *= def.scalar_b;
            attach_pickup_to_weapon(weapon, def_index);
        }
        break;
    }
    case PickupEffectType::UpgradeRandomBurstMode: {
        const int index = pick_random_weapon_index(battle, false);
        if (index >= 0) {
            Weapon& weapon = battle.weapons[static_cast<std::size_t>(index)];
            weapon.type = WeaponType::Burst;
            weapon.burst_count = std::max(3, weapon.burst_count + static_cast<int>(def.scalar_a));
            weapon.burst_interval = weapon.burst_interval <= 0.0f
                                        ? def.scalar_b
                                        : std::min(weapon.burst_interval, def.scalar_b);
            weapon.damage *= 0.92f;
            weapon.cooldown *= 1.08f;
            attach_pickup_to_weapon(weapon, def_index);
        }
        break;
    }
    case PickupEffectType::UpgradeRandomBeamLens: {
        const int index = pick_random_weapon_index(battle, false);
        if (index >= 0) {
            Weapon& weapon = battle.weapons[static_cast<std::size_t>(index)];
            weapon.type = WeaponType::Beam;
            weapon.beam_duration = std::max(weapon.beam_duration, 0.14f) + def.scalar_a;
            weapon.beam_width = std::max(weapon.beam_width, 5.0f) + def.scalar_b;
            weapon.beam_length = std::max(weapon.beam_length, 170.0f) + 14.0f;
            weapon.beam_tick_interval = 0.04f;
            weapon.projectile_count = 1;
            weapon.spread_degrees = 0.0f;
            weapon.damage *= 0.86f;
            weapon.cooldown *= 1.12f;
            attach_pickup_to_weapon(weapon, def_index);
        }
        break;
    }
    case PickupEffectType::UpgradeRandomBurstCount: {
        const int index = pick_random_weapon_index(battle, false);
        if (index >= 0) {
            Weapon& weapon = battle.weapons[static_cast<std::size_t>(index)];
            if (weapon.type == WeaponType::Burst) {
                weapon.burst_count += static_cast<int>(def.scalar_a);
                weapon.damage *= def.scalar_b;
                attach_pickup_to_weapon(weapon, def_index);
            }
        }
        break;
    }
    case PickupEffectType::UpgradeRandomBurstCadence: {
        const int index = pick_random_weapon_index(battle, false);
        if (index >= 0) {
            Weapon& weapon = battle.weapons[static_cast<std::size_t>(index)];
            if (weapon.type == WeaponType::Burst) {
                weapon.burst_interval = weapon.burst_interval <= 0.0f
                                            ? def.scalar_a
                                            : weapon.burst_interval * def.scalar_a;
                weapon.cooldown *= def.scalar_b;
                attach_pickup_to_weapon(weapon, def_index);
            }
        }
        break;
    }
    case PickupEffectType::UpgradeRandomBeamWidth: {
        const int index = pick_random_weapon_index(battle, false);
        if (index >= 0) {
            Weapon& weapon = battle.weapons[static_cast<std::size_t>(index)];
            if (weapon.type == WeaponType::Beam) {
                weapon.beam_width += def.scalar_a;
                weapon.damage *= def.scalar_b;
                attach_pickup_to_weapon(weapon, def_index);
            }
        }
        break;
    }
    case PickupEffectType::UpgradeRandomBeamLength: {
        const int index = pick_random_weapon_index(battle, false);
        if (index >= 0) {
            Weapon& weapon = battle.weapons[static_cast<std::size_t>(index)];
            if (weapon.type == WeaponType::Beam) {
                weapon.beam_length += def.scalar_a;
                weapon.beam_duration += def.scalar_b;
                attach_pickup_to_weapon(weapon, def_index);
            }
        }
        break;
    }
    case PickupEffectType::UpgradeRandomBeamDamage: {
        const int index = pick_random_weapon_index(battle, false);
        if (index >= 0) {
            Weapon& weapon = battle.weapons[static_cast<std::size_t>(index)];
            if (weapon.type == WeaponType::Beam) {
                weapon.damage *= def.scalar_a;
                weapon.cooldown *= def.scalar_b;
                attach_pickup_to_weapon(weapon, def_index);
            }
        }
        break;
    }
    case PickupEffectType::UpgradeRandomOrbitalCount: {
        const int index = pick_random_weapon_index(battle, false);
        if (index >= 0) {
            Weapon& weapon = battle.weapons[static_cast<std::size_t>(index)];
            if (weapon.type == WeaponType::Orbital) {
                weapon.orbital_count += static_cast<int>(def.scalar_a);
                weapon.orbital_radius += def.scalar_b;
                attach_pickup_to_weapon(weapon, def_index);
            }
        }
        break;
    }
    case PickupEffectType::ExtraLife:
        battle.lives += static_cast<int>(def.scalar_a);
        break;
    case PickupEffectType::ExtraWeaponSlot:
        battle.weapon_slots += static_cast<int>(def.scalar_a);
        break;
    case PickupEffectType::ExtraStashSlot:
        battle.weapon_stash_slots += static_cast<int>(def.scalar_a);
        break;
    case PickupEffectType::PickupMagnet:
        battle.pickup_magnet_radius += def.scalar_a;
        break;
    case PickupEffectType::GlassReactor:
        for (Weapon& weapon : battle.weapons) {
            weapon.damage *= def.scalar_a;
        }
        battle.ship.radius += def.scalar_b;
        break;
    case PickupEffectType::StabilityFins:
        battle.camera_shake_scale *= def.scalar_a;
        battle.projectile_speed_scale *= def.scalar_b;
        break;
    }

    remember_pickup(battle, def_index);
    return true;
}

void maybe_spawn_enemy_drop(BattleState& battle, const Enemy& enemy) {
    const int def_index = roll_pickup_drop(enemy);
    if (def_index < 0) {
        return;
    }

    PickupActor pickup{};
    pickup.def_index = def_index;
    pickup.pos = enemy.pos;
    pickup.vel = {random_range(-20.0f, 20.0f), random_range(-28.0f, -10.0f)};
    pickup.guide_target = pickup_safe_target(enemy.pos);
    pickup.guided = !is_pickup_in_safe_band(enemy.pos);
    pickup.base_height = enemy.is_boss ? 3.0f : 2.2f;
    pickup.height = pickup.base_height;
    pickup.target_height = pickup.base_height;
    battle.pickups.push_back(pickup);
}

void maybe_spawn_enemy_gold(BattleState& battle, const Enemy& enemy, bool on_death) {
    int drops = 0;
    if (on_death) {
        drops = enemy.is_boss ? 8 : std::max(1, static_cast<int>(std::ceil(enemy.max_hp * 0.7f)));
    } else if (enemy.max_hp >= 4.0f && random_unit() < 0.16f) {
        drops = 1;
    }

    for (int i = 0; i < drops; ++i) {
        const bool large = enemy.is_boss && (i % 3 == 0);
        const bool medium = !large && (enemy.max_hp >= 2.5f || random_unit() < 0.35f);
        GoldActor gold{};
        gold.pos = enemy.pos;
        gold.vel = {random_range(-28.0f, 28.0f), random_range(-34.0f, -8.0f)};
        gold.guide_target = pickup_safe_target(enemy.pos);
        gold.value = large ? 9 : (medium ? 3 : 1);
        gold.tile = large ? 2 : (medium ? 1 : 0);
        gold.guided = !is_pickup_in_safe_band(enemy.pos);
        gold.base_height = 1.2f + static_cast<float>(gold.tile) * 0.35f;
        gold.height = gold.base_height;
        gold.target_height = gold.base_height;
        battle.gold_pickups.push_back(gold);
    }
}

void update_pickups(BattleState& battle, Assets& assets, float dt) {
    battle.gold_gain_flash = std::max(0.0f, battle.gold_gain_flash - dt * 2.5f);

    for (PickupActor& pickup : battle.pickups) {
        pickup.age += dt;
        pickup.target_height = pickup.base_height + std::sin(pickup.age * 7.0f) * 0.4f;
        pickup.height += (pickup.target_height - pickup.height) * std::min(1.0f, dt * 8.0f);

        const bool can_magnet =
            battle.player_active && vec2_length_sq(battle.ship.pos - pickup.pos) <=
                                        battle.pickup_magnet_radius * battle.pickup_magnet_radius;

        if (can_magnet) {
            const Vec2 delta = battle.ship.pos - pickup.pos;
            const float len = std::max(vec2_length(delta), 0.001f);
            const Vec2 dir = {delta.x / len, delta.y / len};
            pickup.vel += (dir * 240.0f - pickup.vel) * std::min(1.0f, dt * 8.0f);
        } else if (pickup.guided) {
            const Vec2 delta = pickup.guide_target - pickup.pos;
            const float len_sq = vec2_length_sq(delta);
            if (len_sq < 36.0f) {
                pickup.guided = false;
            } else {
                const float len = std::max(vec2_length(delta), 0.001f);
                const Vec2 dir = {delta.x / len, delta.y / len};
                pickup.vel += (dir * 90.0f - pickup.vel) * std::min(1.0f, dt * 4.0f);
            }
        } else {
            const Vec2 drift = {std::sin(pickup.age * 2.5f) * 10.0f, 18.0f};
            pickup.vel += (drift - pickup.vel) * std::min(1.0f, dt * 2.0f);
        }

        pickup.pos += pickup.vel * dt;
    }

    for (std::size_t i = 0; i < battle.pickups.size();) {
        PickupActor& pickup = battle.pickups[i];
        bool collected = false;
        if (battle.player_active) {
            const float radius = pickup.radius + battle.ship.radius + 2.0f;
            if (vec2_length_sq(battle.ship.pos - pickup.pos) <= radius * radius) {
                if (apply_pickup_by_index(battle, pickup.def_index, false)) {
                    Mix_PlayChannel(
                        -1, assets.rock_hit_sounds[static_cast<std::size_t>(random_index(3))], 0);
                    collected = true;
                }
            }
        }

        const bool expired = pickup.age > PICKUP_MAX_AGE || pickup.pos.y > GAME_HEIGHT + 28.0f;
        if (collected || expired) {
            battle.pickups.erase(battle.pickups.begin() + static_cast<std::ptrdiff_t>(i));
            continue;
        }
        ++i;
    }

    for (GoldActor& gold : battle.gold_pickups) {
        gold.age += dt;
        gold.target_height = gold.base_height + std::sin(gold.age * 8.0f) * 0.25f;
        gold.height += (gold.target_height - gold.height) * std::min(1.0f, dt * 8.0f);

        const bool can_magnet =
            battle.player_active && vec2_length_sq(battle.ship.pos - gold.pos) <=
                                        battle.pickup_magnet_radius * battle.pickup_magnet_radius;

        if (can_magnet) {
            const Vec2 delta = battle.ship.pos - gold.pos;
            const float len = std::max(vec2_length(delta), 0.001f);
            const Vec2 dir = {delta.x / len, delta.y / len};
            gold.vel += (dir * 280.0f - gold.vel) * std::min(1.0f, dt * 10.0f);
        } else if (gold.guided) {
            const Vec2 delta = gold.guide_target - gold.pos;
            if (vec2_length_sq(delta) < 36.0f) {
                gold.guided = false;
            } else {
                const float len = std::max(vec2_length(delta), 0.001f);
                const Vec2 dir = {delta.x / len, delta.y / len};
                gold.vel += (dir * 96.0f - gold.vel) * std::min(1.0f, dt * 4.0f);
            }
        } else {
            const Vec2 drift = {std::sin(gold.age * 3.0f) * 8.0f, 22.0f};
            gold.vel += (drift - gold.vel) * std::min(1.0f, dt * 2.0f);
        }

        gold.pos += gold.vel * dt;
    }

    for (std::size_t i = 0; i < battle.gold_pickups.size();) {
        GoldActor& gold = battle.gold_pickups[i];
        bool collected = false;
        if (battle.player_active) {
            const float radius = gold.radius + battle.ship.radius + 2.0f;
            if (vec2_length_sq(battle.ship.pos - gold.pos) <= radius * radius) {
                battle.gold += gold.value;
                battle.gold_gain_flash = 1.0f;
                collected = true;
            }
        }

        const bool expired = gold.age > GOLD_MAX_AGE || gold.pos.y > GAME_HEIGHT + 28.0f;
        if (collected || expired) {
            battle.gold_pickups.erase(battle.gold_pickups.begin() + static_cast<std::ptrdiff_t>(i));
            continue;
        }
        ++i;
    }
}
