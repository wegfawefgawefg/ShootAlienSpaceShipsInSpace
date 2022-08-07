import pygame
import pymunk
from pymunk import Vec2d as pvec2

from entity import Entity


class Debris(Entity):
    def __init__(self, scene) -> None:
        super().__init__(scene)

        mass = 10
        radius = 3
        inertia = pymunk.moment_for_circle(mass, 0, radius, (0, 0))
        body = pymunk.Body(mass, inertia)
        center = self.scene.game.graphics.screen_dims // 2
        body.position = center.x, center.y
        self.shape = pymunk.Circle(body, radius, pvec2(0, 0))

        self.scene.physics.add(body, self.shape)

    def get_pos(self):
        return self.shape.body.position

    def draw(self):
        p = self.get_pos()
        pygame.draw.circle(
            self.scene.game.graphics.primary_surface,
            (255, 0, 0), (p.x, p.y),
            self.shape.radius, 1
        )
        self.scene.game.graphics.draw_sprite(
            self.scene.game.sprite_sheets.ships,
            4, p.x, p.y)

    def remove(self):
        self.scene.physics.remove(self.shape, self.shape.body)
        self.scene.physics.remove(self.shape)

    def step(self):
        pass