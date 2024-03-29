###############################################################################
# Makefile for the project SpriteDemo
###############################################################################

## General Flags
PROJECT = shooter
GAME= shooter
MCU = atmega644
TARGET = $(GAME).elf
CC = avr-gcc
INFO=../gameinfo.properties

## Kernel settings
KERNEL_DIR = ../../trunk/kernel
KERNEL_OPTIONS  = -DVIDEO_MODE=3 -DINTRO_LOGO=0
KERNEL_OPTIONS += -DSCROLLING=1 -DOVERLAY_LINES=2
KERNEL_OPTIONS += -DMAX_SPRITES=18 -DRAM_TILES_COUNT=24
KERNEL_OPTIONS += -DFIRST_RENDER_LINE=28 -DSCREEN_TILES_V=26 -DVRAM_TILES_V=24
KERNEL_OPTIONS += -DSOUND_CHANNEL_3_ENABLE=0


## Options common to compile, link and assembly rules
COMMON = -mmcu=$(MCU)

## Compile options common for all C compilation units.
CFLAGS = $(COMMON)
CFLAGS += -Wall -gdwarf-2 -std=gnu99 -DF_CPU=28636360UL -Os -fsigned-char -ffunction-sections -fno-toplevel-reorder
CFLAGS += -MD -MP -MT $(*F).o -MF dep/$(@F).d 
CFLAGS += $(KERNEL_OPTIONS)


## Assembly specific flags
ASMFLAGS = $(COMMON)
ASMFLAGS += $(CFLAGS)
ASMFLAGS += -x assembler-with-cpp -Wa,-gdwarf2

## Linker flags
LDFLAGS = $(COMMON)
LDFLAGS += -Wl,-Map=$(GAME).map 
LDFLAGS += -Wl,-gc-sections 


## Intel Hex file production flags
HEX_FLASH_FLAGS = -R .eeprom

HEX_EEPROM_FLAGS = -j .eeprom
HEX_EEPROM_FLAGS += --set-section-flags=.eeprom="alloc,load"
HEX_EEPROM_FLAGS += --change-section-lma .eeprom=0 --no-change-warnings


## Objects that must be built in order to link
OBJECTS = uzeboxVideoEngineCore.o uzeboxCore.o uzeboxSoundEngine.o uzeboxSoundEngineCore.o uzeboxVideoEngine.o $(GAME).o 

## Objects explicitly added by the user
LINKONLYOBJECTS = 

## Include Directories
INCLUDES = -I"$(KERNEL_DIR)" 

## Included data files
DATA_FILES =  ../data/overlay.inc ../data/sprites.inc
DATA_FILES += ../data/tiles1.inc ../data/tiles2.inc
DATA_FILES += ../data/level1.inc ../data/level2.inc
DATA_FILES += ../data/level3.inc ../data/level4.inc

## Build
all: $(TARGET) $(GAME).hex $(GAME).eep $(GAME).lss $(GAME).uze

../data/overlay.inc: ../data/overlay.png ../data/overlay.gconvert.xml
	gconvert ../data/overlay.gconvert.xml

../data/sprites.inc: ../data/sprites.png ../data/sprites.gconvert.xml
	gconvert ../data/sprites.gconvert.xml

../data/tiles1.inc: ../data/tiles1.png ../data/tiles1.gconvert.xml
	gconvert ../data/tiles1.gconvert.xml

../data/tiles2.inc: ../data/tiles2.png ../data/tiles2.gconvert.xml
	gconvert ../data/tiles2.gconvert.xml

../data/level1.inc: ../data/level1.tmx tiledconv.pl
	./tiledconv.pl $< > $@

../data/level2.inc: ../data/level2.tmx tiledconv.pl
	./tiledconv.pl $< > $@

../data/level3.inc: ../data/level3.tmx tiledconv.pl
	./tiledconv.pl $< > $@

../data/level4.inc: ../data/level4.tmx tiledconv.pl
	./tiledconv.pl $< > $@

## Compile Kernel files
uzeboxVideoEngineCore.o: $(KERNEL_DIR)/uzeboxVideoEngineCore.s
	$(CC) $(INCLUDES) $(ASMFLAGS) -c  $<

uzeboxSoundEngineCore.o: $(KERNEL_DIR)/uzeboxSoundEngineCore.s
	$(CC) $(INCLUDES) $(ASMFLAGS) -c  $<

uzeboxCore.o: $(KERNEL_DIR)/uzeboxCore.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

uzeboxSoundEngine.o: $(KERNEL_DIR)/uzeboxSoundEngine.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

uzeboxVideoEngine.o: $(KERNEL_DIR)/uzeboxVideoEngine.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

## Compile game sources
$(GAME).o: ../$(GAME).c $(DATA_FILES)
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

##Link
$(TARGET): $(OBJECTS)
	 $(CC) $(LDFLAGS) $(OBJECTS) $(LINKONLYOBJECTS) $(LIBDIRS) $(LIBS) -o $(TARGET)

%.hex: $(TARGET)
	avr-objcopy -O ihex $(HEX_FLASH_FLAGS)  $< $@

%.eep: $(TARGET)
	-avr-objcopy $(HEX_EEPROM_FLAGS) -O ihex $< $@ || exit 0

%.lss: $(TARGET)
	avr-objdump -h -S $< > $@

%.uze: $(TARGET)
	-packrom $(GAME).hex $@ $(INFO)

UNAME := $(shell sh -c 'uname -s 2>/dev/null || echo not')
AVRSIZEFLAGS := -A ${TARGET}
ifneq (,$(findstring MINGW,$(UNAME)))
AVRSIZEFLAGS := -C --mcu=${MCU} ${TARGET}
endif

size: ${TARGET}
	@echo
	@avr-size -C --mcu=${MCU} ${TARGET}

## Clean target
.PHONY: clean
clean:
	-rm -rf $(OBJECTS) $(GAME).* dep/* *.uze $(DATA_FILES)


## Other dependencies
-include $(shell mkdir dep 2>/dev/null) $(wildcard dep/*)

