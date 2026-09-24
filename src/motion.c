#include "vecs.h"

#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

enum MPURate {
    RATE_0_2Hz     = 0x01 // 0.2Hz
  , RATE_0_5Hz     = 0x02 // 0.5Hz
  , RATE_1Hz       = 0x03 // 1Hz
  , RATE_2Hz       = 0x04 // 2Hz
  , RATE_5Hz       = 0x05 // 5Hz
  , RATE_10Hz      = 0x06 // 10Hz (default)
  , RATE_20Hz      = 0x07 // 20Hz
  , RATE_50Hz      = 0x08 // 50Hz
  , RATE_100Hz     = 0x09 // 100Hz
  , RATE_200Hz     = 0x0B // 200Hz
  , SINGLE_RETURN  = 0x0C // Single return
  , NO_RETURN      = 0x0D // no periodic return
  , RATE_INVALID   = 0xFF // Ivalid Hz
};

uint8_t hzToMpuRate(uint8_t hz)
{
    switch (hz) {
    case 1:
        return RATE_1Hz;
    case 2:
        return RATE_2Hz;
    case 5:
        return RATE_5Hz;
    case 10:
        return RATE_10Hz;
    case 20:
        return RATE_20Hz;
    case 50:
        return RATE_50Hz;
    case 100:
        return RATE_100Hz;
    case 200:
        return RATE_200Hz;
    default:
        return RATE_INVALID;
    }
}

uint8_t pps = 10;
uint8_t pps_command = RATE_10Hz;
uint8_t write_rate[5] = {
    0xFF,
    0xAA,
    0x03,
    0x06,
    0x00
};
struct timeval t_old;

/*
 * Accel range
 * 0 - +-2g
 * 1 - +-4g
 * 2 - +-8g
 * 3 - +-16g
 */
static uint8_t accel = 3;

/*
 * Gyro range
 * 0 - +-250 deg/s
 * 1 - +-500 deg/s
 * 2 - +-1000 deg/s
 * 3 - +-2000 deg/s
 */
static uint8_t gyro = 3;

void connected(DEVHANDLER devh)
{
    gettimeofday(&t_old, NULL);
	printf("Set wake up MPU6000 and setting %dHz data rate\n", pps);
    write_rate[3] = pps_command;
    vecs_write(devh, UUID_WRITE, 5, write_rate);
    printf("Enabling notifications on MPU6000 data\n");
    vecs_notify(devh, UUID_NOTIFY, 1);
}

void parsePacket(const void *data) {
    static uint32_t packetIndex = 0;
    static uint32_t counter = 0;
    static double speed = 0;
    struct timeval t_new;
    double dtime = 0;

    counter++;
    gettimeofday(&t_new, NULL);
    dtime = t_new.tv_sec - t_old.tv_sec;
    dtime += (t_new.tv_usec - t_old.tv_usec) / 1000000.0;
    if (dtime >= 1.0) {
        speed = counter / dtime + 0.5;
        counter = 0;
        t_old = t_new;
    }

    const uint8_t *src = (const uint8_t *)data;
    uint8_t header = src[0];
    if(header != 0x55) {
        return;
    }
    uint8_t flag = src[1];
    if(flag != 0x61) {
        return;
    }
    src += 2;

    int16_t mpu_data[7], mpu_data_n[7];
    for (int i = 0; i < 7; i++) {
        if(i == 3) {
            mpu_data[i] = packetIndex++;
            continue;
        }
        mpu_data[i] = (uint16_t)src[0] | ((uint16_t)src[1] << 8);
        src += 2;
    }

    // Новые датчики
    mpu_data_n[0] =  mpu_data[1]; // ax = ay
    mpu_data_n[1] =  mpu_data[2]; // ay = az
    mpu_data_n[2] =  mpu_data[0]; // az = ax

    mpu_data_n[3] =  mpu_data[3]; // packet index

    mpu_data_n[4] =  mpu_data[5]; // gx = gy
    mpu_data_n[5] =  mpu_data[6]; // gy = gz
    mpu_data_n[6] =  mpu_data[4]; // gz = gx

    printf(" = %6d  |  Accel: %6d, %6d, %6d  |  Gyro: %6d, %6d, %6d  |  PPS: %.0fHz\n",
           mpu_data_n[3],
           mpu_data_n[0], mpu_data_n[1], mpu_data_n[2],
           mpu_data_n[4], mpu_data_n[5], mpu_data_n[6],
           speed);
}

void notify(DEVHANDLER devh, char *uuid, uint8_t len, const void *data)
{
    if(strncmp(uuid, UUID_NOTIFY, 38) == 0){
        uint8_t bytes_read = 0;
        while(len - bytes_read) {
            parsePacket(data + bytes_read);
            bytes_read += 20;
        }
    }
}

int main(int argc, char **argv)
{
	if (argc < 2) {
        printf("\nUsage: %s <device addr> [PPS=1, 2, 5, 10, 20, 50, 100, 200]\n\n", argv[0]);
		return -1;
	}

	if (argc >= 3) {
        pps = atoi(argv[2]);
        pps_command = hzToMpuRate(pps);
        if (pps_command == RATE_INVALID) {
            printf("\nSupported PPS: 1, 2, 5, 10, 20, 50, 100, 200\n\n");
            return -1;
        }
	}

	char *dev_addr = argv[1];
	vecs_connect(dev_addr, connected, notify);
	return 0;
}
