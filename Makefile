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

all: $(TARGET).elf

$(TARGET).elf: $(OBJS) $(START) $(OUTDIR)
	@ echo [Linking...]
	@ $(CC) $(OBJS) $(CFLAGS) $(START) -o $@
	@ echo Creating Hex file...
	@$(OBJCOPY) -O ihex -S $(TARGET).elf $(TARGET).hex
	@$(OBJCOPY) -O binary -S $(TARGET).elf $(TARGET).bin
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

clean:
	@ echo [Clean...]
	@ $(RM) $(OBJS) $(DEPS)
	@ echo done

download:
	@ openocd -f $(DOWNCFG)
