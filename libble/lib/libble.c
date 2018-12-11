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
#include <unistd.h>
#include <stdbool.h>

#include "gattrib.h"

#include "libble.h"
#include "gattutils.h"

#include "lib/sdp.h"
#include "lib/uuid.h"
#include "btio/btio.h"
#include "att.h"
#include "gatt.h"
#include "shared/util.h"

typedef struct {
	devstate_t conn_state;
	GMainLoop *event_loop;
	GIOChannel *iochannel;
	GAttrib *attrib;
	uint8_t len;
	uint8_t *buffer;
	lble_event_handler evh;
	void *cb_info;
} devh_t;

static void events_handler(const uint8_t *pdu, uint16_t len, gpointer user_data)
{
	devh_t *dev = user_data;

	if (pdu[0] != ATT_OP_HANDLE_NOTIFY)
		return;

// calling out hi-level handler
	if (dev->evh != NULL)
		dev->evh(get_le16(&pdu[1]), len - 3, &pdu[3], dev->cb_info);
}

static void connect_cb(GIOChannel *io, GError *err, gpointer user_data)
{
	uint16_t mtu, cid;
	devh_t *dev = user_data;

	if (err) {
		dev->conn_state = STATE_DISCONNECTED;
		fprintf(stderr, "LBLE::ERROR %s\n", err->message);
		goto done;
	}

	bt_io_get(io, &err, BT_IO_OPT_IMTU, &mtu, BT_IO_OPT_CID, &cid, BT_IO_OPT_INVALID);
	if (err) {
		fprintf(stderr, "LBLE::ERROR %s\n", err->message);
		g_error_free(err);
		mtu = ATT_DEFAULT_LE_MTU;
	}

	if (cid == ATT_CID)
		mtu = ATT_DEFAULT_LE_MTU;

	dev->attrib = g_attrib_new(io, mtu, false);
	dev->conn_state = STATE_CONNECTED;
done:
	g_main_loop_quit(dev->event_loop);
}

static gboolean channel_watcher(GIOChannel *ch, GIOCondition cond, gpointer user_data)
{
	lble_disconnect(user_data);
	return FALSE;
}


/* exported functions */

DEVHANDLER lble_newdev()
{
	devh_t *dev;

	dev = calloc(1, sizeof(devh_t));
	if (!dev) {
		return NULL;
	}
	dev->conn_state = STATE_DISCONNECTED;
	return dev;
}

void lble_freedev(DEVHANDLER devh)
{
	free(devh);
}

/* connect to BLE device */
void lble_connect(DEVHANDLER devh, const char *addr)
{
	GError *gerr = NULL;
	devh_t *dev = devh;

	if (!dev || dev->conn_state != STATE_DISCONNECTED)
		return;

	dev->event_loop = g_main_loop_new(NULL, FALSE);
	dev->conn_state = STATE_CONNECTING;

	dev->iochannel = gatt_connect(g_strdup(addr), connect_cb, dev, &gerr);
	if (dev->iochannel == NULL) {
		dev->conn_state = STATE_DISCONNECTED;
		fprintf(stderr, "LBLE::ERROR %s\n", gerr->message);
		g_error_free(gerr);
	} else {
		g_io_add_watch(dev->iochannel, G_IO_HUP, channel_watcher, dev);
	}
	g_main_loop_run(dev->event_loop);
}

/* listen for notifications/indications */
void lble_listen(DEVHANDLER devh, lble_event_handler handler, void *cb_info)
{
	devh_t *dev = devh;

	if (!dev || dev->conn_state != STATE_CONNECTED)
		return;

	// setting our own notify/ind high-level handler
	dev->evh = handler;
	dev->cb_info = cb_info;

	g_attrib_register(dev->attrib, ATT_OP_HANDLE_NOTIFY, GATTRIB_ALL_HANDLES, events_handler, dev, NULL);
//	g_attrib_register(dev->attrib, ATT_OP_HANDLE_IND, GATTRIB_ALL_HANDLES, events_handler, dev, NULL);

	g_main_loop_run(dev->event_loop);
}

/* disconnect from BLE device */
void lble_disconnect(DEVHANDLER devh)
{
	devh_t *dev = devh;
	GError *gerr = NULL;

	if (!dev || dev->conn_state == STATE_DISCONNECTED)
		return;

	dev->conn_state = STATE_DISCONNECTED;

	g_attrib_unref(dev->attrib);
	dev->attrib = NULL;

//	g_io_channel_shutdown(dev->iochannel, FALSE, &gerr);
	g_io_channel_unref(dev->iochannel);
	dev->iochannel = NULL;

	g_main_loop_quit(dev->event_loop);
	g_main_loop_unref(dev->event_loop);
	dev->event_loop = NULL;
}

/* get connection state */
devstate_t lble_get_state(DEVHANDLER devh)
{
	devh_t *dev = devh;

	return dev->conn_state;
}

static void char_read_cb(guint8 status, const guint8 *pdu, guint16 plen, gpointer user_data)
{
	devh_t *dev = user_data;

	if (status != 0) {
		fprintf(stderr, "LBLE::ERROR value read failed: %s\n", att_ecode2str(status));
		dev->len = 0;
		goto done;
	}

	dev->len = dec_read_resp(pdu, plen, dev->buffer, plen);

done:
	g_main_loop_quit(dev->event_loop);
}

/* read characteristic by handle */
uint8_t lble_read(DEVHANDLER devh, uint16_t handle, uint8_t *data)
{
	devh_t *dev = devh;

	if (!dev || dev->conn_state != STATE_CONNECTED)
		return;

	dev->len = 0;
	dev->buffer = data;

	gatt_read_char(dev->attrib, handle, char_read_cb, dev);

	g_main_loop_run(dev->event_loop);

	return dev->len;
}

static void char_write_cb(guint8 status, const guint8 *pdu, guint16 plen, gpointer user_data)
{
	devh_t *dev = user_data;

	if (status != 0) {
		fprintf(stderr, "LBLE::ERROR value write failed: %s\n", att_ecode2str(status));
		goto done;
	}

done:
	g_main_loop_quit(dev->event_loop);
}

/* write characteristic by handle */
void lble_write(DEVHANDLER devh, uint16_t handle, uint8_t len, uint8_t *data)
{
	devh_t *dev = devh;

	if (!dev || dev->conn_state != STATE_CONNECTED)
		return;

	if (data != NULL && len > 0) {
		gatt_write_char(dev->attrib, handle, data, len, char_write_cb, dev);
		g_main_loop_run(dev->event_loop);
	}
}
