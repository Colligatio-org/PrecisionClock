# Precision Clock / 精密时钟

**Languages:** [简体中文](#简体中文) · [繁體中文](#繁體中文) · [English](#english) · [Deutsch](#deutsch) · [日本語](#日本語)

---

<a id="简体中文"></a>
## 简体中文

一个免费、开源、GPL-3.0 协议的桌面时间参考工具。连接多个国际可信 NTP 源，交叉验证，显示毫秒级精度时间。

**服务范围**：全球 ｜ **当前版本**：2.8.0-rc1

### 功能

- **普通模式**：精度 1 秒，优先 NTP 校时，连续失败 3 次才降级本地时间
- **精密模式**：精度 0.001 秒，强制多源交叉验证，失败即报错，绝不降级本地时间
- **多源交叉验证**：主源 + 国际备源并发查询，取中位数
- **时区支持**：中国、美国东部 / 中部 / 山地 / 太平洋、德国、日本、英国、国际
- **夏令时**：通过 Windows 时区 API 自动处理
- **5 种语言**：简体中文、繁體中文、English、Deutsch、日本語
- **跟随系统主题**：浅色 / 深色自动切换
- **桌面悬浮窗**：透明、圆角、模拟 Aero 玻璃质感
- **托盘图标 + 右键菜单**
- **单实例**：重复启动不会开新窗口
- **单文件**：无运行时依赖，双击即用

### 时间源

| 地区 | 主源 |
|---|---|
| 中国 | ntp.ntsc.ac.cn |
| 美国 | time.nist.gov |
| 德国 | ptbtime1.ptb.de |
| 日本 | ntp.nict.jp |
| 英国 | ntp1.npl.co.uk |
| 国际 | time.cloudflare.com |

**备源**：time.cloudflare.com、time.google.com、time.nist.gov

### 第三方标识声明

- NTP 服务由各官方授时机构及 Cloudflare、Google 提供
- Windows、GDI+ 为 Microsoft Corporation 商标或注册商标
- 本项目与上述机构无隶属或合作关系

### 免责

**本软件仅作通用时间参考，严禁作为任何医疗、航空、金融交易、法律时效、军事指挥等关键系统的唯一或决定性时间源。因使用本软件、依赖其输出、或因其时间偏差/错误造成的任何直接或间接损失，作者及发布者概不承担任何责任。**

如需可信时间戳，请咨询当地可信时间戳服务提供商（TSA）。

### 许可

**GPL-3.0。** 详见 [LICENSE](LICENSE)。

本项目为免费开源项目，由 Colligatio 个人品牌发布。您可以依据 GPL-3.0 许可证条款使用、修改和分发本软件，包括商业用途。本项目不收取任何费用，亦不提供任何商业担保。

### 归属

Colligatio 开源项目。图标版权归 Colligatio，不随 GPL 自动授予商标权。

### 数字签名说明

本工具未购买代码签名证书。Windows 首次运行时可能提示"未知发布者"，请点击"更多信息" → "仍要运行"。

---

<a id="繁體中文"></a>
## 繁體中文

一個免費、開源、GPL-3.0 協議的桌面時間參考工具。連接多個國際可信 NTP 源，交叉驗證，顯示毫秒級精度時間。

**服務範圍**：全球 ｜ **當前版本**：2.8.0-rc1

### 功能

- **普通模式**：精度 1 秒，優先 NTP 校時，連續失敗 3 次才降級本地時間
- **精密模式**：精度 0.001 秒，強制多源交叉驗證，失敗即報錯，絕不降級本地時間
- **多源交叉驗證**：主源 + 國際備源並發查詢，取中位數
- **時區支持**：中國、美國東部 / 中部 / 山地 / 太平洋、德國、日本、英國、國際
- **夏令時**：通過 Windows 時區 API 自動處理
- **5 種語言**：简体中文、繁體中文、English、Deutsch、日本語
- **跟隨系統主題**：淺色 / 深色自動切換
- **桌面懸浮窗**：透明、圓角、模擬 Aero 玻璃質感
- **托盤圖標 + 右鍵菜單**
- **單實例**：重複啟動不會開新窗口
- **單文件**：無運行時依賴，雙擊即用

### 時間源

| 地區 | 主源 |
|---|---|
| 中國 | ntp.ntsc.ac.cn |
| 美國 | time.nist.gov |
| 德國 | ptbtime1.ptb.de |
| 日本 | ntp.nict.jp |
| 英國 | ntp1.npl.co.uk |
| 國際 | time.cloudflare.com |

**備源**：time.cloudflare.com、time.google.com、time.nist.gov

### 第三方標識聲明

- NTP 服務由各官方授時機構及 Cloudflare、Google 提供
- Windows、GDI+ 為 Microsoft Corporation 商標或註冊商標
- 本項目與上述機構無隸屬或合作關係

### 免責

**本軟體僅作通用時間參考，嚴禁作為任何醫療、航空、金融交易、法律時效、軍事指揮等關鍵系統的唯一或決定性時間源。因使用本軟體、依賴其輸出、或因其時間偏差/錯誤造成的任何直接或間接損失，作者及發布者概不承擔任何責任。**

如需可信時間戳，請諮詢當地可信時間戳服務提供商（TSA）。

### 許可

**GPL-3.0。** 詳見 [LICENSE](LICENSE)。

本項目為免費開源項目，由 Colligatio 個人品牌發布。您可以依據 GPL-3.0 許可證條款使用、修改和分發本軟體，包括商業用途。本項目不收取任何費用，亦不提供任何商業擔保。

### 歸屬

Colligatio 開源項目。圖標版權歸 Colligatio，不隨 GPL 自動授予商標權。

### 數字簽名說明

本工具未購買代碼簽名證書。Windows 首次運行時可能提示"未知發布者"，請點擊"更多信息" → "仍要運行"。

---

<a id="english"></a>
## English

A free, open-source, GPL-3.0 desktop time reference tool. Connects to multiple trusted NTP sources, cross-validates, displays millisecond precision time.

**Service region**: Global | **Current version**: 2.8.0-rc1

### Features

- **Normal mode**: 1-second precision, NTP-first, falls back to local time only after 3 consecutive failures
- **Precision mode**: 0.001-second precision, mandatory multi-source cross-validation, fails loudly, never falls back to local time
- **Multi-source cross-validation**: primary + international fallbacks queried in parallel, median taken
- **Time zones**: China, US Eastern / Central / Mountain / Pacific, Germany, Japan, UK, International
- **DST**: automatic via Windows time zone API
- **5 languages**: 简体中文, 繁體中文, English, Deutsch, 日本語
- **System theme**: light / dark auto
- **Floating window**: transparent, rounded, simulated Aero glass
- **Tray icon + right-click menu**
- **Single instance**: no duplicate windows
- **Single file**: no runtime dependency, double-click to run

### Time Sources

| Region | Primary |
|---|---|
| China | ntp.ntsc.ac.cn |
| US | time.nist.gov |
| Germany | ptbtime1.ptb.de |
| Japan | ntp.nict.jp |
| UK | ntp1.npl.co.uk |
| International | time.cloudflare.com |

**Fallbacks**: time.cloudflare.com, time.google.com, time.nist.gov

### Third-party Trademarks

- NTP services provided by official time authorities and Cloudflare / Google.
- Windows and GDI+ are trademarks of Microsoft Corporation.
- This project is not affiliated with or endorsed by any of the above.

### Disclaimer

**This software is a general-purpose time reference only. It must not be used as the sole or decisive time source for any medical, aviation, financial trading, legal, military command, or other critical systems. The author and publisher assume no liability for any direct or indirect damages arising from the use of this software, reliance on its output, or any time deviation or error.**

For trusted timestamps, consult a local TSA provider.

### License

**GPL-3.0.** See [LICENSE](LICENSE).

This project is a free, open-source project released by the Colligatio brand. You may use, modify, and distribute this software under the terms of the GPL-3.0 license, including for commercial purposes. This project charges no fees and provides no commercial warranty.

### Attribution

Colligatio open-source project. Icon copyright belongs to Colligatio, not automatically granted as trademark rights under GPL.

### Code Signing

This tool is not code-signed. Windows may show "Unknown Publisher" on first run. Click "More info" → "Run anyway".

---

<a id="deutsch"></a>
## Deutsch

Ein kostenloses, quelloffenes GPL-3.0-Zeitreferenztool für den Desktop. Verbindet sich mit mehreren vertrauenswürdigen NTP-Quellen, kreuzvalidiert, zeigt millisekundengenaue Zeit an.

**Servicegebiet**: Global | **Aktuelle Version**: 2.8.0-rc1

### Funktionen

- **Normalmodus**: 1-Sekunden-Genauigkeit, NTP zuerst, Rückfall auf lokale Zeit nur nach 3 aufeinanderfolgenden Fehlern
- **Präzisionsmodus**: 0,001-Sekunden-Genauigkeit, obligatorische Kreuzvalidierung mehrerer Quellen, schlägt laut fehl, kein Rückfall auf lokale Zeit
- **Kreuzvalidierung mehrerer Quellen**: Primärquelle + internationale Fallbacks parallel abgefragt, Median gebildet
- **Zeitzonen**: China, USA Ost / Zentral / Mountain / Pazifik, Deutschland, Japan, UK, International
- **Sommerzeit**: automatisch über Windows-Zeitzonen-API
- **5 Sprachen**: 简体中文, 繁體中文, English, Deutsch, 日本語
- **Systemthema**: Hell / Dunkel automatisch
- **Schwebendes Fenster**: transparent, abgerundet, simuliertes Aero-Glas
- **Taskleistensymbol + Rechtsklickmenü**
- **Einzelinstanz**: keine doppelten Fenster
- **Einzelne Datei**: keine Laufzeitabhängigkeit, Doppelklick zum Ausführen

### Zeitquellen

| Region | Primär |
|---|---|
| China | ntp.ntsc.ac.cn |
| USA | time.nist.gov |
| Deutschland | ptbtime1.ptb.de |
| Japan | ntp.nict.jp |
| UK | ntp1.npl.co.uk |
| International | time.cloudflare.com |

**Fallbacks**: time.cloudflare.com, time.google.com, time.nist.gov

### Marken Dritter

- NTP-Dienste werden von offiziellen Zeitinstitutionen und Cloudflare / Google bereitgestellt.
- Windows und GDI+ sind Marken der Microsoft Corporation.
- Dieses Projekt ist mit den oben genannten Organisationen nicht verbunden.

### Haftungsausschluss

**Diese Software dient nur als allgemeine Zeitreferenz. Sie darf nicht als einzige oder entscheidende Zeitquelle für medizinische, luftfahrttechnische, finanzielle, rechtliche, militärische oder andere kritische Systeme verwendet werden. Der Autor und Herausgeber übernimmt keine Haftung für direkte oder indirekte Schäden, die durch die Nutzung dieser Software, das Vertrauen auf ihre Ausgabe oder jegliche Zeitabweichung oder Fehler entstehen.**

Für vertrauenswürdige Zeitstempel wenden Sie sich bitte an einen lokalen TSA-Anbieter.

### Lizenz

**GPL-3.0.** Siehe [LICENSE](LICENSE).

Dieses Projekt ist ein kostenloses Open-Source-Projekt, veröffentlicht unter der Marke Colligatio. Sie dürfen diese Software gemäß den Bedingungen der GPL-3.0-Lizenz verwenden, ändern und verbreiten, auch für kommerzielle Zwecke. Dieses Projekt erhebt keine Gebühren und bietet keine kommerzielle Garantie.

### Zuordnung

Colligatio Open-Source-Projekt. Das Symbol ist urheberrechtlich geschützt durch Colligatio und wird nicht automatisch als Markenrecht unter der GPL gewährt.

### Codesignierung

Dieses Tool ist nicht codesigniert. Windows zeigt beim ersten Ausführen möglicherweise "Unbekannter Herausgeber" an. Klicken Sie auf "Weitere Informationen" → "Trotzdem ausführen".

---

<a id="日本語"></a>
## 日本語

無料のオープンソース GPL-3.0 デスクトップ時刻参照ツール。複数の信頼できる NTP ソースに接続し、クロス検証し、ミリ秒精度の時刻を表示します。

**サービス地域**：グローバル ｜ **現在のバージョン**：2.8.0-rc1

### 機能

- **通常モード**：1秒精度、NTP優先、3回連続失敗後にのみローカル時刻にフォールバック
- **精密モード**：0.001秒精度、複数ソースのクロス検証必須、失敗時は大声でエラー、ローカル時刻へのフォールバックなし
- **複数ソースのクロス検証**：プライマリ + 国際フォールバックを並列照会、中央値を取得
- **タイムゾーン**：中国、米国東部 / 中部 / 山岳部 / 太平洋、ドイツ、日本、英国、国際
- **夏時間**：Windows タイムゾーン API により自動処理
- **5 言語**：简体中文, 繁體中文, English, Deutsch, 日本語
- **システムテーマ**：ライト / ダーク自動
- **フローティングウィンドウ**：透明、角丸、Aero ガラス風
- **タスクトレイアイコン + 右クリックメニュー**
- **単一インスタンス**：重複ウィンドウなし
- **単一ファイル**：ランタイム依存なし、ダブルクリックで実行

### 時刻ソース

| 地域 | プライマリ |
|---|---|
| 中国 | ntp.ntsc.ac.cn |
| 米国 | time.nist.gov |
| ドイツ | ptbtime1.ptb.de |
| 日本 | ntp.nict.jp |
| 英国 | ntp1.npl.co.uk |
| 国際 | time.cloudflare.com |

**フォールバック**：time.cloudflare.com, time.google.com, time.nist.gov

### 第三者の商標

- NTP サービスは公式時刻機関および Cloudflare / Google によって提供されています。
- Windows および GDI+ は Microsoft Corporation の商標です。
- 本プロジェクトは上記の組織と提携していません。

### 免責事項

**本ソフトウェアは一般的な時刻参照としてのみ提供されます。医療、航空、金融取引、法務、軍事指揮などの重要なシステムの唯一または決定的な時刻源として使用しないでください。本ソフトウェアの使用、その出力への依存、または時刻の偏差や誤りに起因する直接的または間接的な損害について、作者および発行者は一切の責任を負いません。**

信頼できるタイムスタンプについては、お住まいの地域の TSA 提供者にお問い合わせください。

### ライセンス

**GPL-3.0。** 詳細は [LICENSE](LICENSE) を参照。

本プロジェクトは Colligatio ブランドによって公開された無料のオープンソースプロジェクトです。GPL-3.0 ライセンスの条件に従って、本ソフトウェアを商用目的を含めて使用、変更、配布することができます。本プロジェクトは一切の料金を請求せず、商業的な保証も提供しません。

### 帰属

Colligatio オープンソースプロジェクト。アイコンの著作権は Colligatio に帰属し、GPL の下で商標権が自動的に付与されるものではありません。

### コード署名

本ツールはコード署名されていません。Windows は初回実行時に「不明な発行元」と表示する場合があります。「詳細情報」→「実行」をクリックしてください。
