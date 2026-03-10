#include "battle_render.hpp"

#include "level_data.hpp"
#include "pickup_defs.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>

namespace {

constexpr float LABEL_DURATION = 1.2f;
constexpr float PANEL_GAP = 14.0f;
constexpr float LEFT_PANEL_WIDTH = 94.0f;
constexpr float RIGHT_PANEL_WIDTH = 220.0f;
constexpr float INVENTORY_GAP = 24.0f;

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

const char* behavior_name(EnemyBehavior behavior) {
    switch (behavior) {
    case EnemyBehavior::Straight:
        return "straight";
    case EnemyBehavior::SlowChase:
        return "slow chase";
    case EnemyBehavior::TopShooter:
        return "top shooter";
    case EnemyBehavior::Rammer:
        return "rammer";
    case EnemyBehavior::Circler:
        return "circler";
    case EnemyBehavior::Wander:
        return "wander";
    case EnemyBehavior::BossSpray:
        return "boss spray";
    case EnemyBehavior::BossSpokes:
        return "boss spokes";
    case EnemyBehavior::BossCharge:
        return "boss charge";
    }
    return "unknown";
}

const char* weapon_type_name(WeaponType type) {
    switch (type) {
    case WeaponType::Basic:
        return "basic";
    case WeaponType::Missile:
        return "missile";
    }
    return "?";
}

const char* fixture_name(WeaponFixture fixture) {
    switch (fixture) {
    case WeaponFixture::Center:
        return "center";
    case WeaponFixture::EvenlySpread:
        return "even";
    case WeaponFixture::Splayed:
        return "splay";
    }
    return "?";
}

const char* boss_group_name(int group_index) {
    switch (group_index) {
    case 0:
        return "phase 1";
    case 1:
        return "phase 2";
    case 2:
        return "phase 3";
    default:
        return "phase ?";
    }
}

Vec2 world_to_screen(const CameraState& camera, Vec2 world_pos) {
    return {
        GAME_WIDTH * 0.5f + (world_pos.x - camera.center.x) * camera.zoom + camera.shake_offset.x,
        GAME_HEIGHT * 0.5f + (world_pos.y - camera.center.y) * camera.zoom + camera.shake_offset.y};
}

float camera_scale(float radius, float zoom) {
    return std::max(1.0f, radius * zoom);
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
                       Vec2 jitter = {0.0f, 0.0f}, float scale = 1.0f) {
    const SDL_Rect src = atlas_tile_rect(atlas, tile_index);
    const Vec2 offset = atlas_tile_offset(atlas, tile_index);
    const Vec2 screen_pos = world_to_screen(camera, world_pos + offset + jitter);
    const float draw_width = static_cast<float>(atlas.tile_width) * camera.zoom * scale;
    const float draw_height = static_cast<float>(atlas.tile_height) * camera.zoom * scale;
    const SDL_FRect dst = {screen_pos.x - draw_width * 0.5f, screen_pos.y - draw_height * 0.5f,
                           draw_width, draw_height};
    SDL_RenderCopyExF(renderer, atlas.texture, &src, &dst, angle_deg, nullptr, SDL_FLIP_NONE);
}

void draw_world_shadow(SDL_Renderer* renderer, const TextureAtlas& atlas, int tile_index,
                       const CameraState& camera, Vec2 world_pos, float height, float angle_deg,
                       float scale) {
    const SDL_Rect src = atlas_tile_rect(atlas, tile_index);
    const Vec2 offset = atlas_tile_offset(atlas, tile_index);
    const Vec2 screen_pos = world_to_screen(camera, world_pos + offset);
    const float offset_x = 0.9f + height * camera.zoom * 0.14f;
    const float offset_y = 1.2f + height * camera.zoom * 0.24f;
    const float draw_width = static_cast<float>(atlas.tile_width) * camera.zoom * scale;
    const float draw_height = static_cast<float>(atlas.tile_height) * camera.zoom * scale * 0.68f;
    const SDL_FRect dst = {screen_pos.x - draw_width * 0.5f + offset_x,
                           screen_pos.y - draw_height * 0.5f + offset_y, draw_width, draw_height};

    SDL_SetTextureColorMod(atlas.texture, 36, 46, 60);
    SDL_SetTextureAlphaMod(atlas.texture,
                           static_cast<Uint8>(clampf(128.0f - height * 8.0f, 52.0f, 108.0f)));
    SDL_RenderCopyExF(renderer, atlas.texture, &src, &dst, angle_deg, nullptr, SDL_FLIP_NONE);
    SDL_SetTextureColorMod(atlas.texture, 255, 255, 255);
    SDL_SetTextureAlphaMod(atlas.texture, 255);
}

void draw_ui_sprite(SDL_Renderer* renderer, const TextureAtlas& atlas, int tile_index,
                    float center_x, float center_y, float scale) {
    const SDL_Rect src = atlas_tile_rect(atlas, tile_index);
    const Vec2 offset = atlas_tile_offset(atlas, tile_index);
    const SDL_FRect dst = {
        center_x + offset.x * scale - static_cast<float>(atlas.tile_width) * 0.5f * scale,
        center_y + offset.y * scale - static_cast<float>(atlas.tile_height) * 0.5f * scale,
        static_cast<float>(atlas.tile_width) * scale,
        static_cast<float>(atlas.tile_height) * scale};
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

float enemy_scale(const Enemy& enemy) {
    return enemy.is_boss ? std::max(3.0f, enemy.radius / 3.0f)
                         : std::max(1.45f, enemy.radius / 2.6f);
}

void render_title_scene(const Assets& assets, SDL_Renderer* renderer, Uint64 ticks) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    const float t = static_cast<float>(ticks);
    const float offset = static_cast<float>(GAME_HEIGHT) / 8.0f;
    const float background_x = 0.35f * std::sin(t * 0.0012f) * offset - offset;
    const float background_y = 0.35f * std::sin(t * 0.0027f) * offset - offset;
    const SDL_FRect background_dst = {background_x, background_y, GAME_WIDTH * 1.08f,
                                      GAME_HEIGHT * 1.08f};
    SDL_RenderCopyF(renderer, assets.menu_background, nullptr, &background_dst);

    const float title_y = 8.0f + (0.5f + std::sin(t * 0.001f) * 0.5f) * 10.0f;
    const SDL_FRect shadow_dst = {-20.0f, title_y + 6.0f, GAME_WIDTH * 1.04f, GAME_HEIGHT * 0.92f};
    const SDL_FRect title_dst = {-12.0f, title_y, GAME_WIDTH, GAME_HEIGHT * 0.9f};

    SDL_SetTextureColorMod(assets.menu_title, 0, 0, 0);
    SDL_SetTextureAlphaMod(assets.menu_title, 210);
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
                          session.battle.camera, enemy.pos, enemy.height, enemy.angle_deg,
                          enemy_scale(enemy));
    }
    for (const PlayerBullet& bullet : session.battle.player_bullets) {
        draw_world_shadow(renderer, assets.particles, bullet.tile, session.battle.camera,
                          bullet.pos, bullet.height, angle_from_vector(bullet.vel), 1.0f);
    }
    for (const EnemyBullet& bullet : session.battle.enemy_bullets) {
        draw_world_shadow(renderer, assets.particles, bullet.tile, session.battle.camera,
                          bullet.pos, bullet.height, angle_from_vector(bullet.vel), 1.0f);
    }
    for (const Particle& particle : session.battle.particles) {
        draw_world_shadow(renderer, assets.particles, particle.tile, session.battle.camera,
                          particle.pos, particle.height, 0.0f, 1.0f);
    }
    for (const PickupActor& pickup : session.battle.pickups) {
        const PickupDef& def = pickup_def(pickup.def_index);
        draw_world_shadow(renderer, assets.particles, def.icon_tile, session.battle.camera,
                          pickup.pos, pickup.height, 0.0f, 1.2f);
    }
    for (const GoldActor& gold : session.battle.gold_pickups) {
        draw_world_shadow(renderer, assets.coins, gold.tile, session.battle.camera, gold.pos,
                          gold.height, 0.0f, 1.0f);
    }

    if (session.battle.player_active) {
        int ship_tile = player_ship_tile_center(0);
        if (session.battle.ship.vel.x <= -10.0f) {
            ship_tile = 2;
        } else if (session.battle.ship.vel.x >= 10.0f) {
            ship_tile = 0;
        }
        draw_world_shadow(renderer, assets.ships, ship_tile, session.battle.camera,
                          session.battle.ship.pos, session.battle.ship.height, 0.0f, 1.7f);
    }

    for (const Enemy& enemy : session.battle.enemies) {
        const Vec2 jitter = {random_range(-enemy.shake, enemy.shake),
                             random_range(-enemy.shake, enemy.shake)};
        draw_world_sprite(renderer, assets.ships, enemy_small_ship_tile(enemy.type_id),
                          session.battle.camera, enemy.pos, enemy.angle_deg, jitter,
                          enemy_scale(enemy));
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
    for (const PickupActor& pickup : session.battle.pickups) {
        const PickupDef& def = pickup_def(pickup.def_index);
        const float pulse = 1.15f + std::sin(pickup.age * 8.0f) * 0.08f;
        draw_world_sprite(renderer, assets.particles, def.icon_tile, session.battle.camera,
                          pickup.pos, 0.0f, {0.0f, 0.0f}, pulse);
    }
    for (const GoldActor& gold : session.battle.gold_pickups) {
        const float pulse = 1.0f + std::sin(gold.age * 10.0f) * 0.06f;
        draw_world_sprite(renderer, assets.coins, gold.tile, session.battle.camera, gold.pos, 0.0f,
                          {0.0f, 0.0f}, pulse);
    }

    const bool hidden_for_blink = session.battle.invuln_timer > 0.0f &&
                                  std::fmod(session.battle.invuln_timer * 20.0f, 2.0f) < 1.0f;
    if (session.battle.player_active && !hidden_for_blink) {
        int ship_tile = player_ship_tile_center(0);
        if (session.battle.ship.vel.x <= -10.0f) {
            ship_tile = 2;
        } else if (session.battle.ship.vel.x >= 10.0f) {
            ship_tile = 0;
        }
        const Vec2 jitter = {random_range(-session.battle.ship.shake, session.battle.ship.shake),
                             random_range(-session.battle.ship.shake, session.battle.ship.shake)};
        draw_world_sprite(renderer, assets.ships, ship_tile, session.battle.camera,
                          session.battle.ship.pos, 0.0f, jitter, 1.7f);
    }

    if (session.battle.debug_colliders) {
        for (const Enemy& enemy : session.battle.enemies) {
            draw_debug_box(renderer, session.battle.camera, enemy.pos, enemy.radius,
                           {255, 64, 64, 180});
        }
        for (const PlayerBullet& bullet : session.battle.player_bullets) {
            draw_debug_box(renderer, session.battle.camera, bullet.pos, bullet.radius,
                           {255, 255, 64, 180});
        }
        for (const EnemyBullet& bullet : session.battle.enemy_bullets) {
            draw_debug_box(renderer, session.battle.camera, bullet.pos, bullet.radius,
                           {64, 255, 255, 180});
        }
        for (const PickupActor& pickup : session.battle.pickups) {
            draw_debug_box(renderer, session.battle.camera, pickup.pos, pickup.radius,
                           {255, 128, 255, 180});
        }
        for (const GoldActor& gold : session.battle.gold_pickups) {
            draw_debug_box(renderer, session.battle.camera, gold.pos, gold.radius,
                           {255, 220, 80, 180});
        }
        if (session.battle.player_active) {
            draw_debug_box(renderer, session.battle.camera, session.battle.ship.pos,
                           session.battle.ship.radius, {64, 255, 64, 180});
        }
    }
}

void render_wave_label(const SessionState& session, const Assets& assets, SDL_Renderer* renderer,
                       const ScreenLayout& layout) {
    if (!assets.ui_font_large || session.scene != SceneMode::Battle) {
        return;
    }

    const float t = session.battle.level_text_timer;
    if (t > LABEL_DURATION) {
        return;
    }

    const std::string label = "L" + std::to_string(session.battle.current_level_index + 1) + " W" +
                              std::to_string(session.battle.current_wave_index + 1);
    SDL_Color color{240, 242, 246, 255};
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

    const float fly = ease_out_cubic(clampf(t / LABEL_DURATION, 0.0f, 1.0f));
    const SDL_FRect dst = {layout.scene_rect.x + layout.scene_rect.w * 0.5f - width * 0.5f,
                           layout.scene_rect.y + 18.0f + std::sin(t * 8.0f) * 6.0f +
                               (1.0f - fly) * 20.0f,
                           width, height};
    SDL_RenderCopyF(renderer, texture, nullptr, &dst);
    SDL_DestroyTexture(texture);
}

const Enemy* active_boss(const BattleState& battle) {
    for (const Enemy& enemy : battle.enemies) {
        if (enemy.is_boss) {
            return &enemy;
        }
    }
    return nullptr;
}

struct InventoryEntry {
    std::string section{};
    std::string name{};
    std::string subtitle{};
    std::string detail{};
    std::string desc{};
    int icon_tile{0};
    int weapon_index{-1};
    int pickup_def_index{-1};
};

struct InventoryRow {
    bool selectable{false};
    std::string header{};
    InventoryEntry entry{};
};

bool is_weapon_mod_pickup(PickupEffectType effect) {
    return effect == PickupEffectType::UpgradeRandomRate ||
           effect == PickupEffectType::UpgradeRandomDamage ||
           effect == PickupEffectType::UpgradeRandomAuto ||
           effect == PickupEffectType::UpgradeRandomSpread;
}

bool is_ship_pickup(PickupEffectType effect) {
    return effect == PickupEffectType::ExtraLife || effect == PickupEffectType::PickupMagnet ||
           effect == PickupEffectType::GlassReactor || effect == PickupEffectType::StabilityFins;
}

std::vector<std::pair<int, int>> counted_pickups(const BattleState& battle,
                                                 bool (*predicate)(PickupEffectType)) {
    std::vector<std::pair<int, int>> counted;
    for (int def_index : battle.collected_pickups) {
        const PickupDef& def = pickup_def(def_index);
        if (!predicate(def.effect)) {
            continue;
        }

        auto it = std::find_if(counted.begin(), counted.end(),
                               [&](const auto& item) { return item.first == def_index; });
        if (it == counted.end()) {
            counted.push_back({def_index, 1});
        } else {
            it->second += 1;
        }
    }
    return counted;
}

std::vector<InventoryRow> build_inventory_rows(const BattleState& battle) {
    std::vector<InventoryRow> rows;
    rows.push_back({false, "Weapons", {}});
    for (std::size_t i = 0; i < battle.weapons.size(); ++i) {
        const Weapon& weapon = battle.weapons[i];
        InventoryEntry entry{};
        entry.section = "Weapons";
        entry.name = std::to_string(static_cast<int>(i + 1)) + ". " + weapon.name;
        entry.subtitle =
            std::string(fixture_name(weapon.fixture)) + (weapon.automatic ? " auto" : " semi");
        entry.detail = "dmg " + std::to_string(weapon.damage).substr(0, 4) + " cd " +
                       std::to_string(weapon.cooldown).substr(0, 4) + " cnt " +
                       std::to_string(weapon.projectile_count);
        entry.desc = "spd " + std::to_string(static_cast<int>(weapon.projectile_speed)) + " life " +
                     std::to_string(weapon.projectile_life).substr(0, 4) + " rad " +
                     std::to_string(weapon.projectile_radius).substr(0, 4);
        entry.icon_tile = weapon.projectile_tile;
        entry.weapon_index = static_cast<int>(i);
        rows.push_back({true, {}, entry});
    }

    rows.push_back({false, "Weapon Upgrades", {}});
    for (const auto& [def_index, count] : counted_pickups(battle, is_weapon_mod_pickup)) {
        const PickupDef& def = pickup_def(def_index);
        InventoryEntry entry{};
        entry.section = "Weapon Upgrades";
        entry.name = def.name + (count > 1 ? " x" + std::to_string(count) : "");
        entry.subtitle = pickup_tier_name(def.tier);
        entry.detail = def.desc;
        entry.desc = "Affects existing guns.";
        entry.icon_tile = def.icon_tile;
        entry.pickup_def_index = def_index;
        rows.push_back({true, {}, entry});
    }

    rows.push_back({false, "Ship Augments", {}});
    for (const auto& [def_index, count] : counted_pickups(battle, is_ship_pickup)) {
        const PickupDef& def = pickup_def(def_index);
        InventoryEntry entry{};
        entry.section = "Ship Augments";
        entry.name = def.name + (count > 1 ? " x" + std::to_string(count) : "");
        entry.subtitle = pickup_tier_name(def.tier);
        entry.detail = def.desc;
        entry.desc = "Run-wide ship effect.";
        entry.icon_tile = def.icon_tile;
        entry.pickup_def_index = def_index;
        rows.push_back({true, {}, entry});
    }

    rows.push_back({false, "Wingmen", {}});
    InventoryEntry wingmen{};
    wingmen.section = "Wingmen";
    wingmen.name = "No wingmen yet";
    wingmen.subtitle = "planned";
    wingmen.detail = "Wingmen will live here once added.";
    wingmen.desc = "They will get their own weapons and support stats.";
    wingmen.icon_tile = 31;
    rows.push_back({true, {}, wingmen});
    return rows;
}

void render_inventory_overlay(const SessionState& session, const Assets& assets,
                              SDL_Renderer* renderer, const ScreenLayout& layout) {
    const std::vector<InventoryRow> rows = build_inventory_rows(session.battle);
    int selectable_count = 0;
    for (const InventoryRow& row : rows) {
        if (row.selectable) {
            selectable_count += 1;
        }
    }
    const int clamped_selection =
        selectable_count > 0
            ? static_cast<int>(clampf(static_cast<float>(session.battle.inventory_selection), 0.0f,
                                      static_cast<float>(selectable_count - 1)))
            : 0;

    int selected_row = -1;
    int running_selectable = 0;
    for (std::size_t i = 0; i < rows.size(); ++i) {
        if (!rows[i].selectable) {
            continue;
        }
        if (running_selectable == clamped_selection) {
            selected_row = static_cast<int>(i);
        }
        running_selectable += 1;
    }
    if (selected_row < 0) {
        for (std::size_t i = 0; i < rows.size(); ++i) {
            if (rows[i].selectable) {
                selected_row = static_cast<int>(i);
                break;
            }
        }
    }

    const SDL_FRect backdrop = {layout.scene_rect.x, layout.scene_rect.y, layout.scene_rect.w,
                                layout.scene_rect.h};
    SDL_SetRenderDrawColor(renderer, 4, 8, 14, 210);
    SDL_RenderFillRectF(renderer, &backdrop);

    const SDL_FRect list_panel = {layout.scene_rect.x + INVENTORY_GAP,
                                  layout.scene_rect.y + INVENTORY_GAP, 360.0f,
                                  layout.scene_rect.h - INVENTORY_GAP * 2.0f};
    const SDL_FRect detail_panel = {
        list_panel.x + list_panel.w + 20.0f, list_panel.y,
        layout.scene_rect.w - (list_panel.w + INVENTORY_GAP * 3.0f + 20.0f), list_panel.h};
    SDL_SetRenderDrawColor(renderer, 8, 16, 28, 244);
    SDL_RenderFillRectF(renderer, &list_panel);
    SDL_RenderFillRectF(renderer, &detail_panel);

    SDL_Color primary{232, 238, 246, 255};
    SDL_Color secondary{168, 182, 200, 255};
    SDL_Color accent{255, 214, 128, 255};
    draw_text(renderer, assets.ui_font_large, "Inventory", list_panel.x + 12.0f,
              list_panel.y + 8.0f, primary);
    draw_text(renderer, assets.ui_font_small, "Tab close  Up/Down select  PgUp/PgDn jump",
              detail_panel.x + 12.0f, detail_panel.y + 12.0f, secondary);

    const float row_h = 24.0f;
    const float list_y0 = list_panel.y + 44.0f;
    const int visible_rows = std::max(1, static_cast<int>((list_panel.h - 52.0f) / row_h));
    int first_row = 0;
    if (selected_row >= 0) {
        first_row = std::max(0, selected_row - visible_rows / 2);
        first_row = std::min(first_row, std::max(0, static_cast<int>(rows.size()) - visible_rows));
    }

    const InventoryEntry* selected = nullptr;
    int selectable_index = 0;
    for (int row_index = 0; row_index < first_row; ++row_index) {
        if (rows[static_cast<std::size_t>(row_index)].selectable) {
            selectable_index += 1;
        }
    }
    float y = list_y0;
    for (int row_index = first_row;
         row_index < static_cast<int>(rows.size()) && row_index < first_row + visible_rows;
         ++row_index) {
        const InventoryRow& row = rows[static_cast<std::size_t>(row_index)];
        if (!row.selectable) {
            draw_text(renderer, assets.ui_font_small, row.header, list_panel.x + 12.0f, y, accent);
            y += row_h;
            continue;
        }

        const bool is_selected = selectable_index == clamped_selection;
        if (is_selected) {
            const SDL_FRect highlight = {list_panel.x + 8.0f, y - 2.0f, list_panel.w - 16.0f,
                                         row_h};
            SDL_SetRenderDrawColor(renderer, 22, 42, 68, 255);
            SDL_RenderFillRectF(renderer, &highlight);
            selected = &row.entry;
        }
        draw_ui_sprite(renderer, assets.particles, row.entry.icon_tile, list_panel.x + 18.0f,
                       y + 8.0f, 1.4f);
        draw_text(renderer, assets.ui_font_small, row.entry.name, list_panel.x + 32.0f, y - 1.0f,
                  primary);
        draw_text(renderer, assets.ui_font_small, row.entry.subtitle, list_panel.x + 32.0f,
                  y + 11.0f, secondary);
        y += row_h;
        selectable_index += 1;
    }

    if (!selected) {
        for (const InventoryRow& row : rows) {
            if (row.selectable) {
                selected = &row.entry;
                break;
            }
        }
    }

    if (selected) {
        draw_text(renderer, assets.ui_font_large, selected->name, detail_panel.x + 12.0f,
                  detail_panel.y + 40.0f, primary);
        draw_text(renderer, assets.ui_font_small, selected->section + " / " + selected->subtitle,
                  detail_panel.x + 12.0f, detail_panel.y + 76.0f, accent);
        float detail_y = detail_panel.y + 108.0f;
        if (selected->weapon_index >= 0) {
            const Weapon& weapon =
                session.battle.weapons[static_cast<std::size_t>(selected->weapon_index)];
            const std::array<std::string, 8> lines = {
                "Type: " + std::string(weapon_type_name(weapon.type)),
                "Trigger: " + std::string(weapon.automatic ? "Automatic" : "Semi-auto"),
                "Fixture: " + std::string(fixture_name(weapon.fixture)),
                "Damage: " + std::to_string(weapon.damage).substr(0, 5),
                "Cooldown: " + std::to_string(weapon.cooldown).substr(0, 5),
                "Projectile Count: " + std::to_string(weapon.projectile_count),
                "Projectile Speed: " + std::to_string(static_cast<int>(weapon.projectile_speed)),
                "Projectile Life: " + std::to_string(weapon.projectile_life).substr(0, 5),
            };
            for (const std::string& line : lines) {
                draw_text(renderer, assets.ui_font_small, line, detail_panel.x + 12.0f, detail_y,
                          primary);
                detail_y += 18.0f;
            }
            draw_text(renderer, assets.ui_font_small,
                      "Projectile Radius: " + std::to_string(weapon.projectile_radius).substr(0, 5),
                      detail_panel.x + 12.0f, detail_y, primary);
            detail_y += 24.0f;
            draw_text(renderer, assets.ui_font_small, "Attached Upgrades", detail_panel.x + 12.0f,
                      detail_y, accent);
            detail_y += 18.0f;
            if (weapon.attached_pickups.empty()) {
                draw_text(renderer, assets.ui_font_small, "None", detail_panel.x + 12.0f, detail_y,
                          secondary);
            } else {
                for (int def_index : weapon.attached_pickups) {
                    const PickupDef& def = pickup_def(def_index);
                    draw_text(renderer, assets.ui_font_small, "- " + def.name,
                              detail_panel.x + 12.0f, detail_y, primary);
                    detail_y += 18.0f;
                }
            }
        } else {
            draw_text(renderer, assets.ui_font_small, selected->detail, detail_panel.x + 12.0f,
                      detail_y, primary);
            detail_y += 22.0f;
            draw_text(renderer, assets.ui_font_small, selected->desc, detail_panel.x + 12.0f,
                      detail_y, secondary);
        }
    }
}

void render_battle_overlay(const SessionState& session, const Assets& assets,
                           SDL_Renderer* renderer, const ScreenLayout& layout) {
    if (session.battle.inventory_open) {
        render_inventory_overlay(session, assets, renderer, layout);
        return;
    }

    SDL_SetRenderDrawColor(renderer, 7, 14, 24, 235);
    SDL_RenderFillRectF(renderer, &layout.left_panel);
    SDL_RenderFillRectF(renderer, &layout.right_panel);

    for (int i = 0; i < session.battle.lives; ++i) {
        draw_ui_sprite(renderer, assets.ships, player_ship_tile_center(0),
                       layout.left_panel.x + layout.left_panel.w * 0.5f,
                       layout.left_panel.y + 28.0f + 38.0f * static_cast<float>(i), 2.5f);
    }

    SDL_Color primary{232, 238, 246, 255};
    SDL_Color secondary{168, 182, 200, 255};
    SDL_Color accent{255, 214, 128, 255};
    float x = layout.right_panel.x + 10.0f;
    float y = layout.right_panel.y + 10.0f;
    draw_text(renderer, assets.ui_font_large,
              "L" + std::to_string(session.battle.current_level_index + 1), x, y, primary);
    y += 32.0f;
    const SDL_Color gold_color =
        session.battle.gold_gain_flash > 0.0f ? SDL_Color{255, 240, 128, 255} : accent;
    draw_ui_sprite(renderer, assets.coins, 1, x + 8.0f, y + 8.0f, 1.8f);
    draw_text(renderer, assets.ui_font_small, "gold " + std::to_string(session.battle.gold),
              x + 18.0f, y - 2.0f, gold_color);
    y += 20.0f;
    draw_text(renderer, assets.ui_font_small,
              "wave " + std::to_string(session.battle.current_wave_index + 1), x, y, primary);
    y += 16.0f;
    if (session.battle.wave_has_timer) {
        draw_text(renderer, assets.ui_font_small,
                  "timer " + std::to_string(static_cast<int>(std::ceil(session.battle.wave_timer))),
                  x, y, primary);
        y += 18.0f;
    } else {
        draw_text(renderer, assets.ui_font_small, "boss wave", x, y, {255, 182, 110, 255});
        y += 18.0f;
    }

    const Enemy* boss = active_boss(session.battle);
    if (boss) {
        draw_text(renderer, assets.ui_font_small,
                  std::string("boss ") + boss_group_name(boss->boss_group_index), x, y,
                  {255, 132, 132, 255});
        y += 14.0f;
        draw_text(renderer, assets.ui_font_small,
                  std::string("beh ") + behavior_name(boss->behavior), x, y, {255, 212, 168, 255});
        y += 18.0f;
    }

    draw_text(renderer, assets.ui_font_small, "party 1", x, y, secondary);
    y += 14.0f;
    draw_text(renderer, assets.ui_font_small, "wingmen 0", x, y, secondary);
    y += 22.0f;
    draw_text(renderer, assets.ui_font_small, "weapons", x, y, primary);
    y += 16.0f;

    for (std::size_t i = 0; i < session.battle.weapons.size(); ++i) {
        const Weapon& weapon = session.battle.weapons[i];
        draw_ui_sprite(renderer, assets.particles, weapon.projectile_tile, x + 8.0f, y + 8.0f,
                       1.8f);
        std::string fixture = "center";
        if (weapon.fixture == WeaponFixture::EvenlySpread) {
            fixture = "even";
        } else if (weapon.fixture == WeaponFixture::Splayed) {
            fixture = "splay";
        }
        draw_text(renderer, assets.ui_font_small,
                  std::to_string(static_cast<int>(i)) + " " + weapon.name, x + 18.0f, y - 2.0f,
                  primary);
        draw_text(renderer, assets.ui_font_small, fixture + (weapon.automatic ? " auto" : " semi"),
                  x + 18.0f, y + 10.0f, secondary);
        y += 26.0f;
    }

    y += 6.0f;
    draw_text(renderer, assets.ui_font_small,
              "magnet " + std::to_string(static_cast<int>(session.battle.pickup_magnet_radius)), x,
              y, secondary);
    y += 18.0f;
    draw_text(renderer, assets.ui_font_small, "pickup log", x, y, primary);
    y += 16.0f;

    for (auto it = session.battle.collected_pickups.rbegin();
         it != session.battle.collected_pickups.rend() &&
         y < layout.right_panel.y + layout.right_panel.h - 18.0f;
         ++it) {
        const PickupDef& def = pickup_def(*it);
        draw_ui_sprite(renderer, assets.particles, def.icon_tile, x + 8.0f, y + 8.0f, 1.6f);
        draw_text(renderer, assets.ui_font_small, def.name, x + 18.0f, y - 2.0f, primary);
        draw_text(renderer, assets.ui_font_small, pickup_tier_name(def.tier), x + 18.0f, y + 10.0f,
                  secondary);
        y += 24.0f;
    }

    render_wave_label(session, assets, renderer, layout);
}

} // namespace

ScreenLayout make_screen_layout(int window_width, int window_height) {
    const float width = static_cast<float>(window_width);
    const float height = static_cast<float>(window_height);
    const float scale =
        std::max(1.0f, std::floor(std::min(width / static_cast<float>(GAME_WIDTH),
                                           height / static_cast<float>(GAME_HEIGHT))));

    const float scene_width = static_cast<float>(GAME_WIDTH) * scale;
    const float scene_height = static_cast<float>(GAME_HEIGHT) * scale;
    const float scene_x = std::floor((width - scene_width) * 0.5f);
    const float scene_y = std::floor((height - scene_height) * 0.5f);

    ScreenLayout layout{};
    layout.scene_rect = {scene_x, scene_y, scene_width, scene_height};
    layout.left_panel = {PANEL_GAP, PANEL_GAP, LEFT_PANEL_WIDTH, 170.0f};
    layout.right_panel = {width - RIGHT_PANEL_WIDTH - PANEL_GAP, PANEL_GAP, RIGHT_PANEL_WIDTH,
                          290.0f};
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
