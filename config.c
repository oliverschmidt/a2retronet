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
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <f_util.h>

#include "config.h"

#define MAX_PATH 256

#define SECTION_NONE    0
#define SECTION_DRIVES  1

static uint8_t drives_number;

static struct {
    char     path[MAX_PATH];
} drives[MAX_DRIVES];

void config_reset(void) {
    drives_number = 0; 
    memset(drives, 0, sizeof(drives));
}

static void get_config(void) {
    if (drives_number) {
        return;
    }
    drives_number = MAX_DRIVES;

    FIL text;
    FRESULT fr = f_open(&text, "A2retroNET.txt", FA_OPEN_EXISTING | FA_READ);
    if (fr != FR_OK) {
        printf("f_open(A2retroNET.txt) error: %s (%d)\n", FRESULT_str(fr), fr);
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
            if (strncasecmp(line, "[drives]", 8) == 0) {
                section = SECTION_DRIVES;
            }
        }
        switch (section) {
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

    printf("[drives]\n");
    printf("number=%d\n", drives_number);
    for (int drive = 0; drive < drives_number; drive++) {
        printf("%d=%s\n", drive + 1, drives[drive].path);
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
