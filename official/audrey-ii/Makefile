# Project Name
TARGET = MajmaaSynth

CPP_STANDARD = -std=c++17

LIBDAISY_DIR = lib/libDaisy
DAISYSP_DIR = lib/DaisySP
CMSIS_DSP_SRC_DIR = ${LIBDAISY_DIR}/Drivers/CMSIS/DSP/Source

C_DEFS = -DTARGET_DAISY

# Config Options
ifeq ($(DEBUG), 1)
C_DEFS += -DLOG_ENABLED=1
endif

C_INCLUDES = \
	-ISource/

C_SOURCES = \
	${CMSIS_DSP_SRC_DIR}/CommonTables/arm_common_tables.c \
	${CMSIS_DSP_SRC_DIR}/ControllerFunctions/arm_sin_cos_f32.c

CPP_SOURCES = \
	Majmaa_main.cpp \
	Source/BiquadFilters.cpp \
	Source/MajmaaControls.cpp \
	Source/MajmaaEngine.cpp \
	Source/KarplusString.cpp \
	Source/memory/sdram_alloc.cpp

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

libs:
	cd $(LIBDAISY_DIR) && $(MAKE)
	cd $(DAISYSP_DIR) && $(MAKE)

clean-libs:
	cd $(LIBDAISY_DIR) && $(MAKE) clean
	cd $(DAISYSP_DIR) && $(MAKE) clean