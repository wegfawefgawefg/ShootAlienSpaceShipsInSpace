#include "player_weapons.hpp"

#include <SDL2/SDL_mixer.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace {

int random_index(int count) {
    return std::rand() % count;
}

Vec2 nearest_enemy_dir(const BattleState& battle, Vec2 from, Vec2 fallback) {
    float best_dist = 1.0e30f;
    Vec2 best = fallback;
    for (const Enemy& enemy : battle.enemies) {
        const Vec2 delta = enemy.pos - from;
        const float dist = vec2_length_sq(delta);
        if (dist < best_dist) {
            best_dist = dist;
            const float len = std::max(vec2_length(delta), 0.001f);
            best = {delta.x / len, delta.y / len};
        }
    }
    return best;
}

void spawn_player_bullet(BattleState& battle, const Weapon& weapon, Vec2 spawn_pos,
                         float angle_offset_deg, Assets& assets) {
    Mix_PlayChannel(-1, assets.laser_sounds[static_cast<std::size_t>(random_index(3))], 0);
    const float angle = (-90.0f + angle_offset_deg) * 0.0174532925f;
    const Vec2 dir = {std::cos(angle), std::sin(angle)};
    Vec2 vel = dir * (weapon.projectile_speed * battle.projectile_speed_scale);
    float homing_turn = 0.0f;
    float wave_amplitude = 0.0f;
    float wave_frequency = 0.0f;
    float stop_age = 0.0f;
    int pierce = 0;

    switch (weapon.type) {
    case WeaponType::Missile:
        homing_turn = 12.0f;
        break;
    case WeaponType::Rail:
        pierce = 4;
        break;
    case WeaponType::Arc:
        wave_amplitude = 250.0f;
        wave_frequency = 16.0f;
        break;
    case WeaponType::Mine:
        vel = dir * (weapon.projectile_speed * 0.28f * battle.projectile_speed_scale);
        stop_age = 0.16f;
        break;
    case WeaponType::Basic:
        break;
    }

    battle.player_bullets.push_back({
        spawn_pos,
        vel,
        dir,
        0.0f,
        weapon.projectile_radius,
        weapon.damage,
        weapon.projectile_life,
        3.0f,
        3.0f,
        3.0f,
        homing_turn,
        wave_amplitude,
        wave_frequency,
        stop_age,
        pierce,
        weapon.projectile_tile,
        weapon.type,
    });
}

} // namespace

void fire_player_weapons(BattleState& battle, Assets& assets, bool trigger_pressed,
                         bool trigger_held) {
    for (Weapon& weapon : battle.weapons) {
        if (weapon.cooldown_timer > 0.0f) {
            continue;
        }
        if ((weapon.automatic && !trigger_held) || (!weapon.automatic && !trigger_pressed)) {
            continue;
        }

        if (weapon.fixture == WeaponFixture::Center) {
            spawn_player_bullet(battle, weapon, battle.ship.pos, 0.0f, assets);
        } else if (weapon.fixture == WeaponFixture::EvenlySpread) {
            const float spacing = 6.0f;
            const float start =
                -spacing * (static_cast<float>(weapon.projectile_count) - 1.0f) * 0.5f;
            for (int i = 0; i < weapon.projectile_count; ++i) {
                const float offset = start + spacing * static_cast<float>(i);
                spawn_player_bullet(battle, weapon, {battle.ship.pos.x + offset, battle.ship.pos.y},
                                    0.0f, assets);
            }
        } else if (weapon.projectile_count <= 1) {
            spawn_player_bullet(battle, weapon, battle.ship.pos, 0.0f, assets);
        } else {
            const float start = -weapon.spread_degrees *
                                (static_cast<float>(weapon.projectile_count) - 1.0f) * 0.5f;
            for (int i = 0; i < weapon.projectile_count; ++i) {
                const float angle = start + weapon.spread_degrees * static_cast<float>(i);
                spawn_player_bullet(battle, weapon, battle.ship.pos, angle, assets);
            }
        }

        weapon.cooldown_timer = weapon.cooldown;
    }
}

void update_player_bullets(BattleState& battle, float dt) {
    for (Weapon& weapon : battle.weapons) {
        weapon.cooldown_timer = std::max(0.0f, weapon.cooldown_timer - dt);
    }

    for (PlayerBullet& bullet : battle.player_bullets) {
        bullet.age += dt;
        switch (bullet.type) {
        case WeaponType::Missile: {
            const Vec2 desired = nearest_enemy_dir(battle, bullet.pos, bullet.base_dir);
            bullet.base_dir +=
                (desired - bullet.base_dir) * std::min(1.0f, dt * bullet.homing_turn);
            const float len = std::max(vec2_length(bullet.base_dir), 0.001f);
            bullet.base_dir = {bullet.base_dir.x / len, bullet.base_dir.y / len};
            const float speed = vec2_length(bullet.vel);
            bullet.vel = bullet.base_dir * speed;
            bullet.pos += bullet.vel * dt;
            break;
        }
        case WeaponType::Arc: {
            const Vec2 perp = {-bullet.base_dir.y, bullet.base_dir.x};
            const float speed = vec2_length(bullet.vel);
            const Vec2 drift =
                perp * std::sin(bullet.age * bullet.wave_frequency) * bullet.wave_amplitude;
            bullet.pos += (bullet.base_dir * speed + drift) * dt;
            break;
        }
        case WeaponType::Mine:
            if (bullet.age >= bullet.stop_age) {
                bullet.vel = {0.0f, 0.0f};
            }
            bullet.pos += bullet.vel * dt;
            break;
        case WeaponType::Rail:
        case WeaponType::Basic:
            bullet.pos += bullet.vel * dt;
            break;
        }
        bullet.target_height = bullet.base_height + std::sin(bullet.age * 18.0f) * 0.2f;
        bullet.height += (bullet.target_height - bullet.height) * std::min(1.0f, dt * 10.0f);
    }

    battle.player_bullets.erase(
        std::remove_if(battle.player_bullets.begin(), battle.player_bullets.end(),
                       [](const PlayerBullet& bullet) {
                           return bullet.age > bullet.life || bullet.pos.y < -4.0f ||
                                  bullet.pos.x < -8.0f || bullet.pos.x > GAME_WIDTH + 8.0f;
                       }),
        battle.player_bullets.end());
}
