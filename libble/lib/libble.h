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

typedef void * DEVHANDLER;

typedef void (*lble_event_handler)(uint16_t handle, uint8_t len, const uint8_t *data, const void *cb_info);

extern DEVHANDLER lble_newdev();
extern void lble_freedev(DEVHANDLER devh);
extern void lble_connect(DEVHANDLER devh, const char *addr);
extern void lble_disconnect(DEVHANDLER devh);

extern void lble_listen(DEVHANDLER devh, lble_event_handler handler, void *cb_info);

extern uint8_t lble_read(DEVHANDLER devh, uint16_t handle, uint8_t *data);
extern void lble_write(DEVHANDLER devh, uint16_t handle, uint8_t len, uint8_t *data);

extern devstate_t lble_get_state(DEVHANDLER devh);

#ifdef __cplusplus
}
#endif

#endif
