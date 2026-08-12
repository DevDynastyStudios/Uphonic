#include <naui/build.c>

#define STB_VORBIS_IMPLEMENTATION
#include <vendor/stb/stb_vorbis.h>

#define MINIAUDIO_IMPLEMENTATION
#include <vendor/miniaudio/miniaudio.h>

#include "defaults/titlebar.h"
#include "defaults/widgets/widgets.h"

#include "defaults/titlebar.c"
#include "defaults/widgets/widgets.c"

#include "core/types.h"
#include "core/project_manager.h"
#include "core/project_manager.c"

#include "panels/build.c"
#include "main.c"
