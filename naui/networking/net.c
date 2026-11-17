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
	cfg->device_key = NULL;
	net->send_buffer_limit = NET_MIN_SEND_BUFFER;

	if ((size_t)cfg->max_message_size * 2 > net->send_buffer_limit)
		net->send_buffer_limit = (size_t)cfg->max_message_size * 2;

	net->tcp_listener = NAUI_SOCKET_NULL;
	net->udp = NAUI_SOCKET_NULL;
	net->quic = NAUI_SOCKET_NULL;
	net->udp_rx = (uint8_t*)malloc(NET_UDP_RECV_SIZE);

	bool has_key = config && config->device_key;
	if (has_key)
		net->device_key = *config->device_key;

	if (!net->udp_rx || (!has_key && !naui_device_key_generate(&net->device_key)) || !naui_sockets_init())
	{
		free(net->udp_rx);
		naui__net_wipe(net, sizeof(*net));
		free(net);
		return NULL;
	}

	net->local.id = naui_device_id_from_key(&net->device_key);
	net->local.is_local = true;
	net->local.trust = NAUI_DEVICE_TRUST_VERIFIED;

	net->local.first_seen = naui_unix_time();
	net->local.last_seen = net->local.first_seen;

	naui_device_describe_local(&net->local, config ? config->device_name : NULL);
	naui__net_tick(net);
	return net;
}

void naui_net_destroy(Naui_Net net)
{
	if (!net)
		return;

	naui__net_tick(net);
	for (ptrdiff_t i = 0; i < naui_list_len(net->connections); i++)
	{
		naui__net_close_connection(net, net->connections[i], NAUI_DISCONNECT_LOCAL, true);
	}

	naui__net_sweep(net);

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
	naui_socket_close(&net->quic);
	free(net->udp_rx);
	naui_sockets_shutdown();
	naui__net_wipe(net, sizeof(*net));
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
		return naui__net_tcp_listen(net, &address);

	if (transport == NAUI_TRANSPORT_UDP)
		return naui__net_udp_listen(net, &address);

	if (transport == NAUI_TRANSPORT_QUIC)
		return naui__net_quic_listen(net, &address);

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

	if (transport == NAUI_TRANSPORT_QUIC && net->quic_listening)
	{
		*out_address = net->quic.address;
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

	naui__net_tick(net);
	if (naui__net_count_live(net) >= net->config.max_connections)
		return NAUI_CONNECTION_INVALID;

	if (transport == NAUI_TRANSPORT_TCP)
		return naui__net_tcp_connect(net, &address);

	if (transport == NAUI_TRANSPORT_UDP)
		return naui__net_udp_connect(net, &address);

	if (transport == NAUI_TRANSPORT_QUIC)
		return naui__net_quic_connect(net, &address);

	return NAUI_CONNECTION_INVALID;
}

void naui_net_disconnect(Naui_Net net, Naui_Connection connection)
{
	if (!net)
		return;

	NetConnection* c = naui__net_find_connection(net, connection);
	if (!c)
		return;

	naui__net_tick(net);
	naui__net_close_connection(net, c, NAUI_DISCONNECT_LOCAL, true);
}

bool naui_net_send(Naui_Net net, Naui_Connection connection, const void* data, size_t size, Naui_SendMode mode)
{
	if (!net)
		return false;

	if (!data && size)
		return false;

	NetConnection* c = naui__net_find_connection(net, connection);
	if (!c || c->phase != NET_PHASE_ESTABLISHED)
		return false;

	naui__net_tick(net);
	if (c->transport == NAUI_TRANSPORT_TCP)
		return naui__net_tcp_send(net, c, data, size);

	if (c->transport == NAUI_TRANSPORT_UDP)
		return naui__net_udp_send(net, c, data, size, mode);

	return naui__net_quic_send(net, c, data, size, mode);
}

bool naui_net_send_to_device(Naui_Net net, const Naui_Device* device, const void* data, size_t size, Naui_SendMode mode)
{
	if (!net || !device)
		return false;

	NetConnection* best = NULL;
	int best_score = -1;
	for (ptrdiff_t i = 0; i < naui_list_len(net->connections); i++)
	{
		NetConnection* connection = net->connections[i];
		if (connection->device != device || connection->phase != NET_PHASE_ESTABLISHED)
			continue;

		if (device->trust == NAUI_DEVICE_TRUST_VERIFIED && !connection->verified)
			continue;

		if (mode == NAUI_SEND_RELIABLE && connection->transport == NAUI_TRANSPORT_UDP)
			continue;

		int score = connection->verified ? 2 : 0;
		if (mode == NAUI_SEND_UNRELIABLE && connection->transport != NAUI_TRANSPORT_TCP)
			score++;

		if (score > best_score)
		{
			best = connection;
			best_score = score;
		}
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

	const NetConnection* c = naui__net_find_connection(net, connection);
	if (!c)
		return false;

	memset(out_info, 0, sizeof(*out_info));
	out_info->transport = c->transport;
	out_info->state = c->phase == NET_PHASE_ESTABLISHED ? NAUI_CONNECTION_CONNECTED : NAUI_CONNECTION_CONNECTING;

	out_info->incoming = c->incoming;
	out_info->verified = c->verified;
	out_info->remote = c->remote;
	out_info->device = c->device;

	out_info->bytes_sent = c->bytes_sent;
	out_info->bytes_received = c->bytes_received;
	return true;
}

static void naui__net_run_timers(Naui_Net net)
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
				naui__net_close_connection(net, connection, NAUI_DISCONNECT_TIMEOUT, true);

			else if (connection->transport == NAUI_TRANSPORT_UDP && !connection->incoming && net->now_ms - connection->last_connect_ms >= NET_UDP_CONNECT_RETRY_MS)
				naui__net_udp_send_connect(net, connection);

			continue;
		}

		if (net->now_ms - connection->last_recv_ms >= config->idle_timeout_ms)
		{
			naui__net_close_connection(net, connection, NAUI_DISCONNECT_TIMEOUT, true);
			continue;
		}

		if (net->now_ms - connection->last_send_ms < config->keepalive_interval_ms)
			continue;

		if (connection->transport == NAUI_TRANSPORT_UDP)
			naui__net_udp_keepalive(net, connection);
		else if (connection->transport == NAUI_TRANSPORT_TCP)
			naui__net_tcp_keepalive(net, connection);
		else
			naui__net_quic_keepalive(net, connection);
	}
}

void naui_net_update(Naui_Net net, uint32_t timeout_ms)
{
	if (!net)
		return;

	free(net->current_payload);
	net->current_payload = NULL;
	naui__net_evict_devices(net);
	bool reading = !naui__net_events_full(net);

	naui_list_clear(net->poll_items);
	naui_list_clear(net->poll_slots);
	naui__net_tcp_collect(net, reading);
	naui__net_udp_collect(net, reading);
	naui__net_quic_collect(net, reading);

	uint32_t wait = timeout_ms > NET_MAX_WAIT_MS ? NET_MAX_WAIT_MS : timeout_ms;
	wait = naui__net_quic_limit_wait(net, wait);
	if (naui__net_event_backlog(net) > 0)
		wait = 0;

	naui_socket_poll(net->poll_items, (size_t)naui_list_len(net->poll_items), (int32_t)wait);
	naui__net_tick(net);

	for (ptrdiff_t i = 0; i < naui_list_len(net->poll_items); i++)
	{
		Naui_PollEvents got = net->poll_items[i].got;
		if (!got)
			continue;

		NetPollSlot slot = net->poll_slots[i];
		if (slot.kind == NET_SLOT_TCP_LISTENER)
			naui__net_tcp_accept(net);
		else if (slot.kind == NET_SLOT_UDP)
			naui__net_udp_receive(net);
		else if (slot.kind == NET_SLOT_QUIC)
			naui__net_quic_receive(net);
		else
			naui__net_tcp_on_poll(net, slot.conn, got);
	}

	naui__net_tcp_process(net);
	naui__net_quic_process(net);
	naui__net_run_timers(net);
	naui__net_sweep(net);
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

	return naui__net_find_device(net, id);
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