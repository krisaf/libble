#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

#include "libble.h"

#define VECS_CHAR_MPU_TEMPERATURE 0x0032

char *dev_addr;
DEVHANDLER devh;

int main(int argc, char **argv)
{
	float temp;	
	int16_t raw_temp;
	uint8_t data[2];

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

	lble_read(devh, VECS_CHAR_MPU_TEMPERATURE, data);

	raw_temp = (data[0] << 8) | data[1];
	temp = raw_temp / 340.0 + 35.0;

	printf("temperature: %.1f *C\n", temp);

	lble_disconnect(devh);
	lble_freedev(devh);
	return 0;
}


