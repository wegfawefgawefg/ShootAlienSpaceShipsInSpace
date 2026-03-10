#include "player_weapons.hpp"

#include <SDL2/SDL_mixer.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace {

int random_index(int count) {
    return std::rand() % count;
}

void spawn_player_bullet(BattleState& battle, const Weapon& weapon, Vec2 spawn_pos,
                         float angle_offset_deg, Assets& assets) {
    Mix_PlayChannel(-1, assets.laser_sounds[static_cast<std::size_t>(random_index(3))], 0);
    const float angle = (-90.0f + angle_offset_deg) * 0.0174532925f;
    const Vec2 dir = {std::cos(angle), std::sin(angle)};
    battle.player_bullets.push_back({
        spawn_pos,
        dir * (weapon.projectile_speed * battle.projectile_speed_scale),
        0.0f,
        weapon.projectile_radius,
        weapon.damage,
        weapon.projectile_life,
        3.0f,
        3.0f,
        3.0f,
        weapon.projectile_tile,
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
        bullet.pos += bullet.vel * dt;
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
