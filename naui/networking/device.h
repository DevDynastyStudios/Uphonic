#define NAUI_DEVICE_ID_SIZE 32
#define NAUI_DEVICE_ID_STR_SIZE 65
#define NAUI_DEVICE_NAME_MAX 64
#define NAUI_DEVICE_MAX_ENDPOINTS 8

typedef uint8_t Naui_DeviceTrust;
enum
{
	NAUI_DEVICE_TRUST_UNVERIFIED,
	NAUI_DEVICE_TRUST_VERIFIED,
};

typedef uint8_t Naui_DevicePlatform;
enum
{
	NAUI_PLATFORM_UNKNOWN,
	NAUI_PLATFORM_WINDOWS,
	NAUI_PLATFORM_LINUX,
	NAUI_PLATFORM_MACOS,
};

typedef uint8_t Naui_DeviceArch;
enum
{
	NAUI_ARCH_UNKNOWN,
	NAUI_ARCH_X86,
	NAUI_ARCH_X86_64,
	NAUI_ARCH_ARM,
	NAUI_ARCH_ARM64,
};

typedef uint8_t Naui_Transport;
enum
{
	NAUI_TRANSPORT_TCP,
	NAUI_TRANSPORT_UDP,
	NAUI_TRANSPORT_QUIC,
	NAUI_TRANSPORT_COUNT,
};

typedef struct
{
	uint8_t bytes[NAUI_DEVICE_ID_SIZE];
} Naui_DeviceId;

typedef struct
{
	Naui_Address address;
	Naui_Transport transport;
	uint64_t last_seen;
	bool incoming;
} Naui_DeviceEndpoint;

typedef struct Naui_Device
{
	Naui_DeviceId id;
	Naui_DeviceTrust trust;
	char name[NAUI_DEVICE_NAME_MAX];

	Naui_DevicePlatform platform;
	Naui_DeviceArch arch;

	uint64_t first_seen;
	uint64_t last_seen;
	uint32_t connection_count;

	uint32_t endpoint_count;
	Naui_DeviceEndpoint endpoints[NAUI_DEVICE_MAX_ENDPOINTS];
	bool is_local;
} Naui_Device;


Naui_DeviceId naui_device_id_generate(void);
void naui_device_id_format(Naui_DeviceId id, char* buffer);
bool naui_device_id_is_null(Naui_DeviceId id);
bool naui_device_id_equal(Naui_DeviceId a, Naui_DeviceId b);
bool naui_device_id_parse(Naui_DeviceId* out, const char* text);
uint64_t naui_device_id_hash(Naui_DeviceId id);

void naui_device_describe_local(Naui_Device* device, const char* name_override);