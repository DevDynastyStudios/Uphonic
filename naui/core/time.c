float naui_time(void)
{
    return mg_app_time();
}

float naui_delta_time(void)
{
    return mg_app_delta_time();
}

uint64_t naui_unix_time(void)
{
	return (uint64_t)time(NULL);
}
