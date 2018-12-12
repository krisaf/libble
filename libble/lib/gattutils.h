#ifndef _GATTUTILS_H
#define _GATTUTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "btio/btio.h"

GIOChannel *gatt_connect(const char *dst,
			BtIOConnect connect_cb,
			gpointer cb_info, GError **gerr);

#ifdef __cplusplus
}
#endif

#endif /* _GATTUTILS_H */
