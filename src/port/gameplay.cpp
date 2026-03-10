#include "port/gameplay.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>

namespace {

constexpr float PLAYER_ACCEL = 1200.0f;
constexpr float PLAYER_DRAG = 10.0f;
constexpr float PLAYER_INVULN_TIME = 1.8f;
constexpr float PLAYER_RESPAWN_TIME = 1.0f;
constexpr float PARTICLE_LIFE = 0.7f;
constexpr float LEVEL_INTRO_DURATION = 3.0f;
constexpr float LEVEL_TEXT_DURATION = 1.3f;
constexpr float BASE_WARP_LEVEL = 1.0f;
constexpr float INTRO_WARP_LEVEL = 16.0f;

constexpr int SMALL_ENEMY_GRID_START_X = 4;
constexpr int SMALL_ENEMY_GRID_START_Y = 0;
constexpr int SMALL_ENEMY_GRID_COLS = 6;
constexpr int SMALL_ENEMY_GRID_ROWS = 6;

constexpr float UI_LEFT_WIDTH = 24.0f;
constexpr float UI_RIGHT_WIDTH = 64.0f;
constexpr float PLAYFIELD_LEFT = UI_LEFT_WIDTH;
constexpr float PLAYFIELD_WIDTH = GAME_WIDTH - UI_LEFT_WIDTH - UI_RIGHT_WIDTH;
constexpr float PLAYFIELD_HEIGHT = GAME_HEIGHT;

float random_unit() {
    return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
}

float random_range(float min_value, float max_value) {
    return min_value + (max_value - min_value) * random_unit();
}

int random_index(int count) {
    return std::rand() % count;
}

int small_enemy_tile(int visual_index) {
    const int wrapped = visual_index % (SMALL_ENEMY_GRID_COLS * SMALL_ENEMY_GRID_ROWS);
    const int col = wrapped % SMALL_ENEMY_GRID_COLS;
    const int row = wrapped / SMALL_ENEMY_GRID_COLS;
    return (SMALL_ENEMY_GRID_START_Y + row) * 10 + SMALL_ENEMY_GRID_START_X + col;
}

int player_ship_tile_center(int ship_index) {
    return (ship_index % 5) * 10 + 1;
}

Vec2 lerp(Vec2 a, Vec2 b, float t) {
    return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
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

void draw_texture(SDL_Renderer* renderer, SDL_Texture* texture, const SDL_FRect& dst) {
    SDL_RenderCopyF(renderer, texture, nullptr, &dst);
}

void draw_text(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, float x, float y,
               SDL_Color color) {
    if (!font) {
        return;
    }
    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (!surface) {
        return;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FRect dst = {x, y, static_cast<float>(surface->w), static_cast<float>(surface->h)};
    SDL_FreeSurface(surface);
    if (!texture) {
        return;
    }
    SDL_RenderCopyF(renderer, texture, nullptr, &dst);
    SDL_DestroyTexture(texture);
}

void stop_channel_if_playing(int& channel) {
    if (channel >= 0) {
        Mix_HaltChannel(channel);
        channel = -1;
    }
}

void stop_battle_music(BattleState& battle) {
    for (int& channel : battle.music_channels) {
        stop_channel_if_playing(channel);
    }
}

void start_battle_music(BattleState& battle, Assets& assets) {
    for (std::size_t i = 0; i < battle.music_channels.size(); ++i) {
        battle.music_channels[i] = Mix_PlayChannel(-1, assets.battle_music[i], -1);
    }
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
            24 + random_index(6),
        });
    }
}

void populate_starfield(BattleState& battle) {
    battle.stars.clear();
    battle.stars.reserve(200);
    for (int i = 0; i < 200; ++i) {
        const float y = random_range(-GAME_HEIGHT * 0.5f, GAME_HEIGHT * 1.5f);
        const float x = random_range(-GAME_WIDTH * 0.5f, GAME_WIDTH * 1.5f);
        const float center_x = GAME_WIDTH * 0.5f;
        const float speed = 20.0f + std::pow(std::abs(center_x - x) * 0.08f, 2.0f);
        battle.stars.push_back({{x, y}, speed});
    }
}

void add_star(BattleState& battle, float y = -GAME_HEIGHT * 0.5f) {
    const float x = random_range(-GAME_WIDTH * 0.5f, GAME_WIDTH * 1.5f);
    const float center_x = GAME_WIDTH * 0.5f;
    const float speed = 20.0f + std::pow(std::abs(center_x - x) * 0.08f, 2.0f);
    battle.stars.push_back({{x, y}, speed});
}

Vec2 world_to_screen(const CameraState& camera, Vec2 world_pos) {
    return {PLAYFIELD_LEFT + PLAYFIELD_WIDTH * 0.5f + (world_pos.x - camera.center.x) * camera.zoom,
            PLAYFIELD_HEIGHT * 0.5f + (world_pos.y - camera.center.y) * camera.zoom};
}

float camera_scale(float radius, float zoom) {
    return std::max(1.0f, radius * zoom);
}

void draw_world_sprite(SDL_Renderer* renderer, const TextureAtlas& atlas, int tile_index,
                       const CameraState& camera, Vec2 world_pos, float angle_deg = 0.0f,
                       Vec2 jitter = {0.0f, 0.0f}) {
    const SDL_Rect src = atlas_tile_rect(atlas, tile_index);
    const Vec2 screen_pos = world_to_screen(camera, world_pos + jitter);
    const SDL_FRect dst = {
        screen_pos.x - static_cast<float>(atlas.tile_width) * 0.5f * camera.zoom,
        screen_pos.y - static_cast<float>(atlas.tile_height) * 0.5f * camera.zoom,
        static_cast<float>(atlas.tile_width) * camera.zoom,
        static_cast<float>(atlas.tile_height) * camera.zoom,
    };
    SDL_RenderCopyExF(renderer, atlas.texture, &src, &dst, angle_deg, nullptr, SDL_FLIP_NONE);
}

void draw_ui_sprite(SDL_Renderer* renderer, const TextureAtlas& atlas, int tile_index,
                    float center_x, float center_y) {
    const SDL_Rect src = atlas_tile_rect(atlas, tile_index);
    const SDL_FRect dst = {
        center_x - static_cast<float>(atlas.tile_width) * 0.5f,
        center_y - static_cast<float>(atlas.tile_height) * 0.5f,
        static_cast<float>(atlas.tile_width),
        static_cast<float>(atlas.tile_height),
    };
    SDL_RenderCopyF(renderer, atlas.texture, &src, &dst);
}

void draw_debug_box(SDL_Renderer* renderer, const CameraState& camera, Vec2 pos, float radius,
                    SDL_Color color) {
    const Vec2 screen = world_to_screen(camera, pos);
    const float size = camera_scale(radius * 2.0f, camera.zoom);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    const SDL_FRect rect = {screen.x - size * 0.5f, screen.y - size * 0.5f, size, size};
    SDL_RenderDrawRectF(renderer, &rect);
}

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

const std::vector<LevelDef>& levels() {
    static const std::vector<LevelDef> kLevels = build_levels();
    return kLevels;
}

void add_default_weapons(BattleState& battle) {
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
        2.0f,
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
        2.0f,
        1.0f,
        12.0f,
    });
}

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

void reset_battle(BattleState& battle) {
    battle.ship = {{GAME_WIDTH * 0.5f, GAME_HEIGHT * 0.8f}, {0.0f, 0.0f}, 6.0f, 0.0f};
    battle.camera = {{GAME_WIDTH * 0.5f, GAME_HEIGHT * 0.5f}, 0.95f};
    battle.warp_level = BASE_WARP_LEVEL;
    battle.respawn_timer = 0.0f;
    battle.invuln_timer = 0.0f;
    battle.player_active = true;
    battle.lives = 3;
    battle.hitstop_frames = 0;
    battle.particles.clear();
    add_default_weapons(battle);
    populate_starfield(battle);
    start_level(battle, 0);
}

void switch_to_title(GameplayState& gameplay, Assets& assets) {
    stop_battle_music(gameplay.battle);
    if (gameplay.menu_channel < 0) {
        gameplay.menu_channel = Mix_PlayChannel(-1, assets.menu_music, -1);
    }
    gameplay.scene = SceneMode::Title;
}

void switch_to_battle(GameplayState& gameplay, Assets& assets) {
    stop_channel_if_playing(gameplay.menu_channel);
    stop_battle_music(gameplay.battle);
    reset_battle(gameplay.battle);
    start_battle_music(gameplay.battle, assets);
    gameplay.scene = SceneMode::Battle;
}

void spawn_player_bullet(BattleState& battle, const Weapon& weapon, Vec2 spawn_pos,
                         float angle_offset_deg, Assets& assets) {
    Mix_PlayChannel(-1, assets.laser_sounds[static_cast<std::size_t>(random_index(3))], 0);
    const float angle = (-90.0f + angle_offset_deg) * 0.0174532925f;
    const Vec2 dir = {std::cos(angle), std::sin(angle)};
    battle.player_bullets.push_back({
        spawn_pos,
        dir * weapon.projectile_speed,
        0.0f,
        weapon.projectile_radius,
        weapon.damage,
        weapon.projectile_life,
        weapon.projectile_tile,
    });
}

void fire_weapon(BattleState& battle, Weapon& weapon, Assets& assets) {
    if (weapon.cooldown_timer > 0.0f) {
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
    } else {
        if (weapon.projectile_count <= 1) {
            spawn_player_bullet(battle, weapon, battle.ship.pos, 0.0f, assets);
        } else {
            const float start = -weapon.spread_degrees *
                                (static_cast<float>(weapon.projectile_count) - 1.0f) * 0.5f;
            for (int i = 0; i < weapon.projectile_count; ++i) {
                const float angle = start + weapon.spread_degrees * static_cast<float>(i);
                spawn_player_bullet(battle, weapon, battle.ship.pos, angle, assets);
            }
        }
    }

    weapon.cooldown_timer = weapon.cooldown;
}

void spawn_enemy_bullet(BattleState& battle, const Enemy& enemy) {
    Vec2 dir = battle.ship.pos - enemy.pos;
    const float len = std::max(vec2_length(dir), 0.001f);
    dir = {dir.x / len, dir.y / len};
    battle.enemy_bullets.push_back({enemy.pos, dir * 120.0f, 0.0f, 2.0f, 1.0f, 4.0f, 2});
}

void update_ship(BattleState& battle, const InputState& input, float dt) {
    if (!battle.player_active) {
        return;
    }

    Vec2 thrust{};
    if (input.left)
        thrust.x -= PLAYER_ACCEL;
    if (input.right)
        thrust.x += PLAYER_ACCEL;
    if (input.up)
        thrust.y -= PLAYER_ACCEL;
    if (input.down)
        thrust.y += PLAYER_ACCEL;

    battle.ship.vel += thrust * dt;
    battle.ship.vel += (-PLAYER_DRAG * battle.ship.vel) * dt;
    battle.ship.pos += battle.ship.vel * dt;
    battle.ship.pos.x = clampf(battle.ship.pos.x, 8.0f, GAME_WIDTH - 8.0f);
    battle.ship.pos.y = clampf(battle.ship.pos.y, 8.0f, GAME_HEIGHT - 8.0f);
}

void update_camera(BattleState& battle, float dt) {
    const float target_zoom =
        0.9f - std::min(0.2f, 0.01f * static_cast<float>(battle.enemies.size()));
    battle.camera.zoom += (target_zoom - battle.camera.zoom) * std::min(1.0f, dt * 3.0f);
    battle.camera.center.x +=
        (battle.ship.pos.x - battle.camera.center.x) * std::min(1.0f, dt * 4.0f);
    battle.camera.center.y +=
        (battle.ship.pos.y - battle.camera.center.y) * std::min(1.0f, dt * 4.0f);
    battle.camera.center.x = clampf(battle.camera.center.x, 40.0f, GAME_WIDTH - 40.0f);
    battle.camera.center.y = clampf(battle.camera.center.y, 32.0f, GAME_HEIGHT - 32.0f);
}

void update_stars(BattleState& battle, float dt) {
    if (random_unit() > 0.5f) {
        add_star(battle);
    }

    const float speed_scale = dt * 10.0f * (1.0f + battle.warp_level);
    for (Star& star : battle.stars) {
        star.pos.y += star.speed * speed_scale;
    }

    battle.stars.erase(
        std::remove_if(battle.stars.begin(), battle.stars.end(),
                       [](const Star& star) { return star.pos.y >= GAME_HEIGHT * 1.5f; }),
        battle.stars.end());
}

void update_warp(BattleState& battle, float dt) {
    const float target =
        battle.phase == BattlePhase::LevelIntro ? INTRO_WARP_LEVEL : BASE_WARP_LEVEL;
    battle.warp_level += (target - battle.warp_level) * std::min(1.0f, dt * 3.5f);
}

void update_player_bullets(BattleState& battle, float dt) {
    for (Weapon& weapon : battle.weapons) {
        weapon.cooldown_timer = std::max(0.0f, weapon.cooldown_timer - dt);
    }

    for (PlayerBullet& bullet : battle.player_bullets) {
        bullet.age += dt;
        bullet.pos += bullet.vel * dt;
    }

    battle.player_bullets.erase(
        std::remove_if(battle.player_bullets.begin(), battle.player_bullets.end(),
                       [](const PlayerBullet& bullet) {
                           return bullet.age > bullet.life || bullet.pos.y < -4.0f ||
                                  bullet.pos.x < -8.0f || bullet.pos.x > GAME_WIDTH + 8.0f;
                       }),
        battle.player_bullets.end());
}

void update_enemy_bullets(BattleState& battle, float dt) {
    for (EnemyBullet& bullet : battle.enemy_bullets) {
        bullet.age += dt;
        bullet.pos += bullet.vel * dt;
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
    }

    battle.particles.erase(
        std::remove_if(battle.particles.begin(), battle.particles.end(),
                       [](const Particle& particle) { return particle.age >= particle.life; }),
        battle.particles.end());
}

void update_intro(BattleState& battle, float dt) {
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

void update_active(BattleState& battle, float dt) {
    battle.level_timer += dt;
    for (Enemy& enemy : battle.enemies) {
        update_enemy_behavior(enemy, battle, dt);
    }
}

void add_hitstop(BattleState& battle, int frames) {
    battle.hitstop_frames += frames;
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
    battle.ship.shake = 6.0f;
    battle.can_shoot = false;
    add_hitstop(battle, 4);
}

void update_player_state(BattleState& battle, float dt) {
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

void resolve_collisions(BattleState& battle) {
    std::vector<Enemy> surviving_enemies;
    surviving_enemies.reserve(battle.enemies.size());

    for (Enemy& enemy : battle.enemies) {
        bool dead = false;
        for (PlayerBullet& bullet : battle.player_bullets) {
            const float radius = bullet.radius + enemy.radius;
            if (vec2_length_sq(enemy.pos - bullet.pos) <= radius * radius) {
                bullet.age = 999.0f;
                enemy.hp -= bullet.damage;
                enemy.shake = 4.0f;
                add_hitstop(battle, 1);
                if (enemy.hp <= 0.0f) {
                    spawn_explosion(battle, enemy.pos, 10);
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
                break;
            }
        }

        for (const Enemy& enemy : battle.enemies) {
            const float radius = enemy.radius + battle.ship.radius;
            if (vec2_length_sq(battle.ship.pos - enemy.pos) <= radius * radius) {
                damage_player(battle);
                break;
            }
        }
    }

    battle.enemy_bullets.erase(
        std::remove_if(battle.enemy_bullets.begin(), battle.enemy_bullets.end(),
                       [](const EnemyBullet& bullet) { return bullet.age > 900.0f; }),
        battle.enemy_bullets.end());
}

void maybe_advance_level(BattleState& battle) {
    if (battle.phase == BattlePhase::Active && battle.enemies.empty()) {
        start_level(battle, battle.current_level_index + 1);
    }
}

void render_title(const Assets& assets, SDL_Renderer* renderer, Uint64 ticks) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    const float t = static_cast<float>(ticks);
    const float virtual_width = GAME_WIDTH * 2.0f;
    const float virtual_height = GAME_HEIGHT * 2.0f;
    const float offset = virtual_height / 10.0f;
    const float background_x = 0.3f * std::sin(t * 0.0013f) * offset - offset * 1.5f;
    const float background_y = 0.3f * std::sin(t * 0.003f) * offset - offset * 1.5f;
    const float menu_scale = 0.5f;

    draw_texture(renderer, assets.menu_background,
                 {background_x * menu_scale, background_y * menu_scale,
                  virtual_width * 1.2f * menu_scale, virtual_height * 1.2f * menu_scale});

    const float title_y = (0.5f + std::sin(t * 0.001f) * 0.5f) * virtual_height * 0.05f;
    const SDL_FRect shadow_dst = {
        -virtual_width * 0.1f * menu_scale,
        (title_y - virtual_height * 0.1f) * menu_scale,
        virtual_width * 1.2f * 1.1f * menu_scale,
        virtual_height * 1.2f * 1.1f * menu_scale,
    };
    const SDL_FRect title_dst = {
        -virtual_width * 0.06f * menu_scale,
        (title_y - virtual_height * 0.08f) * menu_scale,
        virtual_width * 1.2f * menu_scale,
        virtual_height * 1.2f * menu_scale,
    };

    SDL_SetTextureColorMod(assets.menu_title, 0, 0, 0);
    SDL_SetTextureAlphaMod(assets.menu_title, 220);
    SDL_RenderCopyF(renderer, assets.menu_title, nullptr, &shadow_dst);
    SDL_SetTextureColorMod(assets.menu_title, 255, 255, 255);
    SDL_SetTextureAlphaMod(assets.menu_title, 255);
    SDL_RenderCopyF(renderer, assets.menu_title, nullptr, &title_dst);
}

void render_level_text(const GameplayState& gameplay, const Assets& assets,
                       SDL_Renderer* renderer) {
    if (gameplay.scene != SceneMode::Battle || gameplay.battle.phase != BattlePhase::LevelIntro ||
        !assets.ui_font) {
        return;
    }

    const float t = gameplay.battle.level_text_timer;
    if (t > LEVEL_TEXT_DURATION) {
        return;
    }

    const std::string label = "Level " + std::to_string(gameplay.battle.current_level_index + 1);
    SDL_Color color{255, 255, 255, 255};
    SDL_Surface* surface = TTF_RenderUTF8_Blended(assets.ui_font, label.c_str(), color);
    if (!surface) {
        return;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    const int width = surface->w;
    const int height = surface->h;
    SDL_FreeSurface(surface);
    if (!texture) {
        return;
    }

    const float wiggle = std::sin(t * 10.0f) * 10.0f;
    const float fly = ease_out_cubic(clampf(t / LEVEL_TEXT_DURATION, 0.0f, 1.0f));
    const SDL_FRect dst = {
        PLAYFIELD_LEFT + PLAYFIELD_WIDTH * 0.5f - static_cast<float>(width) * 0.5f +
            std::sin(t * 6.0f) * 6.0f,
        16.0f + wiggle + (1.0f - fly) * 18.0f,
        static_cast<float>(width),
        static_cast<float>(height),
    };
    SDL_RenderCopyF(renderer, texture, nullptr, &dst);
    SDL_DestroyTexture(texture);
}

void render_ui(const GameplayState& gameplay, const Assets& assets, SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 8, 12, 20, 220);
    const SDL_FRect left = {0.0f, 0.0f, UI_LEFT_WIDTH, GAME_HEIGHT};
    const SDL_FRect right = {GAME_WIDTH - UI_RIGHT_WIDTH, 0.0f, UI_RIGHT_WIDTH, GAME_HEIGHT};
    SDL_RenderFillRectF(renderer, &left);
    SDL_RenderFillRectF(renderer, &right);

    for (int i = 0; i < gameplay.battle.lives; ++i) {
        draw_ui_sprite(renderer, assets.ships, player_ship_tile_center(0), 12.0f,
                       14.0f + 12.0f * static_cast<float>(i));
    }

    SDL_Color text = {255, 255, 255, 255};
    draw_text(renderer, assets.ui_font,
              "L" + std::to_string(gameplay.battle.current_level_index + 1),
              GAME_WIDTH - UI_RIGHT_WIDTH + 4.0f, 4.0f, text);
    draw_text(renderer, assets.ui_font, "Party 1", GAME_WIDTH - UI_RIGHT_WIDTH + 4.0f, 18.0f, text);
    draw_text(renderer, assets.ui_font, "Wing 0", GAME_WIDTH - UI_RIGHT_WIDTH + 4.0f, 32.0f, text);

    float y = 48.0f;
    for (std::size_t i = 0; i < gameplay.battle.weapons.size(); ++i) {
        const Weapon& weapon = gameplay.battle.weapons[i];
        std::string type = weapon.type == WeaponType::Basic ? "basic" : "missile";
        std::string fixture = "center";
        if (weapon.fixture == WeaponFixture::EvenlySpread) {
            fixture = "even";
        } else if (weapon.fixture == WeaponFixture::Splayed) {
            fixture = "splay";
        }
        draw_text(renderer, assets.ui_font, std::to_string(static_cast<int>(i)) + ":" + type,
                  GAME_WIDTH - UI_RIGHT_WIDTH + 4.0f, y, text);
        y += 12.0f;
        draw_text(renderer, assets.ui_font,
                  "p" + std::to_string(weapon.projectile_tile) + " " + fixture,
                  GAME_WIDTH - UI_RIGHT_WIDTH + 4.0f, y, text);
        y += 16.0f;
    }
}

void render_battle(const GameplayState& gameplay, const Assets& assets, SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    for (const Star& star : gameplay.battle.stars) {
        draw_world_sprite(renderer, assets.particles, 28, gameplay.battle.camera, star.pos);
    }

    for (const Enemy& enemy : gameplay.battle.enemies) {
        const Vec2 jitter = {random_range(-enemy.shake, enemy.shake),
                             random_range(-enemy.shake, enemy.shake)};
        draw_world_sprite(renderer, assets.ships, small_enemy_tile(enemy.type_id),
                          gameplay.battle.camera, enemy.pos, enemy.angle_deg, jitter);
    }

    for (const PlayerBullet& bullet : gameplay.battle.player_bullets) {
        draw_world_sprite(renderer, assets.particles, bullet.tile, gameplay.battle.camera,
                          bullet.pos, angle_from_vector(bullet.vel));
    }

    for (const EnemyBullet& bullet : gameplay.battle.enemy_bullets) {
        draw_world_sprite(renderer, assets.particles, bullet.tile, gameplay.battle.camera,
                          bullet.pos, angle_from_vector(bullet.vel));
    }

    for (const Particle& particle : gameplay.battle.particles) {
        draw_world_sprite(renderer, assets.particles, particle.tile, gameplay.battle.camera,
                          particle.pos);
    }

    const bool hidden_for_blink = gameplay.battle.invuln_timer > 0.0f &&
                                  std::fmod(gameplay.battle.invuln_timer * 20.0f, 2.0f) < 1.0f;
    if (gameplay.battle.player_active && !hidden_for_blink) {
        int ship_tile = player_ship_tile_center(0);
        if (gameplay.battle.ship.vel.x <= -10.0f) {
            ship_tile = 0;
        } else if (gameplay.battle.ship.vel.x >= 10.0f) {
            ship_tile = 2;
        }
        const Vec2 jitter = {random_range(-gameplay.battle.ship.shake, gameplay.battle.ship.shake),
                             random_range(-gameplay.battle.ship.shake, gameplay.battle.ship.shake)};
        draw_world_sprite(renderer, assets.ships, ship_tile, gameplay.battle.camera,
                          gameplay.battle.ship.pos, 0.0f, jitter);
    }

    if (gameplay.battle.debug_colliders) {
        for (const Enemy& enemy : gameplay.battle.enemies) {
            draw_debug_box(renderer, gameplay.battle.camera, enemy.pos, enemy.radius,
                           {255, 64, 64, 255});
        }
        for (const PlayerBullet& bullet : gameplay.battle.player_bullets) {
            draw_debug_box(renderer, gameplay.battle.camera, bullet.pos, bullet.radius,
                           {255, 255, 64, 255});
        }
        for (const EnemyBullet& bullet : gameplay.battle.enemy_bullets) {
            draw_debug_box(renderer, gameplay.battle.camera, bullet.pos, bullet.radius,
                           {64, 255, 255, 255});
        }
        if (gameplay.battle.player_active) {
            draw_debug_box(renderer, gameplay.battle.camera, gameplay.battle.ship.pos,
                           gameplay.battle.ship.radius, {64, 255, 64, 255});
        }
    }

    render_level_text(gameplay, assets, renderer);
    render_ui(gameplay, assets, renderer);
}

} // namespace

void gameplay_init(GameplayState& gameplay, Assets& assets) {
    gameplay.scene = SceneMode::Title;
    gameplay.menu_channel = Mix_PlayChannel(-1, assets.menu_music, -1);
    reset_battle(gameplay.battle);
}

void gameplay_handle_event(GameplayState& gameplay, Assets& assets, const SDL_Event& event,
                           bool& running) {
    if (event.type == SDL_QUIT) {
        running = false;
        return;
    }

    if (event.type != SDL_KEYDOWN || event.key.repeat != 0) {
        return;
    }

    const SDL_Scancode scancode = event.key.keysym.scancode;
    if (gameplay.scene == SceneMode::Title) {
        if (scancode == SDL_SCANCODE_ESCAPE || scancode == SDL_SCANCODE_Q) {
            running = false;
            return;
        }
        switch_to_battle(gameplay, assets);
        return;
    }

    if (gameplay.scene == SceneMode::Battle) {
        if (scancode == SDL_SCANCODE_ESCAPE || scancode == SDL_SCANCODE_Q) {
            switch_to_title(gameplay, assets);
            return;
        }
        if (scancode == SDL_SCANCODE_SPACE && gameplay.battle.can_shoot &&
            gameplay.battle.player_active) {
            for (Weapon& weapon : gameplay.battle.weapons) {
                fire_weapon(gameplay.battle, weapon, assets);
            }
        }
    }
}

void gameplay_update(GameplayState& gameplay, Assets& assets, float dt) {
    if (gameplay.scene != SceneMode::Battle) {
        return;
    }

    if (gameplay.battle.hitstop_frames > 0) {
        gameplay.battle.hitstop_frames -= 1;
        update_particles(gameplay.battle, dt * 0.25f);
        return;
    }

    const Uint8* keyboard = SDL_GetKeyboardState(nullptr);
    gameplay.input.left = keyboard[SDL_SCANCODE_LEFT] != 0 || keyboard[SDL_SCANCODE_A] != 0;
    gameplay.input.right = keyboard[SDL_SCANCODE_RIGHT] != 0 || keyboard[SDL_SCANCODE_D] != 0;
    gameplay.input.up = keyboard[SDL_SCANCODE_UP] != 0 || keyboard[SDL_SCANCODE_W] != 0;
    gameplay.input.down = keyboard[SDL_SCANCODE_DOWN] != 0 || keyboard[SDL_SCANCODE_S] != 0;

    update_ship(gameplay.battle, gameplay.input, dt);
    update_camera(gameplay.battle, dt);
    update_warp(gameplay.battle, dt);
    update_stars(gameplay.battle, dt);
    update_player_state(gameplay.battle, dt);

    if (gameplay.battle.phase == BattlePhase::LevelIntro) {
        update_intro(gameplay.battle, dt);
    } else {
        update_active(gameplay.battle, dt);
    }

    update_player_bullets(gameplay.battle, dt);
    update_enemy_bullets(gameplay.battle, dt);
    update_particles(gameplay.battle, dt);
    resolve_collisions(gameplay.battle);
    maybe_advance_level(gameplay.battle);

    if (gameplay.battle.lives <= 0 && !gameplay.battle.player_active &&
        gameplay.battle.particles.empty()) {
        switch_to_title(gameplay, assets);
    }
}

void gameplay_render(const GameplayState& gameplay, const Assets& assets, SDL_Renderer* renderer,
                     Uint64 ticks) {
    if (gameplay.scene == SceneMode::Title) {
        render_title(assets, renderer, ticks);
        return;
    }

    render_battle(gameplay, assets, renderer);
}
