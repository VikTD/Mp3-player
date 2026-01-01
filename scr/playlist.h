#ifndef PLAYLIST_H
#define PLAYLIST_H

typedef struct {
    char **files;
    int count;
    int current;
    int sort_mode;
} Playlist;

typedef enum {
    PLAYLIST_SORT_PATH = 0,
    PLAYLIST_SORT_NAME = 1,
    PLAYLIST_SORT_FOLDER = 2
} PlaylistSortMode;

int playlist_load(Playlist *pl, const char *folder);
void playlist_free(Playlist *pl);
const char* playlist_next(Playlist *pl);
const char* playlist_prev(Playlist *pl);
const char* playlist_current(Playlist *pl);
void playlist_set_sort(Playlist *pl, PlaylistSortMode mode);

#endif
