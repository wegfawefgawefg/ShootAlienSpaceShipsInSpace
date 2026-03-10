#include "level_flow.hpp"

#include "battle_render.hpp"
#include "damage.hpp"
#include "enemy_behavior.hpp"
#include "level_data.hpp"
#include "pickups.hpp"
#include "player_weapons.hpp"
#include "shop.hpp"

#include <SDL2/SDL_mixer.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace {

constexpr float PLAYER_ACCEL = 1200.0f;
constexpr float PLAYER_DRAG = 10.0f;
constexpr float BASE_WARP_LEVEL = 0.0f;
constexpr float INTRO_WARP_LEVEL = 1.0f;
constexpr float CAMERA_SHAKE_DECAY = 18.0f;
constexpr int SIM_SUBSTEPS = 2;

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
    battle.stars.reserve(320);
    for (int i = 0; i < 320; ++i) {
        const float y = random_range(-GAME_HEIGHT * 0.5f, GAME_HEIGHT * 1.5f);
        const float x = random_range(-GAME_WIDTH * 0.5f, GAME_WIDTH * 1.5f);
        const float center_x = GAME_WIDTH * 0.5f;
        const float edge_bias = std::abs(center_x - x) / center_x;
        const float speed = 0.25f + edge_bias * edge_bias * 2.4f + random_range(0.0f, 0.8f);
        battle.stars.push_back({{x, y}, speed});
    }
}

void add_star(BattleState& battle, float y = -GAME_HEIGHT * 0.5f) {
    const float x = random_range(-GAME_WIDTH * 0.5f, GAME_WIDTH * 1.5f);
    const float center_x = GAME_WIDTH * 0.5f;
    const float edge_bias = std::abs(center_x - x) / center_x;
    const float speed = 0.25f + edge_bias * edge_bias * 2.4f + random_range(0.0f, 0.8f);
    battle.stars.push_back({{x, y}, speed});
}

void begin_level_transition(BattleState& battle, Assets& assets, int level_index) {
    start_level(battle, level_index);
    Mix_PlayChannel(-1, assets.warp_start, 0);
}

void reset_battle(BattleState& battle, Assets& assets) {
    battle.ship = {{GAME_WIDTH * 0.5f, GAME_HEIGHT * 0.82f},
                   {0.0f, 0.0f},
                   2.75f,
                   0.0f,
                   5.5f,
                   5.5f,
                   5.5f,
                   0.0f};
    battle.camera = {{GAME_WIDTH * 0.5f, GAME_HEIGHT * 0.5f}, {0.0f, 0.0f}, 1.02f, 0.0f};
    battle.warp_level = BASE_WARP_LEVEL;
    battle.respawn_timer = 0.0f;
    battle.invuln_timer = 0.0f;
    battle.pickup_magnet_radius = 22.0f;
    battle.camera_shake_scale = 1.0f;
    battle.projectile_speed_scale = 1.0f;
    battle.gold = 0;
    battle.gold_gain_flash = 0.0f;
    battle.player_active = true;
    battle.lives = 3;
    battle.hitstop_frames = 0;
    battle.inventory_open = false;
    battle.inventory_selection = 0;
    battle.particles.clear();
    battle.pickups.clear();
    battle.gold_pickups.clear();
    battle.player_beams.clear();
    battle.owned_pickups.clear();
    battle.recent_pickups.clear();
    battle.weapon_stash.clear();
    battle.shop_offers.clear();
    seed_default_weapons(battle);
    populate_starfield(battle);
    begin_level_transition(battle, assets, 0);
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
    reset_battle(session.battle, assets);
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
    battle.ship.pos.x = clampf(battle.ship.pos.x, 14.0f, GAME_WIDTH - 14.0f);
    battle.ship.pos.y = clampf(battle.ship.pos.y, 20.0f, GAME_HEIGHT - 18.0f);
    battle.ship.hover_phase += dt * 5.0f + vec2_length(battle.ship.vel) * 0.02f;
    battle.ship.target_height = battle.ship.base_height + std::sin(battle.ship.hover_phase) * 0.7f -
                                battle.ship.shake * 0.08f;
    battle.ship.height +=
        (battle.ship.target_height - battle.ship.height) * std::min(1.0f, dt * 10.0f);
}

void update_camera(BattleState& battle, float dt) {
    battle.camera.center = {GAME_WIDTH * 0.5f, GAME_HEIGHT * 0.5f};
    battle.camera.zoom += (1.02f - battle.camera.zoom) * std::min(1.0f, dt * 5.0f);
    battle.camera.shake = std::max(0.0f, battle.camera.shake - dt * CAMERA_SHAKE_DECAY);
    if (battle.camera.shake > 0.0f) {
        battle.camera.shake_offset = {random_range(-battle.camera.shake, battle.camera.shake),
                                      random_range(-battle.camera.shake, battle.camera.shake)};
    } else {
        battle.camera.shake_offset = {0.0f, 0.0f};
    }
}

void update_starfield(BattleState& battle, float dt) {
    if (random_unit() > 0.2f) {
        add_star(battle);
    }

    for (Star& star : battle.stars) {
        const float edge_bias = std::abs(star.pos.x - GAME_WIDTH * 0.5f) / (GAME_WIDTH * 0.5f);
        const float normal_speed = 7.0f + star.speed * 10.0f;
        const float warp_speed =
            battle.warp_level * battle.warp_level * (10.0f + edge_bias * 120.0f);
        const float side_drift = -battle.ship.vel.x * (0.02f + star.speed * 0.015f);
        star.pos.x += side_drift * dt;
        star.pos.y += (normal_speed + warp_speed) * dt;
        star.pos.x = wrapf(star.pos.x, -GAME_WIDTH * 0.25f, GAME_WIDTH * 1.25f);
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

bool current_wave_cleared(const BattleState& battle) {
    return std::none_of(battle.enemies.begin(), battle.enemies.end(), [&](const Enemy& enemy) {
        return enemy.wave_tag == battle.active_wave_tag;
    });
}

} // namespace

void session_init(SessionState& session, Assets& assets) {
    session.scene = SceneMode::Title;
    session.menu_channel = Mix_PlayChannel(-1, assets.menu_music, -1);
    reset_battle(session.battle, assets);
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
        if (session.battle.phase == BattlePhase::Shop) {
            if (scancode == SDL_SCANCODE_TAB) {
                session.battle.inventory_open = !session.battle.inventory_open;
                return;
            }
            if (session.battle.inventory_open) {
                if (scancode == SDL_SCANCODE_UP || scancode == SDL_SCANCODE_W) {
                    session.battle.inventory_selection =
                        std::max(0, session.battle.inventory_selection - 1);
                } else if (scancode == SDL_SCANCODE_DOWN || scancode == SDL_SCANCODE_S) {
                    session.battle.inventory_selection += 1;
                } else if (scancode == SDL_SCANCODE_PAGEUP) {
                    session.battle.inventory_selection =
                        std::max(0, session.battle.inventory_selection - 5);
                } else if (scancode == SDL_SCANCODE_PAGEDOWN) {
                    session.battle.inventory_selection += 5;
                } else if (scancode == SDL_SCANCODE_RETURN || scancode == SDL_SCANCODE_E ||
                           scancode == SDL_SCANCODE_SPACE) {
                    move_selected_inventory_item(session.battle);
                } else if (scancode == SDL_SCANCODE_X || scancode == SDL_SCANCODE_DELETE ||
                           scancode == SDL_SCANCODE_BACKSPACE) {
                    sell_selected_inventory_item(session.battle);
                }
                return;
            }

            if (scancode == SDL_SCANCODE_LEFT || scancode == SDL_SCANCODE_A ||
                scancode == SDL_SCANCODE_UP || scancode == SDL_SCANCODE_W) {
                session.battle.shop_selection = std::max(0, session.battle.shop_selection - 1);
            } else if (scancode == SDL_SCANCODE_RIGHT || scancode == SDL_SCANCODE_D ||
                       scancode == SDL_SCANCODE_DOWN || scancode == SDL_SCANCODE_S) {
                session.battle.shop_selection += 1;
            } else if (scancode == SDL_SCANCODE_R) {
                if (session.battle.gold >= session.battle.shop_reroll_cost) {
                    session.battle.gold -= session.battle.shop_reroll_cost;
                    reroll_shop(session.battle);
                    session.battle.shop_reroll_cost += 3;
                }
            } else if (scancode == SDL_SCANCODE_RETURN || scancode == SDL_SCANCODE_SPACE) {
                buy_selected_shop_offer(session.battle);
            } else if (scancode == SDL_SCANCODE_N) {
                advance_from_shop(session.battle, assets);
            } else if (scancode == SDL_SCANCODE_ESCAPE || scancode == SDL_SCANCODE_Q) {
                switch_to_title(session, assets);
            }
            return;
        }

        if (scancode == SDL_SCANCODE_TAB) {
            session.battle.inventory_open = !session.battle.inventory_open;
            return;
        }
        if (session.battle.inventory_open) {
            if (scancode == SDL_SCANCODE_UP || scancode == SDL_SCANCODE_W) {
                session.battle.inventory_selection =
                    std::max(0, session.battle.inventory_selection - 1);
            } else if (scancode == SDL_SCANCODE_DOWN || scancode == SDL_SCANCODE_S) {
                session.battle.inventory_selection += 1;
            } else if (scancode == SDL_SCANCODE_PAGEUP) {
                session.battle.inventory_selection =
                    std::max(0, session.battle.inventory_selection - 5);
            } else if (scancode == SDL_SCANCODE_PAGEDOWN) {
                session.battle.inventory_selection += 5;
            }
            return;
        }
        if (scancode == SDL_SCANCODE_ESCAPE || scancode == SDL_SCANCODE_Q) {
            switch_to_title(session, assets);
            return;
        }
    }
}

void session_handle_pointer(SessionState& session, Assets& assets, const ScreenLayout& layout,
                            float x, float y, bool left_pressed) {
    (void)assets;

    if (session.scene != SceneMode::Battle) {
        return;
    }

    if (session.battle.phase == BattlePhase::Shop) {
        if (session.battle.inventory_open) {
            const int hovered = inventory_selectable_index_at(session.battle, layout, x, y,
                                                              session.battle.inventory_selection);
            if (hovered >= 0) {
                session.battle.inventory_selection = hovered;
            }
            if (left_pressed && hovered >= 0) {
                session.battle.inventory_selection = hovered;
            }
            return;
        }

        const int hovered = shop_offer_index_at(session.battle, layout, x, y);
        if (hovered >= 0) {
            session.battle.shop_selection = hovered;
            if (left_pressed) {
                buy_selected_shop_offer(session.battle);
            }
        }
        return;
    }

    if (session.battle.inventory_open) {
        const int hovered = inventory_selectable_index_at(session.battle, layout, x, y,
                                                          session.battle.inventory_selection);
        if (hovered >= 0) {
            session.battle.inventory_selection = hovered;
        }
    }
}

void session_update(SessionState& session, Assets& assets, float dt) {
    if (session.scene != SceneMode::Battle) {
        return;
    }
    update_camera(session.battle, dt);
    update_warp(session.battle, dt);
    update_starfield(session.battle, dt);

    if (session.battle.phase == BattlePhase::Shop) {
        return;
    }
    if (session.battle.inventory_open) {
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
    const bool shoot_now = keyboard[SDL_SCANCODE_SPACE] != 0;
    session.input.shoot_pressed = shoot_now && !session.input.shoot;
    session.input.shoot = shoot_now;

    if (session.battle.can_shoot && session.battle.player_active) {
        fire_player_weapons(session.battle, assets, session.input.shoot_pressed,
                            session.input.shoot);
    }
    const float substep_dt = dt / static_cast<float>(SIM_SUBSTEPS);
    for (int step = 0; step < SIM_SUBSTEPS; ++step) {
        update_player_ship(session.battle, session.input, substep_dt);
        update_player_respawn(session.battle, substep_dt);

        if (session.battle.phase == BattlePhase::LevelIntro) {
            if (update_enemy_intro(session.battle, substep_dt)) {
                Mix_PlayChannel(-1, assets.warp_stop, 0);
            }
        } else {
            update_enemy_wave(session.battle, substep_dt);
        }

        update_player_bullets(session.battle, assets, substep_dt);
        update_enemy_bullets(session.battle, substep_dt);
        update_particles(session.battle, substep_dt);
        update_pickups(session.battle, assets, substep_dt);
        resolve_damage(session.battle, assets);
    }
    if (session.battle.phase == BattlePhase::Active) {
        if (session.battle.wave_has_timer) {
            session.battle.wave_timer = std::max(0.0f, session.battle.wave_timer - dt);
        }

        const bool wave_done = current_wave_cleared(session.battle);
        const bool timer_expired =
            session.battle.wave_has_timer && session.battle.wave_timer <= 0.0f;
        if (wave_done || timer_expired) {
            if (wave_done && session.battle.wave_has_timer) {
                session.battle.gold += static_cast<int>(std::ceil(session.battle.wave_timer)) * 2;
                session.battle.gold_gain_flash = 1.0f;
            }
            if (!spawn_next_wave(session.battle)) {
                enter_shop(session.battle);
            }
        }
    }

    if (session.battle.lives <= 0 && !session.battle.player_active &&
        session.battle.particles.empty()) {
        switch_to_title(session, assets);
    }
}
