#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <windows.h>
#include <conio.h>
#include <shobjidl.h>
#include <objbase.h>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "uuid.lib")
#pragma comment(lib, "shell32.lib")

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#define CLAMP(v,a,b) ((v)<(a)?(a):((v)>(b)?(b):(v)))

// settings
static int g_vol = 50;
static int g_autonext = 1;
static int g_shuffle = 0;
static int g_loop = 0;
static int g_sort = 0; // 0 path, 1 name, 2 folder

// playlist
static char **g_pl = NULL;
static int g_pl_n = 0;
static int g_i = 0;
static char g_folder[MAX_PATH];

// audio
static ma_engine g_eng;
static ma_sound  g_snd;
static int g_snd_ok = 0;
static int g_paused = 0;
static ma_uint64 g_pause_pos = 0;

static void cls(void){ system("cls"); }

static const char* bname(const char *p) {
    const char *a = strrchr(p, '\\');
    const char *b = strrchr(p, '/');
    const char *s = (a > b) ? a : b;
    return s ? s + 1 : p;
}

static void mmss(int sec, char out[6]){
    if (sec < 0) sec = 0;
    snprintf(out, 6, "%02d:%02d", sec/60, sec%60);
}

// config next to exe
static void cfg_file(char out[MAX_PATH]){
    char exe[MAX_PATH] = {0};
    DWORD n = GetModuleFileNameA(NULL, exe, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) { strcpy(out, "config.txt"); return; }
    char *a = strrchr(exe, '\\');
    char *b = strrchr(exe, '/');
    char *s = (a > b) ? a : b;
    if (s) *(s+1) = 0;
    snprintf(out, MAX_PATH, "%sconfig.txt", exe);
}

static void cfg(int save){
    char p[MAX_PATH]; cfg_file(p);
    FILE *f = fopen(p, save ? "w" : "r");
    if (!f) return;

    if (!save) {
        int v=50, an=1, sh=0, lp=0, so=0;
        int r = fscanf(f, "%d %d %d %d %d", &v, &an, &sh, &lp, &so);
        if (r >= 1) g_vol = CLAMP(v,0,100);
        if (r >= 2) g_autonext = (an!=0);
        if (r >= 3) g_shuffle = (sh!=0);
        if (r >= 4) g_loop = (lp!=0);
        if (r >= 5) g_sort = (so<0||so>2)?0:so;
    } else {
        fprintf(f, "%d %d %d %d %d", g_vol, g_autonext, g_shuffle, g_loop, g_sort);
    }

    fclose(f);
}

static int is_mp3(const char *n){
    const char *d = strrchr(n, '.');
    return d && (_stricmp(d, ".mp3") == 0);
}

static void pl_free(void){
    for (int i=0;i<g_pl_n;i++) free(g_pl[i]);
    free(g_pl);
    g_pl = NULL; g_pl_n = 0; g_i = 0;
}

static void pl_add(const char *path){
    char **t = (char**)realloc(g_pl, sizeof(char*)*(g_pl_n+1));
    if (!t) return;
    g_pl = t;
    g_pl[g_pl_n] = (char*)malloc(MAX_PATH);
    if (!g_pl[g_pl_n]) return;
    strncpy(g_pl[g_pl_n], path, MAX_PATH-1);
    g_pl[g_pl_n][MAX_PATH-1] = 0;
    g_pl_n++;
}

static void pl_scan(const char *folder){
    WIN32_FIND_DATAA fd;
    char mask[MAX_PATH];
    snprintf(mask, MAX_PATH, "%s\\*", folder);

    HANDLE h = FindFirstFileA(mask, &fd);
    if (h == INVALID_HANDLE_VALUE) return;

    do {
        const char *n = fd.cFileName;
        if (!strcmp(n,".") || !strcmp(n,"..")) continue;

        char full[MAX_PATH];
        snprintf(full, MAX_PATH, "%s\\%s", folder, n);

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) pl_scan(full);
        else if (is_mp3(n)) pl_add(full);

    } while (FindNextFileA(h, &fd));

    FindClose(h);
}

static void folder_part(const char *p, char out[MAX_PATH]){
    const char *a = strrchr(p,'\\');
    const char *b = strrchr(p,'/');
    const char *s = (a > b) ? a : b;
    size_t len = s ? (size_t)(s - p) : 0;
    if (len >= MAX_PATH) len = MAX_PATH-1;
    memcpy(out, p, len);
    out[len] = 0;
}

static int pl_cmp(const void *A, const void *B){
    const char *a = *(const char**)A;
    const char *b = *(const char**)B;

    if (g_sort == 1) {
        int x = _stricmp(bname(a), bname(b));
        return x ? x : _stricmp(a,b);
    }
    if (g_sort == 2) {
        char fa[MAX_PATH], fb[MAX_PATH];
        folder_part(a, fa); folder_part(b, fb);
        int x = _stricmp(fa, fb);
        if (x) return x;
        x = _stricmp(bname(a), bname(b));
        return x ? x : _stricmp(a,b);
    }

    return _stricmp(a, b);
}

static void pl_sort_keep(void){
    if (g_pl_n <= 1) return;

    char keep[MAX_PATH] = {0};
    if (g_i >= 0 && g_i < g_pl_n) strncpy(keep, g_pl[g_i], MAX_PATH-1);

    qsort(g_pl, g_pl_n, sizeof(char*), pl_cmp);

    if (keep[0]) {
        for (int i=0;i<g_pl_n;i++) {
            if (_stricmp(g_pl[i], keep) == 0) { g_i = i; break; }
        }
    }
}

static int pl_load(const char *folder){
    pl_free();
    pl_scan(folder);
    if (g_pl_n <= 0) return 0;
    pl_sort_keep();
    return 1;
}

static const char* pl_cur(void){
    if (g_pl_n <= 0) return NULL;
    g_i = CLAMP(g_i, 0, g_pl_n-1);
    return g_pl[g_i];
}

static void next_track(int respect_loop){
    if (g_pl_n <= 0) return;
    if (respect_loop && g_loop) return;

    if (g_shuffle && g_pl_n > 1) {
        int n = g_i;
        while (n == g_i) n = rand() % g_pl_n;
        g_i = n;
    } else {
        g_i = (g_i + 1) % g_pl_n;
    }
}

static void prev_track(void){
    if (g_pl_n <= 0) return;
    if (g_shuffle && g_pl_n > 1) g_i = rand() % g_pl_n;
    else g_i = (g_i - 1 + g_pl_n) % g_pl_n;
}

// audio
static void vol_set(int v){
    g_vol = CLAMP(v,0,100);
    if (g_snd_ok) ma_sound_set_volume(&g_snd, g_vol / 100.0f);
}

static void stop_play(void){
    if (!g_snd_ok) return;
    ma_sound_stop(&g_snd);
    ma_sound_uninit(&g_snd);
    g_snd_ok = 0;
    g_paused = 0;
    g_pause_pos = 0;
}

static void play_file(const char *f){
    if (!f) return;
    stop_play();

    if (ma_sound_init_from_file(&g_eng, f, MA_SOUND_FLAG_STREAM, NULL, NULL, &g_snd) != MA_SUCCESS) {
        printf("cant play: %s\n", f);
        Sleep(900);
        return;
    }

    g_snd_ok = 1;
    g_paused = 0;
    g_pause_pos = 0;
    vol_set(g_vol);
    ma_sound_start(&g_snd);
}

static void pause_play(void){
    if (!g_snd_ok || g_paused) return;
    ma_sound_get_cursor_in_pcm_frames(&g_snd, &g_pause_pos);
    ma_sound_stop(&g_snd);
    ma_sound_seek_to_pcm_frame(&g_snd, g_pause_pos);
    g_paused = 1;
}

static void resume_play(void){
    if (!g_snd_ok || !g_paused) return;
    ma_sound_seek_to_pcm_frame(&g_snd, g_pause_pos);
    vol_set(g_vol);
    ma_sound_start(&g_snd);
    g_paused = 0;
}

static int pos_sec(void){
    if (!g_snd_ok) return 0;
    ma_uint64 cur=0;
    if (g_paused) cur = g_pause_pos;
    else ma_sound_get_cursor_in_pcm_frames(&g_snd, &cur);
    return (int)(cur / g_eng.sampleRate);
}

static int len_sec(void){
    if (!g_snd_ok) return 0;
    ma_uint64 len=0;
    ma_sound_get_length_in_pcm_frames(&g_snd, &len);
    return (int)(len / g_eng.sampleRate);
}

static void seek_rel(int ds){
    if (!g_snd_ok) return;

    ma_uint64 len=0, cur=0;
    ma_sound_get_length_in_pcm_frames(&g_snd, &len);
    if (g_paused) cur = g_pause_pos;
    else ma_sound_get_cursor_in_pcm_frames(&g_snd, &cur);

    long long t = (long long)cur + (long long)ds * (long long)g_eng.sampleRate;
    if (t < 0) t = 0;
    if (t > (long long)len) t = (long long)len;

    ma_sound_seek_to_pcm_frame(&g_snd, (ma_uint64)t);
    if (g_paused) g_pause_pos = (ma_uint64)t;
    else ma_sound_start(&g_snd);
}

static int ended(void){
    if (!g_snd_ok || g_paused) return 0;

    ma_uint64 cur=0, len=0;
    ma_sound_get_cursor_in_pcm_frames(&g_snd, &cur);
    ma_sound_get_length_in_pcm_frames(&g_snd, &len);

    if ((len && cur >= len) || !ma_sound_is_playing(&g_snd)) { stop_play(); return 1; }
    return 0;
}

// folder picker
static int pick_folder(char out[MAX_PATH]){
    out[0]=0;

    HRESULT hrI = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    int did = SUCCEEDED(hrI);
    if (FAILED(hrI) && hrI != RPC_E_CHANGED_MODE) return 0;

    IFileDialog *dlg = NULL;
    HRESULT hr = CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, &IID_IFileDialog, (void**)&dlg);
    if (SUCCEEDED(hr) && dlg) {
        DWORD opt=0;
        dlg->lpVtbl->GetOptions(dlg,&opt);
        dlg->lpVtbl->SetOptions(dlg, opt | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
        dlg->lpVtbl->SetTitle(dlg, L"Select folder with mp3 files");

        if (SUCCEEDED(dlg->lpVtbl->Show(dlg, GetConsoleWindow()))) {
            IShellItem *it=NULL;
            if (SUCCEEDED(dlg->lpVtbl->GetResult(dlg,&it)) && it) {
                PWSTR w=NULL;
                if (SUCCEEDED(it->lpVtbl->GetDisplayName(it, SIGDN_FILESYSPATH, &w)) && w) {
                    WideCharToMultiByte(CP_ACP,0,w,-1,out,MAX_PATH,NULL,NULL);
                    CoTaskMemFree(w);
                }
                it->lpVtbl->Release(it);
            }
        }
        dlg->lpVtbl->Release(dlg);
    }

    if (did) CoUninitialize();
    return out[0]!=0;
}

static int ask_folder(char out[MAX_PATH]){
    printf("Choose folder (Explorer dialog will open)...\n");
    if (pick_folder(out)) return 1;

    printf("Type folder path: ");
    fflush(stdout);
    if (!fgets(out, MAX_PATH, stdin)) return 0;

    size_t n = strlen(out);
    while (n && (out[n-1]=='\n' || out[n-1]=='\r')) out[--n]=0;
    return out[0]!=0;
}

// ui
static void status(void){
    const char *c = pl_cur();
    if (!c) c = "(none)";

    int p = pos_sec();
    int l = len_sec();

    char a[6], b[6];
    mmss(p,a); mmss(l,b);

    int pct = (l>0) ? (int)((p*100.0)/(double)l + 0.5) : 0;
    pct = CLAMP(pct,0,100);

    int w=30, fill=(pct*w)/100;

    printf("MP3 PLAYER\n");
    printf("Folder: %s\n", g_folder[0]?g_folder:"(none)");
    printf("Track: %d/%d  %s %s\n", (g_pl_n?g_i+1:0), g_pl_n, bname(c), g_paused?"[PAUSED]":"");
    printf("Time: %s / %s  [", a, b);
    for (int i=0;i<w;i++) putchar(i<fill?'#':'-');
    printf("] %d%%\n", pct);

    printf("Vol:%d  AutoNext:%d  Shuffle:%d  Loop:%d  Sort:%s\n",
        g_vol, g_autonext, g_shuffle, g_loop,
        (g_sort==1?"Name":(g_sort==2?"Folder":"Path")));

    printf("\nKeys:\n");
    printf(" 1 Play/Resume  2 Pause  3 Next  4 Prev\n");
    printf(" 5 Change folder 6 Volume 7 AutoNext 8 Playlist\n");
    printf(" 9 Shuffle  L Loop  A -5s  D +5s  S Sort  0 Exit\n");
}

static void playlist_menu(void){
    cls();
    if (g_pl_n<=0) { printf("Playlist empty.\nPress ENTER..."); getchar(); return; }

    printf("PLAYLIST (%d tracks)\n\n", g_pl_n);
    for (int i=0;i<g_pl_n;i++) printf("%c %3d. %s\n", (i==g_i?'>':' '), i+1, bname(g_pl[i]));

    printf("\nType track number (0=back): ");
    char line[64];
    if (!fgets(line,sizeof(line),stdin)) return;
    int n = atoi(line);
    if (n<=0 || n>g_pl_n) return;

    g_i = n-1;
    play_file(pl_cur());
}

int main(void){
    srand((unsigned)time(NULL));
    cfg(0);

    if (ma_engine_init(NULL, &g_eng) != MA_SUCCESS) {
        printf("audio init failed\n");
        return 1;
    }

    vol_set(g_vol);

    if (!ask_folder(g_folder)) { printf("no folder\n"); ma_engine_uninit(&g_eng); return 1; }
    if (!pl_load(g_folder)) { printf("no mp3 files\n"); ma_engine_uninit(&g_eng); return 1; }

    play_file(pl_cur());

    int lp=-999,ll=-999,li=-999,lpa=-999;
    int run=1;

    while (run) {
        if (ended() && (g_autonext || g_loop) && g_pl_n>0) {
            next_track(1);
            play_file(pl_cur());
        }

        int p=pos_sec(), l=len_sec();
        if (p!=lp || l!=ll || g_i!=li || g_paused!=lpa) {
            cls();
            status();
            lp=p; ll=l; li=g_i; lpa=g_paused;
        }

        if (!_kbhit()) { Sleep(200); continue; }

        int ch=_getch();
        if (ch==0 || ch==224) { _getch(); continue; }

        if (ch=='0') run=0;
        else if (ch=='1') { if (g_paused) resume_play(); else play_file(pl_cur()); }
        else if (ch=='2') pause_play();
        else if (ch=='3') { stop_play(); next_track(0); play_file(pl_cur()); }
        else if (ch=='4') { stop_play(); prev_track(); play_file(pl_cur()); }
        else if (ch=='5') {
            char nf[MAX_PATH]={0};
            cls();
            if (ask_folder(nf)) {
                if (pl_load(nf)) { strcpy(g_folder,nf); play_file(pl_cur()); }
                else { printf("no mp3 in that folder\n"); Sleep(900); }
            }
        }
        else if (ch=='6') {
            cls();
            printf("Volume 0-100: ");
            char line[64];
            if (fgets(line,sizeof(line),stdin)) { vol_set(atoi(line)); cfg(1); }
        }
        else if (ch=='7') { g_autonext=!g_autonext; cfg(1); }
        else if (ch=='8') playlist_menu();
        else if (ch=='9') { g_shuffle=!g_shuffle; cfg(1); }
        else if (ch=='l' || ch=='L') { g_loop=!g_loop; cfg(1); }
        else if (ch=='a' || ch=='A') seek_rel(-5);
        else if (ch=='d' || ch=='D') seek_rel(5);
        else if (ch=='s' || ch=='S') { g_sort=(g_sort+1)%3; pl_sort_keep(); cfg(1); }

        lp=-999; // force redraw
    }

    stop_play();
    ma_engine_uninit(&g_eng);
    pl_free();
    return 0;
}
