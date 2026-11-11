static Naui_List(Uph_Particle) uph_particles;

void uph_particle_emit(const Uph_Particle *particle)
{
	if (particle->duration <= 0.0f)
		return;

	naui_list_push(uph_particles, *particle);
}

void uph_particle_emit_shockwave(Naui_Vec2 position, Naui_Vec2 size, Naui_Color color, float rounding, Leaf_BoundingBox clip)
{
	const float expand = NAUI_DPI(14.0f);
	uph_particle_emit(&(Uph_Particle){
		.position = position,
		.size = size,
		.growth = { expand * 2.0f, expand * 2.0f },
		.color = color,
		.clip = clip,
		.duration = 1.0f / NAUI_UI_ANIM_SPEED,
		.line_width = NAUI_DPI(2.0f),
		.rounding = rounding,
		.rounding_growth = expand,
		.shape = UPH_PARTICLE_RECT_OUTLINE
	});
}

void uph_particles_clear(void)
{
	naui_list_clear(uph_particles);
}

void uph_particles_shutdown(void)
{
	naui_list_free(uph_particles);
	uph_particles = NULL;
}

static void _uph_particles_draw(Leaf_BoundingBox bbox, void *user_data)
{
	(void)bbox;
	(void)user_data;

	for (uint32_t i = 0; i < (uint32_t)naui_list_len(uph_particles); i++)
	{
		const Uph_Particle *particle = &uph_particles[i];
		const float t = NAUI_CLAMP01(particle->age / particle->duration);
		const float grow = naui_ease_out_cubic(t);
		const float fade = (1.0f - t) * (1.0f - t);

		const Naui_Vec2 size = {
			fmaxf(particle->size.x + particle->growth.x * grow, 0.0f),
			fmaxf(particle->size.y + particle->growth.y * grow, 0.0f)
		};
		const Naui_Vec2 position = {
			particle->position.x + particle->velocity.x * particle->age - particle->growth.x * grow * 0.5f,
			particle->position.y + particle->velocity.y * particle->age - particle->growth.y * grow * 0.5f
		};

		Naui_Color color = particle->color;
		color.a = (uint8_t)((float)color.a * fade);

		float rounding = fmaxf(particle->rounding + particle->rounding_growth * grow, 0.0f);
		if (particle->shape == UPH_PARTICLE_CIRCLE || particle->shape == UPH_PARTICLE_CIRCLE_OUTLINE)
			rounding = fminf(size.x, size.y) * 0.5f;

		const bool clipped = particle->clip.width > 0.0f && particle->clip.height > 0.0f;
		if (clipped)
			naui_push_clip_rect(particle->clip.x, particle->clip.y, particle->clip.width, particle->clip.height);

		if (particle->shape == UPH_PARTICLE_RECT_OUTLINE || particle->shape == UPH_PARTICLE_CIRCLE_OUTLINE)
			naui_draw_rect(position, size, color, particle->line_width * (1.0f - t * 0.5f), rounding, NAUI_CORNER_ALL, NAUI_SIDE_ALL);
		else
			naui_fill_rect(position, size, color, rounding, NAUI_CORNER_ALL);

		if (clipped)
			naui_pop_clip_rect();
	}
}

void uph_particles_render(void)
{
	const float dt = naui_delta_time();
	for (uint32_t i = (uint32_t)naui_list_len(uph_particles); i > 0; i--)
	{
		Uph_Particle *particle = &uph_particles[i - 1];
		particle->age += dt;
		if (particle->age >= particle->duration)
			naui_list_remove(uph_particles, i - 1);
	}

	if (naui_list_len(uph_particles) == 0)
		return;

	leaf({
		.positioning = LEAF_POSITIONING_FLOATING_TO_ROOT,
		.size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL},
		.custom_draw = _uph_particles_draw
	});
}
