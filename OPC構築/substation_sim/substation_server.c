/*
 * substation_server.c (v2 — 修正版)
 * ====================================
 * Industroyer 攻撃対象変電所の IED シミュレータ
 *
 * v1 からの修正点:
 *   [Fix #1] OCTET_STRING_64 → Octet64 (ICD ファイル側で対応)
 *   [Fix #2] IedServer_updateDbposValue に DataAttribute (_stVal) を渡すよう修正
 *   [Fix #3] 制御ハンドラからの直接的なデータモデル更新を廃止。
 *            フラグベースの遅延更新に変更し、全更新をメインスレッドの
 *            lockDataModel/unlockDataModel 区間内に集約。
 *            → double free or corruption (fasttop) を解消
 *
 * ビルド前に genmodel.jar で static_model.h/c を生成すること:
 *   java -jar <libiec61850>/tools/model_generator/genmodel.jar substation.icd
 *
 * ビルド:
 *   gcc -o substation_server substation_server.c static_model.c \
 *       -I<libiec61850>/src/inc -I<libiec61850>/hal/inc \
 *       -L<libiec61850>/build/src -liec61850 -lhal -lpthread -lm
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
 * [Fix #3] これらの変数はメインスレッドと制御ハンドラ（サーバスレッド）の
 *          両方からアクセスされる。制御ハンドラは _pending フラグを立て、
 *          メインスレッドが lockDataModel 区間内で実際の更新を行う。
 */

/* XCBR1 */
static volatile int xcbr1_pos     = 2;   /* 初期状態: Closed */
static volatile int xcbr1_opCnt   = 0;
static volatile int xcbr1_pending = 0;   /* 制御コマンド保留フラグ */
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
 *  [Fix #3] データモデルの直接更新を行わない。
 *  _pending フラグと _new_pos を設定するのみ。
 *  実際のデータモデル更新はメインループの lock 区間で行う。
 * ================================================================ */

/*
 * CheckHandler: 制御コマンドの受理可否を判定
 *   → 常に ACCEPTED を返す（検証環境のため制限なし）
 */
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

/*
 * XCBR1 制御ハンドラ
 *   ctlVal = true  → Close (Pos.stVal = 2)
 *   ctlVal = false → Open  (Pos.stVal = 1)
 *
 *   ★ Industroyer はここに false (Open) を送り込んで遮断器を開放した
 */
static ControlHandlerResult
xcbr1_controlHandler(ControlAction action, void *parameter,
                     MmsValue *value, bool test)
{
    (void)parameter;

    if (value == NULL)
        return CONTROL_RESULT_FAILED;

    bool ctlVal = MmsValue_getBoolean(value);
    int newPos = ctlVal ? 2 : 1;

    /* クライアント情報の取得（NULL チェック付き） */
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

    /*
     * [Fix #3] データモデルを直接更新しない。
     * フラグを立てて、メインスレッドに処理を委譲する。
     */
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
 *  制御コマンドの遅延処理
 *
 *  [Fix #3] メインスレッドの lock 区間内で呼ばれる。
 *  制御ハンドラが設定した pending フラグをチェックし、
 *  データモデルを安全に更新する。
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
        /*
         * [Fix #2] DataAttribute (_stVal) を渡す。
         * DataObject (_Pos) を渡すとセグメンテーションフォルトになる。
         */
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
        /* [Fix #2] _stVal を使用 */
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
        /* [Fix #2] _stVal を使用 */
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
    t += 0.25;  /* 250ms 周期に合わせたインクリメント */

    /* --- 三相電流 [A] --- */
    /* 遮断器が Closed (2) の場合のみ電流が流れる */
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
    printf("  Substation IED Simulator v2 (Fixed)\n");
    printf("  Industroyer Target Environment\n");
    printf("============================================================\n");
    printf("  Logical Device : CBIED\n");
    printf("  Breakers       : XCBR1 (Bay1), XCBR2 (Bay2)\n");
    printf("  Disconnector   : XSWI1\n");
    printf("  Measurement    : MMXU1 (3-phase I/V/Hz)\n");
    printf("  MMS Port       : %d\n", tcpPort);
    printf("  Update Interval: 250 ms\n");
    printf("============================================================\n");
    printf("  Fixes applied:\n");
    printf("    #2: updateDbposValue uses _stVal (DataAttribute)\n");
    printf("    #3: Deferred control updates (thread-safe)\n");
    printf("============================================================\n\n");

    /* IedServer 作成 */
    iedServer = IedServer_create(&iedModel);

    if (iedServer == NULL) {
        fprintf(stderr, "[ERROR] Failed to create IedServer\n");
        return 1;
    }

    /* 制御ハンドラの登録 */
    /* 注: setControlHandler には DataObject を渡す（_Pos まで。_stVal は付けない） */

    /* XCBR1 */
    IedServer_setControlHandler(iedServer,
        IEDMODEL_CBIED_XCBR1_Pos,
        (ControlHandler)xcbr1_controlHandler, NULL);
    IedServer_setPerformCheckHandler(iedServer,
        IEDMODEL_CBIED_XCBR1_Pos,
        (ControlPerformCheckHandler)controlCheckHandler, NULL);

    /* XCBR2 */
    IedServer_setControlHandler(iedServer,
        IEDMODEL_CBIED_XCBR2_Pos,
        (ControlHandler)xcbr2_controlHandler, NULL);
    IedServer_setPerformCheckHandler(iedServer,
        IEDMODEL_CBIED_XCBR2_Pos,
        (ControlPerformCheckHandler)controlCheckHandler, NULL);

    /* XSWI1 */
    IedServer_setControlHandler(iedServer,
        IEDMODEL_CBIED_XSWI1_Pos,
        (ControlHandler)xswi1_controlHandler, NULL);
    IedServer_setPerformCheckHandler(iedServer,
        IEDMODEL_CBIED_XSWI1_Pos,
        (ControlPerformCheckHandler)controlCheckHandler, NULL);

    /* 接続イベントハンドラ */
    IedServer_setConnectionIndicationHandler(iedServer,
        connectionHandler, NULL);

    /* --- 初期値の設定（サーバ起動前に行う） --- */

    /* Mod / Beh = on (1), Health = Ok (1) */
    IedServer_updateInt32AttributeValue(iedServer,
        IEDMODEL_CBIED_LLN0_Mod_stVal, 1);
    IedServer_updateInt32AttributeValue(iedServer,
        IEDMODEL_CBIED_LLN0_Beh_stVal, 1);
    IedServer_updateInt32AttributeValue(iedServer,
        IEDMODEL_CBIED_LLN0_Health_stVal, 1);

    /*
     * [Fix #2] 遮断器の初期値設定:
     * _Pos_stVal (DataAttribute) を使用する。
     * _Pos (DataObject) を使うとセグメンテーションフォルトになる。
     */
    IedServer_updateDbposValue(iedServer,
        IEDMODEL_CBIED_XCBR1_Pos_stVal, DBPOS_ON);
    IedServer_updateInt32AttributeValue(iedServer,
        IEDMODEL_CBIED_XCBR1_Beh_stVal, 1);
    IedServer_updateInt32AttributeValue(iedServer,
        IEDMODEL_CBIED_XCBR1_Health_stVal, 1);
    IedServer_updateInt32AttributeValue(iedServer,
        IEDMODEL_CBIED_XCBR1_OpCnt_stVal, 0);

    IedServer_updateDbposValue(iedServer,
        IEDMODEL_CBIED_XCBR2_Pos_stVal, DBPOS_ON);
    IedServer_updateInt32AttributeValue(iedServer,
        IEDMODEL_CBIED_XCBR2_Beh_stVal, 1);
    IedServer_updateInt32AttributeValue(iedServer,
        IEDMODEL_CBIED_XCBR2_Health_stVal, 1);
    IedServer_updateInt32AttributeValue(iedServer,
        IEDMODEL_CBIED_XCBR2_OpCnt_stVal, 0);

    IedServer_updateDbposValue(iedServer,
        IEDMODEL_CBIED_XSWI1_Pos_stVal, DBPOS_ON);
    IedServer_updateInt32AttributeValue(iedServer,
        IEDMODEL_CBIED_XSWI1_Beh_stVal, 1);
    IedServer_updateInt32AttributeValue(iedServer,
        IEDMODEL_CBIED_XSWI1_Health_stVal, 1);

    /* MMXU1 */
    IedServer_updateInt32AttributeValue(iedServer,
        IEDMODEL_CBIED_MMXU1_Beh_stVal, 1);
    IedServer_updateInt32AttributeValue(iedServer,
        IEDMODEL_CBIED_MMXU1_Health_stVal, 1);

    /* ctlModel = direct-with-normal-security (1) */
    IedServer_updateInt32AttributeValue(iedServer,
        IEDMODEL_CBIED_XCBR1_Pos_ctlModel, 1);
    IedServer_updateInt32AttributeValue(iedServer,
        IEDMODEL_CBIED_XCBR2_Pos_ctlModel, 1);
    IedServer_updateInt32AttributeValue(iedServer,
        IEDMODEL_CBIED_XSWI1_Pos_ctlModel, 1);

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

    /*
     * [Fix #3] メインループ:
     * lockDataModel → 制御コマンドの遅延処理 → 計測値更新 → unlockDataModel
     *
     * すべてのデータモデル変更はこの lock/unlock 区間内でのみ行われる。
     * 制御ハンドラ（サーバスレッド）はフラグを設定するだけで、
     * データモデルには一切触れない。これにより double free を防止する。
     */
    while (running) {
        IedServer_lockDataModel(iedServer);

        /* 1. 保留中の制御コマンドを処理 */
        process_pending_controls(iedServer);

        /* 2. 計測値を更新 */
        update_measurements(iedServer);

        IedServer_unlockDataModel(iedServer);

        Thread_sleep(250);  /* 250ms 周期（v1 の 100ms から緩和） */
    }

    /* --- 終了処理 --- */
    printf("\n[*] Shutting down...\n");
    IedServer_stop(iedServer);
    IedServer_destroy(iedServer);

    return 0;
}
