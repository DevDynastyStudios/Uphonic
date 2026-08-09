enum
{
    NAUI_LOG_DEBUG,
    NAUI_LOG_INFO,
    NAUI_LOG_WARNING,
    NAUI_LOG_ERROR,
    NAUI_LOG_FUCKED, // may day, may day, we're going down, we're going down
};
typedef uint32_t Naui_LogLevel;

NAUI_API void naui_set_log_minimum_level(Naui_LogLevel log_level);
NAUI_API void naui_log(Naui_LogLevel log_level, const char *fmt, ...);
