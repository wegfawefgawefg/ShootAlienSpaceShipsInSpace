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
    if (weapon.type == WeaponType::Mine) {
        vel = dir * (weapon.projectile_speed * 0.28f * battle.projectile_speed_scale);
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
        weapon.homing_turn,
        weapon.wave_amplitude,
        weapon.wave_frequency,
        weapon.stop_age,
        weapon.pierce,
        weapon.ricochet,
        weapon.explosion_radius,
        weapon.projectile_tile,
        weapon.type,
    });
}

void spawn_player_beam(BattleState& battle, const Weapon& weapon, Vec2 spawn_pos,
                       float angle_offset_deg, Assets& assets) {
    Mix_PlayChannel(-1, assets.laser_sounds[static_cast<std::size_t>(random_index(3))], 0);
    const float angle = (-90.0f + angle_offset_deg) * 0.0174532925f;
    const Vec2 dir = {std::cos(angle), std::sin(angle)};
    battle.player_beams.push_back({
        spawn_pos,
        dir,
        0.0f,
        weapon.beam_duration,
        weapon.damage,
        weapon.beam_tick_interval,
        0.0f,
        weapon.beam_width,
        weapon.beam_length,
        4.0f,
        4.0f,
        4.0f,
    });
}

void spawn_player_orbitals(BattleState& battle, const Weapon& weapon, Assets& assets) {
    Mix_PlayChannel(-1, assets.laser_sounds[static_cast<std::size_t>(random_index(3))], 0);
    const int count = std::max(1, weapon.orbital_count);
    for (int i = 0; i < count; ++i) {
        const float angle = static_cast<float>(i) / static_cast<float>(count) * 6.2831853f;
        battle.player_orbitals.push_back({
            battle.ship.pos,
            angle,
            weapon.orbital_radius,
            weapon.orbital_speed,
            0.0f,
            weapon.orbital_duration,
            weapon.damage,
            0.0f,
            3.5f,
            3.5f,
            3.5f,
            weapon.projectile_tile,
        });
    }
}

void fire_weapon_once(BattleState& battle, Weapon& weapon, Assets& assets) {
    if (weapon.type == WeaponType::Beam) {
        spawn_player_beam(battle, weapon, battle.ship.pos, 0.0f, assets);
        return;
    }
    if (weapon.type == WeaponType::Orbital) {
        spawn_player_orbitals(battle, weapon, assets);
        return;
    }

    if (weapon.fixture == WeaponFixture::Center) {
        spawn_player_bullet(battle, weapon, battle.ship.pos, 0.0f, assets);
    } else if (weapon.fixture == WeaponFixture::EvenlySpread) {
        const float spacing = 6.0f;
        const float start = -spacing * (static_cast<float>(weapon.projectile_count) - 1.0f) * 0.5f;
        for (int i = 0; i < weapon.projectile_count; ++i) {
            const float offset = start + spacing * static_cast<float>(i);
            spawn_player_bullet(battle, weapon, {battle.ship.pos.x + offset, battle.ship.pos.y},
                                0.0f, assets);
        }
    } else if (weapon.projectile_count <= 1) {
        spawn_player_bullet(battle, weapon, battle.ship.pos, 0.0f, assets);
    } else {
        const float start =
            -weapon.spread_degrees * (static_cast<float>(weapon.projectile_count) - 1.0f) * 0.5f;
        for (int i = 0; i < weapon.projectile_count; ++i) {
            const float angle = start + weapon.spread_degrees * static_cast<float>(i);
            spawn_player_bullet(battle, weapon, battle.ship.pos, angle, assets);
        }
    }
}

} // namespace

void fire_player_weapons(BattleState& battle, Assets& assets, bool trigger_pressed,
                         bool trigger_held) {
    for (Weapon& weapon : battle.weapons) {
        if (weapon.cooldown_timer > 0.0f || weapon.burst_remaining > 0) {
            continue;
        }
        if ((weapon.automatic && !trigger_held) || (!weapon.automatic && !trigger_pressed)) {
            continue;
        }

        fire_weapon_once(battle, weapon, assets);
        if (weapon.type == WeaponType::Burst) {
            weapon.burst_remaining = std::max(0, weapon.burst_count - 1);
            weapon.burst_timer = weapon.burst_interval;
        }
        weapon.cooldown_timer = weapon.cooldown;
    }
}

void update_player_bullets(BattleState& battle, Assets& assets, float dt) {
    for (Weapon& weapon : battle.weapons) {
        weapon.cooldown_timer = std::max(0.0f, weapon.cooldown_timer - dt);
        if (weapon.burst_remaining > 0) {
            weapon.burst_timer -= dt;
            while (weapon.burst_remaining > 0 && weapon.burst_timer <= 0.0f) {
                fire_weapon_once(battle, weapon, assets);
                weapon.burst_remaining -= 1;
                weapon.burst_timer += weapon.burst_interval;
            }
        }
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
        case WeaponType::Orbital:
        case WeaponType::Burst:
        case WeaponType::Rail:
        case WeaponType::Basic:
        case WeaponType::Beam:
            bullet.pos += bullet.vel * dt;
            break;
        }
        if (bullet.ricochet > 0) {
            bool bounced = false;
            if (bullet.pos.x < 4.0f || bullet.pos.x > GAME_WIDTH - 4.0f) {
                bullet.vel.x = -bullet.vel.x;
                bullet.base_dir.x = -bullet.base_dir.x;
                bullet.pos.x = clampf(bullet.pos.x, 4.0f, GAME_WIDTH - 4.0f);
                bounced = true;
            }
            if (bullet.pos.y < 4.0f) {
                bullet.vel.y = -bullet.vel.y;
                bullet.base_dir.y = -bullet.base_dir.y;
                bullet.pos.y = 4.0f;
                bounced = true;
            }
            if (bounced) {
                bullet.ricochet -= 1;
            }
        }
        bullet.target_height = bullet.base_height + std::sin(bullet.age * 18.0f) * 0.2f;
        bullet.height += (bullet.target_height - bullet.height) * std::min(1.0f, dt * 10.0f);
    }

    for (PlayerBeam& beam : battle.player_beams) {
        beam.age += dt;
        beam.tick_timer -= dt;
        beam.target_height = beam.base_height + std::sin(beam.age * 22.0f) * 0.2f;
        beam.height += (beam.target_height - beam.height) * std::min(1.0f, dt * 12.0f);
    }

    for (PlayerOrbital& orbital : battle.player_orbitals) {
        orbital.age += dt;
        orbital.angle += orbital.speed * dt * 0.0174532925f;
        orbital.pos = battle.ship.pos +
                      Vec2{std::cos(orbital.angle), std::sin(orbital.angle)} * orbital.radius;
        orbital.hit_cooldown = std::max(0.0f, orbital.hit_cooldown - dt);
        orbital.target_height = orbital.base_height + std::sin(orbital.age * 18.0f) * 0.18f;
        orbital.height += (orbital.target_height - orbital.height) * std::min(1.0f, dt * 12.0f);
    }

    battle.player_bullets.erase(
        std::remove_if(battle.player_bullets.begin(), battle.player_bullets.end(),
                       [](const PlayerBullet& bullet) {
                           return bullet.age > bullet.life || bullet.pos.y < -4.0f ||
                                  bullet.pos.x < -8.0f || bullet.pos.x > GAME_WIDTH + 8.0f;
                       }),
        battle.player_bullets.end());
    battle.player_beams.erase(
        std::remove_if(battle.player_beams.begin(), battle.player_beams.end(),
                       [](const PlayerBeam& beam) { return beam.age >= beam.duration; }),
        battle.player_beams.end());
    battle.player_orbitals.erase(
        std::remove_if(battle.player_orbitals.begin(), battle.player_orbitals.end(),
                       [](const PlayerOrbital& orbital) { return orbital.age >= orbital.life; }),
        battle.player_orbitals.end());
}
