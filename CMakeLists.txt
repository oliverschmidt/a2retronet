set(PROJECT_NAME A2retroNET)

cmake_minimum_required(VERSION 3.12)
include(pico_sdk_import.cmake)
project(${PROJECT_NAME} C CXX ASM)
pico_sdk_init()

add_executable(${PROJECT_NAME})
pico_add_extra_outputs(${PROJECT_NAME})

set(MEDIUM "SD" CACHE STRING "SD or USB")
if (MEDIUM STREQUAL "SD")
        target_compile_definitions(${PROJECT_NAME} PRIVATE MEDIUM SD)
elseif (MEDIUM STREQUAL "USB")
        target_compile_definitions(${PROJECT_NAME} PRIVATE MEDIUM USB)
endif ()

# Make sure TinyUSB can find tusb_config.h
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR})

# Use FatFs coming with TinyUSB
target_include_directories(${PROJECT_NAME} PRIVATE
        ${PICO_TINYUSB_PATH}/lib/fatfs/source)
target_sources(${PROJECT_NAME} PRIVATE
        ${PICO_TINYUSB_PATH}/lib/fatfs/source/ff.c
        ${PICO_TINYUSB_PATH}/lib/fatfs/source/ffsystem.c
        ${PICO_TINYUSB_PATH}/lib/fatfs/source/ffunicode.c
        )

# https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico requires
# glue.c to be modified in order to allow multiple FatFs devices
target_include_directories(${PROJECT_NAME} PRIVATE
        sd_spi/include
        sd_spi/sd_driver)
target_sources(${PROJECT_NAME} PRIVATE
        sd_spi/sd_driver/crc.c
        sd_spi/sd_driver/demo_logging.c
        sd_spi/sd_driver/hw_config.c
        sd_spi/sd_driver/sd_card.c
        sd_spi/sd_driver/sd_spi.c
        sd_spi/sd_driver/spi.c
        sd_spi/src/f_util.c
        sd_spi/src/ff_stdio.c
        sd_spi/src/glue.c
        sd_spi/src/my_debug.c
        sd_spi/src/rtc.c
        )
target_link_libraries(${PROJECT_NAME} PRIVATE
        hardware_dma
        hardware_rtc
        hardware_spi
        )

include(FetchContent)
FetchContent_Declare(a2pico
        GIT_REPOSITORY https://github.com/oliverschmidt/a2pico.git
        GIT_TAG main
        )
FetchContent_MakeAvailable(a2pico)

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
        diskio.c
        incbin.S
        )
if (MEDIUM STREQUAL "SD")
        target_sources(${PROJECT_NAME} PRIVATE
        msc_device.c
        usb_descriptors.c
        )
elseif (MEDIUM STREQUAL "USB")
        target_sources(${PROJECT_NAME} PRIVATE
        msc_host.c
        usb_diskio.c
        )
endif ()

target_link_libraries(${PROJECT_NAME} PRIVATE
        pico_stdlib
        pico_multicore
        pico_unique_id
        a2pico
        )
if (MEDIUM STREQUAL "SD")
        target_link_libraries(${PROJECT_NAME} PRIVATE
        tinyusb_device
        )
elseif (MEDIUM STREQUAL "USB")
        target_link_libraries(${PROJECT_NAME} PRIVATE
        tinyusb_host
        )
endif ()
