#include "vecs.h"

#include <glib.h>

GMainLoop *event_loop;
vecs_connected_cb m_connected = NULL;
vecs_notify_cb m_notify = NULL;

char_info_t profiles[] = {
		  {VECS_UUID_CHARGER_ADC, 0, 0, 0}
		, {VECS_UUID_CHARGER_OFF, 0, 0, 0}
		, {VECS_UUID_CHARGER_LOW, 0, 0, 0}
		, {VECS_UUID_KEY_PRESS, 0, 0, 0}
		, {VECS_UUID_KEY_BEEP, 0, 0, 0}
		, {VECS_UUID_MOTION_ACCEL, 0, 0, 0}
		, {VECS_UUID_MOTION_GYRO, 0, 0, 0}
		, {VECS_UUID_MOTION_RATE, 0, 0, 0}
		, {VECS_UUID_MOTION_NOTI, 0, 0, 0}
		, {VECS_UUID_MOTION_TEMP, 0, 0, 0}
};

const int profiles_count = sizeof(profiles)/sizeof(profiles[0]);

uint16_t profile(uint16_t handle)
{
	for (int i = 0; i < profiles_count; i++) {
		if (profiles[i].value_handle == handle) {
			return profiles[i].uuid;
		}
	}
	return 0;
}

uint16_t handle(uint16_t profile)
{
	for (int i = 0; i < profiles_count; i++) {
		if (profiles[i].uuid == profile) {
			return profiles[i].value_handle;
		}
	}
	return 0;
}

uint16_t handle_notify(uint16_t profile)
{
	uint16_t h = handle(profile);
	return h ? h + 1 : h;
}

void vecs_notify(DEVHANDLER devh, uint16_t uuid, uint8_t enable)
{
	lble_write(devh, handle_notify(uuid), 2, enable ? (uint8_t *)"\x01\x00" : (uint8_t *)"\x00\x00");
}

void vecs_request(DEVHANDLER devh, uint16_t uuid)
{
	lble_request(devh, handle(uuid));
}

void vecs_write(DEVHANDLER devh, uint16_t uuid, uint8_t len, void *data)
{
	lble_write(devh, handle(uuid), len, data);
}

void quit()
{
	if (event_loop) {
		g_main_loop_quit(event_loop);
		g_main_loop_unref(event_loop);
		event_loop = NULL;
	}
}

void sigint(int a)
{
	printf("\n");
	quit();
}

void notify_handler(event_t event, uint16_t value_handle, uint8_t len, const void *data, DEVHANDLER devh)
{
	static uint8_t discover = 2;

	switch ((uint8_t)event) {
		case EVENT_INTERNAL:
			switch (value_handle) {
				case STATE_CHANGED:
					switch (((uint8_t *)data)[0]) {
						case STATE_CONNECTED:
							printf("Connection successful\n");
							printf("Enable listening for notifications\n");
							lble_listen(devh);
							printf("Discover handles\n");
							lble_discover_hand(devh);
							break;
						case STATE_DISCONNECTED:
							printf("Disconnected\n");
							quit();
							break;
						default:
							break;
					}
					break;
				case DATA_TO_READ:
					printf("Data to read event\n");
					break;
				case DATA_TO_WRITE:
					printf("Data to write event\n");
					break;
				case ERROR_OCCURED:
					printf("Error: %s\n", (const char *)data);
					quit();
					break;
			}
			break;
		case EVENT_DEVICE:
			m_notify(devh, profile(value_handle), len, data);
			break;
		case EVENT_UUID:
			if (data) {
				char_info_t *info = (char_info_t *)data;
				if (info->uuid) {
					for (int i = 0; i < profiles_count; i++) {
						if (profiles[i].uuid == info->uuid) {
							profiles[i].value_handle = value_handle;
							break;
						}
					}
				} else {
					for (int i = 0; i < profiles_count; i++) {
						if (profiles[i].value_handle == info->value_handle) {
							profiles[i].properties = info->properties;
							profiles[i].handle = info->handle;
							printf("UUID: %.4x, Value handle: %.4x, Handle: 0x%.4x\n", profiles[i].uuid, info->value_handle, info->handle);
						break;
						}
					}
				}
			} else {
				if (discover) {
					if (--discover) {
						printf("Discover characteristics\n");
						lble_discover_char(devh);
					} else {
						m_connected(devh);
					}
				}
			}
			break;
	}
}

void vecs_connect(const char *addr, vecs_connected_cb connected, vecs_notify_cb notify)
{
	m_connected = connected;
	m_notify = notify;
	signal(SIGINT, sigint);
	DEVHANDLER devh = lble_newdev();
	lble_set_event_handler(devh, notify_handler);

	printf("Connecting to %s\n", addr);
	lble_connect(devh, addr);

	event_loop = g_main_loop_new(NULL, FALSE);
	if (event_loop) {
		g_main_loop_run(event_loop);
		if (event_loop) {
			g_main_loop_unref(event_loop);
		}
	}

	printf("Disconnect\n");
	lble_disconnect(devh);
	lble_freedev(devh);
}
