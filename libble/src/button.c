#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>

#include "libble.h"

#define VECS_BUTTON_NOTI_VAL	0x003b
#define VECS_BUTTON_NOTI_CFG	0x003c

char *dev_addr;
DEVHANDLER devh;

void sigint(int a)
{
	lble_disconnect(devh);
}

void noti_handler(uint16_t handle, uint8_t len, const uint8_t *data, const void *cb_info)
{
	if (handle == VECS_BUTTON_NOTI_VAL && len == 1) {
		switch (*data) {
			case 1:
				printf("NOTIFICATION: single click\n");
				break;
			case 2:
				printf("NOTIFICATION: double click\n");
				break;
			case 3:
				printf("NOTIFICATION: long click\n");
				break;
		}
	} else
		printf("unnokwn notification (handle: 0x%x)\n", handle);
}

int main(int argc, char **argv)
{
	if ( argc != 2 ) {
		printf("\nusage: %s <device addr>\n\n", argv[0]);
		return -1;
	}
	dev_addr = argv[1];

	signal(SIGINT, sigint);
	devh = lble_newdev();
	printf("connecting to %s\n", dev_addr);
	lble_connect(devh, dev_addr);

	if (lble_get_state(devh) != STATE_CONNECTED) {
		fprintf(stderr, "error: connection failed\n");
		return -1;
	}
	printf("connection successful\n");

	printf("enabling notifications for button events...\n");
	lble_write(devh, VECS_BUTTON_NOTI_CFG, 2, (uint8_t *)"\x01\x00");

	printf("listening for notifications\n");
	lble_listen(devh, noti_handler, NULL);

	lble_freedev(devh);
	return 0;
}

