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
#define DEFAULT_SELECTDELAY 3

#define MAX_PATH    256
#define MAX_DIR     256
#define MAX_CONFIGS 16

#define CONFIG_DEFAULT      "A2retroNET.txt"
#define CONFIG_PREFIX       "A2retroNET_"
#define CONFIG_SUFFIX       ".txt"
#define CONFIG_SELECTED     "A2retroNET.select"

#define SECTION_NONE        0
#define SECTION_SETTINGS    1
#define SECTION_DRIVES      2

#define CONFIG_I_KEY    0
#define CONFIG_I_BOOT   1

#define CONFIG_O_RETVAL 0
#define CONFIG_O_BUFFER 1

#define CONFIG_SET_BOOT     100
#define CONFIG_UPPERCASE    101
#define CONFIG_LOWERCASE    102
#define CONFIG_SELECT_UPPER 103
#define CONFIG_SELECT_LOWER 104

#define BOOT_OKAY   0
#define BOOT_FAIL   1

#define DELAY_MORE  0
#define DELAY_DONE  1
#define DELAY_SELECT 2

#define CONFIG_QUIT 0
#define CONFIG_CONT 1
#define CONFIG_EDIT 2
#define CONFIG_NEXT 3

#define COLS    40
#define ROWS    24

static uint8_t bootdelay;

static uint8_t selectdelay;

static char config_path[MAX_PATH] = CONFIG_DEFAULT;

typedef struct {
    char name[MAX_PATH];
    char path[MAX_PATH];
} config_file_t;

static config_file_t configs[MAX_CONFIGS];

static int configs_number;

static int config_selected;

static bool configs_loaded;

static bool config_selecting;

static int config_select_entry;

static int config_select_start;

static int config_select_initial;

static uint8_t config_select_counter;

static bool config_select_dirty;

static bool config_select_manual;

static struct {
    char path[MAX_PATH];
} drives[MAX_DRIVES];

static uint8_t drives_number;

static bool lowercase;

static FILINFO directory[MAX_DIR];

static int directory_size;

void config_reset(void) {
    configs_loaded = false;
    config_selecting = false;
    drives_number = 0; 
    memset(drives, 0, sizeof(drives));
}

static bool suffix_matches(const char *string, const char *suffix) {
    int string_len = strlen(string);
    int suffix_len = strlen(suffix);
    if (string_len < suffix_len) {
        return false;
    }
    return strcasecmp(string + string_len - suffix_len, suffix) == 0;
}

static bool is_config_file(const char *name) {
    if (strcasecmp(name, CONFIG_DEFAULT) == 0) {
        return true;
    }
    if (strncasecmp(name, CONFIG_PREFIX, strlen(CONFIG_PREFIX)) != 0 ||
        !suffix_matches(name, CONFIG_SUFFIX)) {
        return false;
    }
    return strlen(name) > strlen(CONFIG_PREFIX) + strlen(CONFIG_SUFFIX);
}

static void config_name(char *target, const char *source) {
    if (strcasecmp(source, CONFIG_DEFAULT) == 0) {
        strcpy(target, "default");
        return;
    }

    int len = strlen(source) - strlen(CONFIG_PREFIX) - strlen(CONFIG_SUFFIX);
    strncpy(target, source + strlen(CONFIG_PREFIX), len);
    target[len] = '\0';
}

static int config_compare(const void *c1, const void *c2) {
    const char *p1 = ((const config_file_t*)c1)->path;
    const char *p2 = ((const config_file_t*)c2)->path;

    if (strcasecmp(p1, CONFIG_DEFAULT) == 0) {
        return -1;
    }
    if (strcasecmp(p2, CONFIG_DEFAULT) == 0) {
        return 1;
    }
    return strcasecmp(((const config_file_t*)c1)->name,
                      ((const config_file_t*)c2)->name);
}

static void add_config_file(const char *path) {
    char name[MAX_PATH];
    config_name(name, path);

    int pos = configs_number;
    while (pos > 0) {
        config_file_t config;
        strcpy(config.path, path);
        strcpy(config.name, name);
        if (config_compare(&configs[pos - 1], &config) <= 0) {
            break;
        }
        if (pos < MAX_CONFIGS) {
            configs[pos] = configs[pos - 1];
        }
        pos--;
    }

    if (pos >= MAX_CONFIGS) {
        return;
    }

    if (configs_number < MAX_CONFIGS) {
        configs_number++;
    }
    for (int i = configs_number - 1; i > pos; i--) {
        configs[i] = configs[i - 1];
    }
    strcpy(configs[pos].path, path);
    strcpy(configs[pos].name, name);
}

static void select_config(int selected, bool save) {
    if (selected < 0 || selected >= configs_number) {
        return;
    }

    if (strcasecmp(config_path, configs[selected].path) != 0) {
        hdd_reset();
        drives_number = 0;
        memset(drives, 0, sizeof(drives));
    }

    strcpy(config_path, configs[selected].path);
    config_selected = selected;

    if (!save || configs_number < 2) {
        return;
    }

    FIL text;
    FRESULT fr = f_open(&text, CONFIG_SELECTED, FA_CREATE_ALWAYS | FA_WRITE);
    if (fr != FR_OK) {
        printf("f_open(%s, write) error: %s (%d)\n", CONFIG_SELECTED, FRESULT_str(fr), fr);
        return;
    }

    if(f_printf(&text, "%s\n", config_path) < 0 && f_error(&text)) {
        printf("f_printf(%s) error\n", CONFIG_SELECTED);
    }

    fr = f_close(&text);
    if (fr != FR_OK) {
        printf("f_close(%s, write) error: %s (%d)\n", CONFIG_SELECTED, FRESULT_str(fr), fr);
    }
}

static void get_configs(void) {
    if (configs_loaded) {
        return;
    }

    configs_loaded = true;
    configs_number = 0;
    config_selected = -1;
    strcpy(config_path, CONFIG_DEFAULT);

    char root[MAX_PATH];
    strcpy(root, hdd_usb_mounted() ? "USB:/" : "SD:/");

    DIR dir;
    FRESULT fr = f_opendir(&dir, root);
    if (fr != FR_OK) {
        printf("f_opendir(%s) error: %s (%d)\n", root, FRESULT_str(fr), fr);
        return;
    }

    while (true) {
        FILINFO entry;
        fr = f_readdir(&dir, &entry);
        if (fr != FR_OK) {
            printf("f_readdir(%s) error: %s (%d)\n", root, FRESULT_str(fr), fr);
            f_closedir(&dir);
            return;
        }
        if (entry.fname[0] == '\0') {
            break;
        }
        if (entry.fattrib & (AM_DIR | AM_HID | AM_SYS) ||
            !is_config_file(entry.fname)) {
            continue;
        }

        add_config_file(entry.fname);
    }

    fr = f_closedir(&dir);
    if (fr != FR_OK) {
        printf("f_closedir(%s) error: %s (%d)\n", root, FRESULT_str(fr), fr);
    }

    if (configs_number == 0) {
        return;
    }

    int selected = -1;
    FIL text;
    fr = f_open(&text, CONFIG_SELECTED, FA_OPEN_EXISTING | FA_READ);
    if (fr == FR_OK) {
        char line[MAX_PATH];
        if (f_gets(line, sizeof(line), &text)) {
            int len = strlen(line);
            while (len && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
                line[--len] = '\0';
            }
            for (int i = 0; i < configs_number; i++) {
                if (strcasecmp(line, configs[i].path) == 0) {
                    selected = i;
                    break;
                }
            }
        }
        f_close(&text);
    }

    if (selected < 0) {
        for (int i = 0; i < configs_number; i++) {
            if (strcasecmp(configs[i].path, CONFIG_DEFAULT) == 0) {
                selected = i;
                break;
            }
        }
    }
    if (selected < 0) {
        selected = 0;
    }

    select_config(selected, false);
}

static void get_config(void) {
    if (drives_number) {
        return;
    }
    get_configs();
    bootdelay = DEFAULT_BOOTDELAY;
    drives_number = MAX_DRIVES;

    FIL text;
    FRESULT fr = f_open(&text, config_path, FA_OPEN_EXISTING | FA_READ);
    if (fr != FR_OK) {
        printf("f_open(%s, read) error: %s (%d)\n", config_path, FRESULT_str(fr), fr);
        return;
    }

    int section = SECTION_NONE;
    while (true) {
        char line[MAX_PATH];
        char *success = f_gets(line, sizeof(line), &text);
        if (success == NULL) {
            if (f_error(&text)) {
                printf("f_gets(%s) error\n", config_path);
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
        printf("f_close(%s) error: %s (%d)\n", config_path, FRESULT_str(fr), fr);
    }

    printf("%s\n", config_path);
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
    FRESULT fr = f_open(&text, config_path, FA_CREATE_ALWAYS | FA_WRITE);
    if (fr != FR_OK) {
        printf("f_open(%s, write) error: %s (%d)\n", config_path, FRESULT_str(fr), fr);
        return;
    }

    if(f_printf(&text, "[settings]\nbootdelay=%d\n[drives]\nnumber=%d\n",
            bootdelay, drives_number) < 0) {
        if (f_error(&text)) {
            printf("f_printf(%s) error\n", config_path);
        }
        f_close(&text);
        return;
    }

    for (int drive = 0; drive < drives_number; drive++) {
        if(f_printf(&text, "%d=%s\n", drive + 1, drives[drive].path) < 0) {
            if (f_error(&text)) {
                printf("f_printf(%s) error\n", config_path);
            }
            f_close(&text);
            return;
        }
    }

    fr = f_close(&text);
    if (fr != FR_OK) {
        printf("f_close(%s, write) error: %s (%d)\n", config_path, FRESULT_str(fr), fr);
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

static void set_boot(uint8_t boot) {
    uint8_t device = boot - 0x80 - '0';
    if (device > drives_number) {
        ack(BOOT_FAIL);
        return;
    }
    sp_boot = device - 1;
    printf("Set Boot(Drive=%d)\n", sp_boot);
    ack(BOOT_OKAY);
}

static void delay(uint8_t counter) {
    if (counter == 0 && configs_number > 1) {
        selectdelay = bootdelay ? bootdelay : DEFAULT_SELECTDELAY;
        ack(DELAY_SELECT);
        return;
    }
    if (counter < bootdelay * 10) {
        sleep_ms(100);
        int seconds = bootdelay - counter / 10 - 1;
        sp_buffer[CONFIG_O_BUFFER] = (seconds ? seconds + '0' : ' ') + 0x80;
        ack(DELAY_MORE);
        return;
    }
    ack(DELAY_DONE);
}

static void config_select(uint8_t command) {
    if (command == CONFIG_SELECT_UPPER || command == CONFIG_SELECT_LOWER) {
        lowercase = command == CONFIG_SELECT_LOWER;
        config_selecting = true;
        config_select_entry = config_selected < 0 ? 0 : config_selected;
        config_select_initial = config_select_entry;
        config_select_start = 0;
        config_select_counter = 0;
        config_select_dirty = false;
        config_select_manual = false;
    }

    if (!config_selecting || configs_number < 2) {
        config_selecting = false;
        ack(CONFIG_QUIT);
        return;
    }

    int key = command >= 0x80 ? command - 0x80 : 0;
    int entries = ROWS - 6;

    switch (key) {
        case 27:    // Esc
            select_config(config_select_initial, false);
            config_selecting = false;
            ack(CONFIG_QUIT);
            return;
        case 'C':
        case 'c':
            select_config(config_select_entry, true);
            config_selecting = false;
            ack(CONFIG_EDIT);
            return;
        case 'N':
        case 'n':
            select_config(config_select_entry, config_select_dirty);
            config_selecting = false;
            ack(CONFIG_NEXT);
            return;
        case 8:     // Left
        case 11:    // Up
            config_select_entry = (config_select_entry - 1 + configs_number) % configs_number;
            config_select_dirty = true;
            config_select_manual = true;
            break;
        case 10:    // Down
        case 21:    // Right
            config_select_entry = (config_select_entry + 1) % configs_number;
            config_select_dirty = true;
            config_select_manual = true;
            break;
        case 13:    // Return
            select_config(config_select_entry, true);
            config_selecting = false;
            ack(CONFIG_QUIT);
            return;
        case '1' ... '8':
            config_select_manual = true;
            select_config(config_select_entry, false);
            get_config();
            if (key - '0' > drives_number) {
                break;
            }
            select_config(config_select_entry, true);
            sp_boot = key - '1';
            printf("Set Boot(Drive=%d)\n", sp_boot);
            config_selecting = false;
            ack(CONFIG_QUIT);
            return;
    }

    if (config_select_entry < config_select_start) {
        config_select_start = config_select_entry;
    }
    if (config_select_entry >= config_select_start + entries) {
        config_select_start = config_select_entry - entries + 1;
    }

    if (!config_select_manual && config_select_counter >= selectdelay * 10) {
        select_config(config_select_entry, config_select_dirty);
        config_selecting = false;
        ack(CONFIG_QUIT);
        return;
    }

    int seconds = selectdelay - config_select_counter / 10 - 1;

    clrscr();
    printfxy(2, 0, false, "A2retroNET Configs (%s)",
        hdd_usb_mounted() ? "USB" : "SD");
    hline(1);

    for (int i = 0; i < entries && config_select_start + i < configs_number; i++) {
        int c = config_select_start + i;
        printfxy(0, 3 + i, config_select_entry == c, ">");
        printfxy(2, 3 + i, config_select_entry == c, "%s", configs[c].name);
    }

    hline(ROWS - 2);
    printfxy( 0, ROWS - 1, true,  "Esc");
    printfxy( 3, ROWS - 1, false, "Boot");
    printfxy( 9, ROWS - 1, true,  "Ret");
    printfxy(12, ROWS - 1, false, "Boot");
    printfxy(18, ROWS - 1, true,  "1-8");
    printfxy(21, ROWS - 1, false, "Drive");
    printfxy(28, ROWS - 1, true,  "C");
    printfxy(29, ROWS - 1, false, "Cfg");
    printfxy(34, ROWS - 1, true,  "N");
    printfxy(35, ROWS - 1, false, "Next");
    if (!config_select_manual && seconds) {
        printfxy(39, ROWS - 1, false, "%d", seconds);
    }

    if (!config_select_manual) {
        config_select_counter++;
    }
    sleep_ms(100);
    ack(CONFIG_CONT);
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
    if (ext == NULL) {
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

    if (sp_buffer[CONFIG_I_KEY] == CONFIG_SET_BOOT) {
        set_boot(sp_buffer[CONFIG_I_BOOT]);
        return;
    }

    if (sp_buffer[CONFIG_I_KEY] == CONFIG_SELECT_UPPER ||
        sp_buffer[CONFIG_I_KEY] == CONFIG_SELECT_LOWER ||
        config_selecting) {
        config_select(sp_buffer[CONFIG_I_KEY]);
        return;
    }

    if (sp_buffer[CONFIG_I_KEY] < CONFIG_UPPERCASE) {
        delay(sp_buffer[CONFIG_I_KEY]);
        return;
    }

    sp_boot = 0;
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
