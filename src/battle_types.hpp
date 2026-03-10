#pragma once

#include "assets.hpp"
#include "math.hpp"

#include <SDL2/SDL.h>
#include <array>
#include <vector>

inline constexpr int GAME_WIDTH = 240;
inline constexpr int GAME_HEIGHT = 160;
inline constexpr int WINDOW_WIDTH = 1280;
inline constexpr int WINDOW_HEIGHT = 720;

enum class SceneMode {
    Title,
    Battle,
};

enum class BattlePhase {
    LevelIntro,
    Active,
};

enum class EnemyBehavior {
    Straight,
    SlowChase,
    TopShooter,
    Rammer,
    Circler,
    Wander,
};

enum class EnemyFacing {
    FacePlayer,
    FaceMovement,
    FaceDown,
};

enum class WeaponType {
    Basic,
    Missile,
};

enum class WeaponFixture {
    Center,
    EvenlySpread,
    Splayed,
};

struct Weapon {
    WeaponType type{WeaponType::Basic};
    WeaponFixture fixture{WeaponFixture::Center};
    int projectile_tile{1};
    int projectile_count{1};
    float cooldown{0.12f};
    float cooldown_timer{0.0f};
    float projectile_speed{1600.0f};
    float projectile_life{0.5f};
    float projectile_radius{1.25f};
    float damage{1.0f};
    float spread_degrees{14.0f};
};

struct CameraState {
    Vec2 center{GAME_WIDTH * 0.5f, GAME_HEIGHT * 0.5f};
    float zoom{0.9f};
};

struct PlayerBullet {
    Vec2 pos{};
    Vec2 vel{};
    float age{0.0f};
    float radius{1.25f};
    float damage{1.0f};
    float life{0.5f};
    float base_height{3.0f};
    float height{3.0f};
    float target_height{3.0f};
    int tile{1};
};

struct EnemyBullet {
    Vec2 pos{};
    Vec2 vel{};
    float age{0.0f};
    float radius{1.25f};
    float damage{1.0f};
    float life{4.0f};
    float base_height{3.0f};
    float height{3.0f};
    float target_height{3.0f};
    int tile{2};
};

struct Particle {
    Vec2 pos{};
    Vec2 vel{};
    float age{0.0f};
    float life{0.5f};
    float base_height{1.5f};
    float height{1.5f};
    float target_height{1.5f};
    int tile{0};
};

struct Star {
    Vec2 pos{};
    float speed{0.0f};
};

struct Ship {
    Vec2 pos{};
    Vec2 vel{};
    float radius{2.75f};
    float shake{0.0f};
    float base_height{5.0f};
    float height{5.0f};
    float target_height{5.0f};
    float hover_phase{0.0f};
};

struct Enemy {
    Vec2 pos{};
    Vec2 vel{};
    Vec2 intro_start{};
    Vec2 formation_pos{};
    Vec2 wander_target{};
    float radius{3.0f};
    float angle_deg{0.0f};
    float behavior_timer{0.0f};
    float shoot_timer{0.0f};
    float ram_timer{0.0f};
    float orbit_phase{0.0f};
    float hp{1.0f};
    float max_hp{1.0f};
    float shake{0.0f};
    float base_height{4.0f};
    float height{4.0f};
    float target_height{4.0f};
    float hover_phase{0.0f};
    int type_id{0};
    EnemyBehavior behavior{EnemyBehavior::Straight};
    EnemyFacing facing{EnemyFacing::FaceDown};
};

struct InputState {
    bool left{false};
    bool right{false};
    bool up{false};
    bool down{false};
};

struct LevelSpawnDef {
    int type_id{0};
    EnemyBehavior behavior{EnemyBehavior::Straight};
    EnemyFacing facing{EnemyFacing::FaceDown};
    Vec2 intro_start{};
    Vec2 formation_pos{};
};

struct LevelDef {
    int number{1};
    std::vector<LevelSpawnDef> spawns{};
};

struct BattleState {
    Ship ship{};
    std::vector<Weapon> weapons{};
    std::vector<PlayerBullet> player_bullets{};
    std::vector<EnemyBullet> enemy_bullets{};
    std::vector<Enemy> enemies{};
    std::vector<Particle> particles{};
    std::vector<Star> stars{};
    CameraState camera{};
    float warp_level{0.0f};
    float level_timer{0.0f};
    float level_text_timer{0.0f};
    float respawn_timer{0.0f};
    float invuln_timer{0.0f};
    bool can_shoot{false};
    bool player_active{true};
    bool debug_colliders{true};
    int lives{3};
    int current_level_index{0};
    int hitstop_frames{0};
    BattlePhase phase{BattlePhase::LevelIntro};
    std::array<int, 2> music_channels{{-1, -1}};
};

struct SessionState {
    SceneMode scene{SceneMode::Title};
    InputState input{};
    BattleState battle{};
    int menu_channel{-1};
};
