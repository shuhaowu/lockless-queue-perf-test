cmake_minimum_required(VERSION 3.18)
project(lockless_queue_perf)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(iceoryx_mpnc)
add_subdirectory(boost_spsc)
