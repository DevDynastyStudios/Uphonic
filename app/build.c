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

#include "core/audio_engine.h"
#include "core/resource_manager.h"
#include "core/resource_manager.c"
#include "core/midi_manager.h"
#include "core/midi_manager.c"
#include "io/serialization.h"
#include "io/serialization.c"
#include "core/project_manager.h"
#include "core/project_manager.c"

#include "plugins/plugin_manager.c"

#include "plugins/plugin_cache.h"
#include "plugins/plugin_cache.c"

#include "ui/titlebar.h"
#include "ui/waveform.h"
#include "ui/widgets.h"
#include "ui/containers.h"

#include "ui/titlebar.c"
#include "ui/waveform.c"
#include "ui/widgets.c"
#include "ui/containers.c"

#include "actions/action_track.c"
#include "actions/action_block.c"
#include "actions/action_pattern.c"
#include "actions/action_sample.c"
#include "actions/action_automation.c"
#include "actions/action_manager.c"

#include "core/audio_engine.c"

#include "panels/song_timeline.c"
#include "panels/mixer.c"
#include "panels/resource_list.c"
#include "panels/effect_list.c"
#include "panels/plugin_list.c"
#include "panels/resource_editor.c"
#include "panels/settings.c"

#include "main.c"
