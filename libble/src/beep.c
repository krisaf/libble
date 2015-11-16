#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "libble.h"

#define VECS_CHAR_BEEP_REQUEST	0x003f

char *dev_addr;
DEVHANDLER devh;

int main(int argc, char **argv)
{
    uint8_t delay = 1;

	if ( argc != 3 ) {
		printf("\nusage: %s <device addr> <delay between beeps>\n\n", argv[0]);
		return -1;
	}
	dev_addr = argv[1];
	delay = atoi(argv[2]);

	printf("connecting to %s\n", dev_addr);
	devh = lble_newdev();
	lble_connect(devh, dev_addr);

	if (lble_get_state(devh) != STATE_CONNECTED) {
		fprintf(stderr, "error: connection failed\n");
		return -1;
	}
	printf("connection successful\n");

	printf("asking to beep every %d seconds...\n", delay);
	lble_write(devh, VECS_CHAR_BEEP_REQUEST, 1, &delay);

	printf("disconnect\n");
	lble_disconnect(devh);
	lble_freedev(devh);
	return 0;
}


