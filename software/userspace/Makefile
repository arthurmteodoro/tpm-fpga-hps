TARGET = main
ALT_DEVICE_FAMILY ?= soc_cv_av
HWLIBS_ROOT = $(SOCEDS_DEST_ROOT)/ip/altera/hps/altera_hps/hwlib

CFLAGS = -static -g -Wall -std=gnu99 -I${HWLIBS_ROOT}/include -I$(HWLIBS_ROOT)/include/$(ALT_DEVICE_FAMILY) -D$(ALT_DEVICE_FAMILY) -Iinclude/ -Iusr/include -Incurses_arm/include
LDFLAGS =  -g -Wall -lm -Lncurses_arm/lib -Lusr/lib
CC = $(CROSS_COMPILE)gcc
ARCH= arm

C_FILES := $(wildcard src/*.c)
OBJ_FILES := $(addprefix build/,$(notdir $(C_FILES:.c=.o)))

.PHONY : all
all : $(TARGET)

build: $(TARGET)
$(TARGET): $(OBJ_FILES)
	$(CC) $(LDFLAGS) -o $@ $^
$(OBJ_FILES): build/%.o : src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	rm -f $(TARGET) build/*.a build/*.o build/*~ 