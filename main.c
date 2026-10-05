// precision_clock_linux.c
// Precision Clock Linux v1.1.0
// 精密时钟 Linux 版 / Precision Clock for Linux
// Colligatio open-source project
// License: GPL-3.0

#define _GNU_SOURCE

#include <gtk/gtk.h>
#include <cairo.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/file.h>
#include <fcntl.h>
#include <fontconfig/fontconfig.h>

#ifdef HAVE_APPINDICATOR
#include <libappindicator/app-indicator.h>
#endif

#define CANVAS_W       300
#define CANVAS_H       90
#define WINDOW_MARGIN  24

#define NTP_TIMEOUT_MS     1500
#define EVIDENCE_RESYNC_MS 30000
#define NORMAL_RESYNC_MS   60000
#define NTP_TO_UNIX_SEC    2208988800ULL
#define MAX_NTP_SOURCES    4
#define CROSS_VALIDATE_MAX_DIFF_MS 50ULL
#define MAX_ACCEPT_DIFF_MS 60000ULL

#define TICK_NORMAL_MS    500
#define TICK_EVIDENCE_MS  33

enum { COUNTRY_CN, COUNTRY_US_EAST, COUNTRY_US_CENTRAL, COUNTRY_US_MOUNTAIN,
       COUNTRY_US_PACIFIC, COUNTRY_DE, COUNTRY_JP, COUNTRY_UK, COUNTRY_INTL, COUNTRY_COUNT };
enum { LANG_CN, LANG_TW, LANG_EN, LANG_DE, LANG_JP, LANG_COUNT };

typedef enum { MODE_NORMAL, MODE_EVIDENCE } ClockMode;

typedef struct { int country; int language; int winX; int winY; } Config;

typedef struct {
    const char *wndTitle;
    const char *switching;
    const char *syncOnFmt;
    const char *synced;
    const char *unsynced;
    const char *syncFailed;
    const char *sourceConflict;
    const char *singleSourceWarn;
    const char *menuToggle;
    const char *menuResync;
    const char *menuCountry;
    const char *menuLanguage;
    const char *menuDisclaimer;
    const char *menuAbout;
    const char *menuExit;
    const char *disclaimerTitle;
    const char *disclaimerBody;
    const char *aboutBody;
    const char *fontStatus;
} LangPack;

static const LangPack L_CN = {
    "精密时钟", "正在切换精密时钟...", "● 精密时钟｜%s", "已校时", "未校时｜本地",
    "切换失败：时间源不可达", "源间分歧：时间差超过 50ms", "单源可用｜未交叉验证",
    "切换精密时钟", "重新校时", "国家 / 地区", "语言", "免责声明", "关于", "退出",
    "免责声明",
    "本软件仅作通用时间参考，严禁作为任何医疗、航空、金融交易、法律时效、军事指挥等关键系统的唯一或决定性时间源。",
    "精密时钟 / Precision Clock\n版本 1.1.0\n\nCopyright (C) 2026 Colligatio\nLicense: GPL-3.0",
    "Noto Sans CJK SC"
};

static const LangPack L_TW = {
    "精密時鐘", "正在切換精密時鐘...", "● 精密時鐘｜%s", "已校時", "未校時｜本地",
    "切換失敗：時間源不可達", "源間分歧：時間差超過 50ms", "單源可用｜未交叉驗證",
    "切換精密時鐘", "重新校時", "國家 / 地區", "語言", "免責聲明", "關於", "結束",
    "免責聲明",
    "本軟體僅作通用時間參考，嚴禁作為任何醫療、航空、金融交易、法律時效、軍事指揮等關鍵系統的唯一或決定性時間源。",
    "精密時鐘 / Precision Clock\n版本 1.1.0\n\nCopyright (C) 2026 Colligatio\nLicense: GPL-3.0",
    "Noto Sans CJK SC"
};

static const LangPack L_EN = {
    "Precision Clock", "Switching to Precision Clock...", "● Precision Clock | %s", "Synced", "Unsynced | Local",
    "Sync failed: time source unreachable", "Source conflict: time diff over 50ms", "Single source | not cross-validated",
    "Toggle Precision Clock", "Resync", "Country / Region", "Language", "Disclaimer", "About", "Exit",
    "Disclaimer",
    "This software is a general-purpose time reference only.",
    "Precision Clock\nVersion 1.1.0\n\nCopyright (C) 2026 Colligatio\nLicense: GPL-3.0",
    "Noto Sans CJK SC"
};

static const LangPack L_DE = {
    "Präzisionsuhr", "Wechsle zu Präzisionsuhr...", "● Präzisionsuhr | %s", "Synchronisiert", "Nicht synchron | Lokal",
    "Sync fehlgeschlagen: Zeitquelle nicht erreichbar", "Quellenkonflikt: Zeitdifferenz über 50ms", "Einzelquelle | nicht kreuzvalidiert",
    "Präzisionsuhr umschalten", "Neu synchronisieren", "Land / Region", "Sprache", "Haftungsausschluss", "Über", "Beenden",
    "Haftungsausschluss",
    "Diese Software dient nur als allgemeine Zeitreferenz.",
    "Präzisionsuhr\nVersion 1.1.0\n\nCopyright (C) 2026 Colligatio\nLizenz: GPL-3.0",
    "Noto Sans CJK SC"
};

static const LangPack L_JP = {
    "精密時計", "精密時計に切り替え中...", "● 精密時計 | %s", "同期済み", "未同期 | ローカル",
    "同期失敗：時刻ソースに到達できません", "ソース競合：時刻差が50msを超えています", "単一ソース | クロス検証なし",
    "精密時計の切替", "再同期", "国 / 地域", "言語", "免責事項", "バージョン情報", "終了",
    "免責事項",
    "本ソフトウェアは一般的な時刻参照としてのみ提供されます。",
    "精密時計 / Precision Clock\nバージョン 1.1.0\n\nCopyright (C) 2026 Colligatio\nライセンス: GPL-3.0",
    "Noto Sans CJK SC"
};

static GtkWidget *g_window = NULL;
static Config     g_config = { COUNTRY_INTL, LANG_EN, -1, -1 };
static const LangPack *g_lang = &L_EN;
static ClockMode  g_mode = MODE_NORMAL;
static gboolean   g_normalSynced = FALSE;
static gboolean   g_switching = FALSE;
static gboolean   g_showFailMsg = FALSE;
static int        g_hoverBtn = 0;
static gboolean   g_darkTheme = FALSE;
static guint      g_display_timer_id = 0;
static int        g_lock_fd = -1;

static guint64    g_baseTimeMs = 0;
static struct timespec g_baseTs = {0, 0};

static int        g_ntpStatus = -1;
static guint64    g_ntpResult = 0;
static int        g_ntpTask = 0;
static char       g_lastSource[64] = {0};
static guint64    g_lastRttMs = 0;
static volatile int g_ntpPending = 0;
static volatile int g_pendingToggle = 0;

#ifdef HAVE_APPINDICATOR
static AppIndicator *g_indicator = NULL;
static GtkWidget     *g_tray_menu = NULL;
#endif

#define BTN_R 5.0
#define BTN_RED_X    255.0
#define BTN_YELLOW_X 270.0
#define BTN_GREEN_X  285.0
#define BTN_CY       80.0

static gboolean on_draw_tick(gpointer data);
static void toggle_mode(void);

extern unsigned char font_subset_ttf[];
extern unsigned int font_subset_ttf_len;
extern unsigned char precision_clock_png[];
extern unsigned int precision_clock_png_len;

static char g_font_tmp_path[256] = {0};
static char g_icon_tmp_path[256] = {0};

// ---------- 内嵌字体注册 ----------
static void register_embedded_font(void) {
    const char *rt = getenv("XDG_RUNTIME_DIR");
    if (rt && rt[0])
        snprintf(g_font_tmp_path, sizeof(g_font_tmp_path),
                 "%s/precision-clock-font.ttf", rt);
    else
        snprintf(g_font_tmp_path, sizeof(g_font_tmp_path),
                 "/tmp/precision-clock-font-%d.ttf", (int)getpid());

    FILE *f = fopen(g_font_tmp_path, "wb");
    if (!f) { g_font_tmp_path[0] = 0; return; }
    fwrite(font_subset_ttf, 1, font_subset_ttf_len, f);
    fclose(f);

    FcConfig *cfg = FcConfigGetCurrent();
    FcConfigAppFontAddFile(cfg, (const FcChar8 *)g_font_tmp_path);
}

// ---------- 内嵌图标注册 ----------
static void register_embedded_icon(void) {
    const char *rt = getenv("XDG_RUNTIME_DIR");
    if (rt && rt[0])
        snprintf(g_icon_tmp_path, sizeof(g_icon_tmp_path),
                 "%s/precision-clock-icon.png", rt);
    else
        snprintf(g_icon_tmp_path, sizeof(g_icon_tmp_path),
                 "/tmp/precision-clock-icon-%d.png", (int)getpid());

    FILE *f = fopen(g_icon_tmp_path, "wb");
    if (!f) { g_icon_tmp_path[0] = 0; return; }
    fwrite(precision_clock_png, 1, precision_clock_png_len, f);
    fclose(f);
}

// ---------- 单实例锁 ----------
static gboolean acquire_single_instance(void) {
    const char *home = getenv("HOME");
    if (!home) return TRUE;
    char dir[512], path[600];
    snprintf(dir, sizeof(dir), "%s/.config/PrecisionClock", home);
    mkdir(dir, 0755);
    snprintf(path, sizeof(path), "%s/lock", dir);

    g_lock_fd = open(path, O_RDWR | O_CREAT, 0644);
    if (g_lock_fd < 0) return TRUE;

    if (flock(g_lock_fd, LOCK_EX | LOCK_NB) != 0) {
        close(g_lock_fd);
        g_lock_fd = -1;
        return FALSE;
    }
    return TRUE;
}

static void release_single_instance(void) {
    if (g_lock_fd >= 0) {
        flock(g_lock_fd, LOCK_UN);
        close(g_lock_fd);
        g_lock_fd = -1;
    }
}

static void config_path(char *out, size_t cap) {
    const char *home = getenv("HOME");
    if (!home) { snprintf(out, cap, "./precision-clock.ini"); return; }
    snprintf(out, cap, "%s/.config/PrecisionClock", home);
    mkdir(out, 0755);
    snprintf(out + strlen(out), cap - strlen(out), "/config.ini");
}

static void load_config(void) {
    gboolean country_set = FALSE;
    char tzbuf[128] = {0};
    FILE *tzf = fopen("/etc/timezone", "r");
    if (tzf) {
        if (fgets(tzbuf, sizeof(tzbuf), tzf)) {
            size_t len = strlen(tzbuf);
            while (len > 0 && (tzbuf[len-1] == '\n' || tzbuf[len-1] == '\r'))
                tzbuf[--len] = 0;
        }
        fclose(tzf);
    }
    if (tzbuf[0] == 0) {
        const char *tz = getenv("TZ");
        if (tz) snprintf(tzbuf, sizeof(tzbuf), "%s", tz);
    }
    if (strstr(tzbuf, "Shanghai") || strstr(tzbuf, "Chongqing") ||
        strstr(tzbuf, "Urumqi")   || strstr(tzbuf, "Harbin") || strstr(tzbuf, "PRC")) {
        g_config.country = COUNTRY_CN; country_set = TRUE;
    } else if (strstr(tzbuf, "Tokyo")) {
        g_config.country = COUNTRY_JP; country_set = TRUE;
    } else if (strstr(tzbuf, "Berlin")) {
        g_config.country = COUNTRY_DE; country_set = TRUE;
    } else if (strstr(tzbuf, "London")) {
        g_config.country = COUNTRY_UK; country_set = TRUE;
    }

    const char *lang = getenv("LANG");
    if (lang && strncmp(lang, "zh_CN", 5) == 0) { g_config.language = LANG_CN; if (!country_set) g_config.country = COUNTRY_CN; }
    else if (lang && strncmp(lang, "zh_TW", 5) == 0) { g_config.language = LANG_TW; if (!country_set) g_config.country = COUNTRY_CN; }
    else if (lang && strncmp(lang, "de", 2) == 0) { g_config.language = LANG_DE; if (!country_set) g_config.country = COUNTRY_DE; }
    else if (lang && strncmp(lang, "ja", 2) == 0) { g_config.language = LANG_JP; if (!country_set) g_config.country = COUNTRY_JP; }
    else { g_config.language = LANG_EN; if (!country_set) g_config.country = COUNTRY_INTL; }

    char path[512];
    config_path(path, sizeof(path));
    FILE *f = fopen(path, "r");
    if (!f) return;
    char line[128];
    while (fgets(line, sizeof(line), f)) {
        int v;
        if (sscanf(line, "country=%d", &v) == 1 && v >= 0 && v < COUNTRY_COUNT) g_config.country = v;
        else if (sscanf(line, "language=%d", &v) == 1 && v >= 0 && v < LANG_COUNT) g_config.language = v;
        else if (sscanf(line, "winX=%d", &v) == 1) g_config.winX = v;
        else if (sscanf(line, "winY=%d", &v) == 1) g_config.winY = v;
    }
    fclose(f);
}

static void save_config(void) {
    char path[512];
    config_path(path, sizeof(path));
    FILE *f = fopen(path, "w");
    if (!f) return;
    fprintf(f, "country=%d\n", g_config.country);
    fprintf(f, "language=%d\n", g_config.language);
    fprintf(f, "winX=%d\n", g_config.winX);
    fprintf(f, "winY=%d\n", g_config.winY);
    fclose(f);
}

static const LangPack *current_lang(void) {
    switch (g_config.language) {
        case LANG_CN: return &L_CN;
        case LANG_TW: return &L_TW;
        case LANG_EN: return &L_EN;
        case LANG_DE: return &L_DE;
        case LANG_JP: return &L_JP;
        default:      return &L_EN;
    }
}

static guint64 now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    gint64 elapsed_sec = (gint64)ts.tv_sec - (gint64)g_baseTs.tv_sec;
    gint64 elapsed_nsec = (gint64)ts.tv_nsec - (gint64)g_baseTs.tv_nsec;
    if (elapsed_nsec < 0) { elapsed_sec--; elapsed_nsec += 1000000000LL; }
    guint64 elapsed = (guint64)elapsed_sec * 1000ULL + (guint64)elapsed_nsec / 1000000ULL;
    return g_baseTimeMs + elapsed;
}

static void apply_normal_local(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    g_baseTimeMs = (guint64)ts.tv_sec * 1000ULL + ts.tv_nsec / 1000000ULL;
    clock_gettime(CLOCK_MONOTONIC, &g_baseTs);
    g_normalSynced = FALSE;
    g_lastRttMs = 0;
}

static void get_display_time(struct tm *out, int *ms_out) {
    guint64 total = now_ms();
    time_t sec = (time_t)(total / 1000);
    *ms_out = (int)(total % 1000);

    if (g_config.country == COUNTRY_INTL) {
        unsetenv("TZ");
        tzset();
        localtime_r(&sec, out);
        return;
    }

    static const char *tz_ids[COUNTRY_COUNT] = {
        "Asia/Shanghai", "America/New_York", "America/Chicago",
        "America/Denver", "America/Los_Angeles", "Europe/Berlin",
        "Asia/Tokyo", "Europe/London", "UTC"
    };
    setenv("TZ", tz_ids[g_config.country], 1);
    tzset();
    localtime_r(&sec, out);
}

typedef struct {
    char host[128];
    guint64 timeMs;
    guint64 rttMs;
    volatile int ok;
    volatile int done;
} NtpQuery;

static int ntp_get_time_ms(const char *host, guint64 *out, guint64 *out_rtt) {
    int sock = -1;
    struct addrinfo hints = {0}, *res = NULL;
    unsigned char packet[48];
    int ok = -1;
    struct timespec t1, t4;
    struct timeval tv = { NTP_TIMEOUT_MS / 1000, (NTP_TIMEOUT_MS % 1000) * 1000 };

    memset(packet, 0, sizeof(packet));
    packet[0] = 0x1B;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    if (getaddrinfo(host, "123", &hints, &res) != 0) goto cleanup;
    sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0) goto cleanup;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    clock_gettime(CLOCK_MONOTONIC, &t1);
    if (sendto(sock, packet, 48, 0, res->ai_addr, res->ai_addrlen) < 0) goto cleanup;
    if (recvfrom(sock, packet, 48, 0, NULL, NULL) < 48) goto cleanup;
    clock_gettime(CLOCK_MONOTONIC, &t4);

    {
        guint64 sec = ((guint64)packet[40] << 24) | ((guint64)packet[41] << 16)
                    | ((guint64)packet[42] << 8)  |  (guint64)packet[43];
        guint64 fr  = ((guint64)packet[44] << 24) | ((guint64)packet[45] << 16)
                    | ((guint64)packet[46] << 8)  |  (guint64)packet[47];
        guint64 unixSec = sec - NTP_TO_UNIX_SEC;

        gint64 rtt_sec = (gint64)t4.tv_sec - (gint64)t1.tv_sec;
        gint64 rtt_nsec = (gint64)t4.tv_nsec - (gint64)t1.tv_nsec;
        if (rtt_nsec < 0) { rtt_sec--; rtt_nsec += 1000000000LL; }
        guint64 rtt = (guint64)rtt_sec * 1000ULL + (guint64)rtt_nsec / 1000000ULL;

        *out = unixSec * 1000ULL + (fr * 1000ULL) / 4294967296ULL;
        if (out_rtt) *out_rtt = rtt;
        ok = 0;
    }
cleanup:
    if (sock >= 0) close(sock);
    if (res) freeaddrinfo(res);
    return ok;
}

static void *ntp_worker(void *arg) {
    NtpQuery *q = (NtpQuery *)arg;
    guint64 ms = 0, rtt = 0;
    if (ntp_get_time_ms(q->host, &ms, &rtt) == 0 && ms > 0) {
        q->timeMs = ms; q->rttMs = rtt;
        __sync_lock_test_and_set(&q->ok, 1);
    }
    __sync_lock_test_and_set(&q->done, 1);
    return NULL;
}

static int query_ntp_sources(guint64 *out_time, char *out_src, size_t src_cap) {
    NtpQuery *queries = calloc(MAX_NTP_SOURCES, sizeof(NtpQuery));
    if (!queries) return -1;
    int n = 0;
    const char *hosts[COUNTRY_COUNT] = {
        "ntp.ntsc.ac.cn", "time.nist.gov", "time.nist.gov", "time.nist.gov", "time.nist.gov",
        "ptbtime1.ptb.de", "ntp.nict.jp", "ntp1.npl.co.uk", "time.cloudflare.com"
    };
    snprintf(queries[n].host, sizeof(queries[n].host), "%s", hosts[g_config.country]);
    n++;
    const char *fallbacks[3] = { "time.cloudflare.com", "time.google.com", "time.nist.gov" };
    for (int i = 0; i < 3 && n < MAX_NTP_SOURCES; i++) {
        int dup = 0;
        for (int j = 0; j < n; j++)
            if (strcmp(queries[j].host, fallbacks[i]) == 0) { dup = 1; break; }
        if (dup) continue;
        snprintf(queries[n].host, sizeof(queries[n].host), "%s", fallbacks[i]);
        n++;
    }
    pthread_t th[MAX_NTP_SOURCES];
    for (int i = 0; i < n; i++) pthread_create(&th[i], NULL, ntp_worker, &queries[i]);
    for (int i = 0; i < n; i++) pthread_join(th[i], NULL);

    guint64 times[MAX_NTP_SOURCES], rtts[MAX_NTP_SOURCES];
    int cnt = 0;
    for (int i = 0; i < n; i++) {
        if (__sync_fetch_and_add(&queries[i].done, 0) == 1 &&
            __sync_fetch_and_add(&queries[i].ok,   0) == 1 && queries[i].timeMs > 0) {
            times[cnt] = queries[i].timeMs; rtts[cnt] = queries[i].rttMs; cnt++;
        }
    }
    if (cnt == 0) {
        if (out_src && src_cap) out_src[0] = 0;
        g_lastRttMs = 0; free(queries); return -1;
    }
    for (int i = 0; i < cnt - 1; i++)
        for (int j = i + 1; j < cnt; j++)
            if (times[i] > times[j]) {
                guint64 t = times[i]; times[i] = times[j]; times[j] = t;
                guint64 r = rtts[i]; rtts[i] = rtts[j]; rtts[j] = r;
            }
    guint64 maxDiff = times[cnt - 1] - times[0];
    if (cnt >= 2 && maxDiff > CROSS_VALIDATE_MAX_DIFF_MS) {
        if (out_src && src_cap) out_src[0] = 0;
        g_lastRttMs = 0; free(queries); return 2;
    }
    guint64 median = times[cnt / 2];
    *out_time = median;
    guint64 minRtt = rtts[0];
    for (int i = 1; i < cnt; i++) if (rtts[i] < minRtt) minRtt = rtts[i];
    g_lastRttMs = minRtt;

    static const char *country_names[COUNTRY_COUNT][LANG_COUNT] = {
        { "中国","中國","China","China","中国" },
        { "美国东部","美國東部","US Eastern","US Ost","アメリカ東部" },
        { "美国中部","美國中部","US Central","US Zentral","アメリカ中部" },
        { "美国山地","美國山地","US Mountain","US Mountain","アメリカ山地" },
        { "美国太平洋","美國太平洋","US Pacific","US Pazifik","アメリカ太平洋" },
        { "德国","德國","Germany","Deutschland","ドイツ" },
        { "日本","日本","Japan","Japan","日本" },
        { "英国","英國","United Kingdom","Vereinigtes Königreich","イギリス" },
        { "国际","國際","International","International","国際" }
    };
    if (out_src && src_cap)
        snprintf(out_src, src_cap, "%s", country_names[g_config.country][g_config.language]);
    int result = (cnt >= 2) ? 0 : 1;
    free(queries);
    return result;
}

static gboolean ntp_done_idle(gpointer data);

static void *ntp_thread(void *arg) {
    (void)arg;
    guint64 ms = 0;
    char src[64] = {0};
    int status = query_ntp_sources(&ms, src, sizeof(src));
    g_ntpResult = ms; g_ntpStatus = status;
    strncpy(g_lastSource, src, sizeof(g_lastSource) - 1);
    g_lastSource[sizeof(g_lastSource) - 1] = 0;
    __sync_lock_test_and_set(&g_ntpPending, 0);
    g_idle_add((GSourceFunc)ntp_done_idle, NULL);
    return NULL;
}

static gboolean start_ntp_task(int task_type) {
    if (__sync_val_compare_and_swap(&g_ntpPending, 0, 1) != 0) return FALSE;
    g_ntpTask = task_type;
    pthread_t th;
    if (pthread_create(&th, NULL, ntp_thread, NULL) == 0) {
        pthread_detach(th);
        return TRUE;
    } else {
        __sync_lock_test_and_set(&g_ntpPending, 0);
        return FALSE;
    }
}

static void draw_aero_glass(cairo_t *cr, int w, int h, gboolean dark) {
    double r = 14.0;
    cairo_new_path(cr);
    cairo_arc(cr, r, r, r, M_PI, 3 * M_PI / 2);
    cairo_arc(cr, w - r, r, r, 3 * M_PI / 2, 0);
    cairo_arc(cr, w - r, h - r, r, 0, M_PI / 2);
    cairo_arc(cr, r, h - r, r, M_PI / 2, M_PI);
    cairo_close_path(cr);
    cairo_pattern_t *pat = cairo_pattern_create_linear(0, 0, 0, h);
    if (dark) {
        cairo_pattern_add_color_stop_rgba(pat, 0.0, 28/255.0, 32/255.0, 40/255.0, 0.51);
        cairo_pattern_add_color_stop_rgba(pat, 1.0, 8/255.0, 10/255.0, 14/255.0, 0.43);
    } else {
        cairo_pattern_add_color_stop_rgba(pat, 0.0, 1.0, 1.0, 1.0, 0.47);
        cairo_pattern_add_color_stop_rgba(pat, 1.0, 235/255.0, 238/255.0, 245/255.0, 0.35);
    }
    cairo_set_source(cr, pat);
    cairo_fill_preserve(cr);
    cairo_pattern_destroy(pat);
    cairo_set_source_rgba(cr, 1, 1, 1, dark ? 0.20 : 0.31);
    cairo_set_line_width(cr, 1.0);
    cairo_stroke(cr);
}

static void draw_logo(cairo_t *cr, int x, int y) {
    cairo_set_line_width(cr, 1.5);
    cairo_set_source_rgb(cr, 46/255.0, 94/255.0, 140/255.0);
    cairo_rectangle(cr, x + 15, y + 2, 15, 15); cairo_stroke(cr);
    cairo_set_source_rgb(cr, 20/255.0, 20/255.0, 20/255.0);
    cairo_rectangle(cr, x, y + 8, 22, 22); cairo_stroke(cr);
    cairo_set_source_rgb(cr, 201/255.0, 162/255.0, 39/255.0);
    cairo_rectangle(cr, x + 26, y + 12, 22, 22); cairo_stroke(cr);
}

static int hit_test_button(double lx, double ly) {
    struct { double cx; int id; } btns[3] = {
        { BTN_RED_X, 1 }, { BTN_YELLOW_X, 2 }, { BTN_GREEN_X, 3 }
    };
    for (int i = 0; i < 3; i++) {
        double dx = lx - btns[i].cx;
        double dy = ly - BTN_CY;
        if (dx * dx + dy * dy <= BTN_R * BTN_R) return btns[i].id;
    }
    return 0;
}

static void build_status(char *buf, size_t cap) {
    buf[0] = 0;
    if (g_switching) { snprintf(buf, cap, "%s", g_lang->switching); return; }
    if (g_showFailMsg) {
        snprintf(buf, cap, "%s", g_ntpStatus == 2 ? g_lang->sourceConflict : g_lang->syncFailed);
        return;
    }
    if (g_mode == MODE_EVIDENCE) {
        if (g_ntpStatus == 1) { snprintf(buf, cap, "%s", g_lang->singleSourceWarn); return; }
        if (g_ntpStatus == 2) { snprintf(buf, cap, "%s", g_lang->sourceConflict); return; }
        snprintf(buf, cap, g_lang->syncOnFmt, g_lastSource);
        return;
    }
    snprintf(buf, cap, "%s", g_normalSynced ? g_lang->synced : g_lang->unsynced);
}

static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
    (void)widget; (void)data;
    cairo_set_source_rgba(cr, 0, 0, 0, 0);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    draw_aero_glass(cr, CANVAS_W, CANVAS_H, g_darkTheme);
    draw_logo(cr, 18, 20);

    struct tm tm; int ms;
    get_display_time(&tm, &ms);
    char timeBuf[32];
    if (g_mode == MODE_EVIDENCE)
        snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d.%03d", tm.tm_hour, tm.tm_min, tm.tm_sec, ms);
    else
        snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);

    cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 28);
    cairo_set_source_rgba(cr, g_darkTheme ? 1 : 0, g_darkTheme ? 1 : 0, g_darkTheme ? 1 : 0, 1);
    cairo_move_to(cr, 66, 42);
    cairo_show_text(cr, timeBuf);

    char status[160];
    build_status(status, sizeof(status));
    cairo_select_font_face(cr, g_lang->fontStatus, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 12);
    cairo_set_source_rgba(cr, g_darkTheme ? 1 : 0, g_darkTheme ? 1 : 0, g_darkTheme ? 1 : 0, 1);
    cairo_move_to(cr, 66, 68);
    cairo_show_text(cr, status);

    if (g_lastRttMs > 0) {
        char rttBuf[32];
        snprintf(rttBuf, sizeof(rttBuf), "RTT %llums", (unsigned long long)g_lastRttMs);

        cairo_select_font_face(cr, g_lang->fontStatus, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 10);
        cairo_text_extents_t ext;
        cairo_text_extents(cr, rttBuf, &ext);
        double rtt_x = 240.0 - ext.width;
        double rtt_y = BTN_CY + ext.height / 2.0;

        if (g_lastRttMs > 500) cairo_set_source_rgb(cr, 240/255.0, 140/255.0, 60/255.0);
        else cairo_set_source_rgba(cr, g_darkTheme ? 1 : 0, g_darkTheme ? 1 : 0, g_darkTheme ? 1 : 0, 1);

        cairo_move_to(cr, rtt_x, rtt_y);
        cairo_show_text(cr, rttBuf);
    }

    double cy = BTN_CY, r = BTN_R;
    struct { double x; double rr, gg, bb, rh, gh, bh; int id; } btns[3] = {
        { BTN_RED_X,    1.0, 0.37, 0.34, 1.0, 0.51, 0.47, 1 },
        { BTN_YELLOW_X, 1.0, 0.74, 0.18, 1.0, 0.84, 0.35, 2 },
        { BTN_GREEN_X,  0.15, 0.79, 0.25, 0.31, 0.88, 0.39, 3 }
    };
    for (int i = 0; i < 3; i++) {
        if (g_hoverBtn == btns[i].id) cairo_set_source_rgb(cr, btns[i].rh, btns[i].gh, btns[i].bh);
        else cairo_set_source_rgb(cr, btns[i].rr, btns[i].gg, btns[i].bb);
        cairo_arc(cr, btns[i].x, cy, r, 0, 2 * M_PI);
        cairo_fill(cr);
    }
    cairo_set_source_rgba(cr, 40/255.0, 20/255.0, 20/255.0, 200/255.0);
    cairo_set_line_width(cr, 1.2);
    double s = 2.0;
    if (g_hoverBtn == 1) {
        cairo_move_to(cr, BTN_RED_X - s, cy - s); cairo_line_to(cr, BTN_RED_X + s, cy + s);
        cairo_move_to(cr, BTN_RED_X - s, cy + s); cairo_line_to(cr, BTN_RED_X + s, cy - s);
        cairo_stroke(cr);
    } else if (g_hoverBtn == 2) {
        cairo_move_to(cr, BTN_YELLOW_X - s, cy); cairo_line_to(cr, BTN_YELLOW_X + s, cy);
        cairo_stroke(cr);
    } else if (g_hoverBtn == 3) {
        cairo_rectangle(cr, BTN_GREEN_X - s, cy - s, s * 2, s * 2); cairo_stroke(cr);
    }
    return FALSE;
}

static gboolean on_motion(GtkWidget *widget, GdkEventMotion *event, gpointer data) {
    (void)widget; (void)data;
    int btn = hit_test_button(event->x, event->y);
    if (btn != g_hoverBtn) { g_hoverBtn = btn; gtk_widget_queue_draw(g_window); }
    return FALSE;
}

static gboolean on_leave(GtkWidget *widget, GdkEventCrossing *event, gpointer data) {
    (void)widget; (void)event; (void)data;
    if (g_hoverBtn != 0) { g_hoverBtn = 0; gtk_widget_queue_draw(g_window); }
    return FALSE;
}

static const char *COUNTRY_NAMES_L10N[COUNTRY_COUNT][LANG_COUNT] = {
    { "中国","中國","China","China","中国" },
    { "美国东部","美國東部","US Eastern","US Ost","アメリカ東部" },
    { "美国中部","美國中部","US Central","US Zentral","アメリカ中部" },
    { "美国山地","美國山地","US Mountain","US Mountain","アメリカ山地" },
    { "美国太平洋","美國太平洋","US Pacific","US Pazifik","アメリカ太平洋" },
    { "德国","德國","Germany","Deutschland","ドイツ" },
    { "日本","日本","Japan","Japan","日本" },
    { "英国","英國","United Kingdom","Vereinigtes Königreich","イギリス" },
    { "国际","國際","International","International","国際" }
};

static const char *LANG_NAMES[LANG_COUNT] = {
    "简体中文", "繁體中文", "English", "Deutsch", "日本語"
};

static void on_menu_toggle(GtkMenuItem *item, gpointer data) { (void)item; (void)data; toggle_mode(); }
static void on_menu_resync(GtkMenuItem *item, gpointer data) { (void)item; (void)data; start_ntp_task(g_mode == MODE_EVIDENCE ? 2 : 0); }

#ifdef HAVE_APPINDICATOR
static void rebuild_tray_menu(void);
#endif

static void on_menu_country(GtkMenuItem *item, gpointer data) {
    (void)item;
    int idx = GPOINTER_TO_INT(data);
    if (idx == g_config.country) return;
    g_config.country = idx; save_config();
    g_switching = TRUE; gtk_widget_queue_draw(g_window);
    if (!start_ntp_task(g_mode == MODE_EVIDENCE ? 2 : 0)) {
        g_switching = FALSE;
    }
}

static void on_menu_lang(GtkMenuItem *item, gpointer data) {
    (void)item;
    int idx = GPOINTER_TO_INT(data);
    if (idx == g_config.language) return;
    g_config.language = idx; g_lang = current_lang();
    save_config(); gtk_widget_queue_draw(g_window);
#ifdef HAVE_APPINDICATOR
    rebuild_tray_menu();
#endif
}

static void on_menu_disclaimer(GtkMenuItem *item, gpointer data) {
    (void)item; (void)data;
    GtkWidget *dlg = gtk_message_dialog_new(GTK_WINDOW(g_window), GTK_DIALOG_MODAL,
        GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "%s", g_lang->disclaimerBody);
    gtk_window_set_title(GTK_WINDOW(dlg), g_lang->disclaimerTitle);
    gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(dlg);
}

static void on_menu_about(GtkMenuItem *item, gpointer data) {
    (void)item; (void)data;
    GtkWidget *dlg = gtk_message_dialog_new(GTK_WINDOW(g_window), GTK_DIALOG_MODAL,
        GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "%s", g_lang->aboutBody);
    gtk_window_set_title(GTK_WINDOW(dlg), g_lang->menuAbout);
    gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(dlg);
}

static void on_menu_exit(GtkMenuItem *item, gpointer data) { (void)item; (void)data; gtk_main_quit(); }

static GtkWidget *build_context_menu(void) {
    GtkWidget *menu = gtk_menu_new();

    GtkWidget *it_toggle = gtk_menu_item_new_with_label(g_lang->menuToggle);
    GtkWidget *it_resync = gtk_menu_item_new_with_label(g_lang->menuResync);
    GtkWidget *sep1 = gtk_separator_menu_item_new();

    GtkWidget *it_country = gtk_menu_item_new_with_label(g_lang->menuCountry);
    GtkWidget *sub_country = gtk_menu_new();
    GSList *grp_c = NULL;
    for (int i = 0; i < COUNTRY_COUNT; i++) {
        GtkWidget *it = gtk_radio_menu_item_new_with_label(grp_c, COUNTRY_NAMES_L10N[i][g_config.language]);
        grp_c = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(it));
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(it), i == g_config.country);
        g_signal_connect(it, "activate", G_CALLBACK(on_menu_country), GINT_TO_POINTER(i));
        gtk_menu_shell_append(GTK_MENU_SHELL(sub_country), it);
    }
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(it_country), sub_country);

    GtkWidget *it_lang = gtk_menu_item_new_with_label(g_lang->menuLanguage);
    GtkWidget *sub_lang = gtk_menu_new();
    GSList *grp_l = NULL;
    for (int i = 0; i < LANG_COUNT; i++) {
        GtkWidget *it = gtk_radio_menu_item_new_with_label(grp_l, LANG_NAMES[i]);
        grp_l = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(it));
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(it), i == g_config.language);
        g_signal_connect(it, "activate", G_CALLBACK(on_menu_lang), GINT_TO_POINTER(i));
        gtk_menu_shell_append(GTK_MENU_SHELL(sub_lang), it);
    }
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(it_lang), sub_lang);

    GtkWidget *it_disclaimer = gtk_menu_item_new_with_label(g_lang->menuDisclaimer);
    GtkWidget *it_about = gtk_menu_item_new_with_label(g_lang->menuAbout);
    GtkWidget *sep2 = gtk_separator_menu_item_new();
    GtkWidget *it_exit = gtk_menu_item_new_with_label(g_lang->menuExit);

    g_signal_connect(it_toggle, "activate", G_CALLBACK(on_menu_toggle), NULL);
    g_signal_connect(it_resync, "activate", G_CALLBACK(on_menu_resync), NULL);
    g_signal_connect(it_disclaimer, "activate", G_CALLBACK(on_menu_disclaimer), NULL);
    g_signal_connect(it_about, "activate", G_CALLBACK(on_menu_about), NULL);
    g_signal_connect(it_exit, "activate", G_CALLBACK(on_menu_exit), NULL);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), it_toggle);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), it_resync);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), sep1);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), it_country);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), it_lang);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), it_disclaimer);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), it_about);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), sep2);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), it_exit);

    return menu;
}

static void show_context_menu(GdkEventButton *event) {
    GtkWidget *menu = build_context_menu();
    gtk_menu_attach_to_widget(GTK_MENU(menu), g_window, NULL);
    gtk_widget_show_all(menu);
    gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, event->button, event->time);
}

#ifdef HAVE_APPINDICATOR
static void rebuild_tray_menu(void) {
    if (!g_indicator) return;
    GtkWidget *new_menu = build_context_menu();
    gtk_widget_show_all(new_menu);
    app_indicator_set_menu(g_indicator, GTK_MENU(new_menu));
    if (g_tray_menu && GTK_IS_WIDGET(g_tray_menu)) {
    }
    g_tray_menu = new_menu;
}

static void setup_tray(void) {
    g_indicator = app_indicator_new("precision-clock",
                                     "precision-clock",
                                     APP_INDICATOR_CATEGORY_APPLICATION_STATUS);
    if (g_icon_tmp_path[0]) {
        app_indicator_set_icon_full(g_indicator, g_icon_tmp_path, "Precision Clock");
    }
    app_indicator_set_status(g_indicator, APP_INDICATOR_STATUS_ACTIVE);

    g_tray_menu = build_context_menu();
    gtk_widget_show_all(g_tray_menu);
    app_indicator_set_menu(g_indicator, GTK_MENU(g_tray_menu));
}
#endif

static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data) {
    (void)data;
    if (event->button == 3) { show_context_menu(event); return TRUE; }
    if (event->button != 1) return FALSE;
    int btn = hit_test_button(event->x, event->y);
    if (btn == 1) { gtk_main_quit(); return TRUE; }
    if (btn == 2) {
#ifdef HAVE_APPINDICATOR
        gtk_widget_hide(g_window);
#endif
        return TRUE;
    }
    if (btn == 3) { toggle_mode(); return TRUE; }
    GdkWindow *gdk_win = gtk_widget_get_window(widget);
    if (gdk_win)
        gdk_window_begin_move_drag(gdk_win, event->button,
            (int)event->x_root, (int)event->y_root, event->time);
    return TRUE;
}

static gboolean on_draw_tick(gpointer data) {
    (void)data;
    gtk_widget_queue_draw(g_window);
    return TRUE;
}

static void set_display_tick(guint ms) {
    if (g_display_timer_id) g_source_remove(g_display_timer_id);
    g_display_timer_id = g_timeout_add(ms, on_draw_tick, NULL);
}

static void toggle_mode(void) {
    if (g_switching) return;
    if (g_mode == MODE_NORMAL) {
        g_switching = TRUE;
        gtk_widget_queue_draw(g_window);
        if (!start_ntp_task(1)) {
            __sync_lock_test_and_set(&g_pendingToggle, 1);
        }
    } else {
        g_mode = MODE_NORMAL;
        apply_normal_local();
        set_display_tick(TICK_NORMAL_MS);
        start_ntp_task(0);
        gtk_widget_queue_draw(g_window);
    }
}

static gboolean clear_fail_msg(gpointer d) {
    (void)d; g_showFailMsg = FALSE;
    gtk_widget_queue_draw(g_window);
    return FALSE;
}

static void apply_ntp_result(guint64 ms) {
    guint64 current = now_ms();
    gint64 diff = (gint64)ms - (gint64)current;
    if (diff < 0) diff = -diff;
    if (diff > (gint64)MAX_ACCEPT_DIFF_MS) return;
    g_baseTimeMs = ms;
    clock_gettime(CLOCK_MONOTONIC, &g_baseTs);
}

static gboolean ntp_done_idle(gpointer data) {
    (void)data;
    int status = g_ntpStatus;
    guint64 ms = g_ntpResult;
    if (g_ntpTask == 0) {
        if (status == 0 || status == 1) { apply_ntp_result(ms); g_normalSynced = TRUE; }
        g_switching = FALSE;
    } else if (g_ntpTask == 1) {
        if (status == 0 || status == 1) {
            apply_ntp_result(ms);
            g_mode = MODE_EVIDENCE;
            set_display_tick(TICK_EVIDENCE_MS);
        } else {
            g_showFailMsg = TRUE;
            g_timeout_add(2500, clear_fail_msg, NULL);
        }
        g_switching = FALSE;
    } else if (g_ntpTask == 2) {
        if (status == 0 || status == 1) { apply_ntp_result(ms); }
    }
    gtk_widget_queue_draw(g_window);

    if (__sync_val_compare_and_swap(&g_pendingToggle, 1, 0) == 1) {
        g_switching = FALSE;
        toggle_mode();
    }

    return FALSE;
}

static gboolean on_normal_resync_tick(gpointer data) {
    (void)data;
    if (g_mode == MODE_NORMAL) start_ntp_task(0);
    return TRUE;
}

static gboolean on_evidence_resync_tick(gpointer data) {
    (void)data;
    if (g_mode == MODE_EVIDENCE) start_ntp_task(2);
    return TRUE;
}

static void detect_theme(void) {
    GtkSettings *settings = gtk_settings_get_default();
    gchar *theme = NULL;
    g_object_get(settings, "gtk-theme-name", &theme, NULL);
    if (theme) {
        g_darkTheme = (strstr(theme, "dark") != NULL || strstr(theme, "Dark") != NULL);
        g_free(theme);
    }
}

static gboolean save_window_position(gpointer data) {
    (void)data;
    if (g_window) {
        gtk_window_get_position(GTK_WINDOW(g_window), &g_config.winX, &g_config.winY);
        save_config();
    }
    return TRUE;
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    if (!acquire_single_instance()) {
        fprintf(stderr, "Precision Clock is already running.\n");
        return 0;
    }

    register_embedded_font();
    register_embedded_icon();
    load_config();
    g_lang = current_lang();
    detect_theme();

    g_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(g_window), g_lang->wndTitle);
    if (g_icon_tmp_path[0]) {
        gtk_window_set_icon_from_file(GTK_WINDOW(g_window),
            g_icon_tmp_path, NULL);
    }
    gtk_window_set_default_size(GTK_WINDOW(g_window), CANVAS_W, CANVAS_H);
    gtk_window_set_decorated(GTK_WINDOW(g_window), FALSE);
    gtk_window_set_keep_above(GTK_WINDOW(g_window), TRUE);
    gtk_window_set_resizable(GTK_WINDOW(g_window), FALSE);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(g_window), TRUE);
    gtk_window_set_type_hint(GTK_WINDOW(g_window), GDK_WINDOW_TYPE_HINT_UTILITY);

    GdkScreen *screen = gtk_widget_get_screen(g_window);
    GdkVisual *visual = gdk_screen_get_rgba_visual(screen);
    if (visual && gdk_screen_is_composited(screen)) gtk_widget_set_visual(g_window, visual);
    gtk_widget_set_app_paintable(g_window, TRUE);

    int screen_w = gdk_screen_get_width(screen);
    int screen_h = gdk_screen_get_height(screen);
    int wx = (g_config.winX >= 0) ? g_config.winX : screen_w - CANVAS_W - WINDOW_MARGIN;
    int wy = (g_config.winY >= 0) ? g_config.winY : screen_h - CANVAS_H - WINDOW_MARGIN - 60;
    gtk_window_move(GTK_WINDOW(g_window), wx, wy);

    g_signal_connect(g_window, "draw", G_CALLBACK(on_draw), NULL);
    g_signal_connect(g_window, "motion-notify-event", G_CALLBACK(on_motion), NULL);
    g_signal_connect(g_window, "leave-notify-event", G_CALLBACK(on_leave), NULL);
    g_signal_connect(g_window, "button-press-event", G_CALLBACK(on_button_press), NULL);
    g_signal_connect(g_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_add_events(g_window,
        GDK_POINTER_MOTION_MASK | GDK_LEAVE_NOTIFY_MASK |
        GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_ENTER_NOTIFY_MASK);

    gtk_widget_show_all(g_window);

#ifdef HAVE_APPINDICATOR
    setup_tray();
#endif

    g_timeout_add_seconds(2, save_window_position, NULL);

    apply_normal_local();
    start_ntp_task(0);

    set_display_tick(TICK_NORMAL_MS);
    g_timeout_add(NORMAL_RESYNC_MS, on_normal_resync_tick, NULL);
    g_timeout_add(EVIDENCE_RESYNC_MS, on_evidence_resync_tick, NULL);

    gtk_main();
    release_single_instance();
    return 0;
}