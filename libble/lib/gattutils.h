#ifndef _GATTUTILS_H
#define _GATTUTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "btio/btio.h"

GIOChannel *gatt_connect(const char *src, const char *dst,
			const char *dst_type, const char *sec_level,
			int psm, int mtu, BtIOConnect connect_cb,
			gpointer cb_info, GError **gerr);

size_t gatt_attr_data_from_string(const char *str, uint8_t **data);

#ifdef __cplusplus
}
#endif

#endif /* _GATTUTILS_H */
