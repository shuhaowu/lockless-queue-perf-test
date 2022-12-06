Include(FetchContent)

set(FETCHCONTENT_QUIET FALSE)

FetchContent_Declare(
  iceoryx
  GIT_REPOSITORY https://github.com/eclipse-iceoryx/iceoryx.git
  GIT_TAG        v2.0.2
  SOURCE_SUBDIR  iceoryx_meta
)

FetchContent_MakeAvailable(iceoryx)

# This is needed to make sure that when building the tests, catch2's headers
# are treated as system headers, as otherwise clang-tidy will run on them. This
# is different from the above because clang-tidy will run on the headers which
# is included from the tests, where as the above is for compiling catch2.
#
# After cmake 3.25, this shouldn't be needed anymore: https://gitlab.kitware.com/cmake/cmake/-/issues/18040
get_target_property(ICEORYX_POSH_INC iceoryx_posh INTERFACE_INCLUDE_DIRECTORIES)
set_target_properties(iceoryx_posh PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${ICEORYX_POSH_INC}")
get_target_property(ICEORYX_HOOFS_INC iceoryx_hoofs INTERFACE_INCLUDE_DIRECTORIES)
set_target_properties(iceoryx_hoofs PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${ICEORYX_HOOFS_INC}")

add_executable(iceoryx_mpnc
  main.cc
)

target_link_libraries(iceoryx_mpnc
  PRIVATE
  iceoryx_hoofs
  iceoryx_posh
  iceoryx_posh::iceoryx_posh_roudi
)
