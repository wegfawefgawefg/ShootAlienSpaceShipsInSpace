#include "damage.hpp"

#include "assets.hpp"
#include "pickups.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace {

constexpr float PLAYER_INVULN_TIME = 1.8f;
constexpr float PLAYER_RESPAWN_TIME = 1.0f;
constexpr float PARTICLE_LIFE = 0.7f;

float random_unit() {
    return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
}

float random_range(float min_value, float max_value) {
    return min_value + (max_value - min_value) * random_unit();
}

int random_index(int count) {
    return std::rand() % count;
}

void add_hitstop(BattleState& battle, int frames) {
    battle.hitstop_frames += frames;
    battle.camera.shake = std::max(battle.camera.shake, (3.2f + static_cast<float>(frames) * 0.8f) *
                                                            battle.camera_shake_scale);
}

void spawn_explosion(BattleState& battle, Vec2 pos, int count) {
    for (int i = 0; i < count; ++i) {
        const float angle = random_range(0.0f, 6.2831853f);
        const float speed = random_range(20.0f, 90.0f);
        battle.particles.push_back({
            pos,
            {std::cos(angle) * speed, std::sin(angle) * speed},
            0.0f,
            PARTICLE_LIFE * random_range(0.8f, 1.2f),
            random_range(0.8f, 1.4f),
            random_range(0.8f, 1.4f),
            random_range(0.8f, 1.4f),
            24 + random_index(6),
        });
    }
}

void damage_player(BattleState& battle) {
    if (!battle.player_active || battle.invuln_timer > 0.0f) {
        return;
    }

    spawn_explosion(battle, battle.ship.pos, 18);
    battle.lives -= 1;
    battle.player_active = false;
    battle.respawn_timer = (battle.lives > 0) ? PLAYER_RESPAWN_TIME : 0.0f;
    battle.invuln_timer = PLAYER_INVULN_TIME;
    battle.ship.vel = {0.0f, 0.0f};
    battle.ship.shake = 10.0f;
    battle.can_shoot = false;
    battle.ship.target_height = battle.ship.base_height - 1.5f;
    add_hitstop(battle, 6);
}

} // namespace

void update_enemy_bullets(BattleState& battle, float dt) {
    for (EnemyBullet& bullet : battle.enemy_bullets) {
        bullet.age += dt;
        bullet.pos += bullet.vel * dt;
        bullet.target_height = bullet.base_height + std::sin(bullet.age * 16.0f) * 0.18f;
        bullet.height += (bullet.target_height - bullet.height) * std::min(1.0f, dt * 10.0f);
    }

    battle.enemy_bullets.erase(
        std::remove_if(battle.enemy_bullets.begin(), battle.enemy_bullets.end(),
                       [](const EnemyBullet& bullet) {
                           return bullet.age > bullet.life || bullet.pos.x < -8.0f ||
                                  bullet.pos.x > GAME_WIDTH + 8.0f || bullet.pos.y < -8.0f ||
                                  bullet.pos.y > GAME_HEIGHT + 8.0f;
                       }),
        battle.enemy_bullets.end());
}

void update_particles(BattleState& battle, float dt) {
    for (Particle& particle : battle.particles) {
        particle.age += dt;
        particle.pos += particle.vel * dt;
        particle.vel = particle.vel * 0.98f;
        particle.target_height =
            std::max(0.2f, particle.base_height * (1.0f - particle.age / particle.life));
        particle.height += (particle.target_height - particle.height) * std::min(1.0f, dt * 12.0f);
    }

    battle.particles.erase(
        std::remove_if(battle.particles.begin(), battle.particles.end(),
                       [](const Particle& particle) { return particle.age >= particle.life; }),
        battle.particles.end());
}

void update_player_respawn(BattleState& battle, float dt) {
    battle.ship.shake *= 0.5f;
    if (battle.invuln_timer > 0.0f) {
        battle.invuln_timer = std::max(0.0f, battle.invuln_timer - dt);
    }

    if (!battle.player_active && battle.lives > 0) {
        battle.respawn_timer = std::max(0.0f, battle.respawn_timer - dt);
        if (battle.respawn_timer <= 0.0f) {
            battle.player_active = true;
            battle.can_shoot = true;
            battle.ship.pos = {GAME_WIDTH * 0.5f, GAME_HEIGHT * 0.8f};
            battle.ship.vel = {0.0f, 0.0f};
        }
    }
}

void resolve_damage(BattleState& battle, Assets& assets) {
    std::vector<Enemy> surviving_enemies;
    surviving_enemies.reserve(battle.enemies.size());

    for (Enemy& enemy : battle.enemies) {
        bool dead = false;
        for (PlayerBullet& bullet : battle.player_bullets) {
            const float radius = bullet.radius + enemy.radius;
            if (vec2_length_sq(enemy.pos - bullet.pos) <= radius * radius) {
                bullet.age = 999.0f;
                enemy.hp -= bullet.damage;
                enemy.shake = enemy.is_boss ? 6.0f : 5.0f;
                enemy.target_height = enemy.base_height - 1.0f;
                add_hitstop(battle, enemy.is_boss ? 3 : 2);
                maybe_spawn_enemy_gold(battle, enemy, false);
                if (enemy.hp <= 0.0f) {
                    spawn_explosion(battle, enemy.pos, 10);
                    maybe_spawn_enemy_gold(battle, enemy, true);
                    maybe_spawn_enemy_drop(battle, enemy);
                    dead = true;
                }
                break;
            }
        }
        if (!dead) {
            surviving_enemies.push_back(enemy);
        }
    }

    battle.enemies = std::move(surviving_enemies);
    battle.player_bullets.erase(
        std::remove_if(battle.player_bullets.begin(), battle.player_bullets.end(),
                       [](const PlayerBullet& bullet) { return bullet.age > 900.0f; }),
        battle.player_bullets.end());

    if (battle.player_active) {
        for (EnemyBullet& bullet : battle.enemy_bullets) {
            const float radius = bullet.radius + battle.ship.radius;
            if (vec2_length_sq(battle.ship.pos - bullet.pos) <= radius * radius) {
                bullet.age = 999.0f;
                damage_player(battle);
                Mix_PlayChannel(
                    -1, assets.rock_hit_sounds[static_cast<std::size_t>(random_index(3))], 0);
                break;
            }
        }

        for (const Enemy& enemy : battle.enemies) {
            const float radius = enemy.radius + battle.ship.radius;
            if (vec2_length_sq(battle.ship.pos - enemy.pos) <= radius * radius) {
                damage_player(battle);
                Mix_PlayChannel(
                    -1, assets.rock_hit_sounds[static_cast<std::size_t>(random_index(3))], 0);
                break;
            }
        }
    }

    battle.enemy_bullets.erase(
        std::remove_if(battle.enemy_bullets.begin(), battle.enemy_bullets.end(),
                       [](const EnemyBullet& bullet) { return bullet.age > 900.0f; }),
        battle.enemy_bullets.end());
}
