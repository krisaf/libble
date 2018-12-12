/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2011  Nokia Corporation
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

#include "gattutils.h"

#include "lib/sdp.h"
#include "lib/uuid.h"
#include "att.h"

#include <glib.h>

GIOChannel *gatt_connect(const char *dst,
				BtIOConnect connect_cb,
				gpointer cb_info, GError **gerr)
{
	GIOChannel *chan;
	bdaddr_t sba, dba;
	GError *tmp_err = NULL;

	str2ba(dst, &dba);
	bacpy(&sba, BDADDR_ANY);

	chan = bt_io_connect(connect_cb, cb_info, NULL, &tmp_err,
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
