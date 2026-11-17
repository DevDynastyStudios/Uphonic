#if defined(_WIN32) || defined(_WIN64)

typedef SOCKET net_native;
typedef int net_socklen;
typedef WSAPOLLFD net_pollfd;

#define NET_NATIVE_INVALID INVALID_SOCKET

#define NET_LAST_ERROR() WSAGetLastError()

#define NET_ERR_WOULD_BLOCK(error) ((error) == WSAEWOULDBLOCK)
#define NET_ERR_IN_PROGRESS(error) ((error) == WSAEWOULDBLOCK || (error) == WSAEINPROGRESS)
#define NET_ERR_INTERRUPTED(error) ((error) == WSAEINTR)

#define NET_SEND_FLAGS 0

#define NET_POLL(fds, count, timeout_ms) WSAPoll((fds), (ULONG)(count), (timeout_ms))

#ifndef SIO_UDP_CONNRESET
	#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR, 12)
#endif

static bool _naui_net_platform_init(void)
{
	WSADATA data;

	if (WSAStartup(MAKEWORD(2, 2), &data) != 0)
		return false;

	return true;
}

static void _naui_net_platform_shutdown(void)
{
	WSACleanup();
}

static void _naui_net_native_close(net_native handle)
{
	closesocket(handle);
}

static bool _naui_net_native_set_nonblocking(net_native handle)
{
	u_long enable = 1;
	return ioctlsocket(handle, FIONBIO, &enable) == 0;
}

static void _naui_net_native_udp_fixups(net_native handle)
{
	BOOL disable = FALSE;
	DWORD returned = 0;
	WSAIoctl(handle, SIO_UDP_CONNRESET, &disable, sizeof(disable), NULL, 0, &returned, NULL, NULL);
}

static void _naui_net_native_tcp_fixups(net_native handle)
{
	(void)handle;
}

static void _naui_net_native_apply_reuse(net_native handle, bool reuse)
{
	(void)reuse;
	BOOL exclusive = TRUE;
	setsockopt(handle, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (const char*)&exclusive, sizeof(exclusive));
}

static bool _naui_net_native_is_transient_accept_error(int error)
{
	(void)error;
	return false;
}

#endif