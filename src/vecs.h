#ifndef _VECS_H
#define _VECS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "libble.h"

#include <stdio.h>
#include <unistd.h>

#define UUID_NOTIFY "0000ffe4-0000-1000-8000-00805f9a34fb"
#define UUID_WRITE "0000ffe9-0000-1000-8000-00805f9a34fb"

typedef void (*vecs_connected_cb)(DEVHANDLER dev);
typedef void (*vecs_notify_cb)(DEVHANDLER dev, char *uuid128, uint8_t len, const void *data);

void vecs_connect(const char *addr, vecs_connected_cb connected, vecs_notify_cb notify);
void vecs_notify(DEVHANDLER devh, char *uuid, uint8_t enable);
void vecs_request(DEVHANDLER devh, char *uuid);
void vecs_write(DEVHANDLER devh, char *uuid, uint8_t len, void *data);

#ifdef __cplusplus
}
#endif

#endif
