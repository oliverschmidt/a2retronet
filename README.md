# A2retroNET

This project is based on [A2Pico](https://github.com/oliverschmidt/a2pico).

This firmware is currently still in a prototype stage. It implements a ProDOS 8 compatible mass storage controller with two block devices. These block devices are represented by two files “drive1.po” and “drive2.po” in the root directory of the FAT formatted SD card.

When A2Pico is connected to a PC via USB, it acts as a USB drive. This allows the `drive1.po`and `drive2.po` files to be modified without opening the computer and removing the SD card. That functionality is independent of the Apple II's power-on status. However make sure to not remove A2Pico from the Apple II slot while it is connected to a PC.

Note that the USB drive performs much slower than expected because there is only a USB 1.1 Full Speed connection (12 Mbps) instead of the usual USB 2.0 Hi-Speed connection (480 Mbps).
