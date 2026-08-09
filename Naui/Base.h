#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#	define NAUI_WINDOWS 1
#elif defined(__linux__) || defined(__gnu_linux__)
#	define NAUI_LINUX 1
#elif defined(__APPLE__) && defined(__MACH__)
#	define NAUI_MACOS 1
#endif

#define NAUI_API

#if defined(_MSC_VER)
#	define NAUI_THREAD_LOCAL __declspec(thread)
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_THREADS__)
#	define NAUI_THREAD_LOCAL _Thread_local
#else
#	define NAUI_THREAD_LOCAL __thread
#endif

#if defined(_MSC_VER)
#	define NAUI_NODISCARD _Check_return_
#elif defined(__GNUC__) || defined(__clang__)
#	define NAUI_NODISCARD __attribute__((warn_unused_result))
#else
#	define NAUI_NODISCARD
#endif

#if defined(__clang__)
#if defined(_WIN32)
#pragma clang diagnostic ignored "-Wdeprecated"
#endif // _WIN32
#pragma clang diagnostic ignored "-Wc99-designator"
#endif // __clang__
