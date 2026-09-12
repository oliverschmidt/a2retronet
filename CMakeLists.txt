set(PROJECT_NAME A2retroNET)

cmake_minimum_required(VERSION 3.13...3.27)
include(pico_sdk_import.cmake)
project(${PROJECT_NAME} C CXX ASM)
pico_sdk_init()

add_executable(${PROJECT_NAME})
pico_add_extra_outputs(${PROJECT_NAME})

option(HOST "USB Host Mode")
message("HOST is ${HOST}")
target_compile_definitions(${PROJECT_NAME} PRIVATE HOST=$<BOOL:${HOST}>)

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

include(FetchContent)

FetchContent_Declare(a2pico
        GIT_REPOSITORY https://github.com/oliverschmidt/a2pico.git
        GIT_TAG main
        )
FetchContent_MakeAvailable(a2pico)

FetchContent_Declare(fatfs
        GIT_REPOSITORY https://github.com/FarLeftLane/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico.git
        GIT_TAG main
        SOURCE_SUBDIR src
        )
FetchContent_MakeAvailable(fatfs)

target_compile_definitions(no-OS-FatFS-SD-SDIO-SPI-RPi-Pico INTERFACE
        FF_USE_STRFUNC=2
        FF_VOLUMES=2
        FF_STR_VOLUME_ID=1
        FF_VOLUME_STRS="SD","USB"
        GLUE_DISK_STATUS=sd_disk_status
        GLUE_DISK_INITIALIZE=sd_disk_initialize
        GLUE_DISK_READ=sd_disk_read
        GLUE_DISK_WRITE=sd_disk_write
        GLUE_DISK_IOCTL=sd_disk_ioctl
        )

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
        board.c
        config.c
        diskio.c
        gen.c
        hdd.c
        hw_config.c
        incbin.S
        main.c
        sp.c
        )
if(HOST)
        target_sources(${PROJECT_NAME} PRIVATE
                msc_host.c
                usb_diskio.c
                )
else()
        target_sources(${PROJECT_NAME} PRIVATE
                msc_device.c
                ser.c
                slip.c
                usb_descriptors.c
                )
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
        a2pico
        no-OS-FatFS-SD-SDIO-SPI-RPi-Pico
        pico_multicore
        pico_stdlib
        pico_unique_id
        )
if(HOST)
        target_link_libraries(${PROJECT_NAME} PRIVATE
                tinyusb_host
                )
else()
        target_link_libraries(${PROJECT_NAME} PRIVATE
                tinyusb_device
                )
endif()

if(NOT HOST)

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
        VERBATIM
        )

endif()  # NOT HOST
