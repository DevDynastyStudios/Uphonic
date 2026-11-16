typedef uint8_t Naui_AddressFamily;
enum
{
	NAUI_ADDRESS_NONE,
	NAUI_ADDRESS_IPV4,
	NAUI_ADDRESS_IPV6,
};

typedef uint8_t Naui_SocketType;
enum
{
	NAUI_SOCKET_TCP,
	NAUI_SOCKET_UDP,
};

typedef uint8_t Naui_NetResult;
enum
{
	NAUI_NET_OK,
	NAUI_NET_WOULD_BLOCK,
	NAUI_NET_IN_PROGRESS,
	NAUI_NET_CLOSED,
	NAUI_NET_ERROR,
};

typedef uint8_t Naui_PollEvents;
enum
{
	NAUI_POLL_READ = 1 << 0,
	NAUI_POLL_WRITE = 1 << 1,
	NAUI_POLL_ERROR = 1 << 2,
};

enum
{
	NAUI_SOCKET_FLAG_LISTENING = 1 << 0,
};

typedef struct
{
	uint16_t port;
	Naui_AddressFamily family;
	uint32_t scope_id;
	uint8_t data[16];
} Naui_Address;

typedef struct
{
	intptr_t handle;
	Naui_SocketType type;
	Naui_AddressFamily family;
	uint32_t flags;
	Naui_Address address;
} Naui_Socket;

typedef struct
{
	const Naui_Socket* sock;
	Naui_PollEvents want;
	Naui_PollEvents got;
} Naui_PollItem;

#define NAUI_ADDRESS_STR_SIZE 64
#define NAUI_SOCKET_INVALID ((intptr_t)-1)
#define NAUI_SOCKET_NULL ((Naui_Socket){ .handle = NAUI_SOCKET_INVALID })

Naui_Address naui_address_any(Naui_AddressFamily family, uint16_t port);
Naui_Address naui_address_loopback(Naui_AddressFamily family, uint16_t port);

bool naui_address_parse(Naui_Address* out, const char* text);
bool naui_address_resolve(Naui_Address* out, const char* host, uint16_t port);
void naui_address_format(const Naui_Address* address, char* buffer, size_t size);
bool naui_address_equal(const Naui_Address* a, const Naui_Address* b);
void naui_address_normalize(Naui_Address* address);

bool naui_sockets_init(void);
void naui_sockets_shutdown(void);

bool naui_socket_open(Naui_Socket* sock, Naui_SocketType type, Naui_AddressFamily family);
void naui_socket_close(Naui_Socket* sock);

bool naui_socket_is_valid(const Naui_Socket* sock);
bool naui_socket_bind(Naui_Socket* sock, const Naui_Address* address, bool reuse);
bool naui_socket_listen(Naui_Socket* sock, int32_t backlog);

Naui_NetResult naui_socket_accept(Naui_Socket* listener, Naui_Socket* out_client, Naui_Address* out_peer);
Naui_NetResult naui_socket_connect(Naui_Socket* sock, const Naui_Address* address);
Naui_NetResult naui_socket_connect_finish(Naui_Socket* sock);
Naui_NetResult naui_socket_send(Naui_Socket* sock, const void* data, size_t size, size_t* out_count);
Naui_NetResult naui_socket_recv(Naui_Socket* sock, void* buffer, size_t size, size_t* out_count);
Naui_NetResult naui_socket_sendto(Naui_Socket* sock, const void* data, size_t size, const Naui_Address* to, size_t* out_count);
Naui_NetResult naui_socket_recvfrom(Naui_Socket* sock, void* buffer, size_t size, size_t* out_count, Naui_Address* out_from);

int32_t naui_socket_poll(Naui_PollItem* items, size_t count, int32_t timeout_ms);