#!/usr/bin/python3
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
"""This is a cocotb testbench harness to interface with afl-fuzz.

Description:
The testbench starts by reseting the DUT for a duration of DUT_RESET_DURATION_NS
nanoseconds. After reset, the testbench reads bytes from STDIN and feeds them to
the input port(s) of the DUT. The testbench proceeds until there are no inputs
more inputs to provide the DUT.

Assertions:
This testbench contains a single assertion that raises an AssertionError to
crash the program when the final lock state is reached. AFL requires program
crashes as a feedback mechanism to know when it has reached its goal.

Environment Vars:
Since cocotb does not support passing arguments to the tests implemented in
Python, any arguments must be passed as environment variables.
"""

import math
import os
import sys

import cocotb
from cocotb.clock import Clock
from cocotb.triggers import FallingEdge, Timer

CLK_PERIOD_NS = 10  # duration of simulation clock period
DUT_RESET_DURATION_NS = 50  # duration to hold DUT in reset for in ns


async def reset_dut(reset_n, duration_ns):
  reset_n._log.debug("Resetting DUT ...")
  reset_n <= 0
  await Timer(duration_ns, units="ns")
  reset_n <= 1
  reset_n._log.debug("Reset complete!")


@cocotb.test()
async def lock_tb(dut):
  """Reads input codes from STDIN (generated by AFL) to try to unlock the lock.

  Args:
    dut: The object representing the DUT being simulated.

  Required Environment Vars:
    LOCK_COMP_WIDTH: The width (in # of bits) of the input code port.

  Returns:
    None

  Raises:
    AssertionError: The DUT (lock state machine) has been unlocked.
  """

  # Get test parameters
  input_size_bits = int(os.getenv("LOCK_COMP_WIDTH"))
  input_size_bytes = math.ceil(input_size_bits / 8)
  dut._log.info(f"Input Port Size: {input_size_bytes}")

  # Create and start the clock
  clock = Clock(dut.clk, CLK_PERIOD_NS, units="ns")
  cocotb.fork(clock.start())

  # Reset the DUT
  await reset_dut(dut.reset_n, DUT_RESET_DURATION_NS)

  # Send in random input values
  dut_input_bytes = sys.stdin.buffer.read(input_size_bytes)
  while dut_input_bytes:
    dut_input_int = int.from_bytes(dut_input_bytes,
                                   byteorder="big",
                                   signed=False)
    dut._log.info("Setting code to:")
    dut._log.info(f"  (bytes): {dut_input_bytes.hex()}")
    dut._log.info(f"  (int):   {dut_input_int}")
    dut.code <= dut_input_int
    await FallingEdge(dut.clk)
    dut._log.info(f" Code: {dut.code.value.binstr}")
    dut._log.info(f" State: {dut.state.value.integer}")

    # Check we unlocked the lock
    assert dut.unlocked.value.integer == 0, "REACHED FINAL STATE!"

    # read next input
    dut_input_bytes = sys.stdin.buffer.read(input_size_bytes)
