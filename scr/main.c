#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <shobjidl.h>
#include <objbase.h>
#include <process.h>
#include <conio.h>
#include <time.h>
#include <ctype.h>
#include "audio.h"
#include "playlist.h"
#include "config.h"

#define PROGRESS_WIDTH 30


#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

#define UI_WIDTH 60

static int g_use_ansi = 0;
static int g_use_utf8 = 0;

// ANSI helpers (only used if g_use_ansi == 1)
#define ANSI_RESET   "\x1b[0m"
#define ANSI_BOLD    "\x1b[1m"
#define ANSI_DIM     "\x1b[2m"
#define ANSI_GREEN   "\x1b[32m"
#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_CYAN    "\x1b[36m"

static void console_init(void) {
    // UTF-8 output (for box drawing / block progress bar).
    if (SetConsoleOutputCP(CP_UTF8)) {
        g_use_utf8 = 1;
        SetConsoleCP(CP_UTF8);
    }

    // ANSI colors (Windows 10+; harmless if unsupported).
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD mode = 0;
        if (GetConsoleMode(hOut, &mode)) {
            DWORD newMode = mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            if (SetConsoleMode(hOut, newMode)) {
                g_use_ansi = 1;
            }
        }
    }
}

static void clear_screen(void) {
    if (g_use_ansi) {
        fputs("\x1b[2J\x1b[H", stdout);
    } else {
        clear_screen();
    }
}

static void ui_hr(int thick) {
    const char *L = "+", *M = "-", *R = "+";
    if (g_use_utf8) {
        L = thick ? "┏" : "┌";
        M = thick ? "━" : "─";
        R = thick ? "┓" : "┐";
    }
    fputs(L, stdout);
    for (int i = 0; i < UI_WIDTH; i++) fputs(M, stdout);
    fputs(R, stdout);
    putchar('\n');
}

static void ui_hr_mid(void) {
    const char *L = "+", *M = "-", *R = "+";
    if (g_use_utf8) {
        L = "┣";
        M = "─";
        R = "┫";
    }
    fputs(L, stdout);
    for (int i = 0; i < UI_WIDTH; i++) fputs(M, stdout);
    fputs(R, stdout);
    putchar('\n');
}

static void ui_hr_bottom(int thick) {
    const char *L = "+", *M = "-", *R = "+";
    if (g_use_utf8) {
        L = thick ? "┗" : "└";
        M = thick ? "━" : "─";
        R = thick ? "┛" : "┘";
    }
    fputs(L, stdout);
    for (int i = 0; i < UI_WIDTH; i++) fputs(M, stdout);
    fputs(R, stdout);
    putchar('\n');
}

static void ui_row(const char *text) {
    const char *V = "|";
    if (g_use_utf8) V = "┃";
    // Keep the row simple; don't over-pad to avoid ANSI width issues.
    printf("%s %s\n", V, text);
}


static const char* pick_next_track(AudioEngine *audio, Playlist *pl, int respect_loop);
static const char* pick_prev_track(AudioEngine *audio, Playlist *pl);

static const char* filename_from_path(const char *path);
static int prompt_for_folder(char *folder, size_t size, const char *message);

static const char* pick_next_track(AudioEngine *audio, Playlist *pl, int respect_loop) {
    if (pl->count == 0) return NULL;
    if (respect_loop && audio->loop) {
        return pl->files[pl->current];
    }
    if (audio->shuffle && pl->count > 1) {
        int next_idx = pl->current;
        do {
            next_idx = rand() % pl->count;
        } while (next_idx == pl->current);
        pl->current = next_idx;
        return pl->files[next_idx];
    }
    pl->current = (pl->current + 1) % pl->count;
    return pl->files[pl->current];
}

static const char* pick_prev_track(AudioEngine *audio, Playlist *pl) {
    if (pl->count == 0) return NULL;
    if (audio->shuffle && pl->count > 1) {
        int prev_idx = rand() % pl->count;
        pl->current = prev_idx;
        return pl->files[prev_idx];
    }
    pl->current = (pl->current - 1 + pl->count) % pl->count;
    return pl->files[pl->current];
}

static const char* filename_from_path(const char *path) {
    const char *slash1 = strrchr(path, '\\');
    const char *slash2 = strrchr(path, '/');
    const char *sep = slash1 > slash2 ? slash1 : slash2;
    return sep ? sep + 1 : path;
}

static void folder_from_path(const char *path, char *out, size_t out_size) {
    const char *last_backslash = strrchr(path, '\\');
    const char *last_slash = strrchr(path, '/');
    const char *sep = last_backslash > last_slash ? last_backslash : last_slash;
    size_t len = sep ? (size_t)(sep - path) : 0;
    if (len >= out_size) len = out_size - 1;
    if (len > 0) {
        memcpy(out, path, len);
        out[len] = '\0';
    } else {
        strncpy(out, "(root)", out_size - 1);
        out[out_size - 1] = '\0';
    }
}

static void print_playlist(const Playlist *pl) {
    if (!pl || pl->count == 0) {
        printf("Playlist is empty.\n");
        printf("Press Enter to continue...");
        getchar();
        return;
    }

    printf("Playlist by folders (%d tracks):\n", pl->count);
    char current_folder[MAX_PATH] = "";
    for (int i = 0; i < pl->count; i++) {
        char track_folder[MAX_PATH];
        folder_from_path(pl->files[i], track_folder, sizeof(track_folder));
        if (strcmp(current_folder, track_folder) != 0) {
            strncpy(current_folder, track_folder, sizeof(current_folder) - 1);
            current_folder[sizeof(current_folder) - 1] = '\0';
            printf("\n[%s]\n", current_folder);
        }
        printf("%c %d. %s\n", (i == pl->current) ? '*' : ' ', i + 1, filename_from_path(pl->files[i]));
    }
    printf("\n* marks the current track.\n");
    printf("Press Enter to continue...");
    getchar();
}


static void print_playlist_list(const Playlist *pl) {
    if (!pl || pl->count == 0) {
        printf("Playlist is empty.\n");
        return;
    }

    if (g_use_ansi) {
        printf(ANSI_BOLD "Playlist (%d tracks):" ANSI_RESET "\n", pl->count);
    } else {
        printf("Playlist (%d tracks):\n", pl->count);
    }

    char current_folder[MAX_PATH] = "";
    for (int i = 0; i < pl->count; i++) {
        char track_folder[MAX_PATH];
        folder_from_path(pl->files[i], track_folder, sizeof(track_folder));
        if (strcmp(current_folder, track_folder) != 0) {
            strncpy(current_folder, track_folder, sizeof(current_folder) - 1);
            current_folder[sizeof(current_folder) - 1] = '\0';
            if (g_use_ansi) {
                printf("\n" ANSI_CYAN "[%s]" ANSI_RESET "\n", current_folder);
            } else {
                printf("\n[%s]\n", current_folder);
            }
        }

        const char *mark = " ";
        if (i == pl->current) mark = (g_use_utf8 ? "▶" : ">");
        if (i == pl->current && g_use_ansi) {
            printf("%s " ANSI_GREEN "%3d." ANSI_RESET " %s\n", mark, i + 1, filename_from_path(pl->files[i]));
        } else {
            printf("%s %3d. %s\n", mark, i + 1, filename_from_path(pl->files[i]));
        }
    }

    printf("\n%s marks the current track.\n", (g_use_utf8 ? "▶" : ">"));
}

static int load_playlist_from_folder(AudioEngine *audio, Playlist *pl, char *folder_out, size_t folder_out_size, const char *new_folder) {
    if (!new_folder || !new_folder[0]) return 0;

    Playlist newpl;
    memset(&newpl, 0, sizeof(newpl));
    newpl.sort_mode = pl->sort_mode;

    if (playlist_load(&newpl, new_folder) <= 0) {
        playlist_free(&newpl);
        return 0;
    }

    audio_stop(audio);
    playlist_free(pl);
    *pl = newpl;

    strncpy(folder_out, new_folder, folder_out_size - 1);
    folder_out[folder_out_size - 1] = '\0';
    return 1;
}

static int change_playlist_folder(AudioEngine *audio, Playlist *pl, char *folder, size_t folder_size) {
    char newFolder[MAX_PATH] = {0};

    if (!prompt_for_folder(newFolder, MAX_PATH, "Choose a playlist folder (Explorer will open)...")) {
        return 0; // cancelled
    }

    if (!load_playlist_from_folder(audio, pl, folder, folder_size, newFolder)) {
        printf("No MP3 files found in the selected folder.\n");
        Sleep(1000);
        return 0;
    }

    printf("Playlist loaded.\n");
    Sleep(500);
    return 1;
}

static void playlist_menu(AudioEngine *audio, Playlist *pl, char *folder, size_t folder_size) {
    char line[128];

    for (;;) {
        clear_screen();
        ui_hr(0);
        if (g_use_ansi) {
            printf("%s %sPLAYLIST MENU%s\n", (g_use_utf8 ? "│" : "|"), ANSI_BOLD ANSI_CYAN, ANSI_RESET);
        } else {
            printf("%s PLAYLIST MENU\n", (g_use_utf8 ? "│" : "|"));
        }
        ui_hr_mid();
        print_playlist_list(pl);

        printf("\nOptions:\n");
        printf("  [number] Play track\n");
        printf("  C) Change playlist folder\n");
        printf("  B) Back\n");
        printf("\nChoice: ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) return;

        // trim
        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) line[--len] = '\0';
        if (len == 0) continue;

        char c = (char)toupper((unsigned char)line[0]);
        if (c == 'B') return;

        if (c == 'C') {
            if (change_playlist_folder(audio, pl, folder, folder_size)) {
                // Reload OK; start from current.
                audio_stop(audio);
                audio_play(audio, playlist_current(pl));
            }
            continue;
        }

        int idx = atoi(line);
        if (idx >= 1 && idx <= pl->count) {
            pl->current = idx - 1;
            audio_stop(audio);
            audio_play(audio, playlist_current(pl));
            if (g_use_ansi) {
                printf(ANSI_GREEN "Now playing: %s" ANSI_RESET "\n", filename_from_path(playlist_current(pl)));
            } else {
                printf("Now playing: %s\n", filename_from_path(playlist_current(pl)));
            }
            Sleep(700);
        } else {
            printf("Invalid choice.\n");
            Sleep(700);
        }
    }
}

static int pick_folder(char *out, size_t out_size) {
    if (!out || out_size == 0) return 0;
    out[0] = '\0';

    HRESULT hrInit = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    int didInit = SUCCEEDED(hrInit);
    if (FAILED(hrInit) && hrInit != RPC_E_CHANGED_MODE) {
        return 0;
    }

    IFileDialog *dlg = NULL;
    HRESULT hr = CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, &IID_IFileDialog, (void**)&dlg);
    if (SUCCEEDED(hr) && dlg) {
        DWORD opts = 0;
        dlg->lpVtbl->GetOptions(dlg, &opts);
        dlg->lpVtbl->SetOptions(dlg, opts | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
        dlg->lpVtbl->SetTitle(dlg, L"Select a folder (playlist)");

        HWND owner = GetConsoleWindow();
        hr = dlg->lpVtbl->Show(dlg, owner);

        if (SUCCEEDED(hr)) {
            IShellItem *item = NULL;
            if (SUCCEEDED(dlg->lpVtbl->GetResult(dlg, &item)) && item) {
                PWSTR wpath = NULL;
                if (SUCCEEDED(item->lpVtbl->GetDisplayName(item, SIGDN_FILESYSPATH, &wpath)) && wpath) {
                    int bytes = WideCharToMultiByte(CP_ACP, 0, wpath, -1, out, (int)out_size, NULL, NULL);
                    if (bytes == 0) out[0] = '\0';
                    CoTaskMemFree(wpath);
                    item->lpVtbl->Release(item);
                    dlg->lpVtbl->Release(dlg);
                    if (didInit) CoUninitialize();
                    return (out[0] != '\0');
                }
                if (wpath) CoTaskMemFree(wpath);
                item->lpVtbl->Release(item);
            }
        }
        dlg->lpVtbl->Release(dlg);
    }

    if (didInit) CoUninitialize();
    return 0;
}


static int prompt_for_folder(char *folder, size_t size, const char *message) {
    printf("%s\n", message);
    if (pick_folder(folder, size)) {
        return 1;
    }
    printf("Enter folder path manually: ");
    return scanf("%[^\n]%*c", folder) == 1;
}

static void format_time(int seconds, char *out, size_t out_size) {
    if (seconds < 0) seconds = 0;
    int mins = seconds / 60;
    int secs = seconds % 60;
    snprintf(out, out_size, "%02d:%02d", mins, secs);
}

static void render_progress(int position, int duration) {
    char left[6], right[6];
    format_time(position, left, sizeof(left));
    format_time(duration, right, sizeof(right));

    float pct = (duration > 0) ? (float)position / (float)duration : 0.0f;
    if (pct < 0.0f) pct = 0.0f;
    if (pct > 1.0f) pct = 1.0f;

    // Smooth progress bar with Unicode blocks (fallback to ASCII if needed).
    const char *full = (g_use_utf8 ? "█" : "#");
    const char *empty = (g_use_utf8 ? "░" : "-");
    const char *partials[] = {"", "▏", "▎", "▍", "▌", "▋", "▊", "▉"}; // 1/8 .. 7/8

    float total = pct * (float)PROGRESS_WIDTH;
    int whole = (int)total;
    float frac = total - (float)whole;
    int pidx = (int)(frac * 8.0f + 0.5f); // 0..8
    if (pidx < 0) pidx = 0;
    if (pidx > 8) pidx = 8;
    if (whole > PROGRESS_WIDTH) whole = PROGRESS_WIDTH;

    int used = whole;
    int show_partial = (pidx > 0 && used < PROGRESS_WIDTH);
    if (show_partial) used++;

    if (g_use_ansi) {
        printf(ANSI_DIM "%s / %s  " ANSI_RESET, left, right);
    } else {
        printf("%s / %s  ", left, right);
    }

    putchar('[');
    for (int i = 0; i < whole && i < PROGRESS_WIDTH; i++) {
        fputs(full, stdout);
    }
    if (show_partial) {
        // If partial blocks are not available, just print full.
        if (g_use_utf8 && pidx < 8) fputs(partials[pidx], stdout);
        else fputs(full, stdout);
    }
    for (int i = used; i < PROGRESS_WIDTH; i++) {
        fputs(empty, stdout);
    }
    putchar(']');

    printf(" %3d%%\n", (int)(pct * 100.0f + 0.5f));
}

static void render_menu(const Playlist *pl, const AudioEngine *audio, int paused, int position, int duration) {
    clear_screen();

    const char *cur = playlist_current((Playlist*)pl);
    if (!cur) cur = "(none)";

    // Header
    ui_hr(1);
    if (g_use_ansi) {
        printf("%s %sMP3 PLAYER%s\n", (g_use_utf8 ? "┃" : "|"), ANSI_BOLD ANSI_CYAN, ANSI_RESET);
    } else {
        printf("%s MP3 PLAYER\n", (g_use_utf8 ? "┃" : "|"));
    }
    ui_hr_mid();

    // Now playing
    char nowLine[1024];
    snprintf(nowLine, sizeof(nowLine), "Now: %d/%d  %s%s",
             pl->count ? pl->current + 1 : 0, pl->count,
             filename_from_path(cur),
             paused ? "  [PAUSED]" : "");
    printf("%s %s\n", (g_use_utf8 ? "┃" : "|"), nowLine);

    // Progress + status
    render_progress(position, duration);

    const char *on = "ON";
    const char *off = "OFF";
    if (g_use_ansi) {
        printf("%s Status: Auto-Next[%s%s%s]  Shuffle[%s%s%s]  Loop[%s%s%s]  Sort[%s]\n",
               (g_use_utf8 ? "┃" : "|"),
               audio->auto_next ? ANSI_GREEN : ANSI_DIM, audio->auto_next ? on : off, ANSI_RESET,
               audio->shuffle ? ANSI_GREEN : ANSI_DIM, audio->shuffle ? on : off, ANSI_RESET,
               audio->loop ? ANSI_GREEN : ANSI_DIM, audio->loop ? on : off, ANSI_RESET,
               (pl->sort_mode == PLAYLIST_SORT_NAME) ? "Name" :
               (pl->sort_mode == PLAYLIST_SORT_FOLDER) ? "Folder" : "Path");
    } else {
        printf("%s Status: Auto-Next[%s]  Shuffle[%s]  Loop[%s]  Sort[%s]\n",
               (g_use_utf8 ? "┃" : "|"),
               audio->auto_next ? on : off,
               audio->shuffle ? on : off,
               audio->loop ? on : off,
               (pl->sort_mode == PLAYLIST_SORT_NAME) ? "Name" :
               (pl->sort_mode == PLAYLIST_SORT_FOLDER) ? "Folder" : "Path");
    }

    ui_hr_mid();

    // Controls
    printf("%s [1] Play/Resume   [2] Pause      [3] Next        [4] Prev\n", (g_use_utf8 ? "┃" : "|"));
    printf("%s [5] Change playlist   [6] Volume(%d%%)   [7] Auto-Next   [8] Playlist menu\n", (g_use_utf8 ? "┃" : "|"), audio->volume);
    printf("%s [9] Shuffle       [L] Loop       [A] -5s         [D] +5s\n", (g_use_utf8 ? "┃" : "|"));
    printf("%s [S] Sort (Path/Name/Folder)                          [0] Exit\n", (g_use_utf8 ? "┃" : "|"));

    ui_hr_bottom(1);
    printf("Choice: ");
    fflush(stdout);
}

int main() {
    
    console_init();
int running = 1;
    AudioEngine audio;
    Playlist pl;
    srand((unsigned int)time(NULL));

    if (audio_init(&audio) != 0) return -1;

    char folder[MAX_PATH];

    // -----------------------------
    // Load config
    // -----------------------------
    int volume = 50;
    int auto_next = 1;
    int shuffle = 0;
    int loop = 0;
    int sort_mode = PLAYLIST_SORT_PATH;
    load_config(&volume, &auto_next, &shuffle, &loop, &sort_mode);

    audio.volume = volume;
    audio.auto_next = auto_next;
    audio.shuffle = shuffle;
    audio.loop = loop;
    pl.sort_mode = sort_mode;

    if (!prompt_for_folder(folder, MAX_PATH, "Choose a folder with MP3 files (folder picker will open)...")) {
        printf("No folder selected.\n");
        return -1;
    }

    if (playlist_load(&pl, folder) <= 0) {
        printf("No MP3 files found.\n");
        audio_uninit(&audio);
        return -1;
    }

    audio.playlist = &pl; // Correctly assign Playlist pointer

    audio_set_volume(&audio, volume);
    char last_song[MAX_PATH] = "";
    int last_auto = audio.auto_next;
    int last_shuffle = audio.shuffle;
    int last_loop = audio.loop;
    int last_sort = pl.sort_mode;
    int last_pos = -1;
    int last_dur = -1;
    render_menu(&pl, &audio, audio_is_paused(&audio), 0, 0);
    while (running) {
        /* Auto-next / loop when a track ends (single-threaded to avoid races). */
        if (audio_poll(&audio) && (audio.auto_next || audio.loop) && pl.count > 0) {
            const char *next_track = pick_next_track(&audio, &pl, 1);
            if (next_track) {
                audio_play(&audio, next_track);
            }
            last_song[0] = '\0';
            last_pos = -1;
            last_dur = -1;
        }

        const char *cur_song = playlist_current(&pl);
        if (!cur_song) cur_song = "(none)";
        int position = audio_get_position(&audio);
        int duration = audio_get_duration(&audio);
        int paused = audio_is_paused(&audio);
        if (strcmp(last_song, cur_song) != 0 || last_auto != audio.auto_next || last_shuffle != audio.shuffle || last_loop != audio.loop || last_sort != pl.sort_mode || last_pos != position || last_dur != duration) {
            strncpy(last_song, cur_song, sizeof(last_song) - 1);
            last_song[sizeof(last_song) - 1] = '\0';
            last_auto = audio.auto_next;
            last_shuffle = audio.shuffle;
            last_loop = audio.loop;
            last_sort = pl.sort_mode;
            last_pos = position;
            last_dur = duration;
            render_menu(&pl, &audio, paused, position, duration);
        }

        if (!_kbhit()) {
            Sleep(200);
            continue;
        }

        int ch = _getch();
        if (ch == 0 || ch == 224) { // ignore special keys
            _getch();
            continue;
        }
        switch (ch) {
            case '1':
                if (audio_is_paused(&audio)) {
                    audio_resume(&audio);
                } else {
                    audio_play(&audio, playlist_current(&pl));
                }
                break;

            case '2':
                audio_pause(&audio);
                break;

            case '3':
                audio_stop(&audio);
                if (pick_next_track(&audio, &pl, 0)) {
                    audio_play(&audio, playlist_current(&pl));
                }
                break;

            case '4':
                audio_stop(&audio);
                if (pick_prev_track(&audio, &pl)) {
                    audio_play(&audio, playlist_current(&pl));
                }
                break;

            case '5': {
                if (!change_playlist_folder(&audio, &pl, folder, MAX_PATH)) {
                    printf("Playlist not changed.\n");
                    Sleep(700);
                }
            } break;

            case '6': {
                printf("Set volume (0-100): ");
                if (scanf("%d%*c", &audio.volume) == 1) {
                    if (audio.volume < 0) audio.volume = 0;
                    if (audio.volume > 100) audio.volume = 100;

                    audio_set_volume(&audio, audio.volume);
                    save_config(audio.volume, audio.auto_next, audio.shuffle, audio.loop, pl.sort_mode);
                    printf("Volume set to: %d\n", audio.volume);
                } else {
                    printf("Invalid input. Please enter a number between 0 and 100.\n");
                    Sleep(1000);
                }
            } break;

            case '7':
                audio.auto_next = !audio.auto_next;
                save_config(audio.volume, audio.auto_next, audio.shuffle, audio.loop, pl.sort_mode);
                printf("Auto-Next is now %s\n", audio.auto_next ? "ON" : "OFF");
                Sleep(600);
                break;

            case '8':
                playlist_menu(&audio, &pl, folder, MAX_PATH);
                break;

            case '9':
                audio.shuffle = !audio.shuffle;
                save_config(audio.volume, audio.auto_next, audio.shuffle, audio.loop, pl.sort_mode);
                printf("Shuffle is now %s\n", audio.shuffle ? "ON" : "OFF");
                Sleep(600);
                break;

            case 'l':
            case 'L':
                audio.loop = !audio.loop;
                save_config(audio.volume, audio.auto_next, audio.shuffle, audio.loop, pl.sort_mode);
                printf("Loop is now %s\n", audio.loop ? "ON" : "OFF");
                Sleep(600);
                break;

            case 'a':
            case 'A':
                audio_seek_relative(&audio, -5);
                break;

            case 'd':
            case 'D':
                audio_seek_relative(&audio, 5);
                break;

            case 's':
            case 'S': {
                pl.sort_mode = (pl.sort_mode + 1) % 3;
                playlist_set_sort(&pl, pl.sort_mode);
                save_config(audio.volume, audio.auto_next, audio.shuffle, audio.loop, pl.sort_mode);
            } break;

            case '0':
                running = 0;
                break;

            default:
                break;
        }

        last_pos = -1; // force redraw after handling input
        render_menu(&pl, &audio, audio_is_paused(&audio), audio_get_position(&audio), audio_get_duration(&audio));
    }

    running = 0;
    playlist_free(&pl);
    audio_uninit(&audio);
    return 0;
}
