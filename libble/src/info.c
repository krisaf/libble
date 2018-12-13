#include <stdio.h>
#include <unistd.h>

#include "libble.h"

#include <glib.h>

#define VECS_CHAR_MPU_TEMPERATURE   0x0032
#define VECS_CHAR_BATT_LEVEL        0x0036

#define VECS_BUTTON_NOTI_VAL        0x003b
#define VECS_BUTTON_NOTI_CFG        0x003c

#define VECS_CHAR_BEEP_REQUEST      0x003f

uint8_t delay = 0;
GMainLoop *event_loop;

void sigint(int a)
{
	printf("\n");
	g_main_loop_quit(event_loop);
}

void notify_handler(event_t event, uint16_t handle, uint8_t len, const uint8_t *data, DEVHANDLER devh)
{
	float temp;	
	int16_t raw_temp;
	uint8_t bat_level;
	static uint8_t configured = 0;

	switch ((uint8_t)event) {
		case EVENT_INTERNAL:
			switch (handle) {
				case STATE_CHANGED:
					if (data[0] == STATE_CONNECTED) {
						printf("Connection successful\n");
						printf("Request temperature\n");
						lble_request(devh, VECS_CHAR_MPU_TEMPERATURE);
						printf("Request battery\n");
						lble_request(devh, VECS_CHAR_BATT_LEVEL);
						printf("Enable listening for notifications\n");
						lble_listen(devh);
					}
					break;
				case DATA_TO_READ:
					printf("Data to read event\n");
					break;
				case DATA_TO_WRITE:
					printf("Data to write event\n");
					if (!configured) {
						configured = 1;
						printf("Enabling notifications for button events\n");
						lble_write(devh, VECS_BUTTON_NOTI_CFG, 2, (uint8_t *)"\x01\x00");
						if (delay) {
							printf("Asking to beep every %d seconds\n", delay);
							lble_write(devh, VECS_CHAR_BEEP_REQUEST, 1, &delay);
						}
					}
					break;
				case ERROR_OCCURED:
					printf("An error occured\n");
					g_main_loop_quit(event_loop);
					break;
			}
			break;
		case EVENT_DEVICE:
			switch (handle) {
				case VECS_CHAR_MPU_TEMPERATURE:
					raw_temp = (data[0] << 8) | data[1];
					temp = raw_temp / 340.0 + 35.0;
					printf(" * Temperature: %.1f *C\n", temp);
					break;
				case VECS_CHAR_BATT_LEVEL:
					bat_level = data[0];
					printf(" * Battery: %d%%\n", bat_level);
					break;
				case VECS_BUTTON_NOTI_VAL:
					switch (*data) {
						case 1:
							printf(" +   Single click\n");
							break;
						case 2:
							printf(" ++  Double click\n");
							break;
						case 3:
							printf(" +++ Long click\n");
							break;
					}
					break;
			}
			break;
	}
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		printf("\nUsage: %s <device addr> [delay between beeps in sec]\n\n", argv[0]);
		return -1;
	}

	if (argc >= 3) {
		delay = atoi(argv[2]);
	}

	char *dev_addr = argv[1];
	DEVHANDLER devh = lble_newdev(notify_handler);

	printf("Connecting to %s\n", dev_addr);
	lble_connect(devh, dev_addr);

	signal(SIGINT, sigint);
	event_loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(event_loop);
	g_main_loop_unref(event_loop);

	printf("Disconnect\n");
	lble_disconnect(devh);
	lble_freedev(devh);
	return 0;
}
