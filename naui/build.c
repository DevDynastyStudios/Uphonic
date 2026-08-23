// header files
#include "base.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>
#if NAUI_WINDOWS
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
		#ifndef UNICODE
			#define UNICODE
		#endif
	#ifndef _UNICODE
		#define _UNICODE
	#endif
	#include <windows.h>
	#include <shlobj.h>
	#include "vendor/dirent/dirent.h"
	#include <direct.h>
#else
	#ifndef _DEFAULT_SOURCE
		#define _DEFAULT_SOURCE
	#endif

	#ifdef __APPLE__
		#include <mach-o/dyld.h>
	#endif

	#include <unistd.h>
	#include <dirent.h>
	#include <fcntl.h>
	#include <sys/file.h>
#endif

#include "vendor/stb/stb.c"
#include "vendor/miniz/miniz.c"
#include "vendor/magma/magma.c"
#include "vendor/leaf/leaf.c"

#include "utils/list.h"
#include "utils/hash.h"
#include "utils/uuid.h"
#include "utils/arena.h"
#include "utils/string.h"
#include "utils/map.h"

#include "math/math.h"
#include "math/vec2.h"
#include "math/vec4.h"

#include "core/action.h"
#include "core/time.h"
#include "core/app.h"
#include "core/input.h"
#include "core/log.h"
#include "core/shortcut.h"

#include "ui/panel.h"

#include "filesystem/filesystem.h"
#include "filesystem/iterator.h"
#include "filesystem/archive.h"

#include "serialization/json_writer.h"
#include "serialization/json_reader.h"
#include "serialization/json.h"

#include "renderer/renderer.h"
#include "renderer/shaders/base.glsl.h"
#include "renderer/asset_manager.h"
#include "core/theme.h" // need Naui_Color

#include "threading/jobs.h"
#include "threading/threads.h"

#include "localization/localization.h"

// source files
#include "utils/arena.c"
#include "utils/uuid_unix.c"
#include "utils/uuid_win32.c"
#include "utils/string.c"

#include "core/shortcut.c"
#include "core/time.c"
#include "core/app.c"
#include "core/action.c"
#include "core/input.c"
#include "core/log.c"
#include "core/theme.c"

#include "ui/panel.c"

#include "serialization/json.c"
#include "serialization/json_writer.c"
#include "serialization/json_reader.c"

#include "renderer/renderer.c"
#include "renderer/asset_manager.c"

#include "threading/jobs.c"
#include "threading/thread_win32.c"
#include "threading/thread_unix.c"

#include "filesystem/iterator_unix.c"
#include "filesystem/filesystem_win32.c"
#include "filesystem/iterator_win32.c"
#include "filesystem/filesystem_unix.c"
#include "filesystem/archive.c"

#include "localization/localization.c"
