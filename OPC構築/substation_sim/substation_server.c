/*
 * substation_server.c (v4 — CF PosCmd 制御方式)
 * ================================================================
 * Industroyer 攻撃対象変電所の IED シミュレータ
 *
 * v4 修正点:
 *   [Fix #5] KEPServerEX からの制御が通らない問題を完全修正
 *     - ST (stVal)  → KEPServerEX が Read Only にする → OPC DA レベルで拒否
 *     - CO (ctlVal) → libIEC61850 が MMS Write をハードコードで拒否
 *     - CF (PosCmd) → KEPServerEX=Read/Write, libIEC61850=Write可 → ★唯一通る
 *
 *     ICD に CF 属性 PosCmd (INT32) を追加。
 *     WriteAccessHandler で制御ロジックを起動。
 *     値: 1=Open（遮断器開放）, 2=Close（遮断器投入）
 *
 * v3 修正点:
 *   [Fix #3] DPC Oper 構造体化 + DataSet/Report 削除 → double free 解消
 *   [追加]   全 MmsValue の明示的初期化
 *
 * ビルド手順:
 *   java -jar ../libiec61850/tools/model_generator/genmodel.jar substation.icd
 *   cd build && rm -rf * && cmake .. -DLIBIEC61850_HOME=../../libiec61850 && make
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <math.h>
#include <time.h>
#include <string.h>

#include "iec61850_server.h"
#include "hal_thread.h"
#include "static_model.h"

/* ================================================================
 *  グローバル状態
 * ================================================================ */

static int running = 1;
static IedServer iedServer = NULL;

/*
 * 遮断器/断路器の状態
 *   Dbpos: 0=intermediate, 1=off/open, 2=on/closed, 3=bad
 *
 * 制御ハンドラは pending フラグを立てるのみ。
 * メインスレッドが lockDataModel 区間内で実際の更新を行う。
 */

/* XCBR1 */
static volatile int xcbr1_pos     = 2;   /* 初期状態: Closed */
static volatile int xcbr1_opCnt   = 0;
static volatile int xcbr1_pending = 0;
static volatile int xcbr1_new_pos = 2;

/* XCBR2 */
static volatile int xcbr2_pos     = 2;
static volatile int xcbr2_opCnt   = 0;
static volatile int xcbr2_pending = 0;
static volatile int xcbr2_new_pos = 2;

/* XSWI1 */
static volatile int xswi1_pos     = 2;
static volatile int xswi1_pending = 0;
static volatile int xswi1_new_pos = 2;

/* ================================================================
 *  シグナルハンドラ
 * ================================================================ */

static void sigint_handler(int signalId)
{
    running = 0;
}

/* ================================================================
 *  制御ハンドラ
 *
 *  データモデルの直接更新を行わない。
 *  _pending フラグと _new_pos を設定するのみ。
 * ================================================================ */

static CheckHandlerResult
controlCheckHandler(ControlAction action, void *parameter,
                    MmsValue *ctlVal, bool test, bool interlockCheck,
                    ClientConnection connection)
{
    (void)parameter;
    (void)test;
    (void)interlockCheck;
    (void)connection;

    if (ctlVal != NULL) {
        printf("[CHECK] Control check: ctlVal=%s\n",
               MmsValue_getBoolean(ctlVal) ? "true(Close)" : "false(Open)");
    }

    return CONTROL_ACCEPTED;
}

/* XCBR1 制御ハンドラ */
static ControlHandlerResult
xcbr1_controlHandler(ControlAction action, void *parameter,
                     MmsValue *value, bool test)
{
    (void)parameter;

    if (value == NULL)
        return CONTROL_RESULT_FAILED;

    bool ctlVal = MmsValue_getBoolean(value);
    int newPos = ctlVal ? 2 : 1;

    const char *clientAddr = "unknown";
    if (action != NULL) {
        ClientConnection client = ControlAction_getClientConnection(action);
        if (client != NULL) {
            const char *addr = ClientConnection_getPeerAddress(client);
            if (addr != NULL)
                clientAddr = addr;
        }
    }

    printf("==========================================================\n");
    printf("[CONTROL] XCBR1 command from %s\n", clientAddr);
    printf("[CONTROL]   ctlVal = %s\n", ctlVal ? "true (CLOSE)" : "false (OPEN)");
    printf("[CONTROL]   Current state = %d (%s)\n", xcbr1_pos,
           xcbr1_pos == 2 ? "Closed" : xcbr1_pos == 1 ? "Open" : "Intermediate");
    printf("[CONTROL]   Requested state = %d (%s)\n", newPos,
           newPos == 2 ? "Closed" : "Open");

    if (test) {
        printf("[CONTROL]   (Test mode — no actual change)\n");
        printf("==========================================================\n");
        return CONTROL_RESULT_OK;
    }

    xcbr1_new_pos = newPos;
    xcbr1_pending = 1;

    printf("[CONTROL]   → Queued for main thread update\n");
    printf("==========================================================\n");

    return CONTROL_RESULT_OK;
}

/* XCBR2 制御ハンドラ */
static ControlHandlerResult
xcbr2_controlHandler(ControlAction action, void *parameter,
                     MmsValue *value, bool test)
{
    (void)parameter;

    if (value == NULL)
        return CONTROL_RESULT_FAILED;

    bool ctlVal = MmsValue_getBoolean(value);
    int newPos = ctlVal ? 2 : 1;

    const char *clientAddr = "unknown";
    if (action != NULL) {
        ClientConnection client = ControlAction_getClientConnection(action);
        if (client != NULL) {
            const char *addr = ClientConnection_getPeerAddress(client);
            if (addr != NULL)
                clientAddr = addr;
        }
    }

    printf("[CONTROL] XCBR2 command from %s: %s → %d → %d\n",
           clientAddr, ctlVal ? "CLOSE" : "OPEN", xcbr2_pos, newPos);

    if (test)
        return CONTROL_RESULT_OK;

    xcbr2_new_pos = newPos;
    xcbr2_pending = 1;

    return CONTROL_RESULT_OK;
}

/* XSWI1 制御ハンドラ */
static ControlHandlerResult
xswi1_controlHandler(ControlAction action, void *parameter,
                     MmsValue *value, bool test)
{
    (void)parameter;

    if (value == NULL)
        return CONTROL_RESULT_FAILED;

    bool ctlVal = MmsValue_getBoolean(value);
    int newPos = ctlVal ? 2 : 1;

    const char *clientAddr = "unknown";
    if (action != NULL) {
        ClientConnection client = ControlAction_getClientConnection(action);
        if (client != NULL) {
            const char *addr = ClientConnection_getPeerAddress(client);
            if (addr != NULL)
                clientAddr = addr;
        }
    }

    printf("[CONTROL] XSWI1 command from %s: %s → %d → %d\n",
           clientAddr, ctlVal ? "CLOSE" : "OPEN", xswi1_pos, newPos);

    if (test)
        return CONTROL_RESULT_OK;

    xswi1_new_pos = newPos;
    xswi1_pending = 1;

    return CONTROL_RESULT_OK;
}

/* ================================================================
 *  MMS Write ハンドラ（CF PosCmd 制御方式）
 *
 *  IEC 61850 の機能制約（FC）と各レイヤーの制約:
 *
 *    FC    KEPServerEX OPC DA    libIEC61850 MMS Write    結論
 *    ──    ───────────────    ───────────────────    ────
 *    ST    Read Only     ✗       受入可能 (Handler)  ✓    ✗
 *    CO    Read/Write    ✓       拒否 (ハードコード)  ✗    ✗
 *    CF    Read/Write    ✓       受入可能 (Handler)  ✓    ✓ ★
 *
 *  CF 属性 PosCmd (INT32) を DPC に追加し、制御コマンドの入力点とする。
 *    値: 1=Open（遮断器開放）, 2=Close（遮断器投入）
 *
 *  KEPServerEX タグ: XCBR1$CF$Pos$PosCmd (INT32, Read/Write)
 * ================================================================ */

static MmsDataAccessError
posCmdWriteHandler(DataAttribute *dataAttribute, MmsValue *value,
                   ClientConnection connection, void *parameter)
{
    const char *clientAddr = "unknown";
    if (connection != NULL) {
        const char *addr = ClientConnection_getPeerAddress(connection);
        if (addr != NULL)
            clientAddr = addr;
    }

    int cmd = MmsValue_toInt32(value);

    /* 値検証: 1=Open, 2=Close のみ受付 */
    if (cmd != 1 && cmd != 2) {
        printf("[CMD] Rejected: invalid PosCmd=%d from %s (expected 1=Open or 2=Close)\n",
               cmd, clientAddr);
        return DATA_ACCESS_ERROR_OBJECT_VALUE_INVALID;
    }

    int deviceId = (int)(intptr_t)parameter;

    switch (deviceId) {
    case 1: /* XCBR1 */
        printf("========================================================\n");
        printf("[CMD] XCBR1 PosCmd=%d (%s) from %s\n",
               cmd, cmd == 1 ? "OPEN" : "CLOSE", clientAddr);
        printf("========================================================\n");
        xcbr1_new_pos = cmd;
        xcbr1_pending = 1;
        break;
    case 2: /* XCBR2 */
        printf("[CMD] XCBR2 PosCmd=%d (%s) from %s\n",
               cmd, cmd == 1 ? "OPEN" : "CLOSE", clientAddr);
        xcbr2_new_pos = cmd;
        xcbr2_pending = 1;
        break;
    case 3: /* XSWI1 */
        printf("[CMD] XSWI1 PosCmd=%d (%s) from %s\n",
               cmd, cmd == 1 ? "OPEN" : "CLOSE", clientAddr);
        xswi1_new_pos = cmd;
        xswi1_pending = 1;
        break;
    default:
        return DATA_ACCESS_ERROR_OBJECT_ACCESS_DENIED;
    }

    return DATA_ACCESS_ERROR_SUCCESS;
}

/* ================================================================
 *  制御コマンドの遅延処理
 * ================================================================ */

static void process_pending_controls(IedServer server)
{
    /* XCBR1 */
    if (xcbr1_pending) {
        int newPos = xcbr1_new_pos;
        if (xcbr1_pos != newPos) {
            xcbr1_pos = newPos;
            xcbr1_opCnt++;
            printf("[UPDATE] XCBR1: state=%d (%s), OpCnt=%d\n",
                   xcbr1_pos, xcbr1_pos == 2 ? "Closed" : "Open", xcbr1_opCnt);
        }
        IedServer_updateDbposValue(server,
            IEDMODEL_CBIED_XCBR1_Pos_stVal, (Dbpos)xcbr1_pos);
        IedServer_updateInt32AttributeValue(server,
            IEDMODEL_CBIED_XCBR1_OpCnt_stVal, xcbr1_opCnt);
        xcbr1_pending = 0;
    }

    /* XCBR2 */
    if (xcbr2_pending) {
        int newPos = xcbr2_new_pos;
        if (xcbr2_pos != newPos) {
            xcbr2_pos = newPos;
            xcbr2_opCnt++;
            printf("[UPDATE] XCBR2: state=%d (%s), OpCnt=%d\n",
                   xcbr2_pos, xcbr2_pos == 2 ? "Closed" : "Open", xcbr2_opCnt);
        }
        IedServer_updateDbposValue(server,
            IEDMODEL_CBIED_XCBR2_Pos_stVal, (Dbpos)xcbr2_pos);
        IedServer_updateInt32AttributeValue(server,
            IEDMODEL_CBIED_XCBR2_OpCnt_stVal, xcbr2_opCnt);
        xcbr2_pending = 0;
    }

    /* XSWI1 */
    if (xswi1_pending) {
        int newPos = xswi1_new_pos;
        if (xswi1_pos != newPos) {
            xswi1_pos = newPos;
            printf("[UPDATE] XSWI1: state=%d (%s)\n",
                   xswi1_pos, xswi1_pos == 2 ? "Closed" : "Open");
        }
        IedServer_updateDbposValue(server,
            IEDMODEL_CBIED_XSWI1_Pos_stVal, (Dbpos)xswi1_pos);
        xswi1_pending = 0;
    }
}

/* ================================================================
 *  計測値シミュレーション
 * ================================================================ */

static void update_measurements(IedServer server)
{
    static double t = 0.0;
    t += 0.25;

    /* --- 三相電流 [A] --- */
    float base_current = (xcbr1_pos == 2) ? 250.0f : 0.0f;
    float ia = base_current + (float)(5.0 * sin(t * 0.7));
    float ib = base_current + (float)(5.0 * sin(t * 0.7 + 2.094));
    float ic = base_current + (float)(5.0 * sin(t * 0.7 + 4.189));

    IedServer_updateFloatAttributeValue(server,
        IEDMODEL_CBIED_MMXU1_A_phsA_cVal_mag_f, ia);
    IedServer_updateFloatAttributeValue(server,
        IEDMODEL_CBIED_MMXU1_A_phsB_cVal_mag_f, ib);
    IedServer_updateFloatAttributeValue(server,
        IEDMODEL_CBIED_MMXU1_A_phsC_cVal_mag_f, ic);

    IedServer_updateFloatAttributeValue(server,
        IEDMODEL_CBIED_MMXU1_A_phsA_cVal_ang_f, 0.0f);
    IedServer_updateFloatAttributeValue(server,
        IEDMODEL_CBIED_MMXU1_A_phsB_cVal_ang_f, 120.0f);
    IedServer_updateFloatAttributeValue(server,
        IEDMODEL_CBIED_MMXU1_A_phsC_cVal_ang_f, 240.0f);

    /* --- 三相電圧 [kV] --- */
    float base_voltage = 110.0f;
    float va = base_voltage + (float)(0.5 * sin(t * 0.3));
    float vb = base_voltage + (float)(0.5 * sin(t * 0.3 + 2.094));
    float vc = base_voltage + (float)(0.5 * sin(t * 0.3 + 4.189));

    IedServer_updateFloatAttributeValue(server,
        IEDMODEL_CBIED_MMXU1_PhV_phsA_cVal_mag_f, va);
    IedServer_updateFloatAttributeValue(server,
        IEDMODEL_CBIED_MMXU1_PhV_phsB_cVal_mag_f, vb);
    IedServer_updateFloatAttributeValue(server,
        IEDMODEL_CBIED_MMXU1_PhV_phsC_cVal_mag_f, vc);

    IedServer_updateFloatAttributeValue(server,
        IEDMODEL_CBIED_MMXU1_PhV_phsA_cVal_ang_f, 0.0f);
    IedServer_updateFloatAttributeValue(server,
        IEDMODEL_CBIED_MMXU1_PhV_phsB_cVal_ang_f, 120.0f);
    IedServer_updateFloatAttributeValue(server,
        IEDMODEL_CBIED_MMXU1_PhV_phsC_cVal_ang_f, 240.0f);

    /* --- 周波数 [Hz] --- */
    float freq = 50.0f + (float)(0.02 * sin(t * 0.05));
    IedServer_updateFloatAttributeValue(server,
        IEDMODEL_CBIED_MMXU1_Hz_mag_f, freq);
}

/* ================================================================
 *  接続イベントハンドラ
 * ================================================================ */

static void connectionHandler(IedServer server, ClientConnection connection,
                               bool connected, void *parameter)
{
    (void)server;
    (void)parameter;

    const char *clientAddr = "unknown";
    if (connection != NULL) {
        const char *addr = ClientConnection_getPeerAddress(connection);
        if (addr != NULL)
            clientAddr = addr;
    }

    if (connected) {
        printf("[CONNECT]    Client connected: %s\n", clientAddr);
    } else {
        printf("[DISCONNECT] Client disconnected: %s\n", clientAddr);
    }
}

/* ================================================================
 *  メインループ
 * ================================================================ */

int main(int argc, char **argv)
{
    int tcpPort = 102;

    if (argc > 1) {
        tcpPort = atoi(argv[1]);
    }

    printf("============================================================\n");
    printf("  Substation IED Simulator v4\n");
    printf("  Industroyer Target Environment\n");
    printf("============================================================\n");
    printf("  Logical Device : CBIED\n");
    printf("  Breakers       : XCBR1 (Bay1), XCBR2 (Bay2)\n");
    printf("  Disconnector   : XSWI1\n");
    printf("  Measurement    : MMXU1 (3-phase I/V/Hz)\n");
    printf("  MMS Port       : %d\n", tcpPort);
    printf("  Update Interval: 250 ms\n");
    printf("  Control via    : CF$PosCmd (MMS Write)\n");
    printf("    PosCmd=1 -> Open,  PosCmd=2 -> Close\n");
    printf("============================================================\n\n");

    /* IedServer 作成 */
    iedServer = IedServer_create(&iedModel);

    if (iedServer == NULL) {
        fprintf(stderr, "[ERROR] Failed to create IedServer\n");
        return 1;
    }

    /*
     * 制御ハンドラの登録
     *
     * setControlHandler には DataObject (_Pos まで) を渡す。
     * ICD v3 では Oper が構造体 DA になったが、
     * setControlHandler の引数は DataObject のままで変更なし。
     */
    IedServer_setControlHandler(iedServer,
        IEDMODEL_CBIED_XCBR1_Pos,
        (ControlHandler)xcbr1_controlHandler, NULL);
    IedServer_setPerformCheckHandler(iedServer,
        IEDMODEL_CBIED_XCBR1_Pos,
        (ControlPerformCheckHandler)controlCheckHandler, NULL);

    IedServer_setControlHandler(iedServer,
        IEDMODEL_CBIED_XCBR2_Pos,
        (ControlHandler)xcbr2_controlHandler, NULL);
    IedServer_setPerformCheckHandler(iedServer,
        IEDMODEL_CBIED_XCBR2_Pos,
        (ControlPerformCheckHandler)controlCheckHandler, NULL);

    IedServer_setControlHandler(iedServer,
        IEDMODEL_CBIED_XSWI1_Pos,
        (ControlHandler)xswi1_controlHandler, NULL);
    IedServer_setPerformCheckHandler(iedServer,
        IEDMODEL_CBIED_XSWI1_Pos,
        (ControlPerformCheckHandler)controlCheckHandler, NULL);

    /* 接続イベントハンドラ */
    IedServer_setConnectionIndicationHandler(iedServer,
        connectionHandler, NULL);

    /*
     * CF への MMS Write を許可 + PosCmd の WriteAccessHandler 登録
     *
     * CF (Configuration) は IEC 61850 で唯一、
     * KEPServerEX が Read/Write とし、かつ libIEC61850 も
     * MMS Write を受け付ける機能制約。
     *
     * KEPServerEX Quick Client / Cogent DataHub から:
     *   XCBR1$CF$Pos$PosCmd に 1 (Open) or 2 (Close) を Write
     *   → posCmdWriteHandler → pending → メインスレッドで状態更新
     */
    IedServer_setWriteAccessPolicy(iedServer,
        IEC61850_FC_CF, ACCESS_POLICY_ALLOW);

    IedServer_handleWriteAccess(iedServer,
        IEDMODEL_CBIED_XCBR1_Pos_PosCmd,
        (WriteAccessHandler)posCmdWriteHandler, (void*)(intptr_t)1);
    IedServer_handleWriteAccess(iedServer,
        IEDMODEL_CBIED_XCBR2_Pos_PosCmd,
        (WriteAccessHandler)posCmdWriteHandler, (void*)(intptr_t)2);
    IedServer_handleWriteAccess(iedServer,
        IEDMODEL_CBIED_XSWI1_Pos_PosCmd,
        (WriteAccessHandler)posCmdWriteHandler, (void*)(intptr_t)3);

    printf("[+] Write handlers registered for CF$PosCmd\n");

    /* ================================================================
     *  初期値の設定（サーバ起動前に全属性を明示的に初期化）
     *
     *  genmodel が生成する static_model.c では全ての MmsValue が
     *  アロケートされるが、Check や Octet64 の初期バッファが
     *  正しく確保されない場合がある。
     *  MMS Read で未初期化値が読まれるとクラッシュするため、
     *  ここで全属性を明示的に初期化する。
     * ================================================================ */

    /* --- LLN0 --- */
    IedServer_updateInt32AttributeValue(iedServer,
        IEDMODEL_CBIED_LLN0_Mod_stVal, 1);
    IedServer_updateInt32AttributeValue(iedServer,
        IEDMODEL_CBIED_LLN0_Beh_stVal, 1);
    IedServer_updateInt32AttributeValue(iedServer,
        IEDMODEL_CBIED_LLN0_Health_stVal, 1);

    /* --- XCBR1 --- */
    IedServer_updateDbposValue(iedServer,
        IEDMODEL_CBIED_XCBR1_Pos_stVal, DBPOS_ON);
    IedServer_updateInt32AttributeValue(iedServer,
        IEDMODEL_CBIED_XCBR1_Beh_stVal, 1);
    IedServer_updateInt32AttributeValue(iedServer,
        IEDMODEL_CBIED_XCBR1_Health_stVal, 1);
    IedServer_updateInt32AttributeValue(iedServer,
        IEDMODEL_CBIED_XCBR1_OpCnt_stVal, 0);
    IedServer_updateInt32AttributeValue(iedServer,
        IEDMODEL_CBIED_XCBR1_Pos_ctlModel, 1);
    IedServer_updateInt32AttributeValue(iedServer,
        IEDMODEL_CBIED_XCBR1_Pos_PosCmd, 2);  /* 初期状態: Closed */

    /* --- XCBR2 --- */
    IedServer_updateDbposValue(iedServer,
        IEDMODEL_CBIED_XCBR2_Pos_stVal, DBPOS_ON);
    IedServer_updateInt32AttributeValue(iedServer,
        IEDMODEL_CBIED_XCBR2_Beh_stVal, 1);
    IedServer_updateInt32AttributeValue(iedServer,
        IEDMODEL_CBIED_XCBR2_Health_stVal, 1);
    IedServer_updateInt32AttributeValue(iedServer,
        IEDMODEL_CBIED_XCBR2_OpCnt_stVal, 0);
    IedServer_updateInt32AttributeValue(iedServer,
        IEDMODEL_CBIED_XCBR2_Pos_ctlModel, 1);
    IedServer_updateInt32AttributeValue(iedServer,
        IEDMODEL_CBIED_XCBR2_Pos_PosCmd, 2);  /* 初期状態: Closed */

    /* --- XSWI1 --- */
    IedServer_updateDbposValue(iedServer,
        IEDMODEL_CBIED_XSWI1_Pos_stVal, DBPOS_ON);
    IedServer_updateInt32AttributeValue(iedServer,
        IEDMODEL_CBIED_XSWI1_Beh_stVal, 1);
    IedServer_updateInt32AttributeValue(iedServer,
        IEDMODEL_CBIED_XSWI1_Health_stVal, 1);
    IedServer_updateInt32AttributeValue(iedServer,
        IEDMODEL_CBIED_XSWI1_Pos_ctlModel, 1);
    IedServer_updateInt32AttributeValue(iedServer,
        IEDMODEL_CBIED_XSWI1_Pos_PosCmd, 2);  /* 初期状態: Closed */

    /* --- MMXU1 --- */
    IedServer_updateInt32AttributeValue(iedServer,
        IEDMODEL_CBIED_MMXU1_Beh_stVal, 1);
    IedServer_updateInt32AttributeValue(iedServer,
        IEDMODEL_CBIED_MMXU1_Health_stVal, 1);

    /* 計測値の初期化（全フロート値を明示的に 0.0 で初期化） */
    IedServer_updateFloatAttributeValue(iedServer,
        IEDMODEL_CBIED_MMXU1_A_phsA_cVal_mag_f, 250.0f);
    IedServer_updateFloatAttributeValue(iedServer,
        IEDMODEL_CBIED_MMXU1_A_phsB_cVal_mag_f, 250.0f);
    IedServer_updateFloatAttributeValue(iedServer,
        IEDMODEL_CBIED_MMXU1_A_phsC_cVal_mag_f, 250.0f);
    IedServer_updateFloatAttributeValue(iedServer,
        IEDMODEL_CBIED_MMXU1_A_phsA_cVal_ang_f, 0.0f);
    IedServer_updateFloatAttributeValue(iedServer,
        IEDMODEL_CBIED_MMXU1_A_phsB_cVal_ang_f, 120.0f);
    IedServer_updateFloatAttributeValue(iedServer,
        IEDMODEL_CBIED_MMXU1_A_phsC_cVal_ang_f, 240.0f);
    IedServer_updateFloatAttributeValue(iedServer,
        IEDMODEL_CBIED_MMXU1_PhV_phsA_cVal_mag_f, 110.0f);
    IedServer_updateFloatAttributeValue(iedServer,
        IEDMODEL_CBIED_MMXU1_PhV_phsB_cVal_mag_f, 110.0f);
    IedServer_updateFloatAttributeValue(iedServer,
        IEDMODEL_CBIED_MMXU1_PhV_phsC_cVal_mag_f, 110.0f);
    IedServer_updateFloatAttributeValue(iedServer,
        IEDMODEL_CBIED_MMXU1_PhV_phsA_cVal_ang_f, 0.0f);
    IedServer_updateFloatAttributeValue(iedServer,
        IEDMODEL_CBIED_MMXU1_PhV_phsB_cVal_ang_f, 120.0f);
    IedServer_updateFloatAttributeValue(iedServer,
        IEDMODEL_CBIED_MMXU1_PhV_phsC_cVal_ang_f, 240.0f);
    IedServer_updateFloatAttributeValue(iedServer,
        IEDMODEL_CBIED_MMXU1_Hz_mag_f, 50.0f);

    printf("[+] All data model values initialized\n");

    /* --- MMS サーバ起動 --- */
    IedServer_start(iedServer, tcpPort);

    if (!IedServer_isRunning(iedServer)) {
        fprintf(stderr, "[ERROR] Failed to start MMS server on port %d\n", tcpPort);
        fprintf(stderr, "        (port 102 requires root/admin privileges)\n");
        IedServer_destroy(iedServer);
        return 1;
    }

    printf("[+] MMS server started on port %d\n", tcpPort);
    printf("[+] Waiting for connections... (Ctrl+C to stop)\n\n");

    signal(SIGINT, sigint_handler);

    /* メインループ */
    while (running) {
        IedServer_lockDataModel(iedServer);
        process_pending_controls(iedServer);
        update_measurements(iedServer);
        IedServer_unlockDataModel(iedServer);

        Thread_sleep(250);
    }

    /* --- 終了処理 --- */
    printf("\n[*] Shutting down...\n");
    IedServer_stop(iedServer);
    IedServer_destroy(iedServer);

    return 0;
}
