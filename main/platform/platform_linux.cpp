#include "platform.h"

#if UPH_PLATFORM_LINUX

// We'll get Big Smoke to handle the rest

void uph_platform_initialize(const UphPlatformCreateInfo *create_info)
{

}

void uph_platform_shutdown(void)
{

}

void uph_platform_begin(void)
{

}

void uph_platform_end(void)
{

}

double uph_get_time(void)
{

}

std::tm uph_localtime(std::time_t t)
{
    std::tm tm{};
    localtime_r(&t, &tm);
    return tm;
}

std::tm uph_localtime(const std::filesystem::file_time_type& ftime)
{
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now()
    );

    std::time_t tt = std::chrono::system_clock::to_time_t(sctp);
    return uph_localtime(tt);
}

#endif