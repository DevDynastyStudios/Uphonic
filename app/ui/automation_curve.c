void uph_ui_automation_curve(
    const Uph_Automation *automation,
    float origin_x,
    float pixels_per_beat,
    float top_y,
    float height,
    Naui_Color line_color,
    Naui_Color fill_color,
    float line_width
) {
    const uint32_t point_count = (uint32_t)naui_list_len(automation->points);
    if (point_count < 2)
        return;

    const Naui_Vec4 clip = naui_current_clip_rect();
    const float baseline_y = top_y + height;

    for (uint32_t i = 0; i + 1 < point_count; i++)
    {
        const Uph_AutomationPoint *from = &automation->points[i];
        const Uph_AutomationPoint *to = &automation->points[i + 1];
        const Naui_Vec2 a = { origin_x + (float)from->beat * pixels_per_beat, top_y + (1.0f - from->value) * height };
        const Naui_Vec2 b = { origin_x + (float)to->beat * pixels_per_beat, top_y + (1.0f - to->value) * height };

        if (b.x < clip.x || a.x > clip.x + clip.z)
            continue;

        if (from->curve == UPH_AUTOMATION_CURVE_STEP)
        {
            naui_fill_rect(a, (Naui_Vec2) { b.x - a.x, baseline_y - a.y }, fill_color, 0.0f, NAUI_CORNER_NONE);
            naui_draw_line(a, (Naui_Vec2) { b.x, a.y }, line_color, line_width);
            naui_draw_line((Naui_Vec2) { b.x, a.y }, b, line_color, line_width);
            continue;
        }

        if (from->tension == 0.0f)
        {
            const Naui_Vec2 fill_poly[4] = {
                a,
                b,
                (Naui_Vec2) { b.x, baseline_y },
                (Naui_Vec2) { a.x, baseline_y }
            };
            naui_fill_polygon(fill_poly, 4, fill_color);
            naui_draw_line(a, b, line_color, line_width);
            continue;
        }

        float controls[4];
        uph_automation_segment_controls(from, controls);
        const Naui_Vec2 c1 = { a.x + controls[0] * (b.x - a.x), a.y + controls[1] * (b.y - a.y) };
        const Naui_Vec2 c2 = { a.x + controls[2] * (b.x - a.x), a.y + controls[3] * (b.y - a.y) };

        naui_fill_bezier_area(a, c1, c2, b, baseline_y, fill_color);
        naui_draw_bezier(a, c1, c2, b, line_color, line_width);
    }
}