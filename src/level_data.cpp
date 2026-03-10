#include "level_data.hpp"

#include <array>
#include <cmath>

namespace {

constexpr int SMALL_ENEMY_GRID_START_X = 4;
constexpr int SMALL_ENEMY_GRID_START_Y = 0;
constexpr int SMALL_ENEMY_GRID_COLS = 6;
constexpr int SMALL_ENEMY_GRID_ROWS = 6;

std::vector<LevelSpawnDef> shape_line(int count, float y, EnemyBehavior behavior,
                                      EnemyFacing facing, int visual_start) {
    std::vector<LevelSpawnDef> spawns;
    spawns.reserve(static_cast<std::size_t>(count));
    const float spacing = 18.0f;
    const float start_x = GAME_WIDTH * 0.5f - spacing * (static_cast<float>(count) - 1.0f) * 0.5f;
    for (int i = 0; i < count; ++i) {
        const float x = start_x + spacing * static_cast<float>(i);
        spawns.push_back({
            visual_start + i,
            behavior,
            facing,
            {x, -20.0f - 6.0f * static_cast<float>(i)},
            {x, y},
        });
    }
    return spawns;
}

std::vector<LevelDef> build_levels() {
    std::vector<LevelDef> levels;
    levels.reserve(10);

    levels.push_back({1, shape_line(6, 34.0f, EnemyBehavior::Straight, EnemyFacing::FaceDown, 0)});

    {
        LevelDef level{2, {}};
        const std::array<Vec2, 7> positions = {
            {{120, 26}, {102, 40}, {138, 40}, {84, 54}, {156, 54}, {66, 68}, {174, 68}}};
        for (std::size_t i = 0; i < positions.size(); ++i) {
            level.spawns.push_back({static_cast<int>(i),
                                    EnemyBehavior::SlowChase,
                                    EnemyFacing::FacePlayer,
                                    {positions[i].x, -32.0f - 12.0f * static_cast<float>(i)},
                                    positions[i]});
        }
        levels.push_back(level);
    }

    {
        LevelDef level{3, {}};
        for (int i = 0; i < 8; ++i) {
            const float x = 28.0f + 26.0f * static_cast<float>(i);
            level.spawns.push_back({i,
                                    EnemyBehavior::TopShooter,
                                    EnemyFacing::FaceDown,
                                    {x, -24.0f - 8.0f * static_cast<float>(i)},
                                    {x, 24.0f}});
        }
        levels.push_back(level);
    }

    {
        LevelDef level{4, {}};
        const std::array<Vec2, 8> positions = {
            {{72, 26}, {96, 40}, {120, 54}, {144, 40}, {168, 26}, {96, 72}, {144, 72}, {120, 88}}};
        for (std::size_t i = 0; i < positions.size(); ++i) {
            level.spawns.push_back({static_cast<int>(i),
                                    EnemyBehavior::Rammer,
                                    EnemyFacing::FaceMovement,
                                    {positions[i].x, -20.0f - 10.0f * static_cast<float>(i)},
                                    positions[i]});
        }
        levels.push_back(level);
    }

    {
        LevelDef level{5, {}};
        const std::array<Vec2, 8> positions = {
            {{72, 32}, {96, 32}, {144, 32}, {168, 32}, {72, 64}, {96, 64}, {144, 64}, {168, 64}}};
        for (std::size_t i = 0; i < positions.size(); ++i) {
            level.spawns.push_back({static_cast<int>(i),
                                    EnemyBehavior::Circler,
                                    EnemyFacing::FaceMovement,
                                    {positions[i].x, -16.0f - 6.0f * static_cast<float>(i)},
                                    positions[i]});
        }
        levels.push_back(level);
    }

    {
        LevelDef level{6, {}};
        const std::array<Vec2, 9> positions = {{{48, 30},
                                                {84, 48},
                                                {120, 30},
                                                {156, 48},
                                                {192, 30},
                                                {66, 78},
                                                {102, 96},
                                                {138, 78},
                                                {174, 96}}};
        for (std::size_t i = 0; i < positions.size(); ++i) {
            level.spawns.push_back({static_cast<int>(i),
                                    EnemyBehavior::Wander,
                                    EnemyFacing::FaceMovement,
                                    {positions[i].x, -24.0f - 9.0f * static_cast<float>(i)},
                                    positions[i]});
        }
        levels.push_back(level);
    }

    {
        LevelDef level{7, {}};
        const std::array<EnemyBehavior, 10> behaviors = {
            EnemyBehavior::Straight, EnemyBehavior::SlowChase,  EnemyBehavior::TopShooter,
            EnemyBehavior::Rammer,   EnemyBehavior::Circler,    EnemyBehavior::Wander,
            EnemyBehavior::Straight, EnemyBehavior::TopShooter, EnemyBehavior::SlowChase,
            EnemyBehavior::Rammer};
        for (int i = 0; i < 10; ++i) {
            const float x = 30.0f + 20.0f * static_cast<float>(i);
            const float y = (i % 2 == 0) ? 36.0f : 66.0f;
            level.spawns.push_back({i,
                                    behaviors[static_cast<std::size_t>(i)],
                                    EnemyFacing::FacePlayer,
                                    {x, -20.0f - 7.0f * static_cast<float>(i)},
                                    {x, y}});
        }
        levels.push_back(level);
    }

    {
        LevelDef level{8, {}};
        const std::array<Vec2, 10> positions = {{{40, 24},
                                                 {120, 24},
                                                 {200, 24},
                                                 {56, 48},
                                                 {184, 48},
                                                 {72, 72},
                                                 {168, 72},
                                                 {88, 96},
                                                 {152, 96},
                                                 {120, 120}}};
        for (std::size_t i = 0; i < positions.size(); ++i) {
            level.spawns.push_back({static_cast<int>(i),
                                    EnemyBehavior::Circler,
                                    EnemyFacing::FacePlayer,
                                    {positions[i].x, -24.0f - 10.0f * static_cast<float>(i)},
                                    positions[i]});
        }
        levels.push_back(level);
    }

    {
        LevelDef level{9, {}};
        for (int i = 0; i < 12; ++i) {
            const float x = 24.0f + 18.0f * static_cast<float>(i);
            const float y = 22.0f + 10.0f * std::sin(static_cast<float>(i) * 0.9f);
            level.spawns.push_back({i,
                                    EnemyBehavior::TopShooter,
                                    EnemyFacing::FaceDown,
                                    {x, -10.0f - 6.0f * static_cast<float>(i)},
                                    {x, y}});
        }
        levels.push_back(level);
    }

    {
        LevelDef level{10, {}};
        const std::array<EnemyBehavior, 12> behaviors = {
            EnemyBehavior::Rammer,    EnemyBehavior::SlowChase,  EnemyBehavior::Wander,
            EnemyBehavior::Straight,  EnemyBehavior::TopShooter, EnemyBehavior::Circler,
            EnemyBehavior::Straight,  EnemyBehavior::TopShooter, EnemyBehavior::Wander,
            EnemyBehavior::SlowChase, EnemyBehavior::Circler,    EnemyBehavior::Rammer};
        const std::array<Vec2, 12> positions = {{{48, 26},
                                                 {78, 38},
                                                 {108, 26},
                                                 {138, 38},
                                                 {168, 26},
                                                 {198, 38},
                                                 {63, 70},
                                                 {93, 82},
                                                 {123, 70},
                                                 {153, 82},
                                                 {183, 70},
                                                 {120, 104}}};
        for (std::size_t i = 0; i < positions.size(); ++i) {
            level.spawns.push_back({static_cast<int>(i),
                                    behaviors[i],
                                    EnemyFacing::FaceMovement,
                                    {positions[i].x, -18.0f - 8.0f * static_cast<float>(i)},
                                    positions[i]});
        }
        levels.push_back(level);
    }

    return levels;
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
        WeaponType::Basic,
        WeaponFixture::Center,
        1,
        1,
        0.12f,
        0.0f,
        1600.0f,
        0.5f,
        1.25f,
        1.0f,
        0.0f,
    });
    battle.weapons.push_back({
        WeaponType::Basic,
        WeaponFixture::Splayed,
        0,
        2,
        0.3f,
        0.0f,
        1500.0f,
        0.5f,
        1.25f,
        1.0f,
        12.0f,
    });
}
