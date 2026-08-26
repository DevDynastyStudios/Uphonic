NAUI_APP("Uphonic")

Uph_State uph_state = { 0 };

void naui_app_start(void)
{
    uph_state.settings.audio = (Uph_AudioSettings){
        .sample_rate = 48000
    };
    uph_state.project.bpm = 120.0f;

	naui_load_theme("Default");
	naui_load_font(0, "MYRIADPRO-REGULAR");
    uph_audio_engine_init();
    naui_set_main_viewport(naui_dock_panel(
        naui_dock_panel(
            NAUI_ATTACH_PANEL(uph_song_timeline),
            naui_dock_panel(
                NAUI_ATTACH_PANEL(uph_pattern_list),
                NAUI_ATTACH_PANEL(uph_sample_list),
                NAUI_DOCK_DIRECTION_CENTER, 0.0f
            ),
            NAUI_DOCK_DIRECTION_RIGHT, 0.8f
        ),
        naui_dock_panel(
            NAUI_ATTACH_PANEL(uph_mixer),
            NAUI_ATTACH_PANEL(uph_midi_editor),
            NAUI_DOCK_DIRECTION_LEFT, 0.7f
        ),
        NAUI_DOCK_DIRECTION_BOTTOM, 0.6f
    ));

}

void naui_app_end(void)
{
    uph_audio_engine_shutdown();
}

void naui_app_update(void)
{
	uph_render_main_titlebar();

    const Leaf_Color bg_color = naui_theme_color("naui_panel_title_bg_color");
    const Leaf_Color tool_icon_color = naui_theme_color("uph_tool_icon_color");
    const Leaf_Color play_icon_color = naui_theme_color("uph_play_icon_color");
    const Leaf_Color pause_icon_color = naui_theme_color("uph_pause_icon_color");
    const Leaf_Color stop_icon_color = naui_theme_color("uph_stop_icon_color");

    leaf({
        .direction = LEAF_DIRECTION_HORIZONTAL,
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(NAUI_DPI(15.0f))},
        .padding = LEAF_PADDING_AXES(NAUI_DPI(12.0f), NAUI_DPI(8.0f)),
        .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
        .color = bg_color,
        .child_gap = NAUI_DPI(10.0f)
    })
    {
        leaf({
            .direction = LEAF_DIRECTION_HORIZONTAL,
            .size = {LEAF_SIZE_GROW, LEAF_SIZE_FULL},
            .child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
            .child_gap = NAUI_DPI(4.0f)
        })
        {
            /*naui_image_button(
                naui_asset_image("uph_icon_select"),
                naui_vec2(15.0f, 15.0f),
                tool_icon_color
            );

            naui_image_button(
                naui_asset_image("uph_icon_draw"),
                naui_vec2(15.0f, 15.0f),
                tool_icon_color
            );

            naui_image_button(
                naui_asset_image("uph_icon_cut"),
                naui_vec2(15.0f, 15.0f),
                tool_icon_color
            );*/
        }
        leaf({
            .direction = LEAF_DIRECTION_HORIZONTAL,
            .size = {LEAF_SIZE_GROW, LEAF_SIZE_FULL},
            .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
            .child_gap = NAUI_DPI(4.0f)
        })
        {
            /*if (naui_image_button(
                naui_asset_image(uph_state.shared.song_timeline_playing ? "uph_icon_pause" : "uph_icon_play"),
                naui_vec2(15.0f, 15.0f),
                uph_state.shared.song_timeline_playing ?  pause_icon_color : play_icon_color
            ))
            {
                uph_state.shared.song_timeline_playing = !uph_state.shared.song_timeline_playing;
            }

            if (naui_image_button(
                naui_asset_image("uph_icon_stop"),
                naui_vec2(15.0f, 15.0f),
                stop_icon_color
            ))
            {
                uph_state.shared.song_timeline_playing = false;
                uph_state.shared.song_timeline_playhead_position = 0.0;
            }*/
        }


        leaf({
            .direction = LEAF_DIRECTION_HORIZONTAL,
            .size = {LEAF_SIZE_GROW, LEAF_SIZE_FULL},
            .child_alignment = {LEAF_ALIGN_X_RIGHT, LEAF_ALIGN_Y_CENTER},
            .child_gap = NAUI_DPI(10.0f)
        })
        {
            // metronome
            // TODO(doomguy): replace text with metronome icon
            {
                static bool metronome_enabled;
                static float metronome_last_time;

                if (metronome_enabled) {
                    if (uph_ui_text_button("bpm button", leaf_id("uph_bpm_button"))) {
                        const float time = naui_time();
                        uph_state.project.bpm = 60.0f / (time - metronome_last_time);
                        metronome_last_time = time;
                    }
                }

                uph_ui_text_toggle_button("metronome enable", leaf_id("uph_metronome_toggle"), &metronome_enabled);
            }

            const Naui_String bpm = naui_string_format("BPM: %.1f", uph_state.project.bpm);

            leaf_text(bpm.data, {
                .font_size = NAUI_DPI(13.0f),
                .color = tool_icon_color
            });

            /*leaf_text("Zoom: 10.5%", {
                .font_size = NAUI_DPI(13.0f),
                .color = tool_icon_color
            });*/
        }
    }
	naui_render_panels_and_viewport();
    uph_ui_widgets_flush();
}

void naui_app_event(const Naui_AppEventData *data)
{
    if (data->type == NAUI_APP_EVENT_FILE_DROP)
    {
        uph_resources_add_sample_from_file(NAUI_PATH(data->file_drop.paths[0]));
    }
}