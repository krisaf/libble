#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

#include "libble.h"

#include <glib.h>

#define VECS_CHAR_MPU_TEMPERATURE   0x0032
#define VECS_CHAR_BATT_LEVEL        0x0036

#define VECS_MOTION_RATE_CFG        0x002b
#define VECS_MOTION_ACCEL_CFG       0x0025
#define VECS_MOTION_GYRO_CFG        0x0028
#define VECS_MOTION_NOTI_VAL        0x002e
#define VECS_MOTION_NOTI_CFG        0x002f

// Maximum PPS up to 200
const uint8_t max_pps = 200;
uint8_t pps = max_pps;

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

GMainLoop *event_loop;

void sigint(int a)
{
	printf("\n");
	g_main_loop_quit(event_loop);
}

void notify_handler(event_t event, uint16_t handle, uint8_t len, const void *data, DEVHANDLER devh)
{
	float temp;	
	int16_t raw_temp;
	uint8_t bat_level;
	static uint8_t configured = 0;

	static uint32_t counter = 0;
	static double speed = 0;
	static struct timeval t_old;
	struct timeval t_new;
	double dtime = 0;
	int16_t mpu_data[7];

	switch ((uint8_t)event) {
		case EVENT_INTERNAL:
			switch (handle) {
				case STATE_CHANGED:
					if (((uint8_t *)data)[0] == STATE_CONNECTED) {
						printf("Connection successful\n");
						printf("Request temperature\n");
						lble_request(devh, VECS_CHAR_MPU_TEMPERATURE);
						printf("Request battery\n");
						lble_request(devh, VECS_CHAR_BATT_LEVEL);
					}
					break;
				case DATA_TO_READ:
					printf("Data to read event\n");
					break;
				case DATA_TO_WRITE:
					printf("Data to write event\n");
					if (!configured) {
						configured = 1;
						printf("Set wake up MPU6000 and setting %dHz data rate\n", pps);
						lble_write(devh, VECS_MOTION_RATE_CFG, 1, &pps);
						printf("Setting accel range\n");
						lble_write(devh, VECS_MOTION_ACCEL_CFG, 1, &accel);
						printf("Setting gyro range\n");
						lble_write(devh, VECS_MOTION_GYRO_CFG, 1, &gyro);
						sleep(1);
						printf("Enabling notifications on MPU6000 data\n");
						lble_write(devh, VECS_MOTION_NOTI_CFG, 2, (uint8_t *)"\x01\x00");
						printf("Enable listening for notifications\n");
						lble_listen(devh);
						gettimeofday(&t_old, NULL);
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
					raw_temp = ((uint16_t)((uint8_t *)data)[0] << 8) | ((uint8_t *)data)[1];
					temp = raw_temp / 340.0 + 35.0;
					printf(" * Temperature: %.1f *C\n", temp);
					break;
				case VECS_CHAR_BATT_LEVEL:
					bat_level = ((uint8_t *)data)[0];
					printf(" * Battery: %d%%\n", bat_level);
					break;
				case VECS_MOTION_NOTI_VAL:
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
						mpu_data[i] = (int16_t)((uint8_t *)data)[i << 1] << 8
						              | ((uint8_t *)data)[(i << 1) + 1];
					}
					printf(" = %6d  |  Accel: %6d, %6d, %6d  |  Gyro: %6d, %6d, %6d  |  PPS: %.0fHz\n",
						(uint16_t)mpu_data[3],
						mpu_data[0], mpu_data[1], mpu_data[2],
						mpu_data[4], mpu_data[5], mpu_data[6], 
						speed);
					break;
			}
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
	DEVHANDLER devh = lble_newdev();
	lble_set_event_handler(devh, notify_handler);

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
