#define NET_MAX_IO_CHUNK ((size_t)1 << 30)
#define NET_POLL_STACK_ITEMS 64

static int _naui_net_sockets_refcount = 0;

static bool _naui_net_to_sockaddr(const Naui_Address* address, Naui_AddressFamily socket_family, struct sockaddr_storage* storage, net_socklen* out_len)
{
	memset(storage, 0, sizeof(*storage));

	if (address->family == NAUI_ADDRESS_IPV4 && socket_family == NAUI_ADDRESS_IPV4)
	{
		struct sockaddr_in* sin = (struct sockaddr_in*)storage;
		sin->sin_family = AF_INET;
		sin->sin_port = htons(address->port);
		memcpy(&sin->sin_addr, address->data, 4);
		*out_len = (net_socklen)sizeof(*sin);
		return true;
	}

	if (socket_family != NAUI_ADDRESS_IPV6)
		return false;

	struct sockaddr_in6* sin6 = (struct sockaddr_in6*)storage;
	sin6->sin6_family = AF_INET6;
	sin6->sin6_port = htons(address->port);

	if (address->family == NAUI_ADDRESS_IPV4)
	{
		static const uint8_t zero4[4] = { 0 };
		if (memcmp(address->data, zero4, 4) != 0)
		{
			sin6->sin6_addr.s6_addr[10] = 0xff;
			sin6->sin6_addr.s6_addr[11] = 0xff;
			memcpy(&sin6->sin6_addr.s6_addr[12], address->data, 4);
		}
	}
	else if (address->family == NAUI_ADDRESS_IPV6)
	{
		memcpy(&sin6->sin6_addr, address->data, 16);
		sin6->sin6_scope_id = address->scope_id;
	}
	else
	{
		return false;
	}

	*out_len = (net_socklen)sizeof(*sin6);
	return true;
}

static bool _naui_net_from_sockaddr(const struct sockaddr* sa, Naui_Address* out)
{
	memset(out, 0, sizeof(*out));

	if (sa->sa_family == AF_INET)
	{
		const struct sockaddr_in* sin = (const struct sockaddr_in*)sa;
		out->family = NAUI_ADDRESS_IPV4;
		out->port = ntohs(sin->sin_port);
		memcpy(out->data, &sin->sin_addr, 4);
		return true;
	}

	if (sa->sa_family == AF_INET6)
	{
		const struct sockaddr_in6* sin6 = (const struct sockaddr_in6*)sa;
		out->family = NAUI_ADDRESS_IPV6;
		out->port = ntohs(sin6->sin6_port);
		out->scope_id = (uint32_t)sin6->sin6_scope_id;
		memcpy(out->data, &sin6->sin6_addr, 16);
		naui_address_normalize(out);
		return true;
	}

	return false;
}

static net_native _naui_net_native_of(const Naui_Socket* sock)
{
	return (net_native)sock->handle;
}

static void _naui_net_refresh_local_address(Naui_Socket* sock)
{
	struct sockaddr_storage storage;
	net_socklen len = (net_socklen)sizeof(storage);

	if (getsockname(_naui_net_native_of(sock), (struct sockaddr*)&storage, &len) == 0)
		_naui_net_from_sockaddr((const struct sockaddr*)&storage, &sock->address);
}

static void _naui_net_apply_options(net_native handle, Naui_SocketType type)
{
	if (type == NAUI_SOCKET_TCP)
	{
		int on = 1;
		setsockopt(handle, IPPROTO_TCP, TCP_NODELAY, (const char*)&on, sizeof(on));
		_naui_net_native_tcp_fixups(handle);
		return;
	}

	_naui_net_native_udp_fixups(handle);
}

static bool _naui_net_parse_port(const char* text, uint16_t* out)
{
	if (!text || !*text)
		return false;

	uint32_t value = 0;
	size_t digits = 0;
	for (; *text; ++text, ++digits)
	{
		if (*text < '0' || *text > '9' || digits >= 5)
			return false;

		value = value * 10 + (uint32_t)(*text - '0');
	}

	if (value > 65535)
		return false;

	*out = (uint16_t)value;
	return true;
}

Naui_Address naui_address_any(Naui_AddressFamily family, uint16_t port)
{
	Naui_Address address;
	memset(&address, 0, sizeof(address));
	address.family = family;
	address.port = port;
	return address;
}

Naui_Address naui_address_loopback(Naui_AddressFamily family, uint16_t port)
{
	Naui_Address address = naui_address_any(family, port);
	if (family == NAUI_ADDRESS_IPV4)
	{
		address.data[0] = 127;
		address.data[3] = 1;
	}
	else if (family == NAUI_ADDRESS_IPV6)
	{
		address.data[15] = 1;
	}

	return address;
}

bool naui_address_parse(Naui_Address* out, const char* text)
{
	if (!out || !text)
		return false;

	char host[64];
	const char* port_text = NULL;

	if (text[0] == '[')
	{
		const char* close = strchr(text, ']');
		if (!close)
			return false;

		size_t host_len = (size_t)(close - text - 1);
		if (host_len == 0 || host_len >= sizeof(host))
			return false;

		memcpy(host, text + 1, host_len);
		host[host_len] = 0;

		if (close[1] == ':')
			port_text = close + 2;
		else if (close[1] != 0)
			return false;
	}
	else
	{
		const char* first_colon = strchr(text, ':');
		if (first_colon && strchr(first_colon + 1, ':'))
		{
			size_t host_len = strlen(text);
			if (host_len >= sizeof(host))
				return false;

			memcpy(host, text, host_len + 1);
		}
		else
		{
			size_t host_len = first_colon ? (size_t)(first_colon - text) : strlen(text);
			if (host_len == 0 || host_len >= sizeof(host))
				return false;

			memcpy(host, text, host_len);
			host[host_len] = 0;
			if (first_colon)
				port_text = first_colon + 1;
		}
	}

	Naui_Address address;
	memset(&address, 0, sizeof(address));

	if (port_text && !_naui_net_parse_port(port_text, &address.port))
		return false;

	char* percent = strchr(host, '%');
	if (percent)
	{
		*percent = 0;
		const char* scope_text = percent + 1;
		if (!*scope_text)
			return false;

		uint64_t scope = 0;
		for (; *scope_text; ++scope_text)
		{
			if (*scope_text < '0' || *scope_text > '9')
				return false;

			scope = scope * 10 + (uint64_t)(*scope_text - '0');
			if (scope > 0xffffffffull)
				return false;
		}

		address.scope_id = (uint32_t)scope;
	}

	if (!percent && inet_pton(AF_INET, host, address.data) == 1)
	{
		address.family = NAUI_ADDRESS_IPV4;
	}
	else if (inet_pton(AF_INET6, host, address.data) == 1)
	{
		address.family = NAUI_ADDRESS_IPV6;
		naui_address_normalize(&address);
	}
	else
	{
		return false;
	}

	*out = address;
	return true;
}

bool naui_address_resolve(Naui_Address* out, const char* host, uint16_t port)
{
	if (!out || !host || !*host)
		return false;

	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	struct addrinfo* results = NULL;
	if (getaddrinfo(host, NULL, &hints, &results) != 0 || !results)
		return false;

	bool found = false;
	for (struct addrinfo* it = results; it && !found; it = it->ai_next)
	{
		Naui_Address address;
		if (_naui_net_from_sockaddr(it->ai_addr, &address))
		{
			address.port = port;
			*out = address;
			found = true;
		}
	}

	freeaddrinfo(results);
	return found;
}

void naui_address_format(const Naui_Address* address, char* buf, size_t size)
{
	if (!buf || size == 0)
		return;

	buf[0] = 0;
	if (!address)
		return;

	char host[INET6_ADDRSTRLEN] = { 0 };
	if (address->family == NAUI_ADDRESS_IPV4)
	{
		inet_ntop(AF_INET, address->data, host, sizeof(host));
		snprintf(buf, size, "%s:%u", host, (unsigned)address->port);
	}
	else if (address->family == NAUI_ADDRESS_IPV6)
	{
		inet_ntop(AF_INET6, address->data, host, sizeof(host));
		if (address->scope_id)
			snprintf(buf, size, "[%s%%%u]:%u", host, (unsigned)address->scope_id, (unsigned)address->port);
		else
			snprintf(buf, size, "[%s]:%u", host, (unsigned)address->port);
	}
	else
	{
		snprintf(buf, size, "(none)");
	}
}

bool naui_address_equal(const Naui_Address* a, const Naui_Address* b)
{
	if (a->family != b->family || a->port != b->port)
		return false;

	if (a->family == NAUI_ADDRESS_IPV4)
		return memcmp(a->data, b->data, 4) == 0;

	if (a->family == NAUI_ADDRESS_IPV6)
		return a->scope_id == b->scope_id && memcmp(a->data, b->data, 16) == 0;

	return true;
}

void naui_address_normalize(Naui_Address* address)
{
	static const uint8_t mapped_prefix[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff };
	if (!address || address->family != NAUI_ADDRESS_IPV6)
		return;

	if (memcmp(address->data, mapped_prefix, sizeof(mapped_prefix)) != 0)
		return;

	uint8_t v4[4];
	memcpy(v4, address->data + 12, 4);
	memset(address->data, 0, sizeof(address->data));
	memcpy(address->data, v4, 4);
	address->family = NAUI_ADDRESS_IPV4;
	address->scope_id = 0;
}

bool naui_sockets_init(void)
{
	if (_naui_net_sockets_refcount == 0 && !_naui_net_platform_init())
		return false;

	_naui_net_sockets_refcount++;
	return true;
}

void naui_sockets_shutdown(void)
{
	if (_naui_net_sockets_refcount <= 0)
		return;

	_naui_net_sockets_refcount--;
	if (_naui_net_sockets_refcount == 0)
		_naui_net_platform_shutdown();
}

bool naui_socket_is_valid(const Naui_Socket* sock)
{
	return sock && sock->handle != NAUI_SOCKET_INVALID;
}

bool naui_socket_open(Naui_Socket* sock, Naui_SocketType type, Naui_AddressFamily family)
{
	if (!sock)
		return false;

	*sock = NAUI_SOCKET_NULL;
	if (family != NAUI_ADDRESS_IPV4 && family != NAUI_ADDRESS_IPV6)
		return false;

	int af = family == NAUI_ADDRESS_IPV6 ? AF_INET6 : AF_INET;
	int kind = type == NAUI_SOCKET_TCP ? SOCK_STREAM : SOCK_DGRAM;
	net_native handle = socket(af, kind, 0);

	if (handle == NET_NATIVE_INVALID)
		return false;

	if (!_naui_net_native_set_nonblocking(handle))
	{
		_naui_net_native_close(handle);
		return false;
	}

	if (family == NAUI_ADDRESS_IPV6)
	{
		int v6_only = 0;
		setsockopt(handle, IPPROTO_IPV6, IPV6_V6ONLY, (const char*)&v6_only, sizeof(v6_only));
	}

	_naui_net_apply_options(handle, type);
	sock->handle = (intptr_t)handle;
	sock->type = type;
	sock->family = family;
	return true;
}

void naui_socket_close(Naui_Socket* sock)
{
	if (!naui_socket_is_valid(sock))
		return;

	_naui_net_native_close(_naui_net_native_of(sock));
	*sock = NAUI_SOCKET_NULL;
}

bool naui_socket_bind(Naui_Socket* sock, const Naui_Address* address, bool reuse)
{
	if (!naui_socket_is_valid(sock))
		return false;

	if (!address)
		return false;

	_naui_net_native_apply_reuse(_naui_net_native_of(sock), reuse);
	struct sockaddr_storage storage;
	net_socklen len = 0;

	if (!_naui_net_to_sockaddr(address, sock->family, &storage, &len))
		return false;

	if (bind(_naui_net_native_of(sock), (struct sockaddr*)&storage, len) != 0)
		return false;

	_naui_net_refresh_local_address(sock);
	return true;
}

bool naui_socket_listen(Naui_Socket* sock, int32_t backlog)
{
	if (!naui_socket_is_valid(sock))
		return false;

	if (sock->type != NAUI_SOCKET_TCP)
		return false;

	if (listen(_naui_net_native_of(sock), backlog > 0 ? backlog : 16) != 0)
		return false;

	sock->flags |= NAUI_SOCKET_FLAG_LISTENING;
	return true;
}

Naui_NetResult naui_socket_accept(Naui_Socket* listener, Naui_Socket* out_client, Naui_Address* out_peer)
{
	if (!naui_socket_is_valid(listener))
		return NAUI_NET_ERROR;

	if (!out_client)
		return NAUI_NET_ERROR;

	struct sockaddr_storage storage;
	net_socklen len = (net_socklen)sizeof(storage);
	net_native handle = accept(_naui_net_native_of(listener), (struct sockaddr*)&storage, &len);

	if (handle == NET_NATIVE_INVALID)
	{
		int error = NET_LAST_ERROR();
		if (NET_ERR_WOULD_BLOCK(error))
			return NAUI_NET_WOULD_BLOCK;

		if (NET_ERR_INTERRUPTED(error))
			return NAUI_NET_WOULD_BLOCK;

		if (_naui_net_native_is_transient_accept_error(error))
			return NAUI_NET_WOULD_BLOCK;

		return NAUI_NET_ERROR;
	}

	if (!_naui_net_native_set_nonblocking(handle))
	{
		_naui_net_native_close(handle);
		return NAUI_NET_ERROR;
	}

	_naui_net_apply_options(handle, NAUI_SOCKET_TCP);
	*out_client = NAUI_SOCKET_NULL;
	out_client->handle = (intptr_t)handle;
	out_client->type = NAUI_SOCKET_TCP;
	out_client->family = listener->family;
	_naui_net_refresh_local_address(out_client);

	if (out_peer && !_naui_net_from_sockaddr((const struct sockaddr*)&storage, out_peer))
		memset(out_peer, 0, sizeof(*out_peer));

	return NAUI_NET_OK;
}

Naui_NetResult naui_socket_connect(Naui_Socket* sock, const Naui_Address* address)
{
	if (!naui_socket_is_valid(sock))
		return NAUI_NET_ERROR;

	if (!address)
		return NAUI_NET_ERROR;

	struct sockaddr_storage storage;
	net_socklen len = 0;

	if (!_naui_net_to_sockaddr(address, sock->family, &storage, &len))
		return NAUI_NET_ERROR;

	if (connect(_naui_net_native_of(sock), (struct sockaddr*)&storage, len) == 0)
	{
		_naui_net_refresh_local_address(sock);
		return NAUI_NET_OK;
	}

	return NET_ERR_IN_PROGRESS(NET_LAST_ERROR()) ? NAUI_NET_IN_PROGRESS : NAUI_NET_ERROR;
}

Naui_NetResult naui_socket_connect_finish(Naui_Socket* sock)
{
	if (!naui_socket_is_valid(sock))
		return NAUI_NET_ERROR;

	int error = 0;
	net_socklen len = (net_socklen)sizeof(error);
	if (getsockopt(_naui_net_native_of(sock), SOL_SOCKET, SO_ERROR, (char*)&error, &len) != 0)
		return NAUI_NET_ERROR;

	if (error != 0)
		return NAUI_NET_ERROR;

	_naui_net_refresh_local_address(sock);
	return NAUI_NET_OK;
}

Naui_NetResult naui_socket_send(Naui_Socket* sock, const void* data, size_t size, size_t* out_count)
{
	if (out_count)
		*out_count = 0;

	if (!naui_socket_is_valid(sock) || !data && size)
		return NAUI_NET_ERROR;

	if (size == 0)
		return NAUI_NET_OK;

	if (size > NET_MAX_IO_CHUNK)
		size = NET_MAX_IO_CHUNK;

	while (true)
	{
		long count = (long)send(_naui_net_native_of(sock), (const char*)data, (int)size, NET_SEND_FLAGS);
		if (count >= 0)
		{
			if (out_count)
				*out_count = (size_t)count;

			return NAUI_NET_OK;
		}

		int error = NET_LAST_ERROR();
		if (NET_ERR_INTERRUPTED(error))
			continue;

		if (NET_ERR_WOULD_BLOCK(error))
			return NAUI_NET_WOULD_BLOCK;

		return NAUI_NET_ERROR;
	}
}

Naui_NetResult naui_socket_recv(Naui_Socket* sock, void* buffer, size_t size, size_t* out_count)
{
	if (out_count)
		*out_count = 0;

	if (!naui_socket_is_valid(sock))
		return NAUI_NET_ERROR;

	if (!buffer)
		return NAUI_NET_ERROR;

	if (size == 0)
		return NAUI_NET_ERROR;

	if (size > NET_MAX_IO_CHUNK)
		size = NET_MAX_IO_CHUNK;

	while (true)
	{
		long count = (long)recv(_naui_net_native_of(sock), (char*)buffer, (int)size, 0);
		if (count > 0)
		{
			if (out_count)
				*out_count = (size_t)count;

			return NAUI_NET_OK;
		}

		if (count == 0)
			return NAUI_NET_CLOSED;

		int error = NET_LAST_ERROR();
		if (NET_ERR_INTERRUPTED(error))
			continue;

		if (NET_ERR_WOULD_BLOCK(error))
			return NAUI_NET_WOULD_BLOCK;

		return NAUI_NET_ERROR;
	}
}

Naui_NetResult naui_socket_sendto(Naui_Socket* sock, const void* data, size_t size, const Naui_Address* to, size_t* out_count)
{
	if (out_count)
		*out_count = 0;

	if (!naui_socket_is_valid(sock) || !to || !data && size)
		return NAUI_NET_ERROR;

	struct sockaddr_storage storage;
	net_socklen len = 0;

	if (!_naui_net_to_sockaddr(to, sock->family, &storage, &len))
		return NAUI_NET_ERROR;

	if (size > NET_MAX_IO_CHUNK)
		return NAUI_NET_ERROR;

	while (true)
	{
		long count = (long)sendto(_naui_net_native_of(sock), (const char*)data, (int)size, NET_SEND_FLAGS, (struct sockaddr*)&storage, len);
		if (count >= 0)
		{
			if (out_count)
				*out_count = (size_t)count;

			return NAUI_NET_OK;
		}

		int error = NET_LAST_ERROR();
		if (NET_ERR_INTERRUPTED(error))
			continue;

		if (NET_ERR_WOULD_BLOCK(error))
			return NAUI_NET_WOULD_BLOCK;

		return NAUI_NET_ERROR;
	}
}

Naui_NetResult naui_socket_recvfrom(Naui_Socket* sock, void* buffer, size_t size, size_t* out_count, Naui_Address* out_from)
{
	if (out_count)
		*out_count = 0;

	if (!naui_socket_is_valid(sock) || !buffer || size == 0)
		return NAUI_NET_ERROR;

	if (size > NET_MAX_IO_CHUNK)
		size = NET_MAX_IO_CHUNK;

	while (true)
	{
		struct sockaddr_storage storage;
		net_socklen len = (net_socklen)sizeof(storage);
		long count = (long)recvfrom(_naui_net_native_of(sock), (char*)buffer, (int)size, 0, (struct sockaddr*)&storage, &len);

		if (count >= 0)
		{
			if (out_count)
				*out_count = (size_t)count;

			if (out_from && !_naui_net_from_sockaddr((const struct sockaddr*)&storage, out_from))
				memset(out_from, 0, sizeof(*out_from));

			return NAUI_NET_OK;
		}

		int error = NET_LAST_ERROR();
		if (NET_ERR_INTERRUPTED(error))
			continue;

		if (NET_ERR_WOULD_BLOCK(error))
			return NAUI_NET_WOULD_BLOCK;

		return NAUI_NET_ERROR;
	}
}

int32_t naui_socket_poll(Naui_PollItem* items, size_t count, int32_t timeout_ms)
{
	if (count == 0)
	{
		if (timeout_ms > 0)
			naui_thread_sleep_ms((uint32_t)timeout_ms);

		return 0;
	}

	if (!items)
		return -1;

	net_pollfd stack_fds[NET_POLL_STACK_ITEMS];
	size_t stack_map[NET_POLL_STACK_ITEMS];

	net_pollfd* fds = stack_fds;
	size_t* map = stack_map;

	if (count > NET_POLL_STACK_ITEMS)
	{
		fds = (net_pollfd*)malloc(count * sizeof(*fds));
		map = (size_t*)malloc(count * sizeof(*map));

		if (!fds || !map)
		{
			free(fds);
			free(map);
			return -1;
		}
	}

	size_t used = 0;
	for (size_t i = 0; i < count; i++)
	{
		items[i].got = 0;
		if (!naui_socket_is_valid(items[i].sock))
			continue;

		memset(&fds[used], 0, sizeof(fds[used]));
		fds[used].fd = _naui_net_native_of(items[i].sock);

		if (items[i].want & NAUI_POLL_READ)
			fds[used].events |= POLLIN;

		if (items[i].want & NAUI_POLL_WRITE)
			fds[used].events |= POLLOUT;

		map[used] = i;
		used++;
	}

	int32_t result = 0;
	if (used == 0)
	{
		if (timeout_ms > 0)
			naui_thread_sleep_ms((uint32_t)timeout_ms);
	}
	else
	{
		int rc = NET_POLL(fds, used, timeout_ms);
		if (rc < 0)
			result = NET_ERR_INTERRUPTED(NET_LAST_ERROR()) ? 0 : -1;
		else
		{
			for (size_t i = 0; i < used; i++)
			{
				Naui_PollEvents got = 0;
				if (fds[i].revents & POLLIN)
					got |= NAUI_POLL_READ;

				if (fds[i].revents & POLLOUT)
					got |= NAUI_POLL_WRITE;

				if (fds[i].revents & (POLLERR | POLLHUP | POLLNVAL))
					got |= NAUI_POLL_ERROR;

				items[map[i]].got = got;
				if (got)
					result++;
			}
		}
	}

	if (fds != stack_fds)
	{
		free(fds);
		free(map);
	}

	return result;
}