#include "level_flow.hpp"

#include "damage.hpp"
#include "enemy_behavior.hpp"
#include "level_data.hpp"
#include "player_weapons.hpp"

#include <SDL2/SDL_mixer.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace {

constexpr float PLAYER_ACCEL = 1200.0f;
constexpr float PLAYER_DRAG = 10.0f;
constexpr float BASE_WARP_LEVEL = 1.0f;
constexpr float INTRO_WARP_LEVEL = 16.0f;

float random_unit() {
    return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
}

float random_range(float min_value, float max_value) {
    return min_value + (max_value - min_value) * random_unit();
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
    seed_default_weapons(battle);
    populate_starfield(battle);
    start_level(battle, 0);
}

void switch_to_title(SessionState& session, Assets& assets) {
    stop_battle_music(session.battle);
    if (session.menu_channel < 0) {
        session.menu_channel = Mix_PlayChannel(-1, assets.menu_music, -1);
    }
    session.scene = SceneMode::Title;
}

void switch_to_battle(SessionState& session, Assets& assets) {
    stop_channel_if_playing(session.menu_channel);
    stop_battle_music(session.battle);
    reset_battle(session.battle);
    start_battle_music(session.battle, assets);
    session.scene = SceneMode::Battle;
}

void update_player_ship(BattleState& battle, const InputState& input, float dt) {
    if (!battle.player_active) {
        return;
    }

    Vec2 thrust{};
    if (input.left) {
        thrust.x -= PLAYER_ACCEL;
    }
    if (input.right) {
        thrust.x += PLAYER_ACCEL;
    }
    if (input.up) {
        thrust.y -= PLAYER_ACCEL;
    }
    if (input.down) {
        thrust.y += PLAYER_ACCEL;
    }

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

void update_starfield(BattleState& battle, float dt) {
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

void maybe_advance_level(BattleState& battle) {
    if (battle.phase == BattlePhase::Active && battle.enemies.empty()) {
        start_level(battle, battle.current_level_index + 1);
    }
}

} // namespace

void session_init(SessionState& session, Assets& assets) {
    session.scene = SceneMode::Title;
    session.menu_channel = Mix_PlayChannel(-1, assets.menu_music, -1);
    reset_battle(session.battle);
}

void session_handle_event(SessionState& session, Assets& assets, const SDL_Event& event,
                          bool& running) {
    if (event.type == SDL_QUIT) {
        running = false;
        return;
    }

    if (event.type != SDL_KEYDOWN || event.key.repeat != 0) {
        return;
    }

    const SDL_Scancode scancode = event.key.keysym.scancode;
    if (session.scene == SceneMode::Title) {
        if (scancode == SDL_SCANCODE_ESCAPE || scancode == SDL_SCANCODE_Q) {
            running = false;
            return;
        }
        switch_to_battle(session, assets);
        return;
    }

    if (session.scene == SceneMode::Battle) {
        if (scancode == SDL_SCANCODE_ESCAPE || scancode == SDL_SCANCODE_Q) {
            switch_to_title(session, assets);
            return;
        }
        if (scancode == SDL_SCANCODE_SPACE && session.battle.can_shoot &&
            session.battle.player_active) {
            fire_player_weapons(session.battle, assets);
        }
    }
}

void session_update(SessionState& session, Assets& assets, float dt) {
    (void)assets;
    if (session.scene != SceneMode::Battle) {
        return;
    }

    if (session.battle.hitstop_frames > 0) {
        session.battle.hitstop_frames -= 1;
        update_particles(session.battle, dt * 0.25f);
        return;
    }

    const Uint8* keyboard = SDL_GetKeyboardState(nullptr);
    session.input.left = keyboard[SDL_SCANCODE_LEFT] != 0 || keyboard[SDL_SCANCODE_A] != 0;
    session.input.right = keyboard[SDL_SCANCODE_RIGHT] != 0 || keyboard[SDL_SCANCODE_D] != 0;
    session.input.up = keyboard[SDL_SCANCODE_UP] != 0 || keyboard[SDL_SCANCODE_W] != 0;
    session.input.down = keyboard[SDL_SCANCODE_DOWN] != 0 || keyboard[SDL_SCANCODE_S] != 0;

    update_player_ship(session.battle, session.input, dt);
    update_camera(session.battle, dt);
    update_warp(session.battle, dt);
    update_starfield(session.battle, dt);
    update_player_respawn(session.battle, dt);

    if (session.battle.phase == BattlePhase::LevelIntro) {
        update_enemy_intro(session.battle, dt);
    } else {
        update_enemy_wave(session.battle, dt);
    }

    update_player_bullets(session.battle, dt);
    update_enemy_bullets(session.battle, dt);
    update_particles(session.battle, dt);
    resolve_damage(session.battle);
    maybe_advance_level(session.battle);

    if (session.battle.lives <= 0 && !session.battle.player_active &&
        session.battle.particles.empty()) {
        switch_to_title(session, assets);
    }
}
