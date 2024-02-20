# A2retroNET

This project is based on [A2Pico](https://github.com/oliverschmidt/a2pico).

At the moment this project is in an early prototype stage. It implements a ProDOS 8 compatible mass storage controller with two block devices. Those block devices are represented by the two files `drive1.po`and `drive2.po` in the root directory of the FAT-formatted SD card. When A2Pico is connected to a PC via USB, then SD card acts as a USB drive. This USB drive works much slower then likely expected as there is only a USB 1.1 Full-Speed (12 Mbps) connection instead of the usual USB 2.0 Hi-Speed (480 Mbps) connection.