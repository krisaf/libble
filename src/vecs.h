#ifndef _VECS_H
#define _VECS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "libble.h"

#include <stdio.h>
#include <unistd.h>

#define VECS_UUID_CHARGER_ADC   0xffd1
#define VECS_UUID_CHARGER_OFF   0xffd2
#define VECS_UUID_CHARGER_LOW   0xffd3

#define VECS_UUID_KEY_PRESS     0xffe1
#define VECS_UUID_KEY_BEEP      0xffe2

#define VECS_UUID_MOTION_ACCEL  0xfff1
#define VECS_UUID_MOTION_GYRO   0xfff2
#define VECS_UUID_MOTION_RATE   0xfff3
#define VECS_UUID_MOTION_NOTI   0xfff4
#define VECS_UUID_MOTION_TEMP   0xfff5

typedef void (*vecs_connected_cb)(DEVHANDLER dev);
typedef void (*vecs_notify_cb)(DEVHANDLER dev, uint16_t uuid, uint8_t len, const void *data);

void vecs_connect(const char *addr, vecs_connected_cb connected, vecs_notify_cb notify);
void vecs_notify(DEVHANDLER devh, uint16_t uuid, uint8_t enable);
void vecs_request(DEVHANDLER devh, uint16_t uuid);
void vecs_write(DEVHANDLER devh, uint16_t uuid, uint8_t len, void *data);

#ifdef __cplusplus
}
#endif

#endif
