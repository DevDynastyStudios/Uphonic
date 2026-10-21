#include <naui/build.c>

#include <stdatomic.h>
#include <float.h>

#include <vendor/stb/stb_vorbis.h>
#define MINIAUDIO_IMPLEMENTATION
#include <vendor/miniaudio/miniaudio.h>

#define CMIDI_IMPLEMENTATION
#include <vendor/cmidi/cmidi.h>

#include "core/types.h"
#include "actions/action_manager.h"

#include "utils.h"
#include "utils.c"

#include <vendor/vst3_c_api.h>
#include <vendor/clap/clap.h>

#include "plugins/event_ring.h"
#include "plugins/plugin_manager.h"

#include "core/language.h"
#include "core/language.c"
#include "core/audio_engine.h"
#include "core/resource_manager.h"
#include "core/midi_manager.h"
#include "core/midi_manager.c"
#include "core/midi_recorder.h"
#include "core/midi_preview.h"
#include "io/file_signature.h"
#include "io/file_signature.c"
#include "io/serialization.h"
#include "io/serialization.c"
#include "core/autosave.h"
#include "core/recovery.h"
#include "core/project_manager.h"
#include "core/project_manager.c"
#include "core/autosave.c"
#include "core/recovery.c"
#include "core/metronome.h"

#include "plugins/plugin_manager.c"

#include "plugins/plugin_cache.h"
#include "plugins/plugin_cache.c"

#include "ui/titlebar.h"
#include "ui/waveform.h"
#include "ui/automation_curve.h"
#include "ui/widgets.h"
#include "ui/dialog.h"
#include "ui/containers.h"

#include "ui/titlebar.c"
#include "ui/waveform.c"
#include "ui/automation_curve.c"
#include "ui/widgets.c"
#include "ui/dialog.c"
#include "ui/containers.c"

#include "actions/action_track.c"
#include "actions/action_block.c"
#include "actions/action_pattern.c"
#include "actions/action_sample.c"
#include "actions/action_automation.c"
#include "actions/action_manager.c"

#include "core/resource_manager.c"

#include "core/midi_preview.c"
#include "core/audio_engine.c"
#include "core/midi_recorder.c"

#include "panels/file_dialog.c"

#include "panels/song_timeline/song_timeline.h"
#include "panels/song_timeline/song_timeline_automation.c"
#include "panels/song_timeline/song_timeline_track_list.c"
#include "panels/song_timeline/song_timeline_track.c"
#include "panels/song_timeline/song_timeline_block.c"
#include "panels/song_timeline/song_timeline_block_render.c"
#include "panels/song_timeline/song_timeline_toolbox.c"
#include "panels/song_timeline/song_timeline_ruler.c"
#include "panels/song_timeline/song_timeline_input.c"
#include "panels/song_timeline.c"

#include "panels/mixer.c"
#include "panels/resource_list.c"
#include "panels/plugin_list.c"
#include "panels/automation_list.c"

#include "panels/resource_editor/midi/note_map.h"
#include "panels/resource_editor/midi/note_map.c"
#include "panels/resource_editor/midi/piano_keys.h"
#include "panels/resource_editor/midi/piano_keys.c"
#include "panels/resource_editor/midi/midi_view.h"
#include "panels/resource_editor/midi/midi_editor.h"
#include "panels/resource_editor/midi/midi_grid.h"
#include "panels/resource_editor/midi/midi_grid.c"
#include "panels/resource_editor/midi/midi_notes.h"
#include "panels/resource_editor/midi/midi_notes.c"
#include "panels/resource_editor/midi/midi_note_input.h"
#include "panels/resource_editor/midi/midi_note_input.c"
#include "panels/resource_editor/midi/midi_velocity.h"
#include "panels/resource_editor/midi/midi_velocity.c"
#include "panels/resource_editor/midi/midi_toolbar.h"
#include "panels/resource_editor/midi/midi_toolbar.c"
#include "panels/resource_editor/midi/midi_editor.c"

#include "panels/resource_editor/sample/sample_editor.h"
#include "panels/resource_editor/sample/sample_editor.c"

#include "panels/resource_editor/automation/automation_editor.h"
#include "panels/resource_editor/automation/automation_editor.c"

#include "panels/resource_editor.c"
#include "panels/settings.c"

#include "main.c"