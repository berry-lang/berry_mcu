CFLAGS	 = -g -Wall -Wextra -Os
CFLAGS	+= -DSTM32F10X_HD -DUSE_STDPERIPH_DRIVER
CFLAGS	+= -ffunction-sections -Wl,-gc-sections -mcpu=cortex-m3 -mthumb 
CFLAGS  += -specs=nosys.specs -specs=nano.specs -u _printf_float
CFLAGS	+= -T"./toolcfg/stm32f10x_flash.ld"
CFLAGS	+= -lm
DOWNCFG	+= "./toolcfg/stm32f10x_download.cfg"
DEBUG	 = -DDEBUG
OUTDIR	 = output
TARGET	 = $(OUTDIR)/app
CC	 = arm-none-eabi-gcc
OBJCOPY	 = arm-none-eabi-objcopy
SIZE	 = arm-none-eabi-size
MKDIR	 = mkdir

BERRY_PATH = middleware/berry
GENERATE   = generate
MAP_BUILD  = $(BERRY_PATH)/tools/map_build/map_build
STR_BUILD  = $(BERRY_PATH)/tools/str_build/str_build

INCPATH	 = ./core				\
	   ./hardware				\
	   ./std_lib/inc			\
	   ./middleware/berry/src		\
	   ./user
SRCPATH	 = ./core 				\
	   ./hardware 				\
	   ./std_lib/src 			\
	   ./middleware/berry/src		\
	   ./user
START 	 = ./core/startup/gcc/startup_stm32f10x_hd.s

SRCS	 = $(foreach dir, $(SRCPATH), $(wildcard $(dir)/*.c))
OBJS	 = $(patsubst %.c, %.o, $(SRCS))
DEPS	 = $(patsubst %.c, %.d, $(SRCS))
CFLAGS	+= $(foreach dir, $(INCPATH), -I"$(dir)")

ifeq ($(debug), 1)
    CFLAGS += $(DEBUG)
endif

ifeq ($(OS), Windows_NT) # Windows
    MAP_BUILD := $(MAP_BUILD).exe
    STR_BUILD := $(STR_BUILD).exe
endif

all: $(TARGET).elf

$(TARGET).elf: $(OBJS) $(START) $(OUTDIR)
	@ echo [Linking...]
	@ $(CC) $(OBJS) $(CFLAGS) $(START) -o $@
	@ echo Creating Hex file...
	@ $(OBJCOPY) -O ihex -S $(TARGET).elf $(TARGET).hex
	@ $(OBJCOPY) -O binary -S $(TARGET).elf $(TARGET).bin
	@ echo Build info:
	@ $(SIZE) $(TARGET).elf
	@ echo done

$(OBJS): %.o: %.c
	@ echo [Compile] $<
	@ $(CC) -MM $(CFLAGS) -MT"$*.d" -MT"$(<:.c=.o)" $< > $*.d
	@ $(CC) $(CFLAGS) -c $< -o $@

sinclude $(DEPS)

$(OUTDIR):
	@ $(MKDIR) $(OUTDIR)

$(GENERATE):
	@ $(MKDIR) $(GENERATE)

prebuild: $(STR_BUILD) $(MAP_BUILD) $(GENERATE)
	@ echo [Prebuild] generate resources
	@ $(MAP_BUILD) $(GENERATE) $(SRCPATH)
	@ $(STR_BUILD) $(GENERATE) $(SRCPATH) $(GENERATE)
	@ echo done

$(STR_BUILD):
	@ echo [Make] str_build
	@ $(MAKE) -C $(BERRY_PATH)/tools/str_build -s

$(MAP_BUILD):
	@ echo [Make] map_build
	@ $(MAKE) -C $(BERRY_PATH)/tools/map_build -s

clean:
	@ echo [Clean...]
	@ $(RM) $(OBJS) $(DEPS)
	@ echo done

download:
	@ openocd -f $(DOWNCFG)
