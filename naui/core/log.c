#define NAUI_LOG_COLOR_WHT "\033[0;97m"
#define NAUI_LOG_COLOR_GRN "\033[0;92m"
#define NAUI_LOG_COLOR_YEL "\033[0;93m"
#define NAUI_LOG_COLOR_RED "\033[0;91m"
#define NAUI_LOG_COLOR_MAG "\033[1;95m"

#define NAUI_LOG_MAX_TEXT_LEN 256

static Naui_LogLevel _minimum_log_level = 0;

void naui_set_log_minimum_level(Naui_LogLevel log_level)
{
    _minimum_log_level = log_level;
}

void naui_log(Naui_LogLevel log_level, const char *fmt, ...)
{
    if (log_level >= _minimum_log_level && fmt)
    {
        va_list args;
        va_start(args, fmt);

        char buffer[NAUI_LOG_MAX_TEXT_LEN] = {0};
        size_t fmt_len = strlen(fmt);
        assert(fmt_len < NAUI_LOG_MAX_TEXT_LEN); // get rekt idiot

        switch (log_level)
        {
#ifdef DEBUG
            case NAUI_LOG_DEBUG:    strncpy(buffer, NAUI_LOG_COLOR_WHT "[DEBUG]: "   NAUI_LOG_COLOR_WHT, 23); break;
#endif
            case NAUI_LOG_INFO:     strncpy(buffer, NAUI_LOG_COLOR_GRN "[INFO]: "    NAUI_LOG_COLOR_WHT, 22); break;
            case NAUI_LOG_WARNING:  strncpy(buffer, NAUI_LOG_COLOR_YEL "[WARNING]: " NAUI_LOG_COLOR_WHT, 25); break;
            case NAUI_LOG_ERROR:    strncpy(buffer, NAUI_LOG_COLOR_RED "[ERROR]: "   NAUI_LOG_COLOR_WHT, 23); break;
            case NAUI_LOG_FUCKED:   strncpy(buffer, NAUI_LOG_COLOR_MAG "[FUCKED]: "  NAUI_LOG_COLOR_WHT, 24); break;
            default: break;
        }

        memcpy(buffer + strlen(buffer), fmt, (fmt_len < (NAUI_LOG_MAX_TEXT_LEN - 12))? fmt_len : (NAUI_LOG_MAX_TEXT_LEN - 12));
        strcat(buffer, "\n");
        vprintf(buffer, args);
        fflush(stdout);

        va_end(args);

        if (log_level == NAUI_LOG_FUCKED) abort();
    }
}
