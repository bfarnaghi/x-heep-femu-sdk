# Copyright 2023 EPFL
# Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
# SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
#
# Author: Simone Machetti - simone.machetti@epfl.ch

MAKE               = make

RISCV              ?= /tools/riscv
RISCV_EXE_PREFIX   = $(RISCV)/bin/riscv32-unknown-elf-

CUSTOM_GCC_FLAGS   = 
LIB_CRT            = $(wildcard ../../lib/crt/*.S)
LIB_BASE           = $(wildcard ../../lib/base/*.c)
LIB_RUNTIME        = $(wildcard ../../lib/runtime/*.c)
LIB_DRIVERS        = $(wildcard ../../lib/hal/soc_ctrl/*.c)
LIB_DRIVERS        += $(wildcard ../../lib/hal/uart/*.c)
LIB_DRIVERS        += $(wildcard ../../lib/hal/rv_plic/*.c)
LIB_DRIVERS        += $(wildcard ../../lib/hal/rv_timer/*.c)
LIB_DRIVERS        += $(wildcard ../../lib/hal/gpio/*.c)
LIB_DRIVERS        += $(wildcard ../../lib/hal/i2c/*.c)
LIB_DRIVERS        += $(wildcard ../../lib/hal/dma/*.c)
LIB_DRIVERS        += $(wildcard ../../lib/hal/power_manager/*.c)
LIB_DRIVERS        += $(wildcard ../../lib/hal/fast_intr_ctrl/*.c)
LIB_DRIVERS        += $(wildcard ../../lib/hal/spi_mem/*.c)
LIB_DRIVERS        += $(wildcard ../../lib/hal/spi_memio/*.c)
LIB_DRIVERS        += $(wildcard ../../lib/hal/spi_host/*.c)
LIB_DRIVERS        += $(wildcard ../../lib/hal/pad_control/*.c)
INC_FOLDERS        = $(sort $(dir $(wildcard ../../lib/target/)))
INC_FOLDERS        += $(sort $(dir $(wildcard ../../lib/base/)))
INC_FOLDERS        += $(sort $(dir $(wildcard ../../lib/hal/*/)))
INC_FOLDERS        += $(sort $(dir $(wildcard ../../lib/runtime/)))
INC_FOLDERS				   += $(sort $(dir $(wildcard ../../lib/tflite-micro/tensorflow)))
INC_FOLDERS				   += $(sort $(dir $(wildcard ../../lib/tflite-micro/third_party/flatbuffers/include/)))
INC_FOLDERS				   += $(sort $(dir $(wildcard ../../lib/tflite-micro/third_party/kissfft)))
INC_FOLDERS				   += $(sort $(dir $(wildcard ../../lib/tflite-micro/third_party/gemmlowp/*)))
INC_FOLDERS				   += $(sort $(dir $(wildcard ../../lib/tflite-micro/third_party/ruy)))
INC_FOLDERS				   += $(sort $(dir $(wildcard ../../lib/tflite-micro/third_party/*)))
INC_FOLDERS				   += $(sort $(dir $(wildcard scpi-parser/libscpi/inc/scpi/)))
INC_FOLDERS += .

LIBSCPI = ./scpi-parser/libscpi
LIBSCPI_SRC = $(wildcard $(LIBSCPI)/src/*.c)
LIBSCPI_OBJ = $(LIBSCPI_SRC:.c=.c.o)

LIBTFLM = ../../lib/tflite-micro/bin/libtflm.a

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.c.o)

CPP_SRCS = $(wildcard *.cc)
CPP_OBJS = $(CPP_SRCS:.cc=.cc.o)

INC_FOLDERS_GCC    = $(addprefix -I ,$(INC_FOLDERS))

TFLITE_COMMON_FLAGS = -fno-unwind-tables -fno-exceptions -fmessage-length=0 \
				-DTF_LITE_STATIC_MEMORY \
				-DTF_LITE_DISABLE_X86_NEON \
				-mabi=ilp32 \
				-mcmodel=medany \
				-mexplicit-relocs \
				-DTF_LITE_MCU_DEBUG_LOG \
				-DTF_LITE_USE_GLOBAL_CMATH_FUNCTIONS \
				-funsigned-char \
				-fno-delete-null-pointer-checks \
				-fomit-frame-pointer

TFLITE_CPP_FLAGS = -fno-use-cxa-atexit \
				-fpermissive \
				-fno-rtti \
				-fno-threadsafe-statics \
				-Wnon-virtual-dtor \
				-DTF_LITE_USE_GLOBAL_MIN \
				-DTF_LITE_USE_GLOBAL_MAX

INC_FOLDERS_GCC    = $(addprefix -I ,$(INC_FOLDERS))

%.c.o: %.c
	$(RISCV_EXE_PREFIX)gcc -march=rv32imc -c -o $@ -w -Os -g -nostdlib -std=c11 \
		$(CUSTOM_GCC_FLAGS) \
		-I $(RISCV)/riscv32-unknown-elf/include \
		$(TFLITE_COMMON_FLAGS) \
		$(INC_FOLDERS_GCC) \
		-static \
		$^ \
		-ffunction-sections -fdata-sections -specs=nano.specs


%.cc.o: %.cc
	$(RISCV_EXE_PREFIX)g++ -march=rv32imc -c -o $@ -w -Os -g -nostdlib -std=c++14 \
		$(CUSTOM_GCC_FLAGS) \
		-DHOST_BUILD \
		$(TFLITE_COMMON_FLAGS) \
		$(TFLITE_CPP_FLAGS) \
		-I $(RISCV)/riscv32-unknown-elf/include \
		$(INC_FOLDERS_GCC) \
		-static \
		$^ \
		-ffunction-sections -fdata-sections -specs=nano.specs


%.bin: %.elf
	$(RISCV_EXE_PREFIX)objcopy -O binary $< $@

%.dis: %.elf
	$(RISCV_EXE_PREFIX)objdump -S $< > $@

tflite_scpi.elf: $(CPP_OBJS) $(OBJS) $(LIBSCPI_OBJ) $(LIBTFLM)
	$(RISCV_EXE_PREFIX)gcc -march=rv32imc -o $@ -w -Os -g -std=gnu11 -nostdlib \
		$(CUSTOM_GCC_FLAGS) \
                -DHOST_BUILD \
                -T link.ld \
                -I $(RISCV)/riscv32-unknown-elf/include \
                $(INC_FOLDERS_GCC) \
                -static \
                $(LIB_CRT) \
                $(OBJS) $(CPP_OBJS) $(LIBSCPI_OBJ) $(LIBTFLM) $(LIB_RUNTIME) \
                $(LIB_BASE) \
                $(LIB_DRIVERS) \
                -Wl,-Map=tflite_scpi.map \
                -L $(RISCV)/riscv32-unknown-elf/lib \
                -lc -lm -lgcc -lstdc++ -flto -ffunction-sections -fdata-sections -specs=nano.specs

clean:
	rm -f $(OBJS) $(LIBSCPI_OBJ)
