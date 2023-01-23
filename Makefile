include $(HOME)/bluez.mk

VERSION = 1
SUBVERSION = 0
PATCH = 0

TARGET0 = libble.so
TARGET = $(TARGET0).$(VERSION).$(SUBVERSION).$(PATCH)
TARGET1 = $(TARGET0).$(VERSION)
TARGET2 = $(TARGET0).$(VERSION).$(SUBVERSION)

BLUEZ_SRCS  = lib/bluetooth.c lib/hci.c lib/sdp.c lib/uuid.c
BLUEZ_SRCS += attrib/att.c attrib/gatt.c attrib/gattrib.c
BLUEZ_SRCS += btio/btio.c
BLUEZ_SRCS += src/log.c src/shared/log.c
BLUEZ_SRCS += src/shared/att.c src/shared/crypto.c src/shared/queue.c src/shared/timeout-glib.c src/shared/io-glib.c src/shared/util.c
IMPORT_SRCS = $(addprefix $(BLUEZ_PATH)/, $(BLUEZ_SRCS))

LIB_SRCS = lib/libble.c

INSTALL_PROGRAM = install -m 755 -p
SYMLINK = ln -f -s

CC = gcc
CFLAGS =

CPPFLAGS = -DVERSION=\"libble-test\"
CPPFLAGS += -I$(BLUEZ_PATH)/attrib -I$(BLUEZ_PATH) -I$(BLUEZ_PATH)/lib -I$(BLUEZ_PATH)/src -I$(BLUEZ_PATH)/gdbus -I$(BLUEZ_PATH)/btio

PROGCPPFLAGS += `pkg-config glib-2.0 --cflags`

CPPFLAGS += $(PROGCPPFLAGS)
LDLIBS += `pkg-config glib-2.0 --libs`

all: $(TARGET) motion info

$(TARGET): $(LIB_SRCS) $(IMPORT_SRCS)
	$(CC) -shared -L. $(CFLAGS) $(CPPFLAGS) -fPIC -o $@ $(LIB_SRCS) $(IMPORT_SRCS) $(LDLIBS)
	$(SYMLINK) $@ $(TARGET0)
	$(SYMLINK) $@ $(TARGET1)
	$(SYMLINK) $@ $(TARGET2)

motion: src/motion.c src/vecs.c $(TARGET)
	$(CC) -L. -Wl,-rpath=. -Wall -I./lib -o $@ src/motion.c src/vecs.c -lble $(PROGCPPFLAGS) $(LDLIBS)

info: src/info.c src/vecs.c $(TARGET)
	$(CC) -L. -Wl,-rpath=. -Wall -I./lib -o $@ src/info.c src/vecs.c -lble $(PROGCPPFLAGS) $(LDLIBS)

clean:
	rm -f libble.so libble.so.* motion info

install: $(TARGET)
	@test -d $(DESTDIR)/usr/lib || mkdir -p $(DESTDIR)/usr/lib
	-$(INSTALL) $(TARGET) $(DESTDIR)/usr/lib/$(TARGET)
	-$(SYMLINK) $(TARGET) $(DESTDIR)/usr/lib/$(TARGET0)
	-$(SYMLINK) $(TARGET) $(DESTDIR)/usr/lib/$(TARGET1)
	-$(SYMLINK) $(TARGET) $(DESTDIR)/usr/lib/$(TARGET2)
