/*
 * substation_server.c
 * ===================
 * Industroyer 攻撃対象変電所の IED シミュレータ
 *
 * libIEC61850 の IedServer API を使用し、以下を実装する:
 *   - 遮断器 (XCBR1, XCBR2) の状態管理と制御ハンドラ
 *   - 断路器 (XSWI1) の状態管理
 *   - 計測値 (MMXU1: 三相電流・電圧・周波数) の周期的更新
 *   - 制御コマンド受信時のログ出力
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

#include "iec61850_server.h"
#include "hal_thread.h"
#include "static_model.h"

/* ================================================================
 *  グローバル状態
 * ================================================================ */

static int running = 1;
static IedServer iedServer = NULL;

/* 遮断器の状態 (Dbpos: 0=intermediate, 1=off/open, 2=on/closed, 3=bad) */
static int xcbr1_pos = 2;  /* 初期状態: Closed */
static int xcbr2_pos = 2;  /* 初期状態: Closed */
static int xswi1_pos = 2;  /* 初期状態: Closed */

/* 動作回数カウンタ */
static int xcbr1_opCnt = 0;
static int xcbr2_opCnt = 0;

/* ================================================================
 *  シグナルハンドラ
 * ================================================================ */

static void sigint_handler(int signalId)
{
    running = 0;
}

/* ================================================================
 *  制御ハンドラ (遮断器の Open/Close コマンド受信時に呼ばれる)
 * ================================================================ */

/*
 * CheckHandler: 制御コマンドの受理可否を判定
 *   → 常に ACCEPTED を返す（検証環境のため制限なし）
 *   → 実環境ではインターロック条件等をチェックする
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

    printf("[CHECK] Control check requested (ctlVal=%s)\n",
           MmsValue_getBoolean(ctlVal) ? "true(Close)" : "false(Open)");

    return CONTROL_ACCEPTED;
}

/*
 * ControlHandler: 制御コマンドの実行
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
    (void)action;

    bool ctlVal = MmsValue_getBoolean(value);
    int newPos = ctlVal ? 2 : 1;  /* true=Close(2), false=Open(1) */

    ClientConnection client = ControlAction_getClientConnection(action);
    const char *clientAddr = ClientConnection_getPeerAddress(client);

    printf("==========================================================\n");
    printf("[CONTROL] XCBR1 command received from %s\n", clientAddr);
    printf("[CONTROL]   ctlVal = %s\n", ctlVal ? "true (CLOSE)" : "false (OPEN)");
    printf("[CONTROL]   Old state = %d (%s)\n", xcbr1_pos,
           xcbr1_pos == 2 ? "Closed" : xcbr1_pos == 1 ? "Open" : "Intermediate");
    printf("[CONTROL]   New state = %d (%s)\n", newPos,
           newPos == 2 ? "Closed" : "Open");

    if (xcbr1_pos != newPos) {
        xcbr1_pos = newPos;
        xcbr1_opCnt++;
        printf("[CONTROL]   *** BREAKER STATE CHANGED *** OpCnt=%d\n", xcbr1_opCnt);
    }
    printf("==========================================================\n");

    if (test)
        return CONTROL_RESULT_OK;

    /* MMS データモデルに反映 */
    IedServer_updateDbposValue(iedServer,
        IEDMODEL_CBIED_XCBR1_Pos, (Dbpos)newPos);
    IedServer_updateInt32AttributeValue(iedServer,
        IEDMODEL_CBIED_XCBR1_OpCnt_stVal, xcbr1_opCnt);

    return CONTROL_RESULT_OK;
}

static ControlHandlerResult
xcbr2_controlHandler(ControlAction action, void *parameter,
                     MmsValue *value, bool test)
{
    (void)parameter;
    (void)action;

    bool ctlVal = MmsValue_getBoolean(value);
    int newPos = ctlVal ? 2 : 1;

    ClientConnection client = ControlAction_getClientConnection(action);
    const char *clientAddr = ClientConnection_getPeerAddress(client);

    printf("==========================================================\n");
    printf("[CONTROL] XCBR2 command received from %s\n", clientAddr);
    printf("[CONTROL]   ctlVal = %s → state %d → %d\n",
           ctlVal ? "CLOSE" : "OPEN", xcbr2_pos, newPos);

    if (xcbr2_pos != newPos) {
        xcbr2_pos = newPos;
        xcbr2_opCnt++;
        printf("[CONTROL]   *** BREAKER STATE CHANGED *** OpCnt=%d\n", xcbr2_opCnt);
    }
    printf("==========================================================\n");

    if (test)
        return CONTROL_RESULT_OK;

    IedServer_updateDbposValue(iedServer,
        IEDMODEL_CBIED_XCBR2_Pos, (Dbpos)newPos);
    IedServer_updateInt32AttributeValue(iedServer,
        IEDMODEL_CBIED_XCBR2_OpCnt_stVal, xcbr2_opCnt);

    return CONTROL_RESULT_OK;
}

static ControlHandlerResult
xswi1_controlHandler(ControlAction action, void *parameter,
                     MmsValue *value, bool test)
{
    (void)parameter;
    (void)action;

    bool ctlVal = MmsValue_getBoolean(value);
    int newPos = ctlVal ? 2 : 1;

    ClientConnection client = ControlAction_getClientConnection(action);
    const char *clientAddr = ClientConnection_getPeerAddress(client);

    printf("[CONTROL] XSWI1 command from %s: %s → state %d → %d\n",
           clientAddr, ctlVal ? "CLOSE" : "OPEN", xswi1_pos, newPos);

    if (xswi1_pos != newPos) {
        xswi1_pos = newPos;
    }

    if (test)
        return CONTROL_RESULT_OK;

    IedServer_updateDbposValue(iedServer,
        IEDMODEL_CBIED_XSWI1_Pos, (Dbpos)newPos);

    return CONTROL_RESULT_OK;
}

/* ================================================================
 *  計測値シミュレーション
 * ================================================================ */

/*
 * 三相電流・電圧・周波数をリアルタイムに模擬する
 * 遮断器が Open の場合、電流は 0 になる（実際の物理動作を模擬）
 */
static void update_measurements(IedServer server)
{
    static double t = 0.0;
    t += 0.1;

    /* --- 三相電流 [A] --- */
    /* 遮断器が Closed (2) の場合のみ電流が流れる */
    float base_current = (xcbr1_pos == 2) ? 250.0f : 0.0f;
    float ia = base_current + (float)(5.0 * sin(t * 0.7));
    float ib = base_current + (float)(5.0 * sin(t * 0.7 + 2.094));  /* +120° */
    float ic = base_current + (float)(5.0 * sin(t * 0.7 + 4.189));  /* +240° */

    IedServer_updateFloatAttributeValue(server,
        IEDMODEL_CBIED_MMXU1_A_phsA_cVal_mag_f, ia);
    IedServer_updateFloatAttributeValue(server,
        IEDMODEL_CBIED_MMXU1_A_phsB_cVal_mag_f, ib);
    IedServer_updateFloatAttributeValue(server,
        IEDMODEL_CBIED_MMXU1_A_phsC_cVal_mag_f, ic);

    /* 位相角 */
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

    const char *clientAddr = ClientConnection_getPeerAddress(connection);

    if (connected) {
        printf("[CONNECT]  Client connected: %s\n", clientAddr);
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
    printf("  Substation IED Simulator (Industroyer Target Environment)\n");
    printf("============================================================\n");
    printf("  Logical Device : CBIED\n");
    printf("  Breakers       : XCBR1 (Bay1), XCBR2 (Bay2)\n");
    printf("  Disconnector   : XSWI1\n");
    printf("  Measurement    : MMXU1 (3-phase I/V/Hz)\n");
    printf("  MMS Port       : %d\n", tcpPort);
    printf("============================================================\n\n");

    /* IedServer 作成 */
    iedServer = IedServer_create(&iedModel);

    if (iedServer == NULL) {
        fprintf(stderr, "[ERROR] Failed to create IedServer\n");
        return 1;
    }

    /* 制御ハンドラの登録 */

    /* XCBR1: direct-with-normal-security (ctlModel=1) */
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

    /* --- 初期値の設定 --- */

    /* Mod / Beh = on (1) */
    IedServer_updateInt32AttributeValue(iedServer,
        IEDMODEL_CBIED_LLN0_Mod_stVal, 1);
    IedServer_updateInt32AttributeValue(iedServer,
        IEDMODEL_CBIED_LLN0_Beh_stVal, 1);
    IedServer_updateInt32AttributeValue(iedServer,
        IEDMODEL_CBIED_LLN0_Health_stVal, 1); /* Ok */

    /* 遮断器: 初期状態 = Closed (2) */
    IedServer_updateDbposValue(iedServer,
        IEDMODEL_CBIED_XCBR1_Pos, DBPOS_ON);
    IedServer_updateInt32AttributeValue(iedServer,
        IEDMODEL_CBIED_XCBR1_Beh_stVal, 1);
    IedServer_updateInt32AttributeValue(iedServer,
        IEDMODEL_CBIED_XCBR1_OpCnt_stVal, 0);

    IedServer_updateDbposValue(iedServer,
        IEDMODEL_CBIED_XCBR2_Pos, DBPOS_ON);
    IedServer_updateInt32AttributeValue(iedServer,
        IEDMODEL_CBIED_XCBR2_Beh_stVal, 1);
    IedServer_updateInt32AttributeValue(iedServer,
        IEDMODEL_CBIED_XCBR2_OpCnt_stVal, 0);

    /* 断路器: 初期状態 = Closed (2) */
    IedServer_updateDbposValue(iedServer,
        IEDMODEL_CBIED_XSWI1_Pos, DBPOS_ON);
    IedServer_updateInt32AttributeValue(iedServer,
        IEDMODEL_CBIED_XSWI1_Beh_stVal, 1);

    /* MMXU1 */
    IedServer_updateInt32AttributeValue(iedServer,
        IEDMODEL_CBIED_MMXU1_Beh_stVal, 1);

    /* ctlModel を direct-with-normal-security (1) に設定 */
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

    /* --- メインループ: 計測値の周期更新 --- */
    while (running) {
        IedServer_lockDataModel(iedServer);
        update_measurements(iedServer);
        IedServer_unlockDataModel(iedServer);

        Thread_sleep(100);  /* 100ms 周期 */
    }

    /* --- 終了処理 --- */
    printf("\n[*] Shutting down...\n");
    IedServer_stop(iedServer);
    IedServer_destroy(iedServer);

    return 0;
}
