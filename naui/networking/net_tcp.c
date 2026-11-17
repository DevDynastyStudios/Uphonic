#define NET_TCP_READ_CHUNK 16384
#define NET_TCP_MAX_READS 64
#define NET_TCP_MAX_ACCEPTS 32
#define NET_TCP_BACKLOG 32

static bool net_tcp_try_flush(NetConnection* connection)
{
	while (net_buffer_len(&connection->tx) > 0)
	{
		size_t sent = 0;
		Naui_NetResult result = naui_socket_send(&connection->sock, net_buffer_ptr(&connection->tx), net_buffer_len(&connection->tx), &sent);
		if (result == NAUI_NET_WOULD_BLOCK)
			return true;

		if (result != NAUI_NET_OK)
			return false;

		if (sent == 0)
			return true;

		net_buffer_consume(&connection->tx, sent);
		connection->bytes_sent += sent;
	}

	return true;
}

static bool net_tcp_send_frame(Naui_Net net, NetConnection* connection, uint8_t type, const void* payload, size_t size)
{
	if (!net_frame_append(net, connection, &connection->tx, type, payload, size))
		return false;

	connection->last_send_ms = net->now_ms;
	if (!net_tcp_try_flush(connection))
	{
		net_close_connection(net, connection, NAUI_DISCONNECT_ERROR, false);
		return false;
	}

	return true;
}

static void net_tcp_send_hello(Naui_Net net, NetConnection* connection)
{
	uint8_t hello[NET_HELLO_MAX];
	size_t size = net_hello_encode(&net->local, hello);
	net_tcp_send_frame(net, connection, NET_FRAME_HELLO, hello, size);
}

static void net_tcp_read(Naui_Net net, NetConnection* connection)
{
	uint8_t chunk[NET_TCP_READ_CHUNK];
	for (int i = 0; i < NET_TCP_MAX_READS; i++)
	{
		size_t count = 0;
		Naui_NetResult result = naui_socket_recv(&connection->sock, chunk, sizeof(chunk), &count);
		if (result == NAUI_NET_OK)
		{
			if (!net_buffer_append(&connection->rx, chunk, count))
			{
				net_close_connection(net, connection, NAUI_DISCONNECT_ERROR, false);
				return;
			}

			connection->bytes_received += count;
			connection->last_recv_ms = net->now_ms;
			if (count < sizeof(chunk))
				break;

			continue;
		}

		if (result == NAUI_NET_CLOSED)
		{
			connection->tcp_eof = true;
			break;
		}

		if (result != NAUI_NET_WOULD_BLOCK)
		{
			net_close_connection(net, connection, NAUI_DISCONNECT_ERROR, false);
			return;
		}

		break;
	}
}

static void net_tcp_accept(Naui_Net net)
{
	for (int i = 0; i < NET_TCP_MAX_ACCEPTS; i++)
	{
		Naui_Socket client;
		Naui_Address peer;
		if (naui_socket_accept(&net->tcp_listener, &client, &peer) != NAUI_NET_OK)
			return;

		if (net_count_live(net) >= net->config.max_connections || net_count_pending(net) >= NET_MAX_PENDING)
		{
			naui_socket_close(&client);
			continue;
		}

		NetConnection* connection = net_new_connection(net, NAUI_TRANSPORT_TCP, true, &peer);
		if (!connection)
		{
			naui_socket_close(&client);
			continue;
		}

		connection->sock = client;
		connection->phase = NET_PHASE_HANDSHAKE;
		net_tcp_send_hello(net, connection);
	}
}

static void net_tcp_connect_finished(Naui_Net net, NetConnection* connection)
{
	if (naui_socket_connect_finish(&connection->sock) != NAUI_NET_OK)
	{
		net_close_connection(net, connection, NAUI_DISCONNECT_ERROR, false);
		return;
	}

	connection->phase = NET_PHASE_HANDSHAKE;
	connection->last_recv_ms = net->now_ms;
	net_tcp_send_hello(net, connection);
}

static void net_tcp_close(NetConnection* connection, bool flush)
{
	if (flush && connection->phase != NET_PHASE_CONNECTING)
		net_tcp_try_flush(connection);

	naui_socket_close(&connection->sock);
}

static bool net_tcp_listen(Naui_Net net, const Naui_Address* address)
{
	if (naui_socket_is_valid(&net->tcp_listener))
		return false;

	if (naui_socket_open(&net->tcp_listener, NAUI_SOCKET_TCP, address->family) &&
		naui_socket_bind(&net->tcp_listener, address, true) &&
		naui_socket_listen(&net->tcp_listener, NET_TCP_BACKLOG))
		return true;

	naui_socket_close(&net->tcp_listener);
	return false;
}

static Naui_Connection net_tcp_connect(Naui_Net net, const Naui_Address* address)
{
	Naui_Socket sock;
	if (!naui_socket_open(&sock, NAUI_SOCKET_TCP, address->family))
		return NAUI_CONNECTION_INVALID;

	Naui_NetResult result = naui_socket_connect(&sock, address);
	NetConnection* connection = result == NAUI_NET_ERROR ? NULL : net_new_connection(net, NAUI_TRANSPORT_TCP, false, address);
	if (!connection)
	{
		naui_socket_close(&sock);
		return NAUI_CONNECTION_INVALID;
	}

	connection->sock = sock;
	if (result == NAUI_NET_OK)
	{
		connection->phase = NET_PHASE_HANDSHAKE;
		net_tcp_send_hello(net, connection);
	}

	return (Naui_Connection){ connection->id };
}

static bool net_tcp_send(Naui_Net net, NetConnection* connection, const void* data, size_t size)
{
	if (size > net->config.max_message_size)
		return false;

	return net_tcp_send_frame(net, connection, NET_FRAME_DATA, data, size);
}

static void net_tcp_keepalive(Naui_Net net, NetConnection* connection)
{
	net_tcp_send_frame(net, connection, NET_FRAME_PING, NULL, 0);
}

static void net_tcp_collect(Naui_Net net, bool reading)
{
	if (reading && naui_socket_is_valid(&net->tcp_listener))
		net_poll_add(net, &net->tcp_listener, NAUI_POLL_READ, NET_SLOT_TCP_LISTENER, NULL);

	for (ptrdiff_t i = 0; i < naui_list_len(net->connections); i++)
	{
		NetConnection* connection = net->connections[i];
		if (connection->transport != NAUI_TRANSPORT_TCP || connection->phase == NET_PHASE_CLOSED)
			continue;

		Naui_PollEvents want = 0;
		if (reading && !connection->tcp_eof && connection->phase != NET_PHASE_CONNECTING)
			want |= NAUI_POLL_READ;

		if (connection->phase == NET_PHASE_CONNECTING || net_buffer_len(&connection->tx) > 0)
			want |= NAUI_POLL_WRITE;

		if (want)
			net_poll_add(net, &connection->sock, want, NET_SLOT_TCP_CONN, connection);
	}
}

static void net_tcp_on_poll(Naui_Net net, NetConnection* connection, Naui_PollEvents got)
{
	if (connection->phase == NET_PHASE_CLOSED)
		return;

	if (connection->phase == NET_PHASE_CONNECTING)
	{
		if (got & (NAUI_POLL_WRITE | NAUI_POLL_ERROR))
			net_tcp_connect_finished(net, connection);

		return;
	}

	if (got & NAUI_POLL_WRITE && !net_tcp_try_flush(connection))
	{
		net_close_connection(net, connection, NAUI_DISCONNECT_ERROR, false);
		return;
	}

	if (got & (NAUI_POLL_READ | NAUI_POLL_ERROR))
		net_tcp_read(net, connection);
}

static void net_tcp_process(Naui_Net net)
{
	for (ptrdiff_t i = 0; i < naui_list_len(net->connections); i++)
	{
		NetConnection* connection = net->connections[i];
		if (connection->transport != NAUI_TRANSPORT_TCP)
			continue;

		if (connection->phase == NET_PHASE_CLOSED || connection->phase == NET_PHASE_CONNECTING)
			continue;

		if (net_buffer_len(&connection->rx) > 0)
			net_frame_parse(net, connection);

		if (connection->phase != NET_PHASE_CLOSED && connection->tcp_eof && !net_events_full(net))
			net_close_connection(net, connection, NAUI_DISCONNECT_REMOTE, false);
	}
}
