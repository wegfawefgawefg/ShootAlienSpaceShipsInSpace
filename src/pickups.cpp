#include "pickups.hpp"

#include "pickup_defs.hpp"

#include <SDL2/SDL_mixer.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace {

constexpr float PICKUP_MAX_AGE = 18.0f;

float random_unit() {
    return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
}

float random_range(float min_value, float max_value) {
    return min_value + (max_value - min_value) * random_unit();
}

int random_index(int count) {
    return std::rand() % count;
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
    battle.collected_pickups.push_back(def_index);
    if (battle.collected_pickups.size() > 10) {
        battle.collected_pickups.erase(battle.collected_pickups.begin());
    }
}

void apply_pickup(BattleState& battle, int def_index) {
    const PickupDef& def = pickup_def(def_index);
    switch (def.effect) {
    case PickupEffectType::GrantWeapon: {
        Weapon weapon = def.weapon;
        battle.weapons.push_back(weapon);
        break;
    }
    case PickupEffectType::UpgradeRandomRate: {
        const int index = pick_random_weapon_index(battle, false);
        if (index >= 0) {
            Weapon& weapon = battle.weapons[static_cast<std::size_t>(index)];
            weapon.cooldown = std::max(0.03f, weapon.cooldown * def.scalar_a);
            weapon.damage *= def.scalar_b;
        }
        break;
    }
    case PickupEffectType::UpgradeRandomDamage: {
        const int index = pick_random_weapon_index(battle, false);
        if (index >= 0) {
            Weapon& weapon = battle.weapons[static_cast<std::size_t>(index)];
            weapon.damage *= def.scalar_a;
            weapon.cooldown *= def.scalar_b;
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
        }
        break;
    }
    case PickupEffectType::ExtraLife:
        battle.lives += static_cast<int>(def.scalar_a);
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
}

} // namespace

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

void update_pickups(BattleState& battle, Assets& assets, float dt) {
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
                apply_pickup(battle, pickup.def_index);
                Mix_PlayChannel(
                    -1, assets.rock_hit_sounds[static_cast<std::size_t>(random_index(3))], 0);
                collected = true;
            }
        }

        const bool expired = pickup.age > PICKUP_MAX_AGE || pickup.pos.y > GAME_HEIGHT + 28.0f;
        if (collected || expired) {
            battle.pickups.erase(battle.pickups.begin() + static_cast<std::ptrdiff_t>(i));
            continue;
        }
        ++i;
    }
}
