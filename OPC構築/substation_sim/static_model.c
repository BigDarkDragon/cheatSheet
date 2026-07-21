/*
 * static_model.c
 *
 * automatically generated from substation.icd
 */
#include "static_model.h"

static void initializeValues();



LogicalDevice iedModel_CBIED = {
    LogicalDeviceModelType,
    "CBIED",
    (ModelNode*) &iedModel,
    NULL,
    (ModelNode*) &iedModel_CBIED_LLN0,
    NULL
};

LogicalNode iedModel_CBIED_LLN0 = {
    LogicalNodeModelType,
    "LLN0",
    (ModelNode*) &iedModel_CBIED,
    (ModelNode*) &iedModel_CBIED_XCBR1,
    (ModelNode*) &iedModel_CBIED_LLN0_Mod,
};

DataObject iedModel_CBIED_LLN0_Mod = {
    DataObjectModelType,
    "Mod",
    (ModelNode*) &iedModel_CBIED_LLN0,
    (ModelNode*) &iedModel_CBIED_LLN0_Beh,
    (ModelNode*) &iedModel_CBIED_LLN0_Mod_stVal,
    0,
    -1
};

DataAttribute iedModel_CBIED_LLN0_Mod_stVal = {
    DataAttributeModelType,
    "stVal",
    (ModelNode*) &iedModel_CBIED_LLN0_Mod,
    (ModelNode*) &iedModel_CBIED_LLN0_Mod_q,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_ENUMERATED,
    0 + TRG_OPT_DATA_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_LLN0_Mod_q = {
    DataAttributeModelType,
    "q",
    (ModelNode*) &iedModel_CBIED_LLN0_Mod,
    (ModelNode*) &iedModel_CBIED_LLN0_Mod_t,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_QUALITY,
    0 + TRG_OPT_QUALITY_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_LLN0_Mod_t = {
    DataAttributeModelType,
    "t",
    (ModelNode*) &iedModel_CBIED_LLN0_Mod,
    (ModelNode*) &iedModel_CBIED_LLN0_Mod_ctlModel,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_TIMESTAMP,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_LLN0_Mod_ctlModel = {
    DataAttributeModelType,
    "ctlModel",
    (ModelNode*) &iedModel_CBIED_LLN0_Mod,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_CF,
    IEC61850_ENUMERATED,
    0,
    NULL,
    0};

DataObject iedModel_CBIED_LLN0_Beh = {
    DataObjectModelType,
    "Beh",
    (ModelNode*) &iedModel_CBIED_LLN0,
    (ModelNode*) &iedModel_CBIED_LLN0_Health,
    (ModelNode*) &iedModel_CBIED_LLN0_Beh_stVal,
    0,
    -1
};

DataAttribute iedModel_CBIED_LLN0_Beh_stVal = {
    DataAttributeModelType,
    "stVal",
    (ModelNode*) &iedModel_CBIED_LLN0_Beh,
    (ModelNode*) &iedModel_CBIED_LLN0_Beh_q,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_ENUMERATED,
    0 + TRG_OPT_DATA_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_LLN0_Beh_q = {
    DataAttributeModelType,
    "q",
    (ModelNode*) &iedModel_CBIED_LLN0_Beh,
    (ModelNode*) &iedModel_CBIED_LLN0_Beh_t,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_QUALITY,
    0 + TRG_OPT_QUALITY_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_LLN0_Beh_t = {
    DataAttributeModelType,
    "t",
    (ModelNode*) &iedModel_CBIED_LLN0_Beh,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_TIMESTAMP,
    0,
    NULL,
    0};

DataObject iedModel_CBIED_LLN0_Health = {
    DataObjectModelType,
    "Health",
    (ModelNode*) &iedModel_CBIED_LLN0,
    (ModelNode*) &iedModel_CBIED_LLN0_NamPlt,
    (ModelNode*) &iedModel_CBIED_LLN0_Health_stVal,
    0,
    -1
};

DataAttribute iedModel_CBIED_LLN0_Health_stVal = {
    DataAttributeModelType,
    "stVal",
    (ModelNode*) &iedModel_CBIED_LLN0_Health,
    (ModelNode*) &iedModel_CBIED_LLN0_Health_q,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_ENUMERATED,
    0 + TRG_OPT_DATA_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_LLN0_Health_q = {
    DataAttributeModelType,
    "q",
    (ModelNode*) &iedModel_CBIED_LLN0_Health,
    (ModelNode*) &iedModel_CBIED_LLN0_Health_t,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_QUALITY,
    0 + TRG_OPT_QUALITY_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_LLN0_Health_t = {
    DataAttributeModelType,
    "t",
    (ModelNode*) &iedModel_CBIED_LLN0_Health,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_TIMESTAMP,
    0,
    NULL,
    0};

DataObject iedModel_CBIED_LLN0_NamPlt = {
    DataObjectModelType,
    "NamPlt",
    (ModelNode*) &iedModel_CBIED_LLN0,
    NULL,
    (ModelNode*) &iedModel_CBIED_LLN0_NamPlt_vendor,
    0,
    -1
};

DataAttribute iedModel_CBIED_LLN0_NamPlt_vendor = {
    DataAttributeModelType,
    "vendor",
    (ModelNode*) &iedModel_CBIED_LLN0_NamPlt,
    (ModelNode*) &iedModel_CBIED_LLN0_NamPlt_swRev,
    NULL,
    0,
    -1,
    IEC61850_FC_DC,
    IEC61850_VISIBLE_STRING_255,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_LLN0_NamPlt_swRev = {
    DataAttributeModelType,
    "swRev",
    (ModelNode*) &iedModel_CBIED_LLN0_NamPlt,
    (ModelNode*) &iedModel_CBIED_LLN0_NamPlt_d,
    NULL,
    0,
    -1,
    IEC61850_FC_DC,
    IEC61850_VISIBLE_STRING_255,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_LLN0_NamPlt_d = {
    DataAttributeModelType,
    "d",
    (ModelNode*) &iedModel_CBIED_LLN0_NamPlt,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_DC,
    IEC61850_VISIBLE_STRING_255,
    0,
    NULL,
    0};

LogicalNode iedModel_CBIED_XCBR1 = {
    LogicalNodeModelType,
    "XCBR1",
    (ModelNode*) &iedModel_CBIED,
    (ModelNode*) &iedModel_CBIED_XCBR2,
    (ModelNode*) &iedModel_CBIED_XCBR1_Mod,
};

DataObject iedModel_CBIED_XCBR1_Mod = {
    DataObjectModelType,
    "Mod",
    (ModelNode*) &iedModel_CBIED_XCBR1,
    (ModelNode*) &iedModel_CBIED_XCBR1_Beh,
    (ModelNode*) &iedModel_CBIED_XCBR1_Mod_stVal,
    0,
    -1
};

DataAttribute iedModel_CBIED_XCBR1_Mod_stVal = {
    DataAttributeModelType,
    "stVal",
    (ModelNode*) &iedModel_CBIED_XCBR1_Mod,
    (ModelNode*) &iedModel_CBIED_XCBR1_Mod_q,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_ENUMERATED,
    0 + TRG_OPT_DATA_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_XCBR1_Mod_q = {
    DataAttributeModelType,
    "q",
    (ModelNode*) &iedModel_CBIED_XCBR1_Mod,
    (ModelNode*) &iedModel_CBIED_XCBR1_Mod_t,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_QUALITY,
    0 + TRG_OPT_QUALITY_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_XCBR1_Mod_t = {
    DataAttributeModelType,
    "t",
    (ModelNode*) &iedModel_CBIED_XCBR1_Mod,
    (ModelNode*) &iedModel_CBIED_XCBR1_Mod_ctlModel,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_TIMESTAMP,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_XCBR1_Mod_ctlModel = {
    DataAttributeModelType,
    "ctlModel",
    (ModelNode*) &iedModel_CBIED_XCBR1_Mod,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_CF,
    IEC61850_ENUMERATED,
    0,
    NULL,
    0};

DataObject iedModel_CBIED_XCBR1_Beh = {
    DataObjectModelType,
    "Beh",
    (ModelNode*) &iedModel_CBIED_XCBR1,
    (ModelNode*) &iedModel_CBIED_XCBR1_Health,
    (ModelNode*) &iedModel_CBIED_XCBR1_Beh_stVal,
    0,
    -1
};

DataAttribute iedModel_CBIED_XCBR1_Beh_stVal = {
    DataAttributeModelType,
    "stVal",
    (ModelNode*) &iedModel_CBIED_XCBR1_Beh,
    (ModelNode*) &iedModel_CBIED_XCBR1_Beh_q,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_ENUMERATED,
    0 + TRG_OPT_DATA_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_XCBR1_Beh_q = {
    DataAttributeModelType,
    "q",
    (ModelNode*) &iedModel_CBIED_XCBR1_Beh,
    (ModelNode*) &iedModel_CBIED_XCBR1_Beh_t,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_QUALITY,
    0 + TRG_OPT_QUALITY_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_XCBR1_Beh_t = {
    DataAttributeModelType,
    "t",
    (ModelNode*) &iedModel_CBIED_XCBR1_Beh,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_TIMESTAMP,
    0,
    NULL,
    0};

DataObject iedModel_CBIED_XCBR1_Health = {
    DataObjectModelType,
    "Health",
    (ModelNode*) &iedModel_CBIED_XCBR1,
    (ModelNode*) &iedModel_CBIED_XCBR1_Pos,
    (ModelNode*) &iedModel_CBIED_XCBR1_Health_stVal,
    0,
    -1
};

DataAttribute iedModel_CBIED_XCBR1_Health_stVal = {
    DataAttributeModelType,
    "stVal",
    (ModelNode*) &iedModel_CBIED_XCBR1_Health,
    (ModelNode*) &iedModel_CBIED_XCBR1_Health_q,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_ENUMERATED,
    0 + TRG_OPT_DATA_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_XCBR1_Health_q = {
    DataAttributeModelType,
    "q",
    (ModelNode*) &iedModel_CBIED_XCBR1_Health,
    (ModelNode*) &iedModel_CBIED_XCBR1_Health_t,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_QUALITY,
    0 + TRG_OPT_QUALITY_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_XCBR1_Health_t = {
    DataAttributeModelType,
    "t",
    (ModelNode*) &iedModel_CBIED_XCBR1_Health,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_TIMESTAMP,
    0,
    NULL,
    0};

DataObject iedModel_CBIED_XCBR1_Pos = {
    DataObjectModelType,
    "Pos",
    (ModelNode*) &iedModel_CBIED_XCBR1,
    (ModelNode*) &iedModel_CBIED_XCBR1_OpCnt,
    (ModelNode*) &iedModel_CBIED_XCBR1_Pos_stVal,
    0,
    -1
};

DataAttribute iedModel_CBIED_XCBR1_Pos_stVal = {
    DataAttributeModelType,
    "stVal",
    (ModelNode*) &iedModel_CBIED_XCBR1_Pos,
    (ModelNode*) &iedModel_CBIED_XCBR1_Pos_q,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_CODEDENUM,
    0 + TRG_OPT_DATA_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_XCBR1_Pos_q = {
    DataAttributeModelType,
    "q",
    (ModelNode*) &iedModel_CBIED_XCBR1_Pos,
    (ModelNode*) &iedModel_CBIED_XCBR1_Pos_t,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_QUALITY,
    0 + TRG_OPT_QUALITY_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_XCBR1_Pos_t = {
    DataAttributeModelType,
    "t",
    (ModelNode*) &iedModel_CBIED_XCBR1_Pos,
    (ModelNode*) &iedModel_CBIED_XCBR1_Pos_Oper,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_TIMESTAMP,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_XCBR1_Pos_Oper = {
    DataAttributeModelType,
    "Oper",
    (ModelNode*) &iedModel_CBIED_XCBR1_Pos,
    (ModelNode*) &iedModel_CBIED_XCBR1_Pos_ctlModel,
    (ModelNode*) &iedModel_CBIED_XCBR1_Pos_Oper_ctlVal,
    0,
    -1,
    IEC61850_FC_CO,
    IEC61850_CONSTRUCTED,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_XCBR1_Pos_Oper_ctlVal = {
    DataAttributeModelType,
    "ctlVal",
    (ModelNode*) &iedModel_CBIED_XCBR1_Pos_Oper,
    (ModelNode*) &iedModel_CBIED_XCBR1_Pos_Oper_origin,
    NULL,
    0,
    -1,
    IEC61850_FC_CO,
    IEC61850_BOOLEAN,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_XCBR1_Pos_Oper_origin = {
    DataAttributeModelType,
    "origin",
    (ModelNode*) &iedModel_CBIED_XCBR1_Pos_Oper,
    (ModelNode*) &iedModel_CBIED_XCBR1_Pos_Oper_ctlNum,
    (ModelNode*) &iedModel_CBIED_XCBR1_Pos_Oper_origin_orCat,
    0,
    -1,
    IEC61850_FC_CO,
    IEC61850_CONSTRUCTED,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_XCBR1_Pos_Oper_origin_orCat = {
    DataAttributeModelType,
    "orCat",
    (ModelNode*) &iedModel_CBIED_XCBR1_Pos_Oper_origin,
    (ModelNode*) &iedModel_CBIED_XCBR1_Pos_Oper_origin_orIdent,
    NULL,
    0,
    -1,
    IEC61850_FC_CO,
    IEC61850_ENUMERATED,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_XCBR1_Pos_Oper_origin_orIdent = {
    DataAttributeModelType,
    "orIdent",
    (ModelNode*) &iedModel_CBIED_XCBR1_Pos_Oper_origin,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_CO,
    IEC61850_OCTET_STRING_64,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_XCBR1_Pos_Oper_ctlNum = {
    DataAttributeModelType,
    "ctlNum",
    (ModelNode*) &iedModel_CBIED_XCBR1_Pos_Oper,
    (ModelNode*) &iedModel_CBIED_XCBR1_Pos_Oper_T,
    NULL,
    0,
    -1,
    IEC61850_FC_CO,
    IEC61850_INT8U,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_XCBR1_Pos_Oper_T = {
    DataAttributeModelType,
    "T",
    (ModelNode*) &iedModel_CBIED_XCBR1_Pos_Oper,
    (ModelNode*) &iedModel_CBIED_XCBR1_Pos_Oper_Test,
    NULL,
    0,
    -1,
    IEC61850_FC_CO,
    IEC61850_TIMESTAMP,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_XCBR1_Pos_Oper_Test = {
    DataAttributeModelType,
    "Test",
    (ModelNode*) &iedModel_CBIED_XCBR1_Pos_Oper,
    (ModelNode*) &iedModel_CBIED_XCBR1_Pos_Oper_Check,
    NULL,
    0,
    -1,
    IEC61850_FC_CO,
    IEC61850_BOOLEAN,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_XCBR1_Pos_Oper_Check = {
    DataAttributeModelType,
    "Check",
    (ModelNode*) &iedModel_CBIED_XCBR1_Pos_Oper,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_CO,
    IEC61850_CHECK,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_XCBR1_Pos_ctlModel = {
    DataAttributeModelType,
    "ctlModel",
    (ModelNode*) &iedModel_CBIED_XCBR1_Pos,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_CF,
    IEC61850_ENUMERATED,
    0,
    NULL,
    0};

DataObject iedModel_CBIED_XCBR1_OpCnt = {
    DataObjectModelType,
    "OpCnt",
    (ModelNode*) &iedModel_CBIED_XCBR1,
    (ModelNode*) &iedModel_CBIED_XCBR1_CBOpCap,
    (ModelNode*) &iedModel_CBIED_XCBR1_OpCnt_stVal,
    0,
    -1
};

DataAttribute iedModel_CBIED_XCBR1_OpCnt_stVal = {
    DataAttributeModelType,
    "stVal",
    (ModelNode*) &iedModel_CBIED_XCBR1_OpCnt,
    (ModelNode*) &iedModel_CBIED_XCBR1_OpCnt_q,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_INT32,
    0 + TRG_OPT_DATA_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_XCBR1_OpCnt_q = {
    DataAttributeModelType,
    "q",
    (ModelNode*) &iedModel_CBIED_XCBR1_OpCnt,
    (ModelNode*) &iedModel_CBIED_XCBR1_OpCnt_t,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_QUALITY,
    0 + TRG_OPT_QUALITY_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_XCBR1_OpCnt_t = {
    DataAttributeModelType,
    "t",
    (ModelNode*) &iedModel_CBIED_XCBR1_OpCnt,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_TIMESTAMP,
    0,
    NULL,
    0};

DataObject iedModel_CBIED_XCBR1_CBOpCap = {
    DataObjectModelType,
    "CBOpCap",
    (ModelNode*) &iedModel_CBIED_XCBR1,
    NULL,
    (ModelNode*) &iedModel_CBIED_XCBR1_CBOpCap_stVal,
    0,
    -1
};

DataAttribute iedModel_CBIED_XCBR1_CBOpCap_stVal = {
    DataAttributeModelType,
    "stVal",
    (ModelNode*) &iedModel_CBIED_XCBR1_CBOpCap,
    (ModelNode*) &iedModel_CBIED_XCBR1_CBOpCap_q,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_INT32,
    0 + TRG_OPT_DATA_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_XCBR1_CBOpCap_q = {
    DataAttributeModelType,
    "q",
    (ModelNode*) &iedModel_CBIED_XCBR1_CBOpCap,
    (ModelNode*) &iedModel_CBIED_XCBR1_CBOpCap_t,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_QUALITY,
    0 + TRG_OPT_QUALITY_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_XCBR1_CBOpCap_t = {
    DataAttributeModelType,
    "t",
    (ModelNode*) &iedModel_CBIED_XCBR1_CBOpCap,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_TIMESTAMP,
    0,
    NULL,
    0};

LogicalNode iedModel_CBIED_XCBR2 = {
    LogicalNodeModelType,
    "XCBR2",
    (ModelNode*) &iedModel_CBIED,
    (ModelNode*) &iedModel_CBIED_XSWI1,
    (ModelNode*) &iedModel_CBIED_XCBR2_Mod,
};

DataObject iedModel_CBIED_XCBR2_Mod = {
    DataObjectModelType,
    "Mod",
    (ModelNode*) &iedModel_CBIED_XCBR2,
    (ModelNode*) &iedModel_CBIED_XCBR2_Beh,
    (ModelNode*) &iedModel_CBIED_XCBR2_Mod_stVal,
    0,
    -1
};

DataAttribute iedModel_CBIED_XCBR2_Mod_stVal = {
    DataAttributeModelType,
    "stVal",
    (ModelNode*) &iedModel_CBIED_XCBR2_Mod,
    (ModelNode*) &iedModel_CBIED_XCBR2_Mod_q,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_ENUMERATED,
    0 + TRG_OPT_DATA_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_XCBR2_Mod_q = {
    DataAttributeModelType,
    "q",
    (ModelNode*) &iedModel_CBIED_XCBR2_Mod,
    (ModelNode*) &iedModel_CBIED_XCBR2_Mod_t,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_QUALITY,
    0 + TRG_OPT_QUALITY_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_XCBR2_Mod_t = {
    DataAttributeModelType,
    "t",
    (ModelNode*) &iedModel_CBIED_XCBR2_Mod,
    (ModelNode*) &iedModel_CBIED_XCBR2_Mod_ctlModel,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_TIMESTAMP,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_XCBR2_Mod_ctlModel = {
    DataAttributeModelType,
    "ctlModel",
    (ModelNode*) &iedModel_CBIED_XCBR2_Mod,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_CF,
    IEC61850_ENUMERATED,
    0,
    NULL,
    0};

DataObject iedModel_CBIED_XCBR2_Beh = {
    DataObjectModelType,
    "Beh",
    (ModelNode*) &iedModel_CBIED_XCBR2,
    (ModelNode*) &iedModel_CBIED_XCBR2_Health,
    (ModelNode*) &iedModel_CBIED_XCBR2_Beh_stVal,
    0,
    -1
};

DataAttribute iedModel_CBIED_XCBR2_Beh_stVal = {
    DataAttributeModelType,
    "stVal",
    (ModelNode*) &iedModel_CBIED_XCBR2_Beh,
    (ModelNode*) &iedModel_CBIED_XCBR2_Beh_q,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_ENUMERATED,
    0 + TRG_OPT_DATA_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_XCBR2_Beh_q = {
    DataAttributeModelType,
    "q",
    (ModelNode*) &iedModel_CBIED_XCBR2_Beh,
    (ModelNode*) &iedModel_CBIED_XCBR2_Beh_t,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_QUALITY,
    0 + TRG_OPT_QUALITY_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_XCBR2_Beh_t = {
    DataAttributeModelType,
    "t",
    (ModelNode*) &iedModel_CBIED_XCBR2_Beh,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_TIMESTAMP,
    0,
    NULL,
    0};

DataObject iedModel_CBIED_XCBR2_Health = {
    DataObjectModelType,
    "Health",
    (ModelNode*) &iedModel_CBIED_XCBR2,
    (ModelNode*) &iedModel_CBIED_XCBR2_Pos,
    (ModelNode*) &iedModel_CBIED_XCBR2_Health_stVal,
    0,
    -1
};

DataAttribute iedModel_CBIED_XCBR2_Health_stVal = {
    DataAttributeModelType,
    "stVal",
    (ModelNode*) &iedModel_CBIED_XCBR2_Health,
    (ModelNode*) &iedModel_CBIED_XCBR2_Health_q,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_ENUMERATED,
    0 + TRG_OPT_DATA_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_XCBR2_Health_q = {
    DataAttributeModelType,
    "q",
    (ModelNode*) &iedModel_CBIED_XCBR2_Health,
    (ModelNode*) &iedModel_CBIED_XCBR2_Health_t,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_QUALITY,
    0 + TRG_OPT_QUALITY_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_XCBR2_Health_t = {
    DataAttributeModelType,
    "t",
    (ModelNode*) &iedModel_CBIED_XCBR2_Health,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_TIMESTAMP,
    0,
    NULL,
    0};

DataObject iedModel_CBIED_XCBR2_Pos = {
    DataObjectModelType,
    "Pos",
    (ModelNode*) &iedModel_CBIED_XCBR2,
    (ModelNode*) &iedModel_CBIED_XCBR2_OpCnt,
    (ModelNode*) &iedModel_CBIED_XCBR2_Pos_stVal,
    0,
    -1
};

DataAttribute iedModel_CBIED_XCBR2_Pos_stVal = {
    DataAttributeModelType,
    "stVal",
    (ModelNode*) &iedModel_CBIED_XCBR2_Pos,
    (ModelNode*) &iedModel_CBIED_XCBR2_Pos_q,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_CODEDENUM,
    0 + TRG_OPT_DATA_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_XCBR2_Pos_q = {
    DataAttributeModelType,
    "q",
    (ModelNode*) &iedModel_CBIED_XCBR2_Pos,
    (ModelNode*) &iedModel_CBIED_XCBR2_Pos_t,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_QUALITY,
    0 + TRG_OPT_QUALITY_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_XCBR2_Pos_t = {
    DataAttributeModelType,
    "t",
    (ModelNode*) &iedModel_CBIED_XCBR2_Pos,
    (ModelNode*) &iedModel_CBIED_XCBR2_Pos_Oper,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_TIMESTAMP,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_XCBR2_Pos_Oper = {
    DataAttributeModelType,
    "Oper",
    (ModelNode*) &iedModel_CBIED_XCBR2_Pos,
    (ModelNode*) &iedModel_CBIED_XCBR2_Pos_ctlModel,
    (ModelNode*) &iedModel_CBIED_XCBR2_Pos_Oper_ctlVal,
    0,
    -1,
    IEC61850_FC_CO,
    IEC61850_CONSTRUCTED,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_XCBR2_Pos_Oper_ctlVal = {
    DataAttributeModelType,
    "ctlVal",
    (ModelNode*) &iedModel_CBIED_XCBR2_Pos_Oper,
    (ModelNode*) &iedModel_CBIED_XCBR2_Pos_Oper_origin,
    NULL,
    0,
    -1,
    IEC61850_FC_CO,
    IEC61850_BOOLEAN,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_XCBR2_Pos_Oper_origin = {
    DataAttributeModelType,
    "origin",
    (ModelNode*) &iedModel_CBIED_XCBR2_Pos_Oper,
    (ModelNode*) &iedModel_CBIED_XCBR2_Pos_Oper_ctlNum,
    (ModelNode*) &iedModel_CBIED_XCBR2_Pos_Oper_origin_orCat,
    0,
    -1,
    IEC61850_FC_CO,
    IEC61850_CONSTRUCTED,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_XCBR2_Pos_Oper_origin_orCat = {
    DataAttributeModelType,
    "orCat",
    (ModelNode*) &iedModel_CBIED_XCBR2_Pos_Oper_origin,
    (ModelNode*) &iedModel_CBIED_XCBR2_Pos_Oper_origin_orIdent,
    NULL,
    0,
    -1,
    IEC61850_FC_CO,
    IEC61850_ENUMERATED,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_XCBR2_Pos_Oper_origin_orIdent = {
    DataAttributeModelType,
    "orIdent",
    (ModelNode*) &iedModel_CBIED_XCBR2_Pos_Oper_origin,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_CO,
    IEC61850_OCTET_STRING_64,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_XCBR2_Pos_Oper_ctlNum = {
    DataAttributeModelType,
    "ctlNum",
    (ModelNode*) &iedModel_CBIED_XCBR2_Pos_Oper,
    (ModelNode*) &iedModel_CBIED_XCBR2_Pos_Oper_T,
    NULL,
    0,
    -1,
    IEC61850_FC_CO,
    IEC61850_INT8U,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_XCBR2_Pos_Oper_T = {
    DataAttributeModelType,
    "T",
    (ModelNode*) &iedModel_CBIED_XCBR2_Pos_Oper,
    (ModelNode*) &iedModel_CBIED_XCBR2_Pos_Oper_Test,
    NULL,
    0,
    -1,
    IEC61850_FC_CO,
    IEC61850_TIMESTAMP,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_XCBR2_Pos_Oper_Test = {
    DataAttributeModelType,
    "Test",
    (ModelNode*) &iedModel_CBIED_XCBR2_Pos_Oper,
    (ModelNode*) &iedModel_CBIED_XCBR2_Pos_Oper_Check,
    NULL,
    0,
    -1,
    IEC61850_FC_CO,
    IEC61850_BOOLEAN,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_XCBR2_Pos_Oper_Check = {
    DataAttributeModelType,
    "Check",
    (ModelNode*) &iedModel_CBIED_XCBR2_Pos_Oper,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_CO,
    IEC61850_CHECK,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_XCBR2_Pos_ctlModel = {
    DataAttributeModelType,
    "ctlModel",
    (ModelNode*) &iedModel_CBIED_XCBR2_Pos,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_CF,
    IEC61850_ENUMERATED,
    0,
    NULL,
    0};

DataObject iedModel_CBIED_XCBR2_OpCnt = {
    DataObjectModelType,
    "OpCnt",
    (ModelNode*) &iedModel_CBIED_XCBR2,
    (ModelNode*) &iedModel_CBIED_XCBR2_CBOpCap,
    (ModelNode*) &iedModel_CBIED_XCBR2_OpCnt_stVal,
    0,
    -1
};

DataAttribute iedModel_CBIED_XCBR2_OpCnt_stVal = {
    DataAttributeModelType,
    "stVal",
    (ModelNode*) &iedModel_CBIED_XCBR2_OpCnt,
    (ModelNode*) &iedModel_CBIED_XCBR2_OpCnt_q,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_INT32,
    0 + TRG_OPT_DATA_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_XCBR2_OpCnt_q = {
    DataAttributeModelType,
    "q",
    (ModelNode*) &iedModel_CBIED_XCBR2_OpCnt,
    (ModelNode*) &iedModel_CBIED_XCBR2_OpCnt_t,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_QUALITY,
    0 + TRG_OPT_QUALITY_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_XCBR2_OpCnt_t = {
    DataAttributeModelType,
    "t",
    (ModelNode*) &iedModel_CBIED_XCBR2_OpCnt,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_TIMESTAMP,
    0,
    NULL,
    0};

DataObject iedModel_CBIED_XCBR2_CBOpCap = {
    DataObjectModelType,
    "CBOpCap",
    (ModelNode*) &iedModel_CBIED_XCBR2,
    NULL,
    (ModelNode*) &iedModel_CBIED_XCBR2_CBOpCap_stVal,
    0,
    -1
};

DataAttribute iedModel_CBIED_XCBR2_CBOpCap_stVal = {
    DataAttributeModelType,
    "stVal",
    (ModelNode*) &iedModel_CBIED_XCBR2_CBOpCap,
    (ModelNode*) &iedModel_CBIED_XCBR2_CBOpCap_q,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_INT32,
    0 + TRG_OPT_DATA_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_XCBR2_CBOpCap_q = {
    DataAttributeModelType,
    "q",
    (ModelNode*) &iedModel_CBIED_XCBR2_CBOpCap,
    (ModelNode*) &iedModel_CBIED_XCBR2_CBOpCap_t,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_QUALITY,
    0 + TRG_OPT_QUALITY_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_XCBR2_CBOpCap_t = {
    DataAttributeModelType,
    "t",
    (ModelNode*) &iedModel_CBIED_XCBR2_CBOpCap,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_TIMESTAMP,
    0,
    NULL,
    0};

LogicalNode iedModel_CBIED_XSWI1 = {
    LogicalNodeModelType,
    "XSWI1",
    (ModelNode*) &iedModel_CBIED,
    (ModelNode*) &iedModel_CBIED_MMXU1,
    (ModelNode*) &iedModel_CBIED_XSWI1_Mod,
};

DataObject iedModel_CBIED_XSWI1_Mod = {
    DataObjectModelType,
    "Mod",
    (ModelNode*) &iedModel_CBIED_XSWI1,
    (ModelNode*) &iedModel_CBIED_XSWI1_Beh,
    (ModelNode*) &iedModel_CBIED_XSWI1_Mod_stVal,
    0,
    -1
};

DataAttribute iedModel_CBIED_XSWI1_Mod_stVal = {
    DataAttributeModelType,
    "stVal",
    (ModelNode*) &iedModel_CBIED_XSWI1_Mod,
    (ModelNode*) &iedModel_CBIED_XSWI1_Mod_q,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_ENUMERATED,
    0 + TRG_OPT_DATA_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_XSWI1_Mod_q = {
    DataAttributeModelType,
    "q",
    (ModelNode*) &iedModel_CBIED_XSWI1_Mod,
    (ModelNode*) &iedModel_CBIED_XSWI1_Mod_t,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_QUALITY,
    0 + TRG_OPT_QUALITY_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_XSWI1_Mod_t = {
    DataAttributeModelType,
    "t",
    (ModelNode*) &iedModel_CBIED_XSWI1_Mod,
    (ModelNode*) &iedModel_CBIED_XSWI1_Mod_ctlModel,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_TIMESTAMP,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_XSWI1_Mod_ctlModel = {
    DataAttributeModelType,
    "ctlModel",
    (ModelNode*) &iedModel_CBIED_XSWI1_Mod,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_CF,
    IEC61850_ENUMERATED,
    0,
    NULL,
    0};

DataObject iedModel_CBIED_XSWI1_Beh = {
    DataObjectModelType,
    "Beh",
    (ModelNode*) &iedModel_CBIED_XSWI1,
    (ModelNode*) &iedModel_CBIED_XSWI1_Health,
    (ModelNode*) &iedModel_CBIED_XSWI1_Beh_stVal,
    0,
    -1
};

DataAttribute iedModel_CBIED_XSWI1_Beh_stVal = {
    DataAttributeModelType,
    "stVal",
    (ModelNode*) &iedModel_CBIED_XSWI1_Beh,
    (ModelNode*) &iedModel_CBIED_XSWI1_Beh_q,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_ENUMERATED,
    0 + TRG_OPT_DATA_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_XSWI1_Beh_q = {
    DataAttributeModelType,
    "q",
    (ModelNode*) &iedModel_CBIED_XSWI1_Beh,
    (ModelNode*) &iedModel_CBIED_XSWI1_Beh_t,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_QUALITY,
    0 + TRG_OPT_QUALITY_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_XSWI1_Beh_t = {
    DataAttributeModelType,
    "t",
    (ModelNode*) &iedModel_CBIED_XSWI1_Beh,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_TIMESTAMP,
    0,
    NULL,
    0};

DataObject iedModel_CBIED_XSWI1_Health = {
    DataObjectModelType,
    "Health",
    (ModelNode*) &iedModel_CBIED_XSWI1,
    (ModelNode*) &iedModel_CBIED_XSWI1_Pos,
    (ModelNode*) &iedModel_CBIED_XSWI1_Health_stVal,
    0,
    -1
};

DataAttribute iedModel_CBIED_XSWI1_Health_stVal = {
    DataAttributeModelType,
    "stVal",
    (ModelNode*) &iedModel_CBIED_XSWI1_Health,
    (ModelNode*) &iedModel_CBIED_XSWI1_Health_q,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_ENUMERATED,
    0 + TRG_OPT_DATA_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_XSWI1_Health_q = {
    DataAttributeModelType,
    "q",
    (ModelNode*) &iedModel_CBIED_XSWI1_Health,
    (ModelNode*) &iedModel_CBIED_XSWI1_Health_t,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_QUALITY,
    0 + TRG_OPT_QUALITY_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_XSWI1_Health_t = {
    DataAttributeModelType,
    "t",
    (ModelNode*) &iedModel_CBIED_XSWI1_Health,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_TIMESTAMP,
    0,
    NULL,
    0};

DataObject iedModel_CBIED_XSWI1_Pos = {
    DataObjectModelType,
    "Pos",
    (ModelNode*) &iedModel_CBIED_XSWI1,
    NULL,
    (ModelNode*) &iedModel_CBIED_XSWI1_Pos_stVal,
    0,
    -1
};

DataAttribute iedModel_CBIED_XSWI1_Pos_stVal = {
    DataAttributeModelType,
    "stVal",
    (ModelNode*) &iedModel_CBIED_XSWI1_Pos,
    (ModelNode*) &iedModel_CBIED_XSWI1_Pos_q,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_CODEDENUM,
    0 + TRG_OPT_DATA_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_XSWI1_Pos_q = {
    DataAttributeModelType,
    "q",
    (ModelNode*) &iedModel_CBIED_XSWI1_Pos,
    (ModelNode*) &iedModel_CBIED_XSWI1_Pos_t,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_QUALITY,
    0 + TRG_OPT_QUALITY_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_XSWI1_Pos_t = {
    DataAttributeModelType,
    "t",
    (ModelNode*) &iedModel_CBIED_XSWI1_Pos,
    (ModelNode*) &iedModel_CBIED_XSWI1_Pos_Oper,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_TIMESTAMP,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_XSWI1_Pos_Oper = {
    DataAttributeModelType,
    "Oper",
    (ModelNode*) &iedModel_CBIED_XSWI1_Pos,
    (ModelNode*) &iedModel_CBIED_XSWI1_Pos_ctlModel,
    (ModelNode*) &iedModel_CBIED_XSWI1_Pos_Oper_ctlVal,
    0,
    -1,
    IEC61850_FC_CO,
    IEC61850_CONSTRUCTED,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_XSWI1_Pos_Oper_ctlVal = {
    DataAttributeModelType,
    "ctlVal",
    (ModelNode*) &iedModel_CBIED_XSWI1_Pos_Oper,
    (ModelNode*) &iedModel_CBIED_XSWI1_Pos_Oper_origin,
    NULL,
    0,
    -1,
    IEC61850_FC_CO,
    IEC61850_BOOLEAN,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_XSWI1_Pos_Oper_origin = {
    DataAttributeModelType,
    "origin",
    (ModelNode*) &iedModel_CBIED_XSWI1_Pos_Oper,
    (ModelNode*) &iedModel_CBIED_XSWI1_Pos_Oper_ctlNum,
    (ModelNode*) &iedModel_CBIED_XSWI1_Pos_Oper_origin_orCat,
    0,
    -1,
    IEC61850_FC_CO,
    IEC61850_CONSTRUCTED,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_XSWI1_Pos_Oper_origin_orCat = {
    DataAttributeModelType,
    "orCat",
    (ModelNode*) &iedModel_CBIED_XSWI1_Pos_Oper_origin,
    (ModelNode*) &iedModel_CBIED_XSWI1_Pos_Oper_origin_orIdent,
    NULL,
    0,
    -1,
    IEC61850_FC_CO,
    IEC61850_ENUMERATED,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_XSWI1_Pos_Oper_origin_orIdent = {
    DataAttributeModelType,
    "orIdent",
    (ModelNode*) &iedModel_CBIED_XSWI1_Pos_Oper_origin,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_CO,
    IEC61850_OCTET_STRING_64,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_XSWI1_Pos_Oper_ctlNum = {
    DataAttributeModelType,
    "ctlNum",
    (ModelNode*) &iedModel_CBIED_XSWI1_Pos_Oper,
    (ModelNode*) &iedModel_CBIED_XSWI1_Pos_Oper_T,
    NULL,
    0,
    -1,
    IEC61850_FC_CO,
    IEC61850_INT8U,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_XSWI1_Pos_Oper_T = {
    DataAttributeModelType,
    "T",
    (ModelNode*) &iedModel_CBIED_XSWI1_Pos_Oper,
    (ModelNode*) &iedModel_CBIED_XSWI1_Pos_Oper_Test,
    NULL,
    0,
    -1,
    IEC61850_FC_CO,
    IEC61850_TIMESTAMP,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_XSWI1_Pos_Oper_Test = {
    DataAttributeModelType,
    "Test",
    (ModelNode*) &iedModel_CBIED_XSWI1_Pos_Oper,
    (ModelNode*) &iedModel_CBIED_XSWI1_Pos_Oper_Check,
    NULL,
    0,
    -1,
    IEC61850_FC_CO,
    IEC61850_BOOLEAN,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_XSWI1_Pos_Oper_Check = {
    DataAttributeModelType,
    "Check",
    (ModelNode*) &iedModel_CBIED_XSWI1_Pos_Oper,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_CO,
    IEC61850_CHECK,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_XSWI1_Pos_ctlModel = {
    DataAttributeModelType,
    "ctlModel",
    (ModelNode*) &iedModel_CBIED_XSWI1_Pos,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_CF,
    IEC61850_ENUMERATED,
    0,
    NULL,
    0};

LogicalNode iedModel_CBIED_MMXU1 = {
    LogicalNodeModelType,
    "MMXU1",
    (ModelNode*) &iedModel_CBIED,
    NULL,
    (ModelNode*) &iedModel_CBIED_MMXU1_Mod,
};

DataObject iedModel_CBIED_MMXU1_Mod = {
    DataObjectModelType,
    "Mod",
    (ModelNode*) &iedModel_CBIED_MMXU1,
    (ModelNode*) &iedModel_CBIED_MMXU1_Beh,
    (ModelNode*) &iedModel_CBIED_MMXU1_Mod_stVal,
    0,
    -1
};

DataAttribute iedModel_CBIED_MMXU1_Mod_stVal = {
    DataAttributeModelType,
    "stVal",
    (ModelNode*) &iedModel_CBIED_MMXU1_Mod,
    (ModelNode*) &iedModel_CBIED_MMXU1_Mod_q,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_ENUMERATED,
    0 + TRG_OPT_DATA_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_MMXU1_Mod_q = {
    DataAttributeModelType,
    "q",
    (ModelNode*) &iedModel_CBIED_MMXU1_Mod,
    (ModelNode*) &iedModel_CBIED_MMXU1_Mod_t,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_QUALITY,
    0 + TRG_OPT_QUALITY_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_MMXU1_Mod_t = {
    DataAttributeModelType,
    "t",
    (ModelNode*) &iedModel_CBIED_MMXU1_Mod,
    (ModelNode*) &iedModel_CBIED_MMXU1_Mod_ctlModel,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_TIMESTAMP,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_MMXU1_Mod_ctlModel = {
    DataAttributeModelType,
    "ctlModel",
    (ModelNode*) &iedModel_CBIED_MMXU1_Mod,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_CF,
    IEC61850_ENUMERATED,
    0,
    NULL,
    0};

DataObject iedModel_CBIED_MMXU1_Beh = {
    DataObjectModelType,
    "Beh",
    (ModelNode*) &iedModel_CBIED_MMXU1,
    (ModelNode*) &iedModel_CBIED_MMXU1_Health,
    (ModelNode*) &iedModel_CBIED_MMXU1_Beh_stVal,
    0,
    -1
};

DataAttribute iedModel_CBIED_MMXU1_Beh_stVal = {
    DataAttributeModelType,
    "stVal",
    (ModelNode*) &iedModel_CBIED_MMXU1_Beh,
    (ModelNode*) &iedModel_CBIED_MMXU1_Beh_q,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_ENUMERATED,
    0 + TRG_OPT_DATA_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_MMXU1_Beh_q = {
    DataAttributeModelType,
    "q",
    (ModelNode*) &iedModel_CBIED_MMXU1_Beh,
    (ModelNode*) &iedModel_CBIED_MMXU1_Beh_t,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_QUALITY,
    0 + TRG_OPT_QUALITY_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_MMXU1_Beh_t = {
    DataAttributeModelType,
    "t",
    (ModelNode*) &iedModel_CBIED_MMXU1_Beh,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_TIMESTAMP,
    0,
    NULL,
    0};

DataObject iedModel_CBIED_MMXU1_Health = {
    DataObjectModelType,
    "Health",
    (ModelNode*) &iedModel_CBIED_MMXU1,
    (ModelNode*) &iedModel_CBIED_MMXU1_A,
    (ModelNode*) &iedModel_CBIED_MMXU1_Health_stVal,
    0,
    -1
};

DataAttribute iedModel_CBIED_MMXU1_Health_stVal = {
    DataAttributeModelType,
    "stVal",
    (ModelNode*) &iedModel_CBIED_MMXU1_Health,
    (ModelNode*) &iedModel_CBIED_MMXU1_Health_q,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_ENUMERATED,
    0 + TRG_OPT_DATA_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_MMXU1_Health_q = {
    DataAttributeModelType,
    "q",
    (ModelNode*) &iedModel_CBIED_MMXU1_Health,
    (ModelNode*) &iedModel_CBIED_MMXU1_Health_t,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_QUALITY,
    0 + TRG_OPT_QUALITY_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_MMXU1_Health_t = {
    DataAttributeModelType,
    "t",
    (ModelNode*) &iedModel_CBIED_MMXU1_Health,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_ST,
    IEC61850_TIMESTAMP,
    0,
    NULL,
    0};

DataObject iedModel_CBIED_MMXU1_A = {
    DataObjectModelType,
    "A",
    (ModelNode*) &iedModel_CBIED_MMXU1,
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV,
    (ModelNode*) &iedModel_CBIED_MMXU1_A_phsA,
    0,
    -1
};

DataObject iedModel_CBIED_MMXU1_A_phsA = {
    DataObjectModelType,
    "phsA",
    (ModelNode*) &iedModel_CBIED_MMXU1_A,
    (ModelNode*) &iedModel_CBIED_MMXU1_A_phsB,
    (ModelNode*) &iedModel_CBIED_MMXU1_A_phsA_cVal,
    0,
    -1
};

DataAttribute iedModel_CBIED_MMXU1_A_phsA_cVal = {
    DataAttributeModelType,
    "cVal",
    (ModelNode*) &iedModel_CBIED_MMXU1_A_phsA,
    (ModelNode*) &iedModel_CBIED_MMXU1_A_phsA_q,
    (ModelNode*) &iedModel_CBIED_MMXU1_A_phsA_cVal_mag,
    0,
    -1,
    IEC61850_FC_MX,
    IEC61850_CONSTRUCTED,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_MMXU1_A_phsA_cVal_mag = {
    DataAttributeModelType,
    "mag",
    (ModelNode*) &iedModel_CBIED_MMXU1_A_phsA_cVal,
    (ModelNode*) &iedModel_CBIED_MMXU1_A_phsA_cVal_ang,
    (ModelNode*) &iedModel_CBIED_MMXU1_A_phsA_cVal_mag_f,
    0,
    -1,
    IEC61850_FC_MX,
    IEC61850_CONSTRUCTED,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_MMXU1_A_phsA_cVal_mag_f = {
    DataAttributeModelType,
    "f",
    (ModelNode*) &iedModel_CBIED_MMXU1_A_phsA_cVal_mag,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_MX,
    IEC61850_FLOAT32,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_MMXU1_A_phsA_cVal_ang = {
    DataAttributeModelType,
    "ang",
    (ModelNode*) &iedModel_CBIED_MMXU1_A_phsA_cVal,
    NULL,
    (ModelNode*) &iedModel_CBIED_MMXU1_A_phsA_cVal_ang_f,
    0,
    -1,
    IEC61850_FC_MX,
    IEC61850_CONSTRUCTED,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_MMXU1_A_phsA_cVal_ang_f = {
    DataAttributeModelType,
    "f",
    (ModelNode*) &iedModel_CBIED_MMXU1_A_phsA_cVal_ang,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_MX,
    IEC61850_FLOAT32,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_MMXU1_A_phsA_q = {
    DataAttributeModelType,
    "q",
    (ModelNode*) &iedModel_CBIED_MMXU1_A_phsA,
    (ModelNode*) &iedModel_CBIED_MMXU1_A_phsA_t,
    NULL,
    0,
    -1,
    IEC61850_FC_MX,
    IEC61850_QUALITY,
    0 + TRG_OPT_QUALITY_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_MMXU1_A_phsA_t = {
    DataAttributeModelType,
    "t",
    (ModelNode*) &iedModel_CBIED_MMXU1_A_phsA,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_MX,
    IEC61850_TIMESTAMP,
    0,
    NULL,
    0};

DataObject iedModel_CBIED_MMXU1_A_phsB = {
    DataObjectModelType,
    "phsB",
    (ModelNode*) &iedModel_CBIED_MMXU1_A,
    (ModelNode*) &iedModel_CBIED_MMXU1_A_phsC,
    (ModelNode*) &iedModel_CBIED_MMXU1_A_phsB_cVal,
    0,
    -1
};

DataAttribute iedModel_CBIED_MMXU1_A_phsB_cVal = {
    DataAttributeModelType,
    "cVal",
    (ModelNode*) &iedModel_CBIED_MMXU1_A_phsB,
    (ModelNode*) &iedModel_CBIED_MMXU1_A_phsB_q,
    (ModelNode*) &iedModel_CBIED_MMXU1_A_phsB_cVal_mag,
    0,
    -1,
    IEC61850_FC_MX,
    IEC61850_CONSTRUCTED,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_MMXU1_A_phsB_cVal_mag = {
    DataAttributeModelType,
    "mag",
    (ModelNode*) &iedModel_CBIED_MMXU1_A_phsB_cVal,
    (ModelNode*) &iedModel_CBIED_MMXU1_A_phsB_cVal_ang,
    (ModelNode*) &iedModel_CBIED_MMXU1_A_phsB_cVal_mag_f,
    0,
    -1,
    IEC61850_FC_MX,
    IEC61850_CONSTRUCTED,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_MMXU1_A_phsB_cVal_mag_f = {
    DataAttributeModelType,
    "f",
    (ModelNode*) &iedModel_CBIED_MMXU1_A_phsB_cVal_mag,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_MX,
    IEC61850_FLOAT32,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_MMXU1_A_phsB_cVal_ang = {
    DataAttributeModelType,
    "ang",
    (ModelNode*) &iedModel_CBIED_MMXU1_A_phsB_cVal,
    NULL,
    (ModelNode*) &iedModel_CBIED_MMXU1_A_phsB_cVal_ang_f,
    0,
    -1,
    IEC61850_FC_MX,
    IEC61850_CONSTRUCTED,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_MMXU1_A_phsB_cVal_ang_f = {
    DataAttributeModelType,
    "f",
    (ModelNode*) &iedModel_CBIED_MMXU1_A_phsB_cVal_ang,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_MX,
    IEC61850_FLOAT32,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_MMXU1_A_phsB_q = {
    DataAttributeModelType,
    "q",
    (ModelNode*) &iedModel_CBIED_MMXU1_A_phsB,
    (ModelNode*) &iedModel_CBIED_MMXU1_A_phsB_t,
    NULL,
    0,
    -1,
    IEC61850_FC_MX,
    IEC61850_QUALITY,
    0 + TRG_OPT_QUALITY_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_MMXU1_A_phsB_t = {
    DataAttributeModelType,
    "t",
    (ModelNode*) &iedModel_CBIED_MMXU1_A_phsB,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_MX,
    IEC61850_TIMESTAMP,
    0,
    NULL,
    0};

DataObject iedModel_CBIED_MMXU1_A_phsC = {
    DataObjectModelType,
    "phsC",
    (ModelNode*) &iedModel_CBIED_MMXU1_A,
    NULL,
    (ModelNode*) &iedModel_CBIED_MMXU1_A_phsC_cVal,
    0,
    -1
};

DataAttribute iedModel_CBIED_MMXU1_A_phsC_cVal = {
    DataAttributeModelType,
    "cVal",
    (ModelNode*) &iedModel_CBIED_MMXU1_A_phsC,
    (ModelNode*) &iedModel_CBIED_MMXU1_A_phsC_q,
    (ModelNode*) &iedModel_CBIED_MMXU1_A_phsC_cVal_mag,
    0,
    -1,
    IEC61850_FC_MX,
    IEC61850_CONSTRUCTED,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_MMXU1_A_phsC_cVal_mag = {
    DataAttributeModelType,
    "mag",
    (ModelNode*) &iedModel_CBIED_MMXU1_A_phsC_cVal,
    (ModelNode*) &iedModel_CBIED_MMXU1_A_phsC_cVal_ang,
    (ModelNode*) &iedModel_CBIED_MMXU1_A_phsC_cVal_mag_f,
    0,
    -1,
    IEC61850_FC_MX,
    IEC61850_CONSTRUCTED,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_MMXU1_A_phsC_cVal_mag_f = {
    DataAttributeModelType,
    "f",
    (ModelNode*) &iedModel_CBIED_MMXU1_A_phsC_cVal_mag,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_MX,
    IEC61850_FLOAT32,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_MMXU1_A_phsC_cVal_ang = {
    DataAttributeModelType,
    "ang",
    (ModelNode*) &iedModel_CBIED_MMXU1_A_phsC_cVal,
    NULL,
    (ModelNode*) &iedModel_CBIED_MMXU1_A_phsC_cVal_ang_f,
    0,
    -1,
    IEC61850_FC_MX,
    IEC61850_CONSTRUCTED,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_MMXU1_A_phsC_cVal_ang_f = {
    DataAttributeModelType,
    "f",
    (ModelNode*) &iedModel_CBIED_MMXU1_A_phsC_cVal_ang,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_MX,
    IEC61850_FLOAT32,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_MMXU1_A_phsC_q = {
    DataAttributeModelType,
    "q",
    (ModelNode*) &iedModel_CBIED_MMXU1_A_phsC,
    (ModelNode*) &iedModel_CBIED_MMXU1_A_phsC_t,
    NULL,
    0,
    -1,
    IEC61850_FC_MX,
    IEC61850_QUALITY,
    0 + TRG_OPT_QUALITY_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_MMXU1_A_phsC_t = {
    DataAttributeModelType,
    "t",
    (ModelNode*) &iedModel_CBIED_MMXU1_A_phsC,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_MX,
    IEC61850_TIMESTAMP,
    0,
    NULL,
    0};

DataObject iedModel_CBIED_MMXU1_PhV = {
    DataObjectModelType,
    "PhV",
    (ModelNode*) &iedModel_CBIED_MMXU1,
    (ModelNode*) &iedModel_CBIED_MMXU1_Hz,
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV_phsA,
    0,
    -1
};

DataObject iedModel_CBIED_MMXU1_PhV_phsA = {
    DataObjectModelType,
    "phsA",
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV,
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV_phsB,
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV_phsA_cVal,
    0,
    -1
};

DataAttribute iedModel_CBIED_MMXU1_PhV_phsA_cVal = {
    DataAttributeModelType,
    "cVal",
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV_phsA,
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV_phsA_q,
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV_phsA_cVal_mag,
    0,
    -1,
    IEC61850_FC_MX,
    IEC61850_CONSTRUCTED,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_MMXU1_PhV_phsA_cVal_mag = {
    DataAttributeModelType,
    "mag",
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV_phsA_cVal,
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV_phsA_cVal_ang,
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV_phsA_cVal_mag_f,
    0,
    -1,
    IEC61850_FC_MX,
    IEC61850_CONSTRUCTED,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_MMXU1_PhV_phsA_cVal_mag_f = {
    DataAttributeModelType,
    "f",
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV_phsA_cVal_mag,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_MX,
    IEC61850_FLOAT32,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_MMXU1_PhV_phsA_cVal_ang = {
    DataAttributeModelType,
    "ang",
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV_phsA_cVal,
    NULL,
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV_phsA_cVal_ang_f,
    0,
    -1,
    IEC61850_FC_MX,
    IEC61850_CONSTRUCTED,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_MMXU1_PhV_phsA_cVal_ang_f = {
    DataAttributeModelType,
    "f",
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV_phsA_cVal_ang,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_MX,
    IEC61850_FLOAT32,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_MMXU1_PhV_phsA_q = {
    DataAttributeModelType,
    "q",
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV_phsA,
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV_phsA_t,
    NULL,
    0,
    -1,
    IEC61850_FC_MX,
    IEC61850_QUALITY,
    0 + TRG_OPT_QUALITY_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_MMXU1_PhV_phsA_t = {
    DataAttributeModelType,
    "t",
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV_phsA,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_MX,
    IEC61850_TIMESTAMP,
    0,
    NULL,
    0};

DataObject iedModel_CBIED_MMXU1_PhV_phsB = {
    DataObjectModelType,
    "phsB",
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV,
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV_phsC,
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV_phsB_cVal,
    0,
    -1
};

DataAttribute iedModel_CBIED_MMXU1_PhV_phsB_cVal = {
    DataAttributeModelType,
    "cVal",
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV_phsB,
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV_phsB_q,
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV_phsB_cVal_mag,
    0,
    -1,
    IEC61850_FC_MX,
    IEC61850_CONSTRUCTED,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_MMXU1_PhV_phsB_cVal_mag = {
    DataAttributeModelType,
    "mag",
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV_phsB_cVal,
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV_phsB_cVal_ang,
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV_phsB_cVal_mag_f,
    0,
    -1,
    IEC61850_FC_MX,
    IEC61850_CONSTRUCTED,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_MMXU1_PhV_phsB_cVal_mag_f = {
    DataAttributeModelType,
    "f",
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV_phsB_cVal_mag,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_MX,
    IEC61850_FLOAT32,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_MMXU1_PhV_phsB_cVal_ang = {
    DataAttributeModelType,
    "ang",
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV_phsB_cVal,
    NULL,
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV_phsB_cVal_ang_f,
    0,
    -1,
    IEC61850_FC_MX,
    IEC61850_CONSTRUCTED,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_MMXU1_PhV_phsB_cVal_ang_f = {
    DataAttributeModelType,
    "f",
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV_phsB_cVal_ang,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_MX,
    IEC61850_FLOAT32,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_MMXU1_PhV_phsB_q = {
    DataAttributeModelType,
    "q",
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV_phsB,
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV_phsB_t,
    NULL,
    0,
    -1,
    IEC61850_FC_MX,
    IEC61850_QUALITY,
    0 + TRG_OPT_QUALITY_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_MMXU1_PhV_phsB_t = {
    DataAttributeModelType,
    "t",
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV_phsB,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_MX,
    IEC61850_TIMESTAMP,
    0,
    NULL,
    0};

DataObject iedModel_CBIED_MMXU1_PhV_phsC = {
    DataObjectModelType,
    "phsC",
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV,
    NULL,
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV_phsC_cVal,
    0,
    -1
};

DataAttribute iedModel_CBIED_MMXU1_PhV_phsC_cVal = {
    DataAttributeModelType,
    "cVal",
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV_phsC,
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV_phsC_q,
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV_phsC_cVal_mag,
    0,
    -1,
    IEC61850_FC_MX,
    IEC61850_CONSTRUCTED,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_MMXU1_PhV_phsC_cVal_mag = {
    DataAttributeModelType,
    "mag",
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV_phsC_cVal,
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV_phsC_cVal_ang,
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV_phsC_cVal_mag_f,
    0,
    -1,
    IEC61850_FC_MX,
    IEC61850_CONSTRUCTED,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_MMXU1_PhV_phsC_cVal_mag_f = {
    DataAttributeModelType,
    "f",
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV_phsC_cVal_mag,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_MX,
    IEC61850_FLOAT32,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_MMXU1_PhV_phsC_cVal_ang = {
    DataAttributeModelType,
    "ang",
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV_phsC_cVal,
    NULL,
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV_phsC_cVal_ang_f,
    0,
    -1,
    IEC61850_FC_MX,
    IEC61850_CONSTRUCTED,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_MMXU1_PhV_phsC_cVal_ang_f = {
    DataAttributeModelType,
    "f",
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV_phsC_cVal_ang,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_MX,
    IEC61850_FLOAT32,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_MMXU1_PhV_phsC_q = {
    DataAttributeModelType,
    "q",
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV_phsC,
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV_phsC_t,
    NULL,
    0,
    -1,
    IEC61850_FC_MX,
    IEC61850_QUALITY,
    0 + TRG_OPT_QUALITY_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_MMXU1_PhV_phsC_t = {
    DataAttributeModelType,
    "t",
    (ModelNode*) &iedModel_CBIED_MMXU1_PhV_phsC,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_MX,
    IEC61850_TIMESTAMP,
    0,
    NULL,
    0};

DataObject iedModel_CBIED_MMXU1_Hz = {
    DataObjectModelType,
    "Hz",
    (ModelNode*) &iedModel_CBIED_MMXU1,
    NULL,
    (ModelNode*) &iedModel_CBIED_MMXU1_Hz_mag,
    0,
    -1
};

DataAttribute iedModel_CBIED_MMXU1_Hz_mag = {
    DataAttributeModelType,
    "mag",
    (ModelNode*) &iedModel_CBIED_MMXU1_Hz,
    (ModelNode*) &iedModel_CBIED_MMXU1_Hz_q,
    (ModelNode*) &iedModel_CBIED_MMXU1_Hz_mag_f,
    0,
    -1,
    IEC61850_FC_MX,
    IEC61850_CONSTRUCTED,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_MMXU1_Hz_mag_f = {
    DataAttributeModelType,
    "f",
    (ModelNode*) &iedModel_CBIED_MMXU1_Hz_mag,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_MX,
    IEC61850_FLOAT32,
    0,
    NULL,
    0};

DataAttribute iedModel_CBIED_MMXU1_Hz_q = {
    DataAttributeModelType,
    "q",
    (ModelNode*) &iedModel_CBIED_MMXU1_Hz,
    (ModelNode*) &iedModel_CBIED_MMXU1_Hz_t,
    NULL,
    0,
    -1,
    IEC61850_FC_MX,
    IEC61850_QUALITY,
    0 + TRG_OPT_QUALITY_CHANGED,
    NULL,
    0};

DataAttribute iedModel_CBIED_MMXU1_Hz_t = {
    DataAttributeModelType,
    "t",
    (ModelNode*) &iedModel_CBIED_MMXU1_Hz,
    NULL,
    NULL,
    0,
    -1,
    IEC61850_FC_MX,
    IEC61850_TIMESTAMP,
    0,
    NULL,
    0};









IedModel iedModel = {
    "IED1",
    &iedModel_CBIED,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    initializeValues
};

static void
initializeValues()
{
}
