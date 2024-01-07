set(PROJECT_NAME A2retroNET)

cmake_minimum_required(VERSION 3.12)
include(pico_sdk_import.cmake)
project(${PROJECT_NAME} C CXX ASM)
pico_sdk_init()

add_executable(${PROJECT_NAME})
pico_add_extra_outputs(${PROJECT_NAME})

include(FetchContent)
FetchContent_Declare(a2pico
        GIT_REPOSITORY https://github.com/oliverschmidt/a2pico.git
        GIT_TAG main
        )
FetchContent_MakeAvailable(a2pico)
FetchContent_Declare(FatFs_SPI
        GIT_REPOSITORY https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico.git
        SOURCE_SUBDIR FatFs_SPI
        )
FetchContent_MakeAvailable(FatFs_SPI)

set_source_files_properties(incbin.S OBJECT_DEPENDS firmware.rom)
add_custom_command(
        COMMAND cl65 ${CMAKE_CURRENT_SOURCE_DIR}/firmware.S
                  -o ${CMAKE_CURRENT_BINARY_DIR}/firmware.rom
                  -C apple2-asm.cfg
        MAIN_DEPENDENCY firmware.S
        OUTPUT firmware.rom
        VERBATIM
        )

target_sources(${PROJECT_NAME} PRIVATE
        main.c
        board.c
        hdd.c
        sp.c
        hw_config.c
        incbin.S
        )

target_link_libraries(${PROJECT_NAME} PRIVATE
        pico_stdlib
        pico_multicore
        a2pico
        FatFs_SPI
        )
