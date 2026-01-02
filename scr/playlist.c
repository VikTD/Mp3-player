#include "playlist.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <time.h>

static int has_mp3_extension(const char *name) {
    const char *dot = strrchr(name, '.');
    if (!dot) return 0;
    return _stricmp(dot, ".mp3") == 0;
}

static void playlist_add(Playlist *pl, const char *path) {
    char **new_files = realloc(pl->files, sizeof(char*) * (pl->count + 1));
    if (!new_files) return;
    pl->files = new_files;

    pl->files[pl->count] = malloc(MAX_PATH);
    if (!pl->files[pl->count]) return;

    strncpy(pl->files[pl->count], path, MAX_PATH - 1);
    pl->files[pl->count][MAX_PATH - 1] = '\0';
    pl->count++;
}

static void playlist_scan_folder(Playlist *pl, const char *folder) {
    WIN32_FIND_DATA findFileData;
    HANDLE hFind;

    char searchPath[MAX_PATH];
    snprintf(searchPath, MAX_PATH, "%s\\*", folder);

    hFind = FindFirstFile(searchPath, &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        return;
    }

    do {
        const char *name = findFileData.cFileName;

        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
            continue;
        }

        char fullPath[MAX_PATH];
        snprintf(fullPath, MAX_PATH, "%s\\%s", folder, name);

        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            playlist_scan_folder(pl, fullPath);
        } else if (has_mp3_extension(name)) {
            playlist_add(pl, fullPath);
        }
    } while (FindNextFile(hFind, &findFileData) != 0);

    FindClose(hFind);
}

static const char* filename_from_path(const char *path) {
    const char *slash1 = strrchr(path, '\\');
    const char *slash2 = strrchr(path, '/');
    const char *sep = slash1 > slash2 ? slash1 : slash2;
    return sep ? sep + 1 : path;
}

static int compare_path(const void *a, const void *b) {
    const char *pa = *(const char**)a;
    const char *pb = *(const char**)b;
    return _stricmp(pa, pb);
}

static int compare_name(const void *a, const void *b) {
    const char *pa = *(const char**)a;
    const char *pb = *(const char**)b;
    int cmp = _stricmp(filename_from_path(pa), filename_from_path(pb));
    if (cmp != 0) return cmp;
    return _stricmp(pa, pb);
}

static int compare_folder(const void *a, const void *b) {
    const char *pa = *(const char**)a;
    const char *pb = *(const char**)b;
    char fa[MAX_PATH], fb[MAX_PATH];
    const char *back_a = strrchr(pa, '\\');
    const char *slash_a = strrchr(pa, '/');
    const char *sep_a = back_a > slash_a ? back_a : slash_a;
    size_t len_a = sep_a ? (size_t)(sep_a - pa) : 0;
    if (len_a >= sizeof(fa)) len_a = sizeof(fa) - 1;
    memcpy(fa, pa, len_a);
    fa[len_a] = '\0';

    const char *back_b = strrchr(pb, '\\');
    const char *slash_b = strrchr(pb, '/');
    const char *sep_b = back_b > slash_b ? back_b : slash_b;
    size_t len_b = sep_b ? (size_t)(sep_b - pb) : 0;
    if (len_b >= sizeof(fb)) len_b = sizeof(fb) - 1;
    memcpy(fb, pb, len_b);
    fb[len_b] = '\0';

    int cmp = _stricmp(fa, fb);
    if (cmp != 0) return cmp;
    return _stricmp(filename_from_path(pa), filename_from_path(pb));
}

static void playlist_sort(Playlist *pl) {
    if (!pl || pl->count <= 1) return;
    int current_index = pl->current;
    char current_path[MAX_PATH] = {0};
    if (pl->files && pl->files[current_index]) {
        strncpy(current_path, pl->files[current_index], sizeof(current_path) - 1);
    }

    switch (pl->sort_mode) {
        case PLAYLIST_SORT_NAME:
            qsort(pl->files, pl->count, sizeof(char*), compare_name);
            break;
        case PLAYLIST_SORT_FOLDER:
            qsort(pl->files, pl->count, sizeof(char*), compare_folder);
            break;
        case PLAYLIST_SORT_PATH:
        default:
            qsort(pl->files, pl->count, sizeof(char*), compare_path);
            break;
    }

    if (current_path[0]) {
        for (int i = 0; i < pl->count; i++) {
            if (_stricmp(pl->files[i], current_path) == 0) {
                pl->current = i;
                break;
            }
        }
    }
}

int playlist_load(Playlist *pl, const char *folder) {
    pl->files = NULL;
    pl->count = 0;
    pl->current = 0;
    if (pl->sort_mode < 0 || pl->sort_mode > 2) pl->sort_mode = PLAYLIST_SORT_PATH;
    playlist_scan_folder(pl, folder);

    if (pl->count == 0) {
        printf("No MP3 files found in %s\n", folder);
        return -1;
    }
    playlist_sort(pl);
    return pl->count;
}

void playlist_free(Playlist *pl) {
    for (int i = 0; i < pl->count; i++) {
        free(pl->files[i]);
    }
    free(pl->files);
    pl->files = NULL;
    pl->count = 0;
    pl->current = 0;
}

const char* playlist_next(Playlist *pl) {
    if (pl->count == 0) return NULL;
    pl->current = (pl->current + 1) % pl->count;
    return pl->files[pl->current];
}

const char* playlist_prev(Playlist *pl) {
    if (pl->count == 0) return NULL;
    pl->current = (pl->current - 1 + pl->count) % pl->count;
    return pl->files[pl->current];
}

const char* playlist_current(Playlist *pl) {
    if (pl->count == 0) return NULL;
    return pl->files[pl->current];
}

void playlist_set_sort(Playlist *pl, PlaylistSortMode mode) {
    if (!pl) return;
    pl->sort_mode = mode;
    playlist_sort(pl);
}
