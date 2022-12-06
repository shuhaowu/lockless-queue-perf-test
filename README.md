Performance testing for various lockless queue implementations
==============================================================

This repository tests the latency performance of various lockless queues. The
test setup is:

- Intel i7-3840QM (ThinkPad W530) with turbo boost on
- Ubuntu 22.04.1 LTS desktop
- Real-time kernel: [5.15.74-rt52+](https://github.com/cactusdynamics/linux-rt-for-ubuntu).
- Kernel cmdline set with `intel_idle.max_cstate=0 processor.max_cstate=0
  idle=poll` to ensure the processor is always at C0 state.

To run:

- `cmake -Bbuild && cmake --build build -j $(nproc)`
- `sudo chrt -f 80 build/boost_spsc/boost_spsc`
