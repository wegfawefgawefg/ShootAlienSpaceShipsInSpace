#include "enemy_behavior.hpp"

#include "level_data.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace {

constexpr float LEVEL_INTRO_DURATION = 3.0f;

float random_unit() {
    return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
}

float random_range(float min_value, float max_value) {
    return min_value + (max_value - min_value) * random_unit();
}

float ease_out_cubic(float t) {
    const float u = 1.0f - t;
    return 1.0f - u * u * u;
}

float wrap_axis(float value, float min_value, float max_value) {
    const float span = max_value - min_value;
    while (value < min_value) {
        value += span;
    }
    while (value >= max_value) {
        value -= span;
    }
    return value;
}

float angle_from_vector(Vec2 dir) {
    if (vec2_length_sq(dir) < 0.0001f) {
        return 0.0f;
    }
    return std::atan2(dir.y, dir.x) * 57.2957795f + 90.0f;
}

void spawn_enemy_bullet(BattleState& battle, const Enemy& enemy) {
    Vec2 dir = battle.ship.pos - enemy.pos;
    const float len = std::max(vec2_length(dir), 0.001f);
    dir = {dir.x / len, dir.y / len};
    battle.enemy_bullets.push_back({enemy.pos, dir * 120.0f, 0.0f, 2.0f, 1.0f, 4.0f, 2});
}

void update_enemy_facing(Enemy& enemy, const Ship& ship) {
    switch (enemy.facing) {
    case EnemyFacing::FacePlayer:
        enemy.angle_deg = angle_from_vector(ship.pos - enemy.pos);
        break;
    case EnemyFacing::FaceMovement:
        enemy.angle_deg = angle_from_vector(enemy.vel);
        break;
    case EnemyFacing::FaceDown:
        enemy.angle_deg = 180.0f;
        break;
    }
}

void update_enemy_behavior(Enemy& enemy, BattleState& battle, float dt) {
    enemy.behavior_timer += dt;
    enemy.shoot_timer -= dt;
    enemy.shake *= 0.5f;

    switch (enemy.behavior) {
    case EnemyBehavior::Straight:
        if (vec2_length_sq(enemy.vel) < 0.01f) {
            enemy.vel = {0.0f, 32.0f};
        }
        enemy.pos += enemy.vel * dt;
        break;

    case EnemyBehavior::SlowChase: {
        Vec2 dir = battle.ship.pos - enemy.pos;
        const float len = std::max(vec2_length(dir), 0.001f);
        dir = {dir.x / len, dir.y / len};
        enemy.vel += (dir * 28.0f - enemy.vel) * std::min(1.0f, dt * 2.0f);
        enemy.pos += enemy.vel * dt;
        break;
    }

    case EnemyBehavior::TopShooter:
        enemy.pos.x = enemy.formation_pos.x + std::sin(enemy.behavior_timer * 1.8f) * 18.0f;
        enemy.pos.y = enemy.formation_pos.y + std::sin(enemy.behavior_timer * 0.7f) * 4.0f;
        enemy.vel = {std::cos(enemy.behavior_timer * 1.8f) * 32.0f, 0.0f};
        if (enemy.shoot_timer <= 0.0f) {
            spawn_enemy_bullet(battle, enemy);
            enemy.shoot_timer = 1.2f + random_range(0.0f, 0.8f);
        }
        break;

    case EnemyBehavior::Rammer: {
        enemy.ram_timer += dt;
        if (enemy.ram_timer < 1.5f) {
            const float dx = battle.ship.pos.x - enemy.pos.x;
            enemy.vel = {clampf(dx * 1.2f, -45.0f, 45.0f), -12.0f};
        } else if (enemy.ram_timer < 2.0f) {
            enemy.vel = {0.0f, -22.0f};
        } else {
            Vec2 dir = battle.ship.pos - enemy.pos;
            const float len = std::max(vec2_length(dir), 0.001f);
            dir = {dir.x / len, dir.y / len};
            enemy.vel = dir * 125.0f;
            if (enemy.ram_timer > 3.4f) {
                enemy.ram_timer = 0.0f;
            }
        }
        enemy.pos += enemy.vel * dt;
        break;
    }

    case EnemyBehavior::Circler:
        enemy.orbit_phase += dt * 1.6f;
        enemy.pos = {enemy.formation_pos.x + std::cos(enemy.orbit_phase) * 18.0f,
                     enemy.formation_pos.y + std::sin(enemy.orbit_phase) * 12.0f};
        enemy.vel = {-std::sin(enemy.orbit_phase) * 28.8f, std::cos(enemy.orbit_phase) * 19.2f};
        break;

    case EnemyBehavior::Wander: {
        Vec2 to_target = enemy.wander_target - enemy.pos;
        if (vec2_length_sq(to_target) < 16.0f) {
            enemy.wander_target = {
                random_range(16.0f, GAME_WIDTH - 16.0f),
                random_range(16.0f, GAME_HEIGHT * 0.6f),
            };
            to_target = enemy.wander_target - enemy.pos;
        }
        const float len = std::max(vec2_length(to_target), 0.001f);
        const Vec2 dir = {to_target.x / len, to_target.y / len};
        enemy.vel += (dir * 44.0f - enemy.vel) * std::min(1.0f, dt * 2.4f);
        enemy.pos += enemy.vel * dt;
        break;
    }
    }

    enemy.pos.x = wrap_axis(enemy.pos.x, -12.0f, GAME_WIDTH + 12.0f);
    enemy.pos.y = wrap_axis(enemy.pos.y, -18.0f, GAME_HEIGHT + 18.0f);
    update_enemy_facing(enemy, battle.ship);
}

} // namespace

void start_level(BattleState& battle, int level_index) {
    const auto& level =
        levels()[static_cast<std::size_t>(level_index % static_cast<int>(levels().size()))];

    battle.current_level_index = level_index % static_cast<int>(levels().size());
    battle.level_timer = 0.0f;
    battle.level_text_timer = 0.0f;
    battle.phase = BattlePhase::LevelIntro;
    battle.can_shoot = false;
    battle.player_bullets.clear();
    battle.enemy_bullets.clear();
    battle.enemies.clear();
    battle.enemies.reserve(level.spawns.size());

    for (const LevelSpawnDef& spawn : level.spawns) {
        Enemy enemy{};
        enemy.pos = spawn.intro_start;
        enemy.intro_start = spawn.intro_start;
        enemy.formation_pos = spawn.formation_pos;
        enemy.wander_target = spawn.formation_pos;
        enemy.type_id = spawn.type_id;
        enemy.behavior = spawn.behavior;
        enemy.facing = spawn.facing;
        enemy.radius = 6.0f + static_cast<float>(spawn.type_id % 3);
        enemy.hp = 1.0f + static_cast<float>(spawn.type_id % 3);
        enemy.max_hp = enemy.hp;
        enemy.orbit_phase = random_range(0.0f, 6.2831853f);
        enemy.shoot_timer = random_range(0.2f, 1.0f);
        battle.enemies.push_back(enemy);
    }
}

void update_enemy_intro(BattleState& battle, float dt) {
    battle.level_timer += dt;
    battle.level_text_timer += dt;
    const float progress = clampf(battle.level_timer / LEVEL_INTRO_DURATION, 0.0f, 1.0f);
    const float eased = ease_out_cubic(progress);

    for (Enemy& enemy : battle.enemies) {
        enemy.pos = lerp(enemy.intro_start, enemy.formation_pos, eased);
        enemy.vel = enemy.formation_pos - enemy.pos;
        enemy.angle_deg = angle_from_vector(enemy.vel);
    }

    if (battle.level_timer >= LEVEL_INTRO_DURATION) {
        battle.level_timer = 0.0f;
        battle.phase = BattlePhase::Active;
        battle.can_shoot = true;
    }
}

void update_enemy_wave(BattleState& battle, float dt) {
    battle.level_timer += dt;
    for (Enemy& enemy : battle.enemies) {
        update_enemy_behavior(enemy, battle, dt);
    }
}
