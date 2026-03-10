#include "battle_render.hpp"

#include "level_data.hpp"

#include <cmath>
#include <cstdlib>
#include <string>

namespace {

constexpr float UI_LEFT_WIDTH = 24.0f;
constexpr float UI_RIGHT_WIDTH = 64.0f;
constexpr float PLAYFIELD_LEFT = UI_LEFT_WIDTH;
constexpr float PLAYFIELD_WIDTH = GAME_WIDTH - UI_LEFT_WIDTH - UI_RIGHT_WIDTH;
constexpr float PLAYFIELD_HEIGHT = GAME_HEIGHT;
constexpr float LEVEL_TEXT_DURATION = 1.3f;

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

float angle_from_vector(Vec2 dir) {
    if (vec2_length_sq(dir) < 0.0001f) {
        return 0.0f;
    }
    return std::atan2(dir.y, dir.x) * 57.2957795f + 90.0f;
}

Vec2 world_to_screen(const CameraState& camera, Vec2 world_pos) {
    return {PLAYFIELD_LEFT + PLAYFIELD_WIDTH * 0.5f + (world_pos.x - camera.center.x) * camera.zoom,
            PLAYFIELD_HEIGHT * 0.5f + (world_pos.y - camera.center.y) * camera.zoom};
}

float camera_scale(float radius, float zoom) {
    return std::max(1.0f, radius * zoom);
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

void render_level_text(const SessionState& session, const Assets& assets, SDL_Renderer* renderer) {
    if (session.scene != SceneMode::Battle || session.battle.phase != BattlePhase::LevelIntro ||
        !assets.ui_font) {
        return;
    }

    const float t = session.battle.level_text_timer;
    if (t > LEVEL_TEXT_DURATION) {
        return;
    }

    const std::string label = "Level " + std::to_string(session.battle.current_level_index + 1);
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

void render_ui(const SessionState& session, const Assets& assets, SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 8, 12, 20, 220);
    const SDL_FRect left = {0.0f, 0.0f, UI_LEFT_WIDTH, GAME_HEIGHT};
    const SDL_FRect right = {GAME_WIDTH - UI_RIGHT_WIDTH, 0.0f, UI_RIGHT_WIDTH, GAME_HEIGHT};
    SDL_RenderFillRectF(renderer, &left);
    SDL_RenderFillRectF(renderer, &right);

    for (int i = 0; i < session.battle.lives; ++i) {
        draw_ui_sprite(renderer, assets.ships, player_ship_tile_center(0), 12.0f,
                       14.0f + 12.0f * static_cast<float>(i));
    }

    SDL_Color text = {255, 255, 255, 255};
    draw_text(renderer, assets.ui_font,
              "L" + std::to_string(session.battle.current_level_index + 1),
              GAME_WIDTH - UI_RIGHT_WIDTH + 4.0f, 4.0f, text);
    draw_text(renderer, assets.ui_font, "Party 1", GAME_WIDTH - UI_RIGHT_WIDTH + 4.0f, 18.0f, text);
    draw_text(renderer, assets.ui_font, "Wing 0", GAME_WIDTH - UI_RIGHT_WIDTH + 4.0f, 32.0f, text);

    float y = 48.0f;
    for (std::size_t i = 0; i < session.battle.weapons.size(); ++i) {
        const Weapon& weapon = session.battle.weapons[i];
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

void render_battle(const SessionState& session, const Assets& assets, SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    for (const Star& star : session.battle.stars) {
        draw_world_sprite(renderer, assets.particles, 28, session.battle.camera, star.pos);
    }

    for (const Enemy& enemy : session.battle.enemies) {
        const Vec2 jitter = {random_range(-enemy.shake, enemy.shake),
                             random_range(-enemy.shake, enemy.shake)};
        draw_world_sprite(renderer, assets.ships, enemy_small_ship_tile(enemy.type_id),
                          session.battle.camera, enemy.pos, enemy.angle_deg, jitter);
    }

    for (const PlayerBullet& bullet : session.battle.player_bullets) {
        draw_world_sprite(renderer, assets.particles, bullet.tile, session.battle.camera,
                          bullet.pos, angle_from_vector(bullet.vel));
    }

    for (const EnemyBullet& bullet : session.battle.enemy_bullets) {
        draw_world_sprite(renderer, assets.particles, bullet.tile, session.battle.camera,
                          bullet.pos, angle_from_vector(bullet.vel));
    }

    for (const Particle& particle : session.battle.particles) {
        draw_world_sprite(renderer, assets.particles, particle.tile, session.battle.camera,
                          particle.pos);
    }

    const bool hidden_for_blink = session.battle.invuln_timer > 0.0f &&
                                  std::fmod(session.battle.invuln_timer * 20.0f, 2.0f) < 1.0f;
    if (session.battle.player_active && !hidden_for_blink) {
        int ship_tile = player_ship_tile_center(0);
        if (session.battle.ship.vel.x <= -10.0f) {
            ship_tile = 0;
        } else if (session.battle.ship.vel.x >= 10.0f) {
            ship_tile = 2;
        }
        const Vec2 jitter = {random_range(-session.battle.ship.shake, session.battle.ship.shake),
                             random_range(-session.battle.ship.shake, session.battle.ship.shake)};
        draw_world_sprite(renderer, assets.ships, ship_tile, session.battle.camera,
                          session.battle.ship.pos, 0.0f, jitter);
    }

    if (session.battle.debug_colliders) {
        for (const Enemy& enemy : session.battle.enemies) {
            draw_debug_box(renderer, session.battle.camera, enemy.pos, enemy.radius,
                           {255, 64, 64, 255});
        }
        for (const PlayerBullet& bullet : session.battle.player_bullets) {
            draw_debug_box(renderer, session.battle.camera, bullet.pos, bullet.radius,
                           {255, 255, 64, 255});
        }
        for (const EnemyBullet& bullet : session.battle.enemy_bullets) {
            draw_debug_box(renderer, session.battle.camera, bullet.pos, bullet.radius,
                           {64, 255, 255, 255});
        }
        if (session.battle.player_active) {
            draw_debug_box(renderer, session.battle.camera, session.battle.ship.pos,
                           session.battle.ship.radius, {64, 255, 64, 255});
        }
    }

    render_level_text(session, assets, renderer);
    render_ui(session, assets, renderer);
}

} // namespace

void session_render(const SessionState& session, const Assets& assets, SDL_Renderer* renderer,
                    Uint64 ticks) {
    if (session.scene == SceneMode::Title) {
        render_title(assets, renderer, ticks);
        return;
    }
    render_battle(session, assets, renderer);
}
