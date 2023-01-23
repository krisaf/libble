#include "vecs.h"

#include <stdlib.h>
#include <sys/time.h>

// Maximum PPS up to 200
const uint8_t max_pps = 200;
uint8_t pps = max_pps;
struct timeval t_old;

/*
 * Accel range
 * 0 - +-2g
 * 1 - +-4g
 * 2 - +-8g
 * 3 - +-16g
 */
static uint8_t accel = 0;

/*
 * Gyro range
 * 0 - +-250 deg/s
 * 1 - +-500 deg/s
 * 2 - +-1000 deg/s
 * 3 - +-2000 deg/s
 */
static uint8_t gyro = 0;

void connected(DEVHANDLER devh)
{
	gettimeofday(&t_old, NULL);
	printf("Set wake up MPU6000 and setting %dHz data rate\n", pps);
	vecs_write(devh, VECS_UUID_MOTION_RATE, 1, &pps);
	printf("Setting accel range\n");
	vecs_write(devh, VECS_UUID_MOTION_ACCEL, 1, &accel);
	printf("Setting gyro range\n");
	vecs_write(devh, VECS_UUID_MOTION_GYRO, 1, &gyro);
	printf("Request temperature\n");
	vecs_request(devh, VECS_UUID_MOTION_TEMP);
	printf("Request battery\n");
	vecs_request(devh, VECS_UUID_CHARGER_ADC);
	printf("Enabling notifications on MPU6000 data\n");
	vecs_notify(devh, VECS_UUID_MOTION_NOTI, 1);
}

void notify(DEVHANDLER devh, uint16_t uuid, uint8_t len, const void *data)
{
	float temp;
	int16_t raw_temp;
	uint8_t bat_level;

	static uint32_t counter = 0;
	static double speed = 0;
	struct timeval t_new;
	double dtime = 0;
	int16_t mpu_data[7];
	switch (uuid) {
		case VECS_UUID_MOTION_TEMP:
			raw_temp = ((uint16_t)((uint8_t *)data)[0] << 8) | ((uint8_t *)data)[1];
			temp = raw_temp / 340.0 + 35.0;
			printf(" * Temperature: %.1f *C\n", temp);
			break;
		case VECS_UUID_CHARGER_ADC:
			bat_level = ((uint8_t *)data)[0];
			printf(" * Battery: %d%%\n", bat_level);
			break;
		case VECS_UUID_MOTION_NOTI:
			counter++;
			gettimeofday(&t_new, NULL);
			dtime = t_new.tv_sec - t_old.tv_sec;
			dtime += (t_new.tv_usec - t_old.tv_usec) / 1000000.0;
			if (dtime >= 1.0) {
				speed = counter / dtime + 0.5;
				counter = 0;
				t_old = t_new;
			}
			for (int i = 0; i < 7; i++) {
				mpu_data[i] = (int16_t)((uint8_t *)data)[i << 1] << 8 | ((uint8_t *)data)[(i << 1) + 1];
			}
			printf(" = %6d  |  Accel: %6d, %6d, %6d  |  Gyro: %6d, %6d, %6d  |  PPS: %.0fHz\n",
				(uint16_t)mpu_data[3],
				mpu_data[0], mpu_data[1], mpu_data[2],
				mpu_data[4], mpu_data[5], mpu_data[6], 
				speed);
			break;
	}
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		printf("\nUsage: %s <device addr> [PPS=200]\n\n", argv[0]);
		return -1;
	}

	if (argc >= 3) {
		pps = atoi(argv[2]);
		if (pps > max_pps)
			pps = max_pps;
	}

	char *dev_addr = argv[1];
	vecs_connect(dev_addr, connected, notify);
	return 0;
}
