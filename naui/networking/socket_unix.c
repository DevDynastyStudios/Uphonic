#if !defined(_WIN32) && !defined(_WIN64)

typedef int net_native;
typedef socklen_t net_socklen;
typedef struct pollfd net_pollfd;

#define NET_NATIVE_INVALID (-1)

#define NET_LAST_ERROR() errno

#define NET_ERR_WOULD_BLOCK(error) ((error) == EAGAIN || (error) == EWOULDBLOCK)
#define NET_ERR_IN_PROGRESS(error) ((error) == EINPROGRESS || (error) == EINTR)
#define NET_ERR_INTERRUPTED(error) ((error) == EINTR)

#define NET_POLL(fds, count, timeout_ms) poll((fds), (nfds_t)(count), (timeout_ms))

#ifdef MSG_NOSIGNAL
	#define NET_SEND_FLAGS MSG_NOSIGNAL
#else
	#define NET_SEND_FLAGS 0
#endif

static bool _naui_net_platform_init(void)
{
	return true;
}

static void _naui_net_platform_shutdown(void)
{

}

static void _naui_net_native_close(net_native handle)
{
	close(handle);
}

static bool _naui_net_native_set_nonblocking(net_native handle)
{
	int flags = fcntl(handle, F_GETFL, 0);

	if (flags < 0)
		return false;

	if (fcntl(handle, F_SETFL, flags | O_NONBLOCK) != 0)
		return false;

	int fd_flags = fcntl(handle, F_GETFD, 0);

	if (fd_flags >= 0)
		fcntl(handle, F_SETFD, fd_flags | FD_CLOEXEC);

	return true;
}

static void _naui_net_native_udp_fixups(net_native handle)
{
	(void)handle;
}

static void _naui_net_native_tcp_fixups(net_native handle)
{
#ifdef SO_NOSIGPIPE
	int on = 1;
	setsockopt(handle, SOL_SOCKET, SO_NOSIGPIPE, (const char*)&on, sizeof(on));
#else
	(void)handle;
#endif
}

static void _naui_net_native_apply_reuse(net_native handle, bool reuse)
{
	if (!reuse)
		return;

	int on = 1;
	setsockopt(handle, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));
}

static bool _naui_net_native_is_transient_accept_error(int error)
{
	return error == ECONNABORTED;
}

#endif