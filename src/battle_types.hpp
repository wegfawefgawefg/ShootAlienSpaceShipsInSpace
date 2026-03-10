#pragma once

#include "assets.hpp"
#include "math.hpp"

#include <SDL2/SDL.h>
#include <array>
#include <string>
#include <vector>

inline constexpr int GAME_WIDTH = 640;
inline constexpr int GAME_HEIGHT = 360;
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
    BossSpray,
    BossSpokes,
    BossCharge,
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
    std::string name{"Weapon"};
    WeaponType type{WeaponType::Basic};
    WeaponFixture fixture{WeaponFixture::Center};
    bool automatic{false};
    int projectile_tile{1};
    int projectile_count{1};
    float cooldown{0.12f};
    float cooldown_timer{0.0f};
    float projectile_speed{1600.0f};
    float projectile_life{0.5f};
    float projectile_radius{1.25f};
    float damage{1.0f};
    float spread_degrees{14.0f};
    std::vector<int> attached_pickups{};
};

struct CameraState {
    Vec2 center{GAME_WIDTH * 0.5f, GAME_HEIGHT * 0.5f};
    Vec2 shake_offset{};
    float zoom{0.9f};
    float shake{0.0f};
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
    float speed_scale{1.0f};
    float boss_behavior_timer{0.0f};
    float entry_timer{0.0f};
    int type_id{0};
    int wave_tag{0};
    int boss_group_index{0};
    int boss_behavior_index{0};
    bool is_boss{false};
    bool entering{true};
    std::array<std::array<EnemyBehavior, 3>, 3> boss_behavior_groups{};
    std::array<int, 3> boss_group_sizes{{0, 0, 0}};
    EnemyBehavior behavior{EnemyBehavior::Straight};
    EnemyFacing facing{EnemyFacing::FaceDown};
};

struct InputState {
    bool left{false};
    bool right{false};
    bool up{false};
    bool down{false};
    bool shoot{false};
    bool shoot_pressed{false};
};

struct PickupActor {
    int def_index{-1};
    Vec2 pos{};
    Vec2 vel{};
    Vec2 guide_target{};
    float age{0.0f};
    float radius{5.0f};
    float base_height{2.0f};
    float height{2.0f};
    float target_height{2.0f};
    bool guided{true};
};

struct GoldActor {
    Vec2 pos{};
    Vec2 vel{};
    Vec2 guide_target{};
    float age{0.0f};
    float radius{5.0f};
    float base_height{1.6f};
    float height{1.6f};
    float target_height{1.6f};
    int value{1};
    int tile{0};
    bool guided{true};
};

struct LevelSpawnDef {
    int type_id{0};
    EnemyBehavior behavior{EnemyBehavior::Straight};
    EnemyFacing facing{EnemyFacing::FaceDown};
    Vec2 intro_start{};
    Vec2 formation_pos{};
    float hp{1.0f};
    float radius{3.0f};
    float speed_scale{1.0f};
    bool is_boss{false};
    std::array<std::array<EnemyBehavior, 3>, 3> boss_behavior_groups{};
    std::array<int, 3> boss_group_sizes{{0, 0, 0}};
};

struct WaveDef {
    int number{1};
    bool timed{true};
    float duration{8.0f};
    bool is_boss{false};
    std::vector<LevelSpawnDef> spawns{};
};

struct LevelDef {
    int number{1};
    std::vector<WaveDef> waves{};
};

struct BattleState {
    Ship ship{};
    std::vector<Weapon> weapons{};
    std::vector<PlayerBullet> player_bullets{};
    std::vector<EnemyBullet> enemy_bullets{};
    std::vector<Enemy> enemies{};
    std::vector<Particle> particles{};
    std::vector<PickupActor> pickups{};
    std::vector<GoldActor> gold_pickups{};
    std::vector<Star> stars{};
    std::vector<int> collected_pickups{};
    CameraState camera{};
    float warp_level{0.0f};
    float level_timer{0.0f};
    float level_text_timer{0.0f};
    float wave_timer{0.0f};
    float gold_gain_flash{0.0f};
    float respawn_timer{0.0f};
    float invuln_timer{0.0f};
    float pickup_magnet_radius{22.0f};
    float camera_shake_scale{1.0f};
    float projectile_speed_scale{1.0f};
    bool can_shoot{false};
    bool player_active{true};
    bool debug_colliders{true};
    bool inventory_open{false};
    bool wave_has_timer{false};
    int lives{3};
    int current_level_index{0};
    int current_wave_index{0};
    int active_wave_tag{0};
    int hitstop_frames{0};
    int inventory_selection{0};
    int gold{0};
    int weapon_slots{5};
    BattlePhase phase{BattlePhase::LevelIntro};
    std::array<int, 2> music_channels{{-1, -1}};
};

struct SessionState {
    SceneMode scene{SceneMode::Title};
    InputState input{};
    BattleState battle{};
    int menu_channel{-1};
};
