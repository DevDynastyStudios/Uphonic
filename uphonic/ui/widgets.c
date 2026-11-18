void uph_ui_widgets_init(void)
{
    Uph_GlobalWidgetState *data = &uph_global_widget_state;
    naui_arena_init(&data->menu_arena, (1 << 14));
}

void uph_ui_widgets_flush(void)
{
    Uph_GlobalWidgetState *data = &uph_global_widget_state;
    uph__ui_render_menu_dropdown(data);
    if (!data->current_open_menu)
        data->open_menu_key = 0;
    data->current_open_menu = NULL;

    if (data->textfield_data.mode != UPH_UI_EDIT_MODE_NONE && !data->textfield_submitted_this_frame)
        uph__ui_end_edit(&data->textfield_data);
    data->textfield_submitted_this_frame = false;

    naui_ime_set_mode(data->ime_mode);
    data->ime_mode = NAUI_IME_MODE_OFF;

    data->id_counter = 0;
    data->any_widget_hovered = false;

    naui_arena_reset(&data->menu_arena);
}

int32_t uph_ui_track_color_palette(const Uph_Track *track)
{
    const Naui_List(Naui_Color) palette = uph_state.colors.track_palette;
    const uint32_t color_count = (uint32_t)naui_list_len(palette);
    if (color_count == 0)
        return -1;

    const uint32_t rows = color_count > 6 ? 2 : 1;
    const uint32_t columns = (color_count + rows - 1) / rows;
    const int32_t current = track->color_index % (int32_t)color_count;

    const int32_t swatch_size = NAUI_DPI(20);
    const float ring_width = NAUI_MAX(1.0f, NAUI_DPI(2));
    const int32_t ring_gap = NAUI_DPI(1);
    const float cell_size = swatch_size + (ring_width + ring_gap) * 2.0f;
    const Leaf_Color ring_color = uph_state.colors.ui_text;
    int32_t picked = -1;

    leaf({
        .padding = LEAF_PADDING_ALL(NAUI_DPI(2)),
        .child_gap = NAUI_DPI(8),
        .direction = LEAF_DIRECTION_VERTICAL
    })
    {
        leaf_text(NAUI_TR("song_timeline.track.color"), {
            .color = uph_state.colors.panel_text,
            .font_size = LEAF_SIZE_FIXED(NAUI_DPI(UPH_UI_FONT_SIZE) * 0.95f)
        });

        leaf({
            .child_gap = NAUI_DPI(5),
            .direction = LEAF_DIRECTION_VERTICAL
        })
        {
            for (uint32_t row = 0; row < rows; row++)
            {
                leaf({
                    .child_gap = NAUI_DPI(5),
                    .direction = LEAF_DIRECTION_HORIZONTAL
                })
                {
                    for (uint32_t column = 0; column < columns; column++)
                    {
                        const uint32_t index = row * columns + column;
                        if (index >= color_count)
                            break;

                        const Leaf_ID id = leaf_id_indexed("uph_track_color_picker", index);
                        const bool hovered = leaf_hovered(id);
                        const bool selected = (int32_t)index == current;

                        if (hovered)
                        {
                            naui_set_cursor(NAUI_CURSOR_HAND);
                            if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
                            {
                                picked = (int32_t)index;
                                uph__ui_close_menu(&uph_global_widget_state);
                            }
                        }

                        Leaf_Color ring = LEAF_COLOR_TRANSPARENT;
                        if (selected)
                            ring = ring_color;
                        else if (hovered)
                            ring = palette[index];

                        leaf({
                            .id = id,
                            .size = { .width = LEAF_SIZE_FIXED(cell_size), .height = LEAF_SIZE_FIXED(cell_size) },
                            .padding = LEAF_PADDING_ALL((int32_t)(ring_width + ring_gap)),
                            .border = {
                                .width = (int32_t)ring_width,
                                .sides = LEAF_SIDE_ALL,
                                .color = ring
                            },
                            .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(4), LEAF_CORNER_ALL)
                        })
                        {
                            leaf({
                                .size = { .width = LEAF_SIZE_FULL, .height = LEAF_SIZE_FULL },
                                .color = palette[index],
                                .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(2), LEAF_CORNER_ALL)
                            });
                        }
                    }
                }
            }
        }
    }

    return picked;
}

#define UPH_UI_SPINNER_SEGMENTS 24
void uph_ui_spinner_custom_draw(Leaf_BoundingBox box, void *user_data)
{
    const Naui_Color *color = user_data;
    const float radius = fminf(box.width, box.height) * 0.3f;
    if (radius <= 0.0f)
        return;

    const float sweep = NAUI_PI * 1.5f;
    const float start = naui_time() * NAUI_PI * 2.0f;
    const Naui_Vec2 center = { box.x + box.width * 0.5f, box.y + box.height * 0.5f };

    Naui_Vec2 points[UPH_UI_SPINNER_SEGMENTS + 1];
    for (int32_t i = 0; i <= UPH_UI_SPINNER_SEGMENTS; i++)
    {
        const float angle = start + sweep * (float)i / (float)UPH_UI_SPINNER_SEGMENTS;
        points[i] = (Naui_Vec2){ center.x + cosf(angle) * radius, center.y + sinf(angle) * radius };
    }

    naui_draw_polyline(points, UPH_UI_SPINNER_SEGMENTS + 1, *color, NAUI_DPI(2));
}

void uph_ui_tinted_spinner_full(void)
{
    const float frame_height = uph__ui_frame_height();
    Naui_Color spinner_color = uph_state.colors.ui_text;
    leaf({
        .size = { .width = LEAF_SIZE_FULL, .height = UPH_UI_SIZE_EXACT(frame_height) },
        .border = uph__ui_border(uph_state.colors.ui_tinted_frame_border),
        .color = uph_state.colors.ui_tinted_frame_bg,
        .child_alignment = { .x = LEAF_ALIGN_X_CENTER, .y = LEAF_ALIGN_Y_CENTER },
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(UPH_UI_FRAME_ROUNDING), NAUI_CORNER_ALL)
    }) leaf({
        .custom_draw = uph_ui_spinner_custom_draw,
        .custom_draw_data = LEAF_DATA_SLICE(spinner_color),
        .size = { .width = UPH_UI_SIZE_EXACT(frame_height), .height = UPH_UI_SIZE_EXACT(frame_height) }
    });
}
