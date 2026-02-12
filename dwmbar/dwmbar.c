/* dwm statusbar in C — status2d colors, nerd font icons, zero forks
 * deps: libX11, libpulse
 * build: gcc -O2 -o dwmbar dwmbar.c -lX11 -lpulse
 * usage: add dwmbar & to ~/.xinitrc
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <pulse/pulseaudio.h>

/* ── colors ── */
#define FG       "#9ea5b8"
#define COL_CPU  "#e06c75"
#define COL_MEM  "#c678dd"
#define COL_BRI  "#e5c07b"
#define COL_VOL  "#61afef"
#define COL_WIFI "#56b6c2"
#define COL_BAT  "#98c379"
#define COL_KB   "#d19a66"
#define COL_DATE "#5570c9"
#define COL_WARN "#e06c75"
#define COL_SEP  "#3a3f55"

/* ── nerd font icons (UTF-8, double space — glyph eats one) ── */
#define ICO_CPU    "\xef\x92\xbc  "
#define ICO_MEM    "\xee\x89\xa6  "
#define ICO_BRIGHT "\xef\x83\xab  "
#define ICO_VOL_HI "\xef\x80\xa8  "
#define ICO_VOL_MD "\xef\x80\xa7  "
#define ICO_VOL_LO "\xef\x80\xa6  "
#define ICO_VOL_MT "\xef\x91\xa6  "
#define ICO_WIFI   "\xef\x87\xab  "
#define ICO_NONET  "\xef\x91\xa7  "
#define ICO_CHRG   "\xef\x83\xa7  "
#define ICO_BAT100 "\xef\x89\x80  "
#define ICO_BAT80  "\xef\x89\x81  "
#define ICO_BAT60  "\xef\x89\x82  "
#define ICO_BAT40  "\xef\x89\x83  "
#define ICO_BAT20  "\xef\x89\x84  "
#define ICO_KB     "\xef\x84\x9c  "
#define ICO_DATE   "\xef\x81\xb3  "

#define SEP  " ^c" COL_SEP "^┊^d^ "
#define BUFLEN 512

static Display *dpy;
static int nproc;
static char bat_path[512];
static int bright_max;
static char bright_path[512];

/* ── pulseaudio state ── */
static int pa_vol = -1;
static int pa_mute = 0;
static int pa_ready = 0;
static pa_context *pa_ctx;
static pa_mainloop *pa_ml;

/* ── helpers ── */
static int read_int(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    int v = 0;
    if (fscanf(f, "%d", &v) != 1) v = -1;
    fclose(f);
    return v;
}

static int read_str(const char *path, char *buf, size_t len) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    if (!fgets(buf, len, f)) { fclose(f); return -1; }
    fclose(f);
    char *nl = strchr(buf, '\n');
    if (nl) *nl = '\0';
    return 0;
}

static void find_battery(void) {
    DIR *d = opendir("/sys/class/power_supply");
    struct dirent *e;
    bat_path[0] = '\0';
    if (!d) return;
    while ((e = readdir(d))) {
        if (strncmp(e->d_name, "BAT", 3) == 0) {
            snprintf(bat_path, sizeof(bat_path),
                     "/sys/class/power_supply/%s", e->d_name);
            break;
        }
    }
    closedir(d);
}

static void find_backlight(void) {
    bright_max = 0;
    bright_path[0] = '\0';
    DIR *d = opendir("/sys/class/backlight");
    struct dirent *e;
    if (!d) return;
    while ((e = readdir(d))) {
        if (e->d_name[0] == '.') continue;
        char p[512];
        snprintf(p, sizeof(p), "/sys/class/backlight/%s/max_brightness", e->d_name);
        int v = read_int(p);
        if (v > 0) {
            bright_max = v;
            snprintf(bright_path, sizeof(bright_path),
                     "/sys/class/backlight/%s/brightness", e->d_name);
            break;
        }
    }
    closedir(d);
}

/* ── pulseaudio callbacks ── */
static void pa_sink_info_cb(pa_context *c, const pa_sink_info *i, int eol, void *ud) {
    (void)c; (void)ud;
    if (eol > 0 || !i) return;
    pa_vol = (int)((pa_cvolume_avg(&i->volume) * 100 + PA_VOLUME_NORM / 2) / PA_VOLUME_NORM);
    pa_mute = i->mute;
}

static void pa_server_info_cb(pa_context *c, const pa_server_info *i, void *ud) {
    (void)ud;
    if (!i || !i->default_sink_name) return;
    pa_context_get_sink_info_by_name(c, i->default_sink_name, pa_sink_info_cb, NULL);
}

static void pa_subscribe_cb(pa_context *c, pa_subscription_event_type_t t, uint32_t idx, void *ud) {
    (void)idx; (void)ud;
    if ((t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) == PA_SUBSCRIPTION_EVENT_SINK)
        pa_context_get_server_info(c, pa_server_info_cb, NULL);
}

static void pa_state_cb(pa_context *c, void *ud) {
    (void)ud;
    switch (pa_context_get_state(c)) {
    case PA_CONTEXT_READY:
        pa_ready = 1;
        pa_context_set_subscribe_callback(c, pa_subscribe_cb, NULL);
        pa_context_subscribe(c, PA_SUBSCRIPTION_MASK_SINK, NULL, NULL);
        pa_context_get_server_info(c, pa_server_info_cb, NULL);
        break;
    case PA_CONTEXT_FAILED:
    case PA_CONTEXT_TERMINATED:
        pa_ready = 0;
        break;
    default:
        break;
    }
}

static void pa_init(void) {
    pa_ml = pa_mainloop_new();
    pa_mainloop_api *api = pa_mainloop_get_api(pa_ml);
    pa_ctx = pa_context_new(api, "dwmbar");
    pa_context_set_state_callback(pa_ctx, pa_state_cb, NULL);
    pa_context_connect(pa_ctx, NULL, PA_CONTEXT_NOFLAGS, NULL);
}

static void pa_poll(void) {
    /* non-blocking: process pending events */
    int ret;
    while (pa_mainloop_iterate(pa_ml, 0, &ret) > 0)
        ;
}

/* ── modules ── */
static void get_cpu(char *buf, size_t len) {
    FILE *f = fopen("/proc/loadavg", "r");
    if (!f) { buf[0] = '\0'; return; }
    float load;
    if (fscanf(f, "%f", &load) != 1) load = 0;
    fclose(f);
    int pct = (int)(load * 100.0f / nproc);
    const char *col = (pct >= 80) ? COL_WARN : COL_CPU;
    snprintf(buf, len, "^c%s^" ICO_CPU "^c" FG "^%d%%^d^", col, pct);
}

static void get_mem(char *buf, size_t len) {
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) { buf[0] = '\0'; return; }
    long total = 0, avail = 0;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        sscanf(line, "MemTotal: %ld kB", &total);
        if (sscanf(line, "MemAvailable: %ld kB", &avail) == 1 && total > 0)
            break;
    }
    fclose(f);
    float used = (total - avail) / 1048576.0f;
    float tot  = total / 1048576.0f;
    snprintf(buf, len, "^c" COL_MEM "^" ICO_MEM "^c" FG "^%.1f/%.0fG^d^",
             used, tot);
}

static void get_brightness(char *buf, size_t len) {
    buf[0] = '\0';
    if (bright_max <= 0 || !bright_path[0]) return;
    int cur = read_int(bright_path);
    if (cur >= 0)
        snprintf(buf, len, "^c" COL_BRI "^" ICO_BRIGHT "^c" FG "^%d%%^d^",
                 cur * 100 / bright_max);
}

static void get_volume(char *buf, size_t len) {
    pa_poll();
    if (pa_vol < 0) { buf[0] = '\0'; return; }

    if (pa_mute) {
        snprintf(buf, len, "^c" COL_WARN "^" ICO_VOL_MT "^c" FG "^mute^d^");
    } else {
        const char *ico = (pa_vol >= 60) ? ICO_VOL_HI :
                          (pa_vol >= 30) ? ICO_VOL_MD : ICO_VOL_LO;
        snprintf(buf, len, "^c" COL_VOL "^%s^c" FG "^%d%%^d^", ico, pa_vol);
    }
}

static void get_wifi(char *buf, size_t len) {
    FILE *f = fopen("/proc/net/wireless", "r");
    if (!f) {
        snprintf(buf, len, "^c" COL_WARN "^" ICO_NONET "^c" FG "^--^d^");
        return;
    }
    char line[256];
    int n = 0;
    float quality = 0;
    while (fgets(line, sizeof(line), f)) {
        if (++n == 3) {
            char *p = strchr(line, ':');
            if (p) {
                int status;
                sscanf(p + 1, "%d %f", &status, &quality);
            }
        }
    }
    fclose(f);
    int pct = (int)(quality * 10.0f / 7.0f);
    if (pct > 0) {
        const char *col = (pct <= 25) ? COL_WARN : COL_WIFI;
        snprintf(buf, len, "^c%s^" ICO_WIFI "^c" FG "^%d%%^d^", col, pct);
    } else {
        snprintf(buf, len, "^c" COL_WARN "^" ICO_NONET "^c" FG "^--^d^");
    }
}

static void get_battery(char *buf, size_t len) {
    buf[0] = '\0';
    if (!bat_path[0]) return;
    char p[512], status[32];
    snprintf(p, sizeof(p), "%s/capacity", bat_path);
    int cap = read_int(p);
    if (cap < 0) return;
    snprintf(p, sizeof(p), "%s/status", bat_path);
    read_str(p, status, sizeof(status));

    const char *ico, *col;
    if (strcmp(status, "Charging") == 0) {
        ico = ICO_CHRG; col = COL_BAT;
    } else if (cap >= 80) {
        ico = ICO_BAT100; col = COL_BAT;
    } else if (cap >= 60) {
        ico = ICO_BAT80; col = COL_BAT;
    } else if (cap >= 40) {
        ico = ICO_BAT60; col = COL_BRI;
    } else if (cap >= 20) {
        ico = ICO_BAT40; col = COL_BRI;
    } else {
        ico = ICO_BAT20; col = COL_WARN;
    }
    snprintf(buf, len, "^c%s^%s^c" FG "^%d%%^d^", col, ico, cap);
}

static void get_layout(char *buf, size_t len) {
    XkbStateRec state;
    XkbGetState(dpy, XkbUseCoreKbd, &state);
    int group = state.group;

    /* только state — без XkbGetKeyboard каждый тик */
    const char *txt;
    switch (group) {
    case 0: txt = "EN"; break;
    case 1: txt = "RU"; break;
    default: txt = "?"; break;
    }
    snprintf(buf, len, "^c" COL_KB "^" ICO_KB "^c" FG "^%s^d^", txt);
}

static void get_date(char *buf, size_t len) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char timebuf[64];
    strftime(timebuf, sizeof(timebuf), "%a %d %b %H:%M:%S", tm);
    snprintf(buf, len, "^c" COL_DATE "^" ICO_DATE "^c" FG "^%s^d^", timebuf);
}

int main(void) {
    dpy = XOpenDisplay(NULL);
    if (!dpy) { fprintf(stderr, "dwmbar: cannot open display\n"); return 1; }

    nproc = sysconf(_SC_NPROCESSORS_ONLN);
    if (nproc < 1) nproc = 1;
    find_battery();
    find_backlight();
    pa_init();

    char cpu[BUFLEN], mem[BUFLEN], bri[BUFLEN], vol[BUFLEN];
    char wifi[BUFLEN], bat[BUFLEN], kb[BUFLEN], dt[BUFLEN];
    char bar[4096];
    int tick = 0;

    for (;;) {
        /* slow: every 5s (tick % 25 at 200ms sleep) */
        if (tick % 25 == 0) {
            get_cpu(cpu, sizeof(cpu));
            get_mem(mem, sizeof(mem));
            get_wifi(wifi, sizeof(wifi));
            get_battery(bat, sizeof(bat));
            get_brightness(bri, sizeof(bri));
        }

        /* fast: every 200ms */
        get_volume(vol, sizeof(vol));
        get_layout(kb, sizeof(kb));
        get_date(dt, sizeof(dt));

        /* assemble */
        char *p = bar;
        char *end = bar + sizeof(bar);
        p += snprintf(p, end - p, " %s" SEP "%s", cpu, mem);
        if (bri[0]) p += snprintf(p, end - p, SEP "%s", bri);
        if (vol[0]) p += snprintf(p, end - p, SEP "%s", vol);
        p += snprintf(p, end - p, SEP "%s", wifi);
        if (bat[0]) p += snprintf(p, end - p, SEP "%s", bat);
        snprintf(p, end - p, SEP "%s" SEP "%s ", kb, dt);

        XStoreName(dpy, DefaultRootWindow(dpy), bar);
        XFlush(dpy);

        tick++;
        usleep(200000);
    }

    pa_context_disconnect(pa_ctx);
    pa_context_unref(pa_ctx);
    pa_mainloop_free(pa_ml);
    XCloseDisplay(dpy);
    return 0;
}
