// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "double_counter_test.h"

#include <bitset>
#include <iostream>

// Constructor
DoubleCounterTest::DoubleCounterTest(int argc, char** argv)
    : num_checks_(0),
      main_time_(0),
      dut_(),
      test_(INPUT_PORT_SIZE_BYTES, argc, argv),
      count_1_(0),
      count_2_(0)
#if VM_TRACE
      ,
      tracing_file_pointer_(NULL),
      vcd_file_name_("")
#endif
{
#if VM_TRACE
    // Initialize VCD tracing
    InitializeTracing(argv[1]);
#endif

    // Initialize DUT model
    InitializeDUT();
}

// Destructor
DoubleCounterTest::~DoubleCounterTest() {
#if VM_TRACE
    // Close VCD trace if opened
    if (tracing_file_pointer_) {
        tracing_file_pointer_->close();
        delete tracing_file_pointer_;
        tracing_file_pointer_ = NULL;
    }
#endif
}

#if VM_TRACE
// Enable Verilator VCD tracing
void DoubleCounterTest::InitializeTracing(std::string fname) {
    // If verilator was invoked with --trace argument enable VCD tracing
    std::cout << "Tracing enabled." << std::endl;

    // Set VCD file name
    if (fname.find_last_of("\\/") != std::string::npos) {
        // input file name was a full path --> strip off base file name
        uint32_t base_file_name_start = fname.find_last_of("\\/") + 1;
        vcd_file_name_ = fname.substr(base_file_name_start) + ".vcd";
    } else {
        vcd_file_name_ = fname + ".vcd";
    }
    std::cout << "VCD file: " << vcd_file_name_ << std::endl;

    // Turn on Verilator tracing
    Verilated::traceEverOn(true);  // Verilator must compute traced signals
    tracing_file_pointer_ = new VerilatedVcdC();
    dut_.trace(tracing_file_pointer_, 99);  // Trace 99 levels of hierarchy
    tracing_file_pointer_->open(vcd_file_name_.c_str());  // Open the dump file
}
#endif

// Initialize DUT inputs
void DoubleCounterTest::InitializeDUT() {
    dut_.clk = 0;
    dut_.n_reset = 0;
    dut_.select = 0;
    dut_.eval();
#if VM_TRACE
    // Dump VCD trace for current time
    if (tracing_file_pointer_) {
        tracing_file_pointer_->dump(main_time_);
    }
#endif
    main_time_++;
}

// Toggle clock for num_toggles half clock periods.
// Model is evaluated AFTER clock state is toggled,
// and regardless of current clock state.
void DoubleCounterTest::ToggleClock(uint32_t num_toggles) {
    for (uint32_t i = 0; i < num_toggles; i++) {
        // Toggle main clock
        if (dut_.clk) {
            dut_.clk = 0;
        } else {
            dut_.clk = 1;
        }

        // Evaluate model
        dut_.eval();

#if VM_TRACE
        // Dump VCD trace for current time
        if (tracing_file_pointer_) {
            tracing_file_pointer_->dump(main_time_);
        }
#endif

        // Increment Time
        main_time_++;
    }
}

// Reset the DUT
void DoubleCounterTest::ResetDUT() {
    // Print reset status
    std::cout << "Resetting the DUT (time: " << unsigned(main_time_);
    std::cout << ") ..." << std::endl;

    // Place DUT in reset
    dut_.n_reset = 0;

    // Toggle clock for NUM_RESET_PERIODS
    ToggleClock((NUM_RESET_PERIODS * 2) + 1);

    // Pull DUT out of reset
    dut_.n_reset = 1;

    // Print reset status
    std::cout << "Reset complete! (time = " << unsigned(main_time_);
    std::cout << ")" << std::endl;
}

// Simulate the DUT with testbench input file
void DoubleCounterTest::SimulateDUT() {
    // Create buffer for test data
    uint8_t test_input[INPUT_PORT_SIZE_BYTES] = {0};

    // Read tests and simulate DUT
    while (test_.ReadTest(test_input) && !Verilated::gotFinish()) {
        // Load test into DUT
        dut_.select = (test_input[0] & 0x1);

        // Print test read from file
        std::cout << "Loading inputs for test " << test_.get_test_num();
        std::cout << " (time = " << unsigned(main_time_) << ") ...";
        std::cout << std::endl;
        std::cout << "  select = " << std::bitset<8>(test_input[0]);
        std::cout << " (0x" << std::hex << unsigned(test_input[0]) << ")";
        std::cout << std::endl;
        std::cout << "  dut.select = " << std::bitset<8>(dut_.select);
        std::cout << " (0x" << std::hex << unsigned(dut_.select) << ")";
        std::cout << std::endl;

        // Update correct "ground truth" state
        if (test_input[0] & 0x1) {
            count_1_++;
        } else {
            count_2_++;
        }

        // Toggle clock period
        ToggleClock(2);

        // Print vital DUT state
        std::cout << "Checking results for test " << num_checks_;
        std::cout << " (time = " << unsigned(main_time_) << ") ...";
        std::cout << std::endl;
        std::cout << "  count_1 (DUT / Correct) = ";
        std::cout << unsigned(dut_.count_1) << "/" << unsigned(count_1_);
        std::cout << std::endl;
        std::cout << "  count_2 (DUT / Correct) = ";
        std::cout << unsigned(dut_.count_2) << "/" << unsigned(count_2_);
        std::cout << std::endl;

        // Verify vital DUT state
        assert(count_1_ == dut_.count_1 &&
            "ERROR: Incorrect value for count_1.");
        assert(count_2_ == dut_.count_2 &&
            "ERROR: Incorrect value for count_2.");
        num_checks_++;
    }

#if VM_TRACE
    // Toggle clock period
    ToggleClock(1);
#endif

    // Final model cleanup
    dut_.final();
}

// Testbench entry point
int main(int argc, char** argv, char** env) {
    // Check command line args
    if (argc == 2) {
        std::cout << "Input file: " << argv[1] << std::endl;
    } else {
        std::cerr << "Usage: " << argv[0];
        std::cerr << " <input file name>" << std::endl;
        exit(1);
    }

    //std::ifstream f( argv[1], std::ios::binary );
    //std::cout << std::setfill( '0' ) << std::hex << std::uppercase;
    //while (f)
    //{
        //// Define buffer
        //char s[ 16 ];
        //std::size_t n, i;

        //// Read bytes
        //f.read( s, sizeof(s) );
        //n = f.gcount();

        //// Print hex bytes
        //for (i = 0; i < n; i++)
            //std::cout << std::setw( 2 ) << (int)s[ i ] << " ";

        //// Write spaces between hex bytes
        //while (i++ < sizeof(s))
            //std::cout << "   ";
        //std::cout << "  ";

        //// Write new line
        //std::cout << "\n";
    //}
    //exit(0);
//}

    // Instantiate testbench
    DoubleCounterTest tb(argc, argv);

    // Reset the DUT
    tb.ResetDUT();

    // Simulate the DUT
    tb.SimulateDUT();
}