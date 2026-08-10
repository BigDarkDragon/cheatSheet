/*
 * substation_server.c (v6 — 4遮断器同居版)
 * ================================================================
 * 1プロセスで4フィーダー分の遮断器・断路器・計測を模擬
 *
 * 構成:
 *   XCBR1〜XCBR4 : 遮断器 ×4
 *   XSWI1〜XSWI4 : 断路器 ×4
 *   MMXU1〜MMXU4 : 計測   ×4
 *
 * ビルド:
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

#define NUM_BAYS 4

/* 遮断器 XCBR1〜4 */
static volatile int xcbr_pos[NUM_BAYS]     = {2, 2, 2, 2};
static volatile int xcbr_opCnt[NUM_BAYS]   = {0, 0, 0, 0};
static volatile int xcbr_pending[NUM_BAYS] = {0, 0, 0, 0};
static volatile int xcbr_new_pos[NUM_BAYS] = {2, 2, 2, 2};

/* 断路器 XSWI1〜4 */
static volatile int xswi_pos[NUM_BAYS]     = {2, 2, 2, 2};
static volatile int xswi_pending[NUM_BAYS] = {0, 0, 0, 0};
static volatile int xswi_new_pos[NUM_BAYS] = {2, 2, 2, 2};

/* ================================================================
 *  シグナルハンドラ
 * ================================================================ */

static void sigint_handler(int signalId) { running = 0; }

/* ================================================================
 *  CF PosCmd Write ハンドラ
 *
 *  parameter: (bayIndex * 10 + deviceType)
 *    deviceType: 1=XCBR, 2=XSWI
 *    bayIndex: 0〜3
 * ================================================================ */

static MmsDataAccessError
posCmdWriteHandler(DataAttribute *dataAttribute, MmsValue *value,
                   ClientConnection connection, void *parameter)
{
    const char *clientAddr = "unknown";
    if (connection != NULL) {
        const char *addr = ClientConnection_getPeerAddress(connection);
        if (addr != NULL) clientAddr = addr;
    }

    int cmd = MmsValue_toInt32(value);
    if (cmd != 1 && cmd != 2) {
        printf("[CMD] Rejected: invalid PosCmd=%d from %s\n", cmd, clientAddr);
        return DATA_ACCESS_ERROR_OBJECT_VALUE_INVALID;
    }

    int encoded = (int)(intptr_t)parameter;
    int bayIdx = encoded / 10;       /* 0〜3 */
    int devType = encoded % 10;      /* 1=XCBR, 2=XSWI */

    if (bayIdx < 0 || bayIdx >= NUM_BAYS)
        return DATA_ACCESS_ERROR_OBJECT_ACCESS_DENIED;

    const char *devName = (devType == 1) ? "XCBR" : "XSWI";

    printf("[CMD] %s%d PosCmd=%d (%s) from %s\n",
           devName, bayIdx + 1, cmd, cmd == 1 ? "OPEN" : "CLOSE", clientAddr);

    if (devType == 1) {
        xcbr_new_pos[bayIdx] = cmd;
        xcbr_pending[bayIdx] = 1;
    } else {
        xswi_new_pos[bayIdx] = cmd;
        xswi_pending[bayIdx] = 1;
    }

    return DATA_ACCESS_ERROR_SUCCESS;
}

/* ================================================================
 *  制御コマンドの遅延処理
 *
 *  static_model.h のシンボルは配列アクセスできないため、
 *  ポインタ配列でマッピングする。
 * ================================================================ */

/* genmodel が生成するシンボルへのポインタ配列 */
static DataAttribute *xcbr_pos_stVal[NUM_BAYS];
static DataAttribute *xcbr_opCnt_stVal[NUM_BAYS];
static DataAttribute *xswi_pos_stVal[NUM_BAYS];

static void init_model_pointers(void)
{
    xcbr_pos_stVal[0] = IEDMODEL_CBIED_XCBR1_Pos_stVal;
    xcbr_pos_stVal[1] = IEDMODEL_CBIED_XCBR2_Pos_stVal;
    xcbr_pos_stVal[2] = IEDMODEL_CBIED_XCBR3_Pos_stVal;
    xcbr_pos_stVal[3] = IEDMODEL_CBIED_XCBR4_Pos_stVal;

    xcbr_opCnt_stVal[0] = IEDMODEL_CBIED_XCBR1_OpCnt_stVal;
    xcbr_opCnt_stVal[1] = IEDMODEL_CBIED_XCBR2_OpCnt_stVal;
    xcbr_opCnt_stVal[2] = IEDMODEL_CBIED_XCBR3_OpCnt_stVal;
    xcbr_opCnt_stVal[3] = IEDMODEL_CBIED_XCBR4_OpCnt_stVal;

    xswi_pos_stVal[0] = IEDMODEL_CBIED_XSWI1_Pos_stVal;
    xswi_pos_stVal[1] = IEDMODEL_CBIED_XSWI2_Pos_stVal;
    xswi_pos_stVal[2] = IEDMODEL_CBIED_XSWI3_Pos_stVal;
    xswi_pos_stVal[3] = IEDMODEL_CBIED_XSWI4_Pos_stVal;
}

static void process_pending_controls(IedServer server)
{
    int i;
    for (i = 0; i < NUM_BAYS; i++) {
        if (xcbr_pending[i]) {
            int newPos = xcbr_new_pos[i];
            if (xcbr_pos[i] != newPos) {
                xcbr_pos[i] = newPos;
                xcbr_opCnt[i]++;
                printf("[UPDATE] XCBR%d: state=%d (%s), OpCnt=%d\n",
                       i + 1, xcbr_pos[i],
                       xcbr_pos[i] == 2 ? "Closed" : "Open",
                       xcbr_opCnt[i]);
            }
            IedServer_updateDbposValue(server,
                xcbr_pos_stVal[i], (Dbpos)xcbr_pos[i]);
            IedServer_updateInt32AttributeValue(server,
                xcbr_opCnt_stVal[i], xcbr_opCnt[i]);
            xcbr_pending[i] = 0;
        }

        if (xswi_pending[i]) {
            int newPos = xswi_new_pos[i];
            if (xswi_pos[i] != newPos) {
                xswi_pos[i] = newPos;
                printf("[UPDATE] XSWI%d: state=%d (%s)\n",
                       i + 1, xswi_pos[i],
                       xswi_pos[i] == 2 ? "Closed" : "Open");
            }
            IedServer_updateDbposValue(server,
                xswi_pos_stVal[i], (Dbpos)xswi_pos[i]);
            xswi_pending[i] = 0;
        }
    }
}

/* ================================================================
 *  計測値シミュレーション
 * ================================================================ */

/* MMXU の計測属性ポインタ */
static DataAttribute *mmxu_ia[NUM_BAYS], *mmxu_ib[NUM_BAYS], *mmxu_ic[NUM_BAYS];
static DataAttribute *mmxu_ia_ang[NUM_BAYS], *mmxu_ib_ang[NUM_BAYS], *mmxu_ic_ang[NUM_BAYS];
static DataAttribute *mmxu_va[NUM_BAYS], *mmxu_vb[NUM_BAYS], *mmxu_vc[NUM_BAYS];
static DataAttribute *mmxu_va_ang[NUM_BAYS], *mmxu_vb_ang[NUM_BAYS], *mmxu_vc_ang[NUM_BAYS];
static DataAttribute *mmxu_hz[NUM_BAYS];

static void init_mmxu_pointers(void)
{
    /* MMXU1 */
    mmxu_ia[0] = IEDMODEL_CBIED_MMXU1_A_phsA_cVal_mag_f;
    mmxu_ib[0] = IEDMODEL_CBIED_MMXU1_A_phsB_cVal_mag_f;
    mmxu_ic[0] = IEDMODEL_CBIED_MMXU1_A_phsC_cVal_mag_f;
    mmxu_ia_ang[0] = IEDMODEL_CBIED_MMXU1_A_phsA_cVal_ang_f;
    mmxu_ib_ang[0] = IEDMODEL_CBIED_MMXU1_A_phsB_cVal_ang_f;
    mmxu_ic_ang[0] = IEDMODEL_CBIED_MMXU1_A_phsC_cVal_ang_f;
    mmxu_va[0] = IEDMODEL_CBIED_MMXU1_PhV_phsA_cVal_mag_f;
    mmxu_vb[0] = IEDMODEL_CBIED_MMXU1_PhV_phsB_cVal_mag_f;
    mmxu_vc[0] = IEDMODEL_CBIED_MMXU1_PhV_phsC_cVal_mag_f;
    mmxu_va_ang[0] = IEDMODEL_CBIED_MMXU1_PhV_phsA_cVal_ang_f;
    mmxu_vb_ang[0] = IEDMODEL_CBIED_MMXU1_PhV_phsB_cVal_ang_f;
    mmxu_vc_ang[0] = IEDMODEL_CBIED_MMXU1_PhV_phsC_cVal_ang_f;
    mmxu_hz[0] = IEDMODEL_CBIED_MMXU1_Hz_mag_f;

    /* MMXU2 */
    mmxu_ia[1] = IEDMODEL_CBIED_MMXU2_A_phsA_cVal_mag_f;
    mmxu_ib[1] = IEDMODEL_CBIED_MMXU2_A_phsB_cVal_mag_f;
    mmxu_ic[1] = IEDMODEL_CBIED_MMXU2_A_phsC_cVal_mag_f;
    mmxu_ia_ang[1] = IEDMODEL_CBIED_MMXU2_A_phsA_cVal_ang_f;
    mmxu_ib_ang[1] = IEDMODEL_CBIED_MMXU2_A_phsB_cVal_ang_f;
    mmxu_ic_ang[1] = IEDMODEL_CBIED_MMXU2_A_phsC_cVal_ang_f;
    mmxu_va[1] = IEDMODEL_CBIED_MMXU2_PhV_phsA_cVal_mag_f;
    mmxu_vb[1] = IEDMODEL_CBIED_MMXU2_PhV_phsB_cVal_mag_f;
    mmxu_vc[1] = IEDMODEL_CBIED_MMXU2_PhV_phsC_cVal_mag_f;
    mmxu_va_ang[1] = IEDMODEL_CBIED_MMXU2_PhV_phsA_cVal_ang_f;
    mmxu_vb_ang[1] = IEDMODEL_CBIED_MMXU2_PhV_phsB_cVal_ang_f;
    mmxu_vc_ang[1] = IEDMODEL_CBIED_MMXU2_PhV_phsC_cVal_ang_f;
    mmxu_hz[1] = IEDMODEL_CBIED_MMXU2_Hz_mag_f;

    /* MMXU3 */
    mmxu_ia[2] = IEDMODEL_CBIED_MMXU3_A_phsA_cVal_mag_f;
    mmxu_ib[2] = IEDMODEL_CBIED_MMXU3_A_phsB_cVal_mag_f;
    mmxu_ic[2] = IEDMODEL_CBIED_MMXU3_A_phsC_cVal_mag_f;
    mmxu_ia_ang[2] = IEDMODEL_CBIED_MMXU3_A_phsA_cVal_ang_f;
    mmxu_ib_ang[2] = IEDMODEL_CBIED_MMXU3_A_phsB_cVal_ang_f;
    mmxu_ic_ang[2] = IEDMODEL_CBIED_MMXU3_A_phsC_cVal_ang_f;
    mmxu_va[2] = IEDMODEL_CBIED_MMXU3_PhV_phsA_cVal_mag_f;
    mmxu_vb[2] = IEDMODEL_CBIED_MMXU3_PhV_phsB_cVal_mag_f;
    mmxu_vc[2] = IEDMODEL_CBIED_MMXU3_PhV_phsC_cVal_mag_f;
    mmxu_va_ang[2] = IEDMODEL_CBIED_MMXU3_PhV_phsA_cVal_ang_f;
    mmxu_vb_ang[2] = IEDMODEL_CBIED_MMXU3_PhV_phsB_cVal_ang_f;
    mmxu_vc_ang[2] = IEDMODEL_CBIED_MMXU3_PhV_phsC_cVal_ang_f;
    mmxu_hz[2] = IEDMODEL_CBIED_MMXU3_Hz_mag_f;

    /* MMXU4 */
    mmxu_ia[3] = IEDMODEL_CBIED_MMXU4_A_phsA_cVal_mag_f;
    mmxu_ib[3] = IEDMODEL_CBIED_MMXU4_A_phsB_cVal_mag_f;
    mmxu_ic[3] = IEDMODEL_CBIED_MMXU4_A_phsC_cVal_mag_f;
    mmxu_ia_ang[3] = IEDMODEL_CBIED_MMXU4_A_phsA_cVal_ang_f;
    mmxu_ib_ang[3] = IEDMODEL_CBIED_MMXU4_A_phsB_cVal_ang_f;
    mmxu_ic_ang[3] = IEDMODEL_CBIED_MMXU4_A_phsC_cVal_ang_f;
    mmxu_va[3] = IEDMODEL_CBIED_MMXU4_PhV_phsA_cVal_mag_f;
    mmxu_vb[3] = IEDMODEL_CBIED_MMXU4_PhV_phsB_cVal_mag_f;
    mmxu_vc[3] = IEDMODEL_CBIED_MMXU4_PhV_phsC_cVal_mag_f;
    mmxu_va_ang[3] = IEDMODEL_CBIED_MMXU4_PhV_phsA_cVal_ang_f;
    mmxu_vb_ang[3] = IEDMODEL_CBIED_MMXU4_PhV_phsB_cVal_ang_f;
    mmxu_vc_ang[3] = IEDMODEL_CBIED_MMXU4_PhV_phsC_cVal_ang_f;
    mmxu_hz[3] = IEDMODEL_CBIED_MMXU4_Hz_mag_f;
}

static void update_measurements(IedServer server)
{
    static double t = 0.0;
    t += 0.25;
    int i;

    /* 系統周波数（全フィーダー共通） */
    float freq = 50.0f + (float)(0.02 * sin(t * 0.05));

    for (i = 0; i < NUM_BAYS; i++) {
        /* 遮断器 AND 断路器が共に Closed の場合のみ電流が流れる */
        int circuit = (xcbr_pos[i] == 2 && xswi_pos[i] == 2);

        /* 各フィーダーで少し位相をずらす */
        float phase_offset = (float)(i * 0.5);

        float base_i = circuit ? 250.0f : 0.0f;
        float ia = circuit ? base_i + (float)(5.0 * sin(t * 0.7 + phase_offset)) : 0.0f;
        float ib = circuit ? base_i + (float)(5.0 * sin(t * 0.7 + phase_offset + 2.094)) : 0.0f;
        float ic = circuit ? base_i + (float)(5.0 * sin(t * 0.7 + phase_offset + 4.189)) : 0.0f;

        IedServer_updateFloatAttributeValue(server, mmxu_ia[i], ia);
        IedServer_updateFloatAttributeValue(server, mmxu_ib[i], ib);
        IedServer_updateFloatAttributeValue(server, mmxu_ic[i], ic);
        IedServer_updateFloatAttributeValue(server, mmxu_ia_ang[i], 0.0f);
        IedServer_updateFloatAttributeValue(server, mmxu_ib_ang[i], 120.0f);
        IedServer_updateFloatAttributeValue(server, mmxu_ic_ang[i], 240.0f);

        float base_v = 110.0f;
        float va = base_v + (float)(0.5 * sin(t * 0.3 + phase_offset));
        float vb = base_v + (float)(0.5 * sin(t * 0.3 + phase_offset + 2.094));
        float vc = base_v + (float)(0.5 * sin(t * 0.3 + phase_offset + 4.189));

        IedServer_updateFloatAttributeValue(server, mmxu_va[i], va);
        IedServer_updateFloatAttributeValue(server, mmxu_vb[i], vb);
        IedServer_updateFloatAttributeValue(server, mmxu_vc[i], vc);
        IedServer_updateFloatAttributeValue(server, mmxu_va_ang[i], 0.0f);
        IedServer_updateFloatAttributeValue(server, mmxu_vb_ang[i], 120.0f);
        IedServer_updateFloatAttributeValue(server, mmxu_vc_ang[i], 240.0f);

        IedServer_updateFloatAttributeValue(server, mmxu_hz[i], freq);
    }
}

/* ================================================================
 *  接続イベントハンドラ
 * ================================================================ */

static void connectionHandler(IedServer server, ClientConnection connection,
                               bool connected, void *parameter)
{
    (void)server; (void)parameter;
    const char *clientAddr = "unknown";
    if (connection != NULL) {
        const char *addr = ClientConnection_getPeerAddress(connection);
        if (addr != NULL) clientAddr = addr;
    }
    printf("[%s] %s\n", connected ? "CONNECT" : "DISCONNECT", clientAddr);
}

/* ================================================================
 *  メイン
 * ================================================================ */

int main(int argc, char **argv)
{
    int tcpPort = 102;
    if (argc > 1) tcpPort = atoi(argv[1]);

    printf("============================================================\n");
    printf("  Substation IED Simulator v6 (4-Bay Single Process)\n");
    printf("  Industroyer Target Environment\n");
    printf("============================================================\n");
    printf("  Logical Device : CBIED\n");
    printf("  Breakers       : XCBR1〜XCBR4 (52R×4)\n");
    printf("  Disconnectors  : XSWI1〜XSWI4 (89R×4)\n");
    printf("  Measurement    : MMXU1〜MMXU4 (3-phase I/V/Hz×4)\n");
    printf("  MMS Port       : %d\n", tcpPort);
    printf("  Control via    : CF.PosCmd (1=Open, 2=Close)\n");
    printf("============================================================\n\n");

    /* モデルポインタ初期化 */
    init_model_pointers();
    init_mmxu_pointers();

    /* IedServer 作成 */
    iedServer = IedServer_create(&iedModel);
    if (iedServer == NULL) {
        fprintf(stderr, "[ERROR] Failed to create IedServer\n");
        return 1;
    }

    /* 接続イベント */
    IedServer_setConnectionIndicationHandler(iedServer, connectionHandler, NULL);

    /* CF Write を許可 */
    IedServer_setWriteAccessPolicy(iedServer, IEC61850_FC_CF, ACCESS_POLICY_ALLOW);

    /* PosCmd WriteHandler 登録 (encoded = bayIdx*10 + devType) */
    /* XCBR1〜4: devType=1 */
    IedServer_handleWriteAccess(iedServer, IEDMODEL_CBIED_XCBR1_Pos_PosCmd,
        (WriteAccessHandler)posCmdWriteHandler, (void*)(intptr_t)(0*10+1));
    IedServer_handleWriteAccess(iedServer, IEDMODEL_CBIED_XCBR2_Pos_PosCmd,
        (WriteAccessHandler)posCmdWriteHandler, (void*)(intptr_t)(1*10+1));
    IedServer_handleWriteAccess(iedServer, IEDMODEL_CBIED_XCBR3_Pos_PosCmd,
        (WriteAccessHandler)posCmdWriteHandler, (void*)(intptr_t)(2*10+1));
    IedServer_handleWriteAccess(iedServer, IEDMODEL_CBIED_XCBR4_Pos_PosCmd,
        (WriteAccessHandler)posCmdWriteHandler, (void*)(intptr_t)(3*10+1));

    /* XSWI1〜4: devType=2 */
    IedServer_handleWriteAccess(iedServer, IEDMODEL_CBIED_XSWI1_Pos_PosCmd,
        (WriteAccessHandler)posCmdWriteHandler, (void*)(intptr_t)(0*10+2));
    IedServer_handleWriteAccess(iedServer, IEDMODEL_CBIED_XSWI2_Pos_PosCmd,
        (WriteAccessHandler)posCmdWriteHandler, (void*)(intptr_t)(1*10+2));
    IedServer_handleWriteAccess(iedServer, IEDMODEL_CBIED_XSWI3_Pos_PosCmd,
        (WriteAccessHandler)posCmdWriteHandler, (void*)(intptr_t)(2*10+2));
    IedServer_handleWriteAccess(iedServer, IEDMODEL_CBIED_XSWI4_Pos_PosCmd,
        (WriteAccessHandler)posCmdWriteHandler, (void*)(intptr_t)(3*10+2));

    printf("[+] Write handlers registered for 4×XCBR + 4×XSWI\n");

    /* ================================================================
     *  初期値設定
     * ================================================================ */

    /* LLN0 */
    IedServer_updateInt32AttributeValue(iedServer, IEDMODEL_CBIED_LLN0_Mod_stVal, 1);
    IedServer_updateInt32AttributeValue(iedServer, IEDMODEL_CBIED_LLN0_Beh_stVal, 1);
    IedServer_updateInt32AttributeValue(iedServer, IEDMODEL_CBIED_LLN0_Health_stVal, 1);

    /* マクロで4ベイ分の初期化 */
    #define INIT_XCBR(N) do { \
        IedServer_updateDbposValue(iedServer, IEDMODEL_CBIED_XCBR##N##_Pos_stVal, DBPOS_ON); \
        IedServer_updateInt32AttributeValue(iedServer, IEDMODEL_CBIED_XCBR##N##_Beh_stVal, 1); \
        IedServer_updateInt32AttributeValue(iedServer, IEDMODEL_CBIED_XCBR##N##_Health_stVal, 1); \
        IedServer_updateInt32AttributeValue(iedServer, IEDMODEL_CBIED_XCBR##N##_OpCnt_stVal, 0); \
        IedServer_updateInt32AttributeValue(iedServer, IEDMODEL_CBIED_XCBR##N##_Pos_ctlModel, 1); \
        IedServer_updateInt32AttributeValue(iedServer, IEDMODEL_CBIED_XCBR##N##_Pos_PosCmd, 2); \
    } while(0)

    #define INIT_XSWI(N) do { \
        IedServer_updateDbposValue(iedServer, IEDMODEL_CBIED_XSWI##N##_Pos_stVal, DBPOS_ON); \
        IedServer_updateInt32AttributeValue(iedServer, IEDMODEL_CBIED_XSWI##N##_Beh_stVal, 1); \
        IedServer_updateInt32AttributeValue(iedServer, IEDMODEL_CBIED_XSWI##N##_Health_stVal, 1); \
        IedServer_updateInt32AttributeValue(iedServer, IEDMODEL_CBIED_XSWI##N##_Pos_ctlModel, 1); \
        IedServer_updateInt32AttributeValue(iedServer, IEDMODEL_CBIED_XSWI##N##_Pos_PosCmd, 2); \
    } while(0)

    #define INIT_MMXU(N) do { \
        IedServer_updateInt32AttributeValue(iedServer, IEDMODEL_CBIED_MMXU##N##_Beh_stVal, 1); \
        IedServer_updateInt32AttributeValue(iedServer, IEDMODEL_CBIED_MMXU##N##_Health_stVal, 1); \
    } while(0)

    INIT_XCBR(1); INIT_XCBR(2); INIT_XCBR(3); INIT_XCBR(4);
    INIT_XSWI(1); INIT_XSWI(2); INIT_XSWI(3); INIT_XSWI(4);
    INIT_MMXU(1); INIT_MMXU(2); INIT_MMXU(3); INIT_MMXU(4);

    printf("[+] All data model values initialized (4 bays)\n");

    /* MMS サーバ起動 */
    IedServer_start(iedServer, tcpPort);

    if (!IedServer_isRunning(iedServer)) {
        fprintf(stderr, "[ERROR] Failed to start MMS server on port %d\n", tcpPort);
        IedServer_destroy(iedServer);
        return 1;
    }

    printf("[+] MMS server started on port %d\n", tcpPort);
    printf("[+] Waiting for connections... (Ctrl+C to stop)\n\n");

    signal(SIGINT, sigint_handler);

    while (running) {
        IedServer_lockDataModel(iedServer);
        process_pending_controls(iedServer);
        update_measurements(iedServer);
        IedServer_unlockDataModel(iedServer);
        Thread_sleep(250);
    }

    printf("\n[*] Shutting down...\n");
    IedServer_stop(iedServer);
    IedServer_destroy(iedServer);
    return 0;
}
