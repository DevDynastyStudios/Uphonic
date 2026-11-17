#define NET_UDP_MAX_PACKETS 512
#define NET_UDP_CONNECT_RETRY_MS 500

#define NET_UDP_HEADER_SIZE 4
#define NET_UDP_PACKET_MAX 1200
#define NET_UDP_CONNECT_SIZE 256

enum
{
	NET_UDP_CONNECT = 1,
	NET_UDP_ACCEPT = 2,
	NET_UDP_REJECT = 3,
	NET_UDP_DATA = 4,
	NET_UDP_PING = 5,
	NET_UDP_BYE = 6,
};

enum
{
	NET_REJECT_FULL = 1,
	NET_REJECT_SELF = 2,
};

static void _naui_net_udp_header(uint8_t* p, uint8_t type)
{
	p[0] = 'N';
	p[1] = 'U';
	p[2] = 1;
	p[3] = type;
}

static void _naui_net_udp_send_raw(Naui_Net net, const Naui_Address* to, const void* data, size_t size)
{
	if (!naui_socket_is_valid(&net->udp))
		return;

	size_t sent = 0;
	naui_socket_sendto(&net->udp, data, size, to, &sent);
}

static void _naui_net_udp_send_session(Naui_Net net, NetConnection* connection, uint8_t type, const void* payload, size_t size)
{
	uint8_t packet[NET_UDP_PACKET_MAX];
	if (NET_UDP_HEADER_SIZE + 8 + size > sizeof(packet))
		return;

	_naui_net_udp_header(packet, type);
	_naui_net_put_u64(packet + 4, connection->session_id);

	if (size)
		memcpy(packet + 12, payload, size);

	size_t total = 12 + size;
	_naui_net_udp_send_raw(net, &connection->remote, packet, total);
	connection->bytes_sent += total;
	connection->last_send_ms = net->now_ms;
}

static void _naui_net_udp_send_connect(Naui_Net net, NetConnection* connection)
{
	uint8_t packet[NET_UDP_CONNECT_SIZE];
	memset(packet, 0, sizeof(packet));

	_naui_net_udp_header(packet, NET_UDP_CONNECT);
	_naui_net_put_u64(packet + 4, connection->client_token);
	_naui_net_hello_encode(&net->local, packet + 12);
	_naui_net_udp_send_raw(net, &connection->remote, packet, sizeof(packet));

	connection->last_connect_ms = net->now_ms;
	connection->last_send_ms = net->now_ms;
}

static void _naui_net_udp_send_accept(Naui_Net net, NetConnection* connection)
{
	uint8_t packet[NET_UDP_HEADER_SIZE + 16 + NET_HELLO_MAX];

	_naui_net_udp_header(packet, NET_UDP_ACCEPT);
	_naui_net_put_u64(packet + 4, connection->client_token);
	_naui_net_put_u64(packet + 12, connection->session_id);

	size_t hello_size = _naui_net_hello_encode(&net->local, packet + 20);
	_naui_net_udp_send_raw(net, &connection->remote, packet, 20 + hello_size);
	connection->last_send_ms = net->now_ms;
}

static void _naui_net_udp_send_reject(Naui_Net net, const Naui_Address* address, uint64_t token, uint8_t reason)
{
	uint8_t packet[13];
	_naui_net_udp_header(packet, NET_UDP_REJECT);
	_naui_net_put_u64(packet + 4, token);
	packet[12] = reason;
	_naui_net_udp_send_raw(net, address, packet, sizeof(packet));
}

static NetConnection* _naui_net_udp_find_incoming(Naui_Net net, const Naui_Address* from, uint64_t token)
{
	for (ptrdiff_t i = 0; i < naui_list_len(net->connections); i++)
	{
		NetConnection* connection = net->connections[i];
		if (connection->transport != NAUI_TRANSPORT_UDP)
			continue;

		if (!connection->incoming)
			continue;

		if (connection->phase == NET_PHASE_CLOSED)
			continue;

		if (connection->client_token != token)
			continue;

		if (!naui_address_equal(&connection->remote, from))
			continue;

		return connection;
	}

	return NULL;
}

static void _naui_net_udp_close_replaced(Naui_Net net, const NetConnection* replacement)
{
	for (ptrdiff_t i = 0; i < naui_list_len(net->connections); i++)
	{
		NetConnection* connection = net->connections[i];
		if (connection == replacement || connection->transport != NAUI_TRANSPORT_UDP || !connection->incoming)
			continue;

		if (naui_address_equal(&connection->remote, &replacement->remote))
			_naui_net_close_connection(net, connection, NAUI_DISCONNECT_REMOTE, false);
	}
}

static NetConnection* _naui_net_udp_find_connecting(Naui_Net net, const Naui_Address* from, uint64_t token)
{
	for (ptrdiff_t i = 0; i < naui_list_len(net->connections); i++)
	{
		NetConnection* connection = net->connections[i];
		if (connection->transport != NAUI_TRANSPORT_UDP)
			continue;

		if (connection->incoming)
			continue;

		if (connection->phase != NET_PHASE_CONNECTING)
			continue;

		if (connection->client_token != token)
			continue;

		if (!naui_address_equal(&connection->remote, from))
			continue;

		return connection;
	}

	return NULL;
}

static NetConnection* _naui_net_udp_find_session(Naui_Net net, const Naui_Address* from, uint64_t session_id)
{
	for (ptrdiff_t i = 0; i < naui_list_len(net->connections); i++)
	{
		NetConnection* connection = net->connections[i];
		if (connection->transport != NAUI_TRANSPORT_UDP)
			continue;

		if (connection->session_id != session_id)
			continue;

		if (connection->phase == NET_PHASE_CLOSED || connection->phase == NET_PHASE_CONNECTING)
			continue;

		if (!naui_address_equal(&connection->remote, from))
			continue;

		return connection;
	}

	return NULL;
}

static void _naui_net_udp_on_connect(Naui_Net net, const uint8_t* packet, size_t size, const Naui_Address* from)
{
	if (!net->udp_listening)
		return;

	if (size < NET_UDP_CONNECT_SIZE)
		return;

	uint64_t token = _naui_net_get_u64(packet + 4);
	NetHello hello;

	if (token == 0)
		return;

	if (!_naui_net_hello_decode(packet + 12, size - 12, &hello))
		return;

	NetConnection* existing = _naui_net_udp_find_incoming(net, from, token);
	if (existing)
	{
		existing->last_recv_ms = net->now_ms;
		_naui_net_udp_send_accept(net, existing);
		return;
	}

	if (naui_device_id_equal(hello.id, net->local.id))
	{
		_naui_net_udp_send_reject(net, from, token, NET_REJECT_SELF);
		return;
	}

	if (_naui_net_count_pending(net) >= NET_MAX_PENDING)
		return;

	if (_naui_net_count_live(net) >= net->config.max_connections)
	{
		_naui_net_udp_send_reject(net, from, token, NET_REJECT_FULL);
		return;
	}

	NetConnection* connection = _naui_net_new_connection(net, NAUI_TRANSPORT_UDP, true, from);
	if (!connection)
		return;

	connection->phase = NET_PHASE_HANDSHAKE;
	connection->client_token = token;
	connection->session_id = _naui_net_random64();
	connection->pending_hello = hello;
	_naui_net_udp_send_accept(net, connection);
}

static void _naui_net_udp_on_accept(Naui_Net net, const uint8_t* packet, size_t size, const Naui_Address* from)
{
	if (size < 20 + NET_HELLO_FIXED)
		return;

	NetConnection* connection = _naui_net_udp_find_connecting(net, from, _naui_net_get_u64(packet + 4));
	if (!connection)
		return;

	uint64_t session = _naui_net_get_u64(packet + 12);
	NetHello hello;

	if (session == 0 || !_naui_net_hello_decode(packet + 20, size - 20, &hello))
	{
		connection->session_id = session;
		_naui_net_close_connection(net, connection, NAUI_DISCONNECT_PROTOCOL, session != 0);
		return;
	}

	connection->session_id = session;
	connection->last_recv_ms = net->now_ms;
	connection->bytes_received += size;
	if (naui_device_id_equal(hello.id, net->local.id))
	{
		_naui_net_close_connection(net, connection, NAUI_DISCONNECT_SELF, true);
		return;
	}

	_naui_net_complete_handshake(net, connection, &hello);
	if (connection->phase == NET_PHASE_ESTABLISHED)
		_naui_net_udp_send_session(net, connection, NET_UDP_PING, NULL, 0);
}

static void _naui_net_udp_on_reject(Naui_Net net, const uint8_t* packet, size_t size, const Naui_Address* from)
{
	if (size < 13)
		return;

	NetConnection* connection = _naui_net_udp_find_connecting(net, from, _naui_net_get_u64(packet + 4));
	if (!connection)
		return;

	_naui_net_close_connection(
		net,
		connection,
		packet[12] == NET_REJECT_SELF ? NAUI_DISCONNECT_SELF : NAUI_DISCONNECT_REJECTED,
		false);
}

static void _naui_net_udp_on_session_packet(Naui_Net net, uint8_t type, const uint8_t* packet, size_t size, const Naui_Address* from)
{
	if (size < 12)
		return;

	NetConnection* connection = _naui_net_udp_find_session(net, from, _naui_net_get_u64(packet + 4));
	if (!connection)
		return;

	connection->last_recv_ms = net->now_ms;
	connection->bytes_received += size;

	if (type == NET_UDP_BYE)
	{
		_naui_net_close_connection(net, connection, NAUI_DISCONNECT_REMOTE, false);
		return;
	}

	if (connection->phase == NET_PHASE_HANDSHAKE)
	{
		_naui_net_udp_close_replaced(net, connection);
		_naui_net_complete_handshake(net, connection, &connection->pending_hello);
		if (connection->phase != NET_PHASE_ESTABLISHED)
			return;
	}

	if (type == NET_UDP_DATA && size - 12 <= NAUI_UDP_MAX_MESSAGE_SIZE && !_naui_net_events_full(net))
		_naui_net_push_event(net, NAUI_NET_EVENT_MESSAGE, connection, NAUI_DISCONNECT_LOCAL, false, packet + 12, size - 12);
}

static void _naui_net_udp_handle_packet(Naui_Net net, const uint8_t* packet, size_t size, const Naui_Address* from)
{
	if (size < NET_UDP_HEADER_SIZE)
		return;

	if (packet[0] != 'N')
		return;

	if (packet[1] != 'U')
		return;

	if (packet[2] != 1)
		return;

	switch (packet[3])
	{
		case NET_UDP_CONNECT:
		{
			_naui_net_udp_on_connect(net, packet, size, from);
			return;
		}

		case NET_UDP_ACCEPT:
		{
			_naui_net_udp_on_accept(net, packet, size, from);
			return;
		}

		case NET_UDP_REJECT:
		{
			_naui_net_udp_on_reject(net, packet, size, from);
			return;
		}

		case NET_UDP_DATA:
		case NET_UDP_PING:
		case NET_UDP_BYE:
		{
			_naui_net_udp_on_session_packet(net, packet[3], packet, size, from);
			return;
		}

		default:
			return;
	}
}

static void _naui_net_udp_receive(Naui_Net net)
{
	for (int i = 0; i < NET_UDP_MAX_PACKETS; i++)
	{
		if (_naui_net_events_full(net))
			return;

		size_t size = 0;
		Naui_Address from;
		if (naui_socket_recvfrom(&net->udp, net->udp_rx, NET_UDP_RECV_SIZE, &size, &from) != NAUI_NET_OK)
			return;

		_naui_net_udp_handle_packet(net, net->udp_rx, size, &from);
	}
}

static bool _naui_net_udp_ensure_socket(Naui_Net net)
{
	if (naui_socket_is_valid(&net->udp))
		return true;

	Naui_AddressFamily families[2] =
	{
		NAUI_ADDRESS_IPV6,
		NAUI_ADDRESS_IPV4,
	};

	for (int i = 0; i < 2; i++)
	{
		Naui_Address address = naui_address_any(families[i], 0);
		if (naui_socket_open(&net->udp, NAUI_SOCKET_UDP, families[i]) && naui_socket_bind(&net->udp, &address, false))
			return true;

		naui_socket_close(&net->udp);
	}

	return false;
}

static void _naui_net_udp_close(Naui_Net net, NetConnection* connection, bool notify_peer)
{
	if (notify_peer && connection->session_id != 0)
		_naui_net_udp_send_session(net, connection, NET_UDP_BYE, NULL, 0);
}

static bool _naui_net_udp_listen(Naui_Net net, const Naui_Address* address)
{
	if (naui_socket_is_valid(&net->udp))
	{
		naui_log(NAUI_LOG_ERROR, "networking: UDP is already open. Call listen() before the first UDP connect()");
		return false;
	}

	if (naui_socket_open(&net->udp, NAUI_SOCKET_UDP, address->family) && naui_socket_bind(&net->udp, address, false))
	{
		net->udp_listening = true;
		return true;
	}

	naui_socket_close(&net->udp);
	return false;
}

static Naui_Connection _naui_net_udp_connect(Naui_Net net, const Naui_Address* address)
{
	if (!_naui_net_udp_ensure_socket(net))
		return NAUI_CONNECTION_INVALID;

	if (net->udp.family == NAUI_ADDRESS_IPV4 && address->family == NAUI_ADDRESS_IPV6)
		return NAUI_CONNECTION_INVALID;

	NetConnection* connection = _naui_net_new_connection(net, NAUI_TRANSPORT_UDP, false, address);
	if (!connection)
		return NAUI_CONNECTION_INVALID;

	connection->client_token = _naui_net_random64();
	_naui_net_udp_send_connect(net, connection);
	return (Naui_Connection){ connection->id };
}

static bool _naui_net_udp_send(Naui_Net net, NetConnection* connection, const void* data, size_t size, Naui_SendMode mode)
{
	if (mode == NAUI_SEND_RELIABLE || size > NAUI_UDP_MAX_MESSAGE_SIZE)
		return false;

	_naui_net_udp_send_session(net, connection, NET_UDP_DATA, data, size);
	return true;
}

static void _naui_net_udp_keepalive(Naui_Net net, NetConnection* connection)
{
	_naui_net_udp_send_session(net, connection, NET_UDP_PING, NULL, 0);
}

static void _naui_net_udp_collect(Naui_Net net, bool reading)
{
	if (reading && naui_socket_is_valid(&net->udp))
		_naui_net_poll_add(net, &net->udp, NAUI_POLL_READ, NET_SLOT_UDP, NULL);
}
