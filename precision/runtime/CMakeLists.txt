# Bare-metal runtime: startup code, HTIF console/exit and a tiny libc.
add_library(caial_runtime STATIC crt.S htif.c libc.c)
# -fno-builtin keeps clang from turning the memcpy/memset loops into calls to themselves
target_compile_options(caial_runtime PRIVATE -O2 -ffreestanding -fno-builtin -Wall)
target_link_options(caial_runtime INTERFACE -nostdlib -static -T ${CMAKE_CURRENT_SOURCE_DIR}/link.ld)
target_link_libraries(caial_runtime INTERFACE gcc)
set_property(TARGET caial_runtime APPEND PROPERTY INTERFACE_LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/link.ld)
