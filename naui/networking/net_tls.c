#define NET_TLS_ALPN "naui"
#define NET_TLS_SUITE_CHACHA20_POLY1305_SHA256 0x1303
#define NET_TLS_GROUP_X25519 0x001d
#define NET_TLS_SIGNATURE_ED25519 0x0807
#define NET_TLS_VERSION_13 0x0304
#define NET_TLS_MAX_PARAMS 512
#define NET_TLS_CERTIFICATE_SIZE 206

enum
{
	NET_TLS_CLIENT_HELLO = 1,
	NET_TLS_SERVER_HELLO = 2,
	NET_TLS_NEW_SESSION_TICKET = 4,
	NET_TLS_ENCRYPTED_EXTENSIONS = 8,
	NET_TLS_CERTIFICATE = 11,
	NET_TLS_CERTIFICATE_REQUEST = 13,
	NET_TLS_CERTIFICATE_VERIFY = 15,
	NET_TLS_FINISHED = 20,
};

enum
{
	NET_TLS_EXT_SUPPORTED_GROUPS = 0x000a,
	NET_TLS_EXT_SIGNATURE_ALGORITHMS = 0x000d,
	NET_TLS_EXT_ALPN = 0x0010,
	NET_TLS_EXT_SUPPORTED_VERSIONS = 0x002b,
	NET_TLS_EXT_KEY_SHARE = 0x0033,
	NET_TLS_EXT_QUIC_PARAMS = 0x0039,
};

enum
{
	NET_TLS_ALERT_UNEXPECTED_MESSAGE = 10,
	NET_TLS_ALERT_HANDSHAKE_FAILURE = 40,
	NET_TLS_ALERT_BAD_CERTIFICATE = 42,
	NET_TLS_ALERT_ILLEGAL_PARAMETER = 47,
	NET_TLS_ALERT_DECODE_ERROR = 50,
	NET_TLS_ALERT_DECRYPT_ERROR = 51,
	NET_TLS_ALERT_PROTOCOL_VERSION = 70,
	NET_TLS_ALERT_INTERNAL_ERROR = 80,
	NET_TLS_ALERT_MISSING_EXTENSION = 109,
	NET_TLS_ALERT_CERTIFICATE_REQUIRED = 116,
	NET_TLS_ALERT_NO_APPLICATION_PROTOCOL = 120,
};

typedef uint8_t NetTlsLevel;
enum
{
	NET_TLS_LEVEL_INITIAL,
	NET_TLS_LEVEL_HANDSHAKE,
	NET_TLS_LEVEL_APPLICATION,
	NET_TLS_LEVEL_COUNT,
};

typedef uint8_t NetTlsState;
enum
{
	NET_TLS_WAIT_CLIENT_HELLO,
	NET_TLS_WAIT_SERVER_HELLO,
	NET_TLS_WAIT_ENCRYPTED_EXTENSIONS,
	NET_TLS_WAIT_CERTIFICATE,
	NET_TLS_WAIT_CERTIFICATE_VERIFY,
	NET_TLS_WAIT_FINISHED,
	NET_TLS_DONE,
};

typedef struct
{
	const uint8_t* p;
	size_t left;
	bool ok;
} NetReader;

typedef struct
{
	bool server;
	NetTlsState state;
	uint8_t alert;

	bool handshake_keys_ready;
	bool application_keys_ready;
	bool complete;
	bool certificate_requested;

	NetSha256 transcript;
	uint8_t seed[NET_ED25519_KEY_SIZE];
	uint8_t public_key[NET_ED25519_KEY_SIZE];
	uint8_t peer_key[NET_ED25519_KEY_SIZE];
	uint8_t ecdh_secret[NET_X25519_SIZE];
	uint8_t secret[NET_SHA256_SIZE];

	uint8_t client_handshake[NET_SHA256_SIZE];
	uint8_t server_handshake[NET_SHA256_SIZE];
	uint8_t client_application[NET_SHA256_SIZE];
	uint8_t server_application[NET_SHA256_SIZE];

	uint8_t request_context[255];
	size_t request_context_size;

	uint8_t local_params[NET_TLS_MAX_PARAMS];
	size_t local_params_size;
	uint8_t peer_params[NET_TLS_MAX_PARAMS];
	size_t peer_params_size;

	NetBuffer out[NET_TLS_LEVEL_COUNT];
} NetTls;

static NetReader _naui_net_reader(const uint8_t* data, size_t size)
{
	return (NetReader){ data, size, true };
}

static const uint8_t* _naui_net_read_bytes(NetReader* reader, size_t size)
{
	if (!reader->ok || reader->left < size)
	{
		reader->ok = false;
		reader->left = 0;
		return NULL;
	}

	const uint8_t* p = reader->p;
	reader->p += size;
	reader->left -= size;
	return p;
}

static uint32_t _naui_net_read_uint(NetReader* reader, size_t bytes)
{
	const uint8_t* p = _naui_net_read_bytes(reader, bytes);
	uint32_t value = 0;
	for (size_t i = 0; p && i < bytes; i++)
	{
		value = (value << 8) | p[i];
	}

	return value;
}

static NetReader _naui_net_read_block(NetReader* reader, size_t length_bytes)
{
	size_t size = _naui_net_read_uint(reader, length_bytes);
	const uint8_t* p = _naui_net_read_bytes(reader, size);
	return (NetReader){ p, p ? size : 0, p != NULL };
}

static bool _naui_net_reader_done(const NetReader* reader)
{
	return reader->ok && reader->left == 0;
}

static void _naui_net_write_uint(NetBuffer* buffer, uint32_t value, size_t bytes)
{
	uint8_t encoded[4];
	_naui_net_put_u32(encoded, value);
	_naui_net_buffer_append(buffer, encoded + 4 - bytes, bytes);
}

static size_t _naui_net_write_block_begin(NetBuffer* buffer, size_t length_bytes)
{
	_naui_net_write_uint(buffer, 0, length_bytes);
	return buffer->size;
}

static void _naui_net_write_block_end(NetBuffer* buffer, size_t mark, size_t length_bytes)
{
	if (buffer->size < mark)
		return;

	size_t size = buffer->size - mark;
	for (size_t i = 0; i < length_bytes; i++)
	{
		buffer->data[mark - 1 - i] = (uint8_t)(size >> (8 * i));
	}
}

static void _naui_net_write_extension(NetBuffer* buffer, uint16_t type, const void* data, size_t size)
{
	_naui_net_write_uint(buffer, type, 2);
	_naui_net_write_uint(buffer, (uint32_t)size, 2);
	_naui_net_buffer_append(buffer, data, size);
}

static bool _naui_net_tls_fail(NetTls* tls, uint8_t alert)
{
	if (!tls->alert)
		tls->alert = alert;

	return false;
}

static void _naui_net_tls_free(NetTls* tls)
{
	for (int i = 0; i < NET_TLS_LEVEL_COUNT; i++)
	{
		_naui_net_buffer_free(&tls->out[i]);
	}

	_naui_net_wipe(tls, sizeof(*tls));
}

static void _naui_net_tls_init(NetTls* tls, bool server, const uint8_t seed[NET_ED25519_KEY_SIZE], const uint8_t public_key[NET_ED25519_KEY_SIZE])
{
	memset(tls, 0, sizeof(*tls));
	tls->server = server;
	tls->state = server ? NET_TLS_WAIT_CLIENT_HELLO : NET_TLS_WAIT_SERVER_HELLO;
	memcpy(tls->seed, seed, NET_ED25519_KEY_SIZE);
	memcpy(tls->public_key, public_key, NET_ED25519_KEY_SIZE);
	_naui_net_sha256_init(&tls->transcript);
}

static bool _naui_net_tls_emit(NetTls* tls, NetTlsLevel level, uint8_t type, const NetBuffer* body)
{
	uint8_t header[4];
	_naui_net_put_u32(header, (uint32_t)body->size);
	header[0] = type;

	_naui_net_sha256_update(&tls->transcript, header, sizeof(header));
	_naui_net_sha256_update(&tls->transcript, body->data, body->size);
	if (!_naui_net_buffer_append(&tls->out[level], header, sizeof(header)) || !_naui_net_buffer_append(&tls->out[level], body->data, body->size))
		return _naui_net_tls_fail(tls, NET_TLS_ALERT_INTERNAL_ERROR);

	return true;
}

static void _naui_net_tls_derive(const uint8_t secret[NET_SHA256_SIZE], const char* label, const uint8_t hash[NET_SHA256_SIZE], uint8_t out[NET_SHA256_SIZE])
{
	_naui_net_hkdf_expand_label(secret, label, hash, NET_SHA256_SIZE, out, NET_SHA256_SIZE);
}

static bool _naui_net_tls_derive_handshake_keys(NetTls* tls, const uint8_t peer_share[NET_X25519_SIZE])
{
	uint8_t shared[NET_X25519_SIZE];
	uint8_t zero[NET_SHA256_SIZE] = { 0 };
	_naui_net_x25519(shared, tls->ecdh_secret, peer_share);
	_naui_net_wipe(tls->ecdh_secret, sizeof(tls->ecdh_secret));
	if (_naui_net_ct_equal(shared, zero, sizeof(shared)))
		return _naui_net_tls_fail(tls, NET_TLS_ALERT_ILLEGAL_PARAMETER);

	uint8_t empty_hash[NET_SHA256_SIZE];
	uint8_t early[NET_SHA256_SIZE];
	uint8_t derived[NET_SHA256_SIZE];
	uint8_t hash[NET_SHA256_SIZE];

	_naui_net_sha256(NULL, 0, empty_hash);
	_naui_net_hkdf_extract(zero, sizeof(zero), zero, sizeof(zero), early);
	_naui_net_tls_derive(early, "derived", empty_hash, derived);
	_naui_net_hkdf_extract(derived, sizeof(derived), shared, sizeof(shared), tls->secret);

	_naui_net_sha256_final(&tls->transcript, hash);
	_naui_net_tls_derive(tls->secret, "c hs traffic", hash, tls->client_handshake);
	_naui_net_tls_derive(tls->secret, "s hs traffic", hash, tls->server_handshake);

	_naui_net_tls_derive(tls->secret, "derived", empty_hash, derived);
	_naui_net_hkdf_extract(derived, sizeof(derived), zero, sizeof(zero), tls->secret);

	_naui_net_wipe(shared, sizeof(shared));
	_naui_net_wipe(derived, sizeof(derived));
	tls->handshake_keys_ready = true;
	return true;
}

static void _naui_net_tls_derive_application_keys(NetTls* tls)
{
	uint8_t hash[NET_SHA256_SIZE];
	_naui_net_sha256_final(&tls->transcript, hash);
	_naui_net_tls_derive(tls->secret, "c ap traffic", hash, tls->client_application);
	_naui_net_tls_derive(tls->secret, "s ap traffic", hash, tls->server_application);
	_naui_net_wipe(tls->secret, sizeof(tls->secret));
	tls->application_keys_ready = true;
}

static void _naui_net_tls_finished_mac(const uint8_t traffic_secret[NET_SHA256_SIZE], const uint8_t hash[NET_SHA256_SIZE], uint8_t out[NET_SHA256_SIZE])
{
	uint8_t key[NET_SHA256_SIZE];
	_naui_net_hkdf_expand_label(traffic_secret, "finished", NULL, 0, key, sizeof(key));
	_naui_net_hmac_sha256(key, sizeof(key), hash, NET_SHA256_SIZE, out);
	_naui_net_wipe(key, sizeof(key));
}

static size_t _naui_net_tls_signed_content(bool server, const uint8_t hash[NET_SHA256_SIZE], uint8_t out[200])
{
	const char* label = server ? "TLS 1.3, server CertificateVerify" : "TLS 1.3, client CertificateVerify";
	size_t label_size = strlen(label);
	memset(out, 0x20, 64);
	memcpy(out + 64, label, label_size);
	out[64 + label_size] = 0;
	memcpy(out + 65 + label_size, hash, NET_SHA256_SIZE);
	return 65 + label_size + NET_SHA256_SIZE;
}

static void _naui_net_tls_build_certificate(const NetTls* tls, uint8_t out[NET_TLS_CERTIFICATE_SIZE])
{
	static const uint8_t prefix[] =
	{
		0x30, 0x81, 0xcb,
		0x30, 0x7f,
		0xa0, 0x03, 0x02, 0x01, 0x02,
		0x02, 0x01, 0x01,
		0x30, 0x05, 0x06, 0x03, 0x2b, 0x65, 0x70,
		0x30, 0x0f, 0x31, 0x0d, 0x30, 0x0b, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0c, 0x04, 'n', 'a', 'u', 'i',
		0x30, 0x20,
		0x17, 0x0d, '7', '0', '0', '1', '0', '1', '0', '0', '0', '0', '0', '0', 'Z',
		0x18, 0x0f, '9', '9', '9', '9', '1', '2', '3', '1', '2', '3', '5', '9', '5', '9', 'Z',
		0x30, 0x0f, 0x31, 0x0d, 0x30, 0x0b, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0c, 0x04, 'n', 'a', 'u', 'i',
		0x30, 0x2a, 0x30, 0x05, 0x06, 0x03, 0x2b, 0x65, 0x70, 0x03, 0x21, 0x00,
	};

	static const uint8_t middle[] =
	{
		0x30, 0x05, 0x06, 0x03, 0x2b, 0x65, 0x70,
		0x03, 0x41, 0x00,
	};

	size_t n = 0;
	memcpy(out, prefix, sizeof(prefix));
	n += sizeof(prefix);
	memcpy(out + n, tls->public_key, NET_ED25519_KEY_SIZE);
	n += NET_ED25519_KEY_SIZE;

	size_t body_end = n;
	memcpy(out + n, middle, sizeof(middle));
	n += sizeof(middle);
	_naui_net_ed25519_sign(tls->seed, tls->public_key, out + 3, body_end - 3, out + n);
}

static bool _naui_net_der_next(NetReader* reader, uint8_t* tag, NetReader* content)
{
	*tag = (uint8_t)_naui_net_read_uint(reader, 1);
	size_t size = _naui_net_read_uint(reader, 1);
	if (size & 0x80)
	{
		size_t length_bytes = size & 0x7f;
		if (length_bytes == 0 || length_bytes > 3)
			return false;

		size = _naui_net_read_uint(reader, length_bytes);
	}

	const uint8_t* p = _naui_net_read_bytes(reader, size);
	*content = _naui_net_reader(p, p ? size : 0);
	return p != NULL;
}

static bool _naui_net_tls_certificate_key(const uint8_t* der, size_t size, uint8_t key[NET_ED25519_KEY_SIZE])
{
	static const uint8_t ed25519_oid[] = { 0x06, 0x03, 0x2b, 0x65, 0x70 };

	NetReader outer = _naui_net_reader(der, size);
	NetReader certificate, body, field, algorithm, bits;
	uint8_t tag;

	if (!_naui_net_der_next(&outer, &tag, &certificate) || tag != 0x30)
		return false;

	if (!_naui_net_der_next(&certificate, &tag, &body) || tag != 0x30)
		return false;

	if (!_naui_net_der_next(&body, &tag, &field))
		return false;

	int skip = tag == 0xa0 ? 5 : 4;
	for (int i = 0; i < skip; i++)
	{
		if (!_naui_net_der_next(&body, &tag, &field))
			return false;
	}

	if (!_naui_net_der_next(&body, &tag, &field) || tag != 0x30)
		return false;

	if (!_naui_net_der_next(&field, &tag, &algorithm) || tag != 0x30)
		return false;

	if (algorithm.left != sizeof(ed25519_oid) || memcmp(algorithm.p, ed25519_oid, sizeof(ed25519_oid)) != 0)
		return false;

	if (!_naui_net_der_next(&field, &tag, &bits) || tag != 0x03)
		return false;

	if (bits.left != 1 + NET_ED25519_KEY_SIZE || bits.p[0] != 0)
		return false;

	memcpy(key, bits.p + 1, NET_ED25519_KEY_SIZE);
	return true;
}

static bool _naui_net_tls_send_certificate(NetTls* tls)
{
	NetBuffer body = { 0 };
	uint8_t certificate[NET_TLS_CERTIFICATE_SIZE];
	_naui_net_tls_build_certificate(tls, certificate);

	_naui_net_write_uint(&body, (uint32_t)tls->request_context_size, 1);
	_naui_net_buffer_append(&body, tls->request_context, tls->request_context_size);
	size_t list = _naui_net_write_block_begin(&body, 3);
	_naui_net_write_uint(&body, sizeof(certificate), 3);
	_naui_net_buffer_append(&body, certificate, sizeof(certificate));
	_naui_net_write_uint(&body, 0, 2);
	_naui_net_write_block_end(&body, list, 3);

	bool ok = _naui_net_tls_emit(tls, NET_TLS_LEVEL_HANDSHAKE, NET_TLS_CERTIFICATE, &body);
	_naui_net_buffer_free(&body);
	if (!ok)
		return false;

	uint8_t hash[NET_SHA256_SIZE];
	uint8_t content[200];
	uint8_t signature[NET_ED25519_SIGNATURE_SIZE];
	_naui_net_sha256_final(&tls->transcript, hash);
	size_t content_size = _naui_net_tls_signed_content(tls->server, hash, content);
	_naui_net_ed25519_sign(tls->seed, tls->public_key, content, content_size, signature);

	_naui_net_write_uint(&body, NET_TLS_SIGNATURE_ED25519, 2);
	_naui_net_write_uint(&body, sizeof(signature), 2);
	_naui_net_buffer_append(&body, signature, sizeof(signature));
	ok = _naui_net_tls_emit(tls, NET_TLS_LEVEL_HANDSHAKE, NET_TLS_CERTIFICATE_VERIFY, &body);
	_naui_net_buffer_free(&body);
	return ok;
}

static bool _naui_net_tls_send_finished(NetTls* tls)
{
	uint8_t hash[NET_SHA256_SIZE];
	uint8_t mac[NET_SHA256_SIZE];
	_naui_net_sha256_final(&tls->transcript, hash);
	_naui_net_tls_finished_mac(tls->server ? tls->server_handshake : tls->client_handshake, hash, mac);

	NetBuffer body = { 0 };
	_naui_net_buffer_append(&body, mac, sizeof(mac));
	bool ok = _naui_net_tls_emit(tls, NET_TLS_LEVEL_HANDSHAKE, NET_TLS_FINISHED, &body);
	_naui_net_buffer_free(&body);
	return ok;
}

static bool _naui_net_tls_start(NetTls* tls)
{
	uint8_t random[32];
	uint8_t share[NET_X25519_SIZE];
	if (!_naui_net_random_bytes(random, sizeof(random)) || !_naui_net_random_bytes(tls->ecdh_secret, sizeof(tls->ecdh_secret)))
		return _naui_net_tls_fail(tls, NET_TLS_ALERT_INTERNAL_ERROR);

	_naui_net_x25519_public(share, tls->ecdh_secret);

	NetBuffer body = { 0 };
	_naui_net_write_uint(&body, 0x0303, 2);
	_naui_net_buffer_append(&body, random, sizeof(random));
	_naui_net_write_uint(&body, 0, 1);
	_naui_net_write_uint(&body, 2, 2);
	_naui_net_write_uint(&body, NET_TLS_SUITE_CHACHA20_POLY1305_SHA256, 2);
	_naui_net_write_uint(&body, 0x0100, 2);

	size_t extensions = _naui_net_write_block_begin(&body, 2);
	_naui_net_write_extension(&body, NET_TLS_EXT_SUPPORTED_VERSIONS, (uint8_t[]){ 2, 0x03, 0x04 }, 3);
	_naui_net_write_extension(&body, NET_TLS_EXT_SUPPORTED_GROUPS, (uint8_t[]){ 0, 2, 0x00, 0x1d }, 4);
	_naui_net_write_extension(&body, NET_TLS_EXT_SIGNATURE_ALGORITHMS, (uint8_t[]){ 0, 2, 0x08, 0x07 }, 4);

	uint8_t key_share[38] = { 0, 36, 0x00, 0x1d, 0, 32 };
	memcpy(key_share + 6, share, sizeof(share));
	_naui_net_write_extension(&body, NET_TLS_EXT_KEY_SHARE, key_share, sizeof(key_share));

	uint8_t alpn[3 + sizeof(NET_TLS_ALPN) - 1] = { 0, sizeof(NET_TLS_ALPN), sizeof(NET_TLS_ALPN) - 1 };
	memcpy(alpn + 3, NET_TLS_ALPN, sizeof(NET_TLS_ALPN) - 1);
	_naui_net_write_extension(&body, NET_TLS_EXT_ALPN, alpn, sizeof(alpn));
	_naui_net_write_extension(&body, NET_TLS_EXT_QUIC_PARAMS, tls->local_params, tls->local_params_size);
	_naui_net_write_block_end(&body, extensions, 2);

	bool ok = _naui_net_tls_emit(tls, NET_TLS_LEVEL_INITIAL, NET_TLS_CLIENT_HELLO, &body);
	_naui_net_buffer_free(&body);
	return ok;
}

static bool _naui_net_tls_list_contains(NetReader list, uint32_t wanted, size_t item_bytes)
{
	while (list.ok && list.left > 0)
	{
		if (_naui_net_read_uint(&list, item_bytes) == wanted && list.ok)
			return true;
	}

	return false;
}

static bool _naui_net_tls_alpn_matches(NetReader extension)
{
	NetReader list = _naui_net_read_block(&extension, 2);
	while (list.ok && list.left > 0)
	{
		NetReader name = _naui_net_read_block(&list, 1);
		if (name.ok && name.left == sizeof(NET_TLS_ALPN) - 1 && memcmp(name.p, NET_TLS_ALPN, name.left) == 0)
			return true;
	}

	return false;
}

static bool _naui_net_tls_store_peer_params(NetTls* tls, NetReader extension)
{
	if (extension.left > sizeof(tls->peer_params))
		return false;

	memcpy(tls->peer_params, extension.p, extension.left);
	tls->peer_params_size = extension.left;
	return true;
}

static bool _naui_net_tls_on_client_hello(NetTls* tls, NetReader body)
{
	_naui_net_read_uint(&body, 2);
	_naui_net_read_bytes(&body, 32);
	NetReader session = _naui_net_read_block(&body, 1);
	NetReader suites = _naui_net_read_block(&body, 2);
	NetReader compression = _naui_net_read_block(&body, 1);
	NetReader extensions = _naui_net_read_block(&body, 2);
	if (!_naui_net_reader_done(&body) || !session.ok || !suites.ok || !compression.ok || session.left > 32)
		return _naui_net_tls_fail(tls, NET_TLS_ALERT_DECODE_ERROR);

	if (!_naui_net_tls_list_contains(suites, NET_TLS_SUITE_CHACHA20_POLY1305_SHA256, 2))
		return _naui_net_tls_fail(tls, NET_TLS_ALERT_HANDSHAKE_FAILURE);

	bool version_ok = false;
	bool signature_ok = false;
	bool alpn_ok = false;
	bool params_ok = false;
	const uint8_t* peer_share = NULL;

	while (extensions.left > 0)
	{
		uint32_t type = _naui_net_read_uint(&extensions, 2);
		NetReader extension = _naui_net_read_block(&extensions, 2);
		if (!extension.ok)
			return _naui_net_tls_fail(tls, NET_TLS_ALERT_DECODE_ERROR);

		if (type == NET_TLS_EXT_SUPPORTED_VERSIONS)
			version_ok = _naui_net_tls_list_contains(_naui_net_read_block(&extension, 1), NET_TLS_VERSION_13, 2);
		else if (type == NET_TLS_EXT_SIGNATURE_ALGORITHMS)
			signature_ok = _naui_net_tls_list_contains(_naui_net_read_block(&extension, 2), NET_TLS_SIGNATURE_ED25519, 2);
		else if (type == NET_TLS_EXT_ALPN)
			alpn_ok = _naui_net_tls_alpn_matches(extension);
		else if (type == NET_TLS_EXT_QUIC_PARAMS)
			params_ok = _naui_net_tls_store_peer_params(tls, extension);
		else if (type == NET_TLS_EXT_KEY_SHARE)
		{
			NetReader shares = _naui_net_read_block(&extension, 2);
			while (shares.ok && shares.left > 0)
			{
				uint32_t group = _naui_net_read_uint(&shares, 2);
				NetReader share = _naui_net_read_block(&shares, 2);
				if (share.ok && group == NET_TLS_GROUP_X25519 && share.left == NET_X25519_SIZE)
					peer_share = share.p;
			}
		}
	}

	if (!version_ok)
		return _naui_net_tls_fail(tls, NET_TLS_ALERT_PROTOCOL_VERSION);

	if (!alpn_ok)
		return _naui_net_tls_fail(tls, NET_TLS_ALERT_NO_APPLICATION_PROTOCOL);

	if (!params_ok)
		return _naui_net_tls_fail(tls, NET_TLS_ALERT_MISSING_EXTENSION);

	if (!signature_ok || !peer_share)
		return _naui_net_tls_fail(tls, NET_TLS_ALERT_HANDSHAKE_FAILURE);

	uint8_t random[32];
	uint8_t share[NET_X25519_SIZE];
	if (!_naui_net_random_bytes(random, sizeof(random)) || !_naui_net_random_bytes(tls->ecdh_secret, sizeof(tls->ecdh_secret)))
		return _naui_net_tls_fail(tls, NET_TLS_ALERT_INTERNAL_ERROR);

	_naui_net_x25519_public(share, tls->ecdh_secret);

	NetBuffer out = { 0 };
	_naui_net_write_uint(&out, 0x0303, 2);
	_naui_net_buffer_append(&out, random, sizeof(random));
	_naui_net_write_uint(&out, (uint32_t)session.left, 1);
	_naui_net_buffer_append(&out, session.p, session.left);
	_naui_net_write_uint(&out, NET_TLS_SUITE_CHACHA20_POLY1305_SHA256, 2);
	_naui_net_write_uint(&out, 0, 1);

	size_t block = _naui_net_write_block_begin(&out, 2);
	_naui_net_write_extension(&out, NET_TLS_EXT_SUPPORTED_VERSIONS, (uint8_t[]){ 0x03, 0x04 }, 2);
	uint8_t key_share[36] = { 0x00, 0x1d, 0, 32 };
	memcpy(key_share + 4, share, sizeof(share));
	_naui_net_write_extension(&out, NET_TLS_EXT_KEY_SHARE, key_share, sizeof(key_share));
	_naui_net_write_block_end(&out, block, 2);

	bool ok = _naui_net_tls_emit(tls, NET_TLS_LEVEL_INITIAL, NET_TLS_SERVER_HELLO, &out);
	_naui_net_buffer_free(&out);
	if (!ok || !_naui_net_tls_derive_handshake_keys(tls, peer_share))
		return false;

	block = _naui_net_write_block_begin(&out, 2);
	uint8_t alpn[3 + sizeof(NET_TLS_ALPN) - 1] = { 0, sizeof(NET_TLS_ALPN), sizeof(NET_TLS_ALPN) - 1 };
	memcpy(alpn + 3, NET_TLS_ALPN, sizeof(NET_TLS_ALPN) - 1);
	_naui_net_write_extension(&out, NET_TLS_EXT_ALPN, alpn, sizeof(alpn));
	_naui_net_write_extension(&out, NET_TLS_EXT_QUIC_PARAMS, tls->local_params, tls->local_params_size);
	_naui_net_write_block_end(&out, block, 2);
	ok = _naui_net_tls_emit(tls, NET_TLS_LEVEL_HANDSHAKE, NET_TLS_ENCRYPTED_EXTENSIONS, &out);
	_naui_net_buffer_free(&out);
	if (!ok)
		return false;

	_naui_net_write_uint(&out, 0, 1);
	block = _naui_net_write_block_begin(&out, 2);
	_naui_net_write_extension(&out, NET_TLS_EXT_SIGNATURE_ALGORITHMS, (uint8_t[]){ 0, 2, 0x08, 0x07 }, 4);
	_naui_net_write_block_end(&out, block, 2);
	ok = _naui_net_tls_emit(tls, NET_TLS_LEVEL_HANDSHAKE, NET_TLS_CERTIFICATE_REQUEST, &out);
	_naui_net_buffer_free(&out);
	if (!ok || !_naui_net_tls_send_certificate(tls) || !_naui_net_tls_send_finished(tls))
		return false;

	_naui_net_tls_derive_application_keys(tls);
	tls->state = NET_TLS_WAIT_CERTIFICATE;
	return true;
}

static bool _naui_net_tls_on_server_hello(NetTls* tls, NetReader body)
{
	uint32_t version = _naui_net_read_uint(&body, 2);
	_naui_net_read_bytes(&body, 32);
	NetReader session = _naui_net_read_block(&body, 1);
	uint32_t suite = _naui_net_read_uint(&body, 2);
	uint32_t compression = _naui_net_read_uint(&body, 1);
	NetReader extensions = _naui_net_read_block(&body, 2);
	if (!_naui_net_reader_done(&body) || !session.ok)
		return _naui_net_tls_fail(tls, NET_TLS_ALERT_DECODE_ERROR);

	if (version != 0x0303 || session.left != 0 || suite != NET_TLS_SUITE_CHACHA20_POLY1305_SHA256 || compression != 0)
		return _naui_net_tls_fail(tls, NET_TLS_ALERT_ILLEGAL_PARAMETER);

	bool version_ok = false;
	const uint8_t* peer_share = NULL;
	while (extensions.left > 0)
	{
		uint32_t type = _naui_net_read_uint(&extensions, 2);
		NetReader extension = _naui_net_read_block(&extensions, 2);
		if (!extension.ok)
			return _naui_net_tls_fail(tls, NET_TLS_ALERT_DECODE_ERROR);

		if (type == NET_TLS_EXT_SUPPORTED_VERSIONS)
			version_ok = _naui_net_read_uint(&extension, 2) == NET_TLS_VERSION_13 && _naui_net_reader_done(&extension);
		else if (type == NET_TLS_EXT_KEY_SHARE)
		{
			uint32_t group = _naui_net_read_uint(&extension, 2);
			NetReader share = _naui_net_read_block(&extension, 2);
			if (_naui_net_reader_done(&extension) && share.ok && group == NET_TLS_GROUP_X25519 && share.left == NET_X25519_SIZE)
				peer_share = share.p;
		}
	}

	if (!version_ok)
		return _naui_net_tls_fail(tls, NET_TLS_ALERT_PROTOCOL_VERSION);

	if (!peer_share)
		return _naui_net_tls_fail(tls, NET_TLS_ALERT_ILLEGAL_PARAMETER);

	if (!_naui_net_tls_derive_handshake_keys(tls, peer_share))
		return false;

	tls->state = NET_TLS_WAIT_ENCRYPTED_EXTENSIONS;
	return true;
}

static bool _naui_net_tls_on_encrypted_extensions(NetTls* tls, NetReader body)
{
	NetReader extensions = _naui_net_read_block(&body, 2);
	if (!_naui_net_reader_done(&body))
		return _naui_net_tls_fail(tls, NET_TLS_ALERT_DECODE_ERROR);

	bool alpn_ok = false;
	bool params_ok = false;
	while (extensions.left > 0)
	{
		uint32_t type = _naui_net_read_uint(&extensions, 2);
		NetReader extension = _naui_net_read_block(&extensions, 2);
		if (!extension.ok)
			return _naui_net_tls_fail(tls, NET_TLS_ALERT_DECODE_ERROR);

		if (type == NET_TLS_EXT_ALPN)
			alpn_ok = _naui_net_tls_alpn_matches(extension);
		else if (type == NET_TLS_EXT_QUIC_PARAMS)
			params_ok = _naui_net_tls_store_peer_params(tls, extension);
	}

	if (!alpn_ok)
		return _naui_net_tls_fail(tls, NET_TLS_ALERT_NO_APPLICATION_PROTOCOL);

	if (!params_ok)
		return _naui_net_tls_fail(tls, NET_TLS_ALERT_MISSING_EXTENSION);

	tls->state = NET_TLS_WAIT_CERTIFICATE;
	return true;
}

static bool _naui_net_tls_on_certificate_request(NetTls* tls, NetReader body)
{
	NetReader context = _naui_net_read_block(&body, 1);
	NetReader extensions = _naui_net_read_block(&body, 2);
	if (!_naui_net_reader_done(&body) || !context.ok || tls->certificate_requested)
		return _naui_net_tls_fail(tls, NET_TLS_ALERT_DECODE_ERROR);

	bool signature_ok = false;
	while (extensions.left > 0)
	{
		uint32_t type = _naui_net_read_uint(&extensions, 2);
		NetReader extension = _naui_net_read_block(&extensions, 2);
		if (!extension.ok)
			return _naui_net_tls_fail(tls, NET_TLS_ALERT_DECODE_ERROR);

		if (type == NET_TLS_EXT_SIGNATURE_ALGORITHMS)
			signature_ok = _naui_net_tls_list_contains(_naui_net_read_block(&extension, 2), NET_TLS_SIGNATURE_ED25519, 2);
	}

	if (!signature_ok)
		return _naui_net_tls_fail(tls, NET_TLS_ALERT_HANDSHAKE_FAILURE);

	memcpy(tls->request_context, context.p, context.left);
	tls->request_context_size = context.left;
	tls->certificate_requested = true;
	return true;
}

static bool _naui_net_tls_on_certificate(NetTls* tls, NetReader body)
{
	NetReader context = _naui_net_read_block(&body, 1);
	NetReader list = _naui_net_read_block(&body, 3);
	if (!_naui_net_reader_done(&body) || !context.ok || context.left != 0)
		return _naui_net_tls_fail(tls, NET_TLS_ALERT_DECODE_ERROR);

	if (list.left == 0)
		return _naui_net_tls_fail(tls, tls->server ? NET_TLS_ALERT_CERTIFICATE_REQUIRED : NET_TLS_ALERT_DECODE_ERROR);

	NetReader certificate = _naui_net_read_block(&list, 3);
	if (!certificate.ok || !_naui_net_tls_certificate_key(certificate.p, certificate.left, tls->peer_key))
		return _naui_net_tls_fail(tls, NET_TLS_ALERT_BAD_CERTIFICATE);

	tls->state = NET_TLS_WAIT_CERTIFICATE_VERIFY;
	return true;
}

static bool _naui_net_tls_on_certificate_verify(NetTls* tls, NetReader body, const uint8_t hash[NET_SHA256_SIZE])
{
	uint32_t algorithm = _naui_net_read_uint(&body, 2);
	NetReader signature = _naui_net_read_block(&body, 2);
	if (!_naui_net_reader_done(&body) || !signature.ok)
		return _naui_net_tls_fail(tls, NET_TLS_ALERT_DECODE_ERROR);

	if (algorithm != NET_TLS_SIGNATURE_ED25519 || signature.left != NET_ED25519_SIGNATURE_SIZE)
		return _naui_net_tls_fail(tls, NET_TLS_ALERT_ILLEGAL_PARAMETER);

	uint8_t content[200];
	size_t content_size = _naui_net_tls_signed_content(!tls->server, hash, content);
	if (!_naui_net_ed25519_verify(tls->peer_key, content, content_size, signature.p))
		return _naui_net_tls_fail(tls, NET_TLS_ALERT_DECRYPT_ERROR);

	tls->state = NET_TLS_WAIT_FINISHED;
	return true;
}

static bool _naui_net_tls_on_finished(NetTls* tls, NetReader body, const uint8_t hash[NET_SHA256_SIZE])
{
	uint8_t mac[NET_SHA256_SIZE];
	_naui_net_tls_finished_mac(tls->server ? tls->client_handshake : tls->server_handshake, hash, mac);
	if (body.left != sizeof(mac) || !_naui_net_ct_equal(body.p, mac, sizeof(mac)))
		return _naui_net_tls_fail(tls, NET_TLS_ALERT_DECRYPT_ERROR);

	if (!tls->server)
	{
		_naui_net_tls_derive_application_keys(tls);
		if (tls->certificate_requested && !_naui_net_tls_send_certificate(tls))
			return false;

		if (!_naui_net_tls_send_finished(tls))
			return false;
	}

	tls->state = NET_TLS_DONE;
	tls->complete = true;
	return true;
}

static bool _naui_net_tls_receive(NetTls* tls, NetTlsLevel level, const uint8_t* message, size_t size)
{
	uint8_t type = message[0];
	NetReader body = _naui_net_reader(message + 4, size - 4);

	if (tls->state == NET_TLS_DONE)
	{
		if (level == NET_TLS_LEVEL_APPLICATION && !tls->server && type == NET_TLS_NEW_SESSION_TICKET)
			return true;

		return _naui_net_tls_fail(tls, NET_TLS_ALERT_UNEXPECTED_MESSAGE);
	}

	bool initial = tls->state == NET_TLS_WAIT_CLIENT_HELLO || tls->state == NET_TLS_WAIT_SERVER_HELLO;
	if (level != (initial ? NET_TLS_LEVEL_INITIAL : NET_TLS_LEVEL_HANDSHAKE))
		return _naui_net_tls_fail(tls, NET_TLS_ALERT_UNEXPECTED_MESSAGE);

	uint8_t hash[NET_SHA256_SIZE];
	_naui_net_sha256_final(&tls->transcript, hash);
	_naui_net_sha256_update(&tls->transcript, message, size);

	if (tls->state == NET_TLS_WAIT_CLIENT_HELLO && type == NET_TLS_CLIENT_HELLO)
		return _naui_net_tls_on_client_hello(tls, body);

	if (tls->state == NET_TLS_WAIT_SERVER_HELLO && type == NET_TLS_SERVER_HELLO)
		return _naui_net_tls_on_server_hello(tls, body);

	if (tls->state == NET_TLS_WAIT_ENCRYPTED_EXTENSIONS && type == NET_TLS_ENCRYPTED_EXTENSIONS)
		return _naui_net_tls_on_encrypted_extensions(tls, body);

	if (tls->state == NET_TLS_WAIT_CERTIFICATE && type == NET_TLS_CERTIFICATE_REQUEST && !tls->server)
		return _naui_net_tls_on_certificate_request(tls, body);

	if (tls->state == NET_TLS_WAIT_CERTIFICATE && type == NET_TLS_CERTIFICATE)
		return _naui_net_tls_on_certificate(tls, body);

	if (tls->state == NET_TLS_WAIT_CERTIFICATE_VERIFY && type == NET_TLS_CERTIFICATE_VERIFY)
		return _naui_net_tls_on_certificate_verify(tls, body, hash);

	if (tls->state == NET_TLS_WAIT_FINISHED && type == NET_TLS_FINISHED)
		return _naui_net_tls_on_finished(tls, body, hash);

	return _naui_net_tls_fail(tls, NET_TLS_ALERT_UNEXPECTED_MESSAGE);
}
