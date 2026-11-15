#include <naui/build.c>

#include <stdatomic.h>
#include <float.h>

#include <vendor/stb/stb_vorbis.h>
#define MINIAUDIO_IMPLEMENTATION
#include <vendor/miniaudio/miniaudio.h>

#define CMIDI_IMPLEMENTATION
#include <vendor/cmidi/cmidi.h>

#include <vendor/vst3_c_api.h>
#include <vendor/clap/clap.h>
#include <vendor/vestige/vestige.h>

#include "types.h"

#include "core/midi_manager.h"
#include "core/project_manager.h"
#include "core/recovery.h"
#include "core/resource_manager.h"
#include "core/sample_preview.h"
#include "core/audio_engine.h"
#include "core/midi_recorder.h"

#include "actions/action_manager.h"

#include "plugins/plugin_manager.h"

#include "ui/widgets.h"
#include "ui/dialog.h"

#include "panels/song_timeline/song_timeline.h"
#include "panels/resource_list.h"
#include "panels/plugin_editor.h"
#include "panels/resource_editor/midi/midi_editor.h"
#include "panels/sample_browser.h"

#include "core/constants.h"
#include "core/paths.h"
#include "ui/metrics.h"
#include "actions/actions.h"

#include "core/keybinds.c"

#include "utils.c"

#include "plugins/event_ring.c"

#include "core/language.c"
#include "core/midi_manager.c"
#include "io/file_signature.c"
#include "core/asio.c"
#include "io/serialization.c"

#include "core/project/project_files.c"
#include "core/project/project_naming.c"
#include "core/project/project_samples.c"
#include "core/project/project_patterns.c"
#include "core/project/project_package.c"
#include "core/autosave.c"
#include "core/project_manager.c"

#include "core/recovery.c"
#include "core/metronome.c"

#include "core/tempo_map.c"
#include "plugins/plugin_manager.c"

#include "plugins/plugin_cache.c"

#include "ui/layouts.c"
#include "ui/titlebar.c"
#include "ui/waveform.c"
#include "ui/automation_curve.c"
#include "ui/widgets.c"
#include "ui/drag.c"
#include "ui/dialog.c"
#include "ui/particles.c"
#include "ui/containers.c"
#include "ui/level_meter.c"

#include "actions/action_track.c"
#include "actions/action_block.c"
#include "actions/action_pattern.c"
#include "actions/action_sample.c"
#include "actions/action_automation.c"
#include "actions/action_manager.c"

#include "core/resource_manager.c"

#include "core/midi_preview.c"
#include "core/audio_engine.c"
#include "core/sample_preview.c"
#include "core/midi_recorder.c"

#include "panels/file_dialog.c"

#include "panels/song_timeline/song_timeline_common.c"
#include "panels/song_timeline/song_timeline_automation.c"
#include "panels/song_timeline/song_timeline_track_list.c"
#include "panels/song_timeline/song_timeline_track.c"
#include "panels/song_timeline/song_timeline_block.c"
#include "panels/song_timeline/song_timeline_block_render.c"
#include "panels/song_timeline/song_timeline_toolbox.c"
#include "panels/song_timeline/song_timeline_ruler.c"
#include "panels/song_timeline/song_timeline_input.c"
#include "panels/song_timeline/song_timeline_minimap.c"
#include "panels/song_timeline.c"

#include "panels/mixer.c"
#include "panels/resource_list.c"
#include "panels/plugin_list.c"
#include "panels/plugin_editor.c"
#include "panels/automation_list.c"

#include "panels/resource_editor/midi/note_map.c"
#include "panels/resource_editor/midi/piano_keys.c"
#include "panels/resource_editor/midi/midi_view.c"
#include "panels/resource_editor/midi/midi_grid.c"
#include "panels/resource_editor/midi/midi_notes.c"
#include "panels/resource_editor/midi/midi_note_input.c"
#include "panels/resource_editor/midi/midi_velocity.c"
#include "panels/resource_editor/midi/midi_pedal.c"
#include "panels/resource_editor/midi/midi_toolbar.c"
#include "panels/resource_editor/midi/midi_menu_bar.c"
#include "panels/resource_editor/midi/midi_editor.c"

#include "panels/resource_editor/sample/sample_editor.c"

#include "panels/resource_editor/automation/automation_editor.c"

#include "panels/resource_editor.c"
#include "panels/settings.c"
#include "panels/render.c"
#include "panels/sample_browser.c"

#include "main.c"