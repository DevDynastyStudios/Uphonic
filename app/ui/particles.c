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
	float duration;
	float start_time;
	float line_width;
	float rounding;
	float rounding_growth;
	Uph_ParticleShape shape;
}
Uph_Particle;

typedef struct
{
	Naui_List(Uph_Particle) particles;
}
Uph_ParticleSystem;

void uph_particle_emit(Uph_ParticleSystem *system, const Uph_Particle *particle)
{
	if (particle->duration <= 0.0f)
		return;

	Uph_Particle emitted = *particle;
	emitted.start_time = naui_time();
	naui_list_push(system->particles, emitted);
}

void uph_particle_emit_shockwave(Uph_ParticleSystem *system, Naui_Vec2 position, Naui_Vec2 size, Naui_Color color, float rounding)
{
	const int32_t expand = NAUI_DPI(14);
	uph_particle_emit(system, &(Uph_Particle){
		.position = position,
		.size = size,
		.growth = { expand * 2.0f, expand * 2.0f },
		.color = color,
		.duration = 1.0f / NAUI_UI_ANIM_SPEED,
		.line_width = NAUI_DPI(2),
		.rounding = rounding,
		.rounding_growth = expand,
		.shape = UPH_PARTICLE_RECT_OUTLINE
	});
}

void uph_particles_free(Uph_ParticleSystem *system)
{
	naui_list_free(system->particles);
	system->particles = NULL;
}

static void uph__particles_draw(Leaf_BoundingBox bbox, Uph_ParticleSystem **user_data)
{
	const Uph_ParticleSystem *system = *user_data;
	const float time = naui_time();
	(void)bbox;

	for (uint32_t i = 0; i < (uint32_t)naui_list_len(system->particles); i++)
	{
		const Uph_Particle *particle = &system->particles[i];
		const float age = time - particle->start_time;
		const float t = NAUI_CLAMP01(age / particle->duration);
		const float grow = naui_ease_out_cubic(t);
		const float fade = (1.0f - t) * (1.0f - t);

		const Naui_Vec2 size = {
			fmaxf(particle->size.x + particle->growth.x * grow, 0.0f),
			fmaxf(particle->size.y + particle->growth.y * grow, 0.0f)
		};
		const Naui_Vec2 position = {
			particle->position.x + particle->velocity.x * age - particle->growth.x * grow * 0.5f,
			particle->position.y + particle->velocity.y * age - particle->growth.y * grow * 0.5f
		};

		Naui_Color color = particle->color;
		color.a = (uint8_t)((float)color.a * fade);

		float rounding = fmaxf(particle->rounding + particle->rounding_growth * grow, 0.0f);
		if (particle->shape == UPH_PARTICLE_CIRCLE || particle->shape == UPH_PARTICLE_CIRCLE_OUTLINE)
			rounding = fminf(size.x, size.y) * 0.5f;

		if (particle->shape == UPH_PARTICLE_RECT_OUTLINE || particle->shape == UPH_PARTICLE_CIRCLE_OUTLINE)
			naui_draw_rect(position, size, color, particle->line_width * (1.0f - t * 0.5f), rounding, NAUI_CORNER_ALL, NAUI_SIDE_ALL);
		else
			naui_fill_rect(position, size, color, rounding, NAUI_CORNER_ALL);
	}
}

void uph_particles_render(Uph_ParticleSystem *system)
{
	const float time = naui_time();
	for (uint32_t i = (uint32_t)naui_list_len(system->particles); i > 0; i--)
	{
		const Uph_Particle *particle = &system->particles[i - 1];
		if (time - particle->start_time >= particle->duration)
			naui_list_remove(system->particles, i - 1);
	}

	if (naui_list_len(system->particles) == 0)
		return;

	leaf({
		.custom_draw = (Leaf_CustomDrawFn)uph__particles_draw,
		.custom_draw_data = LEAF_DATA_SLICE(system),
		.size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FULL },
		.positioning = LEAF_POSITIONING_FLOATING_TO_PARENT
	});
}
