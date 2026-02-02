# A2retroNET

This project is based on [A2Pico](https://github.com/oliverschmidt/a2pico).

A2retroNET implements a SmartPort mass storage controller with (up to) eight drives. The (currently) only supported disk image format is a ProDOS block image of up to 32MB, typically with the file extensions `.hdv`, `.po` or `.2mg`. There are two firmware variants:

## A2retroNET.uf2

This firmware not only provides a SmartPort controller but at the same time also [Super Serial Card (SSC)](https://en.wikipedia.org/wiki/Apple_II_serial_cards#Super_Serial_Card_(Apple_Computer)) functionality. Common so-called multicards create virtual cards in other Apple II slots. Those slots are then blocked for any other use. In contrast, A2retroNET implements both SmartPort and SSC functionality a __single__ Apple II slot - no other slot is blocked!

A2retroNET implements this unique feature by allowing switching between a SmartPort mode and a SSC mode. However, the trick is that most functionality is available in both modes:

|                      | SmartPort mode           | SSC mode                 |
|----------------------|:------------------------:|:------------------------:|
| SmartPort controller | :heavy_check_mark:       | :heavy_check_mark:       |
| Pascal 1.1 char I/O  | :heavy_check_mark:       | :heavy_check_mark:       |
| Direct ACIA char I/O | :heavy_check_mark:       | :heavy_check_mark:       |
| BASIC char I/O       | :heavy_multiplication_x: | :heavy_check_mark:       |
| Reboot               | :heavy_check_mark:       | :heavy_multiplication_x: |

Because the SmartPort controller is available in both modes, ProDOS continues to function as expected when switching to SSC mode. There are several ways to manage switching between the two modes.

### Manual Mode Switch

If BASIC character I/O is only used very rarely, it is useful to manually switch into SSC mode by executing `SSC.SHOW.SYSTEM` before entering `PR#<n>` and to manually switch back into SmartPort mode by executing `SSC.HIDE.SYSTEM` after entering `PR#0`.

### Automatic Mode Switch

If BASIC character I/O is used more frequently, it is useful to automatically switch into SSC mode right after booting by adding `ATINIT` to ProDOS boot volumes. Switching back into SmartPort mode is achieved by holding the __Reset__ key slightly longer than usual during a Ctrl-Reset. On the //e and IIgs, this allows a cold boot directly from SSC mode by holding the __Reset__ key slightly longer than usual during a Ctrl-OpenApple-Reset.

### Character I/O

The SSC functionality differs from a real SSC in several ways. The main differences to a SSC are:
* There is a USB interface instead of an RS-232 interface.
* There is no UART (not even a virtual one). Therefore, the usual connection settings like `9600 Baud` are meaningless. The only exception is the number of data bits.
* The actual connection speed is implicitly always _the highest that both communicating parties can achieve without data loss_. This is usually significantly faster than anything possible with a SSC (incl. its `115.200 Baud` mode).
* Hardware handshake lines are not supported, as they usually don't make any sense without an UART.
* Interrupts are not supported, as they usually don't make any sense without an UART.

When A2Pico is connected to a PC via USB (just as when flashing an A2Pico firmware), a new virtual serial port (called `COMx` in Windows) is opened on the PC. The easiest way to use the SSC functionality in Windows is to open a command prompt and enter `type COMx` (where `COMx` is A2Pico's virtual serial port). Any output generated on the Apple II (i.e. via `PR#<n>`) will be displayed in the command prompt. It can be redirected into a file or piped into another program. When done, type `Ctrl-C` to quit.

The SSC functionality is set to `Printer Mode` (as opposed to `Communication Mode`). The main reason for this is that serious communication needs require a dedicated Apple II program. These programs ignore the SSC settings anyway.

### SmartPort Controller

This firmware uses a Micro SD Card as its storage medium. When A2Pico is connected to a PC via USB (just as when flashing an A2Pico firmware), it acts as an SD Card reader. This allows access to the SD Card contents without having to open the Apple II and remove the SD Card. That functionality is independent of the Apple II's power-on state. The USB cable can be connected or disconnected at any time. However, be careful to not remove A2Pico from the Apple II slot while it is connected to a PC. Note that the SD Card reader will operate significantly slower than expected, as only a USB 1.1 Full Speed connection (12 Mbps) is available instead of the usual USB 2.0 Hi-Speed connection (480 Mbps).

Any changes to the SD Card contents (e.g., replacing a disk image file) are detected by the Apple II in real time. This also applies to changes to the `A2retroNET.txt` configuration file in the SD Card root directory. Simply keep `A2retroNET.txt` open in Windows Notepad. Your changes will be applied immediately each time you save.

Note: Instead of connecting to a PC, A2Pico can also be connected to a smartphone. The SD card contents will then be displayed in the smartphone's standard file browser. If you don't already have one, you'll only need a text file editor app to edit `A2retroNET.txt`. [EZText](https://apps.apple.com/de/app/eztext-text-editor/id1616281411) (for iOS) and [Simple Text Editor](https://play.google.com/store/apps/details?id=com.maxistar.textpad&hl=en) (for Android) are such (free, ad-free) apps. You can find the right adapter or cable to connect A2Pico to a smartphone (with USB-C port) by searching for "USB C OTG Micro USB".

Please ensure the A2Pico `USB Pwr` is set to `off` when using this firmware! 

## A2retroNET-USB.uf2

This firmware uses both a USB Thumb Drive and a Micro SD Card as storage media. Note that the Apple II reads from the SD Card approximately three times faster than from the Thumb Drive. However, unlike the SD Card, the Thumb Drive is fully hot-pluggable. This functionality is best utilized with an extension like the [External USB Port for A2Pico](https://jcm-1.com/product/external-usb-port-for-a2pico-usb-micro-to-usb-a/), which allows access to the Thumb Drive without having to open the Apple II. Any change in the Thumb Drive's state is detected by the Apple II in real time.

Of course, this firmware can be used without an SD Card. However, it is particularily adventageous to use the SD Card to represent (fast) fixed hard drives and the Thumb Drive to represent (flexible) floppy drives. Without a Thumb Drive in place, the `A2retroNET.txt` configuration file is read from the SD Card. However, as soon as a Thumb Drive is plugged in, `A2retroNET.txt` is read from the Thumb Drive. This way, as with real hard drives and floppy drives, you can usually work with the hard drives only, but still quickly insert a floppy to try out something. Additionally, it is possible that the `A2retroNET.txt` configuration file on a Thumb Drive reference disk images on the SD Card, so that both storage media can be accessed simultaneuously. And finally it possible that a Thumb Drive only contains a `A2retroNET.txt` configuration file which overrides the one on the SD Card. Imagine simply plugging in a Thumb Drive to temporarily use the Total Replay hard disk image on the SD Card as boot disk, thus turning the Apple II into a game console.

Note: Some Thumb Drives take several seconds to initialize. Therefore, a cold boot will use the SD Card. However, if no SD Card is present, the firmware waits until a Thumb Drive is plugged in and initialized.

You can find the right adapter or cable to connect a USB Thumb Drive to A2Pico by searching for "Micro USB OTG".

Please ensure the A2Pico `USB Pwr` is set to `on` when using this firmware! 

## Configuration Utility

The A2retroNET firmware contains a configuration utility. It can be invoked in two ways:

* By pressing the `C` key while A2retroNET delays the boot. During the boot delay, a countdown will be displayed in the lower right corner of the screen. Pressing any other key will skip the remaining boot delay. The boot delay is configurable.

* By calling `$C<n>F0`:
  
  | A2retroNET Slot | BASIC Command |
  |:---------------:|:-------------:|
  | 1               | `CALL 49648`  |
  | 2               | `CALL 49904`  |
  | 3               | `CALL 50160`  |
  | 4               | `CALL 50416`  |
  | 5               | `CALL 50672`  |
  | 6               | `CALL 50928`  |
  | 7               | `CALL 51184`  |

The configuration utility provides a convenient way to edit the `A2retroNET.txt` configuration file directly from the Apple II. However, you can still edit `A2retroNET.txt` in a different way at any time. The title on each screen indicates wether you are editing `A2retroNET.txt` on the USB Thumb Drive or the Micro SD Card. 

<img src="/assets/config-iie.jpg" width="400"> <img src="/assets/config-iip.jpg" width="400">

The `Drive Configuration` screen allows you to configure which disk image file is used for which drive.

| Key              | Command                                                                  |
|:----------------:|--------------------------------------------------------------------------|
| `Esc`            | Quit the configuration utility                                           |
| `Space` or `Tab` | Toggle between selecting a drive and selecting a disk image file         |
| `Left` or `Up`   | Select previous drive or disk image file (or directory)                  |
| `Right`or `Down` | Select next drive or disk image file (or directory)                      |
| `Return`         | "Insert" selected disk image file in selected drive (or enter directory) |
| `-`              | "Remove" disk image file from selected drive                             |
| `/`              | Go to root directory                                                     |
| `:`              | Switch between selecting from the USB Thumb Drive and the Micro SD Card  |
| `1` - `8`        | Directly select a drive                                                  |
| `0` or `A` - `Z` | Directly select a disk image file (or directory) with a matching name    |
| `Ctrl-S`         | Enter `Settings` screen                                                  |

The `Settings` screen allows you to configure the boot delay in seconds and the number of drives provided by A2retroNET for the Apple II operating system. Additionally, the A2retroNET version is displayed.

| Key              | Command                                                      |
|:----------------:|--------------------------------------------------------------|
| `Esc`            | Go back to `Drive Configuration` screen                      |
| `Space` or `Tab` | Toggle between selecting a boot delay and a number of drives |
| `Left` or `Up`   | Select a smaller boot delay or number of drives              |
| `Right`or `Down` | Select a larger boot delay or number of drives               |
| `0` - `9`        | Directly select a boot delay or number of drives             |

### A2retroNET.txt

To use a storage device with A2retroNET, the text file `A2retroNET.txt` must be present in the root directory. This file is formatted like a conventional INI file. There are two sections: `[settings]` and `[drives]`.

The `[settings]` section contains the following entry type:

* `bootdelay` allows you to set the time in seconds to wait for a key press when booting A2retroNET before the actual boot process begins. Valid values are `0` to `9`. The default value is `3`.

The `[drives]` section contains the following entry types:

* `number` allows you to set the number of drives provided by A2retroNET for the Apple II operating system. Valid values are `2`, `4`, `6` and `8`. The default value is `8`.

* `1` through `8` indicate the name of the disk image to be used for the drive with the specified number.

A simple example:
```
[settings]
bootdelay=5
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
* `usb:image.hdv`
* `usb:/image.hdv` (same as above)
* `usb:/path/to/image.hdv`

Notes:
* No spaces are allowed around the `=`.
* A drive without an assigment is like a real drive with no media inserted. The same applies to assigning to a nonexistent disk image.
* A disk image with the file attribute Read-Only is used as write protected medium.
* Any line starting with `#` is considered a comment and ignored. This allows for quick switching between multiple assigments to the same drive by commenting out all but one.

## Error Handling

Accessing the A2retroNET SmartPort controller can result in an error for various reasons, including:
* Missing media
* Corrupted media
* Missing `A2retroNET.txt`
* Malformed `A2retroNET.txt`
* Missing disk image
* Corrupted disk image
* Write protected disk image

There are three distinct scenarios for accessing the A2retroNET SmartPort controller:
* The Apple II Autostart ROM searches slots 7 through 1 for a bootable device. If an error occurs, the search continues with the next slot.
* A boot operation is requested via `PR#<n>`. If an error occurs, the message `HDD ERROR` is displayed before returning to the BASIC prompt.
* ProDOS calls the SmartPort interface. If an error occurs, it is reported to ProDOS.
