#ifndef _LIBBLE_H
#define _LIBBLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum {
	STATE_DISCONNECTED = 0,
	STATE_CONNECTING,
	STATE_CONNECTED
} devstate_t;

typedef enum {
	EVENT_INTERNAL = 0,
	EVENT_DEVICE
} event_t;

typedef enum {
	STATE_CHANGED = 0,
	ERROR_OCCURED,
	DATA_TO_READ,
	DATA_TO_WRITE
} handle_t;

typedef void * DEVHANDLER;
typedef void (*lble_event_handler)(event_t event, uint16_t handle, uint8_t len, const uint8_t *data, DEVHANDLER dev);

extern DEVHANDLER lble_newdev();
extern void lble_freedev(DEVHANDLER devh);
extern void lble_connect(DEVHANDLER devh, const char *addr);
extern void lble_disconnect(DEVHANDLER devh);
extern void lble_listen(DEVHANDLER devh);
extern void lble_request(DEVHANDLER devh, uint16_t handle);
extern void lble_write(DEVHANDLER devh, uint16_t handle, uint8_t len, uint8_t *data);

extern void lble_set_event_handler(DEVHANDLER devh, lble_event_handler handler);
extern void lble_set_user_data(DEVHANDLER devh, void *user_data);
extern void *lble_user_data(DEVHANDLER devh);
extern devstate_t lble_state(DEVHANDLER devh);

#ifdef __cplusplus
}
#endif

#endif
