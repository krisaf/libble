VERSION = 2
SUBVERSION = 0
PATCH = 0

TARGET0 = libble.so
TARGET = $(TARGET0).$(VERSION).$(SUBVERSION).$(PATCH)
TARGET1 = $(TARGET0).$(VERSION)
TARGET2 = $(TARGET0).$(VERSION).$(SUBVERSION)

BLUEZ_PATH = bluez
BLUEZ_SRCS += lib/uuid.c
BLUEZ_SRCS += attrib/att.c attrib/gatt.c attrib/gattrib.c
BLUEZ_SRCS += btio/btio.c
BLUEZ_SRCS += src/log.c src/shared/log.c
BLUEZ_SRCS += src/shared/att.c src/shared/crypto.c src/shared/queue.c src/shared/timeout-glib.c src/shared/io-glib.c src/shared/util.c
IMPORT_SRCS += $(addprefix $(BLUEZ_PATH)/, $(BLUEZ_SRCS))

LIB_SRCS = lib/libble.c

INSTALL_PROGRAM = install -p
SYMLINK = ln -f -s

CC = gcc
CFLAGS =

CPPFLAGS = -DVERSION=\"libble-test\"
CPPFLAGS += -I$(BLUEZ_PATH)

PKG-CONFIG += glib-2.0 bluez

PROGCPPFLAGS += $(shell pkg-config --cflags $(PKG-CONFIG))
PROGCPPFLAGS += -I/usr/include/bluetooth
LDFLAGS += $(shell pkg-config --libs $(PKG-CONFIG))
CPPFLAGS += $(PROGCPPFLAGS)

all: $(TARGET) motion #info

$(TARGET): $(LIB_SRCS) $(IMPORT_SRCS)
	$(CC) -shared -L. $(CFLAGS) $(CPPFLAGS) -fPIC -o $@ $(LIB_SRCS) $(IMPORT_SRCS) $(LDFLAGS)
	$(SYMLINK) $@ $(TARGET0)
	$(SYMLINK) $@ $(TARGET1)
	$(SYMLINK) $@ $(TARGET2)

motion: src/motion.c src/vecs.c $(TARGET)
	$(CC) -L. -Wl,-rpath=. -Wall -I./lib -o $@ src/motion.c src/vecs.c -lble $(PROGCPPFLAGS) $(LDFLAGS)

#info: src/info.c src/vecs.c $(TARGET)
#	$(CC) -L. -Wl,-rpath=. -Wall -I./lib -o $@ src/info.c src/vecs.c -lble $(PROGCPPFLAGS) $(LDFLAGS)

clean:
	rm -f libble.so libble.so.* motion #info

install: $(TARGET)
	@test -d $(DESTDIR)/usr/lib/x86_64-linux-gnu || mkdir -p $(DESTDIR)/usr/lib/x86_64-linux-gnu
	-$(INSTALL) -m 755 $(TARGET) $(DESTDIR)/usr/lib/x86_64-linux-gnu/$(TARGET)
	-$(SYMLINK) $(TARGET) $(DESTDIR)/usr/lib/x86_64-linux-gnu/$(TARGET0)
	-$(SYMLINK) $(TARGET) $(DESTDIR)/usr/lib/x86_64-linux-gnu/$(TARGET1)
	-$(SYMLINK) $(TARGET) $(DESTDIR)/usr/lib/x86_64-linux-gnu/$(TARGET2)
	@test -d $(DESTDIR)/usr/lib/x86_64-linux-gnu/pkgconfig || mkdir -p $(DESTDIR)/usr/lib/x86_64-linux-gnu/pkgconfig
	-$(INSTALL) -m 644 rtble.pc $(DESTDIR)/usr/lib/x86_64-linux-gnu/pkgconfig/
	@test -d $(DESTDIR)/usr/include/ble || mkdir -p $(DESTDIR)/usr/include/ble
	-$(INSTALL) -m 644 lib/*.h $(DESTDIR)/usr/include/ble
