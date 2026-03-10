#include "enemy_behavior.hpp"

#include "level_data.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace {

constexpr float LEVEL_INTRO_DURATION = 2.5f;
constexpr float ENTRY_DURATION = 1.2f;

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

void spawn_enemy_bullet_dir(BattleState& battle, Vec2 pos, Vec2 dir, float speed, int tile = 2) {
    const float len = std::max(vec2_length(dir), 0.001f);
    const Vec2 unit = {dir.x / len, dir.y / len};
    battle.enemy_bullets.push_back(
        {pos, unit * speed, 0.0f, 1.25f, 1.0f, 4.0f, 3.0f, 3.0f, 3.0f, tile});
}

void spawn_enemy_bullet_at_player(BattleState& battle, const Enemy& enemy, float speed) {
    spawn_enemy_bullet_dir(battle, enemy.pos, battle.ship.pos - enemy.pos, speed);
}

void move_enemy_toward(Enemy& enemy, Vec2 target, float response, float dt) {
    const Vec2 delta = target - enemy.pos;
    enemy.vel = delta * response;
    enemy.pos += delta * std::min(1.0f, dt * response);
}

void spawn_spoke_burst(BattleState& battle, const Enemy& enemy, int spokes, float rotation_deg,
                       float speed) {
    const float step = 360.0f / static_cast<float>(spokes);
    for (int i = 0; i < spokes; ++i) {
        const float angle = (rotation_deg + step * static_cast<float>(i) - 90.0f) * 0.0174532925f;
        spawn_enemy_bullet_dir(battle, enemy.pos, {std::cos(angle), std::sin(angle)}, speed, 3);
    }
}

int boss_group_from_hp(const Enemy& enemy) {
    if (enemy.max_hp <= 0.0f) {
        return 0;
    }
    const float ratio = enemy.hp / enemy.max_hp;
    if (ratio > 0.5f) {
        return 0;
    }
    if (ratio > 0.25f) {
        return 1;
    }
    return 2;
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

bool update_enemy_entry(Enemy& enemy, float dt) {
    enemy.entry_timer += dt;
    const float progress = clampf(enemy.entry_timer / ENTRY_DURATION, 0.0f, 1.0f);
    const float eased = ease_out_cubic(progress);
    enemy.pos = lerp(enemy.intro_start, enemy.formation_pos, eased);
    enemy.vel = enemy.formation_pos - enemy.pos;
    enemy.target_height = enemy.base_height + (1.0f - eased) * 3.0f;
    enemy.height += (enemy.target_height - enemy.height) * std::min(1.0f, dt * 9.0f);
    enemy.angle_deg = angle_from_vector(enemy.vel);
    if (progress >= 1.0f) {
        enemy.entering = false;
        enemy.pos = enemy.formation_pos;
        enemy.entry_timer = 0.0f;
        enemy.vel = {0.0f, 0.0f};
        return true;
    }
    return false;
}

void update_boss_behavior_selection(Enemy& enemy, float dt) {
    if (!enemy.is_boss) {
        return;
    }

    const int desired_group = boss_group_from_hp(enemy);
    if (desired_group != enemy.boss_group_index) {
        enemy.boss_group_index = desired_group;
        enemy.boss_behavior_index = 0;
        enemy.boss_behavior_timer = 0.0f;
    }

    enemy.boss_behavior_timer -= dt;
    if (enemy.boss_behavior_timer > 0.0f) {
        return;
    }

    const int count = enemy.boss_group_sizes[static_cast<std::size_t>(enemy.boss_group_index)];
    if (count <= 0) {
        return;
    }

    const int slot = enemy.boss_behavior_index % count;
    enemy.behavior = enemy.boss_behavior_groups[static_cast<std::size_t>(enemy.boss_group_index)]
                                               [static_cast<std::size_t>(slot)];
    enemy.boss_behavior_index = (enemy.boss_behavior_index + 1) % count;
    enemy.boss_behavior_timer =
        std::max(1.4f, 3.0f - static_cast<float>(enemy.boss_group_index) * 0.5f);
}

void update_enemy_behavior(Enemy& enemy, BattleState& battle, float dt) {
    enemy.behavior_timer += dt;
    enemy.shoot_timer -= dt;
    enemy.shake *= 0.5f;
    enemy.hover_phase += dt * (2.5f + static_cast<float>(enemy.type_id % 3));
    update_boss_behavior_selection(enemy, dt);

    switch (enemy.behavior) {
    case EnemyBehavior::Straight:
        if (vec2_length_sq(enemy.vel) < 0.01f) {
            enemy.vel = {0.0f, 32.0f * enemy.speed_scale};
        }
        enemy.pos += enemy.vel * dt;
        break;

    case EnemyBehavior::SlowChase: {
        Vec2 dir = battle.ship.pos - enemy.pos;
        const float len = std::max(vec2_length(dir), 0.001f);
        dir = {dir.x / len, dir.y / len};
        enemy.vel += (dir * (38.0f * enemy.speed_scale) - enemy.vel) * std::min(1.0f, dt * 2.0f);
        enemy.pos += enemy.vel * dt;
        break;
    }

    case EnemyBehavior::TopShooter:
        move_enemy_toward(enemy,
                          {enemy.formation_pos.x + std::sin(enemy.behavior_timer * 1.8f) * 36.0f,
                           enemy.formation_pos.y + std::sin(enemy.behavior_timer * 0.7f) * 10.0f},
                          4.5f, dt);
        if (enemy.shoot_timer <= 0.0f) {
            spawn_enemy_bullet_at_player(battle, enemy, 170.0f * enemy.speed_scale);
            enemy.shoot_timer = 1.0f + random_range(0.0f, 0.6f);
        }
        break;

    case EnemyBehavior::Rammer: {
        enemy.ram_timer += dt;
        if (enemy.ram_timer < 1.2f) {
            const float dx = battle.ship.pos.x - enemy.pos.x;
            enemy.vel = {clampf(dx * 1.0f, -70.0f, 70.0f), -18.0f};
        } else if (enemy.ram_timer < 1.7f) {
            enemy.vel = {0.0f, -28.0f};
        } else {
            Vec2 dir = battle.ship.pos - enemy.pos;
            const float len = std::max(vec2_length(dir), 0.001f);
            dir = {dir.x / len, dir.y / len};
            enemy.vel = dir * (170.0f * enemy.speed_scale);
            if (enemy.ram_timer > 3.2f) {
                enemy.ram_timer = 0.0f;
            }
        }
        enemy.pos += enemy.vel * dt;
        break;
    }

    case EnemyBehavior::Circler:
        enemy.orbit_phase += dt * (1.2f + enemy.speed_scale * 0.5f);
        move_enemy_toward(enemy,
                          {enemy.formation_pos.x + std::cos(enemy.orbit_phase) * 34.0f,
                           enemy.formation_pos.y + std::sin(enemy.orbit_phase) * 20.0f},
                          5.0f, dt);
        break;

    case EnemyBehavior::Wander: {
        Vec2 to_target = enemy.wander_target - enemy.pos;
        if (vec2_length_sq(to_target) < 64.0f) {
            enemy.wander_target = {
                random_range(32.0f, GAME_WIDTH - 32.0f),
                random_range(24.0f, GAME_HEIGHT * 0.52f),
            };
            to_target = enemy.wander_target - enemy.pos;
        }
        const float len = std::max(vec2_length(to_target), 0.001f);
        const Vec2 dir = {to_target.x / len, to_target.y / len};
        enemy.vel += (dir * (70.0f * enemy.speed_scale) - enemy.vel) * std::min(1.0f, dt * 2.1f);
        enemy.pos += enemy.vel * dt;
        break;
    }

    case EnemyBehavior::BossSpray:
        move_enemy_toward(enemy,
                          {enemy.formation_pos.x + std::sin(enemy.behavior_timer * 1.1f) * 90.0f,
                           enemy.formation_pos.y + std::sin(enemy.behavior_timer * 0.55f) * 12.0f},
                          3.8f, dt);
        if (enemy.shoot_timer <= 0.0f) {
            for (float spread : {-18.0f, -9.0f, 0.0f, 9.0f, 18.0f}) {
                const float angle = (90.0f + spread) * 0.0174532925f;
                spawn_enemy_bullet_dir(battle, enemy.pos, {std::cos(angle), std::sin(angle)},
                                       190.0f);
            }
            enemy.shoot_timer = 0.45f;
        }
        break;

    case EnemyBehavior::BossSpokes:
        move_enemy_toward(enemy,
                          {enemy.formation_pos.x + std::sin(enemy.behavior_timer * 0.8f) * 56.0f,
                           enemy.formation_pos.y + std::cos(enemy.behavior_timer * 0.45f) * 10.0f},
                          3.8f, dt);
        if (enemy.shoot_timer <= 0.0f) {
            spawn_spoke_burst(battle, enemy, 10, enemy.behavior_timer * 80.0f, 150.0f);
            enemy.shoot_timer = 1.0f;
        }
        break;

    case EnemyBehavior::BossCharge: {
        enemy.ram_timer += dt;
        if (enemy.ram_timer < 1.0f) {
            const float dx = battle.ship.pos.x - enemy.pos.x;
            enemy.vel = {clampf(dx * 1.2f, -90.0f, 90.0f), -18.0f};
        } else if (enemy.ram_timer < 1.5f) {
            enemy.vel = {0.0f, -36.0f};
            if (enemy.shoot_timer <= 0.0f) {
                spawn_spoke_burst(battle, enemy, 6, 0.0f, 120.0f);
                enemy.shoot_timer = 0.6f;
            }
        } else {
            Vec2 dir = battle.ship.pos - enemy.pos;
            const float len = std::max(vec2_length(dir), 0.001f);
            dir = {dir.x / len, dir.y / len};
            enemy.vel = dir * 230.0f;
            if (enemy.ram_timer > 2.8f) {
                enemy.ram_timer = 0.0f;
            }
        }
        enemy.pos += enemy.vel * dt;
        break;
    }
    }

    enemy.pos.x = wrap_axis(enemy.pos.x, -24.0f, GAME_WIDTH + 24.0f);
    enemy.pos.y = wrap_axis(enemy.pos.y, -28.0f, GAME_HEIGHT + 28.0f);
    enemy.target_height =
        enemy.base_height + std::sin(enemy.hover_phase) * 0.6f - enemy.shake * 0.1f;
    enemy.height += (enemy.target_height - enemy.height) * std::min(1.0f, dt * 10.0f);
    update_enemy_facing(enemy, battle.ship);
}

} // namespace

bool spawn_next_wave(BattleState& battle) {
    const LevelDef& level = levels()[static_cast<std::size_t>(battle.current_level_index)];
    const int next_wave_index = battle.current_wave_index + 1;
    if (next_wave_index >= static_cast<int>(level.waves.size())) {
        return false;
    }

    const WaveDef& wave = level.waves[static_cast<std::size_t>(next_wave_index)];
    battle.current_wave_index = next_wave_index;
    battle.active_wave_tag += 1;
    battle.wave_timer = wave.duration;
    battle.wave_has_timer = wave.timed;
    battle.level_text_timer = 0.0f;

    for (const LevelSpawnDef& spawn : wave.spawns) {
        Enemy enemy{};
        enemy.pos = spawn.intro_start;
        enemy.intro_start = spawn.intro_start;
        enemy.formation_pos = spawn.formation_pos;
        enemy.wander_target = spawn.formation_pos;
        enemy.type_id = spawn.type_id;
        enemy.behavior = spawn.behavior;
        enemy.facing = spawn.facing;
        enemy.is_boss = spawn.is_boss;
        enemy.radius = enemy.is_boss ? spawn.radius : std::max(4.25f, spawn.radius * 1.55f);
        enemy.hp = spawn.hp;
        enemy.max_hp = spawn.hp;
        enemy.speed_scale = spawn.speed_scale;
        enemy.boss_behavior_groups = spawn.boss_behavior_groups;
        enemy.boss_group_sizes = spawn.boss_group_sizes;
        enemy.wave_tag = battle.active_wave_tag;
        enemy.base_height =
            spawn.is_boss ? 7.0f : 4.0f + static_cast<float>(spawn.type_id % 3) * 0.35f;
        enemy.height = enemy.base_height;
        enemy.target_height = enemy.base_height;
        enemy.orbit_phase = random_range(0.0f, 6.2831853f);
        enemy.shoot_timer = random_range(0.2f, 1.0f);
        enemy.entering = true;
        enemy.entry_timer = 0.0f;
        battle.enemies.push_back(enemy);
    }
    return true;
}

void start_level(BattleState& battle, int level_index) {
    battle.current_level_index = level_index % static_cast<int>(levels().size());
    battle.current_wave_index = -1;
    battle.level_timer = 0.0f;
    battle.level_text_timer = 0.0f;
    battle.wave_timer = 0.0f;
    battle.phase = BattlePhase::LevelIntro;
    battle.can_shoot = false;
    battle.player_bullets.clear();
    battle.enemy_bullets.clear();
    battle.enemies.clear();
    spawn_next_wave(battle);
}

bool update_enemy_intro(BattleState& battle, float dt) {
    battle.level_timer += dt;
    battle.level_text_timer += dt;
    const float progress = clampf(battle.level_timer / LEVEL_INTRO_DURATION, 0.0f, 1.0f);
    const float eased = ease_out_cubic(progress);

    for (Enemy& enemy : battle.enemies) {
        enemy.pos = lerp(enemy.intro_start, enemy.formation_pos, eased);
        enemy.vel = enemy.formation_pos - enemy.pos;
        enemy.target_height = enemy.base_height + (1.0f - eased) * 3.0f;
        enemy.height += (enemy.target_height - enemy.height) * std::min(1.0f, dt * 9.0f);
        enemy.angle_deg = angle_from_vector(enemy.vel);
    }

    if (battle.level_timer >= LEVEL_INTRO_DURATION) {
        for (Enemy& enemy : battle.enemies) {
            enemy.pos = enemy.formation_pos;
            enemy.vel = {0.0f, 0.0f};
            enemy.entering = false;
            enemy.entry_timer = 0.0f;
            enemy.height = enemy.base_height;
            enemy.target_height = enemy.base_height;
        }
        battle.level_timer = 0.0f;
        battle.phase = BattlePhase::Active;
        battle.can_shoot = true;
        return true;
    }
    return false;
}

void update_enemy_wave(BattleState& battle, float dt) {
    battle.level_timer += dt;
    battle.level_text_timer += dt;
    for (Enemy& enemy : battle.enemies) {
        if (enemy.entering) {
            update_enemy_entry(enemy, dt);
        } else {
            update_enemy_behavior(enemy, battle, dt);
        }
    }
}
