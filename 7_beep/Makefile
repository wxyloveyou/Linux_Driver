CROSS_COMPILE = arm-linux-gnueabihf-
CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld
OBJCOPY :=$(CROSS_COMPILE)objcopy
OBJDUMP :=$(CROSS_COMPILE)objdump


CURRENT_PATH := $(shell pwd)
KERNEL_DIR := /home/wxy/linux/IMX6ULL/linux/linux-imx-rel_imx_4.1.15_2.1.0_ga
KERNEL_INCLUDE := $(KERNEL_DIR)/include
obj-m := beep.o

all:build
build: kernel_modules

kernel_modules:
	$(MAKE) -C $(KERNEL_DIR) M=$(CURRENT_PATH) modules
clean:
	#$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean