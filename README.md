# A2retroNET

This project is based on [A2Pico](https://github.com/oliverschmidt/a2pico).

Despite its name, A2retroNET implements a SmartPort mass storage controller with (up to) eight drives. The (currently) only supported disk image format is a headerless ProDOS block image of up to 32MB, typically with the file extensions `.hdv` or `.po`. There are two firmware variants:

### A2retroNET.uf2

This firmware uses a Micro SD Card as its storage medium. When A2Pico is connected to a PC via USB (just as when flashing an A2Pico firmware), it acts as an SD Card reader. This allows access to the SD Card contents without having to open the Apple II and remove the SD Card. That functionality is independent of the Apple II's power-on state. The USB cable can be connected or disconnected at any time. However, be careful to not remove A2Pico from the Apple II slot while it is connected to a PC. Note that the SD Card reader will operate significantly slower than expected, as only a USB 1.1 Full Speed connection (12 Mbps) is available instead of the usual USB 2.0 Hi-Speed connection (480 Mbps).

Any changes to the SD Card contents (e.g., replacing a disk image file) are detected by the Apple II in real time. This also applies to changes to the `A2retroNET.txt` configuration file in the SD Card root directory. Simply keep `A2retroNET.txt` open in Windows Notepad. Your changes will be applied immediately each time you save.

Please ensure the A2Pico `USB Pwr` is set to `off` when using this firmware! 

### A2retroNET-USB.uf2

This firmware uses both a USB Thumb Drive and a Micro SD Card as storage media. Note that the Apple II accesses the SD Card approximately 50% faster than the Thumb Drive. However, unike the SD Card, the Thumb Drive is fully hot-pluggable. This functionality is best utilized with some extension cable, which allows access to the Thumb Drive without having to open the Apple II. Any change in the Thumb Drive's state is detected by the Apple II in real time.

Of course, this firmware can be used without an SD Card. However, it is particularily adventageous to use the SD Card to represent (fast) fixed hard drives and the Thumb Drive to represent (flexible) floppy drives. Without a Thumb Drive in place, the `A2retroNET.txt` configuration file is read from the SD Card. However, as soon as a Thumb Drive is plugged in, `A2retroNET.txt` is read from the Thumb Drive. This way, as with real hard drives and floppy drives, you can usually work with the hard drives only, but still quickly insert a floppy to try out something. Additionally, it is possible that the `A2retroNET.txt` configuration file on a Thumb Drive reference disk images on the SD Card, so that both storage media can be accessed simultaneuously. And finally it possible that a Thumb Drive only contains a `A2retroNET.txt` configuration file which overrides the one on the SD Card. Imagine simply plugging in a Thumb Drive to temporarily use the Total Replay hard disk image on the SD Card as boot disk, thus turning the Apple II into a game console.

Note: Some Thumb Drives take several seconds to initialize. Therefore, a cold boot will use the SD Card. However, if no SD Card is present, the firmware waits until a Thumb Drive is plugged in and initialized.

You can find the right adapter or cable to connect a USB Thumb Drive to the A2Pico by searching for "Micro USB OTG".

Please ensure the A2Pico `USB Pwr` is set to `on` when using this firmware! 

## A2retroNET.txt

To use a storage device with A2retroNET, the text file `A2retroNET.txt` must be created in the root directory. This file is formatted like a conventional INI file.

The (currently) only section is `[drives]`. This section contains the following entry types:

* `number` allows you to optionally limit the number of drives provided by A2retroNET for the Apple II operating system. Valid values are `2`, `4`, `6` and `8`. The default value is `8`.

* `1` through `8` indicate the name of the disk image to be used for the drive with the specified number.

A simple example:
```
[drives]
number=4
1=system.hdv
2=work.hdv
3=utils.po
4=games.po
```

Valid formats for disk image names:
* `image.hdv`
* `/image.hdv` (same as above)
* `/path/to/image.hdv`
* `sd:image.hdv`
* `sd:/image.hdv` (same as above)
* `sd:/path/to/image.hdv`

Notes:
* No spaces are allowed around the `=`.
* A drive without an assigment is like a real drive with no media inserted. The same applies to assigning to a nonexistent disk image.
* A disk image with the file attribute Read-Only is used as write protected medium.
* Any line starting with `#` is considered a comment and ignored. This allows for quick switching between multiple assigments to the same drive by commenting out all but one.
