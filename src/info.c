#include "vecs.h"
#include <stdlib.h>

// Beep delay in seconds
uint8_t delay = 0;

void connected(DEVHANDLER devh)
{
	printf("Enabling notifications for button events\n");
	vecs_notify(devh, VECS_UUID_KEY_PRESS, 1);
	if (delay) {
		printf("Asking to beep every %d seconds\n", delay);
		vecs_write(devh, VECS_UUID_KEY_BEEP, 1, &delay);
	}
	printf("Request temperature\n");
	vecs_request(devh, VECS_UUID_MOTION_TEMP);
	printf("Request battery\n");
	vecs_request(devh, VECS_UUID_CHARGER_ADC);
}

void notify(DEVHANDLER devh, uint16_t uuid, uint8_t len, const void *data)
{
	float temp;
	int16_t raw_temp;
	uint16_t bat_level;
	switch (uuid) {
		case VECS_UUID_MOTION_TEMP:
			raw_temp = ((uint16_t)((uint8_t *)data)[0] << 8) | ((uint8_t *)data)[1];
			temp = raw_temp / 340.0 + 35.0;
			printf(" * Temperature: %.1f *C\n", temp);
			break;
		case VECS_UUID_CHARGER_ADC:
			bat_level = *(uint16_t *)data;
			printf(" * Battery: %d%%\n", bat_level);
			break;
		case VECS_UUID_KEY_PRESS:
			switch (((uint8_t *)data)[0]) {
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
		default:
			printf("Unknown uuid: %d\n", uuid);
			break;
	}
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		printf("\nUsage: %s <device addr> [delay between beeps in seconds]\n\n", argv[0]);
		return -1;
	}

	if (argc >= 3) {
		delay = atoi(argv[2]);
	}

	char *dev_addr = argv[1];
	vecs_connect(dev_addr, connected, notify);
	return 0;
}
