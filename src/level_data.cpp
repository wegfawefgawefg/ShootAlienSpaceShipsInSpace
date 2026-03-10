#include "level_data.hpp"

#include <array>
#include <cmath>

namespace {

constexpr int SMALL_ENEMY_GRID_START_X = 4;
constexpr int SMALL_ENEMY_GRID_START_Y = 0;
constexpr int SMALL_ENEMY_GRID_COLS = 6;
constexpr int SMALL_ENEMY_GRID_ROWS = 6;

using BossGroups = std::array<std::array<EnemyBehavior, 3>, 3>;
using BossGroupSizes = std::array<int, 3>;

LevelSpawnDef make_enemy(int type_id, EnemyBehavior behavior, EnemyFacing facing, Vec2 intro_start,
                         Vec2 formation_pos, float hp, float radius, float speed_scale) {
    LevelSpawnDef spawn{};
    spawn.type_id = type_id;
    spawn.behavior = behavior;
    spawn.facing = facing;
    spawn.intro_start = intro_start;
    spawn.formation_pos = formation_pos;
    spawn.hp = hp;
    spawn.radius = radius;
    spawn.speed_scale = speed_scale;
    return spawn;
}

LevelSpawnDef make_boss(int type_id, EnemyFacing facing, Vec2 intro_start, Vec2 formation_pos,
                        float hp, float radius, float speed_scale, const BossGroups& groups,
                        const BossGroupSizes& group_sizes) {
    LevelSpawnDef spawn = make_enemy(type_id, EnemyBehavior::BossSpray, facing, intro_start,
                                     formation_pos, hp, radius, speed_scale);
    spawn.is_boss = true;
    spawn.boss_behavior_groups = groups;
    spawn.boss_group_sizes = group_sizes;
    return spawn;
}

WaveDef make_timed_wave(int number, float duration, std::vector<LevelSpawnDef> spawns) {
    WaveDef wave{};
    wave.number = number;
    wave.timed = true;
    wave.duration = duration;
    wave.is_boss = false;
    wave.spawns = std::move(spawns);
    return wave;
}

WaveDef make_boss_wave(int number, std::vector<LevelSpawnDef> spawns) {
    WaveDef wave{};
    wave.number = number;
    wave.timed = false;
    wave.duration = 0.0f;
    wave.is_boss = true;
    wave.spawns = std::move(spawns);
    return wave;
}

std::vector<LevelSpawnDef> line_wave(int count, float y, EnemyBehavior behavior, EnemyFacing facing,
                                     int visual_start, float hp, float radius, float speed_scale) {
    std::vector<LevelSpawnDef> spawns;
    spawns.reserve(static_cast<std::size_t>(count));
    const float spacing = 44.0f;
    const float start_x = GAME_WIDTH * 0.5f - spacing * (static_cast<float>(count) - 1.0f) * 0.5f;
    for (int i = 0; i < count; ++i) {
        const float x = start_x + spacing * static_cast<float>(i);
        spawns.push_back(make_enemy(visual_start + i, behavior, facing,
                                    {x, -48.0f - 8.0f * static_cast<float>(i)}, {x, y}, hp, radius,
                                    speed_scale));
    }
    return spawns;
}

std::vector<LevelSpawnDef> staggered_wave(int rows, int cols, Vec2 origin, Vec2 spacing,
                                          EnemyBehavior behavior, EnemyFacing facing,
                                          int visual_start, float hp, float radius,
                                          float speed_scale) {
    std::vector<LevelSpawnDef> spawns;
    spawns.reserve(static_cast<std::size_t>(rows * cols));
    int index = 0;
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            const float x = origin.x + spacing.x * static_cast<float>(col) +
                            ((row % 2 == 0) ? 0.0f : spacing.x * 0.5f);
            const float y = origin.y + spacing.y * static_cast<float>(row);
            spawns.push_back(make_enemy(visual_start + index, behavior, facing,
                                        {x, -60.0f - 10.0f * static_cast<float>(index)}, {x, y}, hp,
                                        radius, speed_scale));
            ++index;
        }
    }
    return spawns;
}

BossGroups boss_groups(EnemyBehavior high_a, EnemyBehavior high_b, EnemyBehavior high_c,
                       EnemyBehavior mid_a, EnemyBehavior mid_b, EnemyBehavior mid_c,
                       EnemyBehavior low_a, EnemyBehavior low_b, EnemyBehavior low_c) {
    return {{{{high_a, high_b, high_c}}, {{mid_a, mid_b, mid_c}}, {{low_a, low_b, low_c}}}};
}

BossGroupSizes default_boss_group_sizes() {
    return {{3, 3, 3}};
}

std::vector<LevelDef> build_levels() {
    std::vector<LevelDef> result;
    result.reserve(4);

    {
        LevelDef level{1, {}};
        level.waves.push_back(
            make_timed_wave(1, 8.0f,
                            line_wave(8, 56.0f, EnemyBehavior::Straight, EnemyFacing::FaceDown, 0,
                                      1.0f, 3.0f, 1.0f)));
        level.waves.push_back(make_timed_wave(
            2, 9.0f,
            staggered_wave(2, 5, {140.0f, 56.0f}, {58.0f, 36.0f}, EnemyBehavior::SlowChase,
                           EnemyFacing::FacePlayer, 8, 1.2f, 3.0f, 1.0f)));
        level.waves.push_back(make_timed_wave(
            3, 10.0f,
            staggered_wave(2, 6, {112.0f, 44.0f}, {48.0f, 34.0f}, EnemyBehavior::TopShooter,
                           EnemyFacing::FaceDown, 18, 1.3f, 3.1f, 1.0f)));
        level.waves.push_back(make_boss_wave(
            4, {make_boss(2, EnemyFacing::FaceMovement, {GAME_WIDTH * 0.5f, -120.0f},
                          {GAME_WIDTH * 0.5f, 84.0f}, 28.0f, 8.5f, 0.85f,
                          boss_groups(EnemyBehavior::BossSpray, EnemyBehavior::BossSpokes,
                                      EnemyBehavior::BossSpray, EnemyBehavior::BossSpokes,
                                      EnemyBehavior::BossCharge, EnemyBehavior::BossSpray,
                                      EnemyBehavior::BossCharge, EnemyBehavior::BossSpokes,
                                      EnemyBehavior::BossCharge),
                          default_boss_group_sizes())}));
        result.push_back(level);
    }

    {
        LevelDef level{2, {}};
        level.waves.push_back(make_timed_wave(
            1, 8.0f,
            staggered_wave(2, 7, {100.0f, 44.0f}, {52.0f, 34.0f}, EnemyBehavior::Straight,
                           EnemyFacing::FaceMovement, 4, 1.4f, 3.1f, 1.1f)));
        level.waves.push_back(make_timed_wave(
            2, 9.0f,
            staggered_wave(3, 5, {140.0f, 48.0f}, {58.0f, 30.0f}, EnemyBehavior::Rammer,
                           EnemyFacing::FaceMovement, 14, 1.6f, 3.25f, 1.05f)));
        level.waves.push_back(make_timed_wave(
            3, 10.0f,
            staggered_wave(2, 8, {84.0f, 42.0f}, {46.0f, 36.0f}, EnemyBehavior::Wander,
                           EnemyFacing::FaceMovement, 24, 1.7f, 3.2f, 1.15f)));
        level.waves.push_back(make_timed_wave(
            4, 11.0f,
            staggered_wave(3, 6, {112.0f, 38.0f}, {50.0f, 28.0f}, EnemyBehavior::TopShooter,
                           EnemyFacing::FaceDown, 2, 1.8f, 3.3f, 1.1f)));
        level.waves.push_back(make_boss_wave(
            5, {make_boss(6, EnemyFacing::FacePlayer, {GAME_WIDTH * 0.5f, -140.0f},
                          {GAME_WIDTH * 0.5f, 88.0f}, 36.0f, 10.0f, 0.9f,
                          boss_groups(EnemyBehavior::BossSpray, EnemyBehavior::BossSpokes,
                                      EnemyBehavior::BossCharge, EnemyBehavior::BossSpokes,
                                      EnemyBehavior::BossCharge, EnemyBehavior::BossSpray,
                                      EnemyBehavior::BossCharge, EnemyBehavior::BossSpray,
                                      EnemyBehavior::BossCharge),
                          default_boss_group_sizes())}));
        result.push_back(level);
    }

    {
        LevelDef level{3, {}};
        level.waves.push_back(make_timed_wave(
            1, 8.0f,
            staggered_wave(2, 9, {72.0f, 40.0f}, {42.0f, 30.0f}, EnemyBehavior::SlowChase,
                           EnemyFacing::FacePlayer, 10, 1.9f, 3.25f, 1.15f)));
        level.waves.push_back(make_timed_wave(
            2, 9.0f,
            staggered_wave(3, 7, {90.0f, 32.0f}, {44.0f, 28.0f}, EnemyBehavior::Circler,
                           EnemyFacing::FaceMovement, 22, 2.0f, 3.4f, 1.1f)));
        level.waves.push_back(make_timed_wave(
            3, 9.0f,
            staggered_wave(2, 10, {56.0f, 38.0f}, {40.0f, 34.0f}, EnemyBehavior::TopShooter,
                           EnemyFacing::FaceDown, 6, 2.0f, 3.4f, 1.2f)));
        level.waves.push_back(make_timed_wave(
            4, 11.0f,
            staggered_wave(3, 8, {76.0f, 34.0f}, {44.0f, 26.0f}, EnemyBehavior::Rammer,
                           EnemyFacing::FaceMovement, 18, 2.2f, 3.5f, 1.2f)));
        level.waves.push_back(make_boss_wave(
            5, {make_boss(12, EnemyFacing::FaceMovement, {GAME_WIDTH * 0.5f, -150.0f},
                          {GAME_WIDTH * 0.5f, 92.0f}, 44.0f, 11.0f, 1.0f,
                          boss_groups(EnemyBehavior::BossSpokes, EnemyBehavior::BossSpray,
                                      EnemyBehavior::BossCharge, EnemyBehavior::BossCharge,
                                      EnemyBehavior::BossSpokes, EnemyBehavior::BossSpray,
                                      EnemyBehavior::BossCharge, EnemyBehavior::BossSpokes,
                                      EnemyBehavior::BossCharge),
                          default_boss_group_sizes())}));
        result.push_back(level);
    }

    {
        LevelDef level{4, {}};
        level.waves.push_back(make_timed_wave(
            1, 8.0f,
            staggered_wave(3, 8, {70.0f, 36.0f}, {46.0f, 24.0f}, EnemyBehavior::Straight,
                           EnemyFacing::FaceMovement, 12, 2.2f, 3.5f, 1.25f)));
        level.waves.push_back(make_timed_wave(
            2, 9.0f,
            staggered_wave(3, 9, {62.0f, 30.0f}, {44.0f, 24.0f}, EnemyBehavior::SlowChase,
                           EnemyFacing::FacePlayer, 2, 2.4f, 3.6f, 1.25f)));
        level.waves.push_back(make_timed_wave(
            3, 9.0f,
            staggered_wave(3, 9, {66.0f, 34.0f}, {44.0f, 26.0f}, EnemyBehavior::Wander,
                           EnemyFacing::FaceMovement, 20, 2.5f, 3.6f, 1.25f)));
        level.waves.push_back(make_timed_wave(
            4, 10.0f,
            staggered_wave(4, 8, {92.0f, 26.0f}, {44.0f, 24.0f}, EnemyBehavior::TopShooter,
                           EnemyFacing::FaceDown, 4, 2.6f, 3.7f, 1.3f)));
        level.waves.push_back(make_timed_wave(
            5, 10.0f,
            staggered_wave(4, 7, {118.0f, 28.0f}, {50.0f, 22.0f}, EnemyBehavior::Rammer,
                           EnemyFacing::FaceMovement, 16, 2.8f, 3.8f, 1.3f)));
        level.waves.push_back(make_boss_wave(
            6, {make_boss(15, EnemyFacing::FacePlayer, {GAME_WIDTH * 0.5f, -170.0f},
                          {GAME_WIDTH * 0.5f, 96.0f}, 56.0f, 12.5f, 1.05f,
                          boss_groups(EnemyBehavior::BossSpray, EnemyBehavior::BossSpokes,
                                      EnemyBehavior::BossCharge, EnemyBehavior::BossSpokes,
                                      EnemyBehavior::BossCharge, EnemyBehavior::BossSpray,
                                      EnemyBehavior::BossCharge, EnemyBehavior::BossSpokes,
                                      EnemyBehavior::BossCharge),
                          default_boss_group_sizes())}));
        result.push_back(level);
    }

    return result;
}

} // namespace

int enemy_small_ship_tile(int visual_index) {
    const int wrapped = visual_index % (SMALL_ENEMY_GRID_COLS * SMALL_ENEMY_GRID_ROWS);
    const int col = wrapped % SMALL_ENEMY_GRID_COLS;
    const int row = wrapped / SMALL_ENEMY_GRID_COLS;
    return (SMALL_ENEMY_GRID_START_Y + row) * 10 + SMALL_ENEMY_GRID_START_X + col;
}

int player_ship_tile_center(int ship_index) {
    return (ship_index % 5) * 10 + 1;
}

const std::vector<LevelDef>& levels() {
    static const std::vector<LevelDef> kLevels = build_levels();
    return kLevels;
}

void seed_default_weapons(BattleState& battle) {
    battle.weapons.clear();
    battle.weapons.push_back({
        "Starter Blaster",
        WeaponType::Basic,
        WeaponFixture::Center,
        false,
        1,
        1,
        0.12f,
        0.0f,
        1600.0f,
        0.5f,
        1.25f,
        1.0f,
        0.0f,
        {},
    });
    battle.weapons.push_back({
        "Starter Fan",
        WeaponType::Basic,
        WeaponFixture::Splayed,
        false,
        0,
        2,
        0.3f,
        0.0f,
        1500.0f,
        0.5f,
        1.25f,
        1.0f,
        12.0f,
        {},
    });
}
