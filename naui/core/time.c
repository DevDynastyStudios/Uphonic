float naui_time(void)
{
    return mgapp_time();
}

float naui_frame_time(void)
{
    return mgapp_frame_time();
}

float naui_delta_time(void)
{
    return mgapp_delta_time();
}

uint64_t naui_unix_time(void)
{
	return (uint64_t)time(NULL);
}
