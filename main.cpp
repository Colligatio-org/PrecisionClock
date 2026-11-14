#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <gdiplus.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <io.h>

using namespace Gdiplus;

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "advapi32.lib")

#define APP_VERSION L"v2.0.0-rc1"
#define HOTKEY_TOGGLE 1
#define WM_TRAYICON (WM_APP + 1)
#define WM_NTP_DONE (WM_APP + 2)
#define WM_RECONNECT (WM_APP + 3)

#define ID_TRAY_EXIT 1002
#define ID_MENU_LOCKED 2001
#define ID_MENU_TOPMOST 2002
#define ID_MENU_SETTINGS 3000
#define ID_MENU_TIMESTAMP 3001
#define ID_MENU_HELP 3002
#define ID_MENU_ABOUT 3003

#define ID_CHK_LOCKED 4001
#define ID_CHK_TOPMOST 4002
#define ID_CHK_AUTOSTART 4003
#define ID_CHK_ENV_PATH 4004
#define ID_RADIO_HOUR_AUTO 4010
#define ID_RADIO_HOUR_12 4011
#define ID_RADIO_HOUR_24 4012
#define ID_RADIO_MS_AUTO 4020
#define ID_RADIO_MS_ON 4021
#define ID_RADIO_MS_OFF 4022
#define ID_RADIO_COLOR_AUTO 4030
#define ID_RADIO_COLOR_MAN 4031
#define ID_EDIT_COLOR 4032
#define ID_RADIO_OPACITY_AUTO 4040
#define ID_RADIO_OPACITY_MAN 4041
#define ID_EDIT_OPACITY 4042
#define ID_RADIO_NTP_AUTO 4050
#define ID_RADIO_NTP_ON 4051
#define ID_RADIO_NTP_OFF 4052
#define ID_EDIT_NTP_HOST 4053
#define ID_EDIT_TIME_OFFSET 4054
#define ID_EDIT_WIDTH 4060
#define ID_EDIT_HEIGHT 4061
#define ID_EDIT_FONT 4062
#define ID_COMBO_WEIGHT 4063
#define ID_CHK_SHADOW 4064
#define ID_EDIT_TZ 4066
#define ID_STATIC_TZ_INFO 4067
#define ID_STATIC_STATUS_INFO 4068
#define ID_COMBO_LANG 4069
#define ID_BTN_OK 4070
#define ID_BTN_CANCEL 4071
#define ID_BTN_APPLY 4072
#define ID_TIMESTAMP_EDIT 5001
#define ID_TIMESTAMP_COPY 5002

#define NTP_TIMEOUT_MS 800
#define NTP_TO_UNIX_SEC 2208988800ULL
#define FILETIME_TO_UNIX 11644473600000ULL

#define TIMER_RENDER 1
#define TIMER_NTP_SYNC 2
#define TIMER_RELOAD_SETTINGS 3
#define TIMER_SETTINGS_RTT 100

const char* HELP_TEXT_EN =
        "=== PrecisionClock v2.0.0-rc1 ===\n\n"
        "Commands:\n"
        "  PrecisionClock --help      Show this help\n"
        "  PrecisionClock --version   Show version\n"
        "  PrecisionClock --shell     Enter interactive shell\n"
        "  PrecisionClock --install   Add to PATH\n"
        "  PrecisionClock --uninstall Remove from PATH\n"
        "  PrecisionClock --quit      Quit background widget\n\n"
        "Timezone Conversion:\n"
        "  PrecisionClock 3pm EST to PST\n"
        "  PrecisionClock now tokyo in london\n"
        "  PrecisionClock time EST\n\n"
        "Output Modes:\n"
        "  PrecisionClock -1          Print current time\n"
        "  PrecisionClock -1 --ts-ms  Print timestamp\n"
        "  PrecisionClock -1 --json   Print JSON\n";

typedef struct {
    const wchar_t* title;
    const wchar_t* group_system;
    const wchar_t* chk_locked;
    const wchar_t* chk_topmost;
    const wchar_t* chk_autostart;
    const wchar_t* chk_env_path;
    const wchar_t* lang_label;
    const wchar_t* group_time;
    const wchar_t* lbl_hour;
    const wchar_t* radio_auto;
    const wchar_t* radio_12h;
    const wchar_t* radio_24h;
    const wchar_t* lbl_ms;
    const wchar_t* radio_on;
    const wchar_t* radio_off;
    const wchar_t* group_ui;
    const wchar_t* lbl_color;
    const wchar_t* lbl_custom;
    const wchar_t* lbl_opacity;
    const wchar_t* lbl_manual;
    const wchar_t* lbl_font;
    const wchar_t* lbl_weight;
    const wchar_t* weight_regular;
    const wchar_t* weight_bold;
    const wchar_t* weight_italic;
    const wchar_t* chk_shadow;
    const wchar_t* lbl_w;
    const wchar_t* lbl_h;
    const wchar_t* lbl_tz;
    const wchar_t* group_ntp;
    const wchar_t* lbl_host;
    const wchar_t* lbl_offset;
    const wchar_t* ntp_verified;
    const wchar_t* ntp_single;
    const wchar_t* ntp_conflict;
    const wchar_t* ntp_failed;
    const wchar_t* rtt_fmt;
    const wchar_t* btn_save;
    const wchar_t* btn_cancel;
    const wchar_t* btn_apply;
    const wchar_t* tz_system;
    const wchar_t* tz_conf;
    const wchar_t* menu_locked;
    const wchar_t* menu_topmost;
    const wchar_t* menu_ts;
    const wchar_t* menu_settings;
    const wchar_t* menu_exit;
    const wchar_t* menu_help;
    const wchar_t* menu_about;
    const wchar_t* ts_window_title;
    const wchar_t* ts_label;
    const wchar_t* btn_copy;
    const wchar_t* btn_copied;
    const wchar_t* help_title;
    const wchar_t* help_body;
    const wchar_t* about_title;
    const wchar_t* about_body;
} LangPack;

static const LangPack LANG_CN = {
        L"PrecisionClock", L"窗口与系统", L"锁定穿透", L"始终置顶", L"开机自启", L"添加到环境变量(PATH)", L"语言",
        L"时间格式", L"小时制:", L"自动", L"12 小时", L"24 小时", L"毫秒:", L"开启", L"关闭",
        L"外观与排版", L"颜色:", L"自定义(Hex):", L"透明度:", L"手动(0-255):", L"字体:", L"字重:", L"常规", L"粗体", L"斜体", L"文字阴影", L"宽:", L"高:", L"时区:",
        L"NTP 网络对时", L"主机:", L"手动偏移(秒):", L"验证: 3源校验通过", L"验证: 单源可用", L"验证: 源间冲突", L"验证: 连接失败", L" | RTT: %d ms",
        L"保存", L"取消", L"应用", L"系统时区偏移: UTC", L"配置时区: ",
        L"锁定并穿透", L"始终置顶", L"获取时间戳", L"设置", L"退出", L"帮助", L"关于",
        L"提取时间戳", L"Unix 时间戳 (毫秒)", L"复制", L"已复制",
        L"帮助 / Help",
        L"【终端命令】\r\n"
        L"  PrecisionClock --help      显示此帮助\r\n"
        L"  PrecisionClock --version   显示版本号\r\n"
        L"  PrecisionClock --shell     进入交互终端\r\n"
        L"  PrecisionClock --install   添加环境变量\r\n"
        L"  PrecisionClock --uninstall 移除环境变量\r\n"
        L"  PrecisionClock --quit      关闭后台挂件\r\n\r\n"
        L"【时区转换】\r\n"
        L"  PrecisionClock 3pm EST to PST\r\n"
        L"  PrecisionClock now tokyo in london\r\n"
        L"  PrecisionClock time EST\r\n\r\n"
        L"【输出模式】\r\n"
        L"  PrecisionClock -1          输出当前时间\r\n"
        L"  PrecisionClock -1 --ts-ms  输出时间戳\r\n"
        L"  PrecisionClock -1 --json   输出 JSON",
        L"关于 / About",
        L"PrecisionClock " APP_VERSION "\r\n\r\n"
        L"一个免费、开源、GPL-3.0 协议的高精度时钟工具。\r\n"
        L"采用纯 C++ 编写，Win32 原生接口，多源 NTP 交叉验证。\r\n\r\n"
        L"【免责声明】\r\n"
        L"本软件仅作通用时间参考，严禁作为任何医疗、航空、金融交易、法律时效、军事指挥等关键系统的唯一或决定性时间源。\r\n"
        L"因使用本软件、依赖其输出、或因其时间偏差/错误造成的任何直接或间接损失，作者及发布者概不承担任何责任。\r\n\r\n"
        L"如需可信时间戳，请咨询当地可信时间戳服务提供商（TSA）。\r\n\r\n"
        L"Copyright (C) 2026 Colligatio."
};

static const LangPack LANG_TW = {
        L"PrecisionClock", L"視窗與系統", L"鎖定穿透", L"始終置頂", L"開機自啟", L"加入環境變數(PATH)", L"語言",
        L"時間格式", L"小時制:", L"自動", L"12 小時", L"24 小時", L"毫秒:", L"開啟", L"關閉",
        L"外觀與排版", L"顏色:", L"自訂(Hex):", L"透明度:", L"手動(0-255):", L"字體:", L"字重:", L"常規", L"粗體", L"斜體", L"文字陰影", L"寬:", L"高:", L"時區:",
        L"NTP 網路對時", L"主機:", L"手動偏移(秒):", L"驗證: 3源校驗通過", L"驗證: 單源可用", L"驗證: 源間衝突", L"驗證: 連接失敗", L" | RTT: %d ms",
        L"保存", L"取消", L"應用", L"系統時區偏移: UTC", L"配置時區: ",
        L"鎖定並穿透", L"始終置頂", L"獲取時間戳", L"設置", L"退出", L"幫助", L"關於",
        L"提取時間戳", L"Unix 時間戳 (毫秒)", L"複製", L"已複製",
        L"幫助 / Help",
        L"【終端指令】\r\n"
        L"  PrecisionClock --help      顯示此幫助\r\n"
        L"  PrecisionClock --version   顯示版本號\r\n"
        L"  PrecisionClock --shell     進入互動終端\r\n"
        L"  PrecisionClock --install   加入環境變數\r\n"
        L"  PrecisionClock --uninstall 移除環境變數\r\n"
        L"  PrecisionClock --quit      關閉後台掛件\r\n\r\n"
        L"【時區轉換】\r\n"
        L"  PrecisionClock 3pm EST to PST\r\n"
        L"  PrecisionClock now tokyo in london\r\n"
        L"  PrecisionClock time EST\r\n\r\n"
        L"【輸出模式】\r\n"
        L"  PrecisionClock -1          輸出當前時間\r\n"
        L"  PrecisionClock -1 --ts-ms  輸出時間戳\r\n"
        L"  PrecisionClock -1 --json   輸出 JSON",
        L"關於 / About",
        L"PrecisionClock " APP_VERSION "\r\n\r\n"
        L"一個免費、開源、GPL-3.0 協議的高精度時鐘工具。\r\n"
        L"採用純 C++ 編寫，Win32 原生接口，多源 NTP 交叉驗證。\r\n\r\n"
        L"【免責聲明】\r\n"
        L"本軟體僅作通用時間參考，嚴禁作為任何醫療、航空、金融交易、法律時效、軍事指揮等關鍵系統的唯一或決定性時間源。\r\n"
        L"因使用本軟體、依賴其輸出、或因其時間偏差/錯誤造成的任何直接或間接損失，作者及發布者概不承擔任何責任。\r\n\r\n"
        L"如需可信時間戳，請諮詢當地的可信時間戳服務提供商（TSA）。\r\n\r\n"
        L"Copyright (C) 2026 Colligatio."
};

static const LangPack LANG_EN = {
        L"PrecisionClock", L"System", L"Click-Through", L"Always on Top", L"Auto Start", L"Add to PATH", L"Language",
        L"Time Format", L"Hour:", L"Auto", L"12-Hour", L"24-Hour", L"Milliseconds:", L"On", L"Off",
        L"Appearance", L"Color:", L"Custom(Hex):", L"Opacity:", L"Manual(0-255):", L"Font:", L"Weight:", L"Regular", L"Bold", L"Italic", L"Shadow", L"Width:", L"Height:", L"Time Zone:",
        L"NTP Sync", L"Host:", L"Offset(s):", L"Verified: 3 Sources", L"Single Source", L"Conflict (>50ms)", L"Sync Failed", L" | RTT: %d ms",
        L"Save", L"Cancel", L"Apply", L"System TZ Offset: UTC", L"Configured TZ: ",
        L"Click-Through", L"Always on Top", L"Get Timestamp", L"Settings", L"Exit", L"Help", L"About",
        L"Get Timestamp", L"Unix Timestamp (ms)", L"Copy", L"Copied!",
        L"Help",
        L"【Commands】\r\n"
        L"  PrecisionClock --help      Show this help\r\n"
        L"  PrecisionClock --version   Show version\r\n"
        L"  PrecisionClock --shell     Enter interactive shell\r\n"
        L"  PrecisionClock --install   Add to PATH\r\n"
        L"  PrecisionClock --uninstall Remove from PATH\r\n"
        L"  PrecisionClock --quit      Quit background widget\r\n\r\n"
        L"【Timezone Conversion】\r\n"
        L"  PrecisionClock 3pm EST to PST\r\n"
        L"  PrecisionClock now tokyo in london\r\n"
        L"  PrecisionClock time EST\r\n\r\n"
        L"【Output Modes】\r\n"
        L"  PrecisionClock -1          Print current time\r\n"
        L"  PrecisionClock -1 --ts-ms  Print timestamp\r\n"
        L"  PrecisionClock -1 --json   Print JSON",
        L"About",
        L"PrecisionClock " APP_VERSION "\r\n\r\n"
        L"A free, open-source, GPL-3.0 time reference tool.\r\n"
        L"Written in pure C++, Win32 native API, multi-source NTP cross-validation.\r\n\r\n"
        L"【Disclaimer】\r\n"
        L"This software is a general-purpose time reference only. It must not be used as the sole or decisive time source for any medical, aviation, financial trading, legal, military command, or other critical systems.\r\n"
        L"The author and publisher assume no liability for any direct or indirect damages arising from the use of this software, reliance on its output, or any time deviation or error.\r\n\r\n"
        L"For trusted timestamps, consult a local TSA provider.\r\n\r\n"
        L"Copyright (C) 2026 Colligatio."
};

static const LangPack LANG_FR = {
        L"PrecisionClock", L"Système", L"Transparence aux clics", L"Toujours au premier plan", L"Démarrage auto", L"Ajouter au PATH", L"Langue",
        L"Format de l'heure", L"Heure :", L"Auto", L"12 heures", L"24 heures", L"Millisecondes :", L"Activé", L"Désactivé",
        L"Apparence", L"Couleur :", L"Personnalisé (Hex) :", L"Opacité :", L"Manuel (0-255) :", L"Police :", L"Épaisseur :", L"Normal", L"Gras", L"Italique", L"Ombre", L"Largeur :", L"Hauteur :", L"Fuseau horaire :",
        L"Synchronisation NTP", L"Hôte :", L"Décalage (s) :", L"Vérifié : 3 sources", L"Source unique", L"Conflit (>50ms)", L"Échec de sync", L" | RTT : %d ms",
        L"Enregistrer", L"Annuler", L"Appliquer", L"Décalage TZ système : UTC", L"TZ configuré : ",
        L"Transparence aux clics", L"Toujours au premier plan", L"Obtenir le timestamp", L"Paramètres", L"Quitter", L"Aide", L"À propos",
        L"Obtenir le timestamp", L"Timestamp Unix (ms)", L"Copier", L"Copié !",
        L"Aide",
        L"【Commandes】\r\n"
        L"  PrecisionClock --help      Afficher cette aide\r\n"
        L"  PrecisionClock --version   Afficher la version\r\n"
        L"  PrecisionClock --shell     Entrer dans le shell interactif\r\n"
        L"  PrecisionClock --install   Ajouter au PATH\r\n"
        L"  PrecisionClock --uninstall Retirer du PATH\r\n"
        L"  PrecisionClock --quit      Quitter le widget\r\n\r\n"
        L"【Conversion de fuseau horaire】\r\n"
        L"  PrecisionClock 3pm EST to PST\r\n"
        L"  PrecisionClock now tokyo in london\r\n"
        L"  PrecisionClock time EST\r\n\r\n"
        L"【Modes de sortie】\r\n"
        L"  PrecisionClock -1          Afficher l'heure actuelle\r\n"
        L"  PrecisionClock -1 --ts-ms  Afficher le timestamp\r\n"
        L"  PrecisionClock -1 --json   Afficher le JSON",
        L"À propos",
        L"PrecisionClock " APP_VERSION "\r\n\r\n"
        L"Un outil de référence temporelle gratuit, open-source, sous licence GPL-3.0.\r\n"
        L"Écrit en C++ pur, API native Win32, validation croisée NTP multi-sources.\r\n\r\n"
        L"【Avertissement】\r\n"
        L"Ce logiciel n'est qu'une référence temporelle générale. Il ne doit pas être utilisé comme source de temps unique ou décisive pour des systèmes critiques (médical, aviation, trading financier, juridique, militaire, etc.).\r\n"
        L"L'auteur et l'éditeur n'assument aucune responsabilité pour les dommages directs ou indirects résultant de l'utilisation de ce logiciel.\r\n\r\n"
        L"Pour des horodatages certifiés, veuillez consulter un fournisseur TSA local.\r\n\r\n"
        L"Copyright (C) 2026 Colligatio."
};

static const LangPack LANG_JP = {
        L"PrecisionClock", L"システム", L"マウス透過", L"常に手前に表示", L"自動起動", L"環境変数(PATH)に追加", L"言語",
        L"時間フォーマット", L"時間表示:", L"自動", L"12時間", L"24時間", L"ミリ秒:", L"オン", L"オフ",
        L"外観とレイアウト", L"色:", L"カスタム(Hex):", L"不透明度:", L"手動(0-255):", L"フォント:", L"ウェイト:", L"標準", L"太字", L"斜体", L"テキストシャドウ", L"幅:", L"高さ:", L"タイムゾーン:",
        L"NTP 同期", L"ホスト:", L"手動オフセット(秒):", L"検証: 3つのソース", L"単一ソース", L"競合 (>50ms)", L"同期失敗", L" | RTT: %d ms",
        L"保存", L"キャンセル", L"適用", L"システムTZオフセット: UTC", L"設定済みTZ: ",
        L"マウス透過", L"常に手前に表示", L"タイムスタンプ取得", L"設定", L"終了", L"ヘルプ", L"バージョン情報",
        L"タイムスタンプ取得", L"Unix タイムスタンプ (ms)", L"コピー", L"コピーしました!",
        L"ヘルプ",
        L"【コマンド】\r\n"
        L"  PrecisionClock --help      このヘルプを表示\r\n"
        L"  PrecisionClock --version   バージョンを表示\r\n"
        L"  PrecisionClock --shell     対話型シェルを起動\r\n"
        L"  PrecisionClock --install   PATHに追加\r\n"
        L"  PrecisionClock --uninstall PATHから削除\r\n"
        L"  PrecisionClock --quit      バックグラウンドウィジェットを終了\r\n\r\n"
        L"【タイムゾーン変換】\r\n"
        L"  PrecisionClock 3pm EST to PST\r\n"
        L"  PrecisionClock now tokyo in london\r\n"
        L"  PrecisionClock time EST\r\n\r\n"
        L"【出力モード】\r\n"
        L"  PrecisionClock -1          現在時刻を出力\r\n"
        L"  PrecisionClock -1 --ts-ms  タイムスタンプを出力\r\n"
        L"  PrecisionClock -1 --json   JSONを出力",
        L"バージョン情報",
        L"PrecisionClock " APP_VERSION "\r\n\r\n"
        L"無料のオープンソース GPL-3.0 時刻参照ツール。\r\n"
        L"純粋な C++ で書かれ、Win32 ネイティブ API、マルチソース NTP クロス検証。\r\n\r\n"
        L"【免責事項】\r\n"
        L"本ソフトウェアは一般的な時刻参照としてのみ提供されます。医療、航空、金融取引、法務、軍事指揮などの重要なシステムの唯一または決定的な時刻源として使用しないでください。\r\n"
        L"本ソフトウェアの使用、その出力への依存、または時刻の偏差や誤りに起因する直接的または間接的な損害について、作者および発行者は一切の責任を負いません。\r\n\r\n"
        L"信頼できるタイムスタンプについては、お住まいの地域のTSA提供者にお問い合わせください。\r\n\r\n"
        L"Copyright (C) 2026 Colligatio."
};

static const LangPack* g_lang = &LANG_EN;

typedef struct {
    int width, height, winX, winY;
    int is_locked, is_topmost, autostart;
    int color_auto; char color_val[16];
    int opacity_auto; int opacity_val;
    int ms_auto; int ms_val;
    int ntp_auto; int ntp_val;
    int hour_auto; int hour_24h;
    int shadow, font_weight;
    int time_offset;
    int language;
    char font[64];
    char ntp_host[128];
    char tz[64];
} AppConfig;

AppConfig g_config;
BOOL g_is_locked = TRUE;
HWND g_hwnd = NULL;
HWND g_hSettingsWnd = NULL;
HWND g_hTimestampWnd = NULL;
wchar_t g_config_path[MAX_PATH];
ULONGLONG g_offsetMs = 0;
int g_sync_status = 0;
int g_rtt_ms = 0;
HANDLE g_mutex = NULL;
NOTIFYICONDATAW g_nid = {0};

HDC g_memDC = NULL;
HBITMAP g_hBitmap = NULL;
HBITMAP g_oldBmp = NULL;
void* g_pBits = NULL;
int g_bmpWidth = 0, g_bmpHeight = 0;
ULONG_PTR g_gdiplusToken = 0;

BOOL g_isLightTheme = FALSE;
HFONT g_hFont = NULL;

BOOL IsPathAdded() {
    HKEY hKey;
    wchar_t exePath[MAX_PATH]; GetModuleFileNameW(NULL, exePath, MAX_PATH);
    wchar_t* lastSlash = wcsrchr(exePath, L'\\');
    if (lastSlash) *(lastSlash + 1) = L'\0';
    else return FALSE;

    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Environment", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        wchar_t pathValue[8192] = {0};
        DWORD size = sizeof(pathValue);
        DWORD type = 0;
        if (RegQueryValueExW(hKey, L"Path", NULL, &type, (LPBYTE)pathValue, &size) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            if (wcsstr(pathValue, exePath) != NULL) return TRUE;
            return FALSE;
        }
        RegCloseKey(hKey);
    }
    return FALSE;
}

void AddToPath() {
    HKEY hKey;
    wchar_t exePath[MAX_PATH]; GetModuleFileNameW(NULL, exePath, MAX_PATH);
    wchar_t* lastSlash = wcsrchr(exePath, L'\\');
    if (lastSlash) *(lastSlash + 1) = L'\0';
    else return;

    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Environment", 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS) {
        wchar_t pathValue[8192] = {0};
        DWORD size = sizeof(pathValue);
        DWORD type = 0;
        RegQueryValueExW(hKey, L"Path", NULL, &type, (LPBYTE)pathValue, &size);
        if (wcsstr(pathValue, exePath) == NULL) {
            if (wcslen(pathValue) > 0 && pathValue[wcslen(pathValue)-1] != L';') wcscat(pathValue, L";");
            wcscat(pathValue, exePath);
            RegSetValueExW(hKey, L"Path", 0, REG_EXPAND_SZ, (const BYTE*)pathValue, (wcslen(pathValue) + 1) * sizeof(wchar_t));
            SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"Environment", SMTO_ABORTIFHUNG, 5000, NULL);
        }
        RegCloseKey(hKey);
    }
}

void RemoveFromPath() {
    HKEY hKey;
    wchar_t exePath[MAX_PATH]; GetModuleFileNameW(NULL, exePath, MAX_PATH);
    wchar_t* lastSlash = wcsrchr(exePath, L'\\');
    if (lastSlash) *(lastSlash + 1) = L'\0';
    else return;

    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Environment", 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS) {
        wchar_t pathValue[8192] = {0};
        DWORD size = sizeof(pathValue);
        DWORD type = 0;
        if (RegQueryValueExW(hKey, L"Path", NULL, &type, (LPBYTE)pathValue, &size) == ERROR_SUCCESS) {
            wchar_t* pos = wcsstr(pathValue, exePath);
            if (pos != NULL) {
                wchar_t newPath[8192] = {0};
                size_t prefixLen = pos - pathValue;
                wcsncpy(newPath, pathValue, prefixLen);
                if (prefixLen > 0 && newPath[prefixLen - 1] == L';') newPath[prefixLen - 1] = L'\0';
                wchar_t* suffix = pos + wcslen(exePath);
                if (*suffix == L';') suffix++;
                if (wcslen(suffix) > 0) {
                    if (prefixLen > 1) wcscat(newPath, L";");
                    wcscat(newPath, suffix);
                }
                RegSetValueExW(hKey, L"Path", 0, REG_EXPAND_SZ, (const BYTE*)newPath, (wcslen(newPath) + 1) * sizeof(wchar_t));
                SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"Environment", SMTO_ABORTIFHUNG, 5000, NULL);
            }
        }
        RegCloseKey(hKey);
    }
}

void AutoSelectNtpHost(const char* tz) {
    if (!tz) return;
    if (strstr(tz, "UTC+8") || strstr(tz, "CN") || strstr(tz, "Shanghai") || strstr(tz, "Beijing") || strstr(tz, "Asia/Shanghai")) {
        strcpy(g_config.ntp_host, "ntp.ntsc.ac.cn");
    } else if (strstr(tz, "UTC+9") || strstr(tz, "Tokyo") || strstr(tz, "JST") || strstr(tz, "Asia/Tokyo")) {
        strcpy(g_config.ntp_host, "ntp.nict.jp");
    } else if (strstr(tz, "UTC+0") || strstr(tz, "GMT") || strstr(tz, "London") || strstr(tz, "Europe/London")) {
        strcpy(g_config.ntp_host, "ntp1.npl.co.uk");
    } else if (strstr(tz, "UTC-5") || strstr(tz, "EST") || strstr(tz, "New_York") || strstr(tz, "America/New_York")) {
        strcpy(g_config.ntp_host, "time.nist.gov");
    } else if (strstr(tz, "UTC-8") || strstr(tz, "PST") || strstr(tz, "Los_Angeles") || strstr(tz, "America/Los_Angeles")) {
        strcpy(g_config.ntp_host, "time.nist.gov");
    } else {
        strcpy(g_config.ntp_host, "time.cloudflare.com");
    }
}

void CheckSystemTheme() {
    HKEY hKey;
    DWORD value = 1, size = sizeof(value);
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegQueryValueExW(hKey, L"AppsUseLightTheme", NULL, NULL, (LPBYTE)&value, &size);
        RegCloseKey(hKey);
    }
    g_isLightTheme = (value != 0);
}

ULONGLONG GetSystemTimeMs() {
    FILETIME ft; GetSystemTimeAsFileTime(&ft);
    ULARGE_INTEGER uli; uli.LowPart = ft.dwLowDateTime; uli.HighPart = ft.dwHighDateTime;
    return (uli.QuadPart / 10000ULL) - FILETIME_TO_UNIX;
}

ULONGLONG ParseNtpTimestamp(const unsigned char *p) {
    ULONGLONG sec = ((ULONGLONG)p[0] << 24) | ((ULONGLONG)p[1] << 16) | ((ULONGLONG)p[2] << 8) | (ULONGLONG)p[3];
    ULONGLONG fr  = ((ULONGLONG)p[4] << 24) | ((ULONGLONG)p[5] << 16) | ((ULONGLONG)p[6] << 8) | (ULONGLONG)p[7];
    return (sec - NTP_TO_UNIX_SEC) * 1000ULL + (fr * 1000ULL) / 4294967296ULL;
}

typedef struct { const char* host; ULONGLONG timeMs; DWORD rtt; int ok; } NtpResult;

DWORD WINAPI NtpQueryThread(LPVOID param) {
NtpResult* res = (NtpResult*)param;
SOCKET sock = INVALID_SOCKET; struct addrinfo hints, *addr = NULL;
unsigned char packet[48] = {0}; DWORD tv = NTP_TIMEOUT_MS;
packet[0] = 0x1B; memset(&hints, 0, sizeof(hints));
hints.ai_family = AF_INET; hints.ai_socktype = SOCK_DGRAM; hints.ai_protocol = IPPROTO_UDP;
if (getaddrinfo(res->host, "123", &hints, &addr) != 0) return 0;
sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
if (sock == INVALID_SOCKET) { freeaddrinfo(addr); return 0; }
setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

DWORD start = GetTickCount();
if (sendto(sock, (const char*)packet, 48, 0, addr->ai_addr, (int)addr->ai_addrlen) != SOCKET_ERROR) {
if (recvfrom(sock, (char*)packet, 48, 0, NULL, NULL) >= 48) {
res->rtt = GetTickCount() - start;
res->timeMs = ParseNtpTimestamp(packet + 32);
res->ok = 1;
}
}
closesocket(sock); freeaddrinfo(addr); return 0;
}

DWORD WINAPI NtpWorker(LPVOID lpParam) {
if (!(g_config.ntp_auto ? 1 : g_config.ntp_val)) { g_offsetMs = 0; g_sync_status = 0; return 0; }
NtpResult sources[3] = {
        {g_config.ntp_host[0] ? g_config.ntp_host : "ntp.ntsc.ac.cn", 0, 0, 0},
        {"time.cloudflare.com", 0, 0, 0},
        {"time.google.com", 0, 0, 0}
};
HANDLE threads[3];
for (int i = 0; i < 3; i++) threads[i] = CreateThread(NULL, 0, NtpQueryThread, &sources[i], 0, NULL);
WaitForMultipleObjects(3, threads, TRUE, NTP_TIMEOUT_MS + 200);
for (int i = 0; i < 3; i++) if(threads[i]) CloseHandle(threads[i]);

ULONGLONG validTimes[3]; DWORD validRtts[3]; int count = 0;
for (int i = 0; i < 3; i++) {
if (sources[i].ok) { validTimes[count] = sources[i].timeMs; validRtts[count] = sources[i].rtt; count++; }
}

if (count == 0) { g_sync_status = 0; }
else if (count == 1) { g_offsetMs = validTimes[0] - GetSystemTimeMs(); g_rtt_ms = validRtts[0]; g_sync_status = 1; }
else {
for (int i = 0; i < count - 1; i++) {
for (int j = i + 1; j < count; j++) {
if (validTimes[i] > validTimes[j]) {
ULONGLONG tmp = validTimes[i]; validTimes[i] = validTimes[j]; validTimes[j] = tmp;
DWORD tmpr = validRtts[i]; validRtts[i] = validRtts[j]; validRtts[j] = tmpr;
}
}
}
ULONGLONG maxDiff = validTimes[count - 1] - validTimes[0];
if (maxDiff > 50ULL) { g_sync_status = 2; }
else { g_offsetMs = validTimes[count / 2] - GetSystemTimeMs(); g_rtt_ms = validRtts[0]; g_sync_status = 3; }
}
PostMessage(g_hwnd, WM_NTP_DONE, 0, 0); return 0;
}

void StartNtpSync() { HANDLE h = CreateThread(NULL, 0, NtpWorker, NULL, 0, NULL); if (h) CloseHandle(h); }

const wchar_t* MapAliasToWinTz(const wchar_t* tz) {
    if (_wcsicmp(tz, L"EST") == 0 || _wcsicmp(tz, L"NYC") == 0 || _wcsicmp(tz, L"NEWYORK") == 0 || _wcsicmp(tz, L"AMERICA/NEW_YORK") == 0) return L"Eastern Standard Time";
    if (_wcsicmp(tz, L"PST") == 0 || _wcsicmp(tz, L"LA") == 0 || _wcsicmp(tz, L"SF") == 0 || _wcsicmp(tz, L"AMERICA/LOS_ANGELES") == 0) return L"Pacific Standard Time";
    if (_wcsicmp(tz, L"CST") == 0 || _wcsicmp(tz, L"CHICAGO") == 0 || _wcsicmp(tz, L"CDT") == 0) return L"Central Standard Time";
    if (_wcsicmp(tz, L"MST") == 0 || _wcsicmp(tz, L"DENVER") == 0 || _wcsicmp(tz, L"MDT") == 0) return L"Mountain Standard Time";
    if (_wcsicmp(tz, L"GMT") == 0 || _wcsicmp(tz, L"UK") == 0 || _wcsicmp(tz, L"LONDON") == 0 || _wcsicmp(tz, L"EUROPE/LONDON") == 0) return L"GMT Standard Time";
    if (_wcsicmp(tz, L"CET") == 0 || _wcsicmp(tz, L"PARIS") == 0 || _wcsicmp(tz, L"BERLIN") == 0 || _wcsicmp(tz, L"EUROPE/BERLIN") == 0) return L"W. Europe Standard Time";
    if (_wcsicmp(tz, L"MOSCOW") == 0 || _wcsicmp(tz, L"MSK") == 0 || _wcsicmp(tz, L"EUROPE/MOSCOW") == 0) return L"Russian Standard Time";
    if (_wcsicmp(tz, L"JST") == 0 || _wcsicmp(tz, L"TOKYO") == 0 || _wcsicmp(tz, L"JP") == 0 || _wcsicmp(tz, L"ASIA/TOKYO") == 0) return L"Tokyo Standard Time";
    if (_wcsicmp(tz, L"KST") == 0 || _wcsicmp(tz, L"SEOUL") == 0 || _wcsicmp(tz, L"ASIA/SEOUL") == 0) return L"Korea Standard Time";
    if (_wcsicmp(tz, L"CN") == 0 || _wcsicmp(tz, L"CST_CN") == 0 || _wcsicmp(tz, L"SHANGHAI") == 0 || _wcsicmp(tz, L"BEIJING") == 0 || _wcsicmp(tz, L"ASIA/SHANGHAI") == 0) return L"China Standard Time";
    if (_wcsicmp(tz, L"HK") == 0 || _wcsicmp(tz, L"HONGKONG") == 0 || _wcsicmp(tz, L"ASIA/HONG_KONG") == 0) return L"China Standard Time";
    if (_wcsicmp(tz, L"SG") == 0 || _wcsicmp(tz, L"SINGAPORE") == 0 || _wcsicmp(tz, L"ASIA/SINGAPORE") == 0) return L"Singapore Standard Time";
    if (_wcsicmp(tz, L"INDIA") == 0 || _wcsicmp(tz, L"IST") == 0 || _wcsicmp(tz, L"ASIA/KOLKATA") == 0) return L"India Standard Time";
    if (_wcsicmp(tz, L"KATHMANDU") == 0 || _wcsicmp(tz, L"ASIA/KATHMANDU") == 0) return L"Nepal Standard Time";
    if (_wcsicmp(tz, L"SYDNEY") == 0 || _wcsicmp(tz, L"AEST") == 0 || _wcsicmp(tz, L"AUSTRALIA/SYDNEY") == 0) return L"AUS Eastern Standard Time";
    if (_wcsicmp(tz, L"SAO_PAULO") == 0 || _wcsicmp(tz, L"AMERICA/SAO_PAULO") == 0) return L"E. South America Standard Time";
    if (_wcsicmp(tz, L"ASIA/DUBAI") == 0) return L"Arabian Standard Time";
    if (_wcsicmp(tz, L"EUROPE/ROME") == 0) return L"W. Europe Standard Time";
    if (_wcsicmp(tz, L"AFRICA/CAIRO") == 0) return L"Egypt Standard Time";
    if (_wcsicmp(tz, L"AMERICA/VANCOUVER") == 0) return L"Pacific Standard Time";
    if (_wcsicmp(tz, L"PACIFIC/CHATHAM") == 0) return L"Chatham Islands Standard Time";
    return tz;
}

int GetRealTzOffset(const wchar_t* winTzName, const SYSTEMTIME* baseTime) {
    if (!winTzName || _wcsicmp(winTzName, L"UTC") == 0) return 0;
    DYNAMIC_TIME_ZONE_INFORMATION dtzi;
    ZeroMemory(&dtzi, sizeof(dtzi));
    DWORD idx = 0;
    BOOL found = FALSE;
    while (EnumDynamicTimeZoneInformation(idx++, &dtzi) == ERROR_SUCCESS) {
        if (_wcsicmp(dtzi.TimeZoneKeyName, winTzName) == 0) { found = TRUE; break; }
    }
    if (!found) return 999;
    SYSTEMTIME stUtc = *baseTime;
    SYSTEMTIME stLocal;
    if (!SystemTimeToTzSpecificLocalTimeEx(&dtzi, &stUtc, &stLocal)) return 999;
    FILETIME ftUtc, ftLocal;
    SystemTimeToFileTime(&stUtc, &ftUtc);
    SystemTimeToFileTime(&stLocal, &ftLocal);
    ULARGE_INTEGER uUtc, uLocal;
    uUtc.LowPart = ftUtc.dwLowDateTime; uUtc.HighPart = ftUtc.dwHighDateTime;
    uLocal.LowPart = ftLocal.dwLowDateTime; uLocal.HighPart = ftLocal.dwHighDateTime;
    LONGLONG diff = (LONGLONG)uLocal.QuadPart - (LONGLONG)uUtc.QuadPart;
    return (int)(diff / 600000000LL);
}

int GetLocalTzOffset() {
    TIME_ZONE_INFORMATION tzi;
    DWORD ret = GetTimeZoneInformation(&tzi);
    int bias = -(tzi.Bias);
    if (ret == TIME_ZONE_ID_DAYLIGHT) bias -= tzi.DaylightBias;
    else bias -= tzi.StandardBias;
    return bias / 60;
}

BOOL DoTimezoneConversion(int argc, LPWSTR* argv) {
    if (argc < 3) return FALSE;
    SYSTEMTIME stNow; GetLocalTime(&stNow);
    if (argc == 3 && wcsicmp(argv[1], L"time") == 0) {
        wchar_t* tzStr = argv[2];
        if (wcsncmp(tzStr, L"GMT", 3) == 0 || wcsncmp(tzStr, L"UTC", 3) == 0) {
            int sign = (tzStr[3] == '-') ? -1 : 1;
            int h = _wtoi(tzStr + ((tzStr[3] == '+' || tzStr[3] == '-') ? 4 : 3));
            char out[128]; char tz_utf8[64];
            WideCharToMultiByte(CP_UTF8, 0, tzStr, -1, tz_utf8, 64, NULL, NULL);
            sprintf(out, "%02d:%02d %s\n", (stNow.wHour + h + 24) % 24, stNow.wMinute, tz_utf8);
            printf("%s", out); fflush(stdout); return TRUE;
        }
        wchar_t* winTz = (wchar_t*)MapAliasToWinTz(tzStr);
        int offset = GetRealTzOffset(winTz, &stNow);
        if (offset == 999) return FALSE;
        int localOffset = GetLocalTzOffset() * 60;
        int totalMin = stNow.wHour * 60 + stNow.wMinute - localOffset + offset;
        int targetHour = (totalMin / 60) % 24; if (targetHour < 0) targetHour += 24;
        int targetMin = totalMin % 60; if (targetMin < 0) targetMin += 60;
        char out[128]; char tz_utf8[64];
        WideCharToMultiByte(CP_UTF8, 0, tzStr, -1, tz_utf8, 64, NULL, NULL);
        sprintf(out, "%02d:%02d %s\n", targetHour, targetMin, tz_utf8);
        printf("%s", out); fflush(stdout); return TRUE;
    }
    int sepIdx = -1;
    for (int i = 1; i < argc; i++) {
        if (wcsicmp(argv[i], L"to") == 0 || wcsicmp(argv[i], L"in") == 0) { sepIdx = i; break; }
    }
    if (sepIdx == -1 || sepIdx + 1 >= argc) return FALSE;
    wchar_t* timeStr = argv[1]; wchar_t* fromTz = NULL; wchar_t* toTz = argv[sepIdx + 1];
    if (wcsicmp(argv[2], L"from") == 0) { if (sepIdx >= 4) fromTz = argv[3]; }
    else if (wcsicmp(argv[2], L"in") == 0) { fromTz = NULL; }
    else { if (sepIdx >= 3) fromTz = argv[2]; }

    int offsetFromMin = GetLocalTzOffset() * 60;
    if (fromTz != NULL) {
        if (wcsncmp(fromTz, L"GMT", 3) == 0 || wcsncmp(fromTz, L"UTC", 3) == 0) {
            int sign = (fromTz[3] == '-') ? -1 : 1;
            offsetFromMin = sign * _wtoi(fromTz + ((fromTz[3] == '+' || fromTz[3] == '-') ? 4 : 3)) * 60;
        } else {
            offsetFromMin = GetRealTzOffset((wchar_t*)MapAliasToWinTz(fromTz), &stNow);
            if (offsetFromMin == 999) return FALSE;
        }
    }
    int offsetToMin = 0;
    if (wcsncmp(toTz, L"GMT", 3) == 0 || wcsncmp(toTz, L"UTC", 3) == 0) {
        int sign = (toTz[3] == '-') ? -1 : 1;
        offsetToMin = sign * _wtoi(toTz + ((toTz[3] == '+' || toTz[3] == '-') ? 4 : 3)) * 60;
    } else {
        offsetToMin = GetRealTzOffset((wchar_t*)MapAliasToWinTz(toTz), &stNow);
        if (offsetToMin == 999) return FALSE;
    }
    int totalMin = 0; BOOL isNow = FALSE;
    if (wcsicmp(timeStr, L"now") == 0) { isNow = TRUE; totalMin = stNow.wHour * 60 + stNow.wMinute; }
    else {
        int hour = 0, min = 0, isPM = 0;
        if (wcsstr(timeStr, L"pm") || wcsstr(timeStr, L"PM") || wcsstr(timeStr, L"p.m.")) isPM = 1;
        if (wcsstr(timeStr, L":")) swscanf(timeStr, L"%d:%d", &hour, &min); else swscanf(timeStr, L"%d", &hour);
        if (isPM && hour != 12) hour += 12; if (!isPM && hour == 12) hour = 0;
        totalMin = hour * 60 + min;
    }
    int targetTotalMin = totalMin - offsetFromMin + offsetToMin;
    int dayShift = 0;
    while (targetTotalMin < 0) { targetTotalMin += 24 * 60; dayShift--; }
    while (targetTotalMin >= 24 * 60) { targetTotalMin -= 24 * 60; dayShift++; }
    int targetHour = targetTotalMin / 60; int targetMin = targetTotalMin % 60;
    char tz_utf8[64];
    WideCharToMultiByte(CP_UTF8, 0, toTz, -1, tz_utf8, 64, NULL, NULL);
    if (isNow) printf("%02d:%02d %s (now)", targetHour, targetMin, tz_utf8);
    else printf("%02d:%02d %s", targetHour, targetMin, tz_utf8);
    if (dayShift == 1) printf(" (+1 day)"); else if (dayShift == -1) printf(" (-1 day)");
    printf("\n"); fflush(stdout); return TRUE;
}

void GetPrecisionTime(SYSTEMTIME *st) {
    FILETIME ft; GetSystemTimeAsFileTime(&ft); ULARGE_INTEGER uli;
    uli.LowPart = ft.dwLowDateTime; uli.HighPart = ft.dwHighDateTime;
    uli.QuadPart += (g_offsetMs * 10000ULL) + ((ULONGLONG)g_config.time_offset * 10000000ULL);
    ft.dwLowDateTime = uli.LowPart; ft.dwHighDateTime = uli.HighPart;
    SYSTEMTIME utc_st; FileTimeToSystemTime(&ft, &utc_st);
    if (g_config.tz[0] == 0 || _stricmp(g_config.tz, "Local") == 0) {
        SystemTimeToTzSpecificLocalTime(NULL, &utc_st, st);
    } else if (_stricmp(g_config.tz, "UTC") == 0) {
        *st = utc_st;
    } else {
        wchar_t wTz[64]; mbstowcs(wTz, g_config.tz, 64);
        int offsetMin = GetRealTzOffset((wchar_t*)MapAliasToWinTz(wTz), &utc_st);
        if (offsetMin != 999) {
            FILETIME ftUtc; SystemTimeToFileTime(&utc_st, &ftUtc);
            ULARGE_INTEGER uUtc; uUtc.LowPart = ftUtc.dwLowDateTime; uUtc.HighPart = ftUtc.dwHighDateTime;
            uUtc.QuadPart += (LONGLONG)offsetMin * 600000000LL;
            ftUtc.dwLowDateTime = uUtc.LowPart; ftUtc.dwHighDateTime = uUtc.HighPart;
            FileTimeToSystemTime(&ftUtc, st);
        } else { SystemTimeToTzSpecificLocalTime(NULL, &utc_st, st); }
    }
}

void GetConfigPath() {
    wchar_t exePath[MAX_PATH]; GetModuleFileNameW(NULL, exePath, MAX_PATH);
    wchar_t* lastSlash = wcsrchr(exePath, L'\\');
    if (lastSlash) { *(lastSlash + 1) = L'\0'; wcscpy(g_config_path, exePath); wcscat(g_config_path, L"PrecisionClock.ini"); }
    else { wcscpy(g_config_path, L"PrecisionClock.ini"); }
}

void SetDefaultConfig() {
    g_config.width = 320; g_config.height = 100; g_config.winX = 200; g_config.winY = 200;
    g_config.is_locked = 1; g_config.is_topmost = 0; g_config.autostart = 0;
    g_config.color_auto = 1; strcpy(g_config.color_val, "#FFFFFF");
    g_config.opacity_auto = 1; g_config.opacity_val = 180;
    g_config.ms_auto = 1; g_config.ms_val = 1;
    g_config.ntp_auto = 1; g_config.ntp_val = 1;
    g_config.hour_auto = 1; g_config.hour_24h = 1;
    g_config.shadow = 0; g_config.font_weight = 1;
    g_config.time_offset = 0; g_config.language = 2;
    strcpy(g_config.font, "Consolas"); strcpy(g_config.ntp_host, "ntp.ntsc.ac.cn"); strcpy(g_config.tz, "Local");
}

void LoadConfig() {
    GetConfigPath(); SetDefaultConfig();
    HANDLE hFile = CreateFileW(g_config_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        DWORD fileSize = GetFileSize(hFile, NULL);
        if (fileSize > 0 && fileSize < 2048) {
            char* buf = (char*)malloc(fileSize + 1); DWORD bytesRead = 0;
            if (ReadFile(hFile, buf, fileSize, &bytesRead, NULL)) {
                buf[bytesRead] = '\0'; char* line = strtok(buf, "\n");
                while (line != NULL) {
                    if (strncmp(line, "width=", 6) == 0) g_config.width = atoi(line + 6);
                    else if (strncmp(line, "height=", 7) == 0) g_config.height = atoi(line + 7);
                    else if (strncmp(line, "winX=", 5) == 0) g_config.winX = atoi(line + 5);
                    else if (strncmp(line, "winY=", 5) == 0) g_config.winY = atoi(line + 5);
                    else if (strncmp(line, "is_locked=", 10) == 0) g_config.is_locked = atoi(line + 10);
                    else if (strncmp(line, "is_topmost=", 11) == 0) g_config.is_topmost = atoi(line + 11);
                    else if (strncmp(line, "autostart=", 10) == 0) g_config.autostart = atoi(line + 10);
                    else if (strncmp(line, "color_auto=", 11) == 0) g_config.color_auto = atoi(line + 11);
                    else if (strncmp(line, "color_val=", 10) == 0) { strncpy(g_config.color_val, line + 10, 15); g_config.color_val[strcspn(g_config.color_val, "\r\n")] = 0; }
                    else if (strncmp(line, "opacity_auto=", 13) == 0) g_config.opacity_auto = atoi(line + 13);
                    else if (strncmp(line, "opacity_val=", 12) == 0) g_config.opacity_val = atoi(line + 12);
                    else if (strncmp(line, "ms_auto=", 8) == 0) g_config.ms_auto = atoi(line + 8);
                    else if (strncmp(line, "ms_val=", 7) == 0) g_config.ms_val = atoi(line + 7);
                    else if (strncmp(line, "ntp_auto=", 9) == 0) g_config.ntp_auto = atoi(line + 9);
                    else if (strncmp(line, "ntp_val=", 8) == 0) g_config.ntp_val = atoi(line + 8);
                    else if (strncmp(line, "hour_auto=", 10) == 0) g_config.hour_auto = atoi(line + 10);
                    else if (strncmp(line, "hour_24h=", 9) == 0) g_config.hour_24h = atoi(line + 9);
                    else if (strncmp(line, "shadow=", 7) == 0) g_config.shadow = atoi(line + 7);
                    else if (strncmp(line, "font_weight=", 12) == 0) g_config.font_weight = atoi(line + 12);
                    else if (strncmp(line, "time_offset=", 12) == 0) g_config.time_offset = atoi(line + 12);
                    else if (strncmp(line, "language=", 9) == 0) g_config.language = atoi(line + 9);
                    else if (strncmp(line, "font=", 5) == 0) { strncpy(g_config.font, line + 5, 63); g_config.font[strcspn(g_config.font, "\r\n")] = 0; }
                    else if (strncmp(line, "ntp_host=", 9) == 0) { strncpy(g_config.ntp_host, line + 9, 127); g_config.ntp_host[strcspn(g_config.ntp_host, "\r\n")] = 0; }
                    else if (strncmp(line, "tz=", 3) == 0) { strncpy(g_config.tz, line + 3, 63); g_config.tz[strcspn(g_config.tz, "\r\n")] = 0; }
                    line = strtok(NULL, "\n");
                }
            }
            free(buf);
        }
        CloseHandle(hFile);
    }
    g_is_locked = g_config.is_locked;
    if (g_config.language == 0) g_lang = &LANG_CN; else if (g_config.language == 1) g_lang = &LANG_TW;
    else if (g_config.language == 3) g_lang = &LANG_FR; else if (g_config.language == 4) g_lang = &LANG_JP; else g_lang = &LANG_EN;
}

void SaveConfig() {
    GetConfigPath();
    if (g_hwnd && IsWindow(g_hwnd)) {
        RECT rc; GetWindowRect(g_hwnd, &rc);
        g_config.winX = rc.left; g_config.winY = rc.top; g_config.width = rc.right - rc.left; g_config.height = rc.bottom - rc.top;
    }
    char buf[2048];
    int len = sprintf(buf, "width=%d\nheight=%d\nwinX=%d\nwinY=%d\nis_locked=%d\nis_topmost=%d\nautostart=%d\ncolor_auto=%d\ncolor_val=%s\nopacity_auto=%d\nopacity_val=%d\nms_auto=%d\nms_val=%d\nntp_auto=%d\nntp_val=%d\nhour_auto=%d\nhour_24h=%d\nshadow=%d\nfont_weight=%d\ntime_offset=%d\nlanguage=%d\nfont=%s\nntp_host=%s\ntz=%s\n",
                      g_config.width, g_config.height, g_config.winX, g_config.winY, g_config.is_locked, g_config.is_topmost, g_config.autostart, g_config.color_auto, g_config.color_val, g_config.opacity_auto, g_config.opacity_val, g_config.ms_auto, g_config.ms_val, g_config.ntp_auto, g_config.ntp_val, g_config.hour_auto, g_config.hour_24h, g_config.shadow, g_config.font_weight, g_config.time_offset, g_config.language, g_config.font, g_config.ntp_host, g_config.tz);
    SetFileAttributesW(g_config_path, FILE_ATTRIBUTE_NORMAL);
    HANDLE hFile = CreateFileW(g_config_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        DWORD written = 0; WriteFile(hFile, buf, len, &written, NULL); CloseHandle(hFile); SetFileAttributesW(g_config_path, FILE_ATTRIBUTE_HIDDEN);
    }
}

void SetAutoStart() {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
        if (g_config.autostart) {
            wchar_t path[MAX_PATH]; GetModuleFileNameW(NULL, path, MAX_PATH);
            RegSetValueExW(hKey, L"PrecisionClock", 0, REG_SZ, (const BYTE*)path, (wcslen(path) + 1) * sizeof(wchar_t));
        } else { RegDeleteValueW(hKey, L"PrecisionClock"); }
        RegCloseKey(hKey);
    }
}

void CreateBitmap(int w, int h) {
    if (g_hBitmap) { SelectObject(g_memDC, g_oldBmp); DeleteObject(g_hBitmap); }
    HDC screenDC = GetDC(NULL); if (!g_memDC) g_memDC = CreateCompatibleDC(screenDC);
    BITMAPINFO bmi = {0}; bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER); bmi.bmiHeader.biWidth = w; bmi.bmiHeader.biHeight = -h; bmi.bmiHeader.biPlanes = 1; bmi.bmiHeader.biBitCount = 32; bmi.bmiHeader.biCompression = BI_RGB;
    g_hBitmap = CreateDIBSection(screenDC, &bmi, DIB_RGB_COLORS, &g_pBits, NULL, 0);
    g_oldBmp = (HBITMAP)SelectObject(g_memDC, g_hBitmap); ReleaseDC(NULL, screenDC); g_bmpWidth = w; g_bmpHeight = h;
}

void Render() {
    if (!g_hwnd) return;
    RECT rc; GetWindowRect(g_hwnd, &rc); int w = rc.right - rc.left; int h = rc.bottom - rc.top;
    if (w < 50) w = 50; if (h < 50) h = 50;
    if (w != g_bmpWidth || h != g_bmpHeight) CreateBitmap(w, h);
    memset(g_pBits, 0, w * h * 4);

    Graphics graphics(g_memDC); graphics.SetTextRenderingHint(TextRenderingHintAntiAlias); graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    int textAlpha = g_config.opacity_auto ? 180 : g_config.opacity_val;
    Color textColor;
    if (g_config.color_auto) {
        if (g_isLightTheme) textColor = Color(textAlpha, 0, 0, 0); else textColor = Color(textAlpha, 255, 255, 255);
    } else {
        int r = 255, g = 255, b = 255;
        if (g_config.color_val[0] == '#' && strlen(g_config.color_val) == 7) sscanf(g_config.color_val + 1, "%02x%02x%02x", &r, &g, &b);
        textColor = Color(textAlpha, r, g, b);
    }
    int shadowLum = g_isLightTheme ? 255 : 0;
    Color shadowColor(textAlpha, shadowLum, shadowLum, shadowLum);
    if (!g_is_locked) { SolidBrush bgBrush(Color(1, 0, 0, 0)); graphics.FillRectangle(&bgBrush, 0, 0, w, h); }

    SYSTEMTIME st; GetPrecisionTime(&st); wchar_t timeStr[128] = {0};
    BOOL use24h = g_config.hour_auto ? 1 : g_config.hour_24h;
    BOOL showMs = g_config.ms_auto ? 1 : g_config.ms_val;
    if (!use24h) {
        int hr = st.wHour % 12; if (hr == 0) hr = 12;
        if (showMs) swprintf(timeStr, 128, L"%02d:%02d:%02d.%03d ", hr, st.wMinute, st.wSecond, st.wMilliseconds);
        else swprintf(timeStr, 128, L"%02d:%02d:%02d ", hr, st.wMinute, st.wSecond);
        wcsncat(timeStr, (st.wHour < 12) ? L"AM" : L"PM", 4);
    } else {
        if (showMs) swprintf(timeStr, 128, L"%02d:%02d:%02d.%03d", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
        else swprintf(timeStr, 128, L"%02d:%02d:%02d", st.wHour, st.wMinute, st.wSecond);
    }
    wchar_t wfont[64]; mbstowcs(wfont, g_config.font, 64); FontFamily fontFamily(wfont);
    REAL baseSize = (REAL)h * 0.70f;
    INT fontStyle = FontStyleRegular;
    if (g_config.font_weight == 1) fontStyle = FontStyleBold; else if (g_config.font_weight == 2) fontStyle = FontStyleItalic;
    Font font(&fontFamily, baseSize, fontStyle, UnitPixel);
    RectF rectF(10.0f, (REAL)h * 0.15f, (REAL)w, (REAL)h);
    if (g_config.shadow) {
        RectF shadowRect(12.0f, (REAL)h * 0.15f + 2.0f, (REAL)w, (REAL)h);
        SolidBrush sbrush(shadowColor); graphics.DrawString(timeStr, -1, &font, shadowRect, NULL, &sbrush);
    }
    SolidBrush brush(textColor); graphics.DrawString(timeStr, -1, &font, rectF, NULL, &brush);
    if (!g_is_locked) {
        Color borderColor = g_isLightTheme ? Color(255, 180, 130, 20) : Color(255, 201, 162, 39);
        Pen pen(borderColor, 2.0f); graphics.DrawRectangle(&pen, 1, 1, w - 2, h - 2);
    }
    HDC screenDC2 = GetDC(NULL); POINT ptSrc = {0, 0}; SIZE size = {w, h}; POINT ptDst = {rc.left, rc.top};
    BLENDFUNCTION blend = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
    UpdateLayeredWindow(g_hwnd, screenDC2, &ptDst, &size, g_memDC, &ptSrc, 0, &blend, ULW_ALPHA); ReleaseDC(NULL, screenDC2);
}

void ApplyWindowStyles() {
    LONG_PTR exStyle = GetWindowLongPtr(g_hwnd, GWL_EXSTYLE);
    if (g_is_locked) exStyle |= WS_EX_TRANSPARENT; else exStyle &= ~WS_EX_TRANSPARENT;
    if (g_is_locked && !g_config.is_topmost) exStyle &= ~WS_EX_TOPMOST; else exStyle |= WS_EX_TOPMOST;
    SetWindowLongPtr(g_hwnd, GWL_EXSTYLE, exStyle);
    SetWindowPos(g_hwnd, (g_is_locked && !g_config.is_topmost) ? HWND_NOTOPMOST : HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

void SetChildFont(HWND hParent) {
    EnumChildWindows(hParent, [](HWND child, LPARAM lp) -> BOOL { SendMessageW(child, WM_SETFONT, (WPARAM)lp, TRUE); return TRUE; }, (LPARAM)g_hFont);
}

LRESULT CALLBACK InfoProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
switch (msg) {
case WM_CREATE: {
CREATESTRUCT* cs = (CREATESTRUCT*)lParam; const wchar_t* text = (const wchar_t*)cs->lpCreateParams;
g_hFont = CreateFontW(18, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei");
HWND hEdit = CreateWindowW(L"EDIT", text, WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | WS_BORDER, 10, 10, 520, 400, hwnd, NULL, NULL, NULL);
SendMessageW(hEdit, WM_SETFONT, (WPARAM)g_hFont, TRUE);
break;
}
case WM_SIZE: { HWND hEdit = FindWindowExW(hwnd, NULL, L"EDIT", NULL); if (hEdit) { RECT rc; GetClientRect(hwnd, &rc); SetWindowPos(hEdit, NULL, 10, 10, rc.right - 20, rc.bottom - 20, SWP_NOZORDER); } break; }
case WM_CLOSE: DestroyWindow(hwnd); break;
default: return DefWindowProcW(hwnd, msg, wParam, lParam);
}
return 0;
}

void ShowInfoWindow(const wchar_t* title, const wchar_t* text) {
    WNDCLASSW wc = {0}; wc.lpfnWndProc = InfoProc; wc.hInstance = GetModuleHandle(NULL); wc.lpszClassName = L"InfoClass"; wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    RegisterClassW(&wc);
    HWND hDlg = CreateWindowExW(WS_EX_TOPMOST | WS_EX_DLGMODALFRAME, L"InfoClass", title, WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME, (GetSystemMetrics(SM_CXSCREEN) - 560) / 2, (GetSystemMetrics(SM_CYSCREEN) - 480) / 2, 560, 480, g_hwnd, NULL, GetModuleHandle(NULL), (LPVOID)text);
    ShowWindow(hDlg, SW_SHOW); SetForegroundWindow(hDlg);
}

LRESULT CALLBACK TimestampProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
switch (msg) {
case WM_CREATE: {
g_hFont = CreateFontW(20, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei");
CreateWindowW(L"STATIC", g_lang->ts_label, WS_CHILD | WS_VISIBLE, 20, 20, 200, 25, hwnd, NULL, NULL, NULL);
ULONGLONG ts = GetSystemTimeMs() + g_offsetMs + (g_config.time_offset * 1000ULL);
char tsBuf[32]; sprintf(tsBuf, "%llu", ts);
HWND hEdit = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_READONLY | ES_CENTER, 20, 55, 180, 35, hwnd, (HMENU)ID_TIMESTAMP_EDIT, NULL, NULL);
SetWindowTextA(hEdit, tsBuf);
CreateWindowW(L"BUTTON", g_lang->btn_copy, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 210, 55, 80, 35, hwnd, (HMENU)ID_TIMESTAMP_COPY, NULL, NULL);
EnumChildWindows(hwnd, [](HWND child, LPARAM lp) -> BOOL { SendMessageW(child, WM_SETFONT, (WPARAM)lp, TRUE); return TRUE; }, (LPARAM)g_hFont);
break;
}
case WM_COMMAND:
if (LOWORD(wParam) == ID_TIMESTAMP_COPY) {
char tsBuf[32]; GetDlgItemTextA(hwnd, ID_TIMESTAMP_EDIT, tsBuf, 32);
if (OpenClipboard(hwnd)) { EmptyClipboard(); HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, strlen(tsBuf) + 1); if (hMem) { memcpy(GlobalLock(hMem), tsBuf, strlen(tsBuf) + 1); GlobalUnlock(hMem); SetClipboardData(CF_TEXT, hMem); } CloseClipboard(); }
SetDlgItemTextW(hwnd, ID_TIMESTAMP_COPY, g_lang->btn_copied);
}
break;
case WM_CLOSE: DestroyWindow(hwnd); g_hTimestampWnd = NULL; break;
default: return DefWindowProcW(hwnd, msg, wParam, lParam);
}
return 0;
}

void ShowTimestampWindow() {
    if (g_hTimestampWnd && IsWindow(g_hTimestampWnd)) { SetForegroundWindow(g_hTimestampWnd); return; }
    WNDCLASSW wc = {0}; wc.lpfnWndProc = TimestampProc; wc.hInstance = GetModuleHandle(NULL); wc.lpszClassName = L"TimestampClass"; wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    RegisterClassW(&wc);
    g_hTimestampWnd = CreateWindowExW(WS_EX_TOPMOST | WS_EX_DLGMODALFRAME, L"TimestampClass", g_lang->ts_window_title, WS_POPUP | WS_CAPTION | WS_SYSMENU, (GetSystemMetrics(SM_CXSCREEN) - 330) / 2, (GetSystemMetrics(SM_CYSCREEN) - 170) / 2, 330, 170, g_hwnd, NULL, GetModuleHandle(NULL), NULL);
    ShowWindow(g_hTimestampWnd, SW_SHOW);
}

void UpdateTzInfoText(HWND hwnd) {
wchar_t currentTz[64] = {0}; GetDlgItemTextW(hwnd, ID_EDIT_TZ, currentTz, 64);
wchar_t utcChkText[128] = {0};
if (_wcsicmp(currentTz, L"Local") == 0 || currentTz[0] == 0) {
TIME_ZONE_INFORMATION tzi; DWORD ret = GetTimeZoneInformation(&tzi);
int bias = -(tzi.Bias); if (ret == TIME_ZONE_ID_DAYLIGHT) bias -= tzi.DaylightBias; else bias -= tzi.StandardBias; bias /= 60;
if (bias >= 0) swprintf(utcChkText, 128, L"%ls+%d", g_lang->tz_system, bias); else swprintf(utcChkText, 128, L"%ls%d", g_lang->tz_system, bias);
} else { wcscpy(utcChkText, g_lang->tz_conf); wcsncat(utcChkText, currentTz, 64); }
SetDlgItemTextW(hwnd, ID_STATIC_TZ_INFO, utcChkText);
}

void UpdateStatusText(HWND hwnd) {
wchar_t statusText[256] = {0};
const wchar_t* base = g_lang->ntp_failed;
if (g_sync_status == 3) base = g_lang->ntp_verified; else if (g_sync_status == 2) base = g_lang->ntp_conflict; else if (g_sync_status == 1) base = g_lang->ntp_single;
if (g_sync_status == 1 || g_sync_status == 3) {
wchar_t rttBuf[64]; swprintf(rttBuf, 64, g_lang->rtt_fmt, g_rtt_ms); swprintf(statusText, 256, L"%ls%ls", base, rttBuf);
} else { wcscpy(statusText, base); }
SetDlgItemTextW(hwnd, ID_STATIC_STATUS_INFO, statusText);
}

LRESULT CALLBACK SettingsProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
switch (msg) {
case WM_CREATE: {
g_hFont = CreateFontW(18, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei");
int y = 10;
CreateWindowW(L"BUTTON", g_lang->group_system, WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 10, y, 840, 95, hwnd, NULL, NULL, NULL);
CreateWindowW(L"BUTTON", g_lang->chk_locked, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 20, y+28, 130, 25, hwnd, (HMENU)ID_CHK_LOCKED, NULL, NULL);
CreateWindowW(L"BUTTON", g_lang->chk_topmost, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 160, y+28, 200, 25, hwnd, (HMENU)ID_CHK_TOPMOST, NULL, NULL);
CreateWindowW(L"BUTTON", g_lang->chk_autostart, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 370, y+28, 150, 25, hwnd, (HMENU)ID_CHK_AUTOSTART, NULL, NULL);
HWND hChkEnv = CreateWindowW(L"BUTTON", g_lang->chk_env_path, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 530, y+28, 250, 25, hwnd, (HMENU)ID_CHK_ENV_PATH, NULL, NULL);
SendMessageW(hChkEnv, BM_SETCHECK, IsPathAdded() ? BST_CHECKED : BST_UNCHECKED, 0);
CreateWindowW(L"STATIC", g_lang->lang_label, WS_CHILD | WS_VISIBLE, 20, y+60, 80, 25, hwnd, NULL, NULL, NULL);
HWND hComboLang = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 100, y+58, 150, 150, hwnd, (HMENU)ID_COMBO_LANG, NULL, NULL);
SendMessageW(hComboLang, CB_ADDSTRING, 0, (LPARAM)L"简体中文"); SendMessageW(hComboLang, CB_ADDSTRING, 0, (LPARAM)L"繁體中文"); SendMessageW(hComboLang, CB_ADDSTRING, 0, (LPARAM)L"English"); SendMessageW(hComboLang, CB_ADDSTRING, 0, (LPARAM)L"Français"); SendMessageW(hComboLang, CB_ADDSTRING, 0, (LPARAM)L"日本語");
SendMessageW(hComboLang, CB_SETCURSEL, g_config.language, 0);
y += 110;
CreateWindowW(L"BUTTON", g_lang->group_time, WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 10, y, 840, 95, hwnd, NULL, NULL, NULL);
CreateWindowW(L"STATIC", g_lang->lbl_hour, WS_CHILD | WS_VISIBLE, 20, y+28, 70, 25, hwnd, NULL, NULL, NULL);
CreateWindowW(L"BUTTON", g_lang->radio_auto, WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP, 100, y+26, 80, 25, hwnd, (HMENU)ID_RADIO_HOUR_AUTO, NULL, NULL);
CreateWindowW(L"BUTTON", g_lang->radio_12h, WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, 200, y+26, 100, 25, hwnd, (HMENU)ID_RADIO_HOUR_12, NULL, NULL);
CreateWindowW(L"BUTTON", g_lang->radio_24h, WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, 320, y+26, 100, 25, hwnd, (HMENU)ID_RADIO_HOUR_24, NULL, NULL);
CreateWindowW(L"STATIC", g_lang->lbl_ms, WS_CHILD | WS_VISIBLE, 20, y+60, 100, 25, hwnd, NULL, NULL, NULL);
CreateWindowW(L"BUTTON", g_lang->radio_auto, WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP, 120, y+58, 80, 25, hwnd, (HMENU)ID_RADIO_MS_AUTO, NULL, NULL);
CreateWindowW(L"BUTTON", g_lang->radio_on, WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, 220, y+58, 80, 25, hwnd, (HMENU)ID_RADIO_MS_ON, NULL, NULL);
CreateWindowW(L"BUTTON", g_lang->radio_off, WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, 320, y+58, 80, 25, hwnd, (HMENU)ID_RADIO_MS_OFF, NULL, NULL);
y += 110;
CreateWindowW(L"BUTTON", g_lang->group_ui, WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 10, y, 840, 185, hwnd, NULL, NULL, NULL);
CreateWindowW(L"STATIC", g_lang->lbl_color, WS_CHILD | WS_VISIBLE, 20, y+28, 80, 25, hwnd, NULL, NULL, NULL);
CreateWindowW(L"BUTTON", g_lang->radio_auto, WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP, 100, y+26, 80, 25, hwnd, (HMENU)ID_RADIO_COLOR_AUTO, NULL, NULL);
CreateWindowW(L"BUTTON", g_lang->lbl_custom, WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, 200, y+26, 170, 25, hwnd, (HMENU)ID_RADIO_COLOR_MAN, NULL, NULL);
HWND hEditColor = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 380, y+26, 90, 25, hwnd, (HMENU)ID_EDIT_COLOR, NULL, NULL);
SetWindowTextA(hEditColor, g_config.color_val);
CreateWindowW(L"STATIC", g_lang->lbl_opacity, WS_CHILD | WS_VISIBLE, 20, y+60, 80, 25, hwnd, NULL, NULL, NULL);
CreateWindowW(L"BUTTON", g_lang->radio_auto, WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP, 100, y+58, 80, 25, hwnd, (HMENU)ID_RADIO_OPACITY_AUTO, NULL, NULL);
CreateWindowW(L"BUTTON", g_lang->lbl_manual, WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, 200, y+58, 170, 25, hwnd, (HMENU)ID_RADIO_OPACITY_MAN, NULL, NULL);
HWND hEditOpac = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER, 380, y+58, 90, 25, hwnd, (HMENU)ID_EDIT_OPACITY, NULL, NULL);
char buf[32]; sprintf(buf, "%d", g_config.opacity_val); SetWindowTextA(hEditOpac, buf);
CreateWindowW(L"STATIC", g_lang->lbl_font, WS_CHILD | WS_VISIBLE, 20, y+92, 60, 25, hwnd, NULL, NULL, NULL);
HWND hEditFont = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 80, y+92, 130, 25, hwnd, (HMENU)ID_EDIT_FONT, NULL, NULL);
SetWindowTextA(hEditFont, g_config.font);
CreateWindowW(L"STATIC", g_lang->lbl_weight, WS_CHILD | WS_VISIBLE, 230, y+92, 80, 25, hwnd, NULL, NULL, NULL);
HWND hComboW = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 310, y+92, 110, 100, hwnd, (HMENU)ID_COMBO_WEIGHT, NULL, NULL);
SendMessageW(hComboW, CB_ADDSTRING, 0, (LPARAM)g_lang->weight_regular); SendMessageW(hComboW, CB_ADDSTRING, 0, (LPARAM)g_lang->weight_bold); SendMessageW(hComboW, CB_ADDSTRING, 0, (LPARAM)g_lang->weight_italic);
SendMessageW(hComboW, CB_SETCURSEL, g_config.font_weight, 0);
CreateWindowW(L"BUTTON", g_lang->chk_shadow, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 440, y+92, 150, 25, hwnd, (HMENU)ID_CHK_SHADOW, NULL, NULL);
CreateWindowW(L"STATIC", g_lang->lbl_w, WS_CHILD | WS_VISIBLE, 20, y+128, 70, 25, hwnd, NULL, NULL, NULL);
HWND hEditW = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER, 90, y+128, 60, 25, hwnd, (HMENU)ID_EDIT_WIDTH, NULL, NULL);
sprintf(buf, "%d", g_config.width); SetWindowTextA(hEditW, buf);
CreateWindowW(L"STATIC", g_lang->lbl_h, WS_CHILD | WS_VISIBLE, 170, y+128, 70, 25, hwnd, NULL, NULL, NULL);
HWND hEditH = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER, 240, y+128, 60, 25, hwnd, (HMENU)ID_EDIT_HEIGHT, NULL, NULL);
sprintf(buf, "%d", g_config.height); SetWindowTextA(hEditH, buf);
CreateWindowW(L"STATIC", g_lang->lbl_tz, WS_CHILD | WS_VISIBLE, 320, y+128, 120, 25, hwnd, NULL, NULL, NULL);
HWND hEditTz = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 450, y+128, 160, 25, hwnd, (HMENU)ID_EDIT_TZ, NULL, NULL);
SetWindowTextA(hEditTz, g_config.tz);
CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE, 630, y+130, 200, 20, hwnd, (HMENU)ID_STATIC_TZ_INFO, NULL, NULL);
UpdateTzInfoText(hwnd);
y += 200;
CreateWindowW(L"BUTTON", g_lang->group_ntp, WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 10, y, 840, 120, hwnd, NULL, NULL, NULL);
CreateWindowW(L"BUTTON", g_lang->radio_auto, WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP, 20, y+28, 80, 25, hwnd, (HMENU)ID_RADIO_NTP_AUTO, NULL, NULL);
CreateWindowW(L"BUTTON", g_lang->radio_on, WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, 120, y+28, 80, 25, hwnd, (HMENU)ID_RADIO_NTP_ON, NULL, NULL);
CreateWindowW(L"BUTTON", g_lang->radio_off, WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, 220, y+28, 80, 25, hwnd, (HMENU)ID_RADIO_NTP_OFF, NULL, NULL);
CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE, 320, y+30, 400, 20, hwnd, (HMENU)ID_STATIC_STATUS_INFO, NULL, NULL);
UpdateStatusText(hwnd);
CreateWindowW(L"STATIC", g_lang->lbl_host, WS_CHILD | WS_VISIBLE, 20, y+60, 60, 25, hwnd, NULL, NULL, NULL);
HWND hEditNtpHost = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 80, y+60, 180, 25, hwnd, (HMENU)ID_EDIT_NTP_HOST, NULL, NULL);
SetWindowTextA(hEditNtpHost, g_config.ntp_host);
CreateWindowW(L"STATIC", g_lang->lbl_offset, WS_CHILD | WS_VISIBLE, 280, y+60, 120, 25, hwnd, NULL, NULL, NULL);
HWND hEditOffset = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER, 400, y+60, 80, 25, hwnd, (HMENU)ID_EDIT_TIME_OFFSET, NULL, NULL);
sprintf(buf, "%d", g_config.time_offset); SetWindowTextA(hEditOffset, buf);
y += 140;
CreateWindowW(L"BUTTON", g_lang->btn_save, WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 235, y, 130, 40, hwnd, (HMENU)ID_BTN_OK, NULL, NULL);
CreateWindowW(L"BUTTON", g_lang->btn_apply, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 385, y, 100, 40, hwnd, (HMENU)ID_BTN_APPLY, NULL, NULL);
CreateWindowW(L"BUTTON", g_lang->btn_cancel, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 505, y, 100, 40, hwnd, (HMENU)ID_BTN_CANCEL, NULL, NULL);
SetChildFont(hwnd);
SendDlgItemMessage(hwnd, ID_CHK_LOCKED, BM_SETCHECK, g_config.is_locked ? BST_CHECKED : BST_UNCHECKED, 0);
SendDlgItemMessage(hwnd, ID_CHK_TOPMOST, BM_SETCHECK, g_config.is_topmost ? BST_CHECKED : BST_UNCHECKED, 0);
SendDlgItemMessage(hwnd, ID_CHK_AUTOSTART, BM_SETCHECK, g_config.autostart ? BST_CHECKED : BST_UNCHECKED, 0);
SendDlgItemMessage(hwnd, ID_CHK_SHADOW, BM_SETCHECK, g_config.shadow ? BST_CHECKED : BST_UNCHECKED, 0);
SendDlgItemMessage(hwnd, g_config.hour_auto ? ID_RADIO_HOUR_AUTO : (g_config.hour_24h ? ID_RADIO_HOUR_24 : ID_RADIO_HOUR_12), BM_SETCHECK, BST_CHECKED, 0);
SendDlgItemMessage(hwnd, g_config.ms_auto ? ID_RADIO_MS_AUTO : (g_config.ms_val ? ID_RADIO_MS_ON : ID_RADIO_MS_OFF), BM_SETCHECK, BST_CHECKED, 0);
SendDlgItemMessage(hwnd, g_config.color_auto ? ID_RADIO_COLOR_AUTO : ID_RADIO_COLOR_MAN, BM_SETCHECK, BST_CHECKED, 0);
EnableWindow(hEditColor, !g_config.color_auto);
SendDlgItemMessage(hwnd, g_config.opacity_auto ? ID_RADIO_OPACITY_AUTO : ID_RADIO_OPACITY_MAN, BM_SETCHECK, BST_CHECKED, 0);
EnableWindow(hEditOpac, !g_config.opacity_auto);
SendDlgItemMessage(hwnd, g_config.ntp_auto ? ID_RADIO_NTP_AUTO : (g_config.ntp_val ? ID_RADIO_NTP_ON : ID_RADIO_NTP_OFF), BM_SETCHECK, BST_CHECKED, 0);
SetTimer(hwnd, TIMER_SETTINGS_RTT, 1000, NULL);
break;
}
case WM_TIMER: if (wParam == TIMER_SETTINGS_RTT) UpdateStatusText(hwnd); break;
case WM_COMMAND: {
int wmId = LOWORD(wParam); int wmEvent = HIWORD(wParam);
if (wmId == ID_EDIT_TZ && wmEvent == EN_CHANGE) {
UpdateTzInfoText(hwnd); char newTz[64]; GetDlgItemTextA(hwnd, ID_EDIT_TZ, newTz, 64); AutoSelectNtpHost(newTz); SetDlgItemTextA(hwnd, ID_EDIT_NTP_HOST, g_config.ntp_host);
}
if (wmEvent == BN_CLICKED) {
if (wmId == ID_RADIO_COLOR_AUTO) EnableWindow(GetDlgItem(hwnd, ID_EDIT_COLOR), FALSE);
if (wmId == ID_RADIO_COLOR_MAN)  EnableWindow(GetDlgItem(hwnd, ID_EDIT_COLOR), TRUE);
if (wmId == ID_RADIO_OPACITY_AUTO) EnableWindow(GetDlgItem(hwnd, ID_EDIT_OPACITY), FALSE);
if (wmId == ID_RADIO_OPACITY_MAN)  EnableWindow(GetDlgItem(hwnd, ID_EDIT_OPACITY), TRUE);
}
if (wmId == ID_BTN_CANCEL) DestroyWindow(hwnd);
if (wmId == ID_BTN_OK || wmId == ID_BTN_APPLY) {
char buf[128];
BOOL chkEnv = SendDlgItemMessage(hwnd, ID_CHK_ENV_PATH, BM_GETCHECK, 0, 0) == BST_CHECKED; BOOL isEnvAdded = IsPathAdded();
if (chkEnv && !isEnvAdded) AddToPath(); else if (!chkEnv && isEnvAdded) RemoveFromPath();
int newLang = SendDlgItemMessage(hwnd, ID_COMBO_LANG, CB_GETCURSEL, 0, 0); BOOL langChanged = (newLang != g_config.language); g_config.language = newLang;
if (langChanged) {
if (newLang == 0) g_lang = &LANG_CN; else if (newLang == 1) g_lang = &LANG_TW; else if (newLang == 3) g_lang = &LANG_FR; else if (newLang == 4) g_lang = &LANG_JP; else g_lang = &LANG_EN;
}
g_config.is_locked = SendDlgItemMessage(hwnd, ID_CHK_LOCKED, BM_GETCHECK, 0, 0) == BST_CHECKED;
g_config.is_topmost = SendDlgItemMessage(hwnd, ID_CHK_TOPMOST, BM_GETCHECK, 0, 0) == BST_CHECKED;
g_config.autostart = SendDlgItemMessage(hwnd, ID_CHK_AUTOSTART, BM_GETCHECK, 0, 0) == BST_CHECKED;
g_config.shadow = SendDlgItemMessage(hwnd, ID_CHK_SHADOW, BM_GETCHECK, 0, 0) == BST_CHECKED;
g_is_locked = g_config.is_locked;
if (SendDlgItemMessage(hwnd, ID_RADIO_HOUR_AUTO, BM_GETCHECK, 0, 0) == BST_CHECKED) g_config.hour_auto = 1; else { g_config.hour_auto = 0; g_config.hour_24h = SendDlgItemMessage(hwnd, ID_RADIO_HOUR_24, BM_GETCHECK, 0, 0) == BST_CHECKED; }
if (SendDlgItemMessage(hwnd, ID_RADIO_MS_AUTO, BM_GETCHECK, 0, 0) == BST_CHECKED) g_config.ms_auto = 1; else { g_config.ms_auto = 0; g_config.ms_val = SendDlgItemMessage(hwnd, ID_RADIO_MS_ON, BM_GETCHECK, 0, 0) == BST_CHECKED; }
if (SendDlgItemMessage(hwnd, ID_RADIO_COLOR_MAN, BM_GETCHECK, 0, 0) == BST_CHECKED) {
GetDlgItemTextA(hwnd, ID_EDIT_COLOR, buf, 16); if (strlen(buf) == 0 || buf[0] != '#') { MessageBoxW(hwnd, L"Invalid Hex Color!", L"Warning", MB_OK | MB_ICONWARNING); g_config.color_auto = 1; } else { g_config.color_auto = 0; strcpy(g_config.color_val, buf); }
} else g_config.color_auto = 1;
if (SendDlgItemMessage(hwnd, ID_RADIO_OPACITY_MAN, BM_GETCHECK, 0, 0) == BST_CHECKED) {
GetDlgItemTextA(hwnd, ID_EDIT_OPACITY, buf, 16); if (strlen(buf) == 0) { MessageBoxW(hwnd, L"Opacity cannot be empty!", L"Warning", MB_OK | MB_ICONWARNING); g_config.opacity_auto = 1; } else { g_config.opacity_auto = 0; g_config.opacity_val = atoi(buf); }
} else g_config.opacity_auto = 1;
if (SendDlgItemMessage(hwnd, ID_RADIO_NTP_AUTO, BM_GETCHECK, 0, 0) == BST_CHECKED) g_config.ntp_auto = 1; else { g_config.ntp_auto = 0; g_config.ntp_val = SendDlgItemMessage(hwnd, ID_RADIO_NTP_ON, BM_GETCHECK, 0, 0) == BST_CHECKED; }
GetDlgItemTextA(hwnd, ID_EDIT_WIDTH, buf, 16); if (strlen(buf) > 0) g_config.width = atoi(buf);
GetDlgItemTextA(hwnd, ID_EDIT_HEIGHT, buf, 16); if (strlen(buf) > 0) g_config.height = atoi(buf);
GetDlgItemTextA(hwnd, ID_EDIT_FONT, g_config.font, 64);
GetDlgItemTextA(hwnd, ID_EDIT_NTP_HOST, g_config.ntp_host, 128);
GetDlgItemTextA(hwnd, ID_EDIT_TIME_OFFSET, buf, 16); if (strlen(buf) > 0) g_config.time_offset = atoi(buf);
GetDlgItemTextA(hwnd, ID_EDIT_TZ, g_config.tz, 64);
g_config.font_weight = SendDlgItemMessage(hwnd, ID_COMBO_WEIGHT, CB_GETCURSEL, 0, 0);
SetAutoStart(); SetWindowPos(g_hwnd, NULL, 0, 0, g_config.width, g_config.height, SWP_NOMOVE | SWP_NOZORDER); ApplyWindowStyles(); SaveConfig();
if (langChanged) {
if (wmId == ID_BTN_APPLY) MessageBoxW(hwnd, L"Language applied. Settings window will refresh.", L"Info", MB_OK | MB_ICONINFORMATION);
DestroyWindow(hwnd); SetTimer(g_hwnd, TIMER_RELOAD_SETTINGS, 50, NULL);
} else {
StartNtpSync(); Render();
if (wmId == ID_BTN_OK) DestroyWindow(hwnd);
}
}
break;
}
case WM_CLOSE: DestroyWindow(hwnd); g_hSettingsWnd = NULL; break;
default: return DefWindowProcW(hwnd, msg, wParam, lParam);
}
return 0;
}

void ShowSettingsWindow() {
    if (g_hwnd && IsWindow(g_hwnd)) { RECT rc; GetWindowRect(g_hwnd, &rc); g_config.width = rc.right - rc.left; g_config.height = rc.bottom - rc.top; }
    if (g_hSettingsWnd && IsWindow(g_hSettingsWnd)) { SetForegroundWindow(g_hSettingsWnd); return; }
    WNDCLASSW wc = {0}; wc.lpfnWndProc = SettingsProc; wc.hInstance = GetModuleHandle(NULL); wc.lpszClassName = L"SettingsClass"; wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    RegisterClassW(&wc);
    g_hSettingsWnd = CreateWindowExW(WS_EX_TOPMOST | WS_EX_DLGMODALFRAME, L"SettingsClass", g_lang->title, WS_POPUP | WS_CAPTION | WS_SYSMENU, (GetSystemMetrics(SM_CXSCREEN) - 880) / 2, (GetSystemMetrics(SM_CYSCREEN) - 700) / 2, 880, 700, g_hwnd, NULL, GetModuleHandle(NULL), NULL);
    ShowWindow(g_hSettingsWnd, SW_SHOW);
}

void ParseCmdLine(LPWSTR cmdLine) {
    int argc; LPWSTR* argv = CommandLineToArgvW(cmdLine, &argc);
    if (!argv) return;
    for (int i = 1; i < argc; i++) {
        if (wcscmp(argv[i], L"--color") == 0 && i + 1 < argc) { g_config.color_auto = 0; wcstombs(g_config.color_val, argv[i+1], 15); i++; }
        else if (wcscmp(argv[i], L"--auto-color") == 0) { g_config.color_auto = 1; }
        else if (wcscmp(argv[i], L"--opacity") == 0 && i + 1 < argc) { g_config.opacity_auto = 0; g_config.opacity_val = _wtoi(argv[i+1]); i++; }
        else if (wcscmp(argv[i], L"--auto-opacity") == 0) { g_config.opacity_auto = 1; }
        else if (wcscmp(argv[i], L"--ms-on") == 0) { g_config.ms_auto = 0; g_config.ms_val = 1; }
        else if (wcscmp(argv[i], L"--ms-off") == 0) { g_config.ms_auto = 0; g_config.ms_val = 0; }
        else if (wcscmp(argv[i], L"--auto-ms") == 0) { g_config.ms_auto = 1; }
        else if (wcscmp(argv[i], L"--ntp-on") == 0) { g_config.ntp_auto = 0; g_config.ntp_val = 1; }
        else if (wcscmp(argv[i], L"--ntp-off") == 0) { g_config.ntp_auto = 0; g_config.ntp_val = 0; }
        else if (wcscmp(argv[i], L"--auto-ntp") == 0) { g_config.ntp_auto = 1; }
        else if (wcscmp(argv[i], L"--12h") == 0) { g_config.hour_auto = 0; g_config.hour_24h = 0; }
        else if (wcscmp(argv[i], L"--24h") == 0) { g_config.hour_auto = 0; g_config.hour_24h = 1; }
        else if (wcscmp(argv[i], L"--auto-hour") == 0) { g_config.hour_auto = 1; }
        else if (wcscmp(argv[i], L"--size") == 0 && i + 1 < argc) { int w, h; if (swscanf(argv[i+1], L"%dx%d", &w, &h) == 2) { g_config.width = w; g_config.height = h; } i++; }
        else if (wcscmp(argv[i], L"--host") == 0 && i + 1 < argc) { g_config.ntp_auto = 0; g_config.ntp_val = 1; wcstombs(g_config.ntp_host, argv[i+1], 127); i++; }
        else if (wcscmp(argv[i], L"--offset") == 0 && i + 1 < argc) { g_config.time_offset = _wtoi(argv[i+1]); i++; }
        else if (wcscmp(argv[i], L"--shadow-on") == 0) { g_config.shadow = 1; }
        else if (wcscmp(argv[i], L"--shadow-off") == 0) { g_config.shadow = 0; }
        else if (wcscmp(argv[i], L"--tz") == 0 && i + 1 < argc) { wcstombs(g_config.tz, argv[i+1], 63); i++; }
    }
    LocalFree(argv);
}

void ShowContextMenu(HWND hwnd) {
HMENU hMenu = CreatePopupMenu();
wchar_t statusBuf[256] = {0};
const wchar_t* base = g_lang->ntp_failed;
if (g_sync_status == 3) base = g_lang->ntp_verified; else if (g_sync_status == 2) base = g_lang->ntp_conflict; else if (g_sync_status == 1) base = g_lang->ntp_single;
if (g_sync_status == 1 || g_sync_status == 3) {
wchar_t rttBuf[64]; swprintf(rttBuf, 64, g_lang->rtt_fmt, g_rtt_ms); swprintf(statusBuf, 256, L"%ls%ls", base, rttBuf);
} else { wcscpy(statusBuf, base); }
AppendMenuW(hMenu, MF_STRING | MF_GRAYED, 0, statusBuf);
AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
AppendMenuW(hMenu, MF_STRING | (g_is_locked ? MF_CHECKED : 0), ID_MENU_LOCKED, g_lang->menu_locked);
AppendMenuW(hMenu, MF_STRING | (g_config.is_topmost ? MF_CHECKED : 0), ID_MENU_TOPMOST, g_lang->menu_topmost);
AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
AppendMenuW(hMenu, MF_STRING, ID_MENU_TIMESTAMP, g_lang->menu_ts);
AppendMenuW(hMenu, MF_STRING, ID_MENU_SETTINGS, g_lang->menu_settings);
AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
AppendMenuW(hMenu, MF_STRING, ID_MENU_HELP, g_lang->menu_help);
AppendMenuW(hMenu, MF_STRING, ID_MENU_ABOUT, g_lang->menu_about);
AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, g_lang->menu_exit);
POINT pt; GetCursorPos(&pt); SetForegroundWindow(hwnd);
int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
DestroyMenu(hMenu);
switch (cmd) {
case ID_MENU_LOCKED: g_is_locked = !g_is_locked; ApplyWindowStyles(); break;
case ID_MENU_TOPMOST: g_config.is_topmost = !g_config.is_topmost; ApplyWindowStyles(); break;
case ID_MENU_TIMESTAMP: ShowTimestampWindow(); break;
case ID_MENU_SETTINGS: ShowSettingsWindow(); break;
case ID_MENU_HELP: ShowInfoWindow(g_lang->help_title, g_lang->help_body); break;
case ID_MENU_ABOUT: ShowInfoWindow(g_lang->about_title, g_lang->about_body); break;
case ID_TRAY_EXIT: DestroyWindow(hwnd); break;
}
SaveConfig(); Render();
}

void InitTray(HWND hwnd) {
memset(&g_nid, 0, sizeof(g_nid));
g_nid.cbSize = sizeof(g_nid);
g_nid.hWnd = hwnd;
g_nid.uID = 1;
g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
g_nid.uCallbackMessage = WM_TRAYICON;

HINSTANCE hInst = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
g_nid.hIcon = (HICON)LoadImageW(hInst, L"IDI_ICON1", IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
if (!g_nid.hIcon) g_nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);

wcscpy_s(g_nid.szTip, 128, g_lang->title); // 修复：wndTitle 改为 title
Shell_NotifyIconW(NIM_ADD, &g_nid);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
switch (msg) {
case WM_NTP_DONE:
if (g_hSettingsWnd && IsWindow(g_hSettingsWnd)) UpdateStatusText(g_hSettingsWnd);
Render(); break;
case WM_NCHITTEST: {
if (g_is_locked) return HTTRANSPARENT;
POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) }; ScreenToClient(hwnd, &pt);
RECT rc; GetClientRect(hwnd, &rc); int border = 6;
bool left = pt.x < border; bool right = pt.x >= rc.right - border; bool top = pt.y < border; bool bottom = pt.y >= rc.bottom - border;
if (top && left) return HTTOPLEFT; if (top && right) return HTTOPRIGHT; if (bottom && left) return HTBOTTOMLEFT; if (bottom && right) return HTBOTTOMRIGHT;
if (left) return HTLEFT; if (right) return HTRIGHT; if (top) return HTTOP; if (bottom) return HTBOTTOM;
return HTCAPTION;
}
case WM_MOUSEWHEEL:
if (!g_is_locked) {
int delta = GET_WHEEL_DELTA_WPARAM(wParam);
if (delta > 0) { g_config.width += 10; g_config.height += 5; } else { g_config.width -= 10; g_config.height -= 5; }
if (g_config.width < 100) g_config.width = 100; if (g_config.height < 50) g_config.height = 50;
SetWindowPos(hwnd, NULL, 0, 0, g_config.width, g_config.height, SWP_NOMOVE | SWP_NOZORDER);
Render(); SaveConfig();
if (g_hSettingsWnd && IsWindow(g_hSettingsWnd)) {
char buf[32]; sprintf(buf, "%d", g_config.width); SetDlgItemTextA(g_hSettingsWnd, ID_EDIT_WIDTH, buf);
sprintf(buf, "%d", g_config.height); SetDlgItemTextA(g_hSettingsWnd, ID_EDIT_HEIGHT, buf);
}
}
break;
case WM_SETTINGCHANGE: CheckSystemTheme(); Render(); break;
case WM_COPYDATA: {
PCOPYDATASTRUCT pcds = (PCOPYDATASTRUCT)lParam;
if (pcds->dwData == 1 && pcds->lpData) {
LPWSTR cmdLine = (LPWSTR)pcds->lpData;
if (wcsstr(cmdLine, L"--quit")) DestroyWindow(hwnd);
else {
ParseCmdLine(cmdLine);
SetWindowPos(hwnd, NULL, 0, 0, g_config.width, g_config.height, SWP_NOMOVE | SWP_NOZORDER);
SaveConfig(); StartNtpSync(); Render(); g_is_locked = 0; ApplyWindowStyles(); ShowWindow(hwnd, SW_SHOW);
}
}
return 0;
}
case WM_RBUTTONUP: if (!g_is_locked) ShowContextMenu(hwnd); break;
case WM_TRAYICON:
if (LOWORD(lParam) == WM_RBUTTONUP || LOWORD(lParam) == WM_LBUTTONUP) {
ShowContextMenu(hwnd);
}
break;
case WM_HOTKEY: if (wParam == HOTKEY_TOGGLE) { g_is_locked = !g_is_locked; ApplyWindowStyles(); Render(); } break;
case WM_SIZE: Render(); break;
case WM_EXITSIZEMOVE:
SaveConfig();
if (g_hSettingsWnd && IsWindow(g_hSettingsWnd)) {
char buf[32]; sprintf(buf, "%d", g_config.width); SetDlgItemTextA(g_hSettingsWnd, ID_EDIT_WIDTH, buf);
sprintf(buf, "%d", g_config.height); SetDlgItemTextA(g_hSettingsWnd, ID_EDIT_HEIGHT, buf);
}
break;
case WM_PAINT: { PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps); EndPaint(hwnd, &ps); Render(); break; }
case WM_RECONNECT: g_is_locked = 0; ApplyWindowStyles(); ShowWindow(hwnd, SW_SHOW); Render(); break;
case WM_TIMER:
if (wParam == TIMER_RENDER) Render();
else if (wParam == TIMER_NTP_SYNC) { if (g_config.ntp_auto ? 1 : g_config.ntp_val) StartNtpSync(); }
else if (wParam == TIMER_RELOAD_SETTINGS) { KillTimer(hwnd, TIMER_RELOAD_SETTINGS); ShowSettingsWindow(); }
break;
case WM_DESTROY: {
NOTIFYICONDATAW nid_del = {0}; nid_del.cbSize = sizeof(NOTIFYICONDATAW); nid_del.hWnd = hwnd; nid_del.uID = 1;
Shell_NotifyIconW(NIM_DELETE, &nid_del); PostQuitMessage(0); break;
}
default: return DefWindowProcW(hwnd, msg, wParam, lParam);
}
return 0;
}

void RunShell() {
    char input[256];
    printf("PrecisionClock v2.0.0-rc1 Interactive Shell\n");
    printf("Type 'help' for commands, 'exit' to quit.\n\n");
    while (true) {
        printf("PrecisionClock> "); fflush(stdout);
        if (fgets(input, 256, stdin) == NULL) break;
        size_t len = strlen(input);
        if (len > 0 && input[len-1] == '\n') input[len-1] = '\0';
        if (len > 1 && input[len-2] == '\r') input[len-2] = '\0';

        if (_stricmp(input, "exit") == 0 || _stricmp(input, "quit") == 0) break;
        else if (_stricmp(input, "help") == 0 || _stricmp(input, "--help") == 0) { printf("%s\n", HELP_TEXT_EN); continue; }
        else if (_stricmp(input, "version") == 0 || _stricmp(input, "--version") == 0) { printf("PrecisionClock v2.0.0-rc1\n\n"); continue; }

        wchar_t winput[256]; mbstowcs(winput, input, 256);
        int argc = 0; wchar_t* argv[32]; wchar_t* context = NULL;
        wchar_t* token = wcstok_s(winput, L" ", &context);
        while (token != NULL && argc < 32) { argv[argc++] = token; token = wcstok_s(NULL, L" ", &context); }
        if (argc == 0) continue;
        wchar_t* fake_argv[64]; fake_argv[0] = (wchar_t*)L"PrecisionClock";
        for (int i = 0; i < argc; i++) fake_argv[i+1] = argv[i];

        if (DoTimezoneConversion(argc+1, fake_argv)) { }
        else if (argc >= 1 && (wcscmp(argv[0], L"-1") == 0 || wcscmp(argv[0], L"--once") == 0)) {
            SYSTEMTIME st; GetPrecisionTime(&st); printf("%02d:%02d:%02d\n", st.wHour, st.wMinute, st.wSecond);
        } else { printf("Unknown command: %s\n", input); }
        fflush(stdout); printf("\n");
    }
}

int main(int argc, char* argv[]) {
    LPWSTR* wargv = CommandLineToArgvW(GetCommandLineW(), &argc);

    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            if (wcscmp(wargv[i], L"--help") == 0 || wcscmp(wargv[i], L"-h") == 0) {
                printf("%s\n", HELP_TEXT_EN); fflush(stdout); LocalFree(wargv); return 0;
            } else if (wcscmp(wargv[i], L"--version") == 0 || wcscmp(wargv[i], L"-v") == 0) {
                printf("PrecisionClock v2.0.0-rc1\n"); fflush(stdout); LocalFree(wargv); return 0;
            }
        }

        BOOL is_shell_mode = FALSE;
        for (int i = 1; i < argc; i++) {
            if (wcscmp(wargv[i], L"--shell") == 0 || wcscmp(wargv[i], L"-i") == 0 || wcscmp(wargv[i], L"shell") == 0) { is_shell_mode = TRUE; break; }
        }
        if (is_shell_mode) { LoadConfig(); RunShell(); LocalFree(wargv); return 0; }

        for (int i = 1; i < argc; i++) {
            if (wcscmp(wargv[i], L"--install") == 0) {
                AddToPath(); MessageBoxW(NULL, L"环境变量添加成功！\n请重启终端（PowerShell/CMD）以生效。", L"Success", MB_OK | MB_ICONINFORMATION);
                LocalFree(wargv); return 0;
            } else if (wcscmp(wargv[i], L"--uninstall") == 0) {
                RemoveFromPath(); MessageBoxW(NULL, L"环境变量已移除！\n请重启终端以生效。", L"Uninstalled", MB_OK | MB_ICONINFORMATION);
                LocalFree(wargv); return 0;
            }
        }

        BOOL is_cli = FALSE;
        BOOL req_date = FALSE, req_iso = FALSE, req_ts = FALSE, req_ts_ms = FALSE, req_json = FALSE;
        for (int i = 1; i < argc; i++) {
            if (wcscmp(wargv[i], L"-1") == 0 || wcscmp(wargv[i], L"--once") == 0) is_cli = TRUE;
            else if (wcscmp(wargv[i], L"--date") == 0) req_date = TRUE;
            else if (wcscmp(wargv[i], L"--iso") == 0) req_iso = TRUE;
            else if (wcscmp(wargv[i], L"--ts") == 0) req_ts = TRUE;
            else if (wcscmp(wargv[i], L"--ts-ms") == 0) req_ts_ms = TRUE;
            else if (wcscmp(wargv[i], L"--json") == 0) req_json = TRUE;
        }

        if (!is_cli && argc >= 3) {
            if (DoTimezoneConversion(argc, wargv)) { LocalFree(wargv); return 0; }
            else { printf("Timezone conversion failed. Invalid syntax or unknown timezone.\n"); fflush(stdout); LocalFree(wargv); return 1; }
        }

        if (is_cli) {
            LoadConfig(); ParseCmdLine(GetCommandLineW());
            if (g_config.ntp_auto ? 1 : g_config.ntp_val) { StartNtpSync(); Sleep(500); }
            SYSTEMTIME st; GetPrecisionTime(&st);
            BOOL use24h = g_config.hour_auto ? 1 : g_config.hour_24h;
            BOOL showMs = g_config.ms_auto ? 1 : g_config.ms_val;
            if (req_json) {
                ULONGLONG ts = GetSystemTimeMs() + g_offsetMs + (g_config.time_offset * 1000ULL);
                printf("{\n  \"time\": \"%04d-%02d-%02d %02d:%02d:%02d.%03d\",\n  \"timestamp\": %llu,\n  \"tz\": \"%s\"\n}\n", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, ts, g_config.tz);
            } else if (req_ts_ms) {
                ULONGLONG ts = GetSystemTimeMs() + g_offsetMs + (g_config.time_offset * 1000ULL); printf("%llu\n", ts);
            } else if (req_ts) {
                ULONGLONG ts = (GetSystemTimeMs() + g_offsetMs + (g_config.time_offset * 1000ULL)) / 1000ULL; printf("%llu\n", ts);
            } else if (req_iso) {
                printf("%04d-%02d-%02dT%02d:%02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
                if (showMs) printf(".%03d", st.wMilliseconds); printf("\n");
            } else if (req_date) {
                printf("%04d-%02d-%02d ", st.wYear, st.wMonth, st.wDay);
                if (!use24h) {
                    int hr = st.wHour % 12; if (hr == 0) hr = 12;
                    if (showMs) printf("%02d:%02d:%02d.%03d %s\n", hr, st.wMinute, st.wSecond, st.wMilliseconds, (st.wHour < 12) ? "AM" : "PM");
                    else printf("%02d:%02d:%02d %s\n", hr, st.wMinute, st.wSecond, (st.wHour < 12) ? "AM" : "PM");
                } else {
                    if (showMs) printf("%02d:%02d:%02d.%03d\n", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
                    else printf("%02d:%02d:%02d\n", st.wHour, st.wMinute, st.wSecond);
                }
            } else {
                if (!use24h) {
                    int hr = st.wHour % 12; if (hr == 0) hr = 12;
                    if (showMs) printf("%02d:%02d:%02d.%03d %s\n", hr, st.wMinute, st.wSecond, st.wMilliseconds, (st.wHour < 12) ? "AM" : "PM");
                    else printf("%02d:%02d:%02d %s\n", hr, st.wMinute, st.wSecond, (st.wHour < 12) ? "AM" : "PM");
                } else {
                    if (showMs) printf("%02d:%02d:%02d.%03d\n", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
                    else printf("%02d:%02d:%02d\n", st.wHour, st.wMinute, st.wSecond);
                }
            }
            fflush(stdout); LocalFree(wargv); return 0;
        }

        printf("Unknown arguments. Use --help for usage.\n"); fflush(stdout); LocalFree(wargv); return 1;
    }

    LocalFree(wargv);

    HWND hConsole = GetConsoleWindow();
    if (hConsole) ShowWindow(hConsole, SW_HIDE);
    FreeConsole();

    g_mutex = CreateMutexW(NULL, TRUE, L"PrecisionClock_SingleInstance");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND old = FindWindowW(L"PrecisionClockWnd", NULL);
        if (old) {
            COPYDATASTRUCT cds = {0}; cds.dwData = 1;
            cds.cbData = (wcslen(GetCommandLineW()) + 1) * sizeof(wchar_t);
            cds.lpData = GetCommandLineW();
            SendMessageW(old, WM_COPYDATA, 0, (LPARAM)&cds);
        }
        return 0;
    }

    LoadConfig();
    HINSTANCE hInstance = GetModuleHandle(NULL);

    GdiplusStartupInput gsi; GdiplusStartup(&g_gdiplusToken, &gsi, NULL);
    CheckSystemTheme();
    WSADATA wsa; WSAStartup(MAKEWORD(2,2), &wsa);
    if (g_config.ntp_auto ? 1 : g_config.ntp_val) StartNtpSync();

    WNDCLASSW wc = {0}; wc.lpfnWndProc = WndProc; wc.hInstance = hInstance; wc.lpszClassName = L"PrecisionClockWnd";
    RegisterClassW(&wc);

    DWORD exStyle = WS_EX_LAYERED | WS_EX_TOOLWINDOW;
    if (g_is_locked) exStyle |= WS_EX_TRANSPARENT;

    g_hwnd = CreateWindowExW(exStyle, wc.lpszClassName, L"Precision Clock", WS_POPUP, g_config.winX, g_config.winY, g_config.width, g_config.height, NULL, NULL, hInstance, NULL);

    ApplyWindowStyles();
    InitTray(g_hwnd);
    RegisterHotKey(g_hwnd, HOTKEY_TOGGLE, MOD_CONTROL | MOD_ALT, 'M');

    ShowWindow(g_hwnd, SW_SHOW);
    Render(); SaveConfig();

    SetTimer(g_hwnd, TIMER_RENDER, (g_config.ms_auto ? 1 : g_config.ms_val) ? 16 : 1000, NULL);
    SetTimer(g_hwnd, TIMER_NTP_SYNC, 60000, NULL);

    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }

    UnregisterHotKey(g_hwnd, HOTKEY_TOGGLE);
    WSACleanup();
    GdiplusShutdown(g_gdiplusToken);
    return 0;
}