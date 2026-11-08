#pragma once

typedef uint8_t Uph_ParticleShape;
enum
{
	UPH_PARTICLE_RECT,
	UPH_PARTICLE_RECT_OUTLINE,
	UPH_PARTICLE_CIRCLE,
	UPH_PARTICLE_CIRCLE_OUTLINE
};

typedef struct
{
	Naui_Vec2 position;
	Naui_Vec2 size;
	Naui_Vec2 velocity;
	Naui_Vec2 growth;
	Naui_Color color;
	Leaf_BoundingBox clip;
	float duration;
	float age;
	float line_width;
	float rounding;
	float rounding_growth;
	Uph_ParticleShape shape;
}
Uph_Particle;

void uph_particle_emit(const Uph_Particle *particle);
void uph_particle_emit_shockwave(Naui_Vec2 position, Naui_Vec2 size, Naui_Color color, float rounding, Leaf_BoundingBox clip);
void uph_particles_clear(void);
void uph_particles_render(void);
void uph_particles_shutdown(void);
