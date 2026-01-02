#include "config.h"
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "audio.h"
static void get_config_file_path(char *out, size_t out_size) {
    char exePath[MAX_PATH] = {0};
    DWORD len = GetModuleFileNameA(NULL, exePath, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) {
        strncpy(out, "config.txt", out_size - 1);
        out[out_size - 1] = '\0';
        return;
    }
    char *sep1 = strrchr(exePath, '\\');
    char *sep2 = strrchr(exePath, '/');
    char *sep = (sep1 > sep2) ? sep1 : sep2;
    if (sep) *(sep + 1) = '\0'; /* keep trailing slash */
    snprintf(out, out_size, "%sconfig.txt", exePath);
}


void load_config(int *volume, int *auto_next, int *shuffle, int *loop, int *sort_mode) {
    char cfgPath[MAX_PATH];
    get_config_file_path(cfgPath, sizeof(cfgPath));
    FILE *file = fopen(cfgPath, "r");
    if (file) {
        int read = fscanf(file, "%d %d %d %d %d", volume, auto_next, shuffle, loop, sort_mode);
        if (read < 2) {
            *volume = 50;
            *auto_next = 1;
        }
        if (read < 3) *shuffle = 0;
        if (read < 4) *loop = 0;
        if (read < 5) *sort_mode = 0;
        fclose(file);
    } else {
        *volume = 50; // Default volume
        *auto_next = 1; // Default auto-next
        *shuffle = 0;
        *loop = 0;
        *sort_mode = 0;
    }

    if (*volume < 0) *volume = 0;
    if (*volume > 100) *volume = 100;
    *auto_next = (*auto_next != 0);
    *shuffle = (*shuffle != 0);
    *loop = (*loop != 0);
    if (*sort_mode < 0 || *sort_mode > 2) *sort_mode = 0;
}

void save_config(int volume, int auto_next, int shuffle, int loop, int sort_mode) {
    char cfgPath[MAX_PATH];
    get_config_file_path(cfgPath, sizeof(cfgPath));
    FILE *file = fopen(cfgPath, "w");
    if (file) {
        fprintf(file, "%d %d %d %d %d", volume, auto_next, shuffle, loop, sort_mode);
        fclose(file);
    }
}
