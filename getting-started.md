## Getting Started

### All Firmwares

The firmware you selected at purchase is pre-flashed on your A2Pico. However, we recommend flashing A2Pico again if you're not 100% sure which firmware (variant) you chose, or simply want to make sure you're using the latest firmware.

The list of A2Pico projects can always be found at: https://github.com/oliverschmidt/a2pico

A2Pico firmwares come in three types:

* `<project>_A2Pico_<date>.uf2` is intended exclusively for the _A2Pico_.
* `<project>_A2Pico2LiteW_<date>.uf2` is intended exclusively for the _A2Pico2Lite W_.
* `<project>_A2Pico2-generic_<date>.uf2` is intended exclusively for all other A2Pico2 (incl. the _A2Pico2Lite_).

__Warning: Trying to use an A2Pico with the wrong firmware type can result in physical damage to the card and/or the Apple II !__

The actual firmware flashing process is a breeze. All you need is a standard USB cable:
1. Remove the A2Pico from your Apple II.
2. Press and hold the `BOOTSEL` button on the A2Pico.
3. Connect the A2Pico to a PC.
4. Release the `BOOTSEL` button. A new drive, `RPI-RP2` or `RP2350`, will appear.
5. Copy the firmware `.uf2` file to the `RPI-RP2` or `RP2350` drive, e.g., by drag & drop.
6. Disconnect the A2Pico from the PC. Done.

### A2retroNET Firmware

The latest A2retroNET firmware files can always be found at: https://github.com/oliverschmidt/a2retronet/releases/latest

The A2retroNET firmware comes in two variants:

* `A2retroNET-Link_<type>_<date>.uf2` allows access to the Micro SD Card from a connected PC and also emulates a Super Serial Card.
* `A2retroNET-Drive_<type>_<date>.uf2` allows the use of a USB Thumb Drive (in addition to the Micro SD Card if present).

The Micro SD Card or USB Thumb Drive must be formatted with FAT, FAT32, or exFAT.

Now, with the flashed A2Pico and the formatted Micro SD Card or USB Thumb Drive at hand, perform the following steps:

1. Download the Total Replay .hdv file from https://archive.org/details/TotalReplay and save it as the only file in the root directory of the Micro SD Card or USB Thumb Drive.

2. Insert the Micro SD Card into the A2Pico or connect the USB Thumb Drive, and plug the A2Pico into an Apple II slot of your choice. We recommend slot 7.

3. Turn on the Apple II. You'll see a countdown starting with `3` in the lower right corner.

4. Press the `C` key during the countdown.

5. In the menu that now appears, drive 1 should be selected at the top and the Total Replay .hdv file should be selected at the bottom. Simply press `RETURN` to insert the .hdv file into drive 1.

6. Press `ESC` to exit the menu and begin the actual boot process.

7. The Total Replay splash screen should now appear.

The A2retroNET README contains all the information you need to get the most out of it: https://github.com/oliverschmidt/a2retronet
