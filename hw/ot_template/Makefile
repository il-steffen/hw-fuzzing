# Copyright 2020 Timothy Trippel
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

ifndef DUT_HDL_DIR
$(error DUT_HDL_DIR is not set ... ABORTING.)
endif

################################################################################
# Circuit specific configs
################################################################################
export TOPLEVEL      := # OpenTitan IP Toplevel
export TOPLEVEL_LANG := verilog
export TB_TYPE       ?= cpp
export TB            ?= afl
export CPPFLAGS      += -std=c++11
export LDFLAGS       +=
export LDLIBS        :=
export SEED          ?= example.hwf

################################################################################
# Simulation Environment Vars
################################################################################
# N/A

################################################################################
# FuseSoC Settings (for generated OpenTitan IP)
################################################################################
FUSESOC_VERSION  := # OpenTitan IP Version
FUSESOC_LIBRARY  := lowrisc:ip
FUSESOC_CORE     := lowrisc_ip_$(TOPLEVEL)_$(FUSESOC_VERSION)
FUSESOC_GEN_PATH := build/$(FUSESOC_CORE)/default-verilator/generated/lowrisc

################################################################################
# HDL/TB
################################################################################
export HDL_INC_DIRS := ../tb/hdl
export HDL := \
	# OpenTitan IP configuration package
	$(DUT_HDL_DIR)/hw/top_earlgrey/rtl/top_pkg.sv \
	\
	# OpenTitan primitive packages/RTL
$(DUT_HDL_DIR)/hw/ip/prim/rtl/<OpenTitan IP Primitive>.sv \
	\
	# OpenTitan FuseSoC generated primitive packages/RTL
	$(FUSESOC_GEN_PATH)_prim_flop_2sync-impl_0/prim_flop_2sync.sv \
	\
	# Modified OpenTitan primitives with ONLY Verilator-supported SVAs
	../tb/hdl/prim_assert_hwfuzzing_macros.svh \
	../tb/hdl/prim_assert.sv \
	#../tb/hdl/prim_alert_sender.sv
	#../tb/hdl/prim_diff_decode.sv
  \
	# OpenTitan TL-UL Bus Packages/RTL
	$(DUT_HDL_DIR)/hw/ip/tlul/rtl/tlul_pkg.sv \
	$(DUT_HDL_DIR)/hw/ip/tlul/rtl/tlul_adapter_reg.sv \
	$(DUT_HDL_DIR)/hw/ip/tlul/rtl/tlul_err.sv \
	\
	# OpenTitan IP Packages
	$(DUT_HDL_DIR)/hw/ip/$(TOPLEVEL)/rtl/$(TOPLEVEL)_reg_pkg.sv \
	$(DUT_HDL_DIR)/hw/ip/$(TOPLEVEL)/rtl/$(TOPLEVEL)_reg_top.sv \
	\
	# OpenTitan IP RTL
	$(DUT_HDL_DIR)/hw/ip/$(TOPLEVEL)/rtl/$(TOPLEVEL).sv \
	\
	# SV testbench wrapper
	hdl/$(TOPLEVEL)_tb.sv
export SHARED_TB_SRCS := \
	verilator_tb.cpp \
	stdin_fuzz_tb.cpp \
	tlul_host_tb.cpp \
	ot_ip_fuzz_tb.cpp

ifndef DISABLE_VCD_TRACING
HDL += ../tb/hdl/tlul_inspect.sv
endif

################################################################################
# Verilator Flags (optional)
################################################################################
export VFLAGS := --assert

################################################################################
# Include common build targets
################################################################################
include ../common.mk
