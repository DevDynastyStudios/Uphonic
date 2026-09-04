#include <naui/build.c>

#include <vendor/stb/stb_vorbis.h>
#define MINIAUDIO_IMPLEMENTATION
#include <vendor/miniaudio/miniaudio.h>

#include "core/types.h"

#include "io/serialization.h"
#include "io/serialization.c"

#include <vendor/clap/clap.h>

#include "plugins/plugin_manager.h"
#include "plugins/plugin_manager_win32.c"
#include "plugins/plugin_manager_linux.c"

#include "core/audio_engine.h"
#include "core/audio_engine.c"
#include "core/resource_manager.h"
#include "core/resource_manager.c"
#include "core/project_manager.h"
#include "core/project_manager.c"

#include "ui/titlebar.h"
#include "ui/waveform.h"
#include "ui/widgets.h"
#include "ui/list_box.h"

#include "ui/titlebar.c"
#include "ui/waveform.c"
#include "ui/widgets.c"
#include "ui/list_box.c"

#include "panels/song_timeline.c"
#include "panels/mixer.c"
#include "panels/pattern_list.c"
#include "panels/sample_list.c"
#include "panels/midi_editor.c"
#include "main.c"
