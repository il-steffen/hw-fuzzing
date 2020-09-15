# Copyright 2020 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

################################################################################
# Directories
################################################################################
TB_DIR := tb/$(TB_TYPE)/$(TB)
ifeq ($(TB_TYPE), cocotb)
export TB_SRCS_DIR        := $(shell cocotb-config --share)/lib/verilator
export TB_INCS_DIR        :=
export SHARED_TB_SRCS_DIR :=
export SHARED_TB_INCS_DIR :=
else
export TB_SRCS_DIR        := $(TB_DIR)/src
export TB_INCS_DIR        := $(TB_DIR)/inc
export SHARED_TB_SRCS_DIR := ../tb/$(TB_TYPE)/src
export SHARED_TB_INCS_DIR := ../tb/$(TB_TYPE)/inc
endif
export MODEL_DIR := model
export BUILD_DIR := build
export BIN_DIR   := bin

################################################################################
# Sources/Inputs
################################################################################
ifeq ($(TB_TYPE), cocotb)
export TB_SRCS := $(shell cocotb-config --share)/lib/verilator/verilator.cpp
TB_MODULE      := tb.$(TB_TYPE).$(TB).$(TOPLEVEL)_tb
else
export TB_SRCS        := $(wildcard $(TB_SRCS_DIR)/*.cpp)
export SHARED_TB_SRCS := $(wildcard $(SHARED_TB_SRCS_DIR)/*.cpp)
endif
MODEL_SRC := $(MODEL_DIR)/Vtop.cpp

################################################################################
# Verilator flags
################################################################################
VFLAGS += \
	-Wno-fatal \
	--prefix Vtop \
	--top-module $(TOPLEVEL) \
	--Mdir $(MODEL_DIR) \
	--cc \
	--compiler clang

# Other setting for cocotb support
ifeq ($(TB_TYPE), cocotb)
VFLAGS   += -DCOCOTB_SIM=1 --vpi --public-flat-rw
LDFLAGS  += -Wl,-rpath,$(shell cocotb-config --prefix)/cocotb/libs
LDFLAGS  += -L$(shell cocotb-config --prefix)/cocotb/libs
LDLIBS   += -lcocotbvpi_verilator -lgpi -lcocotb -lgpilog -lcocotbutils
CPPFLAGS += "-DVL_TIME_PRECISION_STR=1ps"
export COCOTB_REDUCED_LOG_FMT := 1
export COCOTB_LOG_LEVEL       := INFO
export LD_LIBRARY_PATH        := $(shell cocotb-config --prefix)/cocotb/libs
endif

ifdef HDL_INC_DIRS
VFLAGS += $(addprefix -I, $(HDL_INC_DIRS))
endif

ifndef DISABLE_VCD_TRACING
VFLAGS += --trace
endif

################################################################################
# Compilation rules
################################################################################
$(BIN_DIR)/$(TOPLEVEL): $(MODEL_SRC) $(TB_SRCS) $(SHARED_TB_SRCS)
	@mkdir -p $(BUILD_DIR); \
	mkdir -p $(BIN_DIR); \
	$(MAKE) -f ../exe.mk debug-make; \
	$(MAKE) -f ../exe.mk

$(MODEL_SRC): $(HDL)
	$(VERILATOR_ROOT)/bin/verilator $(VFLAGS) $^

################################################################################
# Utility targets
################################################################################
.PHONY: clean sim debug-make

ifeq ($(TB_TYPE), cocotb)
sim: $(BIN_DIR)/$(TOPLEVEL)
	MODULE=$(TB_MODULE) $< < seeds/$(SEED)
else
sim: $(BIN_DIR)/$(TOPLEVEL)
	$< seeds/$(SEED)
endif

clean:
	@rm -rf $(BIN_DIR)
	@rm -rf $(BUILD_DIR)
	@rm -rf $(MODEL_DIR)
	@rm -f *.vcd
	@rm -f *.xml
	@rm -f *.dat
	@rm -rf tb/cocotb/*/__pycache__

################################################################################
# Debugging
################################################################################
debug-make::
	@echo
	@echo TOPLEVEL: $(TOPLEVEL)
	@echo TOPLEVEL_LANG: $(TOPLEVEL_LANG)
	@echo HDL: $(HDL)
	@echo TB_DIR: $(TB_DIR)
	@echo TB_SRCS_DIR: $(TB_SRCS_DIR)
	@echo TB_INCS_DIR: $(TB_INCS_DIR)
	@echo SHARED_TB_SRCS_DIR: $(SHARED_TB_SRCS_DIR)
	@echo SHARED_TB_INCS_DIR: $(SHARED_TB_INCS_DIR)
	@echo TB_SRCS: $(TB_SRCS)
	@echo CPPFLAGS: $(CPPFLAGS)
	@echo LDFLAGS: $(LDFLAGS)
	@echo LDLIBS: $(LDLIBS)
	@echo LD_LIBRARY_PATH: $(LD_LIBRARY_PATH)
	@echo