// precision_clock.cpp
// Precision Clock v2.8.0-rc1
// 精密时钟 / Precision Clock
// Colligatio open-source project
// Compile:
//   windres app.rc -o app_res.o
//   g++ -O2 -s -static -static-libgcc -static-libstdc++ -mwindows main.cpp app_res.o -o PrecisionClock.exe -lws2_32 -luser32 -lgdi32 -lgdiplus -lole32 -lshell32

#define _WIN32_WINNT 0x0601
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <objidl.h>
#include <gdiplus.h>
#include <shellapi.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

using namespace Gdiplus;

// ---------------- 常量 ----------------
#define NTP_TIMEOUT_MS        1500
#define EVIDENCE_RESYNC_MS    30000
#define NTP_TO_UNIX_SEC       2208988800ULL
#define FILETIME_TO_UNIX      11644473600000ULL

#define NORMAL_RESYNC_MS      60000
#define NORMAL_MAX_FAIL       3

#define CANVAS_W   300
#define CANVAS_H   90
#define WINDOW_MARGIN 24

#define TIMER_DISPLAY       1
#define TIMER_RESYNC        2
#define TIMER_FAILMSG       3
#define TIMER_NORMAL_RESYNC 4

#define WM_NTP_DONE   (WM_APP + 1)
#define WM_TRAYICON   (WM_APP + 2)

#define TRAY_ID 1

#define IDM_TOGGLE       1001
#define IDM_RESYNC       1002
#define IDM_DISCLAIMER   1003
#define IDM_EXIT         1004
#define IDM_ABOUT        1005
#define IDM_COUNTRY_BASE 1100
#define IDM_LANG_BASE    1200

#define MAX_NTP_SOURCES 4
#define CROSS_VALIDATE_MAX_DIFF_MS 50ULL

// 红绿灯
#define BTN_R 5.0f
#define BTN_RED_X    255.0f
#define BTN_YELLOW_X 270.0f
#define BTN_GREEN_X  285.0f
#define BTN_CY       80.0f

enum {
    COUNTRY_CN = 0,
    COUNTRY_US_EAST,
    COUNTRY_US_CENTRAL,
    COUNTRY_US_MOUNTAIN,
    COUNTRY_US_PACIFIC,
    COUNTRY_DE,
    COUNTRY_JP,
    COUNTRY_UK,
    COUNTRY_INTL,
    COUNTRY_COUNT
};
enum { LANG_CN = 0, LANG_TW, LANG_EN, LANG_DE, LANG_JP, LANG_COUNT };

typedef enum { MODE_NORMAL = 0, MODE_EVIDENCE = 1 } ClockMode;

// ---------------- 配置 ----------------
typedef struct { int country; int language; } Config;
static Config g_config = { COUNTRY_INTL, LANG_EN };
static wchar_t g_configPath[MAX_PATH] = {0};

// ---------------- 语言包 ----------------
typedef struct {
    const wchar_t *wndTitle;
    const wchar_t *switching;
    const wchar_t *syncOn;
    const wchar_t *synced;
    const wchar_t *unsynced;
    const wchar_t *syncFailed;
    const wchar_t *sourceConflict;
    const wchar_t *singleSourceWarn;
    const wchar_t *menuToggle;
    const wchar_t *menuResync;
    const wchar_t *menuSettings;
    const wchar_t *menuCountry;
    const wchar_t *menuLanguage;
    const wchar_t *menuDisclaimer;
    const wchar_t *menuAbout;
    const wchar_t *menuExit;
    const wchar_t *disclaimerTitle;
    const wchar_t *disclaimerBody;
    const wchar_t *aboutBody;
    const wchar_t *fontStatus;
} LangPack;

static const LangPack g_langCN = {
        L"精密时钟", L"正在切换精密时钟...", L"● 精密时钟｜%ls", L"已校时", L"未校时｜本地",
        L"切换失败：时间源不可达", L"源间分歧：时间差超过 50ms", L"单源可用｜未交叉验证",
        L"切换精密时钟", L"重新校时", L"设置", L"国家 / 地区", L"语言", L"免责声明", L"关于", L"退出",
        L"免责声明",
        L"本软件仅作通用时间参考，严禁作为任何医疗、航空、金融交易、法律时效、军事指挥等关键系统的唯一或决定性时间源。因使用本软件、依赖其输出、或因其时间偏差/错误造成的任何直接或间接损失，作者及发布者概不承担任何责任。\n\n如需可信时间戳，请咨询当地可信时间戳服务提供商（TSA）。\n\n服务范围：全球。",
        L"精密时钟 / Precision Clock\n版本 2.8.0-rc1\n\n"
        L"Copyright (C) 2026 Colligatio\nLicense: GPL-3.0\n\n"
        L"一个免费、开源、GPL-3.0 协议的桌面时间参考工具。\n"
        L"连接多个国际可信 NTP 源，交叉验证，显示毫秒级精度时间。\n\n"
        L"本软件仅作通用时间参考，严禁作为任何医疗、航空、金融交易、法律时效、军事指挥等关键系统的唯一或决定性时间源。\n"
        L"因使用本软件、依赖其输出、或因其时间偏差/错误造成的任何直接或间接损失，作者及发布者概不承担任何责任。\n\n"
        L"如需可信时间戳，请咨询当地可信时间戳服务提供商（TSA）。",
        L"Microsoft YaHei UI"
};

static const LangPack g_langTW = {
        L"精密時鐘", L"正在切換精密時鐘...", L"● 精密時鐘｜%ls", L"已校時", L"未校時｜本地",
        L"切換失敗：時間源不可達", L"源間分歧：時間差超過 50ms", L"單源可用｜未交叉驗證",
        L"切換精密時鐘", L"重新校時", L"設定", L"國家 / 地區", L"語言", L"免責聲明", L"關於", L"結束",
        L"免責聲明",
        L"本軟體僅作通用時間參考，嚴禁作為任何醫療、航空、金融交易、法律時效、軍事指揮等關鍵系統的唯一或決定性時間源。因使用本軟體、依賴其輸出、或因其時間偏差/錯誤造成的任何直接或間接損失，作者及發布者概不承擔任何責任。\n\n如需可信時間戳，請諮詢當地可信時間戳服務提供商（TSA）。\n\n服務範圍：全球。",
        L"精密時鐘 / Precision Clock\n版本 2.8.0-rc1\n\n"
        L"Copyright (C) 2026 Colligatio\nLicense: GPL-3.0\n\n"
        L"一個免費、開源、GPL-3.0 協議的桌面時間參考工具。\n"
        L"連接多個國際可信 NTP 源，交叉驗證，顯示毫秒級精度時間。\n\n"
        L"本軟體僅作通用時間參考，嚴禁作為任何醫療、航空、金融交易、法律時效、軍事指揮等關鍵系統的唯一或決定性時間源。\n"
        L"因使用本軟體、依賴其輸出、或因其時間偏差/錯誤造成的任何直接或間接損失，作者及發布者概不承擔任何責任。\n\n"
        L"如需可信時間戳，請諮詢當地可信時間戳服務提供商（TSA）。",
        L"Microsoft JhengHei UI"
};

static const LangPack g_langEN = {
        L"Precision Clock", L"Switching to Precision Clock...", L"● Precision Clock | %ls", L"Synced", L"Unsynced | Local",
        L"Sync failed: time source unreachable", L"Source conflict: time diff over 50ms", L"Single source | not cross-validated",
        L"Toggle Precision Clock", L"Resync", L"Settings", L"Country / Region", L"Language", L"Disclaimer", L"About", L"Exit",
        L"Disclaimer",
        L"This software is a general-purpose time reference only. It must not be used as the sole or decisive time source for any medical, aviation, financial trading, legal, military command, or other critical systems. The author and publisher assume no liability for any direct or indirect damages arising from the use of this software, reliance on its output, or any time deviation or error.\n\nFor trusted timestamps, consult a local TSA provider.\n\nService region: Global.",
        L"Precision Clock\nVersion 2.8.0-rc1\n\n"
        L"Copyright (C) 2026 Colligatio\nLicense: GPL-3.0\n\n"
        L"A free, open-source, GPL-3.0 time reference tool.\n"
        L"Connects to multiple trusted NTP sources, cross-validates, displays millisecond precision time.\n\n"
        L"This software is a general-purpose time reference only. It must not be used as the sole or decisive time source for any critical systems.\n"
        L"The author and publisher assume no liability for any direct or indirect damages arising from the use of this software, reliance on its output, or any time deviation or error.\n\n"
        L"For trusted timestamps, consult a local TSA provider.",
        L"Segoe UI"
};

static const LangPack g_langDE = {
        L"Präzisionsuhr", L"Wechsle zu Präzisionsuhr...", L"● Präzisionsuhr | %ls", L"Synchronisiert", L"Nicht synchron | Lokal",
        L"Sync fehlgeschlagen: Zeitquelle nicht erreichbar", L"Quellenkonflikt: Zeitdifferenz über 50ms", L"Einzelquelle | nicht kreuzvalidiert",
        L"Präzisionsuhr umschalten", L"Neu synchronisieren", L"Einstellungen", L"Land / Region", L"Sprache", L"Haftungsausschluss", L"Über", L"Beenden",
        L"Haftungsausschluss",
        L"Diese Software dient nur als allgemeine Zeitreferenz. Sie darf nicht als einzige oder entscheidende Zeitquelle für medizinische, luftfahrttechnische, finanzielle, rechtliche, militärische oder andere kritische Systeme verwendet werden. Der Autor und Herausgeber übernimmt keine Haftung für direkte oder indirekte Schäden, die durch die Nutzung dieser Software, das Vertrauen auf ihre Ausgabe oder jegliche Zeitabweichung oder Fehler entstehen.\n\nFür vertrauenswürdige Zeitstempel wenden Sie sich bitte an einen lokalen TSA-Anbieter.\n\nServicegebiet: Global.",
        L"Präzisionsuhr\nVersion 2.8.0-rc1\n\n"
        L"Copyright (C) 2026 Colligatio\nLizenz: GPL-3.0\n\n"
        L"Ein kostenloses, quelloffenes GPL-3.0-Zeitreferenztool.\n"
        L"Verbindet sich mit mehreren vertrauenswürdigen NTP-Quellen, kreuzvalidiert, zeigt millisekundengenaue Zeit an.\n\n"
        L"Diese Software dient nur als allgemeine Zeitreferenz. Sie darf nicht als einzige oder entscheidende Zeitquelle für kritische Systeme verwendet werden.\n"
        L"Der Autor und Herausgeber übernimmt keine Haftung für direkte oder indirekte Schäden, die durch die Nutzung dieser Software, das Vertrauen auf ihre Ausgabe oder jegliche Zeitabweichung oder Fehler entstehen.\n\n"
        L"Für vertrauenswürdige Zeitstempel wenden Sie sich bitte an einen lokalen TSA-Anbieter.",
        L"Segoe UI"
};

static const LangPack g_langJP = {
        L"精密時計", L"精密時計に切り替え中...", L"● 精密時計 | %ls", L"同期済み", L"未同期 | ローカル",
        L"同期失敗：時刻ソースに到達できません", L"ソース競合：時刻差が50msを超えています", L"単一ソース | クロス検証なし",
        L"精密時計の切替", L"再同期", L"設定", L"国 / 地域", L"言語", L"免責事項", L"バージョン情報", L"終了",
        L"免責事項",
        L"本ソフトウェアは一般的な時刻参照としてのみ提供されます。医療、航空、金融取引、法務、軍事指揮などの重要なシステムの唯一または決定的な時刻源として使用しないでください。本ソフトウェアの使用、その出力への依存、または時刻の偏差や誤りに起因する直接的または間接的な損害について、作者および発行者は一切の責任を負いません。\n\n信頼できるタイムスタンプについては、お住まいの地域のTSA提供者にお問い合わせください。\n\nサービス地域：グローバル。",
        L"精密時計 / Precision Clock\nバージョン 2.8.0-rc1\n\n"
        L"Copyright (C) 2026 Colligatio\nライセンス: GPL-3.0\n\n"
        L"無料のオープンソース GPL-3.0 時刻参照ツール。\n"
        L"複数の信頼できる NTP ソースに接続し、クロス検証し、ミリ秒精度の時刻を表示します。\n\n"
        L"本ソフトウェアは一般的な時刻参照としてのみ提供されます。重要なシステムの唯一または決定的な時刻源として使用しないでください。\n"
        L"本ソフトウェアの使用、その出力への依存、または時刻の偏差や誤りに起因する直接的または間接的な損害について、作者および発行者は一切の責任を負いません。\n\n"
        L"信頼できるタイムスタンプについては、お住まいの地域のTSA提供者にお問い合わせください。",
        L"Yu Gothic UI"
};

static const LangPack *g_lang = &g_langEN;

// ---------------- 国家表 ----------------
static const char* g_countryHosts[COUNTRY_COUNT] = {
        "ntp.ntsc.ac.cn", "time.nist.gov", "time.nist.gov", "time.nist.gov", "time.nist.gov",
        "ptbtime1.ptb.de", "ntp.nict.jp", "ntp1.npl.co.uk", "time.cloudflare.com"
};

static const int g_countryTzOffsetMin[COUNTRY_COUNT] = {
        8 * 60, -5 * 60, -6 * 60, -7 * 60, -8 * 60, 1 * 60, 9 * 60, 0, 0
};

static const wchar_t* g_countryTzIds[COUNTRY_COUNT] = {
        L"China Standard Time", L"Eastern Standard Time", L"Central Standard Time",
        L"Mountain Standard Time", L"Pacific Standard Time", L"W. Europe Standard Time",
        L"Tokyo Standard Time", L"GMT Standard Time", NULL
};

static const wchar_t* g_countryNames[COUNTRY_COUNT][LANG_COUNT] = {
        { L"中国", L"中國", L"China", L"China", L"中国" },
        { L"美国东部", L"美國東部", L"US Eastern", L"US Ost", L"アメリカ東部" },
        { L"美国中部", L"美國中部", L"US Central", L"US Zentral", L"アメリカ中部" },
        { L"美国山地", L"美國山地", L"US Mountain", L"US Mountain", L"アメリカ山地" },
        { L"美国太平洋", L"美國太平洋", L"US Pacific", L"US Pazifik", L"アメリカ太平洋" },
        { L"德国", L"德國", L"Germany", L"Deutschland", L"ドイツ" },
        { L"日本", L"日本", L"Japan", L"Japan", L"日本" },
        { L"英国", L"英國", L"United Kingdom", L"Vereinigtes Königreich", L"イギリス" },
        { L"国际", L"國際", L"International", L"International", L"国際" }
};

static const wchar_t* g_langNames[LANG_COUNT] = {
        L"简体中文", L"繁體中文", L"English", L"Deutsch", L"日本語"
};

// ---------------- 全局 ----------------
static HWND      g_hwnd = NULL;
static ClockMode g_mode = MODE_NORMAL;
static BOOL      g_normalSynced  = FALSE;
static BOOL      g_switching     = FALSE;
static BOOL      g_dragging      = FALSE;
static BOOL      g_lightTheme    = FALSE;
static BOOL      g_windowVisible = TRUE;
static BOOL      g_showFailMsg   = FALSE;
static BOOL      g_pendingResync = FALSE;
static BOOL      g_normalSyncing = FALSE;
static int       g_hoverBtn      = 0;
static POINT     g_dragAnchor = {0, 0};
static DWORD     g_lastClick = 0;
static int       g_winX = 0, g_winY = 0;
static int       g_ntpFailCount    = 0;
static int       g_normalFailCount = 0;

static ULONGLONG g_baseTimeMs = 0;
static LARGE_INTEGER g_qpcFreq, g_qpcBase;

static ULONG_PTR g_gdiplusToken = 0;
static HDC       g_memDC   = NULL;
static HBITMAP   g_hBitmap = NULL;
static HBITMAP   g_oldBmp  = NULL;
static void     *g_pBits   = NULL;

static volatile LONG g_ntpPending = 0;
static ULONGLONG     g_ntpResult  = 0;
static int           g_ntpStatus  = -1;
static int           g_ntpTask    = 0;
static wchar_t       g_lastSource[64] = {0};

static NOTIFYICONDATAW g_nid = {0};

// ---------------- 时区 API 动态加载 ----------------
typedef DWORD (WINAPI *PFN_EnumDynamicTimeZoneInformation)(DWORD, PDYNAMIC_TIME_ZONE_INFORMATION);
typedef BOOL  (WINAPI *PFN_SystemTimeToTzSpecificLocalTimeEx)(const DYNAMIC_TIME_ZONE_INFORMATION*, const SYSTEMTIME*, LPSYSTEMTIME);

static PFN_EnumDynamicTimeZoneInformation    pEnumDynamicTz    = NULL;
static PFN_SystemTimeToTzSpecificLocalTimeEx pSystemTimeToTzEx = NULL;

static void LoadTzApi(void) {
    if (pEnumDynamicTz && pSystemTimeToTzEx) return;
    HMODULE hKernel = GetModuleHandleW(L"kernel32.dll");
    if (!pEnumDynamicTz)
        pEnumDynamicTz = (PFN_EnumDynamicTimeZoneInformation)GetProcAddress(hKernel, "EnumDynamicTimeZoneInformation");
    if (!pSystemTimeToTzEx)
        pSystemTimeToTzEx = (PFN_SystemTimeToTzSpecificLocalTimeEx)GetProcAddress(hKernel, "SystemTimeToTzSpecificLocalTimeEx");
}

static DYNAMIC_TIME_ZONE_INFORMATION g_cachedDtzi;
static int  g_cachedDtziCountry = -1;
static BOOL g_cachedDtziValid   = FALSE;

// ---------------- 前向声明 ----------------
static void Render(void);
static void ToggleMode(void);
static void StartNtpTask(int taskType);
static void ApplyNormalLocal(void);
static void EnterPrecisionModeFromResult(int status, ULONGLONG ms);
static BOOL IsSystemLightTheme(void);

// ---------------- 语言 / 配置 ----------------
static const LangPack* CurrentLang(void) {
    switch (g_config.language) {
        case LANG_CN: return &g_langCN;
        case LANG_TW: return &g_langTW;
        case LANG_EN: return &g_langEN;
        case LANG_DE: return &g_langDE;
        case LANG_JP: return &g_langJP;
        default:      return &g_langEN;
    }
}

static const wchar_t* CountryName(int idx) {
    if (idx < 0 || idx >= COUNTRY_COUNT) return L"?";
    return g_countryNames[idx][g_config.language];
}

static void EnsureConfigPath(void) {
    if (g_configPath[0]) return;
    wchar_t dir[MAX_PATH];
    DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", dir, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) {
        wcscpy_s(g_configPath, MAX_PATH, L"precision_clock.ini");
        return;
    }
    swprintf_s(g_configPath, MAX_PATH, L"%ls\\PrecisionClock", dir);
    CreateDirectoryW(g_configPath, NULL);
    wcscat_s(g_configPath, MAX_PATH, L"\\config.ini");
}

static void LoadConfig(void) {
    EnsureConfigPath();
    LANGID sysLang = GetUserDefaultUILanguage();
    WORD primary = PRIMARYLANGID(sysLang);
    WORD sub     = SUBLANGID(sysLang);

    if (primary == LANG_CHINESE) {
        g_config.language = (sub == SUBLANG_CHINESE_SIMPLIFIED) ? LANG_CN : LANG_TW;
        g_config.country  = COUNTRY_CN;
    } else if (primary == LANG_GERMAN) {
        g_config.language = LANG_DE;
        g_config.country  = COUNTRY_DE;
    } else if (primary == LANG_JAPANESE) {
        g_config.language = LANG_JP;
        g_config.country  = COUNTRY_JP;
    } else {
        g_config.language = LANG_EN;
        g_config.country  = COUNTRY_INTL;
    }

    FILE* f = _wfopen(g_configPath, L"r");
    if (!f) return;
    char line[128];
    while (fgets(line, sizeof(line), f)) {
        int v;
        if (sscanf(line, "country=%d", &v) == 1 && v >= 0 && v < COUNTRY_COUNT)
            g_config.country = v;
        else if (sscanf(line, "language=%d", &v) == 1 && v >= 0 && v < LANG_COUNT)
            g_config.language = v;
    }
    fclose(f);
}

static void SaveConfig(void) {
    EnsureConfigPath();
    FILE* f = _wfopen(g_configPath, L"w");
    if (!f) return;
    fprintf(f, "country=%d\n", g_config.country);
    fprintf(f, "language=%d\n", g_config.language);
    fclose(f);
}

// ---------------- 系统主题 ----------------
static BOOL IsSystemLightTheme(void) {
    HKEY hKey;
    DWORD value = 1;
    DWORD size = sizeof(value);
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegQueryValueExW(hKey, L"AppsUseLightTheme", NULL, NULL, (LPBYTE)&value, &size);
        RegCloseKey(hKey);
    }
    return value != 0;
}

// ---------------- 时间工具 ----------------
static ULONGLONG NowMs(void) {
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    ULONGLONG elapsed = (ULONGLONG)((now.QuadPart - g_qpcBase.QuadPart) * 1000LL / g_qpcFreq.QuadPart);
    return g_baseTimeMs + elapsed;
}

static void UnixMsToUtc(ULONGLONG unixMs, SYSTEMTIME *st) {
    ULONGLONG ftMs = unixMs + FILETIME_TO_UNIX;
    ULARGE_INTEGER uli;
    uli.QuadPart = ftMs * 10000ULL;
    FILETIME ft;
    ft.dwLowDateTime  = uli.LowPart;
    ft.dwHighDateTime = uli.HighPart;
    FileTimeToSystemTime(&ft, st);
}

static BOOL GetCachedDtzi(DYNAMIC_TIME_ZONE_INFORMATION* out) {
    if (g_cachedDtziValid && g_cachedDtziCountry == g_config.country) {
        *out = g_cachedDtzi;
        return TRUE;
    }
    LoadTzApi();
    if (!pEnumDynamicTz) { g_cachedDtziValid = FALSE; return FALSE; }

    DYNAMIC_TIME_ZONE_INFORMATION dtzi;
    DWORD idx = 0;
    while (pEnumDynamicTz(idx, &dtzi) == ERROR_SUCCESS) {
        if (_wcsicmp(dtzi.TimeZoneKeyName, g_countryTzIds[g_config.country]) == 0) {
            g_cachedDtzi = dtzi;
            g_cachedDtziCountry = g_config.country;
            g_cachedDtziValid = TRUE;
            *out = dtzi;
            return TRUE;
        }
        idx++;
    }
    g_cachedDtziValid = FALSE;
    return FALSE;
}

static void GetDisplayTime(SYSTEMTIME *st) {
    ULONGLONG unixMs = NowMs();
    SYSTEMTIME utcSt;
    UnixMsToUtc(unixMs, &utcSt);

    if (g_config.country == COUNTRY_INTL) {
        SystemTimeToTzSpecificLocalTime(NULL, &utcSt, st);
        return;
    }

    LoadTzApi();
    DYNAMIC_TIME_ZONE_INFORMATION dtzi;
    ZeroMemory(&dtzi, sizeof(dtzi));

    if (pSystemTimeToTzEx && GetCachedDtzi(&dtzi)) {
        pSystemTimeToTzEx(&dtzi, &utcSt, st);
    } else {
        int offsetMin = g_countryTzOffsetMin[g_config.country];
        ULONGLONG shifted = unixMs + (ULONGLONG)(offsetMin * 60) * 1000ULL;
        UnixMsToUtc(shifted, st);
    }
}

static ULONGLONG LocalToUnixMs(void) {
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    ULARGE_INTEGER uli;
    uli.LowPart  = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    return (uli.QuadPart / 10000ULL) - FILETIME_TO_UNIX;
}

static ULONGLONG ParseNtpTimestamp(const unsigned char *p) {
    ULONGLONG sec = ((ULONGLONG)p[0] << 24) | ((ULONGLONG)p[1] << 16) | ((ULONGLONG)p[2] << 8) | (ULONGLONG)p[3];
    ULONGLONG fr  = ((ULONGLONG)p[4] << 24) | ((ULONGLONG)p[5] << 16) | ((ULONGLONG)p[6] << 8) | (ULONGLONG)p[7];
    ULONGLONG unixSec = sec - NTP_TO_UNIX_SEC;
    return unixSec * 1000ULL + (fr * 1000ULL) / 4294967296ULL;
}

// ---------------- NTP ----------------
static int NtpGetTimeMsFromHost(const char* host, ULONGLONG* out) {
    SOCKET sock = INVALID_SOCKET;
    struct addrinfo hints, *res = NULL;
    unsigned char packet[48];
    int ok = -1;
    DWORD t1 = 0, t4 = 0;
    DWORD tv = NTP_TIMEOUT_MS;

    memset(packet, 0, sizeof(packet));
    packet[0] = 0x1B;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    if (getaddrinfo(host, "123", &hints, &res) != 0) goto cleanup;
    sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock == INVALID_SOCKET) goto cleanup;

    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

    t1 = GetTickCount();
    if (sendto(sock, (const char*)packet, 48, 0, res->ai_addr, (int)res->ai_addrlen) == SOCKET_ERROR) goto cleanup;
    if (recvfrom(sock, (char*)packet, 48, 0, NULL, NULL) < 48) goto cleanup;
    t4 = GetTickCount();

    {
        ULONGLONG t3 = ParseNtpTimestamp(packet + 32);
        ULONGLONG rtt = (t4 > t1) ? (ULONGLONG)(t4 - t1) : 0ULL;
        *out = t3 + rtt / 2ULL;
        ok = 0;
    }

    cleanup:
    if (sock != INVALID_SOCKET) closesocket(sock);
    if (res) freeaddrinfo(res);
    return ok;
}

typedef struct { char host[128]; ULONGLONG timeMs; int ok; } NtpQuery;

static DWORD WINAPI NtpWorker(LPVOID param) {
NtpQuery* q = (NtpQuery*)param;
ULONGLONG ms = 0;
q->ok = (NtpGetTimeMsFromHost(q->host, &ms) == 0);
q->timeMs = ms;
return 0;
}

static int QueryNtpSources(ULONGLONG* outTime, wchar_t* outSourceName, int nameCap) {
    NtpQuery queries[MAX_NTP_SOURCES];
    int n = 0;
    memset(queries, 0, sizeof(queries));

    const char* primary = g_countryHosts[g_config.country];
    snprintf(queries[n].host, sizeof(queries[n].host), "%s", primary);
    n++;

    const char* fallbacks[3] = { "time.cloudflare.com", "time.google.com", "time.nist.gov" };
    for (int i = 0; i < 3 && n < MAX_NTP_SOURCES; i++) {
        int dup = 0;
        for (int j = 0; j < n; j++) {
            if (strcmp(queries[j].host, fallbacks[i]) == 0) { dup = 1; break; }
        }
        if (dup) continue;
        snprintf(queries[n].host, sizeof(queries[n].host), "%s", fallbacks[i]);
        n++;
    }

    HANDLE threads[MAX_NTP_SOURCES];
    for (int i = 0; i < n; i++) {
        threads[i] = CreateThread(NULL, 0, NtpWorker, &queries[i], 0, NULL);
    }
    for (int i = 0; i < n; i++) {
        if (threads[i]) {
            WaitForSingleObject(threads[i], NTP_TIMEOUT_MS + 1000);
            CloseHandle(threads[i]);
        }
    }

    ULONGLONG times[MAX_NTP_SOURCES];
    int cnt = 0;
    for (int i = 0; i < n; i++) {
        if (queries[i].ok && queries[i].timeMs > 0) times[cnt++] = queries[i].timeMs;
    }

    if (cnt == 0) {
        if (outSourceName) outSourceName[0] = 0;
        return -1;
    }

    for (int i = 0; i < cnt - 1; i++)
        for (int j = i + 1; j < cnt; j++)
            if (times[i] > times[j]) { ULONGLONG t = times[i]; times[i] = times[j]; times[j] = t; }

    ULONGLONG median = times[cnt / 2];
    ULONGLONG maxDiff = times[cnt - 1] - times[0];
    *outTime = median;

    if (outSourceName) {
        const wchar_t* cname = CountryName(g_config.country);
        swprintf_s(outSourceName, nameCap, L"%ls", cname);
    }

    if (cnt >= 2) {
        if (maxDiff <= CROSS_VALIDATE_MAX_DIFF_MS) return 0;
        return 2;
    }
    return 1;
}

static DWORD WINAPI NtpThread(LPVOID param) {
ULONGLONG ms = 0;
wchar_t src[64] = {0};
int status = QueryNtpSources(&ms, src, 64);
g_ntpResult = ms;
g_ntpStatus = status;
wcsncpy_s(g_lastSource, 64, src, _TRUNCATE);
InterlockedExchange(&g_ntpPending, 0);
PostMessage(g_hwnd, WM_NTP_DONE, 0, 0);
return 0;
}

static void StartNtpTask(int taskType) {
    if (InterlockedCompareExchange(&g_ntpPending, 1, 0) != 0) return;
    g_ntpTask = taskType;
    HANDLE h = CreateThread(NULL, 0, NtpThread, NULL, 0, NULL);
    if (h) CloseHandle(h);
    else    InterlockedExchange(&g_ntpPending, 0);
}

// ---------------- 模式切换 ----------------
static void ApplyNormalLocal(void) {
    g_baseTimeMs = LocalToUnixMs();
    QueryPerformanceCounter(&g_qpcBase);
    g_normalSynced = FALSE;
}

static void EnterPrecisionModeFromResult(int status, ULONGLONG ms) {
    if (status == 0 || status == 1) {
        g_baseTimeMs = ms;
        QueryPerformanceCounter(&g_qpcBase);
        g_mode = MODE_EVIDENCE;
        KillTimer(g_hwnd, TIMER_DISPLAY);
        SetTimer(g_hwnd, TIMER_DISPLAY, 33, NULL);
        SetTimer(g_hwnd, TIMER_RESYNC, EVIDENCE_RESYNC_MS, NULL);
        KillTimer(g_hwnd, TIMER_NORMAL_RESYNC);
        g_switching = FALSE;
    } else {
        g_showFailMsg = TRUE;
        g_switching = FALSE;
        KillTimer(g_hwnd, TIMER_FAILMSG);
        SetTimer(g_hwnd, TIMER_FAILMSG, 2500, NULL);
    }
}

static void EnterPrecisionMode(void) {
    if (g_switching) return;
    g_switching = TRUE;
    Render();
    StartNtpTask(1);
}

static void ExitPrecisionMode(void) {
    KillTimer(g_hwnd, TIMER_RESYNC);
    KillTimer(g_hwnd, TIMER_DISPLAY);
    SetTimer(g_hwnd, TIMER_DISPLAY, 500, NULL);
    g_mode = MODE_NORMAL;
    ApplyNormalLocal();
    g_ntpFailCount = 0;
    g_normalFailCount = 0;
    SetTimer(g_hwnd, TIMER_NORMAL_RESYNC, NORMAL_RESYNC_MS, NULL);
    Render();
    if (g_ntpPending == 0) StartNtpTask(0);
    else                  g_pendingResync = TRUE;
}

static void ToggleMode(void) {
    if (g_switching) return;
    if (g_mode == MODE_NORMAL) EnterPrecisionMode();
    else                        ExitPrecisionMode();
}

// ---------------- 绘制 ----------------
static void DrawColligatioLogo(Graphics &g, int x, int y) {
    Pen penBlack(Color(255, 20, 20, 20), 1.5f);
    Pen penBlue (Color(255, 46, 94, 140), 1.5f);
    Pen penGold (Color(255, 201, 162, 39), 1.5f);
    g.SetSmoothingMode(SmoothingModeAntiAlias);

    int bigSize   = 22;
    int smallSize = 15;

    g.DrawRectangle(&penBlue,  x + 15, y + 2,  smallSize, smallSize);
    g.DrawRectangle(&penBlack, x,      y + 8,  bigSize,   bigSize);
    g.DrawRectangle(&penGold,  x + 26, y + 12, bigSize,   bigSize);
}

static void DrawAeroGlass(Graphics &g, int w, int h, BOOL light) {
    GraphicsPath path;
    int r = 14;
    path.AddArc(0, 0, r, r, 180, 90);
    path.AddArc(w - r, 0, r, r, 270, 90);
    path.AddArc(w - r, h - r, r, r, 0, 90);
    path.AddArc(0, h - r, r, r, 90, 90);
    path.CloseFigure();

    if (light) {
        LinearGradientBrush mainGrad(Point(0, 0), Point(0, h), Color(120, 255, 255, 255), Color(90, 235, 238, 245));
        g.FillPath(&mainGrad, &path);
        LinearGradientBrush topHi(Point(0, 0), Point(0, h / 3), Color(70, 255, 255, 255), Color(0, 255, 255, 255));
        g.FillPath(&topHi, &path);
        LinearGradientBrush bottomGlow(Point(0, h * 2 / 3), Point(0, h), Color(0, 255, 255, 255), Color(25, 200, 215, 230));
        g.FillPath(&bottomGlow, &path);
        Pen edge(Color(80, 255, 255, 255), 1.0f);
        g.DrawPath(&edge, &path);
    } else {
        LinearGradientBrush mainGrad(Point(0, 0), Point(0, h), Color(130, 28, 32, 40), Color(110, 8, 10, 14));
        g.FillPath(&mainGrad, &path);
        LinearGradientBrush topHi(Point(0, 0), Point(0, h / 3), Color(45, 255, 255, 255), Color(0, 255, 255, 255));
        g.FillPath(&topHi, &path);
        LinearGradientBrush bottomGlow(Point(0, h * 2 / 3), Point(0, h), Color(0, 255, 255, 255), Color(20, 120, 150, 200));
        g.FillPath(&bottomGlow, &path);
        Pen edge(Color(50, 255, 255, 255), 1.0f);
        g.DrawPath(&edge, &path);
    }
}

static void BuildStatusW(wchar_t *buf, int cap) {
    if (g_switching || g_normalSyncing) {
        wcsncpy_s(buf, cap, g_lang->switching, _TRUNCATE);
        return;
    }
    if (g_showFailMsg) {
        if (g_ntpStatus == 2) wcsncpy_s(buf, cap, g_lang->sourceConflict, _TRUNCATE);
        else                  wcsncpy_s(buf, cap, g_lang->syncFailed, _TRUNCATE);
        return;
    }
    if (g_mode == MODE_EVIDENCE) {
        if (g_ntpStatus == 1) { wcsncpy_s(buf, cap, g_lang->singleSourceWarn, _TRUNCATE); return; }
        if (g_ntpStatus == 2) { wcsncpy_s(buf, cap, g_lang->sourceConflict, _TRUNCATE); return; }
        wchar_t tmp[96];
        swprintf_s(tmp, 96, g_lang->syncOn, g_lastSource);
        wcsncpy_s(buf, cap, tmp, _TRUNCATE);
        return;
    }
    wcsncpy_s(buf, cap, g_normalSynced ? g_lang->synced : g_lang->unsynced, _TRUNCATE);
}

static void DrawTextsWithGDI(void) {
    Graphics g(g_memDC);
    g.SetTextRenderingHint(TextRenderingHintAntiAlias);
    g.SetSmoothingMode(SmoothingModeAntiAlias);

    // 时间
    {
        SYSTEMTIME st;
        GetDisplayTime(&st);
        wchar_t timeBuf[32];
        if (g_mode == MODE_EVIDENCE) {
            swprintf_s(timeBuf, 32, L"%02d:%02d:%02d.%03d", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
        } else {
            swprintf_s(timeBuf, 32, L"%02d:%02d:%02d", st.wHour, st.wMinute, st.wSecond);
        }

        Font fontBig(L"Consolas", 28, FontStyleRegular, UnitPixel);
        SolidBrush brush(g_lightTheme ? Color(255, 0, 0, 0) : Color(255, 255, 255, 255));
        RectF rc(66.0f, 16.0f, 200.0f, 34.0f);
        g.DrawString(timeBuf, -1, &fontBig, rc, NULL, &brush);
    }

    // 状态
    {
        wchar_t status[128];
        BuildStatusW(status, 128);

        Font fontSmall(g_lang->fontStatus, 12, FontStyleRegular, UnitPixel);
        SolidBrush brush(g_lightTheme ? Color(255, 0, 0, 0) : Color(255, 255, 255, 255));
        RectF rc(66.0f, 54.0f, 200.0f, 20.0f);
        g.DrawString(status, -1, &fontSmall, rc, NULL, &brush);
    }

    // 右下角红绿灯
    {
        float cy = BTN_CY;
        float r  = BTN_R;

        Color cRed   = (g_hoverBtn == 1) ? Color(255, 255, 130, 120) : Color(255, 255, 95, 86);
        Color cYellow= (g_hoverBtn == 2) ? Color(255, 255, 215,  90) : Color(255, 255, 189, 46);
        Color cGreen = (g_hoverBtn == 3) ? Color(255,  80, 225, 100) : Color(255,  39, 201, 63);

        SolidBrush brRed(cRed);
        SolidBrush brYellow(cYellow);
        SolidBrush brGreen(cGreen);

        g.FillEllipse(&brRed,    BTN_RED_X    - r, cy - r, r * 2, r * 2);
        g.FillEllipse(&brYellow, BTN_YELLOW_X - r, cy - r, r * 2, r * 2);
        g.FillEllipse(&brGreen,  BTN_GREEN_X  - r, cy - r, r * 2, r * 2);

        Color symColor(200, 40, 20, 20);
        Pen symPen(symColor, 1.2f);
        symPen.SetLineJoin(LineJoinRound);
        symPen.SetStartCap(LineCapRound);
        symPen.SetEndCap(LineCapRound);

        float s = 2.0f;

        if (g_hoverBtn == 1) {
            g.DrawLine(&symPen, BTN_RED_X - s, cy - s, BTN_RED_X + s, cy + s);
            g.DrawLine(&symPen, BTN_RED_X - s, cy + s, BTN_RED_X + s, cy - s);
        }
        if (g_hoverBtn == 2) {
            g.DrawLine(&symPen, BTN_YELLOW_X - s, cy, BTN_YELLOW_X + s, cy);
        }
        if (g_hoverBtn == 3) {
            g.DrawRectangle(&symPen, BTN_GREEN_X - s, cy - s, s * 2, s * 2);
        }
    }
}

static void Render(void) {
    if (!g_memDC || !g_hwnd) return;

    {
        Graphics g(g_memDC);
        g.SetSmoothingMode(SmoothingModeAntiAlias);
        g.SetTextRenderingHint(TextRenderingHintAntiAlias);
        g.Clear(Color(0, 0, 0, 0));
        DrawAeroGlass(g, CANVAS_W, CANVAS_H, g_lightTheme);
        DrawColligatioLogo(g, 18, 20);
    }

    DrawTextsWithGDI();

    HDC screenDC = GetDC(NULL);
    POINT ptSrc = {0, 0};
    SIZE  size  = {CANVAS_W, CANVAS_H};
    POINT ptDst = {g_winX, g_winY};
    BLENDFUNCTION blend = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
    UpdateLayeredWindow(g_hwnd, screenDC, &ptDst, &size, g_memDC, &ptSrc, 0, &blend, ULW_ALPHA);
    ReleaseDC(NULL, screenDC);
}

// ---------------- 托盘 ----------------
static void ShowTrayMenu(void) {
    HMENU hMenu = CreatePopupMenu();
    const LangPack* L = CurrentLang();

    AppendMenuW(hMenu, MF_STRING, IDM_TOGGLE, L->menuToggle);
    AppendMenuW(hMenu, MF_STRING, IDM_RESYNC, L->menuResync);
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);

    HMENU hSettings = CreatePopupMenu();
    HMENU hCountry = CreatePopupMenu();
    for (int i = 0; i < COUNTRY_COUNT; i++) {
        UINT flags = MF_STRING;
        if (i == g_config.country) flags |= MF_CHECKED;
        AppendMenuW(hCountry, flags, IDM_COUNTRY_BASE + i, CountryName(i));
    }
    AppendMenuW(hSettings, MF_POPUP, (UINT_PTR)hCountry, L->menuCountry);

    HMENU hLang = CreatePopupMenu();
    for (int i = 0; i < LANG_COUNT; i++) {
        UINT flags = MF_STRING;
        if (i == g_config.language) flags |= MF_CHECKED;
        AppendMenuW(hLang, flags, IDM_LANG_BASE + i, g_langNames[i]);
    }
    AppendMenuW(hSettings, MF_POPUP, (UINT_PTR)hLang, L->menuLanguage);

    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hSettings, L->menuSettings);
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, IDM_DISCLAIMER, L->menuDisclaimer);
    AppendMenuW(hMenu, MF_STRING, IDM_ABOUT, L->menuAbout);
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, IDM_EXIT, L->menuExit);

    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(g_hwnd);
    int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, 0, g_hwnd, NULL);
    DestroyMenu(hMenu);

    if (cmd == 0) return;
    if (cmd == IDM_TOGGLE) {
        ToggleMode();
    } else if (cmd == IDM_RESYNC) {
        if (g_mode == MODE_EVIDENCE) StartNtpTask(2);
        else                          StartNtpTask(0);
    } else if (cmd == IDM_DISCLAIMER) {
        MessageBoxW(g_hwnd, L->disclaimerBody, L->disclaimerTitle, MB_OK | MB_ICONINFORMATION);
    } else if (cmd == IDM_ABOUT) {
        MessageBoxW(g_hwnd, L->aboutBody, L->menuAbout, MB_OK | MB_ICONINFORMATION);
    } else if (cmd == IDM_EXIT) {
        DestroyWindow(g_hwnd);
    } else if (cmd >= IDM_COUNTRY_BASE && cmd < IDM_COUNTRY_BASE + COUNTRY_COUNT) {
        int newCountry = cmd - IDM_COUNTRY_BASE;
        if (newCountry != g_config.country) {
            g_config.country = newCountry;
            g_cachedDtziValid = FALSE;
            SaveConfig();
            g_switching = TRUE;
            g_ntpFailCount = 0;
            g_normalFailCount = 0;
            g_normalSyncing = (g_mode == MODE_NORMAL);
            Render();
            if (g_ntpPending == 0) {
                if (g_mode == MODE_EVIDENCE) StartNtpTask(2);
                else                          StartNtpTask(0);
            } else {
                g_pendingResync = TRUE;
            }
        }
    } else if (cmd >= IDM_LANG_BASE && cmd < IDM_LANG_BASE + LANG_COUNT) {
        int newLang = cmd - IDM_LANG_BASE;
        if (newLang != g_config.language) {
            g_config.language = newLang;
            g_lang = CurrentLang();
            SaveConfig();
            Render();
        }
    }
}

static void InitTrayIcon(HINSTANCE hInst) {
    memset(&g_nid, 0, sizeof(g_nid));
    g_nid.cbSize = sizeof(g_nid);
    g_nid.hWnd   = g_hwnd;
    g_nid.uID    = TRAY_ID;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon  = (HICON)LoadImageW(hInst, L"IDI_ICON1", IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    if (!g_nid.hIcon) g_nid.hIcon = (HICON)LoadImageW(GetModuleHandleW(NULL), L"IDI_ICON1", IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    if (!g_nid.hIcon) g_nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcsncpy_s(g_nid.szTip, 128, g_lang->wndTitle, _TRUNCATE);
    Shell_NotifyIconW(NIM_ADD, &g_nid);
}

static void UpdateTrayTip(void) {
    wcsncpy_s(g_nid.szTip, 128, g_lang->wndTitle, _TRUNCATE);
    g_nid.uFlags = NIF_TIP;
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
}

static void RemoveTrayIcon(void) {
    Shell_NotifyIconW(NIM_DELETE, &g_nid);
}

static int HitTestButton(int mx, int my) {
    float cy = BTN_CY;
    float r  = BTN_R;

    struct { float cx; int id; } btns[3] = {
            { BTN_RED_X,    1 },
            { BTN_YELLOW_X, 2 },
            { BTN_GREEN_X,  3 }
    };

    for (int i = 0; i < 3; i++) {
        float dx = (float)mx - btns[i].cx;
        float dy = (float)my - cy;
        if (dx * dx + dy * dy <= r * r) return btns[i].id;
    }
    return 0;
}

// ---------------- 窗口过程 ----------------
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
switch (msg) {
case WM_CREATE:
SetWindowTextW(hwnd, g_lang->wndTitle);
return 0;

case WM_TRAYICON:
if (LOWORD(lp) == WM_LBUTTONUP) {
g_windowVisible = !g_windowVisible;
ShowWindow(hwnd, g_windowVisible ? SW_SHOWNOACTIVATE : SW_HIDE);
} else if (LOWORD(lp) == WM_RBUTTONUP) {
ShowTrayMenu();
}
return 0;

case WM_TIMER:
if (wp == TIMER_DISPLAY) {
Render();
} else if (wp == TIMER_RESYNC && g_mode == MODE_EVIDENCE) {
StartNtpTask(2);
} else if (wp == TIMER_FAILMSG) {
KillTimer(hwnd, TIMER_FAILMSG);
g_showFailMsg = FALSE;
Render();
} else if (wp == TIMER_NORMAL_RESYNC && g_mode == MODE_NORMAL) {
StartNtpTask(0);
}
return 0;

case WM_NTP_DONE: {
int status = g_ntpStatus;
ULONGLONG ms = g_ntpResult;

if (g_ntpTask == 0) {
if (status == 0 || status == 1) {
g_baseTimeMs = ms;
QueryPerformanceCounter(&g_qpcBase);
g_normalSynced = TRUE;
g_normalFailCount = 0;
} else {
g_normalFailCount++;
if (g_normalFailCount >= NORMAL_MAX_FAIL) {
ApplyNormalLocal();
}
}
g_switching = FALSE;
g_normalSyncing = FALSE;
} else if (g_ntpTask == 1) {
EnterPrecisionModeFromResult(status, ms);
} else if (g_ntpTask == 2) {
if (status == 0 || status == 1) {
g_baseTimeMs = ms;
QueryPerformanceCounter(&g_qpcBase);
g_ntpFailCount = 0;
KillTimer(hwnd, TIMER_RESYNC);
SetTimer(hwnd, TIMER_RESYNC, EVIDENCE_RESYNC_MS, NULL);
} else if (status == 2) {
KillTimer(hwnd, TIMER_RESYNC);
KillTimer(hwnd, TIMER_DISPLAY);
SetTimer(hwnd, TIMER_DISPLAY, 500, NULL);
g_mode = MODE_NORMAL;
ApplyNormalLocal();
g_ntpFailCount = 0;
SetTimer(hwnd, TIMER_NORMAL_RESYNC, NORMAL_RESYNC_MS, NULL);
g_showFailMsg = TRUE;
KillTimer(hwnd, TIMER_FAILMSG);
SetTimer(hwnd, TIMER_FAILMSG, 2500, NULL);
} else {
g_ntpFailCount++;
if (g_ntpFailCount < 2) {
KillTimer(hwnd, TIMER_RESYNC);
SetTimer(hwnd, TIMER_RESYNC, 5000, NULL);
} else {
KillTimer(hwnd, TIMER_RESYNC);
KillTimer(hwnd, TIMER_DISPLAY);
SetTimer(hwnd, TIMER_DISPLAY, 500, NULL);
g_mode = MODE_NORMAL;
ApplyNormalLocal();
g_ntpFailCount = 0;
SetTimer(hwnd, TIMER_NORMAL_RESYNC, NORMAL_RESYNC_MS, NULL);
g_showFailMsg = TRUE;
KillTimer(hwnd, TIMER_FAILMSG);
SetTimer(hwnd, TIMER_FAILMSG, 2500, NULL);
}
}
g_switching = FALSE;
}

if (g_pendingResync) {
g_pendingResync = FALSE;
if (g_mode == MODE_NORMAL) {
StartNtpTask(0);
} else {
g_switching = TRUE;
StartNtpTask(2);
}
}
Render();
return 0;
}

case WM_MOUSEMOVE: {
int mx = (int)(short)LOWORD(lp);
int my = (int)(short)HIWORD(lp);

int newHover = HitTestButton(mx, my);
if (newHover != g_hoverBtn) {
g_hoverBtn = newHover;
Render();
}

if (g_dragging) {
POINT p;
GetCursorPos(&p);
g_winX += (p.x - g_dragAnchor.x);
g_winY += (p.y - g_dragAnchor.y);
g_dragAnchor = p;
Render();
}

TRACKMOUSEEVENT tme = {0};
tme.cbSize = sizeof(tme);
tme.dwFlags = TME_LEAVE;
tme.hwndTrack = hwnd;
TrackMouseEvent(&tme);
return 0;
}

case WM_MOUSELEAVE:
if (g_hoverBtn != 0) {
g_hoverBtn = 0;
Render();
}
return 0;

case WM_LBUTTONDOWN: {
int mx = (int)(short)LOWORD(lp);
int my = (int)(short)HIWORD(lp);
int btn = HitTestButton(mx, my);

if (btn == 1) {
DestroyWindow(hwnd);
return 0;
} else if (btn == 2) {
g_windowVisible = FALSE;
ShowWindow(hwnd, SW_HIDE);
return 0;
} else if (btn == 3) {
ToggleMode();
return 0;
}

if (GetMessageTime() - (LONG)g_lastClick < GetDoubleClickTime()) {
g_lastClick = 0;
ToggleMode();
} else {
g_lastClick = GetMessageTime();
g_dragging = TRUE;
GetCursorPos(&g_dragAnchor);
SetCapture(hwnd);
}
return 0;
}

case WM_LBUTTONUP:
if (g_dragging) {
g_dragging = FALSE;
ReleaseCapture();
}
return 0;

case WM_RBUTTONUP:
ShowTrayMenu();
return 0;

case WM_SETTINGCHANGE:
g_lightTheme = IsSystemLightTheme();
Render();
return 0;

case WM_DESTROY:
KillTimer(hwnd, TIMER_DISPLAY);
KillTimer(hwnd, TIMER_RESYNC);
KillTimer(hwnd, TIMER_FAILMSG);
KillTimer(hwnd, TIMER_NORMAL_RESYNC);
RemoveTrayIcon();
PostQuitMessage(0);
return 0;
}
return DefWindowProcW(hwnd, msg, wp, lp);
}

// ---------------- 初始化 / 清理 ----------------
static BOOL InitGdiplusResources(void) {
    HDC screenDC = GetDC(NULL);
    g_memDC = CreateCompatibleDC(screenDC);
    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = CANVAS_W;
    bmi.bmiHeader.biHeight      = -CANVAS_H;
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    g_hBitmap = CreateDIBSection(screenDC, &bmi, DIB_RGB_COLORS, &g_pBits, NULL, 0);
    ReleaseDC(NULL, screenDC);
    if (!g_memDC || !g_hBitmap) return FALSE;
    g_oldBmp = (HBITMAP)SelectObject(g_memDC, g_hBitmap);
    return TRUE;
}

static void CleanupGdiplusResources(void) {
    if (g_memDC && g_oldBmp) SelectObject(g_memDC, g_oldBmp);
    if (g_hBitmap) DeleteObject(g_hBitmap);
    if (g_memDC)   DeleteDC(g_memDC);
    g_memDC = NULL; g_hBitmap = NULL; g_oldBmp = NULL; g_pBits = NULL;
}

// ---------------- WinMain ----------------
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR cmd, int show) {
(void)hPrev; (void)cmd; (void)show;

// ---- 单实例锁 ----
HANDLE hMutex = CreateMutexW(NULL, TRUE, L"PrecisionClock_SingleInstance_Mutex");
if (hMutex && GetLastError() == ERROR_ALREADY_EXISTS) {
HWND hPrevWnd = FindWindowW(L"PrecisionClockWnd", NULL);
if (hPrevWnd) {
ShowWindow(hPrevWnd, SW_SHOWNOACTIVATE);
SetForegroundWindow(hPrevWnd);
}
CloseHandle(hMutex);
return 0;
}
// ---- 单实例锁结束 ----

LoadConfig();
g_lang = CurrentLang();
g_lightTheme = IsSystemLightTheme();

GdiplusStartupInput gsi;
if (GdiplusStartup(&g_gdiplusToken, &gsi, NULL) != Ok) return 1;

WSADATA wsa;
if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
GdiplusShutdown(g_gdiplusToken);
return 1;
}

QueryPerformanceFrequency(&g_qpcFreq);
QueryPerformanceCounter(&g_qpcBase);

WNDCLASSEXW wc = {0};
wc.cbSize        = sizeof(wc);
wc.lpfnWndProc   = WndProc;
wc.hInstance     = hInst;
wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
wc.hbrBackground = NULL;
wc.lpszClassName = L"PrecisionClockWnd";
RegisterClassExW(&wc);

g_winX = GetSystemMetrics(SM_CXSCREEN) - CANVAS_W - WINDOW_MARGIN;
g_winY = GetSystemMetrics(SM_CYSCREEN) - CANVAS_H - WINDOW_MARGIN - 60;

g_hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        L"PrecisionClockWnd", g_lang->wndTitle,
        WS_POPUP,
        g_winX, g_winY, CANVAS_W, CANVAS_H,
        NULL, NULL, hInst, NULL);

if (!g_hwnd) {
WSACleanup();
GdiplusShutdown(g_gdiplusToken);
return 1;
}

if (!InitGdiplusResources()) {
DestroyWindow(g_hwnd);
WSACleanup();
GdiplusShutdown(g_gdiplusToken);
return 1;
}

InitTrayIcon(hInst);
UpdateTrayTip();

ApplyNormalLocal();
Render();
ShowWindow(g_hwnd, SW_SHOWNOACTIVATE);

SetTimer(g_hwnd, TIMER_DISPLAY, 500, NULL);
SetTimer(g_hwnd, TIMER_NORMAL_RESYNC, NORMAL_RESYNC_MS, NULL);
StartNtpTask(0);

MSG msg;
while (GetMessageW(&msg, NULL, 0, 0) > 0) {
TranslateMessage(&msg);
DispatchMessageW(&msg);
}

CleanupGdiplusResources();
WSACleanup();
GdiplusShutdown(g_gdiplusToken);
return (int)msg.wParam;
}