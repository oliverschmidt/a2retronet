/*

MIT License

Copyright (c) 2025 Oliver Schmidt (https://a2retro.de/)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <pico/stdlib.h>
#include <f_util.h>

#include "sp.h"
#include "hdd.h"
#include "main.h"

#include "config.h"

#define DEFAULT_BOOTDELAY 3

#define MAX_PATH    256
#define MAX_DIR     256

#define SECTION_NONE        0
#define SECTION_SETTINGS    1
#define SECTION_DRIVES      2

#define CONFIG_I_KEY    0

#define CONFIG_O_RETVAL 0
#define CONFIG_O_BUFFER 1

#define CONFIG_UPPERCASE    100
#define CONFIG_LOWERCASE    101

#define DELAY_MORE  0
#define DELAY_DONE  1

#define CONFIG_QUIT 0
#define CONFIG_CONT 1

#define COLS    40
#define ROWS    24

static uint8_t bootdelay;

static struct {
    char path[MAX_PATH];
} drives[MAX_DRIVES];

static uint8_t drives_number;

static bool lowercase;

static FILINFO directory[MAX_DIR];

static int directory_size;

void config_reset(void) {
    drives_number = 0; 
    memset(drives, 0, sizeof(drives));
}

static void get_config(void) {
    if (drives_number) {
        return;
    }
    bootdelay = DEFAULT_BOOTDELAY;
    drives_number = MAX_DRIVES;

    FIL text;
    FRESULT fr = f_open(&text, "A2retroNET.txt", FA_OPEN_EXISTING | FA_READ);
    if (fr != FR_OK) {
        printf("f_open(A2retroNET.txt, read) error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }

    int section = SECTION_NONE;
    while (true) {
        char line[MAX_PATH];
        char *success = f_gets(line, sizeof(line), &text);
        if (!success) {
            if (f_error(&text)) {
                printf("f_gets(A2retroNET.txt) error\n");
            }
            break;
        }
        int len = strlen(line);
        if (line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }
        if (line[0] == '#') {
            continue;
        }
        if (line[0] == '[') {
            section = SECTION_NONE; 
            if (strncasecmp(line, "[settings]", 10) == 0) {
                section = SECTION_SETTINGS;
            }
            if (strncasecmp(line, "[drives]", 8) == 0) {
                section = SECTION_DRIVES;
            }
        }
        switch (section) {
            case SECTION_SETTINGS:
                if (strncasecmp(line, "bootdelay=", 10) == 0) {
                    if (line[10] < '0' || line[10] > '9') {
                        continue;
                    }
                    bootdelay = line[10] - '0';
                }
                break;
            case SECTION_DRIVES:
                if (strncasecmp(line, "number=", 7) == 0) {
                    if (line[7] < '2' || line[7] > '0' + MAX_DRIVES || line[7] % 2) {
                        continue;
                    }
                    drives_number = line[7] - '0';
                }
                if (line[0] < '1' || line[0] > '0' + MAX_DRIVES || line[1] != '=') {
                    continue;
                }
                int drive = line[0] - '1';
                strcpy(drives[drive].path, &line[2]);
                break;
        }
    }
    fr = f_close(&text);
    if (fr != FR_OK) {
        printf("f_close(A2retroNET.txt) error: %s (%d)\n", FRESULT_str(fr), fr);
    }

    printf("[settings]\n");
    printf("bootdelay=%d\n", bootdelay);
    printf("[drives]\n");
    printf("number=%d\n", drives_number);
    for (int drive = 0; drive < drives_number; drive++) {
        printf("%d=%s\n", drive + 1, drives[drive].path);
    }
}

static void put_config(void) {
    hdd_reset();

    FIL text;
    FRESULT fr = f_open(&text, "A2retroNET.txt", FA_CREATE_ALWAYS | FA_WRITE);
    if (fr != FR_OK) {
        printf("f_open(A2retroNET.txt, write) error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }

    if(f_printf(&text, "[settings]\nbootdelay=%d\n[drives]\nnumber=%d\n",
            bootdelay, drives_number) < 0) {
        if (f_error(&text)) {
            printf("f_printf(A2retroNET.txt) error\n");
        }
        f_close(&text);
        return;
    }

    for (int drive = 0; drive < drives_number; drive++) {
        if(f_printf(&text, "%d=%s\n", drive + 1, drives[drive].path) < 0) {
            if (f_error(&text)) {
                printf("f_printf(A2retroNET.txt) error\n");
            }
            f_close(&text);
            return;
        }
    }

    fr = f_close(&text);
    if (fr != FR_OK) {
        printf("f_close(A2retroNET.txt, write) error: %s (%d)\n", FRESULT_str(fr), fr);
    }
}

uint8_t config_drives(void) {
    get_config();
    return drives_number;
}

char *config_drivepath(uint8_t drive) {
    get_config();
    return drives[drive].path;
}

static void clrscr(void) {
    memset((void*)&sp_buffer[CONFIG_O_BUFFER], ' ' + 0x80, COLS * ROWS);
}

static void hline(int y) {
    memset((void*)&sp_buffer[CONFIG_O_BUFFER + y * COLS], lowercase ? 0x53 : '-' + 0x80, COLS);
}

static void printfxy(int x, int y, bool inv, const char *format, ...) {
    char buffer[COLS + 1];

    va_list va;
    va_start(va, format);
    vsnprintf(buffer, sizeof(buffer) - x, format, va);
    va_end(va);

    char *source = buffer;
    char *target = (char*)&sp_buffer[CONFIG_O_BUFFER + y * COLS + x];
    while(*source != '\0') {
        char c = *source++;
        if (!lowercase) {
            if (c >= 0x60) {
                c -= 0x20;
            }
        }
        if (!inv) {
            c += 0x80;
        } else {
            if (c >= 0x40 && c < 0x60) {
                c -= 0x40;
            }
        }
        *target++ = c;
    }
}

static void ack(uint8_t retval) {
    sp_buffer[CONFIG_O_RETVAL] = retval;
    sp_read_offset = sp_write_offset = 0;
    sp_control = CONTROL_DONE;
}

static void delay(uint8_t counter) {
    if (counter < bootdelay * 10) {
        sleep_ms(100);
        int seconds = bootdelay - counter / 10 - 1;
        sp_buffer[CONFIG_O_BUFFER] = (seconds ? seconds + '0' : ' ') + 0x80;
        ack(DELAY_MORE);
        return;
    }
    ack(DELAY_DONE);
}

static int get_key(void) {
    ack(CONFIG_CONT);

    while (sp_control == CONTROL_DONE) {
        io_task();
    }

    if (sp_control != CONTROL_CONFIG) {
        return -1;
    }

    return sp_buffer[CONFIG_I_KEY] - 0x80;
}

static bool settings(bool *put) {
    int state = 0;

    while (true) {
        clrscr();
        printfxy(8, 0, false, "A2retroNET Settings (%s)",
            hdd_usb_mounted() ? "USB" : "SD");
        hline(1);

        printfxy( 8, 4, false,        "Boot Delay:   Seconds");
        printfxy(20, 4, state == 0,   "%d", bootdelay);
        printfxy( 8, 7, false,        "Drives No.:");
        printfxy(20, 7, state == 1,   "%d", drives_number);
        printfxy( 8, ROWS - 5, false, "Version " GIT_TAG);
        hline(ROWS - 2);

        // 0123456789012345678901234567890123456789
        // Esc     Space
        //    Back      Toggle
        printfxy( 0, ROWS - 1, true,  "Esc");
        printfxy( 3, ROWS - 1, false, "Back");
        printfxy( 8, ROWS - 1, true,  "Space");
        printfxy(13, ROWS - 1, false, "Toggle");

        int key = get_key();
        get_config();

        switch (key) {
            case -1:    // Ctrl-Reset
                return true;
            case 27:    // Esc
                return false;
            case 9:     // Tab
            case ' ':
                state = (state + 1) % 2;
                break;
            case 8:     // Left
            case 11:    // Up
                if (state == 0) {
                    if (bootdelay > 0) {
                        bootdelay--;
                        *put = true;
                    }
                } else {
                    if (drives_number > 2) {
                        drives_number -= 2;
                        *put = true;
                    }
                }
                break;
            case 10:    // Down
            case 21:    // Right
                if (state == 0) {
                    if (bootdelay < 9) {
                        bootdelay++;
                        *put = true;
                    }
                } else {
                    if (drives_number < MAX_DRIVES) {
                        drives_number += 2;
                        *put = true;
                    }
                }
                break;
            case '0' ... '9':
                if (state == 0) {
                    bootdelay = key - '0';
                    *put = true;
                } else {
                    if (key >= '2' && key <= '0' + MAX_DRIVES && key % 2 == 0) {
                        drives_number = key - '0';
                        *put = true;
                    }
                }
                break;
        }        
    }
}

static bool is_image(char *path) {
    static const char *ext_list[] = {".po", ".hdv", ".2mg"};

    char *ext = strrchr(path, '.');
    if (!ext) {
        return false;
    }
    for (int e = 0; e < sizeof(ext_list) / sizeof(ext_list[0]); e++) {
        if (strcasecmp(ext, ext_list[e]) == 0) {
            return true;
        }
    }
    return false;
}

static int directory_compare(const void *f1, const void *f2) {
    return strcasecmp(((FILINFO*)f1)->fname, ((FILINFO*)f2)->fname);
}

static void get_directory(char *path) {
    directory_size = 0;
    directory[directory_size].fname[0] = '\0';

    DIR dir;
    FRESULT fr = f_opendir(&dir, path);
    if (fr != FR_OK) {
        printf("f_opendir(%s) error: %s (%d)\n", path, FRESULT_str(fr), fr);
        return;
    }

    printf("%s\n", path);
    while (directory_size < MAX_DIR) {
        fr = f_readdir(&dir, &directory[directory_size]);
        if (fr != FR_OK) {
            printf("f_readdir(%s) error: %s (%d)\n", path, FRESULT_str(fr), fr);
            f_closedir(&dir);
            return;
        }
        if (directory[directory_size].fname[0] == '\0') {
            break;
        }
        if (directory[directory_size].fattrib & (AM_HID | AM_SYS) ||
            !(directory[directory_size].fattrib & AM_DIR) &&
            !is_image(directory[directory_size].fname)) {
            continue;
        }
        printf("  0x%02X %s\n", directory[directory_size].fattrib,
                                directory[directory_size].fname);
        directory_size++;
    }
    printf("(%d entries)\n", directory_size);

    fr = f_closedir(&dir);
    if (fr != FR_OK) {
        printf("f_closedir(%s) error: %s (%d)\n", path, FRESULT_str(fr), fr);
    }

    qsort(directory, directory_size, sizeof(directory[0]), directory_compare);
}

void config(void) {
    get_config();

    if (sp_buffer[CONFIG_I_KEY] < CONFIG_UPPERCASE) {
        delay(sp_buffer[CONFIG_I_KEY]);
        return;
    }

    lowercase = sp_buffer[CONFIG_I_KEY] == CONFIG_LOWERCASE;

    char dir[MAX_PATH];
    strcpy(dir, hdd_usb_mounted() ? "USB:/" : "SD:/");
    get_directory(dir);

    int state = 0;
    int drive = 0;
    int entry = 0;
    int start = 0;
    bool put = false;

    while (true) {
        clrscr();
        printfxy(2, 0, false, "A2retroNET Drive Configuration (%s)",
            hdd_usb_mounted() ? "USB" : "SD");
        hline(1);

        for (int d = 0; d < drives_number; d++) {
            printfxy(0, 2 + d, drive == d, "%d", d + 1);
            printfxy(2, 2 + d, drive == d && state == 0, "%s",
                drives[d].path[0] ? drives[d].path : "<empty>");
        }
        hline(2 + drives_number);

        printfxy(0, 3 + drives_number, false, "%s", dir);

        int entries = (ROWS - 2) - (4 + drives_number);
        for (int e = 0; e < entries; e++) {
            if (directory[start + e].fname[0] == '\0') {
                break;
            }
            if (entry == e) {
                printfxy(0, 4 + drives_number + e, true, ">");
            }
            printfxy(2, 4 + drives_number + e, entry == e && state == 1,
                directory[start + e].fattrib & AM_DIR ? "%s/" : "%s", directory[start + e].fname);
        }
        hline(ROWS - 2);

        // 0123456789012345678901234567890123456789
        // Esc     Space       Return       -
        //    Back      Toggle       Insert  Remove
        printfxy( 0, ROWS - 1, true,  "Esc");
        printfxy( 3, ROWS - 1, false, "Quit");
        printfxy( 8, ROWS - 1, true,  "Space");
        printfxy(13, ROWS - 1, false, "Toggle");
        printfxy(20, ROWS - 1, true,  "Return");
        printfxy(26, ROWS - 1, false, "Insert");
        printfxy(33, ROWS - 1, true,  "-");
        printfxy(34, ROWS - 1, false, "Remove");

        int key = get_key();
        get_config();

        switch (key) {
            case -1:    // Ctrl-Reset
                return;
            case 27:    // Esc
                goto quit;
            case 19:    // Ctrl-S
                if (settings(&put)) {
                    return;
                }
                break;
            case 9:     // Tab
            case ' ':
                state = (state + 1) % 2;
                break;
            case 8:     // Left
            case 11:    // Up
                if (state == 0) {
                    drive = (drive - 1 + drives_number) % drives_number;
                } else {
                    if (entry > 0) {
                        entry--;
                    } else {
                        if (start > 0) {
                            start--;
                        }
                    }
                }
                break;
            case 10:    // Down
            case 21:    // Right
                if (state == 0) {
                    drive = (drive + 1) % drives_number;
                } else {
                    if (entry < entries - 1 && entry < directory_size - 1) {
                        entry++;
                    } else {
                        if (start < directory_size - entries) {
                            start++;
                        }
                    }      
                }
                break;
            case '1' ... '8':
                if (key - '1' < drives_number) {
                    drive = key - '1';
                }
                break;
            case '0':
            case 'A' ... 'Z':
            case 'a' ... 'z':
                start = 0;
                entry = 0;
                while (start + entry < directory_size) {
                    if (toupper(directory[start + entry].fname[0]) >= toupper(key)) {
                        break;
                    }
                    if (start < directory_size - entries) {
                        start++;
                    } else {
                        entry++;
                    }
                }
                break;
            case '/':
                dir[dir[0] == 'S' ? 4 : 5] = '\0';
                get_directory(dir);
                start = 0;
                entry = 0;
                break;
            case ':':
                if (dir[0] == 'S' && hdd_usb_mounted()) {
                    strcpy(dir, "USB:/");
                    get_directory(dir);
                    start = 0;
                    entry = 0;
                } else if (dir[0] == 'U' && hdd_sd_mounted()) {
                    strcpy(dir, "SD:/");
                    get_directory(dir);
                    start = 0;
                    entry = 0;
                }
                break;
            case 13:    // Return
                if (directory[start + entry].fattrib & AM_DIR) {
                    strcat(dir, directory[start + entry].fname);
                    strcat(dir, "/");
                    get_directory(dir);
                    start = 0;
                    entry = 0;
                } else {
                    strcpy(drives[drive].path, dir);
                    strcat(drives[drive].path, directory[start + entry].fname);
                    put = true;
                }
                break;
            case '-':
                drives[drive].path[0] = '\0';
                put = true;
                break;
        }        
    }

    quit:
    if (put) {
        put_config();
    }
    ack(CONFIG_QUIT);
}
