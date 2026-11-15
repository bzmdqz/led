cmake_minimum_required(VERSION 3.21)
project(hub75e_logo LANGUAGES C ASM)

add_executable(hub75e_logo
    src/startup_stm32f103.S
    src/main.c
    src/hub75.c
    src/hub75_layout.c
    src/logo.c
)
set_target_properties(hub75e_logo PROPERTIES
    SUFFIX ".elf"
    C_STANDARD 11
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS NO
)
target_include_directories(hub75e_logo PRIVATE include)
target_compile_options(hub75e_logo PRIVATE
    -mcpu=cortex-m3 -mthumb
    -g -gdwarf-4
    "$<$<COMPILE_LANGUAGE:C>:-O2;-ffreestanding;-ffunction-sections;-fdata-sections;-Wall;-Wextra;-Wpedantic;-Werror>"
)

set(linker_script "${CMAKE_CURRENT_SOURCE_DIR}/linker/stm32f103c8.ld")
set_property(TARGET hub75e_logo APPEND PROPERTY LINK_DEPENDS "${linker_script}")
target_link_options(hub75e_logo PRIVATE
    -mcpu=cortex-m3 -mthumb -nostdlib
    "-T${linker_script}"
    "-Wl,--gc-sections,-z,noexecstack,-Map=${CMAKE_CURRENT_BINARY_DIR}/hub75e_logo.map"
)
add_custom_command(TARGET hub75e_logo POST_BUILD
    COMMAND "${ARM_OBJCOPY}" -O ihex "$<TARGET_FILE:hub75e_logo>" hub75e_logo.hex
    COMMAND "${ARM_OBJCOPY}" -O binary "$<TARGET_FILE:hub75e_logo>" hub75e_logo.bin
    COMMAND "${ARM_SIZE}" "$<TARGET_FILE:hub75e_logo>"
    BYPRODUCTS hub75e_logo.hex hub75e_logo.bin hub75e_logo.map
    VERBATIM
)
