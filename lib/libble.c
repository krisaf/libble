/*
 *
 *  libble - library for accessing Bluetooth Low-Energy devices over GATT level
 *
 *  Copyright (C) 2014  ICS-Micont
 *  Copyright (C) 2014  Dmitry Nikolaev <borealis@permlug.org>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <glib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include "libble.h"

#include "btio/btio.h"
#include "lib/sdp.h"
#include "lib/uuid.h"
#include "shared/util.h"

#include "gattrib.h"
#include "att.h"
#include "gatt.h"

typedef struct {
	devstate_t conn_state;
	GIOChannel *iochannel;
	GAttrib *attrib;
	lble_event_handler evh;
	void *user_data;
} devh_t;

typedef struct {
	devh_t *dev;
	uint16_t handle;
} request_t;

void emit(devh_t *dev, uint16_t handle, uint8_t len, const uint8_t *data)
{
	if (dev && dev->evh) {
		dev->evh(EVENT_INTERNAL, handle, len, data, dev);
	}
}

static gboolean channel_watcher(GIOChannel *ch, GIOCondition cond, gpointer user_data)
{
	devh_t *dev = user_data;
	switch (cond) {
		case G_IO_HUP:
		case G_IO_ERR:
		case G_IO_NVAL:
			lble_disconnect(user_data);
			break;
		case G_IO_IN:
		case G_IO_PRI:
			emit(dev, DATA_TO_READ, sizeof(user_data), user_data);
			break;
		case G_IO_OUT:
			emit(dev, DATA_TO_WRITE, sizeof(user_data), user_data);
			break;
		default:
			break;
	}
	return FALSE;
}

static void connect_cb(GIOChannel *io, GError *gerr, gpointer user_data)
{
	devh_t *dev = user_data;
	if (gerr) {
		emit(dev, ERROR_OCCURED, 0, gerr->message);
		g_error_free(gerr);
/*
		if (dev->iochannel) {
			GError *err_tmp = NULL;
			if (g_io_channel_shutdown(dev->iochannel, FALSE, &err_tmp) == G_IO_STATUS_ERROR) {
				g_error_free(err_tmp);
			}
			g_io_channel_unref(dev->iochannel);
			dev->iochannel = NULL;
		}
*/
		g_io_channel_unref(dev->iochannel);
		dev->iochannel = NULL;
		dev->conn_state = STATE_DISCONNECTED;
	} else {
		if (dev->conn_state == STATE_DISCONNECTED) {
			GError *err_tmp = NULL;
			if (g_io_channel_shutdown(dev->iochannel, FALSE, &err_tmp) == G_IO_STATUS_ERROR) {
				g_error_free(err_tmp);
			}
			g_io_channel_unref(dev->iochannel);
			dev->iochannel = NULL;
		} else {
			uint16_t mtu, cid;
			bt_io_get(io, &gerr, BT_IO_OPT_IMTU, &mtu, BT_IO_OPT_CID, &cid, BT_IO_OPT_INVALID);
			if (gerr) {
				emit(dev, ERROR_OCCURED, 0, gerr->message);
				g_error_free(gerr);
				mtu = ATT_DEFAULT_LE_MTU;
			}
			if (cid == ATT_CID) {
				mtu = ATT_DEFAULT_LE_MTU;
			}
			g_io_add_watch(dev->iochannel, G_IO_HUP | G_IO_ERR | G_IO_NVAL | G_IO_IN | G_IO_PRI | G_IO_OUT, channel_watcher, dev);
			dev->attrib = g_attrib_new(io, mtu, false);
			dev->conn_state = STATE_CONNECTED;
		}
	}
	emit(dev, STATE_CHANGED, 1, (void *)&dev->conn_state);
}

GIOChannel *gatt_connect(const char *dst, gpointer cb_info, GError **gerr)
{
	bdaddr_t sba, dba;
	str2ba(dst, &dba);
	bacpy(&sba, BDADDR_ANY);

	GError *tmp_err = NULL;
	GIOChannel *chan = bt_io_connect(connect_cb, cb_info, NULL, &tmp_err,
			BT_IO_OPT_SOURCE_BDADDR, &sba,
			BT_IO_OPT_SOURCE_TYPE, BDADDR_LE_PUBLIC,
			BT_IO_OPT_DEST_BDADDR, &dba,
			BT_IO_OPT_DEST_TYPE, BDADDR_LE_PUBLIC,
			BT_IO_OPT_CID, ATT_CID,
			BT_IO_OPT_SEC_LEVEL, BT_IO_SEC_LOW,
			BT_IO_OPT_INVALID);
	if (tmp_err) {
		g_propagate_error(gerr, tmp_err);
		return NULL;
	}
	return chan;
}

static void events_handler(const uint8_t *pdu, uint16_t len, gpointer user_data)
{
	devh_t *dev = user_data;
	if (dev->evh && pdu[0] == ATT_OP_HANDLE_NOTIFY) {
		dev->evh(EVENT_DEVICE, get_le16(&pdu[1]), len - 3, &pdu[3], dev);
	}
}

static void char_read_cb(guint8 status, const guint8 *pdu, guint16 plen, gpointer user_data)
{
	request_t *request = user_data;
	devh_t *dev = request->dev;

	if (status) {
		emit(dev, ERROR_OCCURED, 0, att_ecode2str(status));
	} else {
		ssize_t len = plen + 2;
		uint8_t *data = calloc(1, len);
		if (dec_read_resp(pdu, plen, &data[3], len - 3) > 0) {
			data[0] = ATT_OP_HANDLE_NOTIFY;
			put_le16(request->handle, &data[1]);
			events_handler(data, len, dev);
		}
		free(data);
	}
	free(request);
}

static void char_write_cb(guint8 status, const guint8 *pdu, guint16 plen, gpointer user_data)
{
	devh_t *dev = user_data;
	if (status) {
		emit(dev, ERROR_OCCURED, 0, att_ecode2str(status));
	}
}

/* exported functions */

/* create new handler */
DEVHANDLER lble_newdev()
{
	devh_t *dev = calloc(1, sizeof(devh_t));
	if (dev) {
		dev->conn_state = STATE_DISCONNECTED;
	}
	return dev;
}

/* free handler */
void lble_freedev(DEVHANDLER devh)
{
	if (devh) {
		free(devh);
	}
}

/* connect to BLE device */
void lble_connect(DEVHANDLER devh, const char *addr)
{
	devh_t *dev = devh;
	if (!dev || dev->conn_state != STATE_DISCONNECTED) {
		return;
	}
	dev->conn_state = STATE_CONNECTING;
	GError *gerr = NULL;
	dev->iochannel = gatt_connect(g_strdup(addr), dev, &gerr);
	if (!dev->iochannel) {
		emit(dev, ERROR_OCCURED, 0, gerr->message);
		g_error_free(gerr);
		dev->conn_state = STATE_DISCONNECTED;
		emit(dev, STATE_CHANGED, 1, (void *)&dev->conn_state);
	}
}

/* listen for notifications/indications */
void lble_listen(DEVHANDLER devh)
{
	devh_t *dev = devh;
	if (!dev || dev->conn_state != STATE_CONNECTED) {
		return;
	}
	g_attrib_register(dev->attrib, ATT_OP_HANDLE_NOTIFY, GATTRIB_ALL_HANDLES, events_handler, dev, NULL);
//	g_attrib_register(dev->attrib, ATT_OP_HANDLE_IND, GATTRIB_ALL_HANDLES, events_handler, dev, NULL);
}

/* disconnect from BLE device */
void lble_disconnect(DEVHANDLER devh)
{
	devh_t *dev = devh;
	if (!dev || dev->conn_state == STATE_DISCONNECTED) {
		return;
	}
	if (dev->conn_state == STATE_CONNECTING) {
		dev->conn_state = STATE_DISCONNECTED;
		return;
	}
	if (dev->attrib) {
		g_attrib_unref(dev->attrib);
		dev->attrib = NULL;
	}
	if (dev->iochannel) {
//		GError *gerr = NULL;
//		if (g_io_channel_shutdown(dev->iochannel, FALSE, &gerr) == G_IO_STATUS_ERROR) {
//			g_error_free(gerr);
//		}
//		g_io_channel_unref(dev->iochannel);
		dev->iochannel = NULL;
	}
	dev->conn_state = STATE_DISCONNECTED;
	emit(dev, STATE_CHANGED, 1, (void *)&dev->conn_state);
}

/* set event handler */
void lble_set_event_handler(DEVHANDLER devh, lble_event_handler handler)
{
	devh_t *dev = devh;
	if (dev) {
		dev->evh = handler;
	}
}

/* set user data */
void lble_set_user_data(DEVHANDLER devh, void *user_data)
{
	devh_t *dev = devh;
	if (dev) {
		dev->user_data = user_data;
	}
}

/* get user data */
void *lble_user_data(DEVHANDLER devh)
{
	devh_t *dev = devh;
	return dev ? dev->user_data : NULL;
}

/* get connection state */
devstate_t lble_state(DEVHANDLER devh)
{
	devh_t *dev = devh;
	return dev ? dev->conn_state : STATE_DISCONNECTED;
}

/* read characteristic by handle */
void lble_request(DEVHANDLER devh, uint16_t handle)
{
	devh_t *dev = devh;
	if (!dev || dev->conn_state != STATE_CONNECTED) {
		return;
	}
	request_t *request = calloc(1, sizeof(request_t));
	request->dev = dev;
	request->handle = handle;
	gatt_read_char(dev->attrib, handle, char_read_cb, request);
}

/* write characteristic by handle */
void lble_write(DEVHANDLER devh, uint16_t handle, uint8_t len, void *data)
{
	devh_t *dev = devh;
	if (!dev || dev->conn_state != STATE_CONNECTED) {
		return;
	}
	if (data && len > 0) {
		gatt_write_char(dev->attrib, handle, data, len, char_write_cb, dev);
	}
}
