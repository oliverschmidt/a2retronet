set(PROJECT_NAME A2retroNET)

cmake_minimum_required(VERSION 3.13...3.27)
include(pico_sdk_import.cmake)
project(${PROJECT_NAME} C CXX ASM)
pico_sdk_init()

add_executable(${PROJECT_NAME})
pico_add_extra_outputs(${PROJECT_NAME})

set(MEDIUM "SD" CACHE STRING "SD or USB")
message("MEDIUM is ${MEDIUM}")
if (MEDIUM STREQUAL "SD")
        target_compile_definitions(${PROJECT_NAME} PRIVATE MEDIUM_SD)
elseif (MEDIUM STREQUAL "USB")
        target_compile_definitions(${PROJECT_NAME} PRIVATE MEDIUM_USB)
endif ()

execute_process(
        COMMAND git describe --tags
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_TAG
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )
message("GIT_TAG is ${GIT_TAG}")
target_compile_definitions(${PROJECT_NAME} PRIVATE GIT_TAG="${GIT_TAG}")

# Make sure TinyUSB can find tusb_config.h
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR})

# http://elm-chan.org/fsw/ff/ requires ffconf.h to be
# modified in order to set some configuration options 
target_include_directories(${PROJECT_NAME} PRIVATE
        fatfs/source
        )
target_sources(${PROJECT_NAME} PRIVATE
        fatfs/source/ff.c
        fatfs/source/ffsystem.c
        fatfs/source/ffunicode.c
        )

# https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico requires
# glue.c to be modified in order to allow multiple FatFs devices
target_include_directories(${PROJECT_NAME} PRIVATE
        sd_spi/include
        sd_spi/sd_driver
        )
target_sources(${PROJECT_NAME} PRIVATE
        sd_spi/sd_driver/crc.c
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
        COMMAND cl65 -t apple2 -C apple2-asm.cfg
                        ${CMAKE_CURRENT_SOURCE_DIR}/6502/SSC.S
                     -o ${CMAKE_CURRENT_BINARY_DIR}/firmware.rom
        MAIN_DEPENDENCY 6502/SSC.S
        OUTPUT firmware.rom
        DEPENDS 6502/SSC.CN00.S 6502/SSC.C800.S 6502/SSC.HILEV.S 6502/SSC.TERM.S
                6502/SSC.CORE.S 6502/SSC.UTIL.S 6502/SSC.CMD.S   6502/SSC.CF00.S
                6502/SMARTPORT.S
        VERBATIM
        )

target_sources(${PROJECT_NAME} PRIVATE
        main.c
        board.c
        config.c
        hdd.c
        sp.c
        diskio.c
        incbin.S
        )
if (MEDIUM STREQUAL "SD")
        target_sources(${PROJECT_NAME} PRIVATE
        ser.c
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

if (MEDIUM STREQUAL "SD")

add_custom_command(
        COMMAND cl65 -t apple2 apple2.lib -C apple2-asm.cfg -u __EXEHDR__
                     -Wl -D,__FILETYPE__=$E2 --start-addr $2000 -Wa -D,init
                        ${CMAKE_CURRENT_SOURCE_DIR}/6502/ssc_show_hide.S
                     -o ${CMAKE_CURRENT_BINARY_DIR}/ATINIT.as
        MAIN_DEPENDENCY 6502/ssc_show_hide.S
        OUTPUT ATINIT.as
        VERBATIM
        )

add_custom_command(
        COMMAND cl65 -t apple2 apple2.lib -C apple2-asm.cfg -u __EXEHDR__
                     -Wl -D,__FILETYPE__=$FF --start-addr $2000 -Wa -D,show
                        ${CMAKE_CURRENT_SOURCE_DIR}/6502/ssc_show_hide.S
                     -o ${CMAKE_CURRENT_BINARY_DIR}/SSC.SHOW.SYSTEM.as
        MAIN_DEPENDENCY 6502/ssc_show_hide.S
        OUTPUT SSC.SHOW.SYSTEM.as
        VERBATIM
        )

add_custom_command(
        COMMAND cl65 -t apple2 apple2.lib -C apple2-asm.cfg -u __EXEHDR__
                     -Wl -D,__FILETYPE__=$FF --start-addr $2000 -Wa -D,hide
                        ${CMAKE_CURRENT_SOURCE_DIR}/6502/ssc_show_hide.S
                     -o ${CMAKE_CURRENT_BINARY_DIR}/SSC.HIDE.SYSTEM.as
        MAIN_DEPENDENCY 6502/ssc_show_hide.S
        OUTPUT SSC.HIDE.SYSTEM.as
        VERBATIM
        )

add_custom_target(ssc_show_hide ALL DEPENDS
        ATINIT.as
        SSC.SHOW.SYSTEM.as
        SSC.HIDE.SYSTEM.as
        )

add_custom_target(disk
        COMMAND java -jar ac.jar -as ${CMAKE_CURRENT_BINARY_DIR}/ProDOS_2_4_3.po
                                 ATINIT
                                 < ${CMAKE_CURRENT_BINARY_DIR}/ATINIT.as
        COMMAND java -jar ac.jar -as ${CMAKE_CURRENT_BINARY_DIR}/ProDOS_2_4_3.po
                                 SSC.SHOW.SYSTEM
                                 < ${CMAKE_CURRENT_BINARY_DIR}/SSC.SHOW.SYSTEM.as
        COMMAND java -jar ac.jar -as ${CMAKE_CURRENT_BINARY_DIR}/ProDOS_2_4_3.po
                                 SSC.HIDE.SYSTEM
                                 < ${CMAKE_CURRENT_BINARY_DIR}/SSC.HIDE.SYSTEM.as
        DEPENDS ssc_show_hide
        VERBATIM)

endif ()
