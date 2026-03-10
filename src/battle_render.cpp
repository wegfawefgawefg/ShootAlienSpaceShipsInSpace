#include "battle_render.hpp"

#include "level_data.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>

namespace {

constexpr float LEVEL_TEXT_DURATION = 1.3f;
constexpr float PANEL_PADDING = 12.0f;

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
    return {
        GAME_WIDTH * 0.5f + (world_pos.x - camera.center.x) * camera.zoom,
        GAME_HEIGHT * 0.5f + (world_pos.y - camera.center.y) * camera.zoom,
    };
}

float camera_scale(float radius, float zoom) {
    return std::max(1.0f, radius * zoom);
}

SDL_FRect fit_rect_within(const SDL_FRect& outer, float aspect_ratio) {
    SDL_FRect rect = outer;
    if (outer.w <= 0.0f || outer.h <= 0.0f) {
        return rect;
    }

    const float outer_aspect = outer.w / outer.h;
    if (outer_aspect > aspect_ratio) {
        rect.w = outer.h * aspect_ratio;
        rect.x = outer.x + (outer.w - rect.w) * 0.5f;
    } else {
        rect.h = outer.w / aspect_ratio;
        rect.y = outer.y + (outer.h - rect.h) * 0.5f;
    }
    return rect;
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
    const SDL_FRect dst = {x, y, static_cast<float>(surface->w), static_cast<float>(surface->h)};
    SDL_FreeSurface(surface);
    if (texture) {
        SDL_RenderCopyF(renderer, texture, nullptr, &dst);
        SDL_DestroyTexture(texture);
    }
}

void draw_world_sprite(SDL_Renderer* renderer, const TextureAtlas& atlas, int tile_index,
                       const CameraState& camera, Vec2 world_pos, float angle_deg = 0.0f,
                       Vec2 jitter = {0.0f, 0.0f}, float scale_x = 1.0f, float scale_y = 1.0f) {
    const SDL_Rect src = atlas_tile_rect(atlas, tile_index);
    const Vec2 screen_pos = world_to_screen(camera, world_pos + jitter);
    const float draw_width = static_cast<float>(atlas.tile_width) * camera.zoom * scale_x;
    const float draw_height = static_cast<float>(atlas.tile_height) * camera.zoom * scale_y;
    const SDL_FRect dst = {
        screen_pos.x - draw_width * 0.5f,
        screen_pos.y - draw_height * 0.5f,
        draw_width,
        draw_height,
    };
    SDL_RenderCopyExF(renderer, atlas.texture, &src, &dst, angle_deg, nullptr, SDL_FLIP_NONE);
}

void draw_world_shadow(SDL_Renderer* renderer, const TextureAtlas& atlas, int tile_index,
                       const CameraState& camera, Vec2 world_pos, float height,
                       float angle_deg = 0.0f) {
    const SDL_Rect src = atlas_tile_rect(atlas, tile_index);
    const Vec2 screen_pos = world_to_screen(camera, world_pos);
    const float offset_x = 0.6f + height * camera.zoom * 0.16f;
    const float offset_y = 0.8f + height * camera.zoom * 0.24f;
    const float draw_width = static_cast<float>(atlas.tile_width) * camera.zoom * 0.95f;
    const float draw_height = static_cast<float>(atlas.tile_height) * camera.zoom * 0.72f;
    const SDL_FRect dst = {
        screen_pos.x - draw_width * 0.5f + offset_x,
        screen_pos.y - draw_height * 0.5f + offset_y,
        draw_width,
        draw_height,
    };

    SDL_SetTextureColorMod(atlas.texture, 0, 0, 0);
    SDL_SetTextureAlphaMod(atlas.texture,
                           static_cast<Uint8>(clampf(170.0f - height * 10.0f, 70.0f, 150.0f)));
    SDL_RenderCopyExF(renderer, atlas.texture, &src, &dst, angle_deg, nullptr, SDL_FLIP_NONE);
    SDL_SetTextureColorMod(atlas.texture, 255, 255, 255);
    SDL_SetTextureAlphaMod(atlas.texture, 255);
}

void draw_ui_sprite(SDL_Renderer* renderer, const TextureAtlas& atlas, int tile_index,
                    float center_x, float center_y, float scale = 1.0f) {
    const SDL_Rect src = atlas_tile_rect(atlas, tile_index);
    const SDL_FRect dst = {
        center_x - static_cast<float>(atlas.tile_width) * 0.5f * scale,
        center_y - static_cast<float>(atlas.tile_height) * 0.5f * scale,
        static_cast<float>(atlas.tile_width) * scale,
        static_cast<float>(atlas.tile_height) * scale,
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

void render_title_scene(const Assets& assets, SDL_Renderer* renderer, Uint64 ticks) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    const float t = static_cast<float>(ticks);
    const float virtual_width = GAME_WIDTH * 2.0f;
    const float virtual_height = GAME_HEIGHT * 2.0f;
    const float offset = virtual_height / 10.0f;
    const float background_x = 0.3f * std::sin(t * 0.0013f) * offset - offset * 1.5f;
    const float background_y = 0.3f * std::sin(t * 0.003f) * offset - offset * 1.5f;
    const float menu_scale = 0.5f;

    const SDL_FRect background_dst = {
        background_x * menu_scale,
        background_y * menu_scale,
        virtual_width * 1.2f * menu_scale,
        virtual_height * 1.2f * menu_scale,
    };
    SDL_RenderCopyF(renderer, assets.menu_background, nullptr, &background_dst);

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

void render_battle_scene(const SessionState& session, const Assets& assets,
                         SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    for (const Star& star : session.battle.stars) {
        draw_world_sprite(renderer, assets.particles, 28, session.battle.camera, star.pos);
    }

    for (const Enemy& enemy : session.battle.enemies) {
        draw_world_shadow(renderer, assets.ships, enemy_small_ship_tile(enemy.type_id),
                          session.battle.camera, enemy.pos, enemy.height, enemy.angle_deg);
    }
    for (const PlayerBullet& bullet : session.battle.player_bullets) {
        draw_world_shadow(renderer, assets.particles, bullet.tile, session.battle.camera,
                          bullet.pos, bullet.height, angle_from_vector(bullet.vel));
    }
    for (const EnemyBullet& bullet : session.battle.enemy_bullets) {
        draw_world_shadow(renderer, assets.particles, bullet.tile, session.battle.camera,
                          bullet.pos, bullet.height, angle_from_vector(bullet.vel));
    }
    for (const Particle& particle : session.battle.particles) {
        draw_world_shadow(renderer, assets.particles, particle.tile, session.battle.camera,
                          particle.pos, particle.height);
    }
    if (session.battle.player_active) {
        int ship_tile = player_ship_tile_center(0);
        if (session.battle.ship.vel.x <= -10.0f) {
            ship_tile = 0;
        } else if (session.battle.ship.vel.x >= 10.0f) {
            ship_tile = 2;
        }
        draw_world_shadow(renderer, assets.ships, ship_tile, session.battle.camera,
                          session.battle.ship.pos, session.battle.ship.height);
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
}

void render_level_text(const SessionState& session, const Assets& assets, SDL_Renderer* renderer,
                       const ScreenLayout& layout) {
    if (session.scene != SceneMode::Battle || session.battle.phase != BattlePhase::LevelIntro ||
        !assets.ui_font_large) {
        return;
    }

    const float t = session.battle.level_text_timer;
    if (t > LEVEL_TEXT_DURATION) {
        return;
    }

    const std::string label = "Level " + std::to_string(session.battle.current_level_index + 1);
    SDL_Color color{255, 255, 255, 255};
    SDL_Surface* surface = TTF_RenderUTF8_Blended(assets.ui_font_large, label.c_str(), color);
    if (!surface) {
        return;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    const float width = static_cast<float>(surface->w);
    const float height = static_cast<float>(surface->h);
    SDL_FreeSurface(surface);
    if (!texture) {
        return;
    }

    const float fly = ease_out_cubic(clampf(t / LEVEL_TEXT_DURATION, 0.0f, 1.0f));
    const float wobble = std::sin(t * 8.0f) * 8.0f;
    const SDL_FRect dst = {
        layout.scene_rect.x + layout.scene_rect.w * 0.5f - width * 0.5f +
            std::sin(t * 5.0f) * 10.0f,
        layout.scene_rect.y + 16.0f + wobble + (1.0f - fly) * 28.0f,
        width,
        height,
    };
    SDL_RenderCopyF(renderer, texture, nullptr, &dst);
    SDL_DestroyTexture(texture);
}

void render_battle_overlay(const SessionState& session, const Assets& assets,
                           SDL_Renderer* renderer, const ScreenLayout& layout) {
    SDL_SetRenderDrawColor(renderer, 6, 12, 22, 230);
    SDL_RenderFillRectF(renderer, &layout.left_panel);
    SDL_RenderFillRectF(renderer, &layout.right_panel);

    for (int i = 0; i < session.battle.lives; ++i) {
        draw_ui_sprite(renderer, assets.ships, player_ship_tile_center(0),
                       layout.left_panel.x + layout.left_panel.w * 0.5f,
                       layout.left_panel.y + 28.0f + 44.0f * static_cast<float>(i), 3.0f);
    }

    SDL_Color text = {230, 236, 244, 255};
    float x = layout.right_panel.x + 10.0f;
    float y = layout.right_panel.y + 10.0f;
    draw_text(renderer, assets.ui_font_large,
              "L" + std::to_string(session.battle.current_level_index + 1), x, y, text);
    y += 34.0f;
    draw_text(renderer, assets.ui_font_small, "party: 1", x, y, text);
    y += 18.0f;
    draw_text(renderer, assets.ui_font_small, "wingmen: 0", x, y, text);
    y += 24.0f;
    draw_text(renderer, assets.ui_font_small, "weapons", x, y, text);
    y += 18.0f;

    for (std::size_t i = 0; i < session.battle.weapons.size(); ++i) {
        const Weapon& weapon = session.battle.weapons[i];
        draw_ui_sprite(renderer, assets.particles, weapon.projectile_tile, x + 8.0f, y + 8.0f,
                       2.0f);
        const std::string type = weapon.type == WeaponType::Basic ? "basic" : "missile";
        std::string fixture = "center";
        if (weapon.fixture == WeaponFixture::EvenlySpread) {
            fixture = "even";
        } else if (weapon.fixture == WeaponFixture::Splayed) {
            fixture = "splay";
        }
        draw_text(renderer, assets.ui_font_small, std::to_string(static_cast<int>(i)) + " " + type,
                  x + 22.0f, y - 2.0f, text);
        draw_text(renderer, assets.ui_font_small,
                  fixture + " p" + std::to_string(weapon.projectile_tile), x + 22.0f, y + 10.0f,
                  {170, 180, 200, 255});
        y += 28.0f;
    }

    render_level_text(session, assets, renderer, layout);
}

} // namespace

ScreenLayout make_screen_layout(int window_width, int window_height) {
    const float width = static_cast<float>(window_width);
    const float height = static_cast<float>(window_height);
    const float integer_scale =
        std::max(1.0f, std::floor(std::min(width / static_cast<float>(GAME_WIDTH),
                                           height / static_cast<float>(GAME_HEIGHT))));
    const float scene_width = static_cast<float>(GAME_WIDTH) * integer_scale;
    const float scene_height = static_cast<float>(GAME_HEIGHT) * integer_scale;
    const float scene_x = std::floor((width - scene_width) * 0.5f);
    const float scene_y = std::floor((height - scene_height) * 0.5f);

    ScreenLayout layout{};
    layout.scene_rect = {scene_x, scene_y, scene_width, scene_height};

    const SDL_FRect left_outer = {0.0f, 0.0f, std::max(0.0f, scene_x - PANEL_PADDING), height};
    const SDL_FRect right_outer = {
        scene_x + scene_width + PANEL_PADDING,
        0.0f,
        std::max(0.0f, width - (scene_x + scene_width + PANEL_PADDING)),
        height,
    };
    layout.left_panel = fit_rect_within({left_outer.x + PANEL_PADDING, left_outer.y + PANEL_PADDING,
                                         std::max(0.0f, left_outer.w - PANEL_PADDING * 2.0f),
                                         std::max(0.0f, left_outer.h - PANEL_PADDING * 2.0f)},
                                        0.55f);
    layout.right_panel =
        fit_rect_within({right_outer.x + PANEL_PADDING, right_outer.y + PANEL_PADDING,
                         std::max(0.0f, right_outer.w - PANEL_PADDING * 2.0f),
                         std::max(0.0f, right_outer.h - PANEL_PADDING * 2.0f)},
                        0.62f);
    return layout;
}

void session_render_scene(const SessionState& session, const Assets& assets, SDL_Renderer* renderer,
                          Uint64 ticks) {
    if (session.scene == SceneMode::Title) {
        render_title_scene(assets, renderer, ticks);
        return;
    }
    render_battle_scene(session, assets, renderer);
}

void session_render_overlay(const SessionState& session, const Assets& assets,
                            SDL_Renderer* renderer, const ScreenLayout& layout, Uint64 ticks) {
    (void)ticks;
    if (session.scene != SceneMode::Battle) {
        return;
    }
    render_battle_overlay(session, assets, renderer, layout);
}
