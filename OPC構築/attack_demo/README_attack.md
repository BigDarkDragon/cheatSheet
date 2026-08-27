# Industroyer デモスクリプト — 攻撃再現ツール

## 概要

KEPServerEX の OPC DA インターフェースを利用して、
4つの遮断器 (XCBR1〜4) を開放 (Trip) するデモスクリプト。

SCADA 画面で遮断器が順次開放される様子をリアルタイムで確認できる。

## 動作原理

```
[attack_demo.ps1]
      │
      │ COM (OPC DA Automation)
      ▼
[KEPServerEX] ── OPC.Automation.1 COM wrapper
      │
      │ IEC 61850 MMS Write
      ▼
[IED (Ubuntu)] ── substation_server
      │
      │ MMS Report / Data Change
      ▼
[Cogent DataHub] → [SCADA画面 (HTML)]
```

攻撃経路: `スクリプト → OPC DA → MMS → IED`

## ファイル構成

| ファイル | 説明 |
|---------|------|
| `industroyer_demo.ps1` | メインスクリプト (PowerShell) |
| `run_demo.bat` | ダブルクリック用ランチャー |

## 前提条件

- Windows 7 + .NET Framework 4.0
- KEPServerEX が起動中
- IED シミュレータ (Ubuntu) が起動中
- Cogent DataHub が起動中 (SCADA画面の確認用)

## 使い方

### Step 1: 設定確認

`industroyer_demo.ps1` の先頭にある設定を環境に合わせて変更:

```powershell
# KEPServerEX の ProgID（通常は変更不要）
$SERVER_PROGIDS = @(
    "Kepware.KEPServerEX.V6",
    "Kepware.KEPServerEX.V7",
    ...
)

# タグプレフィックス（KEPServerEX Quick Client で確認）
$TAG_PREFIX = "IED1_Ch1.IED.IED1CBIED"
```

> **重要**: `$TAG_PREFIX` は KEPServerEX Quick Client のタグブラウザで
> 実際のパスを確認してください。

### Step 2: SCADA画面を開く

ブラウザで Cogent DataHub の SCADA 画面を表示しておく。
全4フィーダーの遮断器が「Увімк」(投入) 状態であること。

### Step 3: 攻撃実行

`run_demo.bat` をダブルクリック。

5段階のフェーズが実行される:

| Phase | 内容 |
|-------|------|
| 1. RECONNAISSANCE | OPC DA サーバに接続 |
| 2. TAG DISCOVERY | 攻撃対象タグをスキャン |
| 3. PRE-ATTACK STATUS | 現在の遮断器状態を確認 |
| 4. ATTACK EXECUTION | 遮断器を順次開放（3秒間隔） |
| 5. POST-ATTACK VERIFICATION | 結果を検証 |

### Step 4: SCADA画面で確認

各フィーダーの遮断器がリアルタイムで「Відкл」(開放) に変わることを確認。

## トラブルシューティング

### "OPC.Automation.1 COM object not found"
→ OPC Core Components が未インストール。KEPServerEX の再インストールで解決。

### "Could not connect to any KEPServerEX instance"
→ KEPServerEX が起動していない、または ProgID が異なる。
以下のコマンドで確認:
```powershell
# 登録されている OPC サーバを検索
Get-ChildItem HKLM:\SOFTWARE\Classes -ErrorAction SilentlyContinue |
  Where-Object { $_.Name -match "Kepware|KEPServer|PTC" } |
  ForEach-Object { $_.PSChildName }
```

### "Failed to add OPC items"
→ タグ名が KEPServerEX のパスと一致していない。
KEPServerEX Quick Client でタグをブラウズし、正しいパスを確認。

## 攻撃値の変更

スクリプトの `$ATTACK_VALUE` を変更することで動作を変更可能:

| 値 | 動作 | 意味 |
|---|------|------|
| `1` | 遮断器開放 (OPEN/TRIP) | **停電** |
| `2` | 遮断器投入 (CLOSE) | 復旧 |
