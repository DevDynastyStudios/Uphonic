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
#define NET_UDP_RECV_SIZE 65536

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

	struct NetQuic* quic;
	Naui_DeviceId verified_id;
	bool verified;
} NetConnection;

typedef uint8_t NetSlotKind;
enum
{
	NET_SLOT_TCP_LISTENER,
	NET_SLOT_UDP,
	NET_SLOT_TCP_CONN,
	NET_SLOT_QUIC,
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
	Naui_DeviceKey device_key;

	Naui_List(Naui_Device*) devices;
	Naui_List(NetConnection*) connections;

	uint32_t next_connection_id;
	Naui_Socket tcp_listener;

	Naui_Socket udp;
	bool udp_listening;
	uint8_t* udp_rx;

	Naui_Socket quic;
	bool quic_listening;
	uint8_t quic_token_key[32];

	Naui_List(Naui_NetEvent) events;
	size_t events_head;
	size_t queued_bytes;

	void* current_payload;
	Naui_List(Naui_PollItem) poll_items;
	Naui_List(NetPollSlot) poll_slots;
	uint64_t now_ms;
};

static uint64_t _naui_net_now_ms(void)
{
#if NAUI_WINDOWS

	return (uint64_t)GetTickCount64();

#else

	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t)ts.tv_sec * 1000u + (uint64_t)ts.tv_nsec / 1000000u;

#endif
}

static uint64_t _naui_net_random64(void)
{
	uint64_t value = 0;
	_naui_net_random_bytes(&value, sizeof(value));
	return value;
}

static size_t _naui_net_buffer_len(const NetBuffer* buffer)
{
	return buffer->size - buffer->head;
}

static uint8_t* _naui_net_buffer_ptr(NetBuffer* buffer)
{
	return buffer->data + buffer->head;
}

static void _naui_net_buffer_consume(NetBuffer* buffer, size_t size)
{
	buffer->head += size;

	if (buffer->head >= buffer->size)
	{
		buffer->head = 0;
		buffer->size = 0;
	}
}

static bool _naui_net_buffer_append(NetBuffer* buffer, const void* data, size_t size)
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

static void _naui_net_buffer_free(NetBuffer* buffer)
{
	free(buffer->data);
	memset(buffer, 0, sizeof(*buffer));
}

static void _naui_net_tick(Naui_Net net)
{
	net->now_ms = _naui_net_now_ms();
}

static size_t _naui_net_event_backlog(const Naui_Net net)
{
	return (size_t)naui_list_len(net->events) - net->events_head;
}

static bool _naui_net_events_full(const Naui_Net net)
{
	return
		_naui_net_event_backlog(net) >= NET_MAX_QUEUED_EVENTS ||
		net->queued_bytes >= NET_MAX_QUEUED_BYTES;
}

static void _naui_net_push_event(Naui_Net net, Naui_NetEventType type, const NetConnection* connection, Naui_DisconnectReason reason, bool reliable, const void* data, size_t size)
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
	event.verified = connection->verified;

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

static Naui_Device* _naui_net_find_device(Naui_Net net, Naui_DeviceId id)
{
	for (ptrdiff_t i = 0; i < naui_list_len(net->devices); i++)
	{
		if (naui_device_id_equal(net->devices[i]->id, id))
			return net->devices[i];
	}

	return NULL;
}

static void _naui_net_device_note_endpoint(Naui_Device* device, const Naui_Address* address, Naui_Transport transport, bool incoming, uint64_t unix_now)
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

static Naui_Device* _naui_net_device_from_hello(Naui_Net net, const NetHello* hello, bool verified)
{
	uint64_t unix_now = naui_unix_time();
	Naui_Device* device = _naui_net_find_device(net, hello->id);
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

	device->last_seen = unix_now;
	if (device->trust == NAUI_DEVICE_TRUST_VERIFIED && !verified)
		return device;

	if (verified)
		device->trust = NAUI_DEVICE_TRUST_VERIFIED;

	device->platform = hello->platform;
	device->arch = hello->arch;
	memcpy(device->name, hello->name, sizeof(device->name));
	return device;
}

static void _naui_net_evict_devices(Naui_Net net)
{
	if (_naui_net_event_backlog(net) != 0)
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

static size_t _naui_net_hello_encode(const Naui_Device* local, uint8_t* out)
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

static bool _naui_net_hello_decode(const uint8_t* data, size_t size, NetHello* hello)
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

static NetConnection* _naui_net_new_connection(Naui_Net net, Naui_Transport transport, bool incoming, const Naui_Address* remote)
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

static NetConnection* _naui_net_find_connection(Naui_Net net, Naui_Connection handle)
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

static size_t _naui_net_count_live(const Naui_Net net)
{
	size_t count = 0;
	for (ptrdiff_t i = 0; i < naui_list_len(net->connections); i++)
	{
		count += net->connections[i]->phase != NET_PHASE_CLOSED;
	}

	return count;
}

static size_t _naui_net_count_pending(const Naui_Net net)
{
	size_t count = 0;
	for (ptrdiff_t i = 0; i < naui_list_len(net->connections); i++)
	{
		const NetConnection* connection = net->connections[i];
		count += connection->incoming && connection->phase != NET_PHASE_CLOSED && connection->phase != NET_PHASE_ESTABLISHED;
	}

	return count;
}

static void _naui_net_quic_free(NetConnection* connection);

static void _naui_net_sweep(Naui_Net net)
{
	for (ptrdiff_t i = naui_list_len(net->connections) - 1; i >= 0; --i)
	{
		NetConnection* connection = net->connections[i];
		if (connection->phase != NET_PHASE_CLOSED)
			continue;

		_naui_net_quic_free(connection);
		_naui_net_buffer_free(&connection->rx);
		_naui_net_buffer_free(&connection->tx);
		free(connection);
		naui_list_remove(net->connections, i);
	}
}

static void _naui_net_poll_add(Naui_Net net, const Naui_Socket* sock, Naui_PollEvents want, NetSlotKind kind, NetConnection* connection)
{
	Naui_PollItem item = { sock, want, 0 };
	NetPollSlot slot = { kind, connection };
	naui_list_push(net->poll_items, item);
	naui_list_push(net->poll_slots, slot);
}

static void _naui_net_tcp_close(NetConnection* connection, bool flush);
static void _naui_net_udp_close(Naui_Net net, NetConnection* connection, bool notify_peer);
static void _naui_net_quic_close(Naui_Net net, NetConnection* connection, bool notify_peer);

static void _naui_net_close_connection(Naui_Net net, NetConnection* connection, Naui_DisconnectReason reason, bool notify_peer)
{
	if (connection->phase == NET_PHASE_CLOSED)
		return;

	bool was_established = connection->phase == NET_PHASE_ESTABLISHED;
	if (connection->transport == NAUI_TRANSPORT_TCP)
		_naui_net_tcp_close(connection, notify_peer);
	else if (connection->transport == NAUI_TRANSPORT_UDP)
		_naui_net_udp_close(net, connection, notify_peer);
	else
		_naui_net_quic_close(net, connection, notify_peer);

	connection->phase = NET_PHASE_CLOSED;
	if (connection->device && connection->device->connection_count > 0)
		connection->device->connection_count--;

	if (was_established)
	{
		_naui_net_push_event(net, NAUI_NET_EVENT_DISCONNECTED, connection, reason, false, NULL, 0);
		return;
	}

	if (!connection->incoming)
		_naui_net_push_event(net, NAUI_NET_EVENT_CONNECT_FAILED, connection, reason, false, NULL, 0);
}

static void _naui_net_complete_handshake(Naui_Net net, NetConnection* connection, const NetHello* hello)
{
	Naui_Device* device = _naui_net_device_from_hello(net, hello, connection->verified);
	if (!device)
	{
		_naui_net_close_connection(net, connection, NAUI_DISCONNECT_ERROR, true);
		return;
	}

	connection->device = device;
	device->connection_count++;
	_naui_net_device_note_endpoint(device, &connection->remote, connection->transport, connection->incoming, naui_unix_time());

	connection->phase = NET_PHASE_ESTABLISHED;
	_naui_net_push_event(net, NAUI_NET_EVENT_CONNECTED, connection, NAUI_DISCONNECT_LOCAL, false, NULL, 0);
}

static bool _naui_net_frame_append(Naui_Net net, NetConnection* connection, NetBuffer* tx, uint8_t type, const void* payload, size_t size)
{
	size_t pending = _naui_net_buffer_len(tx);
	if (pending > net->send_buffer_limit || 5 + size > net->send_buffer_limit - pending)
		return false;

	uint8_t header[5];
	_naui_net_put_u32(header, (uint32_t)(1 + size));
	header[4] = type;
	if (!_naui_net_buffer_append(tx, header, sizeof(header)) || !_naui_net_buffer_append(tx, payload, size))
	{
		_naui_net_close_connection(net, connection, NAUI_DISCONNECT_ERROR, false);
		return false;
	}

	return true;
}

static void _naui_net_frame_handle(Naui_Net net, NetConnection* connection, uint8_t type, const uint8_t* payload, size_t size)
{
	switch (type)
	{
		case NET_FRAME_HELLO:
		{
			NetHello hello;
			if (connection->phase != NET_PHASE_HANDSHAKE || !_naui_net_hello_decode(payload, size, &hello))
			{
				_naui_net_close_connection(net, connection, NAUI_DISCONNECT_PROTOCOL, false);
				return;
			}

			if (connection->verified && !naui_device_id_equal(hello.id, connection->verified_id))
			{
				_naui_net_close_connection(net, connection, NAUI_DISCONNECT_PROTOCOL, true);
				return;
			}

			if (naui_device_id_equal(hello.id, net->local.id))
			{
				_naui_net_close_connection(net, connection, NAUI_DISCONNECT_SELF, true);
				return;
			}

			_naui_net_complete_handshake(net, connection, &hello);
			return;
		}

		case NET_FRAME_DATA:
		{
			if (connection->phase != NET_PHASE_ESTABLISHED)
			{
				_naui_net_close_connection(net, connection, NAUI_DISCONNECT_PROTOCOL, false);
				return;
			}

			_naui_net_push_event(net, NAUI_NET_EVENT_MESSAGE, connection, NAUI_DISCONNECT_LOCAL, true, payload, size);
			return;
		}

		case NET_FRAME_PING:
		{
			if (connection->phase != NET_PHASE_ESTABLISHED)
				_naui_net_close_connection(net, connection, NAUI_DISCONNECT_PROTOCOL, false);

			return;
		}

		default:
		{
			if (connection->phase != NET_PHASE_ESTABLISHED)
				_naui_net_close_connection(net, connection, NAUI_DISCONNECT_PROTOCOL, false);

			return;
		}
	}
}

static void _naui_net_frame_parse(Naui_Net net, NetConnection* connection)
{
	while (connection->phase != NET_PHASE_CLOSED)
	{
		if (_naui_net_events_full(net))
			break;

		size_t available = _naui_net_buffer_len(&connection->rx);

		if (available < 4)
			break;

		const uint8_t* data = _naui_net_buffer_ptr(&connection->rx);
		uint32_t length = _naui_net_get_u32(data);
		size_t limit = connection->phase == NET_PHASE_ESTABLISHED ? (size_t)net->config.max_message_size + 1 : NET_HELLO_MAX + 1;
		if (length < 1 || length > limit)
		{
			_naui_net_close_connection(net, connection, NAUI_DISCONNECT_PROTOCOL, false);
			return;
		}

		if (available - 4 < length)
			break;

		_naui_net_frame_handle(net, connection, data[4], data + 5, length - 1);
		if (connection->phase == NET_PHASE_CLOSED)
			return;

		_naui_net_buffer_consume(&connection->rx, 4 + (size_t)length);
	}
}
