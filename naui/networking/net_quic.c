#define NET_QUIC_VERSION 1
#define NET_QUIC_CID_SIZE 8
#define NET_QUIC_MAX_CID_SIZE 20
#define NET_QUIC_DATAGRAM_SIZE 1200
#define NET_QUIC_MAX_TOKEN_SIZE 256
#define NET_QUIC_MAX_PACKETS 512
#define NET_QUIC_MAX_FLUSH_PACKETS 64

#define NET_QUIC_STREAM_WINDOW (1u << 20)
#define NET_QUIC_CRYPTO_WINDOW 16384
#define NET_QUIC_MAX_ACK_RANGES 32
#define NET_QUIC_MAX_SENT_ACK_RANGES 16
#define NET_QUIC_MAX_QUEUED_DATAGRAMS 256
#define NET_QUIC_PEER_DATAGRAM_FRAME_SIZE 65535

#define NET_QUIC_TOKEN_SIZE (8 + 1 + NET_QUIC_MAX_CID_SIZE + 16)
#define NET_QUIC_TOKEN_LIFETIME_MS 10000

#define NET_QUIC_INITIAL_RTT_MS 333
#define NET_QUIC_MAX_ACK_DELAY_MS 25
#define NET_QUIC_PACKET_THRESHOLD 3
#define NET_QUIC_MAX_PTO_SHIFT 8
#define NET_QUIC_INITIAL_CWND (10 * NET_QUIC_DATAGRAM_SIZE)
#define NET_QUIC_MIN_CWND (2 * NET_QUIC_DATAGRAM_SIZE)
#define NET_QUIC_MAX_CWND (16u << 20)

enum
{
	NET_QUIC_ERROR_CONNECTION_REFUSED = 0x02,
	NET_QUIC_ERROR_FLOW_CONTROL = 0x03,
	NET_QUIC_ERROR_STREAM_LIMIT = 0x04,
	NET_QUIC_ERROR_FRAME_ENCODING = 0x07,
	NET_QUIC_ERROR_TRANSPORT_PARAMETER = 0x08,
	NET_QUIC_ERROR_PROTOCOL_VIOLATION = 0x0a,
	NET_QUIC_ERROR_APPLICATION = 0x0c,
	NET_QUIC_ERROR_CRYPTO_BUFFER_EXCEEDED = 0x0d,
	NET_QUIC_ERROR_CRYPTO = 0x100,
};

enum
{
	NET_QUIC_PARAM_ORIGINAL_DCID = 0x00,
	NET_QUIC_PARAM_MAX_IDLE_TIMEOUT = 0x01,
	NET_QUIC_PARAM_MAX_UDP_PAYLOAD_SIZE = 0x03,
	NET_QUIC_PARAM_MAX_DATA = 0x04,
	NET_QUIC_PARAM_MAX_STREAM_DATA_BIDI_LOCAL = 0x05,
	NET_QUIC_PARAM_MAX_STREAM_DATA_BIDI_REMOTE = 0x06,
	NET_QUIC_PARAM_MAX_STREAMS_BIDI = 0x08,
	NET_QUIC_PARAM_ACK_DELAY_EXPONENT = 0x0a,
	NET_QUIC_PARAM_DISABLE_ACTIVE_MIGRATION = 0x0c,
	NET_QUIC_PARAM_INITIAL_SCID = 0x0f,
	NET_QUIC_PARAM_RETRY_SCID = 0x10,
	NET_QUIC_PARAM_MAX_DATAGRAM_FRAME_SIZE = 0x20,
};

typedef struct
{
	uint64_t start;
	uint64_t end;
} NetQuicRange;

typedef struct
{
	bool ready;
	bool aes;
	uint8_t secret[NET_SHA256_SIZE];
	uint8_t key[32];
	uint8_t iv[NET_AEAD_NONCE_SIZE];
	uint8_t hp[32];
} NetQuicKeys;

typedef struct
{
	uint64_t pn;
	uint64_t time_ms;
	uint64_t crypto_offset;
	uint64_t stream_offset;
	uint16_t crypto_size;
	uint16_t stream_size;
	uint16_t size;
	bool handshake_done;
	bool window_update;
	bool retire_cid;
} NetQuicSent;

typedef struct
{
	NetBuffer buffer;
	uint64_t base;
	uint64_t next;
	Naui_List(NetQuicRange) acked;
	Naui_List(NetQuicRange) lost;
} NetQuicSendStream;

typedef struct
{
	uint8_t* data;
	size_t window;
	uint64_t base;
	Naui_List(NetQuicRange) got;
} NetQuicRecvStream;

typedef struct
{
	NetQuicKeys rx;
	NetQuicKeys tx;
	bool discarded;
	bool ack_pending;
	bool probe;
	bool any_received;
	bool any_acked;

	uint64_t next_pn;
	uint64_t largest_received;
	uint64_t received_floor;
	uint64_t largest_acked;
	uint64_t loss_time_ms;
	uint64_t last_ack_eliciting_ms;

	Naui_List(NetQuicRange) received;
	Naui_List(NetQuicSent) sent;
	NetQuicSendStream crypto_tx;
	NetQuicRecvStream crypto_rx;
} NetQuicSpace;

typedef struct
{
	uint8_t* data;
	size_t size;
} NetQuicDatagram;

typedef struct
{
	uint8_t* data;
	size_t size;
	size_t cap;
} NetQuicWriter;

typedef struct NetQuic
{
	NetTls tls;
	NetQuicSpace spaces[NET_TLS_LEVEL_COUNT];

	uint8_t local_cid[NET_QUIC_CID_SIZE];
	uint8_t remote_cid[NET_QUIC_MAX_CID_SIZE];
	uint8_t original_dcid[NET_QUIC_MAX_CID_SIZE];
	uint8_t retry_scid[NET_QUIC_MAX_CID_SIZE];
	uint8_t remote_cid_size;
	uint8_t original_dcid_size;
	uint8_t retry_scid_size;
	uint64_t remote_cid_sequence;
	uint64_t retire_sequence;

	uint8_t* token;
	size_t token_size;

	bool server;
	bool retried;
	bool remote_cid_confirmed;
	bool complete;
	bool confirmed;
	bool stream_open;
	bool handshake_done_pending;
	bool window_update_pending;
	bool retire_cid_pending;
	bool ping_pending;
	bool path_response_pending;
	uint8_t path_response[8];
	uint8_t key_phase;
	uint64_t error;

	NetQuicSendStream stream_tx;
	NetQuicRecvStream stream_rx;
	uint64_t advertised_max_data;
	uint64_t peer_max_data;
	uint64_t peer_max_stream_data;
	uint64_t peer_max_datagram;
	uint64_t peer_ack_delay_exponent;
	Naui_List(NetQuicDatagram) datagrams;

	bool has_rtt;
	uint64_t srtt_ms;
	uint64_t rttvar_ms;
	uint64_t latest_rtt_ms;
	uint64_t min_rtt_ms;
	uint32_t pto_count;
	uint64_t timer_ms;
	uint64_t last_ack_eliciting_ms;

	uint64_t cwnd;
	uint64_t ssthresh;
	uint64_t bytes_in_flight;
	uint64_t recovery_start_ms;
} NetQuic;

static const uint8_t naui__net_quic_initial_salt[20] =
{
	0x38, 0x76, 0x2c, 0xf7, 0xf5, 0x59, 0x34, 0xb3, 0x4d, 0x17,
	0x9a, 0xe6, 0xa4, 0xc8, 0x0c, 0xad, 0xcc, 0xbb, 0x7f, 0x0a,
};

static const uint8_t naui__net_quic_retry_key[16] =
{
	0xbe, 0x0c, 0x69, 0x0b, 0x9f, 0x66, 0x57, 0x5a, 0x1d, 0x76, 0x6b, 0x54, 0xe3, 0x68, 0xc8, 0x4e,
};

static const uint8_t naui__net_quic_retry_nonce[12] =
{
	0x46, 0x15, 0x99, 0xd3, 0x5d, 0x63, 0x2b, 0xf2, 0x23, 0x98, 0x25, 0xbb,
};

static void naui__net_quic_flush(Naui_Net net, NetConnection* connection);

static void naui__net_ranges_add(Naui_List(NetQuicRange)* list, uint64_t start, uint64_t end)
{
	if (start >= end)
		return;

	ptrdiff_t count = naui_list_len(*list);
	ptrdiff_t first = 0;
	while (first < count && (*list)[first].end < start)
		first++;

	ptrdiff_t last = first;
	while (last < count && (*list)[last].start <= end)
	{
		if ((*list)[last].start < start)
			start = (*list)[last].start;

		if ((*list)[last].end > end)
			end = (*list)[last].end;

		last++;
	}

	NetQuicRange range = { start, end };
	if (last == first)
	{
		naui_list_insert(*list, range, first);
		return;
	}

	(*list)[first] = range;
	if (last - first > 1)
		arrdeln(*list, first + 1, last - first - 1);
}

static bool naui__net_ranges_contain(const NetQuicRange* list, uint64_t value)
{
	for (ptrdiff_t i = 0; i < naui_list_len(list); i++)
	{
		if (value >= list[i].start && value < list[i].end)
			return true;
	}

	return false;
}

static void naui__net_ranges_remove_below(Naui_List(NetQuicRange)* list, uint64_t floor)
{
	while (naui_list_len(*list) > 0 && (*list)[0].end <= floor)
		naui_list_remove(*list, 0);

	if (naui_list_len(*list) > 0 && (*list)[0].start < floor)
		(*list)[0].start = floor;
}

static size_t naui__net_quic_varint_size(uint64_t value)
{
	if (value < 0x40)
		return 1;

	if (value < 0x4000)
		return 2;

	if (value < 0x40000000)
		return 4;

	return 8;
}

static size_t naui__net_quic_put_varint(uint8_t* out, uint64_t value)
{
	size_t size = naui__net_quic_varint_size(value);
	for (size_t i = 0; i < size; i++)
	{
		out[size - 1 - i] = (uint8_t)(value >> (8 * i));
	}

	out[0] |= (uint8_t)((size == 1 ? 0 : size == 2 ? 1 : size == 4 ? 2 : 3) << 6);
	return size;
}

static uint64_t naui__net_quic_read_varint(NetReader* reader)
{
	const uint8_t* first = naui__net_read_bytes(reader, 1);
	if (!first)
		return 0;

	size_t extra = ((size_t)1 << (first[0] >> 6)) - 1;
	uint64_t value = first[0] & 0x3f;
	const uint8_t* rest = naui__net_read_bytes(reader, extra);
	for (size_t i = 0; rest && i < extra; i++)
	{
		value = (value << 8) | rest[i];
	}

	return value;
}

static void naui__net_quic_write_u8(NetQuicWriter* writer, uint8_t value)
{
	if (writer->size < writer->cap)
		writer->data[writer->size++] = value;
}

static void naui__net_quic_write_varint(NetQuicWriter* writer, uint64_t value)
{
	if (writer->cap - writer->size >= naui__net_quic_varint_size(value))
		writer->size += naui__net_quic_put_varint(writer->data + writer->size, value);
}

static void naui__net_quic_write_bytes(NetQuicWriter* writer, const void* data, size_t size)
{
	if (size == 0 || writer->cap - writer->size < size)
		return;

	memcpy(writer->data + writer->size, data, size);
	writer->size += size;
}

static size_t naui__net_quic_room(const NetQuicWriter* writer)
{
	return writer->cap - writer->size;
}

static void naui__net_quic_send_stream_free(NetQuicSendStream* stream)
{
	naui__net_buffer_free(&stream->buffer);
	naui_list_free(stream->acked);
	naui_list_free(stream->lost);
	memset(stream, 0, sizeof(*stream));
}

static uint64_t naui__net_quic_send_stream_end(const NetQuicSendStream* stream)
{
	return stream->base + naui__net_buffer_len(&stream->buffer);
}

static const uint8_t* naui__net_quic_send_stream_data(NetQuicSendStream* stream, uint64_t offset)
{
	return naui__net_buffer_ptr(&stream->buffer) + (offset - stream->base);
}

static bool naui__net_quic_send_stream_pick(NetQuicSendStream* stream, size_t max_size, uint64_t limit, uint64_t* out_offset, size_t* out_size)
{
	if (max_size == 0)
		return false;

	while (naui_list_len(stream->lost) > 0)
	{
		NetQuicRange* range = &stream->lost[0];
		if (range->start < stream->base)
			range->start = stream->base;

		uint64_t end = range->end;
		for (ptrdiff_t i = 0; i < naui_list_len(stream->acked); i++)
		{
			NetQuicRange acked = stream->acked[i];
			if (range->start >= acked.start && range->start < acked.end)
				range->start = acked.end;
			else if (acked.start > range->start && acked.start < end)
				end = acked.start;
		}

		if (range->start >= range->end)
		{
			naui_list_remove(stream->lost, 0);
			continue;
		}

		uint64_t size = end - range->start;
		if (size > max_size)
			size = max_size;

		*out_offset = range->start;
		*out_size = (size_t)size;
		range->start += size;
		return true;
	}

	uint64_t end = naui__net_quic_send_stream_end(stream);
	if (end > limit)
		end = limit;

	if (stream->next >= end)
		return false;

	uint64_t size = end - stream->next;
	if (size > max_size)
		size = max_size;

	*out_offset = stream->next;
	*out_size = (size_t)size;
	stream->next += size;
	return true;
}

static void naui__net_quic_send_stream_acked(NetQuicSendStream* stream, uint64_t offset, size_t size)
{
	naui__net_ranges_add(&stream->acked, offset, offset + size);
	while (naui_list_len(stream->acked) > 0 && stream->acked[0].start <= stream->base)
	{
		uint64_t end = stream->acked[0].end;
		if (end > stream->base)
		{
			naui__net_buffer_consume(&stream->buffer, (size_t)(end - stream->base));
			stream->base = end;
		}

		naui_list_remove(stream->acked, 0);
	}
}

static void naui__net_quic_recv_stream_free(NetQuicRecvStream* stream)
{
	free(stream->data);
	naui_list_free(stream->got);
	memset(stream, 0, sizeof(*stream));
}

static bool naui__net_quic_recv_stream_write(NetQuicRecvStream* stream, uint64_t offset, const uint8_t* data, size_t size)
{
	uint64_t end = offset + size;
	if (end < offset || end > stream->base + stream->window)
		return false;

	if (end <= stream->base)
		return true;

	if (offset < stream->base)
	{
		data += stream->base - offset;
		offset = stream->base;
	}

	if (!stream->data)
	{
		stream->data = (uint8_t*)malloc(stream->window);
		if (!stream->data)
			return false;
	}

	memcpy(stream->data + (offset - stream->base), data, (size_t)(end - offset));
	naui__net_ranges_add(&stream->got, offset, end);
	return true;
}

static size_t naui__net_quic_recv_stream_available(const NetQuicRecvStream* stream)
{
	if (naui_list_len(stream->got) == 0 || stream->got[0].start > stream->base)
		return 0;

	return (size_t)(stream->got[0].end - stream->base);
}

static void naui__net_quic_recv_stream_consume(NetQuicRecvStream* stream, size_t size)
{
	uint64_t extent = stream->got[naui_list_len(stream->got) - 1].end - stream->base;
	memmove(stream->data, stream->data + size, (size_t)extent - size);
	stream->base += size;
	naui__net_ranges_remove_below(&stream->got, stream->base);
}

static void naui__net_quic_keys_derive(NetQuicKeys* keys, const uint8_t secret[NET_SHA256_SIZE], bool aes)
{
	size_t key_size = aes ? 16 : 32;
	keys->aes = aes;
	memcpy(keys->secret, secret, NET_SHA256_SIZE);
	naui__net_hkdf_expand_label(secret, "quic key", NULL, 0, keys->key, key_size);
	naui__net_hkdf_expand_label(secret, "quic iv", NULL, 0, keys->iv, sizeof(keys->iv));
	naui__net_hkdf_expand_label(secret, "quic hp", NULL, 0, keys->hp, key_size);
	keys->ready = true;
}

static void naui__net_quic_keys_update(NetQuicKeys* next, const NetQuicKeys* current)
{
	uint8_t secret[NET_SHA256_SIZE];
	naui__net_hkdf_expand_label(current->secret, "quic ku", NULL, 0, secret, sizeof(secret));

	*next = *current;
	memcpy(next->secret, secret, sizeof(secret));
	naui__net_hkdf_expand_label(secret, "quic key", NULL, 0, next->key, 32);
	naui__net_hkdf_expand_label(secret, "quic iv", NULL, 0, next->iv, sizeof(next->iv));
	naui__net_wipe(secret, sizeof(secret));
}

static void naui__net_quic_initial_keys(NetQuic* quic, const uint8_t* dcid, size_t dcid_size)
{
	uint8_t initial[NET_SHA256_SIZE];
	uint8_t client[NET_SHA256_SIZE];
	uint8_t server[NET_SHA256_SIZE];
	naui__net_hkdf_extract(naui__net_quic_initial_salt, sizeof(naui__net_quic_initial_salt), dcid, dcid_size, initial);
	naui__net_hkdf_expand_label(initial, "client in", NULL, 0, client, sizeof(client));
	naui__net_hkdf_expand_label(initial, "server in", NULL, 0, server, sizeof(server));

	NetQuicSpace* space = &quic->spaces[NET_TLS_LEVEL_INITIAL];
	naui__net_quic_keys_derive(&space->rx, quic->server ? client : server, true);
	naui__net_quic_keys_derive(&space->tx, quic->server ? server : client, true);
}

static void naui__net_quic_header_mask(const NetQuicKeys* keys, const uint8_t sample[16], uint8_t mask[5])
{
	if (keys->aes)
	{
		uint8_t block[16];
		naui__net_aes128_encrypt(keys->hp, sample, block);
		memcpy(mask, block, 5);
		return;
	}

	uint8_t zero[5] = { 0 };
	naui__net_chacha20_xor(keys->hp, naui__net_load32_le(sample), sample + 4, zero, mask, 5);
}

static void naui__net_quic_nonce(const NetQuicKeys* keys, uint64_t pn, uint8_t nonce[NET_AEAD_NONCE_SIZE])
{
	memcpy(nonce, keys->iv, NET_AEAD_NONCE_SIZE);
	for (int i = 0; i < 8; i++)
	{
		nonce[NET_AEAD_NONCE_SIZE - 1 - i] ^= (uint8_t)(pn >> (8 * i));
	}
}

static uint64_t naui__net_quic_decode_pn(uint64_t expected, uint64_t truncated, int bits)
{
	uint64_t window = (uint64_t)1 << bits;
	uint64_t half = window / 2;
	uint64_t candidate = (expected & ~(window - 1)) | truncated;

	if (candidate + half <= expected && candidate < ((uint64_t)1 << 62) - window)
		return candidate + window;

	if (candidate > expected + half && candidate >= window)
		return candidate - window;

	return candidate;
}

static void naui__net_quic_space_discard(NetQuic* quic, NetQuicSpace* space)
{
	if (space->discarded)
		return;

	for (ptrdiff_t i = 0; i < naui_list_len(space->sent); i++)
	{
		quic->bytes_in_flight -= space->sent[i].size;
	}

	naui_list_free(space->received);
	naui_list_free(space->sent);
	naui__net_quic_send_stream_free(&space->crypto_tx);
	naui__net_quic_recv_stream_free(&space->crypto_rx);
	naui__net_wipe(&space->rx, sizeof(space->rx));
	naui__net_wipe(&space->tx, sizeof(space->tx));
	space->discarded = true;
	space->ack_pending = false;
	space->probe = false;
	space->loss_time_ms = 0;
}

static void naui__net_quic_free(NetConnection* connection)
{
	NetQuic* quic = connection->quic;
	if (!quic)
		return;

	for (int i = 0; i < NET_TLS_LEVEL_COUNT; i++)
	{
		naui__net_quic_space_discard(quic, &quic->spaces[i]);
	}

	for (ptrdiff_t i = 0; i < naui_list_len(quic->datagrams); i++)
	{
		free(quic->datagrams[i].data);
	}

	naui_list_free(quic->datagrams);
	naui__net_quic_send_stream_free(&quic->stream_tx);
	naui__net_quic_recv_stream_free(&quic->stream_rx);
	naui__net_tls_free(&quic->tls);
	free(quic->token);
	naui__net_wipe(quic, sizeof(*quic));
	free(quic);
	connection->quic = NULL;
}

static NetQuic* naui__net_quic_new(Naui_Net net, NetConnection* connection, bool server)
{
	NetQuic* quic = (NetQuic*)calloc(1, sizeof(*quic));
	if (!quic)
		return NULL;

	quic->server = server;
	quic->srtt_ms = NET_QUIC_INITIAL_RTT_MS;
	quic->rttvar_ms = NET_QUIC_INITIAL_RTT_MS / 2;
	quic->cwnd = NET_QUIC_INITIAL_CWND;
	quic->ssthresh = UINT64_MAX;
	quic->advertised_max_data = NET_QUIC_STREAM_WINDOW;
	quic->stream_rx.window = NET_QUIC_STREAM_WINDOW;
	quic->peer_ack_delay_exponent = 3;
	for (int i = 0; i < NET_TLS_LEVEL_COUNT; i++)
	{
		quic->spaces[i].crypto_rx.window = NET_QUIC_CRYPTO_WINDOW;
	}

	naui__net_tls_init(&quic->tls, server, net->device_key.bytes, net->local.id.bytes);
	connection->quic = quic;
	return quic;
}

static void naui__net_quic_write_param(NetQuicWriter* writer, uint64_t id, uint64_t value)
{
	naui__net_quic_write_varint(writer, id);
	naui__net_quic_write_varint(writer, naui__net_quic_varint_size(value));
	naui__net_quic_write_varint(writer, value);
}

static void naui__net_quic_write_param_bytes(NetQuicWriter* writer, uint64_t id, const uint8_t* data, size_t size)
{
	naui__net_quic_write_varint(writer, id);
	naui__net_quic_write_varint(writer, size);
	naui__net_quic_write_bytes(writer, data, size);
}

static void naui__net_quic_encode_params(Naui_Net net, NetQuic* quic)
{
	NetQuicWriter writer = { quic->tls.local_params, 0, sizeof(quic->tls.local_params) };
	if (quic->server)
	{
		naui__net_quic_write_param_bytes(&writer, NET_QUIC_PARAM_ORIGINAL_DCID, quic->original_dcid, quic->original_dcid_size);
		naui__net_quic_write_param_bytes(&writer, NET_QUIC_PARAM_RETRY_SCID, quic->local_cid, sizeof(quic->local_cid));
		naui__net_quic_write_param(&writer, NET_QUIC_PARAM_MAX_STREAMS_BIDI, 1);
	}

	naui__net_quic_write_param(&writer, NET_QUIC_PARAM_MAX_IDLE_TIMEOUT, net->config.idle_timeout_ms);
	naui__net_quic_write_param(&writer, NET_QUIC_PARAM_MAX_DATA, NET_QUIC_STREAM_WINDOW);
	naui__net_quic_write_param(&writer, NET_QUIC_PARAM_MAX_STREAM_DATA_BIDI_LOCAL, NET_QUIC_STREAM_WINDOW);
	naui__net_quic_write_param(&writer, NET_QUIC_PARAM_MAX_STREAM_DATA_BIDI_REMOTE, NET_QUIC_STREAM_WINDOW);
	naui__net_quic_write_param_bytes(&writer, NET_QUIC_PARAM_DISABLE_ACTIVE_MIGRATION, NULL, 0);
	naui__net_quic_write_param_bytes(&writer, NET_QUIC_PARAM_INITIAL_SCID, quic->local_cid, sizeof(quic->local_cid));
	naui__net_quic_write_param(&writer, NET_QUIC_PARAM_MAX_DATAGRAM_FRAME_SIZE, NET_QUIC_PEER_DATAGRAM_FRAME_SIZE);
	quic->tls.local_params_size = writer.size;
}

static bool naui__net_quic_cid_matches(NetReader value, const uint8_t* cid, size_t size)
{
	return value.left == size && memcmp(value.p, cid, size) == 0;
}

static bool naui__net_quic_parse_params(NetQuic* quic)
{
	NetReader reader = naui__net_reader(quic->tls.peer_params, quic->tls.peer_params_size);
	bool original_ok = false;
	bool initial_ok = false;
	bool retry_ok = !quic->retried;
	uint64_t streams = 0;
	uint64_t bidi_local = 0;
	uint64_t bidi_remote = 0;

	while (reader.left > 0)
	{
		uint64_t id = naui__net_quic_read_varint(&reader);
		size_t size = (size_t)naui__net_quic_read_varint(&reader);
		const uint8_t* data = naui__net_read_bytes(&reader, size);
		if (!data)
			return false;

		NetReader value = naui__net_reader(data, size);
		if (id == NET_QUIC_PARAM_ORIGINAL_DCID)
			original_ok = naui__net_quic_cid_matches(value, quic->original_dcid, quic->original_dcid_size);
		else if (id == NET_QUIC_PARAM_INITIAL_SCID)
			initial_ok = naui__net_quic_cid_matches(value, quic->remote_cid, quic->remote_cid_size);
		else if (id == NET_QUIC_PARAM_RETRY_SCID)
			retry_ok = quic->retried && naui__net_quic_cid_matches(value, quic->retry_scid, quic->retry_scid_size);
		else if (id == NET_QUIC_PARAM_MAX_DATA)
			quic->peer_max_data = naui__net_quic_read_varint(&value);
		else if (id == NET_QUIC_PARAM_MAX_STREAM_DATA_BIDI_LOCAL)
			bidi_local = naui__net_quic_read_varint(&value);
		else if (id == NET_QUIC_PARAM_MAX_STREAM_DATA_BIDI_REMOTE)
			bidi_remote = naui__net_quic_read_varint(&value);
		else if (id == NET_QUIC_PARAM_MAX_STREAMS_BIDI)
			streams = naui__net_quic_read_varint(&value);
		else if (id == NET_QUIC_PARAM_MAX_DATAGRAM_FRAME_SIZE)
			quic->peer_max_datagram = naui__net_quic_read_varint(&value);
		else if (id == NET_QUIC_PARAM_ACK_DELAY_EXPONENT)
			quic->peer_ack_delay_exponent = naui__net_quic_read_varint(&value);
		else if (id == NET_QUIC_PARAM_MAX_UDP_PAYLOAD_SIZE && naui__net_quic_read_varint(&value) < NET_QUIC_DATAGRAM_SIZE)
			return false;

		if (!value.ok)
			return false;
	}

	if (!initial_ok || quic->peer_ack_delay_exponent > 20)
		return false;

	if (quic->server)
	{
		quic->peer_max_stream_data = bidi_local;
		return true;
	}

	quic->peer_max_stream_data = bidi_remote;
	return original_ok && retry_ok && streams >= 1;
}

static size_t naui__net_quic_write_header(const NetQuic* quic, NetTlsLevel level, uint64_t pn, uint8_t* out, size_t* length_offset)
{
	size_t n = 0;
	if (level == NET_TLS_LEVEL_APPLICATION)
	{
		out[n++] = (uint8_t)(0x43 | (quic->key_phase << 2));
		memcpy(out + n, quic->remote_cid, quic->remote_cid_size);
		n += quic->remote_cid_size;
	}
	else
	{
		out[n++] = level == NET_TLS_LEVEL_HANDSHAKE ? 0xe3 : 0xc3;
		naui__net_put_u32(out + n, NET_QUIC_VERSION);
		n += 4;
		out[n++] = quic->remote_cid_size;
		memcpy(out + n, quic->remote_cid, quic->remote_cid_size);
		n += quic->remote_cid_size;
		out[n++] = NET_QUIC_CID_SIZE;
		memcpy(out + n, quic->local_cid, NET_QUIC_CID_SIZE);
		n += NET_QUIC_CID_SIZE;

		if (level == NET_TLS_LEVEL_INITIAL)
		{
			n += naui__net_quic_put_varint(out + n, quic->token_size);
			if (quic->token_size)
				memcpy(out + n, quic->token, quic->token_size);

			n += quic->token_size;
		}

		*length_offset = n;
		n += 2;
	}

	naui__net_put_u32(out + n, (uint32_t)pn);
	return n + 4;
}

static void naui__net_quic_write_ack(NetQuicSpace* space, NetQuicWriter* writer)
{
	ptrdiff_t count = naui_list_len(space->received);
	if (count == 0 || naui__net_quic_room(writer) < 64 + 16 * NET_QUIC_MAX_SENT_ACK_RANGES)
		return;

	ptrdiff_t extra = count - 1;
	if (extra > NET_QUIC_MAX_SENT_ACK_RANGES - 1)
		extra = NET_QUIC_MAX_SENT_ACK_RANGES - 1;

	NetQuicRange top = space->received[count - 1];
	naui__net_quic_write_u8(writer, 0x02);
	naui__net_quic_write_varint(writer, top.end - 1);
	naui__net_quic_write_varint(writer, 0);
	naui__net_quic_write_varint(writer, (uint64_t)extra);
	naui__net_quic_write_varint(writer, top.end - top.start - 1);

	for (ptrdiff_t i = 0; i < extra; i++)
	{
		NetQuicRange above = space->received[count - 1 - i];
		NetQuicRange range = space->received[count - 2 - i];
		naui__net_quic_write_varint(writer, above.start - range.end - 1);
		naui__net_quic_write_varint(writer, range.end - range.start - 1);
	}

	space->ack_pending = false;
}

static void naui__net_quic_seal_and_send(Naui_Net net, NetConnection* connection, NetTlsLevel level, uint8_t* datagram, size_t header_size, size_t length_offset, const uint8_t* plain, size_t plain_size, uint64_t pn)
{
	NetQuic* quic = connection->quic;
	NetQuicSpace* space = &quic->spaces[level];
	size_t pn_offset = header_size - 4;

	if (level != NET_TLS_LEVEL_APPLICATION)
	{
		size_t length = 4 + plain_size + NET_AEAD_TAG_SIZE;
		datagram[length_offset] = (uint8_t)(0x40 | (length >> 8));
		datagram[length_offset + 1] = (uint8_t)length;
	}

	uint8_t nonce[NET_AEAD_NONCE_SIZE];
	uint8_t mask[5];
	naui__net_quic_nonce(&space->tx, pn, nonce);
	naui__net_aead_seal(space->tx.aes, space->tx.key, nonce, datagram, header_size, plain, plain_size, datagram + header_size);
	naui__net_quic_header_mask(&space->tx, datagram + header_size, mask);

	datagram[0] ^= mask[0] & (level == NET_TLS_LEVEL_APPLICATION ? 0x1f : 0x0f);
	for (int i = 0; i < 4; i++)
	{
		datagram[pn_offset + i] ^= mask[1 + i];
	}

	size_t total = header_size + plain_size + NET_AEAD_TAG_SIZE;
	size_t sent = 0;
	naui_socket_sendto(&net->quic, datagram, total, &connection->remote, &sent);
	connection->bytes_sent += total;
}

static void naui__net_quic_requeue(NetQuic* quic, NetQuicSpace* space, const NetQuicSent* sent)
{
	if (sent->crypto_size)
		naui__net_ranges_add(&space->crypto_tx.lost, sent->crypto_offset, sent->crypto_offset + sent->crypto_size);

	if (sent->stream_size)
		naui__net_ranges_add(&quic->stream_tx.lost, sent->stream_offset, sent->stream_offset + sent->stream_size);

	quic->handshake_done_pending |= sent->handshake_done;
	quic->window_update_pending |= sent->window_update;
	quic->retire_cid_pending |= sent->retire_cid;
}

static void naui__net_quic_write_application_frames(Naui_Net net, NetConnection* connection, NetQuicWriter* writer, NetQuicSent* sent)
{
	NetQuic* quic = connection->quic;
	if (quic->handshake_done_pending && naui__net_quic_room(writer) >= 1)
	{
		naui__net_quic_write_u8(writer, 0x1e);
		quic->handshake_done_pending = false;
		sent->handshake_done = true;
	}

	if (quic->window_update_pending && naui__net_quic_room(writer) >= 20)
	{
		quic->advertised_max_data = quic->stream_rx.base + NET_QUIC_STREAM_WINDOW;
		naui__net_quic_write_u8(writer, 0x10);
		naui__net_quic_write_varint(writer, quic->advertised_max_data);
		naui__net_quic_write_u8(writer, 0x11);
		naui__net_quic_write_varint(writer, 0);
		naui__net_quic_write_varint(writer, quic->advertised_max_data);
		quic->window_update_pending = false;
		sent->window_update = true;
	}

	if (quic->retire_cid_pending && naui__net_quic_room(writer) >= 9)
	{
		naui__net_quic_write_u8(writer, 0x19);
		naui__net_quic_write_varint(writer, quic->retire_sequence);
		quic->retire_cid_pending = false;
		sent->retire_cid = true;
	}

	if (quic->path_response_pending && naui__net_quic_room(writer) >= 9)
	{
		naui__net_quic_write_u8(writer, 0x1b);
		naui__net_quic_write_bytes(writer, quic->path_response, sizeof(quic->path_response));
		quic->path_response_pending = false;
	}

	if (quic->ping_pending && naui__net_quic_room(writer) >= 1)
	{
		naui__net_quic_write_u8(writer, 0x01);
		quic->ping_pending = false;
	}

	while (naui_list_len(quic->datagrams) > 0 && naui__net_quic_room(writer) >= quic->datagrams[0].size + 3)
	{
		NetQuicDatagram datagram = quic->datagrams[0];
		naui__net_quic_write_u8(writer, 0x31);
		naui__net_quic_write_varint(writer, datagram.size);
		naui__net_quic_write_bytes(writer, datagram.data, datagram.size);
		free(datagram.data);
		naui_list_remove(quic->datagrams, 0);
	}

	uint64_t limit = quic->peer_max_data < quic->peer_max_stream_data ? quic->peer_max_data : quic->peer_max_stream_data;
	uint64_t offset = 0;
	size_t size = 0;
	if (!quic->stream_open || naui__net_quic_room(writer) < 16)
		return;

	if (!naui__net_quic_send_stream_pick(&quic->stream_tx, naui__net_quic_room(writer) - 12, limit, &offset, &size))
		return;

	naui__net_quic_write_u8(writer, 0x0e);
	naui__net_quic_write_varint(writer, 0);
	naui__net_quic_write_varint(writer, offset);
	naui__net_quic_write_varint(writer, size);
	naui__net_quic_write_bytes(writer, naui__net_quic_send_stream_data(&quic->stream_tx, offset), size);
	sent->stream_offset = offset;
	sent->stream_size = (uint16_t)size;
}

static bool naui__net_quic_flush_level(Naui_Net net, NetConnection* connection, NetTlsLevel level)
{
	NetQuic* quic = connection->quic;
	NetQuicSpace* space = &quic->spaces[level];
	if (space->discarded || !space->tx.ready)
		return false;

	uint8_t datagram[NET_QUIC_DATAGRAM_SIZE + NET_QUIC_MAX_TOKEN_SIZE + 64];
	uint8_t plain[NET_QUIC_DATAGRAM_SIZE] = { 0 };
	size_t length_offset = 0;
	uint64_t pn = space->next_pn;
	size_t header_size = naui__net_quic_write_header(quic, level, pn, datagram, &length_offset);
	if (header_size + NET_AEAD_TAG_SIZE + 64 > NET_QUIC_DATAGRAM_SIZE)
		return false;

	NetQuicWriter writer = { plain, 0, NET_QUIC_DATAGRAM_SIZE - header_size - NET_AEAD_TAG_SIZE };
	NetQuicSent sent = { .pn = pn, .time_ms = net->now_ms };

	if (space->probe && naui_list_len(space->sent) > 0)
		naui__net_quic_requeue(quic, space, &space->sent[0]);

	if (space->ack_pending)
		naui__net_quic_write_ack(space, &writer);

	size_t ack_size = writer.size;
	if (space->probe || quic->bytes_in_flight + NET_QUIC_DATAGRAM_SIZE <= quic->cwnd)
	{
		uint64_t offset = 0;
		size_t size = 0;
		if (naui__net_quic_room(&writer) > 16 && naui__net_quic_send_stream_pick(&space->crypto_tx, naui__net_quic_room(&writer) - 12, UINT64_MAX, &offset, &size))
		{
			naui__net_quic_write_u8(&writer, 0x06);
			naui__net_quic_write_varint(&writer, offset);
			naui__net_quic_write_varint(&writer, size);
			naui__net_quic_write_bytes(&writer, naui__net_quic_send_stream_data(&space->crypto_tx, offset), size);
			sent.crypto_offset = offset;
			sent.crypto_size = (uint16_t)size;
		}

		if (level == NET_TLS_LEVEL_APPLICATION && quic->complete)
			naui__net_quic_write_application_frames(net, connection, &writer, &sent);

		if (space->probe && writer.size == ack_size)
			naui__net_quic_write_u8(&writer, 0x01);
	}

	if (writer.size == 0)
		return false;

	bool ack_eliciting = writer.size > ack_size;
	if (level == NET_TLS_LEVEL_INITIAL)
		writer.size = writer.cap;

	space->next_pn++;
	space->probe = false;
	naui__net_quic_seal_and_send(net, connection, level, datagram, header_size, length_offset, plain, writer.size, pn);
	if (!ack_eliciting)
		return true;

	sent.size = (uint16_t)(header_size + writer.size + NET_AEAD_TAG_SIZE);
	naui_list_push(space->sent, sent);
	quic->bytes_in_flight += sent.size;
	space->last_ack_eliciting_ms = net->now_ms;
	quic->last_ack_eliciting_ms = net->now_ms;
	connection->last_send_ms = net->now_ms;
	return true;
}

static void naui__net_quic_set_timer(Naui_Net net, NetConnection* connection)
{
	NetQuic* quic = connection->quic;
	quic->timer_ms = 0;

	uint64_t base = 0;
	bool in_flight = false;
	for (int i = 0; i < NET_TLS_LEVEL_COUNT; i++)
	{
		const NetQuicSpace* space = &quic->spaces[i];
		if (space->discarded)
			continue;

		if (space->loss_time_ms && (!quic->timer_ms || space->loss_time_ms < quic->timer_ms))
			quic->timer_ms = space->loss_time_ms;

		if (naui_list_len(space->sent) == 0 || in_flight)
			continue;

		if (i == NET_TLS_LEVEL_APPLICATION && !quic->complete)
			continue;

		in_flight = true;
		base = space->last_ack_eliciting_ms;
	}

	if (quic->timer_ms)
		return;

	if (!in_flight)
	{
		if (quic->server || quic->confirmed)
			return;

		base = quic->last_ack_eliciting_ms ? quic->last_ack_eliciting_ms : net->now_ms;
	}

	uint64_t variance = quic->rttvar_ms * 4;
	uint64_t pto = quic->srtt_ms + (variance > 1 ? variance : 1) + NET_QUIC_MAX_ACK_DELAY_MS;
	uint32_t shift = quic->pto_count < NET_QUIC_MAX_PTO_SHIFT ? quic->pto_count : NET_QUIC_MAX_PTO_SHIFT;
	quic->timer_ms = base + (pto << shift);
}

static void naui__net_quic_flush(Naui_Net net, NetConnection* connection)
{
	if (!connection->quic || connection->phase == NET_PHASE_CLOSED)
		return;

	int budget = NET_QUIC_MAX_FLUSH_PACKETS;
	for (int level = 0; level < NET_TLS_LEVEL_COUNT; level++)
	{
		while (budget > 0 && naui__net_quic_flush_level(net, connection, (NetTlsLevel)level))
			budget--;
	}

	naui__net_quic_set_timer(net, connection);
}

static void naui__net_quic_fail(Naui_Net net, NetConnection* connection, uint64_t error)
{
	connection->quic->error = error;
	naui__net_close_connection(net, connection, NAUI_DISCONNECT_PROTOCOL, true);
}

static void naui__net_quic_close(Naui_Net net, NetConnection* connection, bool notify_peer)
{
	NetQuic* quic = connection->quic;
	if (!quic || !notify_peer)
		return;

	NetTlsLevel level = NET_TLS_LEVEL_APPLICATION;
	while (level > NET_TLS_LEVEL_INITIAL && (!quic->spaces[level].tx.ready || quic->spaces[level].discarded))
		level--;

	NetQuicSpace* space = &quic->spaces[level];
	if (!space->tx.ready || space->discarded)
		return;

	uint8_t datagram[NET_QUIC_DATAGRAM_SIZE + NET_QUIC_MAX_TOKEN_SIZE + 64];
	uint8_t plain[NET_QUIC_DATAGRAM_SIZE] = { 0 };
	size_t length_offset = 0;
	uint64_t pn = space->next_pn++;
	size_t header_size = naui__net_quic_write_header(quic, level, pn, datagram, &length_offset);
	if (header_size + NET_AEAD_TAG_SIZE + 64 > NET_QUIC_DATAGRAM_SIZE)
		return;

	NetQuicWriter writer = { plain, 0, NET_QUIC_DATAGRAM_SIZE - header_size - NET_AEAD_TAG_SIZE };
	if (quic->error || level != NET_TLS_LEVEL_APPLICATION)
	{
		naui__net_quic_write_u8(&writer, 0x1c);
		naui__net_quic_write_varint(&writer, quic->error ? quic->error : NET_QUIC_ERROR_APPLICATION);
		naui__net_quic_write_varint(&writer, 0);
	}
	else
	{
		naui__net_quic_write_u8(&writer, 0x1d);
		naui__net_quic_write_varint(&writer, 0);
	}

	naui__net_quic_write_varint(&writer, 0);
	if (level == NET_TLS_LEVEL_INITIAL)
		writer.size = writer.cap;

	naui__net_quic_seal_and_send(net, connection, level, datagram, header_size, length_offset, plain, writer.size, pn);
}

static void naui__net_quic_on_rtt_sample(NetQuic* quic, uint64_t latest, uint64_t ack_delay)
{
	if (latest == 0)
		latest = 1;

	quic->latest_rtt_ms = latest;
	if (!quic->has_rtt)
	{
		quic->has_rtt = true;
		quic->min_rtt_ms = latest;
		quic->srtt_ms = latest;
		quic->rttvar_ms = latest / 2;
		return;
	}

	if (latest < quic->min_rtt_ms)
		quic->min_rtt_ms = latest;

	if (ack_delay > NET_QUIC_MAX_ACK_DELAY_MS)
		ack_delay = NET_QUIC_MAX_ACK_DELAY_MS;

	uint64_t adjusted = latest >= quic->min_rtt_ms + ack_delay ? latest - ack_delay : latest;
	uint64_t difference = quic->srtt_ms > adjusted ? quic->srtt_ms - adjusted : adjusted - quic->srtt_ms;
	quic->rttvar_ms = (3 * quic->rttvar_ms + difference) / 4;
	quic->srtt_ms = (7 * quic->srtt_ms + adjusted) / 8;
}

static void naui__net_quic_on_packet_lost(Naui_Net net, NetQuic* quic, NetQuicSpace* space, const NetQuicSent* sent)
{
	quic->bytes_in_flight -= sent->size;
	naui__net_quic_requeue(quic, space, sent);
	if (sent->time_ms <= quic->recovery_start_ms)
		return;

	quic->recovery_start_ms = net->now_ms;
	quic->ssthresh = quic->cwnd / 2;
	quic->cwnd = quic->ssthresh > NET_QUIC_MIN_CWND ? quic->ssthresh : NET_QUIC_MIN_CWND;
}

static void naui__net_quic_detect_lost(Naui_Net net, NetQuic* quic, NetQuicSpace* space)
{
	space->loss_time_ms = 0;
	if (!space->any_acked)
		return;

	uint64_t rtt = quic->srtt_ms > quic->latest_rtt_ms ? quic->srtt_ms : quic->latest_rtt_ms;
	uint64_t delay = rtt * 9 / 8;
	if (delay == 0)
		delay = 1;

	for (ptrdiff_t i = 0; i < naui_list_len(space->sent);)
	{
		NetQuicSent sent = space->sent[i];
		if (sent.pn > space->largest_acked)
			break;

		if (sent.pn + NET_QUIC_PACKET_THRESHOLD <= space->largest_acked || sent.time_ms + delay <= net->now_ms)
		{
			naui_list_remove(space->sent, i);
			naui__net_quic_on_packet_lost(net, quic, space, &sent);
			continue;
		}

		if (!space->loss_time_ms || sent.time_ms + delay < space->loss_time_ms)
			space->loss_time_ms = sent.time_ms + delay;

		i++;
	}
}

static void naui__net_quic_on_packet_acked(NetQuic* quic, NetQuicSpace* space, const NetQuicSent* sent)
{
	quic->bytes_in_flight -= sent->size;
	if (sent->crypto_size)
		naui__net_quic_send_stream_acked(&space->crypto_tx, sent->crypto_offset, sent->crypto_size);

	if (sent->stream_size)
		naui__net_quic_send_stream_acked(&quic->stream_tx, sent->stream_offset, sent->stream_size);

	if (sent->time_ms <= quic->recovery_start_ms)
		return;

	if (quic->cwnd < quic->ssthresh)
		quic->cwnd += sent->size;
	else
		quic->cwnd += (uint64_t)NET_QUIC_DATAGRAM_SIZE * sent->size / quic->cwnd;

	if (quic->cwnd > NET_QUIC_MAX_CWND)
		quic->cwnd = NET_QUIC_MAX_CWND;
}

static bool naui__net_quic_ack_range(Naui_Net net, NetQuic* quic, NetQuicSpace* space, uint64_t low, uint64_t high, uint64_t largest, uint64_t ack_delay)
{
	bool any = false;
	for (ptrdiff_t i = naui_list_len(space->sent) - 1; i >= 0; i--)
	{
		NetQuicSent sent = space->sent[i];
		if (sent.pn > high)
			continue;

		if (sent.pn < low)
			break;

		if (sent.pn == largest)
			naui__net_quic_on_rtt_sample(quic, net->now_ms - sent.time_ms, ack_delay);

		naui_list_remove(space->sent, i);
		naui__net_quic_on_packet_acked(quic, space, &sent);
		any = true;
	}

	return any;
}

static bool naui__net_quic_on_ack(Naui_Net net, NetConnection* connection, NetTlsLevel level, NetReader* reader, bool has_ecn)
{
	NetQuic* quic = connection->quic;
	NetQuicSpace* space = &quic->spaces[level];

	uint64_t largest = naui__net_quic_read_varint(reader);
	uint64_t delay = naui__net_quic_read_varint(reader);
	uint64_t count = naui__net_quic_read_varint(reader);
	uint64_t first = naui__net_quic_read_varint(reader);
	if (!reader->ok || largest >= space->next_pn || first > largest)
		return false;

	uint64_t ack_delay = level == NET_TLS_LEVEL_APPLICATION ? (delay << quic->peer_ack_delay_exponent) / 1000 : 0;
	uint64_t high = largest;
	uint64_t low = largest - first;
	bool any = naui__net_quic_ack_range(net, quic, space, low, high, largest, ack_delay);

	for (uint64_t i = 0; i < count; i++)
	{
		uint64_t gap = naui__net_quic_read_varint(reader);
		uint64_t length = naui__net_quic_read_varint(reader);
		if (!reader->ok || low < gap + 2)
			return false;

		high = low - gap - 2;
		if (length > high)
			return false;

		low = high - length;
		any |= naui__net_quic_ack_range(net, quic, space, low, high, largest, ack_delay);
	}

	if (has_ecn)
	{
		naui__net_quic_read_varint(reader);
		naui__net_quic_read_varint(reader);
		naui__net_quic_read_varint(reader);
	}

	if (!reader->ok)
		return false;

	if (!space->any_acked || largest > space->largest_acked)
		space->largest_acked = largest;

	space->any_acked = true;
	if (any)
	{
		quic->pto_count = 0;
		naui__net_quic_detect_lost(net, quic, space);
	}

	return true;
}

static void naui__net_quic_on_timeout(Naui_Net net, NetConnection* connection)
{
	NetQuic* quic = connection->quic;
	bool loss = false;
	for (int i = 0; i < NET_TLS_LEVEL_COUNT; i++)
	{
		NetQuicSpace* space = &quic->spaces[i];
		if (space->discarded || !space->loss_time_ms || space->loss_time_ms > net->now_ms)
			continue;

		naui__net_quic_detect_lost(net, quic, space);
		loss = true;
	}

	if (loss)
		return;

	quic->pto_count++;
	NetQuicSpace* probe = NULL;
	for (int i = 0; i < NET_TLS_LEVEL_COUNT && !probe; i++)
	{
		NetQuicSpace* space = &quic->spaces[i];
		if (!space->discarded && space->tx.ready && naui_list_len(space->sent) > 0)
			probe = space;
	}

	for (int i = NET_TLS_LEVEL_HANDSHAKE; i >= 0 && !probe; i--)
	{
		NetQuicSpace* space = &quic->spaces[i];
		if (!space->discarded && space->tx.ready)
			probe = space;
	}

	if (probe)
		probe->probe = true;

	quic->last_ack_eliciting_ms = net->now_ms;
}

static void naui__net_quic_install_handshake_keys(NetQuic* quic)
{
	NetQuicSpace* space = &quic->spaces[NET_TLS_LEVEL_HANDSHAKE];
	const NetTls* tls = &quic->tls;
	naui__net_quic_keys_derive(&space->rx, quic->server ? tls->client_handshake : tls->server_handshake, false);
	naui__net_quic_keys_derive(&space->tx, quic->server ? tls->server_handshake : tls->client_handshake, false);
	if (!quic->server)
		naui__net_quic_space_discard(quic, &quic->spaces[NET_TLS_LEVEL_INITIAL]);
}

static void naui__net_quic_complete(Naui_Net net, NetConnection* connection)
{
	NetQuic* quic = connection->quic;
	NetQuicSpace* space = &quic->spaces[NET_TLS_LEVEL_APPLICATION];
	const NetTls* tls = &quic->tls;

	if (!naui__net_quic_parse_params(quic))
	{
		naui__net_quic_fail(net, connection, NET_QUIC_ERROR_TRANSPORT_PARAMETER);
		return;
	}

	naui__net_quic_keys_derive(&space->rx, quic->server ? tls->client_application : tls->server_application, false);
	naui__net_quic_keys_derive(&space->tx, quic->server ? tls->server_application : tls->client_application, false);
	quic->complete = true;
	connection->verified = true;
	memcpy(connection->verified_id.bytes, tls->peer_key, NAUI_DEVICE_ID_SIZE);

	if (quic->server)
	{
		quic->confirmed = true;
		quic->handshake_done_pending = true;
		naui__net_quic_space_discard(quic, &quic->spaces[NET_TLS_LEVEL_HANDSHAKE]);
	}
	else
	{
		quic->stream_open = true;
	}

	uint8_t hello[NET_HELLO_MAX];
	size_t size = naui__net_hello_encode(&net->local, hello);
	naui__net_frame_append(net, connection, &quic->stream_tx.buffer, NET_FRAME_HELLO, hello, size);
}

static void naui__net_quic_advance_handshake(Naui_Net net, NetConnection* connection)
{
	NetQuic* quic = connection->quic;
	NetTls* tls = &quic->tls;

	for (int level = 0; level < NET_TLS_LEVEL_COUNT; level++)
	{
		NetQuicSpace* space = &quic->spaces[level];
		while (!space->discarded)
		{
			NetQuicRecvStream* stream = &space->crypto_rx;
			size_t available = naui__net_quic_recv_stream_available(stream);
			if (available < 4)
				break;

			size_t size = 4 + (naui__net_get_u32(stream->data) & 0xffffff);
			if (size > NET_QUIC_CRYPTO_WINDOW)
			{
				naui__net_quic_fail(net, connection, NET_QUIC_ERROR_CRYPTO_BUFFER_EXCEEDED);
				return;
			}

			if (available < size)
				break;

			bool ok = naui__net_tls_receive(tls, (NetTlsLevel)level, stream->data, size);
			naui__net_quic_recv_stream_consume(stream, size);
			if (!ok)
			{
				naui__net_quic_fail(net, connection, NET_QUIC_ERROR_CRYPTO + tls->alert);
				return;
			}

			for (int out = 0; out < NET_TLS_LEVEL_COUNT; out++)
			{
				NetBuffer* pending = &tls->out[out];
				if (!quic->spaces[out].discarded)
					naui__net_buffer_append(&quic->spaces[out].crypto_tx.buffer, naui__net_buffer_ptr(pending), naui__net_buffer_len(pending));

				naui__net_buffer_free(pending);
			}

			if (tls->handshake_keys_ready && !quic->spaces[NET_TLS_LEVEL_HANDSHAKE].tx.ready && !quic->spaces[NET_TLS_LEVEL_HANDSHAKE].discarded)
				naui__net_quic_install_handshake_keys(quic);

			if (tls->complete && !quic->complete)
			{
				naui__net_quic_complete(net, connection);
				if (connection->phase == NET_PHASE_CLOSED)
					return;
			}
		}
	}
}

static uint64_t naui__net_quic_on_stream(Naui_Net net, NetConnection* connection, uint64_t type, NetReader* reader)
{
	NetQuic* quic = connection->quic;
	uint64_t id = naui__net_quic_read_varint(reader);
	uint64_t offset = type & 0x04 ? naui__net_quic_read_varint(reader) : 0;
	size_t size = type & 0x02 ? (size_t)naui__net_quic_read_varint(reader) : reader->left;
	const uint8_t* data = naui__net_read_bytes(reader, size);
	if (!data)
		return NET_QUIC_ERROR_FRAME_ENCODING;

	if (id != 0)
		return NET_QUIC_ERROR_STREAM_LIMIT;

	if (!naui__net_quic_recv_stream_write(&quic->stream_rx, offset, data, size))
		return NET_QUIC_ERROR_FLOW_CONTROL;

	quic->stream_open = true;
	if (type & 0x01)
		naui__net_close_connection(net, connection, NAUI_DISCONNECT_REMOTE, true);

	return 0;
}

static uint64_t naui__net_quic_on_new_connection_id(NetQuic* quic, NetReader* reader)
{
	uint64_t sequence = naui__net_quic_read_varint(reader);
	uint64_t retire_prior_to = naui__net_quic_read_varint(reader);
	size_t size = naui__net_read_uint(reader, 1);
	const uint8_t* cid = naui__net_read_bytes(reader, size);
	naui__net_read_bytes(reader, 16);
	if (!reader->ok)
		return NET_QUIC_ERROR_FRAME_ENCODING;

	if (size < 1 || size > NET_QUIC_MAX_CID_SIZE || retire_prior_to > sequence)
		return NET_QUIC_ERROR_FRAME_ENCODING;

	if (retire_prior_to <= quic->remote_cid_sequence || sequence <= quic->remote_cid_sequence)
		return 0;

	quic->retire_sequence = quic->remote_cid_sequence;
	quic->retire_cid_pending = true;
	quic->remote_cid_sequence = sequence;
	quic->remote_cid_size = (uint8_t)size;
	memcpy(quic->remote_cid, cid, size);
	return 0;
}

static void naui__net_quic_on_remote_close(Naui_Net net, NetConnection* connection, uint64_t type, NetReader* reader)
{
	uint64_t code = naui__net_quic_read_varint(reader);
	if (type == 0x1c)
		naui__net_quic_read_varint(reader);

	bool refused = type == 0x1c && code == NET_QUIC_ERROR_CONNECTION_REFUSED;
	naui__net_close_connection(net, connection, refused ? NAUI_DISCONNECT_REJECTED : NAUI_DISCONNECT_REMOTE, false);
}

static uint64_t naui__net_quic_on_frames(Naui_Net net, NetConnection* connection, NetTlsLevel level, const uint8_t* payload, size_t size, bool* ack_eliciting)
{
	NetQuic* quic = connection->quic;
	NetReader reader = naui__net_reader(payload, size);
	if (size == 0)
		return NET_QUIC_ERROR_PROTOCOL_VIOLATION;

	while (reader.left > 0 && connection->phase != NET_PHASE_CLOSED)
	{
		uint64_t type = naui__net_quic_read_varint(&reader);
		uint64_t error = 0;
		if (type == 0x00)
			continue;

		if (type == 0x02 || type == 0x03)
		{
			if (!naui__net_quic_on_ack(net, connection, level, &reader, type == 0x03))
				return NET_QUIC_ERROR_PROTOCOL_VIOLATION;

			continue;
		}

		if (type == 0x1c || (type == 0x1d && level == NET_TLS_LEVEL_APPLICATION))
		{
			naui__net_quic_on_remote_close(net, connection, type, &reader);
			return 0;
		}

		*ack_eliciting = true;
		if (type == 0x01)
			continue;

		if (type == 0x06)
		{
			uint64_t offset = naui__net_quic_read_varint(&reader);
			size_t length = (size_t)naui__net_quic_read_varint(&reader);
			const uint8_t* data = naui__net_read_bytes(&reader, length);
			if (!data)
				return NET_QUIC_ERROR_FRAME_ENCODING;

			if (!naui__net_quic_recv_stream_write(&quic->spaces[level].crypto_rx, offset, data, length))
				return NET_QUIC_ERROR_CRYPTO_BUFFER_EXCEEDED;

			continue;
		}

		if (level != NET_TLS_LEVEL_APPLICATION)
			return NET_QUIC_ERROR_PROTOCOL_VIOLATION;

		if (type >= 0x08 && type <= 0x0f)
			error = naui__net_quic_on_stream(net, connection, type, &reader);
		else if (type == 0x10)
		{
			uint64_t value = naui__net_quic_read_varint(&reader);
			if (value > quic->peer_max_data)
				quic->peer_max_data = value;
		}
		else if (type == 0x11)
		{
			uint64_t id = naui__net_quic_read_varint(&reader);
			uint64_t value = naui__net_quic_read_varint(&reader);
			if (id == 0 && value > quic->peer_max_stream_data)
				quic->peer_max_stream_data = value;
		}
		else if (type == 0x12 || type == 0x13 || type == 0x14 || type == 0x16 || type == 0x17 || type == 0x19)
			naui__net_quic_read_varint(&reader);
		else if (type == 0x15)
		{
			naui__net_quic_read_varint(&reader);
			naui__net_quic_read_varint(&reader);
		}
		else if (type == 0x07 && !quic->server)
			naui__net_read_bytes(&reader, (size_t)naui__net_quic_read_varint(&reader));
		else if (type == 0x18)
			error = naui__net_quic_on_new_connection_id(quic, &reader);
		else if (type == 0x1a)
		{
			const uint8_t* data = naui__net_read_bytes(&reader, 8);
			if (data)
			{
				memcpy(quic->path_response, data, 8);
				quic->path_response_pending = true;
			}
		}
		else if (type == 0x1b)
			naui__net_read_bytes(&reader, 8);
		else if (type == 0x1e && !quic->server)
		{
			quic->confirmed = true;
			naui__net_quic_space_discard(quic, &quic->spaces[NET_TLS_LEVEL_HANDSHAKE]);
		}
		else if (type == 0x30 || type == 0x31)
		{
			size_t length = type == 0x31 ? (size_t)naui__net_quic_read_varint(&reader) : reader.left;
			const uint8_t* data = naui__net_read_bytes(&reader, length);
			if (data && connection->phase == NET_PHASE_ESTABLISHED && !naui__net_events_full(net))
				naui__net_push_event(net, NAUI_NET_EVENT_MESSAGE, connection, NAUI_DISCONNECT_LOCAL, false, data, length);
		}
		else
			return type == 0x04 || type == 0x05 ? NET_QUIC_ERROR_PROTOCOL_VIOLATION : NET_QUIC_ERROR_FRAME_ENCODING;

		if (error)
			return error;

		if (!reader.ok)
			return NET_QUIC_ERROR_FRAME_ENCODING;
	}

	return reader.ok ? 0 : NET_QUIC_ERROR_FRAME_ENCODING;
}

static void naui__net_quic_on_packet(Naui_Net net, NetConnection* connection, NetTlsLevel level, uint8_t* packet, size_t pn_offset, size_t packet_size, const uint8_t* scid, size_t scid_size)
{
	NetQuic* quic = connection->quic;
	NetQuicSpace* space = &quic->spaces[level];
	if (space->discarded || !space->rx.ready || packet_size < pn_offset + 4 + NET_AEAD_TAG_SIZE)
		return;

	bool is_long = level != NET_TLS_LEVEL_APPLICATION;
	uint8_t mask[5];
	naui__net_quic_header_mask(&space->rx, packet + pn_offset + 4, mask);
	packet[0] ^= mask[0] & (is_long ? 0x0f : 0x1f);

	size_t pn_size = (size_t)(packet[0] & 3) + 1;
	uint64_t truncated = 0;
	for (size_t i = 0; i < pn_size; i++)
	{
		packet[pn_offset + i] ^= mask[1 + i];
		truncated = (truncated << 8) | packet[pn_offset + i];
	}

	uint64_t pn = naui__net_quic_decode_pn(space->any_received ? space->largest_received + 1 : 0, truncated, (int)pn_size * 8);
	size_t header_size = pn_offset + pn_size;
	uint8_t* payload = packet + header_size;
	size_t payload_size = packet_size - header_size;

	uint8_t nonce[NET_AEAD_NONCE_SIZE];
	NetQuicKeys* keys = &space->rx;
	NetQuicKeys next;
	bool key_update = !is_long && ((packet[0] >> 2) & 1) != quic->key_phase;
	if (key_update)
	{
		if (!quic->confirmed || !space->any_received || pn <= space->largest_received)
			return;

		naui__net_quic_keys_update(&next, &space->rx);
		keys = &next;
	}

	naui__net_quic_nonce(keys, pn, nonce);
	if (!naui__net_aead_open(keys->aes, keys->key, nonce, packet, header_size, payload, payload_size, payload))
		return;

	payload_size -= NET_AEAD_TAG_SIZE;
	if (key_update)
	{
		NetQuicKeys next_tx;
		naui__net_quic_keys_update(&next_tx, &space->tx);
		space->rx = next;
		space->tx = next_tx;
		quic->key_phase ^= 1;
	}

	if (pn < space->received_floor || naui__net_ranges_contain(space->received, pn))
		return;

	if (packet[0] & (is_long ? 0x0c : 0x18))
	{
		naui__net_quic_fail(net, connection, NET_QUIC_ERROR_PROTOCOL_VIOLATION);
		return;
	}

	if (!quic->server && !quic->remote_cid_confirmed && is_long)
	{
		quic->remote_cid_confirmed = true;
		quic->remote_cid_size = (uint8_t)scid_size;
		memcpy(quic->remote_cid, scid, scid_size);
	}

	if (quic->server && level == NET_TLS_LEVEL_HANDSHAKE)
		naui__net_quic_space_discard(quic, &quic->spaces[NET_TLS_LEVEL_INITIAL]);

	bool ack_eliciting = false;
	uint64_t error = naui__net_quic_on_frames(net, connection, level, payload, payload_size, &ack_eliciting);
	if (connection->phase == NET_PHASE_CLOSED)
		return;

	if (error)
	{
		naui__net_quic_fail(net, connection, error);
		return;
	}

	connection->last_recv_ms = net->now_ms;
	connection->bytes_received += packet_size;
	if (!space->discarded)
	{
		naui__net_ranges_add(&space->received, pn, pn + 1);
		if (naui_list_len(space->received) > NET_QUIC_MAX_ACK_RANGES)
		{
			space->received_floor = space->received[0].end;
			naui_list_remove(space->received, 0);
		}

		if (!space->any_received || pn > space->largest_received)
			space->largest_received = pn;

		space->any_received = true;
		space->ack_pending |= ack_eliciting;
	}

	naui__net_quic_advance_handshake(net, connection);
}

static void naui__net_quic_token_mac(Naui_Net net, const Naui_Address* from, const uint8_t* token, const uint8_t* retry_scid, uint8_t mac[16])
{
	uint8_t input[1 + 2 + 4 + 16 + NET_QUIC_TOKEN_SIZE - 16 + NET_QUIC_CID_SIZE];
	uint8_t full[NET_SHA256_SIZE];
	size_t n = 0;

	input[n++] = from->family;
	input[n++] = (uint8_t)(from->port >> 8);
	input[n++] = (uint8_t)from->port;
	naui__net_put_u32(input + n, from->scope_id);
	n += 4;
	memcpy(input + n, from->data, 16);
	n += 16;
	memcpy(input + n, token, NET_QUIC_TOKEN_SIZE - 16);
	n += NET_QUIC_TOKEN_SIZE - 16;
	memcpy(input + n, retry_scid, NET_QUIC_CID_SIZE);
	n += NET_QUIC_CID_SIZE;

	naui__net_hmac_sha256(net->quic_token_key, sizeof(net->quic_token_key), input, n, full);
	memcpy(mac, full, 16);
}

static void naui__net_quic_send_retry(Naui_Net net, const Naui_Address* from, const uint8_t* dcid, size_t dcid_size, const uint8_t* scid, size_t scid_size)
{
	uint8_t retry_scid[NET_QUIC_CID_SIZE];
	if (!naui__net_random_bytes(retry_scid, sizeof(retry_scid)))
		return;

	uint8_t token[NET_QUIC_TOKEN_SIZE] = { 0 };
	naui__net_put_u64(token, net->now_ms);
	token[8] = (uint8_t)dcid_size;
	memcpy(token + 9, dcid, dcid_size);
	naui__net_quic_token_mac(net, from, token, retry_scid, token + NET_QUIC_TOKEN_SIZE - 16);

	uint8_t pseudo[1 + NET_QUIC_MAX_CID_SIZE + 128];
	size_t n = 0;
	pseudo[n++] = (uint8_t)dcid_size;
	memcpy(pseudo + n, dcid, dcid_size);
	n += dcid_size;

	uint8_t* packet = pseudo + n;
	pseudo[n++] = 0xf0;
	naui__net_put_u32(pseudo + n, NET_QUIC_VERSION);
	n += 4;
	pseudo[n++] = (uint8_t)scid_size;
	memcpy(pseudo + n, scid, scid_size);
	n += scid_size;
	pseudo[n++] = NET_QUIC_CID_SIZE;
	memcpy(pseudo + n, retry_scid, sizeof(retry_scid));
	n += sizeof(retry_scid);
	memcpy(pseudo + n, token, sizeof(token));
	n += sizeof(token);

	naui__net_aes128_gcm_tag(naui__net_quic_retry_key, naui__net_quic_retry_nonce, pseudo, n, NULL, 0, pseudo + n);
	n += NET_AEAD_TAG_SIZE;

	size_t sent = 0;
	naui_socket_sendto(&net->quic, packet, (size_t)(pseudo + n - packet), from, &sent);
}

static NetConnection* naui__net_quic_accept(Naui_Net net, const Naui_Address* from, const uint8_t* dcid, size_t dcid_size, const uint8_t* scid, size_t scid_size, const uint8_t* token, size_t token_size, size_t datagram_size)
{
	if (!net->quic_listening || datagram_size < NET_QUIC_DATAGRAM_SIZE || dcid_size < 8)
		return NULL;

	if (token_size == 0)
	{
		naui__net_quic_send_retry(net, from, dcid, dcid_size, scid, scid_size);
		return NULL;
	}

	if (token_size != NET_QUIC_TOKEN_SIZE || dcid_size != NET_QUIC_CID_SIZE || token[8] > NET_QUIC_MAX_CID_SIZE)
		return NULL;

	uint8_t mac[16];
	naui__net_quic_token_mac(net, from, token, dcid, mac);
	if (!naui__net_ct_equal(mac, token + NET_QUIC_TOKEN_SIZE - 16, sizeof(mac)))
		return NULL;

	if (net->now_ms - naui__net_get_u64(token) > NET_QUIC_TOKEN_LIFETIME_MS)
		return NULL;

	if (naui__net_count_live(net) >= net->config.max_connections || naui__net_count_pending(net) >= NET_MAX_PENDING)
		return NULL;

	NetConnection* connection = naui__net_new_connection(net, NAUI_TRANSPORT_QUIC, true, from);
	NetQuic* quic = connection ? naui__net_quic_new(net, connection, true) : NULL;
	if (!quic)
	{
		if (connection)
			connection->phase = NET_PHASE_CLOSED;

		return NULL;
	}

	connection->phase = NET_PHASE_HANDSHAKE;
	memcpy(quic->local_cid, dcid, NET_QUIC_CID_SIZE);
	memcpy(quic->remote_cid, scid, scid_size);
	quic->remote_cid_size = (uint8_t)scid_size;
	quic->original_dcid_size = token[8];
	memcpy(quic->original_dcid, token + 9, token[8]);
	naui__net_quic_initial_keys(quic, dcid, dcid_size);
	naui__net_quic_encode_params(net, quic);
	return connection;
}

static NetConnection* naui__net_quic_find(Naui_Net net, const Naui_Address* from, const uint8_t* dcid, size_t dcid_size)
{
	if (dcid_size != NET_QUIC_CID_SIZE)
		return NULL;

	for (ptrdiff_t i = 0; i < naui_list_len(net->connections); i++)
	{
		NetConnection* connection = net->connections[i];
		if (connection->transport != NAUI_TRANSPORT_QUIC || connection->phase == NET_PHASE_CLOSED)
			continue;

		if (memcmp(connection->quic->local_cid, dcid, NET_QUIC_CID_SIZE) == 0 && naui_address_equal(&connection->remote, from))
			return connection;
	}

	return NULL;
}

static void naui__net_quic_on_retry(Naui_Net net, NetConnection* connection, const uint8_t* packet, size_t size, size_t header_size, const uint8_t* scid, size_t scid_size)
{
	NetQuic* quic = connection->quic;
	if (quic->server || quic->retried || quic->remote_cid_confirmed)
		return;

	if (size <= header_size + NET_AEAD_TAG_SIZE || size - header_size - NET_AEAD_TAG_SIZE > NET_QUIC_MAX_TOKEN_SIZE)
		return;

	uint8_t pseudo[1 + NET_QUIC_MAX_CID_SIZE + 1 + 4 + 2 + 2 * NET_QUIC_MAX_CID_SIZE + NET_QUIC_MAX_TOKEN_SIZE];
	uint8_t tag[NET_AEAD_TAG_SIZE];
	size_t n = 0;
	pseudo[n++] = quic->original_dcid_size;
	memcpy(pseudo + n, quic->original_dcid, quic->original_dcid_size);
	n += quic->original_dcid_size;
	memcpy(pseudo + n, packet, size - NET_AEAD_TAG_SIZE);
	n += size - NET_AEAD_TAG_SIZE;

	naui__net_aes128_gcm_tag(naui__net_quic_retry_key, naui__net_quic_retry_nonce, pseudo, n, NULL, 0, tag);
	if (!naui__net_ct_equal(tag, packet + size - NET_AEAD_TAG_SIZE, sizeof(tag)))
		return;

	size_t token_size = size - header_size - NET_AEAD_TAG_SIZE;
	quic->token = (uint8_t*)malloc(token_size);
	if (!quic->token)
		return;

	memcpy(quic->token, packet + header_size, token_size);
	quic->token_size = token_size;
	quic->retried = true;
	quic->retry_scid_size = (uint8_t)scid_size;
	memcpy(quic->retry_scid, scid, scid_size);
	quic->remote_cid_size = (uint8_t)scid_size;
	memcpy(quic->remote_cid, scid, scid_size);
	naui__net_quic_initial_keys(quic, quic->remote_cid, quic->remote_cid_size);

	NetQuicSpace* space = &quic->spaces[NET_TLS_LEVEL_INITIAL];
	if (naui_list_len(space->sent) == 1)
		naui__net_quic_on_rtt_sample(quic, net->now_ms - space->sent[0].time_ms, 0);

	for (ptrdiff_t i = 0; i < naui_list_len(space->sent); i++)
	{
		quic->bytes_in_flight -= space->sent[i].size;
	}

	naui_list_clear(space->sent);
	naui_list_clear(space->crypto_tx.lost);
	space->crypto_tx.next = space->crypto_tx.base;
	connection->last_recv_ms = net->now_ms;
}

static void naui__net_quic_on_datagram(Naui_Net net, uint8_t* data, size_t size, const Naui_Address* from)
{
	size_t offset = 0;
	while (offset < size)
	{
		uint8_t* packet = data + offset;
		size_t left = size - offset;
		if (!(packet[0] & 0x80))
		{
			NetConnection* connection = left > NET_QUIC_CID_SIZE ? naui__net_quic_find(net, from, packet + 1, NET_QUIC_CID_SIZE) : NULL;
			if (connection && (packet[0] & 0x40))
				naui__net_quic_on_packet(net, connection, NET_TLS_LEVEL_APPLICATION, packet, 1 + NET_QUIC_CID_SIZE, left, NULL, 0);

			return;
		}

		NetReader reader = naui__net_reader(packet + 1, left - 1);
		uint32_t version = naui__net_read_uint(&reader, 4);
		NetReader dcid = naui__net_read_block(&reader, 1);
		NetReader scid = naui__net_read_block(&reader, 1);
		if (!dcid.ok || !scid.ok || dcid.left > NET_QUIC_MAX_CID_SIZE || scid.left > NET_QUIC_MAX_CID_SIZE)
			return;

		if (version != NET_QUIC_VERSION || !(packet[0] & 0x40))
			return;

		uint8_t type = (packet[0] >> 4) & 3;
		NetConnection* connection = naui__net_quic_find(net, from, dcid.p, dcid.left);
		if (type == 3)
		{
			if (connection)
				naui__net_quic_on_retry(net, connection, packet, left, (size_t)(reader.p - packet), scid.p, scid.left);

			return;
		}

		NetReader token = type == 0 ? naui__net_reader(NULL, 0) : naui__net_reader(packet, 0);
		if (type == 0)
		{
			size_t token_size = (size_t)naui__net_quic_read_varint(&reader);
			token = naui__net_reader(naui__net_read_bytes(&reader, token_size), token_size);
		}

		size_t length = (size_t)naui__net_quic_read_varint(&reader);
		size_t pn_offset = (size_t)(reader.p - packet);
		if (!reader.ok || length > reader.left || (type == 0 && !token.p))
			return;

		if (!connection && type == 0)
			connection = naui__net_quic_accept(net, from, dcid.p, dcid.left, scid.p, scid.left, token.p, token.left, size);

		if (!connection)
			return;

		if (type != 1)
			naui__net_quic_on_packet(net, connection, type == 0 ? NET_TLS_LEVEL_INITIAL : NET_TLS_LEVEL_HANDSHAKE, packet, pn_offset, pn_offset + length, scid.p, scid.left);

		if (connection->phase == NET_PHASE_CLOSED)
			return;

		offset += pn_offset + length;
	}
}

static void naui__net_quic_receive(Naui_Net net)
{
	for (int i = 0; i < NET_QUIC_MAX_PACKETS; i++)
	{
		if (naui__net_events_full(net))
			return;

		size_t size = 0;
		Naui_Address from;
		if (naui_socket_recvfrom(&net->quic, net->udp_rx, NET_UDP_RECV_SIZE, &size, &from) != NAUI_NET_OK)
			return;

		if (size > 0)
			naui__net_quic_on_datagram(net, net->udp_rx, size, &from);
	}
}

static bool naui__net_quic_open_socket(Naui_Net net, const Naui_Address* address)
{
	if (naui_socket_open(&net->quic, NAUI_SOCKET_UDP, address->family) && naui_socket_bind(&net->quic, address, false))
		return true;

	naui_socket_close(&net->quic);
	return false;
}

static bool naui__net_quic_listen(Naui_Net net, const Naui_Address* address)
{
	if (naui_socket_is_valid(&net->quic))
	{
		naui_log(NAUI_LOG_ERROR, "networking: QUIC is already open. Call listen() before the first QUIC connect()");
		return false;
	}

	if (!naui__net_random_bytes(net->quic_token_key, sizeof(net->quic_token_key)) || !naui__net_quic_open_socket(net, address))
		return false;

	net->quic_listening = true;
	return true;
}

static Naui_Connection naui__net_quic_connect(Naui_Net net, const Naui_Address* address)
{
	if (!naui_socket_is_valid(&net->quic))
	{
		Naui_Address any6 = naui_address_any(NAUI_ADDRESS_IPV6, 0);
		Naui_Address any4 = naui_address_any(NAUI_ADDRESS_IPV4, 0);
		if (!naui__net_quic_open_socket(net, &any6) && !naui__net_quic_open_socket(net, &any4))
			return NAUI_CONNECTION_INVALID;
	}

	if (net->quic.family == NAUI_ADDRESS_IPV4 && address->family == NAUI_ADDRESS_IPV6)
		return NAUI_CONNECTION_INVALID;

	NetConnection* connection = naui__net_new_connection(net, NAUI_TRANSPORT_QUIC, false, address);
	NetQuic* quic = connection ? naui__net_quic_new(net, connection, false) : NULL;
	if (!quic)
	{
		if (connection)
			connection->phase = NET_PHASE_CLOSED;

		return NAUI_CONNECTION_INVALID;
	}

	connection->phase = NET_PHASE_HANDSHAKE;
	quic->remote_cid_size = NET_QUIC_CID_SIZE;
	quic->original_dcid_size = NET_QUIC_CID_SIZE;
	bool ok = naui__net_random_bytes(quic->local_cid, sizeof(quic->local_cid)) && naui__net_random_bytes(quic->remote_cid, NET_QUIC_CID_SIZE);
	memcpy(quic->original_dcid, quic->remote_cid, NET_QUIC_CID_SIZE);
	naui__net_quic_initial_keys(quic, quic->remote_cid, quic->remote_cid_size);
	naui__net_quic_encode_params(net, quic);

	if (!ok || !naui__net_tls_start(&quic->tls))
	{
		connection->phase = NET_PHASE_CLOSED;
		return NAUI_CONNECTION_INVALID;
	}

	NetBuffer* hello = &quic->tls.out[NET_TLS_LEVEL_INITIAL];
	naui__net_buffer_append(&quic->spaces[NET_TLS_LEVEL_INITIAL].crypto_tx.buffer, naui__net_buffer_ptr(hello), naui__net_buffer_len(hello));
	naui__net_buffer_free(hello);
	naui__net_quic_flush(net, connection);
	return (Naui_Connection){ connection->id };
}

static bool naui__net_quic_send(Naui_Net net, NetConnection* connection, const void* data, size_t size, Naui_SendMode mode)
{
	NetQuic* quic = connection->quic;
	if (mode == NAUI_SEND_RELIABLE)
	{
		if (size > net->config.max_message_size)
			return false;

		if (!naui__net_frame_append(net, connection, &quic->stream_tx.buffer, NET_FRAME_DATA, data, size))
			return false;

		naui__net_quic_flush(net, connection);
		return true;
	}

	if (size > NAUI_QUIC_MAX_MESSAGE_SIZE || size + 3 > quic->peer_max_datagram)
		return false;

	if (naui_list_len(quic->datagrams) >= NET_QUIC_MAX_QUEUED_DATAGRAMS)
		return false;

	NetQuicDatagram datagram = { (uint8_t*)malloc(size ? size : 1), size };
	if (!datagram.data)
		return false;

	if (size)
		memcpy(datagram.data, data, size);

	naui_list_push(quic->datagrams, datagram);
	naui__net_quic_flush(net, connection);
	return true;
}

static void naui__net_quic_keepalive(Naui_Net net, NetConnection* connection)
{
	connection->quic->ping_pending = true;
	naui__net_quic_flush(net, connection);
}

static void naui__net_quic_collect(Naui_Net net, bool reading)
{
	if (reading && naui_socket_is_valid(&net->quic))
		naui__net_poll_add(net, &net->quic, NAUI_POLL_READ, NET_SLOT_QUIC, NULL);
}

static uint32_t naui__net_quic_limit_wait(Naui_Net net, uint32_t wait)
{
	for (ptrdiff_t i = 0; i < naui_list_len(net->connections); i++)
	{
		const NetConnection* connection = net->connections[i];
		if (connection->transport != NAUI_TRANSPORT_QUIC || connection->phase == NET_PHASE_CLOSED || !connection->quic->timer_ms)
			continue;

		uint64_t timer = connection->quic->timer_ms;
		uint64_t remaining = timer > net->now_ms ? timer - net->now_ms : 0;
		if (remaining < wait)
			wait = (uint32_t)remaining;
	}

	return wait;
}

static void naui__net_quic_deliver(Naui_Net net, NetConnection* connection)
{
	NetQuic* quic = connection->quic;
	size_t available = naui__net_quic_recv_stream_available(&quic->stream_rx);
	if (available > 0 && !naui__net_events_full(net))
	{
		if (!naui__net_buffer_append(&connection->rx, quic->stream_rx.data, available))
		{
			naui__net_close_connection(net, connection, NAUI_DISCONNECT_ERROR, true);
			return;
		}

		naui__net_quic_recv_stream_consume(&quic->stream_rx, available);
		if (quic->stream_rx.base + NET_QUIC_STREAM_WINDOW - quic->advertised_max_data >= NET_QUIC_STREAM_WINDOW / 2)
			quic->window_update_pending = true;
	}

	if (naui__net_buffer_len(&connection->rx) > 0)
		naui__net_frame_parse(net, connection);
}

static void naui__net_quic_process(Naui_Net net)
{
	for (ptrdiff_t i = 0; i < naui_list_len(net->connections); i++)
	{
		NetConnection* connection = net->connections[i];
		if (connection->transport != NAUI_TRANSPORT_QUIC || connection->phase == NET_PHASE_CLOSED)
			continue;

		naui__net_quic_deliver(net, connection);
		if (connection->phase == NET_PHASE_CLOSED)
			continue;

		if (connection->quic->timer_ms && connection->quic->timer_ms <= net->now_ms)
			naui__net_quic_on_timeout(net, connection);

		naui__net_quic_flush(net, connection);
	}
}
