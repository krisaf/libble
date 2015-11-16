#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

#include "libble.h"

#define VECS_CHAR_BATT_LEVEL 0x0036

char *dev_addr;
DEVHANDLER devh;

int main(int argc, char **argv)
{
	uint8_t bat_level;

	if ( argc != 2 ) {
		printf("\nusage: %s <device addr>\n\n", argv[0]);
		return -1;
	}
	dev_addr = argv[1];

	devh = lble_newdev();
	printf("connecting to %s\n", dev_addr);
	lble_connect(devh, dev_addr);
	if (lble_get_state(devh) != STATE_CONNECTED) {
		fprintf(stderr, "error: connection failed\n");
		return -1;
	}
	printf("connection successful\n");

	lble_read(devh, VECS_CHAR_BATT_LEVEL, (uint8_t *)&bat_level);
	printf("battery: %d%%\n", bat_level);

	lble_disconnect(devh);
	lble_freedev(devh);
	return 0;
}


