# OPC導入 指南書

**〜 産業制御システム（ICS）への OPC 導入 実践ガイド 〜**

| 項目 | 内容 |
|---|---|
| **版数** | 第1版 |
| **作成日** | 2026年7月11日 |
| **対象読者** | OPC 初学者、ICS エンジニア、セキュリティ研究者 |
| **前提構成** | IEC 61850 → OPC DA → ヒストリアン（仮想環境） |

---

## 目次

- [第1章 OPC概論](#第1章-opc概論)
  - [1.1 OPCとは何か](#11-opcとは何か)
  - [1.2 OPC DA / OPC HDA / OPC A&E の違い](#12-opc-da--opc-hda--opc-ae-の違い)
  - [1.3 OPC UA とは](#13-opc-ua-とは)
  - [1.4 産業システムにおけるOPCの役割](#14-産業システムにおけるopcの役割)
  - [1.5 OPC Classic と OPC UA の比較](#15-opc-classic-と-opc-ua-の比較)
- [第2章 システム構成と要件](#第2章-システム構成と要件)
  - [2.1 全体アーキテクチャ](#21-全体アーキテクチャ)
  - [2.2 各コンポーネントの役割](#22-各コンポーネントの役割)
  - [2.3 通信プロトコルの詳細](#23-通信プロトコルの詳細)
- [第3章 環境制約と準備](#第3章-環境制約と準備)
  - [3.1 仮想化基盤の選定](#31-仮想化基盤の選定)
  - [3.2 ゲストOSの選定と制約](#32-ゲストosの選定と制約)
  - [3.3 COM/DCOM セキュリティ設定の注意点](#33-comdcom-セキュリティ設定の注意点)
  - [3.4 使用ソフトウェア一覧（無料・OSS・評価版）](#34-使用ソフトウェア一覧無料oss評価版)
- [第4章 構築手順](#第4章-構築手順)
  - [4.1 仮想環境のセットアップ](#41-仮想環境のセットアップ)
  - [4.2 OPCサーバの導入・設定](#42-opcサーバの導入設定)
  - [4.3 IEC 61850 シミュレータの構築](#43-iec-61850-シミュレータの構築)
  - [4.4 OPC DA クライアントの設定](#44-opc-da-クライアントの設定)
  - [4.5 ヒストリアン（データベース）への接続](#45-ヒストリアンデータベースへの接続)
  - [4.6 動作確認・テスト方法](#46-動作確認テスト方法)
- [第5章 トラブルシューティング](#第5章-トラブルシューティング)
  - [5.1 DCOM権限エラー](#51-dcom権限エラー)
  - [5.2 ファイアウォール設定](#52-ファイアウォール設定)
  - [5.3 よくある接続問題と対処法](#53-よくある接続問題と対処法)
  - [5.4 トラブルシューティング・チェックリスト](#54-トラブルシューティングチェックリスト)
- [付録](#付録)
  - [A. 用語集](#a-用語集)
  - [B. 参考文献・資料](#b-参考文献資料)

---

# 第1章 OPC概論

## 1.1 OPCとは何か

### OPCの定義

**OPC（Open Platform Communications）** は、産業オートメーション分野において、異なるメーカーの機器やソフトウェア間で **データを安全かつ確実にやり取りするための相互運用標準** です。

> **初学者向け解説**
> 工場には PLC（プログラマブルロジックコントローラ）、DCS（分散制御システム）、センサ、計測器など、さまざまなメーカーの機器が混在しています。以前は、上位の監視ソフト（HMI/SCADA）が各機器と通信するために、メーカーごとに個別のドライバを開発する必要がありました。OPC は、この「共通語」の役割を果たします。

### OPC の基本的な仕組み

OPC は **クライアント-サーバモデル** を採用しています。

```
┌──────────────┐          ┌──────────────┐          ┌──────────────┐
│   PLC/計測器  │◀──独自──▶│  OPC サーバ   │◀──OPC──▶│ OPC クライアント│
│ （ベンダ固有） │  プロトコル │ （"通訳"役）  │  (共通語) │（HMI/SCADA等）│
└──────────────┘          └──────────────┘          └──────────────┘
```

| 要素 | 役割 | たとえ |
|---|---|---|
| **OPC サーバ** | 機器固有のプロトコルを「OPC」という共通語に変換する | 各国語の"通訳者" |
| **OPC クライアント** | 共通語（OPC）で通訳者に話しかけ、データを取得・制御する | "利用者" |

> **出典**: OPC Foundation 公式整理。本ワークスペースの『調査報告_OPC-Classicとは.md』にて詳細に解説されています。

### OPC 規格群の歴史的経緯

| 年代 | 出来事 |
|---|---|
| **1996年** | OPC Foundation 設立。OPC DA 1.0 公開（Microsoft COM/DCOM ベース） |
| **1998–2003年** | OPC DA 2.0/3.0、OPC HDA、OPC A&E 等が順次策定 |
| **2006年** | OPC UA（Unified Architecture）の開発開始 |
| **2008年** | OPC UA 仕様の初版公開。IEC 62541 として国際標準化 |
| **2017年** | OPC UA PubSub（Publisher/Subscriber モデル）追加 |
| **2021年〜** | OPC UA v1.04 リリース。OPC UA FX（Field eXchange）等の拡張 |

---

## 1.2 OPC DA / OPC HDA / OPC A&E の違い

OPC Classic（COM/DCOM ベースの旧規格群）は、用途別に複数の仕様に分かれています。

### 比較表

| 仕様 | 正式名称 | 目的 | データの性質 |
|---|---|---|---|
| **OPC DA** | Data Access | リアルタイムのプロセスデータ（温度、圧力、流量等）を読み書きする | **現在値**（値＋タイムスタンプ＋品質） |
| **OPC HDA** | Historical Data Access | 過去のプロセスデータ（履歴・トレンド）を取得する | **履歴値**（時系列データ） |
| **OPC A&E** | Alarms & Events | アラーム（警報）やイベント（状態変化通知）を扱う | **イベント情報**（発生時刻・重大度・状態遷移） |

### OPC DA の詳細

OPC DA は OPC Classic の中核であり、最も広く使われている仕様です。

**データ構造:**
- **アイテム（タグ）**: データの最小単位。温度センサの値なら「Sensor01.Temperature」のような名前
- **グループ**: アイテムをまとめる単位。更新周期やアクティブ状態を管理
- **値の三要素**: `値（Value）` ＋ `タイムスタンプ（Timestamp）` ＋ `品質（Quality）`

**データの読み方:**
| 方式 | 説明 | 適用場面 |
|---|---|---|
| **同期読み出し（Sync Read）** | クライアントが明示的に「今の値をくれ」と要求 | 一度だけ値を確認したいとき |
| **非同期読み出し（Async Read）** | 要求を出して、結果はコールバックで受け取る | 応答待ちの間に他の処理をしたいとき |
| **購読（Subscription）** | サーバが値の変化を検知して自動通知する | 継続的にモニタリングしたいとき（効率的） |

> **出典**: 本ワークスペースの『翻訳版_Codex_OPC Unified Architecture.md』第10章（COM OPC → OPC UA マッピング）および『調査報告_OPC-Classicとは.md』より。

### OPC HDA の詳細

OPC HDA は、過去に記録されたプロセスデータを取得するための仕様です。

- **用途**: トレンド分析、品質管理、事故原因調査
- **機能例**: 指定期間のデータ取得、集計（平均・最大・最小）、補間
- **本構成での関連**: ヒストリアン（データベース）に蓄積したデータを後から参照する場面で使用

### OPC A&E の詳細

OPC A&E は、警報やイベントを管理する仕様です。

- **用途**: 温度上限超過警報、機器故障通知、操作ログ
- **イベント種類**: Simple Event（単発通知）、Tracking Event（追跡）、Condition Event（状態管理）

---

## 1.3 OPC UA とは

### OPC UA（Unified Architecture）の定義

**OPC UA** は、OPC Classic の後継として 2008 年に OPC Foundation が公開した **次世代の産業通信標準** です。Classic の機能（DA、HDA、A&E）を **単一の統合アーキテクチャ** に集約し、以下の設計目標を掲げています。

| 設計目標 | 説明 |
|---|---|
| **プラットフォーム非依存** | Windows だけでなく、Linux、組込みOS、リアルタイムOS でも動作 |
| **セキュリティの標準化** | 暗号化・認証・監査を仕様として内蔵（DCOM への委譲ではない） |
| **ファイアウォール対応** | 固定ポート（TCP 4840）による通信。動的ポート問題を解消 |
| **情報モデリング** | 「値」だけでなく「意味」まで共有できるオブジェクト指向の情報モデル |
| **サービス指向** | 抽象的なサービス定義により、特定の通信技術に依存しない |
| **スケーラビリティ** | 小型センサから ERP まで幅広い規模に対応 |

> **出典**: Wolfgang Mahnke 他著『OPC Unified Architecture』（本ワークスペース『概要_OPC Unified Architecture.md』参照）。「OPC UA は Classic OPC の機能を統合・拡張し、通信の抽象化、豊かな情報モデリング、強固なセキュリティ、標準化された相互運用性、段階的移行戦略を備えた、産業システム向けの共通情報基盤である」。

### OPC UA のアーキテクチャ（3層構造）

```
┌─────────────────────────────────────────────────────┐
│           Information Model Layer（情報モデル層）      │
│  ┌────────────┬──────────────┬────────────────────┐  │
│  │ OPC UA     │ Built-In     │ Companion Spec /   │  │
│  │ Meta Model │ Info Models  │ Vendor Extensions  │  │
│  └────────────┴──────────────┴────────────────────┘  │
├─────────────────────────────────────────────────────┤
│         Communication Model Layer（通信モデル層）      │
│  ┌──────────────────┬────────────────────────────┐  │
│  │ Client/Server    │ Publish/Subscribe (PubSub) │  │
│  └──────────────────┴────────────────────────────┘  │
├─────────────────────────────────────────────────────┤
│         Protocol Bindings Layer（プロトコル層）        │
│  ┌──────┬───────────┬───────┬──────┬──────┬──────┐  │
│  │UA TCP│ WebSocket │ HTTPS │ UDP  │ AMQP │ MQTT │  │
│  └──────┴───────────┴───────┴──────┴──────┴──────┘  │
└─────────────────────────────────────────────────────┘
```

> **出典**: 本田俊明博士論文（本ワークスペース『翻訳_Honda.md』）図2.4〜2.8 参照。

### OPC UA のセキュリティ

本田博士論文および OPC UA 仕様によれば、OPC UA のセキュリティは PKI（公開鍵基盤）と共通鍵暗号のハイブリッド方式を採用しています。

| セキュリティ機能 | 説明 |
|---|---|
| **認証（Authentication）** | アプリケーション証明書による相互認証 |
| **認可（Authorization）** | ノード単位のアクセスレベル制御 |
| **機密性（Confidentiality）** | 通信データの暗号化（AES 等） |
| **完全性（Integrity）** | メッセージの署名による改ざん検知 |
| **監査可能性（Auditability）** | 監査イベント（AuditEventType）による操作記録 |
| **否認防止（Non-Repudiation）** | 証拠の保全 |

**想定脅威:** DoS、メッセージ偽装、盗聴、メッセージ改ざん、メッセージ再送、不正形式メッセージ、サーバプロファイリング、システム乗っ取り、偽サーバ、資格情報侵害、否認/証拠破壊

> **出典**: 本田博士論文 p.33–35、図2.11

---

## 1.4 産業システムにおけるOPCの役割

### Purdue モデルにおける OPC の位置づけ

産業制御システムは一般に **Purdue Enterprise Reference Architecture（Purdue モデル）** に基づく階層構造を持ちます。OPC はこの階層間のデータ通信を担う重要な「接着剤」です。

```
┌─────────────────────────────────────────┐
│ Level 4-5:  エンタープライズ（ERP/MES）   │
│             ↕ OPC UA / Web API          │
├─────────────────────────────────────────┤
│ Level 3:    製造オペレーション管理       │
│             (MES/ヒストリアン)           │
│             ↕ OPC DA / OPC UA           │
├─────────────────────────────────────────┤
│ Level 2:    制御システム                │
│             (HMI/SCADA/DCS)             │
│             ↕ OPC DA / OPC UA           │
├─────────────────────────────────────────┤
│ Level 1:    基本制御 (PLC/RTU)          │
│             ↕ 独自プロトコル / IEC 61850  │
├─────────────────────────────────────────┤
│ Level 0:    フィールド機器（センサ/アクチュエータ）│
└─────────────────────────────────────────┘
```

### IEC 62443 との関係

本ワークスペースの IEC 62443 資料群によれば、ICS のセキュリティはゾーンとコンジット（Zone & Conduit）モデルで設計されます。OPC 通信はゾーン間のコンジットとして重要な位置を占めるため、以下が求められます。

- **通信の暗号化**: OPC UA の SecurityMode を適切に設定
- **認証**: ユーザ認証・アプリケーション認証の実装
- **DMZ の設置**: IT 層と OT 層の間にデータ中継層を配置

> **出典**: 本田博士論文 p.22（石油プラントでの DMZ 構成に Cogent DataHub を使用した事例）、IEC 62443-3-1/3-2

---

## 1.5 OPC Classic と OPC UA の比較

### 包括比較表

| 観点 | OPC Classic（COM/DCOM） | OPC UA |
|---|---|---|
| **依存基盤** | Windows の COM/DCOM に完全依存 | プラットフォーム非依存（Windows, Linux, 組込み等） |
| **通信方式** | DCOM/RPC による分散オブジェクト呼出 | サービス指向 ＋ SecureChannel |
| **ポート** | TCP 135（エンドポイントマッパ）＋ **動的ポート** | TCP 4840（IANA 登録済み、固定） |
| **セキュリティ** | DCOM に委譲（規格に内蔵されていない） | 証明書・署名・暗号・監査を **仕様として標準化** |
| **暗号化** | DCOM の認証レベルで可能だが設定が複雑 | SecurityMode で None / Sign / SignAndEncrypt を選択 |
| **ファイアウォール越え** | RPC の動的ポートにより困難。NAT 越し不可 | Firewall-friendly（固定ポート） |
| **情報モデル** | フラットなタグ列（値＋タイムスタンプ＋品質） | オブジェクト指向の階層型情報モデル（意味付きデータ） |
| **データの種類** | DA/HDA/A&E が独立仕様 | 単一アドレス空間に統合 |
| **OS サポート** | Windows のみ | クロスプラットフォーム |
| **国際標準** | なし（業界デファクト） | IEC 62541 として標準化 |
| **設定ミスの起きやすさ** | 高い（DCOM は多変数依存） | 証明書運用に注意が必要だが構造化されている |
| **移行** | - | ラッパ/プロキシによる段階的移行が可能 |

### なぜ OPC Classic が今も残っているのか

OPC UA の優位性は明白ですが、OPC Classic が現場から消えない理由は技術だけではありません。

| 理由 | 詳細 |
|---|---|
| **既存資産の大きさ** | 長年にわたり稼働中のシステムは「動いているものに触りたくない」 |
| **停止・検証コスト** | UA 化は単なる通信置換ではなく、証明書・権限・テストの再設計が必要 |
| **ベンダ対応の差** | 対象機器が UA サーバを持たない・限定的な場合がある |
| **暫定策の存在** | ラッパ/ゲートウェイで Classic → UA 変換して延命可能 |
| **セキュリティ優先度の現実** | 工場ネットワークは閉域と認識され、対応が後回しになりがち |

> **出典**: 『調査報告_OPC-Classicとは.md』の移行阻害要因分析より。

### DCOM の根本的な問題

OPC Classic が「使いにくい」「脆い」と言われる根本原因は DCOM にあります。

> **DCOM（Distributed COM）**: COM（Component Object Model）をネットワーク越しに拡張した技術。RPC（Remote Procedure Call）に基づき、認証・認可・メッセージ整合性に依存する。

**DCOM が問題視される3つの理由:**

1. **成功条件が多変数**: ネットワーク（ポート・名前解決）＋ 認証（NTLM/Kerberos）＋ 権限（起動/アクセス許可）＋ OS バージョンが全て整わないと動かない
2. **設計上の脆さ**: Classic の「セキュリティが内蔵されず DCOM に委譲される」構造が、統一管理を困難にする
3. **DCOM Hardening**: CVE-2021-26414 対応により、Windows Update が DCOM の認証要件を段階的に引き上げ、古い OPC アプリケーションが動かなくなるケースが多発

> **初学者向けたとえ**: DCOM は「家の鍵が1つじゃなく、玄関・勝手口・部屋ごとに鍵と合鍵と入室権限があって、一部は"家全体の方針"で上書きされる」ようなもの。
>
> **出典**: 『調査報告_OPC-Classicとは.md』より。Microsoft KB5004442 のタイムライン、横河電機のベンダ通知も参照。

---

# 第2章 システム構成と要件

## 2.1 全体アーキテクチャ

本指南書の対象構成は以下のとおりです。

```
┌────────────────────────────────────────┐
│   IEC 61850 対応機器 / シミュレータ     │  ← 変電所の保護・制御装置を模擬
│   （IED: Intelligent Electronic Device）│
└──────────────────┬─────────────────────┘
                   │ IEC 61850 (MMS/GOOSE)
                   ▼
┌────────────────────────────────────────┐
│   OPC サーバ                            │  ← IEC 61850 → OPC DA 変換
│  （IEC 61850 Gateway / OPC DA Server）  │
└──────────────────┬─────────────────────┘
                   │ OPC DA (COM/DCOM)
                   ▼
┌────────────────────────────────────────┐
│   OPC クライアント / データブリッジ      │  ← OPC DA からデータを取得し
│  （DataHub / OPC-to-DB Bridge）         │     DB へ書き込む中継役
└──────────────────┬─────────────────────┘
                   │ SQL (ODBC / ADO)
                   ▼
┌────────────────────────────────────────┐
│   ヒストリアン（データベース）           │  ← 時系列データの蓄積・分析
│  （SQL Server Express / MySQL 等）      │
└────────────────────────────────────────┘
```

## 2.2 各コンポーネントの役割

### コンポーネント①: IEC 61850 対応機器 / シミュレータ

| 項目 | 内容 |
|---|---|
| **役割** | 変電所の保護・制御装置（IED）の機能を模擬 |
| **プロトコル** | IEC 61850 MMS（Manufacturing Message Specification） |
| **本構成での位置** | データ発生源。電力系統のスイッチ状態、計測値等を提供 |

> **IEC 61850 とは**: 変電所通信を標準化した国際規格。MMS はアプリケーション層のプロトコルで、名前付き変数の読み書き、レポート通知などをサポート。

### コンポーネント②: OPC サーバ（IEC 61850 → OPC DA 変換）

| 項目 | 内容 |
|---|---|
| **役割** | IEC 61850 MMS のデータを OPC DA のタグに変換（ゲートウェイ機能） |
| **通信** | 下位: IEC 61850 MMS / 上位: OPC DA (COM/DCOM) |
| **本構成での位置** | プロトコル変換の中核。IEC 61850 のデータオブジェクト → OPC Item にマッピング |

### コンポーネント③: OPC クライアント / データブリッジ

| 項目 | 内容 |
|---|---|
| **役割** | OPC DA サーバからデータを購読し、ヒストリアン（DB）へ書き込む |
| **通信** | 上位: OPC DA / 下位: SQL (ODBC/ADO) |
| **本構成での位置** | データの橋渡し役。受信した値をSQL INSERT 文としてDBへ格納 |

### コンポーネント④: ヒストリアン（データベース）

| 項目 | 内容 |
|---|---|
| **役割** | 時系列データを蓄積し、トレンド分析・帳票・監査に利用 |
| **通信** | SQL（ODBC / ADO.NET） |
| **本構成での位置** | データの最終保存先。可視化ツール等から参照される |

## 2.3 通信プロトコルの詳細

### IEC 61850 MMS

| 項目 | 内容 |
|---|---|
| **OSI 層** | アプリケーション層（ISO 9506） |
| **トランスポート** | TCP/IP 上で動作（ポート 102） |
| **データモデル** | 論理ノード（LN）、データオブジェクト（DO）、データ属性（DA）の階層 |
| **通信モデル** | クライアント-サーバ型（Read/Write/Report） |

### OPC DA（COM/DCOM）

| 項目 | 内容 |
|---|---|
| **基盤** | Microsoft COM/DCOM（Windows 専用） |
| **ポート** | TCP 135（DCOM エンドポイントマッパ）＋ 動的ポート（1024–65535） |
| **データ単位** | Item（タグ）: 値 ＋ タイムスタンプ ＋ 品質 |
| **認証** | DCOM セキュリティ（NTLM / Kerberos） |

### SQL（ODBC / ADO）

| 項目 | 内容 |
|---|---|
| **用途** | OPC クライアントからヒストリアン DB へのデータ書き込み |
| **接続方式** | ODBC ドライバ / ADO / ADO.NET 経由 |
| **ポート** | データベース依存（SQL Server: TCP 1433、MySQL: TCP 3306） |

---

# 第3章 環境制約と準備

## 3.1 仮想化基盤の選定

本構成は **すべて仮想環境上で構築** します（実機は使用不可）。

### 推奨仮想化ソフトウェア

| ソフトウェア | ライセンス | Windows XP 対応 | 推奨度 | ダウンロード先 |
|---|---|---|---|---|
| **Oracle VirtualBox** | GPLv3（無料） | ○（レガシーサポート） | ★★★ | https://www.virtualbox.org/wiki/Downloads |
| **VMware Workstation Player** | 個人利用無料 | ○（古いバージョンで対応） | ★★★ | https://www.vmware.com/products/workstation-player.html |
| **QEMU + KVM**（Linux ホスト） | GPLv2（無料） | ○ | ★★ | https://www.qemu.org/download/ |

### VirtualBox を用いた場合の推奨設定

| 設定項目 | 推奨値 | 備考 |
|---|---|---|
| メモリ | 1024 MB 以上（XP）、2048 MB 以上（Win 7） | OPC サーバの動作に必要 |
| CPU | 1–2 コア | |
| ネットワーク | **内部ネットワーク**または**ホストオンリーアダプタ** | DCOM 通信のため同一サブネット必須 |
| ストレージ | VDI 20 GB 以上 | ソフトウェアインストール用 |
| ゲスト拡張 | Guest Additions インストール推奨 | 共有フォルダ、クリップボード共有 |

### ネットワーク構成

```
   ホストOS（Windows 10/11 or Linux）
      │
      ├── VirtualBox ホストオンリーネットワーク: 192.168.56.0/24
      │
      ├── VM1: IEC 61850 シミュレータ    192.168.56.10
      ├── VM2: OPC サーバ               192.168.56.20
      ├── VM3: OPC クライアント/DB       192.168.56.30
      └── (VM4: ヒストリアン DB を別建てする場合  192.168.56.40)
```

> [!IMPORTANT]
> DCOM は **NAT 越し不可** です。VM 間は必ず **ブリッジ** または **ホストオンリー/内部ネットワーク** を使用し、同一サブネット上に配置してください。

---

## 3.2 ゲストOSの選定と制約

### 対応OSとリスク

| OS | OPC Classic 対応 | 入手方法 | リスク・制約 |
|---|---|---|---|
| **Windows XP SP3** | ◎（DCOM 完全対応） | MSDN / 評価版ISO（入手困難） | セキュリティパッチ終了。隔離ネットワーク必須 |
| **Windows Server 2003** | ◎ | 評価版（180日） | 同上 |
| **Windows 7 SP1** | ◎ | 評価版ISO | DCOM Hardening の影響を受ける可能性 |
| **Windows Server 2008 R2** | ◎ | 評価版ISO（180日） | 同上 |
| **Windows 10/11** | ○（DCOM Hardening 要対応） | 評価版（90日） | KB5004442 以降の影響を確認必要 |

> **推奨**: 「OPC Classic のレガシー動作検証」が目的なら **Windows XP SP3** または **Windows 7 SP1** を使用。DCOM Hardening の影響を体験・検証したい場合は **Windows 10** も追加で用意すると学習効果が高い。

### Windows XP 特有の注意点

- **SP3 必須**: SP2 以前は DCOM のセキュリティモデルが古く、多くの OPC ソフトが対応しない
- **Windows ファイアウォール**: XP SP2 以降で有効化されるため、DCOM 通信用のポート開放が必要
- **名前解決**: DNS が使えない場合、`C:\WINDOWS\system32\drivers\etc\hosts` ファイルで相互に名前解決を設定

---

## 3.3 COM/DCOM セキュリティ設定の注意点

### DCOM 構成の基本

DCOM の設定は **DCOMCNFG**（コンポーネントサービス管理ツール）で行います。

```
操作手順:
[スタート] → [ファイル名を指定して実行] → "dcomcnfg" → [Enter]
```

### 設定の階層構造

```
コンポーネントサービス
  └── コンピュータ
       └── マイ コンピュータ
            ├── ★ プロパティ ← システム全体の DCOM 設定
            │    ├── 既定のプロパティ
            │    │    ├── DCOM の有効化 ← 必ず ON
            │    │    ├── 既定の認証レベル
            │    │    └── 既定の偽装レベル
            │    ├── 既定のプロトコル
            │    └── COM セキュリティ
            │         ├── アクセス許可
            │         └── 起動/アクティブ化の許可
            │
            └── DCOM の構成
                 └── [OPC サーバ名] ← アプリごとの設定
                      ├── 全般（認証レベル）
                      ├── セキュリティ
                      │    ├── 起動/アクティブ化の許可
                      │    ├── アクセス許可
                      │    └── 構成許可
                      ├── エンドポイント
                      └── ID（実行ユーザ）
```

### 推奨設定値（検証環境用）

> [!WARNING]
> 以下は **隔離された検証環境** 用の設定です。本番環境では適切な権限設計を行ってください。

#### システム全体設定（マイ コンピュータ → プロパティ）

| 設定 | 推奨値（検証用） | 説明 |
|---|---|---|
| DCOM の有効化 | **ON（チェック）** | OFF だと全ての DCOM 通信が不可 |
| 既定の認証レベル | **接続（Connect）** | 接続時のみ認証。検証用として最も緩やか |
| 既定の偽装レベル | **識別（Identify）** | サーバがクライアントの身元を確認可能 |

#### COM セキュリティ（アクセス許可・起動許可）

| 設定 | 追加するユーザ/グループ | 権限 |
|---|---|---|
| アクセス許可 → 制限の編集 | `Everyone` / `ANONYMOUS LOGON` | ローカルアクセス ＋ リモートアクセス |
| 起動/アクティブ化の許可 → 制限の編集 | `Everyone` / `ANONYMOUS LOGON` | ローカル起動 ＋ リモート起動 ＋ ローカルアクティブ化 ＋ リモートアクティブ化 |

#### OPC サーバ固有設定

| 設定 | 推奨値 | 説明 |
|---|---|---|
| 認証レベル | **なし（None）** または **接続（Connect）** | 検証環境では None で接続性を優先 |
| ID | **対話ユーザ** または **このユーザ（指定アカウント）** | サービス化する場合は「このユーザ」 |
| セキュリティ → アクセス許可 | **カスタマイズ** → Everyone 追加 | リモート接続を許可 |
| セキュリティ → 起動許可 | **カスタマイズ** → Everyone 追加 | リモート起動を許可 |

### 認証レベル定数（参考）

| 定数 | 値 | 説明 |
|---|---|---|
| `RPC_C_AUTHN_LEVEL_NONE` | 1 | 認証なし |
| `RPC_C_AUTHN_LEVEL_CONNECT` | 2 | 接続時のみ認証 |
| `RPC_C_AUTHN_LEVEL_CALL` | 3 | 各呼出時に認証 |
| `RPC_C_AUTHN_LEVEL_PKT` | 4 | パケットごとに認証 |
| `RPC_C_AUTHN_LEVEL_PKT_INTEGRITY` | 5 | 改ざん検知付き |
| `RPC_C_AUTHN_LEVEL_PKT_PRIVACY` | 6 | 暗号化付き（最強） |

> **出典**: 『調査報告_OPC-Classicとは.md』より。「DCOM には認証レベルとして PKT_INTEGRITY（改ざん検知）や PKT_PRIVACY（暗号化）まで定義されている。つまり "できる" はある。ただし、全端末・全アプリで矛盾なく適用するのが難しい」。

---

## 3.4 使用ソフトウェア一覧（無料・OSS・評価版）

### IEC 61850 シミュレータ

| ソフトウェア | 種別 | 説明 | 対応OS | ダウンロード先 |
|---|---|---|---|---|
| **libIEC61850** | OSS (GPLv3) | IEC 61850 MMS/GOOSE の C ライブラリ。サーバ・クライアントのサンプル付属 | Windows/Linux | https://github.com/mz-automation/libiec61850 |
| **IEDScout**（Omicron社） | 評価版（30日） | IEC 61850 テスト・設定ツール。MMS クライアント/ブラウザ機能 | Windows | https://www.omicronenergy.com/en/products/iedscout/ |
| **OpenIEC61850** | OSS (GPLv3) | Java ベースの IEC 61850 ライブラリ | クロスプラットフォーム | https://github.com/gythialy/openmuc |

### OPC DA サーバ

| ソフトウェア | 種別 | 説明 | 対応OS | ダウンロード先 |
|---|---|---|---|---|
| **MatrikonOPC Server for Simulation** | 無料 | シミュレーション用 OPC DA サーバ。Sin/Cos/Random 等のタグを自動生成 | Windows XP〜 | https://www.matrikonopc.com/opc-server/opc-simulation-server.aspx |
| **MatrikonOPC Explorer** | 無料 | OPC DA クライアント / ブラウザ。タグの閲覧・値の確認に使用 | Windows XP〜 | https://www.matrikonopc.com/opc-explorer.aspx |
| **Cogent DataHub** | 評価版（30日） | OPC DA/UA ゲートウェイ、データブリッジ、DB 連携機能 | Windows | 本ワークスペース内に同梱 |
| **Softing OPC Toolkit** | 評価版 | OPC DA/UA サーバ/クライアント開発キット | Windows | https://industrial.softing.com/ |

### OPC DA クライアント / データブリッジ

| ソフトウェア | 種別 | 説明 | 対応OS | ダウンロード先 |
|---|---|---|---|---|
| **Cogent DataHub** | 評価版（30日） | OPC-to-DB ブリッジ機能内蔵。OPC DA → ODBC 直結可能 | Windows | 上記と同じ |
| **OpenOPC (for Python)** | OSS (GPLv2) | Python から OPC DA にアクセスするライブラリ | Windows (Python 2/3) | https://openopc2.readthedocs.io/ |
| **OPC DA Auto Wrapper** | 無料 | OPC DA Automation Interface ラッパ。VBScript/Excel から OPC にアクセス | Windows | OPC Foundation 提供 |

### IEC 61850 → OPC DA ゲートウェイ

| ソフトウェア | 種別 | 説明 | 対応OS | ダウンロード先 |
|---|---|---|---|---|
| **Triangle MicroWorks SCADA Data Gateway** | 評価版（30日） | IEC 61850 / DNP3 / Modbus → OPC DA/UA 変換 | Windows | https://www.trianglemicroworks.com/ |
| **SystemCORP Energist** | 評価版 | IEC 61850 MMS サーバ/クライアント ＋ OPC DA 出力 | Windows | https://www.systemcorp.com.au/ |
| **自作ブリッジ**（libIEC61850 + OpenOPC） | OSS | libIEC61850 でデータ取得 → OpenOPC で OPC DA に書込む Python スクリプト | Windows | 上記の組合せ |

### ヒストリアン（データベース）

| ソフトウェア | 種別 | 説明 | 対応OS | ダウンロード先 |
|---|---|---|---|---|
| **SQL Server 2019 Express** | 無料（10GB制限） | Microsoft 製 RDBMS。ODBC/ADO.NET 対応 | Windows | https://www.microsoft.com/ja-jp/sql-server/sql-server-downloads |
| **MySQL Community Edition** | OSS (GPLv2) | 広く使われる RDBMS。ODBC ドライバあり | Windows/Linux | https://dev.mysql.com/downloads/ |
| **PostgreSQL** | OSS (PostgreSQL License) | 高機能 RDBMS | Windows/Linux | https://www.postgresql.org/download/ |
| **SQLite** | パブリックドメイン | ファイルベースの軽量 DB。小規模検証向き | クロスプラットフォーム | https://www.sqlite.org/download.html |

### 補助ツール

| ソフトウェア | 種別 | 説明 | ダウンロード先 |
|---|---|---|---|
| **OPC Foundation OPC Core Components** | 無料 | OPC DA 動作に必要なランタイム | https://opcfoundation.org/ |
| **Wireshark** | OSS (GPLv2) | ネットワークキャプチャ。MMS/OPC UA の解析に使用 | https://www.wireshark.org/ |
| **OPCEnum Service** | 無料 | ネットワーク上の OPC サーバ列挙サービス（Core Components に含まれる） | 上記に含まれる |

---

# 第4章 構築手順

## 4.1 仮想環境のセットアップ

### 手順 1: VirtualBox のインストール

1. https://www.virtualbox.org/wiki/Downloads から **VirtualBox** の最新版をダウンロード
2. **VirtualBox Extension Pack** も併せてダウンロード（USB 2.0/3.0 サポート等に必要）
3. インストーラを実行し、画面の指示に従いインストール
4. Extension Pack をダブルクリックしてインストール

### 手順 2: ホストオンリーネットワークの作成

```
VirtualBox を起動
→ [ファイル] → [ツール] → [ネットワークマネージャ]
→ [作成] をクリック
→ アダプター:
     IPv4 アドレス: 192.168.56.1
     ネットマスク: 255.255.255.0
→ DHCP サーバー: 無効化（固定IP を使用するため）
→ [適用]
```

### 手順 3: 仮想マシンの作成（VM1: IEC 61850 シミュレータ）

```
[新規] をクリック
→ 名前: VM1_IEC61850_Sim
→ タイプ: Microsoft Windows
→ バージョン: Windows XP (32-bit) または Windows 7 (64-bit)
→ メモリ: 1024 MB（XP）/ 2048 MB（Win7）
→ ハードディスク: VDI, 可変サイズ, 20 GB
→ [作成]

作成後、[設定] → [ネットワーク]:
  アダプター1: ホストオンリーアダプター（vboxnet0）
→ [OK]
```

### 手順 4: 仮想マシンの作成（VM2: OPC サーバ）

同様の手順で作成。名前を `VM2_OPC_Server` とする。

### 手順 5: 仮想マシンの作成（VM3: OPC クライアント / ヒストリアン）

同様の手順で作成。名前を `VM3_OPC_Client_DB` とする。

> [!TIP]
> OPC クライアントとヒストリアン DB を同一 VM に入れると、ODBC 接続がローカルで完結し構成が簡素になります。

### 手順 6: ゲスト OS のインストール

1. 各 VM の [設定] → [ストレージ] で ISO ファイルをマウント
2. VM を起動し、Windows のインストールを実施
3. VirtualBox Guest Additions をインストール
4. 固定 IP アドレスを設定:

| VM | IP アドレス | サブネットマスク | デフォルトゲートウェイ |
|---|---|---|---|
| VM1 (IEC 61850) | 192.168.56.10 | 255.255.255.0 | なし |
| VM2 (OPC Server) | 192.168.56.20 | 255.255.255.0 | なし |
| VM3 (Client/DB) | 192.168.56.30 | 255.255.255.0 | なし |

### 手順 7: 各 VM の初期設定

各 VM で以下を実施:

```
1. hosts ファイルの編集:
   メモ帳で C:\WINDOWS\system32\drivers\etc\hosts を開き、以下を追記:
     192.168.56.10  VM1-IEC61850
     192.168.56.20  VM2-OPCSVR
     192.168.56.30  VM3-CLIENT

2. Windows ファイアウォールの設定:
   [コントロールパネル] → [Windows ファイアウォール]
   → 検証中は [無効] にするか、以下のポートを例外に追加:
     - TCP 135（DCOM エンドポイントマッパ）
     - TCP 1024-65535（DCOM 動的ポート、範囲を絞る場合は後述）
     - TCP 102（IEC 61850 MMS）
     - TCP 1433（SQL Server の場合）

3. 簡易ファイル共有の無効化（Windows XP の場合）:
   エクスプローラ → [ツール] → [フォルダオプション] → [表示]
   → 「簡易ファイルの共有を使用する」のチェックを外す

4. ping 疎通確認:
   > ping 192.168.56.20   （VM1 → VM2）
   > ping VM2-OPCSVR       （名前解決の確認）
```

---

## 4.2 OPCサーバの導入・設定

### 手順概要

ここでは2つのアプローチを紹介します。

- **アプローチA**: MatrikonOPC Simulation Server（まず OPC DA の基本動作を確認）
- **アプローチB**: IEC 61850 → OPC DA ゲートウェイ（本来の構成）

> [!TIP]
> 初学者は **まずアプローチAで OPC DA の動作を理解** し、その後アプローチBに進むことを推奨します。

### アプローチA: MatrikonOPC Simulation Server

#### 手順 A-1: OPC Core Components のインストール

1. OPC Foundation のサイトから **OPC Core Components Redistributable** をダウンロード
2. VM2（OPC サーバ）にインストール
3. 再起動

> **OPC Core Components とは**: OPC DA の動作に必要なランタイムライブラリ（OPCEnum サービス、OPC プロキシ/スタブ DLL 等）を含むパッケージです。

#### 手順 A-2: MatrikonOPC Server for Simulation のインストール

1. https://www.matrikonopc.com/ からダウンロード（無料登録が必要）
2. ダウンロードしたインストーラを VM2 で実行
3. インストール完了後、スタートメニューから **MatrikonOPC Server for Simulation and Testing** を起動
4. タスクトレイにアイコンが表示されれば起動成功

#### 手順 A-3: OPC サーバの動作確認（ローカル）

1. VM2 で **MatrikonOPC Explorer**（同じくダウンロード）を起動
2. 左ペインで **ローカル** → **MatrikonOPC Server for Simulation** を選択
3. 右クリック → **接続**
4. ツリーを展開すると、シミュレーションタグ（Sine, Random 等）が表示される
5. タグをダブルクリックして値の変化を確認

#### 手順 A-4: DCOM の設定（リモートアクセス用）

VM2（OPC サーバ側）での設定:

```
1. dcomcnfg を起動
2. [コンポーネントサービス] → [コンピュータ] → [マイ コンピュータ]
   → 右クリック → [プロパティ]

3. [既定のプロパティ] タブ:
   ☑ このコンピュータ上で DCOM を有効にする
   既定の認証レベル: [接続]
   既定の偽装レベル: [識別]

4. [COM セキュリティ] タブ:
   [アクセス許可] → [制限の編集]
     → Everyone: ローカルアクセス ☑ / リモートアクセス ☑
     → ANONYMOUS LOGON: ローカルアクセス ☑ / リモートアクセス ☑

   [起動とアクティブ化の許可] → [制限の編集]
     → Everyone: すべて ☑

5. [DCOM の構成] → [MatrikonOPC Server for Simulation]
   → 右クリック → [プロパティ]
   → [セキュリティ] タブ:
     起動/アクティブ化の許可: [カスタマイズ] → Everyone 追加
     アクセス許可: [カスタマイズ] → Everyone 追加
   → [ID] タブ:
     [対話ユーザ] を選択（または「このユーザ」で管理者アカウント指定）
```

VM3（OPC クライアント側）での設定:

```
同様に dcomcnfg を開き、マイ コンピュータの
既定のプロパティ、COM セキュリティを同じ値に設定
```

#### 手順 A-5: リモート接続テスト

1. VM3 で MatrikonOPC Explorer を起動
2. 左ペインの **リモート** で VM2 のホスト名（VM2-OPCSVR）を入力
3. VM2 上の MatrikonOPC Server が列挙されることを確認
4. 接続してタグを閲覧

---

### アプローチB: IEC 61850 → OPC DA ゲートウェイ

#### 手順 B-1: Triangle MicroWorks SCADA Data Gateway の導入

1. https://www.trianglemicroworks.com/ から **SCADA Data Gateway** の評価版をダウンロード
2. VM2 にインストール
3. Gateway Configuration Tool を起動

#### 手順 B-2: IEC 61850 クライアント接続の設定

```
Gateway Configuration Tool:
1. [Channels] → [Add Channel]
   → Protocol: IEC 61850 Client
   → Name: IEC61850_Ch1
   → IP Address: 192.168.56.10（VM1 のアドレス）
   → Port: 102

2. [Devices] → [Add Device]
   → Channel: IEC61850_Ch1
   → Device Name: IED_Simulator
   → AP Title: 任意（デフォルト可）

3. IEC 61850 データモデルのインポート:
   → IED の SCL（Substation Configuration Language）ファイルを指定
   → または Auto-Discovery で接続先からモデルを取得
```

#### 手順 B-3: OPC DA サーバ出力の設定

```
Gateway Configuration Tool:
1. [Servers] → [Add Server]
   → Protocol: OPC DA Server
   → Name: OPC_DA_Output

2. データマッピング:
   → IEC 61850 のデータオブジェクトが OPC DA のアイテム（タグ）に自動マッピングされる
   → 例: IED1/LLN0$ST$Beh$stVal → IED1.LLN0.ST.Beh.stVal

3. Gateway を起動（[Start] ボタン）
```

---

## 4.3 IEC 61850 シミュレータの構築

### libIEC61850 を用いたシミュレータ構築

#### 手順 1: libIEC61850 のビルド

```powershell
# VM1 で実施（Windows 環境、Visual Studio または MinGW が必要）

# 1. GitHubからソースを取得（または ZIP ダウンロード）
git clone https://github.com/mz-automation/libiec61850.git

# 2. CMake でビルド
cd libiec61850
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

#### 手順 2: サンプルサーバの起動

```powershell
# ビルドされたサンプルサーバを起動
cd examples\server_example_basic\Release
server_example_basic.exe

# 出力例:
# Using default configuration
# MMS server started on port 102
```

#### 手順 3: 接続確認

```powershell
# 別のコマンドプロンプトからクライアントサンプルで接続
cd examples\client_example_basic\Release
client_example_basic.exe 192.168.56.10
```

#### 代替: IEDScout を使う場合

IEDScout はGUIベースのため、より直感的です。

1. IEDScout を VM1（または VM2）にインストール
2. [Connect] → IP: 192.168.56.10, Port: 102
3. データモデルがツリー形式で表示される
4. 値の読み書き、レポート設定が GUI で可能

---

## 4.4 OPC DA クライアントの設定

### Cogent DataHub を用いたデータブリッジ構成

#### 手順 1: Cogent DataHub のインストール

1. 本ワークスペース内の `CogentDataHubFull_x64-11.0.4-260116-Windows.exe` を VM3 にコピー
2. インストーラを実行（評価版として30日間使用可能）
3. インストール完了後、DataHub が自動起動

> **出典**: 本田博士論文（p.22）でも、石油プラントの DMZ 構成で Cogent DataHub が使用されている事例が紹介されています。

#### 手順 2: OPC DA 接続の設定

```
DataHub Manager を開く:
1. 左ペインの [OPC DA] をクリック
2. [Enabled] にチェック
3. [Add Server] をクリック:
   → Computer Name: VM2-OPCSVR（OPC サーバの VM）
   → Server: MatrikonOPC.Simulation.1
   → Update Rate: 1000 ms
4. [Apply] → [Connect]
5. 接続成功後、Data Viewer でタグと値が表示される
```

### OpenOPC（Python）を用いる場合

```python
# VM3 に Python と OpenOPC をインストール
# pip install openopc-da

import OpenOPC

# OPC DA サーバに接続
opc = OpenOPC.client()
opc.connect('MatrikonOPC.Simulation.1', 'VM2-OPCSVR')

# サーバ上のタグを列挙
tags = opc.list('Simulation Items.*')
print(tags)

# タグの値を読み取り
value = opc.read('Simulation Items.Random.Int1')
print(f"Value: {value[0]}, Quality: {value[1]}, Timestamp: {value[2]}")

# 複数タグの購読
for tag, value, quality, timestamp in opc.iread(
    ['Simulation Items.Random.Int1',
     'Simulation Items.Random.Real4'],
    timeout=10000
):
    print(f"{tag} = {value} [{quality}] @ {timestamp}")

opc.close()
```

---

## 4.5 ヒストリアン（データベース）への接続

### SQL Server Express を用いる場合

#### 手順 1: SQL Server Express のインストール

1. https://www.microsoft.com/ja-jp/sql-server/sql-server-downloads から **Express** 版をダウンロード
2. VM3 にインストール（混合モード認証を選択）
3. **SQL Server Management Studio (SSMS)** もインストール

#### 手順 2: データベースとテーブルの作成

```sql
-- SSMS で以下を実行

-- 1. データベース作成
CREATE DATABASE OPC_Historian;
GO

USE OPC_Historian;
GO

-- 2. データ格納テーブル作成
CREATE TABLE ProcessData (
    ID          BIGINT IDENTITY(1,1) PRIMARY KEY,
    TagName     NVARCHAR(256) NOT NULL,       -- OPC タグ名
    TagValue    FLOAT         NOT NULL,       -- 値
    Quality     NVARCHAR(50)  NOT NULL,       -- 品質（Good/Bad/Uncertain）
    SourceTime  DATETIME2     NOT NULL,       -- ソースタイムスタンプ
    InsertTime  DATETIME2     DEFAULT GETDATE() -- DB 挿入時刻
);
GO

-- 3. インデックス作成（検索高速化）
CREATE INDEX IX_ProcessData_TagTime
    ON ProcessData (TagName, SourceTime);
GO
```

#### 手順 3: ODBC データソースの設定

```
[コントロールパネル] → [管理ツール] → [ODBC データ ソース]
→ [システム DSN] タブ → [追加]
→ ドライバ: SQL Server / SQL Server Native Client
→ データソース名: OPC_Historian_DSN
→ サーバー: (local)\SQLEXPRESS  （ローカルの場合）
→ 認証: SQL Server 認証（先ほど設定した sa アカウント等）
→ データベース: OPC_Historian
→ [テスト] で接続確認 → [OK]
```

#### 手順 4: Cogent DataHub でのDB連携設定

```
DataHub Manager:
1. 左ペインの [Database] をクリック
2. [Enabled] にチェック
3. [Add Connection]:
   → Connection String: DSN=OPC_Historian_DSN;UID=sa;PWD=xxxxx
   → または直接:
     Provider=SQLOLEDB;Data Source=(local)\SQLEXPRESS;
     Initial Catalog=OPC_Historian;User Id=sa;Password=xxxxx

4. [Logging] タブ:
   → Table Name: ProcessData
   → Mapping:
     TagName   ← {Point Name}
     TagValue  ← {Value}
     Quality   ← {Quality}
     SourceTime ← {Timestamp}
   → Log Interval: 1000 ms（1秒ごと記録）

5. [Apply] → 動作開始
```

#### 手順 5: Python からの直接書き込み（OpenOPC + pyodbc）

```python
import OpenOPC
import pyodbc
import time
from datetime import datetime

# OPC DA 接続
opc = OpenOPC.client()
opc.connect('MatrikonOPC.Simulation.1', 'VM2-OPCSVR')

# DB 接続
conn = pyodbc.connect('DSN=OPC_Historian_DSN;UID=sa;PWD=xxxxx')
cursor = conn.cursor()

# タグリスト
tags = [
    'Simulation Items.Random.Int1',
    'Simulation Items.Random.Real4',
    'Simulation Items.Sine.Real8'
]

# 定期的にデータ取得・書き込み
try:
    while True:
        for tag in tags:
            value, quality, timestamp = opc.read(tag)
            cursor.execute(
                "INSERT INTO ProcessData (TagName, TagValue, Quality, SourceTime) "
                "VALUES (?, ?, ?, ?)",
                tag, float(value), quality, timestamp
            )
        conn.commit()
        print(f"[{datetime.now()}] {len(tags)} tags recorded.")
        time.sleep(1)
except KeyboardInterrupt:
    print("Recording stopped.")
finally:
    opc.close()
    conn.close()
```

---

## 4.6 動作確認・テスト方法

### テスト 1: エンドツーエンド データフロー確認

| 確認項目 | 方法 | 期待結果 |
|---|---|---|
| IEC 61850 データ発生 | libIEC61850 サーバで値を変更 | MMS クライアントで変更を確認 |
| OPC DA マッピング | OPC Explorer で OPC サーバに接続 | IEC 61850 のデータが OPC タグとして表示 |
| OPC DA リモート通信 | VM3 の OPC クライアントから VM2 に接続 | タグの値がリアルタイムで表示 |
| DB 書き込み | SSMS で `SELECT * FROM ProcessData` | データが記録されている |
| 時系列確認 | `SELECT * FROM ProcessData WHERE TagName = '...' ORDER BY SourceTime` | 時系列でデータが並ぶ |

### テスト 2: OPC DA 品質コードの確認

| 品質 | 意味 | 発生条件 |
|---|---|---|
| **Good (192)** | 正常 | 正常なデータ取得 |
| **Bad (0)** | 不良 | センサ故障、通信断 |
| **Uncertain (64)** | 不確定 | 古い値、キャリブレーション未完 |

### テスト 3: ストレステスト

```python
# 大量タグの高速ポーリングテスト
import time
import OpenOPC

opc = OpenOPC.client()
opc.connect('MatrikonOPC.Simulation.1', 'VM2-OPCSVR')

start = time.time()
for i in range(1000):
    opc.read('Simulation Items.Random.Int1')
elapsed = time.time() - start
print(f"1000 reads in {elapsed:.2f} seconds ({1000/elapsed:.0f} reads/sec)")

opc.close()
```

### テスト 4: Wireshark によるプロトコル解析

1. Wireshark を VM2 にインストール
2. ホストオンリーアダプタでキャプチャ開始
3. フィルタ: `tcp.port == 135` で DCOM 通信を確認
4. フィルタ: `mms` で IEC 61850 MMS 通信を確認

---

# 第5章 トラブルシューティング

## 5.1 DCOM権限エラー

### エラー: 「アクセスが拒否されました」(0x80070005)

| 原因 | 対処法 |
|---|---|
| DCOM アクセス許可に適切なユーザが追加されていない | dcomcnfg → COM セキュリティ → アクセス許可に Everyone / ANONYMOUS LOGON 追加 |
| OPC サーバ固有のアクセス許可が未設定 | dcomcnfg → DCOM の構成 → [OPCサーバ] → セキュリティ → カスタマイズで許可追加 |
| ワークグループ環境で NTLM 認証が失敗 | 両マシンに **同一ユーザ名・同一パスワード** のアカウントを作成 |
| 簡易ファイル共有が有効（XP） | フォルダオプション → 表示 → 「簡易ファイルの共有」を無効化 |

### エラー: 「RPC サーバーを利用できません」(0x800706BA)

| 原因 | 対処法 |
|---|---|
| Windows ファイアウォールが TCP 135 をブロック | ファイアウォールに TCP 135 の受信規則を追加 |
| DCOM 動的ポートがブロックされている | 下記のレジストリ設定でポート範囲を制限し、ファイアウォールに追加 |
| RPC サービスが停止 | `services.msc` → 「Remote Procedure Call (RPC)」が「開始」であることを確認 |
| ネットワーク疎通不可 | `ping` コマンドで相手先の到達性を確認 |

### 動的ポート範囲の制限

```
レジストリで DCOM の動的ポートを制限する:

regedit で以下のキーを開く（なければ作成）:
HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Rpc\Internet

値:
  Ports (複数文字列値): "5000-5020"
  PortsInternetAvailable (文字列値): "Y"
  UseInternetPorts (文字列値): "Y"

→ 再起動後、DCOM は 5000-5020 の範囲のみ使用
→ ファイアウォールで TCP 5000-5020 を開放
```

### エラー: 「クラスが登録されていません」(0x80040154)

| 原因 | 対処法 |
|---|---|
| OPC Core Components がインストールされていない | OPC Core Components Redistributable をインストール |
| OPC サーバが正しく登録されていない | OPC サーバを再インストール、または `regsvr32` で手動登録 |
| 32bit/64bit の不一致 | 32bit OPC サーバには 32bit クライアントを使用（または逆） |

---

## 5.2 ファイアウォール設定

### 必要なポートの一覧

| ポート | プロトコル | 用途 | 開放先 |
|---|---|---|---|
| TCP 135 | RPC Endpoint Mapper | DCOM 初期接続 | OPC サーバ ↔ クライアント間 |
| TCP 5000-5020 | RPC 動的ポート（制限後） | DCOM データ通信 | 同上 |
| TCP 102 | IEC 61850 MMS | IEC 61850 通信 | シミュレータ ↔ ゲートウェイ間 |
| TCP 1433 | SQL Server | DB 接続 | クライアント ↔ DB 間 |
| TCP 3306 | MySQL（使用する場合） | DB 接続 | 同上 |
| TCP 4840 | OPC UA（将来の拡張用） | OPC UA 通信 | UA サーバ ↔ クライアント間 |

### Windows ファイアウォールの設定コマンド（Windows 7 以降）

```powershell
# TCP 135 (DCOM)
netsh advfirewall firewall add rule name="DCOM-TCP135" dir=in action=allow protocol=TCP localport=135

# DCOM 動的ポート（範囲制限後）
netsh advfirewall firewall add rule name="DCOM-Dynamic" dir=in action=allow protocol=TCP localport=5000-5020

# IEC 61850 MMS
netsh advfirewall firewall add rule name="IEC61850-MMS" dir=in action=allow protocol=TCP localport=102

# SQL Server
netsh advfirewall firewall add rule name="SQLServer" dir=in action=allow protocol=TCP localport=1433
```

> **Windows XP の場合**: `netsh firewall` コマンドまたは GUI（コントロールパネル → Windows ファイアウォール → 例外）で設定。

---

## 5.3 よくある接続問題と対処法

### 問題 1: OPCEnum でリモートサーバが見えない

```
症状: OPC クライアントで「リモートコンピュータの OPC サーバを列挙できません」
原因: OPCEnum サービスが停止している、またはファイアウォールが遮断

対処:
  1. サーバ側で services.msc → OPCEnum が「開始」になっているか確認
  2. OPCEnum がない場合 → OPC Core Components をインストール
  3. ファイアウォールで TCP 135 が開放されているか確認
  4. dcomcnfg → OPCEnum のDCOM設定で認証レベルを確認
```

### 問題 2: DCOM 接続は成功するがデータが来ない

```
症状: OPC Explorer で接続成功、タグも見えるが、値が「Bad」のまま
原因: OPC サーバのデータソース（IEC 61850等）との接続が未確立

対処:
  1. OPC サーバ側のログを確認
  2. IEC 61850 シミュレータが稼働しているか確認
  3. OPC サーバのタグ設定でデータソースマッピングを確認
```

### 問題 3: 接続が断続的に切れる

```
症状: 数分間は動作するが、突然「RPC サーバーを利用できません」エラー
原因: DCOM タイムアウト、ファイアウォールのステートフルパケット検査

対処:
  1. OPC サーバの Group 設定で KeepAlive を適切に設定
  2. ファイアウォールの接続追跡タイムアウトを延長
  3. ネットワーク品質を確認（特に VM 環境でのリソース競合）
```

### 問題 4: Windows Update 後に OPC が動かなくなった

```
症状: KB5004442 等の適用後、DCOM 認証エラーが発生
原因: DCOM Hardening による認証レベル強化

対処:
  1. イベントビューアで イベントID 10036 を確認（DCOM警告）
  2. 暫定対策（非推奨）:
     レジストリキー HKLM\SOFTWARE\Microsoft\Ole\AppCompat
     → RequireIntegrityActivationAuthenticationLevel = 0 で一時無効化
  3. 恒久対策:
     OPC サーバ/クライアントの認証レベルを PKT_INTEGRITY 以上に引き上げ
```

> **出典**: 『調査報告_OPC-Classicとは.md』より。「Microsoft は CVE-2021-26414 対応として段階的に強化し、フェーズ3で無効化手段の削除を予定（実施）した。つまり『設定が甘い古いアプリは動かなくても仕方ない』方向に OS が寄った」。

### 問題 5: ワークグループ環境での認証失敗

```
症状: ドメインに参加していない PC 間で DCOM 接続が失敗
原因: Kerberos が使えず、NTLM 認証で資格情報が不一致

対処:
  1. 両方のPCに同じユーザ名・同じパスワードのアカウントを作成
  2. OPC サーバの DCOM 設定 → [ID] タブで「このユーザ」を選択し、
     共通アカウントを指定
  3. それでも失敗する場合:
     ローカルセキュリティポリシー → セキュリティオプション
     → 「ネットワークアクセス: ローカルアカウントの共有とセキュリティモデル」
     → 「クラシック - ローカルユーザをそのまま認証する」に変更
```

---

## 5.4 トラブルシューティング・チェックリスト

問題が発生したときは、以下のチェックリストを上から順に確認してください。

### ネットワーク層

- [ ] VM 間で `ping` が通るか
- [ ] `hosts` ファイルに正しいホスト名が登録されているか
- [ ] ネットワークアダプタの種類（ホストオンリー/ブリッジ）は適切か
- [ ] NAT が介在していないか（DCOM は NAT 越し不可）

### ファイアウォール層

- [ ] TCP 135 が開放されているか
- [ ] DCOM 動的ポート範囲が開放されているか
- [ ] Windows ファイアウォールを一時的に無効化して切り分けできるか
- [ ] ウイルス対策ソフトが DCOM をブロックしていないか

### DCOM 設定層

- [ ] `dcomcnfg` で DCOM が有効化されているか
- [ ] 既定の認証レベルは「接続」以下か（検証環境）
- [ ] COM セキュリティのアクセス許可に Everyone が追加されているか
- [ ] COM セキュリティの起動許可に Everyone が追加されているか
- [ ] OPC サーバ固有のセキュリティ設定は適切か
- [ ] OPC サーバの実行ユーザ（ID）は適切か

### OPC サーバ層

- [ ] OPC Core Components はインストールされているか
- [ ] OPC サーバプロセスが稼働しているか（タスクマネージャで確認）
- [ ] OPCEnum サービスが起動しているか
- [ ] ローカル接続（同一マシン）は成功するか

### データソース層

- [ ] IEC 61850 シミュレータは稼働しているか
- [ ] MMS 通信が確立されているか（Wireshark で確認）
- [ ] OPC サーバのデータソース設定（IP, ポート）は正しいか

### データベース層

- [ ] DB サービス（SQL Server 等）は起動しているか
- [ ] ODBC データソースの接続テストは成功するか
- [ ] テーブルは正しく作成されているか
- [ ] DB ユーザの権限（INSERT 権限）は付与されているか

---

# 付録

## A. 用語集

| 用語 | 読み | 説明 |
|---|---|---|
| **OPC** | オーピーシー | Open Platform Communications。産業用データ交換の相互運用標準 |
| **OPC DA** | オーピーシー ディーエー | OPC Data Access。リアルタイムデータの読み書き仕様 |
| **OPC HDA** | オーピーシー エイチディーエー | OPC Historical Data Access。履歴データの取得仕様 |
| **OPC A&E** | オーピーシー エーアンドイー | OPC Alarms & Events。アラーム・イベント管理仕様 |
| **OPC UA** | オーピーシー ユーエー | OPC Unified Architecture。次世代の統合アーキテクチャ |
| **COM** | コム | Component Object Model。Windows のバイナリコンポーネント連携技術 |
| **DCOM** | ディーコム | Distributed COM。COM のネットワーク拡張 |
| **RPC** | アールピーシー | Remote Procedure Call。ネットワーク越しの関数呼び出し |
| **IEC 61850** | アイイーシー ロクイチハチゴマル | 変電所通信の国際標準規格 |
| **MMS** | エムエムエス | Manufacturing Message Specification。IEC 61850 のアプリケーション層プロトコル |
| **IED** | アイイーディー | Intelligent Electronic Device。変電所の保護・制御装置 |
| **PLC** | ピーエルシー | Programmable Logic Controller。プログラム制御装置 |
| **DCS** | ディーシーエス | Distributed Control System。分散制御システム |
| **SCADA** | スキャダ | Supervisory Control and Data Acquisition。監視制御システム |
| **HMI** | エイチエムアイ | Human-Machine Interface。人間と機械のインターフェース |
| **ヒストリアン** | ヒストリアン | 時系列データを蓄積するデータベースシステム |
| **ODBC** | オーディービーシー | Open Database Connectivity。データベース接続の標準インターフェース |
| **ADO** | エーディーオー | ActiveX Data Objects。Microsoft のデータアクセス技術 |
| **SecureChannel** | セキュアチャネル | OPC UA の暗号化通信路 |
| **PKI** | ピーケーアイ | Public Key Infrastructure。公開鍵基盤 |
| **TrustList** | トラストリスト | OPC UA で信頼する証明書のリスト |
| **DMZ** | ディーエムゼット | De-Militarized Zone。IT/OT 間の中間セグメント |
| **IEC 62443** | アイイーシー ロクニヨンヨンサン | 産業用制御システムのセキュリティ国際標準 |
| **SCL** | エスシーエル | Substation Configuration Language。IEC 61850 の構成記述言語 |
| **Purdue モデル** | パデューモデル | 産業制御システムの階層参照モデル |

## B. 参考文献・資料

### ワークスペース内の参考資料

| # | 資料名 | 概要 |
|---|---|---|
| 1 | 本田俊明 博士論文「産業用制御システムと OPC UA」 | OPC UA の暗号化通信監視と不適切コマンド検知に関する研究。OPC UA セキュリティの詳細な技術解説を含む |
| 2 | 『OPC Unified Architecture』(Mahnke 他) | OPC UA の設計思想・アーキテクチャ・情報モデリング・セキュリティを包括的に解説する教科書 |
| 3 | 「調査報告_OPC-Classicとは」 | OPC Classic/DA (DCOM) の技術・セキュリティ・運用リスク分析。DCOM の弱点と移行阻害要因を詳説 |
| 4 | DataHub ユーザマニュアル (Skkynet社) | Cogent DataHub の設定・運用ガイド |
| 5 | Yokogawa OPC UA Server ユーザーズマニュアル | 横河電機の OPC UA サーバ実装ガイド |
| 6 | IEC 62443 関連資料群 | 産業制御システムセキュリティの国際標準解説 |
| 7 | NIST SP 800-82r3 | ICS セキュリティガイドライン |
| 8 | VDMA 40000 ドラフト | OPC UA for Machinery ホワイトペーパー |
| 9 | OPC UA 移行論文群 | Classic → UA 移行の事例・ロードマップ論文 |
| 10 | 「OPC UA のセキュリティ対策と証明書の運用」(OPC協会) | OPC UA 証明書管理の実務ガイド |

### 外部参考資料

| # | 資料名 | URL |
|---|---|---|
| 1 | OPC Foundation 公式サイト | https://opcfoundation.org/ |
| 2 | MatrikonOPC ツール | https://www.matrikonopc.com/ |
| 3 | libIEC61850 (GitHub) | https://github.com/mz-automation/libiec61850 |
| 4 | Microsoft KB5004442 (DCOM Hardening) | https://support.microsoft.com/en-us/topic/kb5004442 |
| 5 | IANA OPC UA ポート登録 | https://www.iana.org/assignments/service-names-port-numbers/ |
| 6 | BSI OPC UA セキュリティ分析 | https://www.bsi.bund.de/ |

---

> **改訂履歴**
>
> | 版数 | 日付 | 内容 |
> |---|---|---|
> | 第1版 | 2026-07-11 | 初版作成 |
