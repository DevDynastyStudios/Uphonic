/* UDP payload limit that stays inside the minimum IPv6 MTU. */
#define NAUI_UDP_MAX_MESSAGE_SIZE 1188

typedef struct Naui_NetImpl* Naui_Net;

typedef uint8_t Naui_SendMode;
enum
{
	NAUI_SEND_RELIABLE,
	NAUI_SEND_UNRELIABLE,
};

typedef struct
{
	uint32_t _id;
} Naui_Connection;

#define NAUI_CONNECTION_INVALID ((Naui_Connection){ 0 })

typedef uint8_t Naui_DisconnectReason;
enum
{
	NAUI_DISCONNECT_LOCAL,
	NAUI_DISCONNECT_REMOTE,
	NAUI_DISCONNECT_TIMEOUT,
	NAUI_DISCONNECT_ERROR,
	NAUI_DISCONNECT_PROTOCOL,
	NAUI_DISCONNECT_REJECTED,
	NAUI_DISCONNECT_SELF,
};

typedef struct
{
	const char* device_name;
	const Naui_DeviceId* device_id;
	uint32_t max_connections;
	uint32_t max_message_size;
	uint32_t max_devices;
	uint32_t handshake_timeout_ms;
	uint32_t idle_timeout_ms;
	uint32_t keepalive_interval_ms;
} Naui_NetConfig;

typedef uint8_t Naui_NetEventType;
enum
{
	NAUI_NET_EVENT_NONE,
	NAUI_NET_EVENT_CONNECTED,
	NAUI_NET_EVENT_CONNECT_FAILED,
	NAUI_NET_EVENT_DISCONNECTED,
	NAUI_NET_EVENT_MESSAGE,
};

typedef struct
{
	Naui_NetEventType type;
	Naui_Connection connection;
	Naui_Transport transport;
	Naui_Address address;
	Naui_DisconnectReason reason;
	const Naui_Device* device;
	const void* data;
	size_t size;
	bool reliable;
} Naui_NetEvent;

typedef uint8_t Naui_ConnectionState;
enum
{
	NAUI_CONNECTION_CONNECTING,
	NAUI_CONNECTION_CONNECTED,
};

typedef struct
{
	Naui_Transport transport;
	Naui_ConnectionState state;
	Naui_Address remote;
	const Naui_Device* device;
	uint64_t bytes_sent;
	uint64_t bytes_received;
	bool incoming;
} Naui_ConnectionInfo;

Naui_Net naui_net_create(const Naui_NetConfig* config);
void naui_net_destroy(Naui_Net net);

const Naui_Device* naui_net_local_device(Naui_Net net);

bool naui_net_listen(Naui_Net net, Naui_Transport transport, const Naui_Address* bind_address);
bool naui_net_listen_address(Naui_Net net, Naui_Transport transport, Naui_Address* out_address);

Naui_Connection naui_net_connect(Naui_Net net, Naui_Transport transport, const Naui_Address* remote);
void naui_net_disconnect(Naui_Net net, Naui_Connection connection);

bool naui_net_send(Naui_Net net, Naui_Connection connection, const void* data, size_t size, Naui_SendMode mode);
bool naui_net_send_to_device(Naui_Net net, const Naui_Device* device, const void* data, size_t size, Naui_SendMode mode);
bool naui_net_connection_info(Naui_Net net, Naui_Connection connection, Naui_ConnectionInfo* out_info);

void naui_net_update(Naui_Net net, uint32_t timeout_ms);
bool naui_net_poll(Naui_Net net, Naui_NetEvent* out_event);

const Naui_Device* naui_net_find_device(Naui_Net net, Naui_DeviceId id);
size_t naui_net_device_count(Naui_Net net);
const Naui_Device* naui_net_device_at(Naui_Net net, size_t index);
bool naui_net_forget_device(Naui_Net net, Naui_DeviceId id);