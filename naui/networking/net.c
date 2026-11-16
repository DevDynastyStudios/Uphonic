#define NET_DEFAULT_MAX_CONNECTIONS 64
#define NET_DEFAULT_MAX_MESSAGE_SIZE (1u << 20)
#define NET_LIMIT_MAX_MESSAGE_SIZE (64u << 20)

#define NET_DEFAULT_MAX_DEVICES 1024

#define NET_DEFAULT_HANDSHAKE_MS 5000
#define NET_DEFAULT_IDLE_MS 15000
#define NET_DEFAULT_KEEPALIVE_MS 3000

#define NET_MAX_WAIT_MS 100

#define NET_MAX_PENDING 32
#define NET_MAX_QUEUED_EVENTS 16384
#define NET_MAX_QUEUED_BYTES (64u << 20)

#define NET_MIN_SEND_BUFFER (8u << 20)

#define NET_TCP_READ_CHUNK 16384
#define NET_TCP_MAX_READS 64
#define NET_TCP_MAX_ACCEPTS 32
#define NET_TCP_BACKLOG 32

#define NET_UDP_RECV_SIZE 65536
#define NET_UDP_MAX_PACKETS 512
#define NET_UDP_CONNECT_RETRY_MS 500

#define NET_UDP_HEADER_SIZE 4
#define NET_UDP_PACKET_MAX 1200
#define NET_UDP_CONNECT_SIZE 256

#define NET_HELLO_VERSION 1
#define NET_HELLO_FIXED 36
#define NET_HELLO_NAME_MAX 63
#define NET_HELLO_MAX (NET_HELLO_FIXED + NET_HELLO_NAME_MAX)

enum
{
	NET_FRAME_HELLO = 1,
	NET_FRAME_DATA = 2,
	NET_FRAME_PING = 3,
};

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

typedef struct
{
	uint8_t* data;
	size_t head;
	size_t size;
	size_t cap;
} NetBuffer;

typedef struct
{
	Naui_DeviceId id;
	uint8_t platform;
	uint8_t arch;
	char name[NAUI_DEVICE_NAME_MAX];
} NetHello;

typedef uint8_t NetPhase;
enum
{
	NET_PHASE_CONNECTING,
	NET_PHASE_HANDSHAKE,
	NET_PHASE_ESTABLISHED,
	NET_PHASE_CLOSED,
};

typedef struct NetConnection
{
	uint32_t id;
	Naui_Transport transport;
	NetPhase phase;

	Naui_Address remote;
	Naui_Device* device;

	uint64_t created_ms;
	uint64_t last_recv_ms;
	uint64_t last_send_ms;

	uint64_t bytes_sent;
	uint64_t bytes_received;

	Naui_Socket sock;
	NetBuffer rx;
	NetBuffer tx;

	uint64_t client_token;
	uint64_t session_id;
	uint64_t last_connect_ms;

	NetHello pending_hello;
	bool incoming;
	bool tcp_eof;
} NetConnection;

typedef uint8_t NetSlotKind;
enum
{
	NET_SLOT_TCP_LISTENER,
	NET_SLOT_UDP,
	NET_SLOT_TCP_CONN,
};

typedef struct
{
	NetSlotKind kind;
	NetConnection* conn;
} NetPollSlot;

struct Naui_NetImpl
{
	Naui_NetConfig config;
	size_t send_buffer_limit;
	Naui_Device local;

	Naui_List(Naui_Device*) devices;
	Naui_List(NetConnection*) connections;

	uint32_t next_connection_id;
	Naui_Socket tcp_listener;

	Naui_Socket udp;
	bool udp_listening;
	uint8_t* udp_rx;

	Naui_List(Naui_NetEvent) events;
	size_t events_head;
	size_t queued_bytes;

	void* current_payload;
	Naui_List(Naui_PollItem) poll_items;
	Naui_List(NetPollSlot) poll_slots;
	uint64_t now_ms;
};

static void net_put_u32(uint8_t* p, uint32_t value)
{
	p[0] = (uint8_t)(value >> 24);
	p[1] = (uint8_t)(value >> 16);
	p[2] = (uint8_t)(value >> 8);
	p[3] = (uint8_t)value;
}

static uint32_t net_get_u32(const uint8_t* p)
{
	return
		((uint32_t)p[0] << 24) |
		((uint32_t)p[1] << 16) |
		((uint32_t)p[2] << 8) |
		(uint32_t)p[3];
}

static void net_put_u64(uint8_t* p, uint64_t value)
{
	net_put_u32(p, (uint32_t)(value >> 32));
	net_put_u32(p + 4, (uint32_t)value);
}

static uint64_t net_get_u64(const uint8_t* p)
{
	return ((uint64_t)net_get_u32(p) << 32) | (uint64_t)net_get_u32(p + 4);
}

static uint64_t net_now_ms(void)
{
#if NAUI_WINDOWS

	return (uint64_t)GetTickCount64();

#else

	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t)ts.tv_sec * 1000u + (uint64_t)ts.tv_nsec / 1000000u;

#endif
}

static uint64_t net_random64(void)
{
	return naui_uuid_generate();
}

static size_t net_buffer_len(const NetBuffer* buffer)
{
	return buffer->size - buffer->head;
}

static uint8_t* net_buffer_ptr(NetBuffer* buffer)
{
	return buffer->data + buffer->head;
}

static void net_buffer_consume(NetBuffer* buffer, size_t size)
{
	buffer->head += size;

	if (buffer->head >= buffer->size)
	{
		buffer->head = 0;
		buffer->size = 0;
	}
}

static bool net_buffer_append(NetBuffer* buffer, const void* data, size_t size)
{
	if (size == 0)
		return true;

	if (buffer->head > 0 && buffer->cap - buffer->size < size)
	{
		memmove(
			buffer->data,
			buffer->data + buffer->head,
			buffer->size - buffer->head);

		buffer->size -= buffer->head;
		buffer->head = 0;
	}

	if (buffer->cap - buffer->size < size)
	{
		if (size > SIZE_MAX / 2 - buffer->size)
			return false;

		size_t cap = buffer->cap ? buffer->cap : 1024;

		while (cap < buffer->size + size)
			cap *= 2;

		uint8_t* grown = (uint8_t*)realloc(buffer->data, cap);

		if (!grown)
			return false;

		buffer->data = grown;
		buffer->cap = cap;
	}

	memcpy(buffer->data + buffer->size, data, size);
	buffer->size += size;

	return true;
}

static void net_buffer_free(NetBuffer* buffer)
{
	free(buffer->data);
	memset(buffer, 0, sizeof(*buffer));
}

static void net_tick(Naui_Net net)
{
	net->now_ms = net_now_ms();
}

static size_t net_event_backlog(const Naui_Net net)
{
	return (size_t)naui_list_len(net->events) - net->events_head;
}

static bool net_events_full(const Naui_Net net)
{
	return
		net_event_backlog(net) >= NET_MAX_QUEUED_EVENTS ||
		net->queued_bytes >= NET_MAX_QUEUED_BYTES;
}

static void net_push_event(Naui_Net net, Naui_NetEventType type, const NetConnection* connection, Naui_DisconnectReason reason, bool reliable, const void* data, size_t size)
{
	Naui_NetEvent event;

	memset(&event, 0, sizeof(event));

	event.type = type;
	event.connection._id = connection->id;
	event.transport = connection->transport;
	event.address = connection->remote;
	event.reason = reason;
	event.device = type == NAUI_NET_EVENT_CONNECT_FAILED ? NULL : connection->device;
	event.reliable = reliable;

	if (type == NAUI_NET_EVENT_MESSAGE)
	{
		void* copy = malloc(size ? size : 1);

		if (!copy)
			return;

		if (size)
			memcpy(copy, data, size);

		event.data = copy;
		event.size = size;

		net->queued_bytes += size;
	}

	naui_list_push(net->events, event);
}

static Naui_Device* net_find_device(Naui_Net net, Naui_DeviceId id)
{
	for (ptrdiff_t i = 0; i < naui_list_len(net->devices); i++)
	{
		if (naui_device_id_equal(net->devices[i]->id, id))
			return net->devices[i];
	}

	return NULL;
}

static void net_device_note_endpoint(Naui_Device* device, const Naui_Address* address, Naui_Transport transport, bool incoming, uint64_t unix_now)
{
	Naui_DeviceEndpoint endpoint;
	memset(&endpoint, 0, sizeof(endpoint));
	endpoint.address = *address;
	endpoint.transport = transport;
	endpoint.incoming = incoming;
	endpoint.last_seen = unix_now;

	if (incoming && transport == NAUI_TRANSPORT_TCP)
		endpoint.address.port = 0;

	uint32_t oldest = 0;
	for (uint32_t i = 0; i < device->endpoint_count; i++)
	{
		Naui_DeviceEndpoint* existing = &device->endpoints[i];
		if (existing->transport == transport && existing->incoming == incoming && naui_address_equal(&existing->address, &endpoint.address))
		{
			existing->last_seen = unix_now;
			return;
		}

		if (existing->last_seen < device->endpoints[oldest].last_seen)
			oldest = i;
	}

	if (device->endpoint_count < NAUI_DEVICE_MAX_ENDPOINTS)
	{
		device->endpoints[device->endpoint_count] = endpoint;
		device->endpoint_count++;
		return;
	}

	device->endpoints[oldest] = endpoint;
}

static Naui_Device* net_device_from_hello(Naui_Net net, const NetHello* hello)
{
	uint64_t unix_now = naui_unix_time();
	Naui_Device* device = net_find_device(net, hello->id);
	if (!device)
	{
		device = (Naui_Device*)calloc(1, sizeof(*device));
		if (!device)
			return NULL;

		device->id = hello->id;
		device->trust = NAUI_DEVICE_TRUST_UNVERIFIED;
		device->first_seen = unix_now;
		naui_list_push(net->devices, device);
	}

	device->platform = hello->platform;
	device->arch = hello->arch;
	memcpy(device->name, hello->name, sizeof(device->name));
	device->last_seen = unix_now;
	return device;
}

static void net_evict_devices(Naui_Net net)
{
	if (net_event_backlog(net) != 0)
		return;

	while ((size_t)naui_list_len(net->devices) > net->config.max_devices)
	{
		ptrdiff_t victim = -1;
		for (ptrdiff_t i = 0; i < naui_list_len(net->devices); i++)
		{
			const Naui_Device* device = net->devices[i];
			if (device->connection_count != 0)
				continue;

			if (victim < 0 || device->last_seen < net->devices[victim]->last_seen)
				victim = i;
		}

		if (victim < 0)
			return;

		free(net->devices[victim]);
		naui_list_remove(net->devices, victim);
	}
}

static size_t net_hello_encode(const Naui_Device* local, uint8_t* out)
{
	size_t name_len = strlen(local->name);
	if (name_len > NET_HELLO_NAME_MAX)
		name_len = NET_HELLO_NAME_MAX;

	out[0] = NET_HELLO_VERSION;
	out[1] = local->platform;
	out[2] = local->arch;
	out[3] = (uint8_t)name_len;

	memcpy(out + 4, local->id.bytes, NAUI_DEVICE_ID_SIZE);
	memcpy(out + NET_HELLO_FIXED, local->name, name_len);
	return NET_HELLO_FIXED + name_len;
}

static bool net_hello_decode(const uint8_t* data, size_t size, NetHello* hello)
{
	if (size < NET_HELLO_FIXED || data[0] != NET_HELLO_VERSION)
		return false;

	size_t name_len = data[3];
	if (name_len > NET_HELLO_NAME_MAX)
		return false;

	if (NET_HELLO_FIXED + name_len > size)
		return false;

	memset(hello, 0, sizeof(*hello));
	memcpy(hello->id.bytes, data + 4, NAUI_DEVICE_ID_SIZE);

	if (naui_device_id_is_null(hello->id))
		return false;

	hello->platform = data[1];
	hello->arch = data[2];
	for (size_t i = 0; i < name_len; i++)
	{
		uint8_t c = data[NET_HELLO_FIXED + i];
		hello->name[i] = (c < 0x20 || c == 0x7f) ? '?' : (char)c;
	}

	return true;
}

static NetConnection* net_new_connection(Naui_Net net, Naui_Transport transport, bool incoming, const Naui_Address* remote)
{
	NetConnection* connection = (NetConnection*)calloc(1, sizeof(*connection));
	if (!connection)
		return NULL;

	do
	{
		connection->id = ++net->next_connection_id;
	}
	while (connection->id == 0);

	connection->transport = transport;
	connection->incoming = incoming;
	connection->remote = *remote;
	connection->sock = NAUI_SOCKET_NULL;
	connection->created_ms = net->now_ms;
	connection->last_recv_ms = net->now_ms;
	connection->last_send_ms = net->now_ms;
	naui_list_push(net->connections, connection);
	return connection;
}

static NetConnection* net_find_connection(Naui_Net net, Naui_Connection handle)
{
	if (handle._id == 0)
		return NULL;

	for (ptrdiff_t i = 0; i < naui_list_len(net->connections); i++)
	{
		NetConnection* connection = net->connections[i];
		if (connection->id != handle._id)
			continue;

		return connection->phase == NET_PHASE_CLOSED ? NULL : connection;
	}

	return NULL;
}

static size_t net_count_live(const Naui_Net net)
{
	size_t count = 0;
	for (ptrdiff_t i = 0; i < naui_list_len(net->connections); i++)
	{
		count += net->connections[i]->phase != NET_PHASE_CLOSED;
	}

	return count;
}

static size_t net_count_pending(const Naui_Net net)
{
	size_t count = 0;
	for (ptrdiff_t i = 0; i < naui_list_len(net->connections); i++)
	{
		const NetConnection* connection = net->connections[i];
		count += connection->incoming && connection->phase != NET_PHASE_CLOSED && connection->phase != NET_PHASE_ESTABLISHED;
	}

	return count;
}

static void net_sweep(Naui_Net net)
{
	for (ptrdiff_t i = naui_list_len(net->connections) - 1; i >= 0; --i)
	{
		NetConnection* connection = net->connections[i];
		if (connection->phase != NET_PHASE_CLOSED)
			continue;

		net_buffer_free(&connection->rx);
		net_buffer_free(&connection->tx);
		free(connection);
		naui_list_remove(net->connections, i);
	}
}

static bool net_tcp_try_flush(NetConnection* connection);

static void net_udp_send_session(Naui_Net net, NetConnection* connection, uint8_t type, const void* payload, size_t size);

static void net_close_connection(Naui_Net net, NetConnection* connection, Naui_DisconnectReason reason, bool notify_peer)
{
	if (connection->phase == NET_PHASE_CLOSED)
		return;

	bool was_established = connection->phase == NET_PHASE_ESTABLISHED;
	if (notify_peer)
	{
		if (connection->transport == NAUI_TRANSPORT_UDP && connection->session_id != 0)
			net_udp_send_session(net, connection, NET_UDP_BYE, NULL, 0);
		else if (connection->transport == NAUI_TRANSPORT_TCP && connection->phase != NET_PHASE_CONNECTING)
			net_tcp_try_flush(connection);
	}

	if (connection->transport == NAUI_TRANSPORT_TCP)
		naui_socket_close(&connection->sock);

	connection->phase = NET_PHASE_CLOSED;
	if (connection->device && connection->device->connection_count > 0)
		connection->device->connection_count--;

	if (was_established)
	{
		net_push_event(net, NAUI_NET_EVENT_DISCONNECTED, connection, reason, false, NULL, 0);
		return;
	}

	if (!connection->incoming)
		net_push_event(net, NAUI_NET_EVENT_CONNECT_FAILED, connection, reason, false, NULL, 0);
}

static void net_complete_handshake(Naui_Net net, NetConnection* connection, const NetHello* hello)
{
	Naui_Device* device = net_device_from_hello(net, hello);
	if (!device)
	{
		net_close_connection(net, connection, NAUI_DISCONNECT_ERROR, true);
		return;
	}

	connection->device = device;
	device->connection_count++;
	net_device_note_endpoint(device, &connection->remote, connection->transport, connection->incoming, naui_unix_time());

	connection->phase = NET_PHASE_ESTABLISHED;
	net_push_event(net, NAUI_NET_EVENT_CONNECTED, connection, NAUI_DISCONNECT_LOCAL, false, NULL, 0);
}

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
	size_t pending = net_buffer_len(&connection->tx);
	size_t frame = 5 + size;
	if (pending > net->send_buffer_limit)
		return false;

	if (frame > net->send_buffer_limit - pending)
		return false;

	uint8_t header[5];
	net_put_u32(header, (uint32_t)(1 + size));
	header[4] = type;
	if (!net_buffer_append(&connection->tx, header, sizeof(header)))
	{
		net_close_connection(net, connection, NAUI_DISCONNECT_ERROR, false);
		return false;
	}

	if (!net_buffer_append(&connection->tx, payload, size))
	{
		net_close_connection(net, connection, NAUI_DISCONNECT_ERROR, false);
		return false;
	}

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

static void net_tcp_handle_frame(Naui_Net net, NetConnection* connection, uint8_t type, const uint8_t* payload, size_t size)
{
	switch (type)
	{
		case NET_FRAME_HELLO:
		{
			NetHello hello;
			if (connection->phase != NET_PHASE_HANDSHAKE || !net_hello_decode(payload, size, &hello))
			{
				net_close_connection(net, connection, NAUI_DISCONNECT_PROTOCOL, false);
				return;
			}

			if (naui_device_id_equal(hello.id, net->local.id))
			{
				net_close_connection(net, connection, NAUI_DISCONNECT_SELF, true);
				return;
			}

			net_complete_handshake(net, connection, &hello);
			return;
		}

		case NET_FRAME_DATA:
		{
			if (connection->phase != NET_PHASE_ESTABLISHED)
			{
				net_close_connection(net, connection, NAUI_DISCONNECT_PROTOCOL, false);
				return;
			}

			net_push_event(net, NAUI_NET_EVENT_MESSAGE, connection, NAUI_DISCONNECT_LOCAL, true, payload, size);
			return;
		}

		case NET_FRAME_PING:
		{
			if (connection->phase != NET_PHASE_ESTABLISHED)
				net_close_connection(net, connection, NAUI_DISCONNECT_PROTOCOL, false);

			return;
		}

		default:
		{
			if (connection->phase != NET_PHASE_ESTABLISHED)
				net_close_connection(net, connection, NAUI_DISCONNECT_PROTOCOL, false);

			return;
		}
	}
}

static void net_tcp_parse(Naui_Net net, NetConnection* connection)
{
	while (connection->phase != NET_PHASE_CLOSED)
	{
		if (net_events_full(net))
			break;

		size_t available = net_buffer_len(&connection->rx);

		if (available < 4)
			break;

		const uint8_t* data = net_buffer_ptr(&connection->rx);
		uint32_t length = net_get_u32(data);
		size_t limit = connection->phase == NET_PHASE_ESTABLISHED ? (size_t)net->config.max_message_size + 1 : NET_HELLO_MAX + 1;
		if (length < 1 || length > limit)
		{
			net_close_connection(net, connection, NAUI_DISCONNECT_PROTOCOL, false);
			return;
		}

		if (available - 4 < length)
			break;

		net_tcp_handle_frame(net, connection, data[4], data + 5, length - 1);
		if (connection->phase == NET_PHASE_CLOSED)
			return;

		net_buffer_consume(&connection->rx, 4 + (size_t)length);
	}
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

static void net_udp_header(uint8_t* p, uint8_t type)
{
	p[0] = 'N';
	p[1] = 'U';
	p[2] = 1;
	p[3] = type;
}

static void net_udp_send_raw(Naui_Net net, const Naui_Address* to, const void* data, size_t size)
{
	if (!naui_socket_is_valid(&net->udp))
		return;

	size_t sent = 0;
	naui_socket_sendto(&net->udp, data, size, to, &sent);
}

static void net_udp_send_session(Naui_Net net, NetConnection* connection, uint8_t type, const void* payload, size_t size)
{
	uint8_t packet[NET_UDP_PACKET_MAX];
	if (NET_UDP_HEADER_SIZE + 8 + size > sizeof(packet))
		return;

	net_udp_header(packet, type);
	net_put_u64(packet + 4, connection->session_id);

	if (size)
		memcpy(packet + 12, payload, size);

	size_t total = 12 + size;
	net_udp_send_raw(net, &connection->remote, packet, total);
	connection->bytes_sent += total;
	connection->last_send_ms = net->now_ms;
}

static void net_udp_send_connect(Naui_Net net, NetConnection* connection)
{
	uint8_t packet[NET_UDP_CONNECT_SIZE];
	memset(packet, 0, sizeof(packet));

	net_udp_header(packet, NET_UDP_CONNECT);
	net_put_u64(packet + 4, connection->client_token);
	net_hello_encode(&net->local, packet + 12);
	net_udp_send_raw(net, &connection->remote, packet, sizeof(packet));

	connection->last_connect_ms = net->now_ms;
	connection->last_send_ms = net->now_ms;
}

static void net_udp_send_accept(Naui_Net net, NetConnection* connection)
{
	uint8_t packet[NET_UDP_HEADER_SIZE + 16 + NET_HELLO_MAX];

	net_udp_header(packet, NET_UDP_ACCEPT);
	net_put_u64(packet + 4, connection->client_token);
	net_put_u64(packet + 12, connection->session_id);

	size_t hello_size = net_hello_encode(&net->local, packet + 20);
	net_udp_send_raw(net, &connection->remote, packet, 20 + hello_size);
	connection->last_send_ms = net->now_ms;
}

static void net_udp_send_reject(Naui_Net net, const Naui_Address* address, uint64_t token, uint8_t reason)
{
	uint8_t packet[13];
	net_udp_header(packet, NET_UDP_REJECT);
	net_put_u64(packet + 4, token);
	packet[12] = reason;
	net_udp_send_raw(net, address, packet, sizeof(packet));
}

static NetConnection* net_udp_find_server_session(Naui_Net net, const Naui_Address* from)
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

		if (!naui_address_equal(&connection->remote, from))
			continue;

		return connection;
	}

	return NULL;
}

static NetConnection* net_udp_find_connecting(Naui_Net net, const Naui_Address* from, uint64_t token)
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

static NetConnection* net_udp_find_session(Naui_Net net, const Naui_Address* from, uint64_t session_id)
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

static void net_udp_on_connect(Naui_Net net, const uint8_t* packet, size_t size, const Naui_Address* from)
{
	if (!net->udp_listening)
		return;

	if (size < NET_UDP_CONNECT_SIZE)
		return;

	uint64_t token = net_get_u64(packet + 4);
	NetHello hello;

	if (token == 0)
		return;

	if (!net_hello_decode(packet + 12, size - 12, &hello))
		return;

	NetConnection* existing = net_udp_find_server_session(net, from);
	if (existing)
	{
		if (existing->client_token == token)
		{
			existing->last_recv_ms = net->now_ms;
			net_udp_send_accept(net, existing);
			return;
		}

		net_close_connection(net, existing, NAUI_DISCONNECT_REMOTE, false);
	}

	if (naui_device_id_equal(hello.id, net->local.id))
	{
		net_udp_send_reject(net, from, token, NET_REJECT_SELF);
		return;
	}

	if (net_count_pending(net) >= NET_MAX_PENDING)
		return;

	if (net_count_live(net) >= net->config.max_connections)
	{
		net_udp_send_reject(net, from, token, NET_REJECT_FULL);
		return;
	}

	NetConnection* connection = net_new_connection(net, NAUI_TRANSPORT_UDP, true, from);
	if (!connection)
		return;

	connection->phase = NET_PHASE_HANDSHAKE;
	connection->client_token = token;
	connection->session_id = net_random64();
	connection->pending_hello = hello;
	net_udp_send_accept(net, connection);
}

static void net_udp_on_accept(Naui_Net net, const uint8_t* packet, size_t size, const Naui_Address* from)
{
	if (size < 20 + NET_HELLO_FIXED)
		return;

	NetConnection* connection = net_udp_find_connecting(net, from, net_get_u64(packet + 4));
	if (!connection)
		return;

	uint64_t session = net_get_u64(packet + 12);
	NetHello hello;

	if (session == 0 || !net_hello_decode(packet + 20, size - 20, &hello))
	{
		connection->session_id = session;
		net_close_connection(net, connection, NAUI_DISCONNECT_PROTOCOL, session != 0);
		return;
	}

	connection->session_id = session;
	connection->last_recv_ms = net->now_ms;
	connection->bytes_received += size;
	if (naui_device_id_equal(hello.id, net->local.id))
	{
		net_close_connection(net, connection, NAUI_DISCONNECT_SELF, true);
		return;
	}

	net_complete_handshake(net, connection, &hello);
	if (connection->phase == NET_PHASE_ESTABLISHED)
		net_udp_send_session(net, connection, NET_UDP_PING, NULL, 0);
}

static void net_udp_on_reject(Naui_Net net, const uint8_t* packet, size_t size, const Naui_Address* from)
{
	if (size < 13)
		return;

	NetConnection* connection = net_udp_find_connecting(net, from, net_get_u64(packet + 4));
	if (!connection)
		return;

	net_close_connection(
		net,
		connection,
		packet[12] == NET_REJECT_SELF ? NAUI_DISCONNECT_SELF : NAUI_DISCONNECT_REJECTED,
		false);
}

static void net_udp_on_session_packet(Naui_Net net, uint8_t type, const uint8_t* packet, size_t size, const Naui_Address* from)
{
	if (size < 12)
		return;

	NetConnection* connection = net_udp_find_session(net, from, net_get_u64(packet + 4));
	if (!connection)
		return;

	connection->last_recv_ms = net->now_ms;
	connection->bytes_received += size;

	if (type == NET_UDP_BYE)
	{
		net_close_connection(net, connection, NAUI_DISCONNECT_REMOTE, false);
		return;
	}

	if (connection->phase == NET_PHASE_HANDSHAKE)
	{
		net_complete_handshake(net, connection, &connection->pending_hello);
		if (connection->phase != NET_PHASE_ESTABLISHED)
			return;
	}

	if (type == NET_UDP_DATA && !net_events_full(net))
		net_push_event(net, NAUI_NET_EVENT_MESSAGE, connection, NAUI_DISCONNECT_LOCAL, false, packet + 12, size - 12);
}

static void net_udp_handle_packet(Naui_Net net, const uint8_t* packet, size_t size, const Naui_Address* from)
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
			net_udp_on_connect(net, packet, size, from);
			return;
		}

		case NET_UDP_ACCEPT:
		{
			net_udp_on_accept(net, packet, size, from);
			return;
		}

		case NET_UDP_REJECT:
		{
			net_udp_on_reject(net, packet, size, from);
			return;
		}

		case NET_UDP_DATA:
		case NET_UDP_PING:
		case NET_UDP_BYE:
		{
			net_udp_on_session_packet(net, packet[3], packet, size, from);
			return;
		}

		default:
			return;
	}
}

static void net_udp_receive(Naui_Net net)
{
	for (int i = 0; i < NET_UDP_MAX_PACKETS; i++)
	{
		if (net_events_full(net))
			return;

		size_t size = 0;
		Naui_Address from;
		if (naui_socket_recvfrom(&net->udp, net->udp_rx, NET_UDP_RECV_SIZE, &size, &from) != NAUI_NET_OK)
			return;

		net_udp_handle_packet(net, net->udp_rx, size, &from);
	}
}

static bool net_udp_ensure_socket(Naui_Net net)
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

Naui_Net naui_net_create(const Naui_NetConfig* config)
{
	Naui_Net net = (Naui_Net)calloc(1, sizeof(*net));
	if (!net)
		return NULL;

	if (config)
		net->config = *config;

	Naui_NetConfig* cfg = &net->config;
	if (!cfg->max_connections)
		cfg->max_connections = NET_DEFAULT_MAX_CONNECTIONS;

	if (!cfg->max_message_size)
		cfg->max_message_size = NET_DEFAULT_MAX_MESSAGE_SIZE;

	if (cfg->max_message_size > NET_LIMIT_MAX_MESSAGE_SIZE)
		cfg->max_message_size = NET_LIMIT_MAX_MESSAGE_SIZE;

	if (!cfg->max_devices)
		cfg->max_devices = NET_DEFAULT_MAX_DEVICES;

	if (!cfg->handshake_timeout_ms)
		cfg->handshake_timeout_ms = NET_DEFAULT_HANDSHAKE_MS;

	if (!cfg->idle_timeout_ms)
		cfg->idle_timeout_ms = NET_DEFAULT_IDLE_MS;

	if (!cfg->keepalive_interval_ms)
		cfg->keepalive_interval_ms = NET_DEFAULT_KEEPALIVE_MS;

	uint32_t keepalive_cap = cfg->idle_timeout_ms / 3;
	if (cfg->handshake_timeout_ms / 2 < keepalive_cap)
		keepalive_cap = cfg->handshake_timeout_ms / 2;

	if (keepalive_cap == 0)
		keepalive_cap = 1;

	if (cfg->keepalive_interval_ms > keepalive_cap)
		cfg->keepalive_interval_ms = keepalive_cap;

	cfg->device_name = NULL;
	cfg->device_id = NULL;
	net->send_buffer_limit = NET_MIN_SEND_BUFFER;

	if ((size_t)cfg->max_message_size * 2 > net->send_buffer_limit)
		net->send_buffer_limit = (size_t)cfg->max_message_size * 2;

	net->tcp_listener = NAUI_SOCKET_NULL;
	net->udp = NAUI_SOCKET_NULL;
	net->udp_rx = (uint8_t*)malloc(NET_UDP_RECV_SIZE);

	if (!net->udp_rx || !naui_sockets_init())
	{
		free(net->udp_rx);
		free(net);
		return NULL;
	}

	net->local.id = (config && config->device_id && !naui_device_id_is_null(*config->device_id)) ? *config->device_id : naui_device_id_generate();
	net->local.is_local = true;
	net->local.trust = NAUI_DEVICE_TRUST_VERIFIED;

	net->local.first_seen = naui_unix_time();
	net->local.last_seen = net->local.first_seen;

	naui_device_describe_local(&net->local, config ? config->device_name : NULL);
	net_tick(net);
	return net;
}

void naui_net_destroy(Naui_Net net)
{
	if (!net)
		return;

	net_tick(net);
	for (ptrdiff_t i = 0; i < naui_list_len(net->connections); i++)
	{
		net_close_connection(net, net->connections[i], NAUI_DISCONNECT_LOCAL, true);
	}

	net_sweep(net);

	for (size_t i = net->events_head; i < (size_t)naui_list_len(net->events); i++)
	{
		free((void*)net->events[i].data);
	}

	free(net->current_payload);
	for (ptrdiff_t i = 0; i < naui_list_len(net->devices); i++)
	{
		free(net->devices[i]);
	}

	naui_list_free(net->events);
	naui_list_free(net->devices);
	naui_list_free(net->connections);
	naui_list_free(net->poll_items);
	naui_list_free(net->poll_slots);

	naui_socket_close(&net->tcp_listener);
	naui_socket_close(&net->udp);
	free(net->udp_rx);
	naui_sockets_shutdown();
	free(net);
}

const Naui_Device* naui_net_local_device(Naui_Net net)
{
	return net ? &net->local : NULL;
}

bool naui_net_listen(Naui_Net net, Naui_Transport transport, const Naui_Address* bind_address)
{
	if (!net || !bind_address)
		return false;

	Naui_Address address = *bind_address;
	naui_address_normalize(&address);
	if (address.family != NAUI_ADDRESS_IPV4 && address.family != NAUI_ADDRESS_IPV6)
		return false;

	if (transport == NAUI_TRANSPORT_TCP)
	{
		if (naui_socket_is_valid(&net->tcp_listener))
			return false;

		bool socket_opened = naui_socket_open(&net->tcp_listener, NAUI_SOCKET_TCP, address.family);
		bool socket_binded = naui_socket_bind(&net->tcp_listener, &address, true);
		bool socket_listening = naui_socket_listen(&net->tcp_listener, NET_TCP_BACKLOG);

		if (socket_opened && socket_binded && socket_listening)
			return true;

		naui_socket_close(&net->tcp_listener);
		return false;
	}

	if (transport == NAUI_TRANSPORT_UDP)
	{
		if (naui_socket_is_valid(&net->udp))
		{
			naui_log(NAUI_LOG_ERROR, "networking: UDP is already open. Call listen() before the first UDP connect()");
			return false;
		}

		if (naui_socket_open(&net->udp, NAUI_SOCKET_UDP, address.family) && naui_socket_bind(&net->udp, &address, false))
		{
			net->udp_listening = true;
			return true;
		}

		naui_socket_close(&net->udp);
		return false;
	}

	naui_log(NAUI_LOG_ERROR, "networking: this transport is not implemented (QUIC needs a QUIC/TLS library, see net.h)");
	return false;
}

bool naui_net_listen_address(Naui_Net net, Naui_Transport transport, Naui_Address* out_address)
{
	if (!net || !out_address)
		return false;

	if (transport == NAUI_TRANSPORT_TCP && naui_socket_is_valid(&net->tcp_listener))
	{
		*out_address = net->tcp_listener.address;
		return true;
	}

	if (transport == NAUI_TRANSPORT_UDP && net->udp_listening)
	{
		*out_address = net->udp.address;
		return true;
	}

	return false;
}

Naui_Connection naui_net_connect(Naui_Net net, Naui_Transport transport, const Naui_Address* remote)
{
	if (!net || !remote)
		return NAUI_CONNECTION_INVALID;

	Naui_Address address = *remote;
	naui_address_normalize(&address);

	if ((address.family != NAUI_ADDRESS_IPV4 && address.family != NAUI_ADDRESS_IPV6) || address.port == 0)
		return NAUI_CONNECTION_INVALID;

	net_tick(net);
	if (net_count_live(net) >= net->config.max_connections)
		return NAUI_CONNECTION_INVALID;

	if (transport == NAUI_TRANSPORT_TCP)
	{
		Naui_Socket sock;
		if (!naui_socket_open(&sock, NAUI_SOCKET_TCP, address.family))
			return NAUI_CONNECTION_INVALID;

		Naui_NetResult result = naui_socket_connect(&sock, &address);
		if (result == NAUI_NET_ERROR)
		{
			naui_socket_close(&sock);
			return NAUI_CONNECTION_INVALID;
		}

		NetConnection* connection = net_new_connection(net, NAUI_TRANSPORT_TCP, false, &address);
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

	if (transport == NAUI_TRANSPORT_UDP)
	{
		if (!net_udp_ensure_socket(net))
			return NAUI_CONNECTION_INVALID;

		if (net->udp.family == NAUI_ADDRESS_IPV4 && address.family == NAUI_ADDRESS_IPV6)
			return NAUI_CONNECTION_INVALID;

		NetConnection* connection = net_new_connection(net, NAUI_TRANSPORT_UDP, false, &address);
		if (!connection)
			return NAUI_CONNECTION_INVALID;

		connection->client_token = net_random64();
		net_udp_send_connect(net, connection);
		return (Naui_Connection){ connection->id };
	}

	naui_log(NAUI_LOG_ERROR, "networking: this transport is not implemented (QUIC needs a QUIC/TLS library, see net.h)");
	return NAUI_CONNECTION_INVALID;
}

void naui_net_disconnect(Naui_Net net, Naui_Connection connection)
{
	if (!net)
		return;

	NetConnection* c = net_find_connection(net, connection);
	if (!c)
		return;

	net_tick(net);
	net_close_connection(net, c, NAUI_DISCONNECT_LOCAL, true);
}

bool naui_net_send(Naui_Net net, Naui_Connection connection, const void* data, size_t size, Naui_SendMode mode)
{
	if (!net)
		return false;

	if (!data && size)
		return false;

	NetConnection* c = net_find_connection(net, connection);
	if (!c)
		return false;

	if (c->phase != NET_PHASE_ESTABLISHED)
		return false;

	net_tick(net);
	if (c->transport == NAUI_TRANSPORT_TCP)
	{
		if (size > net->config.max_message_size)
			return false;

		return net_tcp_send_frame(net, c, NET_FRAME_DATA, data, size);
	}

	if (c->transport == NAUI_TRANSPORT_UDP)
	{
		if (mode == NAUI_SEND_RELIABLE)
			return false;

		if (size > NAUI_UDP_MAX_MESSAGE_SIZE)
			return false;

		net_udp_send_session(net, c, NET_UDP_DATA, data, size);
		return true;
	}

	return false;
}

bool naui_net_send_to_device(Naui_Net net, const Naui_Device* device, const void* data, size_t size, Naui_SendMode mode)
{
	if (!net || !device)
		return false;

	NetConnection* best = NULL;
	for (ptrdiff_t i = 0; i < naui_list_len(net->connections); i++)
	{
		NetConnection* connection = net->connections[i];
		if (connection->device != device)
			continue;

		if (connection->phase != NET_PHASE_ESTABLISHED)
			continue;

		if (mode == NAUI_SEND_RELIABLE)
		{
			if (connection->transport == NAUI_TRANSPORT_TCP)
			{
				best = connection;
				break;
			}

			continue;
		}

		if (connection->transport == NAUI_TRANSPORT_UDP)
		{
			best = connection;
			break;
		}

		if (!best)
			best = connection;
	}

	if (!best)
		return false;

	Naui_Connection handle = { best->id };
	return naui_net_send(net, handle, data, size, mode);
}

bool naui_net_connection_info(Naui_Net net, Naui_Connection connection, Naui_ConnectionInfo* out_info)
{
	if (!net || !out_info)
		return false;

	const NetConnection* c = net_find_connection(net, connection);
	if (!c)
		return false;

	memset(out_info, 0, sizeof(*out_info));
	out_info->transport = c->transport;
	out_info->state = c->phase == NET_PHASE_ESTABLISHED ? NAUI_CONNECTION_CONNECTED : NAUI_CONNECTION_CONNECTING;

	out_info->incoming = c->incoming;
	out_info->remote = c->remote;
	out_info->device = c->device;

	out_info->bytes_sent = c->bytes_sent;
	out_info->bytes_received = c->bytes_received;
	return true;
}

static void net_run_timers(Naui_Net net)
{
	const Naui_NetConfig* config = &net->config;

	for (ptrdiff_t i = 0; i < naui_list_len(net->connections); i++)
	{
		NetConnection* connection = net->connections[i];
		if (connection->phase == NET_PHASE_CLOSED)
			continue;

		if (connection->phase != NET_PHASE_ESTABLISHED)
		{
			if (net->now_ms - connection->created_ms >= config->handshake_timeout_ms)
				net_close_connection(net, connection, NAUI_DISCONNECT_TIMEOUT, true);

			else if (connection->transport == NAUI_TRANSPORT_UDP && !connection->incoming && net->now_ms - connection->last_connect_ms >= NET_UDP_CONNECT_RETRY_MS)
				net_udp_send_connect(net, connection);

			continue;
		}

		if (net->now_ms - connection->last_recv_ms >= config->idle_timeout_ms)
		{
			net_close_connection(net, connection, NAUI_DISCONNECT_TIMEOUT, true);
			continue;
		}

		if (net->now_ms - connection->last_send_ms < config->keepalive_interval_ms)
			continue;

		if (connection->transport == NAUI_TRANSPORT_UDP)
			net_udp_send_session(net, connection, NET_UDP_PING, NULL, 0);
		else
			net_tcp_send_frame(net, connection, NET_FRAME_PING, NULL, 0);
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

void naui_net_update(Naui_Net net, uint32_t timeout_ms)
{
	if (!net)
		return;

	free(net->current_payload);
	net->current_payload = NULL;
	net_evict_devices(net);
	bool reading = !net_events_full(net);

	naui_list_clear(net->poll_items);
	naui_list_clear(net->poll_slots);

	if (naui_socket_is_valid(&net->tcp_listener) && reading)
	{
		Naui_PollItem item = { &net->tcp_listener, NAUI_POLL_READ, 0 };
		NetPollSlot slot = { NET_SLOT_TCP_LISTENER, NULL };
		naui_list_push(net->poll_items, item);
		naui_list_push(net->poll_slots, slot);
	}

	if (naui_socket_is_valid(&net->udp) && reading)
	{
		Naui_PollItem item = { &net->udp, NAUI_POLL_READ, 0 };
		NetPollSlot slot = { NET_SLOT_UDP, NULL };
		naui_list_push(net->poll_items, item);
		naui_list_push(net->poll_slots, slot);
	}

	bool work_waiting = net_event_backlog(net) > 0;
	for (ptrdiff_t i = 0; i < naui_list_len(net->connections); i++)
	{
		NetConnection* connection = net->connections[i];
		if (connection->transport != NAUI_TRANSPORT_TCP)
			continue;

		if (connection->phase == NET_PHASE_CLOSED)
			continue;

		Naui_PollEvents want = 0;

		if (reading && !connection->tcp_eof && connection->phase != NET_PHASE_CONNECTING)
			want |= NAUI_POLL_READ;

		if (connection->phase == NET_PHASE_CONNECTING || net_buffer_len(&connection->tx) > 0)
			want |= NAUI_POLL_WRITE;

		if (net_buffer_len(&connection->rx) > 0 || connection->tcp_eof)
			work_waiting = true;

		if (!want)
			continue;

		Naui_PollItem item = { &connection->sock, want, 0 };
		NetPollSlot slot = { NET_SLOT_TCP_CONN, connection };
		naui_list_push(net->poll_items, item);
		naui_list_push(net->poll_slots, slot);
	}

	uint32_t wait = work_waiting ? 0 : (timeout_ms > NET_MAX_WAIT_MS ? NET_MAX_WAIT_MS : timeout_ms);
	naui_socket_poll(net->poll_items, (size_t)naui_list_len(net->poll_items), (int32_t)wait);
	net_tick(net);

	for (ptrdiff_t i = 0; i < naui_list_len(net->poll_items); i++)
	{
		if (!net->poll_items[i].got)
			continue;

		NetPollSlot slot = net->poll_slots[i];
		Naui_PollEvents got = net->poll_items[i].got;
		if (slot.kind == NET_SLOT_TCP_LISTENER)
		{
			net_tcp_accept(net);
			continue;
		}

		if (slot.kind == NET_SLOT_UDP)
		{
			net_udp_receive(net);
			continue;
		}

		if (slot.conn->phase == NET_PHASE_CLOSED)
			continue;

		NetConnection* connection = slot.conn;
		if (connection->phase == NET_PHASE_CONNECTING)
		{
			if (got & (NAUI_POLL_WRITE | NAUI_POLL_ERROR))
				net_tcp_connect_finished(net, connection);

			continue;
		}

		if (got & NAUI_POLL_WRITE && !net_tcp_try_flush(connection))
		{
			net_close_connection(net, connection, NAUI_DISCONNECT_ERROR, false);
			continue;
		}

		if (got & (NAUI_POLL_READ | NAUI_POLL_ERROR))
			net_tcp_read(net, connection);
	}

	for (ptrdiff_t i = 0; i < naui_list_len(net->connections); i++)
	{
		NetConnection* connection = net->connections[i];
		if (connection->transport != NAUI_TRANSPORT_TCP)
			continue;

		if (connection->phase == NET_PHASE_CLOSED || connection->phase == NET_PHASE_CONNECTING)
			continue;

		if (net_buffer_len(&connection->rx) > 0)
			net_tcp_parse(net, connection);

		if (connection->phase != NET_PHASE_CLOSED && connection->tcp_eof && net_buffer_len(&connection->rx) < 4)
			net_close_connection(net, connection, NAUI_DISCONNECT_REMOTE, false);
	}

	net_run_timers(net);
	net_sweep(net);
}

bool naui_net_poll(Naui_Net net, Naui_NetEvent* out_event)
{
	if (!net || !out_event)
		return false;

	free(net->current_payload);
	net->current_payload = NULL;

	if (net->events_head >= (size_t)naui_list_len(net->events))
		return false;

	Naui_NetEvent event = net->events[net->events_head++];
	if (event.data)
	{
		net->current_payload = (void*)event.data;
		net->queued_bytes -= event.size;
	}

	if (net->events_head >= (size_t)naui_list_len(net->events))
	{
		naui_list_clear(net->events);
		net->events_head = 0;
	}
	else if (net->events_head >= 1024 && net->events_head * 2 >= (size_t)naui_list_len(net->events))
	{
		arrdeln(net->events, 0, net->events_head);
		net->events_head = 0;
	}

	*out_event = event;
	return true;
}

const Naui_Device* naui_net_find_device(Naui_Net net, Naui_DeviceId id)
{
	if (!net)
		return NULL;

	return net_find_device(net, id);
}

size_t naui_net_device_count(Naui_Net net)
{
	if (!net)
		return 0;

	return (size_t)naui_list_len(net->devices);
}

const Naui_Device* naui_net_device_at(Naui_Net net, size_t index)
{
	if (!net)
		return NULL;

	if (index >= (size_t)naui_list_len(net->devices))
		return NULL;

	return net->devices[index];
}

bool naui_net_forget_device(Naui_Net net, Naui_DeviceId id)
{
	if (!net)
		return false;

	for (ptrdiff_t i = 0; i < naui_list_len(net->devices); i++)
	{
		Naui_Device* device = net->devices[i];
		if (!naui_device_id_equal(device->id, id))
			continue;

		if (device->connection_count > 0)
			return false;

		for (size_t event_index = net->events_head; event_index < (size_t)naui_list_len(net->events); event_index++)
		{
			if (net->events[event_index].device == device)
				return false;
		}

		free(device);
		naui_list_remove(net->devices, i);
		return true;
	}

	return false;
}