#ifndef _GCT_CONFIG_RESPONSE_H
#define _GCT_CONFIG_RESPONSE_H 1

void gctParam_lteEventNotify ( char* reqData );

void gctParam_lockPin ( char* reqData );
void gctParam_enterPin ( char* reqData );
void gctParam_modifyPin ( char* reqData );
void gctParam_enterPuk ( char* reqData );
void gctParam_config_commandsetting ( char* reqData );
void gctParam_netSelectModeSet ( char* reqData );
void gctParam_connectNetwork ( char* reqData );
void gctParam_disconnectNetwork ( char* reqData );
void gctParam_apnSetting ( apn_profile* const apnSetting_data );
void gctParam_lockband ( char* reqData );
void gctParam_lockearfcn ( char* reqData );
void gctParam_setDefaultGateway ( char* reqData );
void gctParam_setSearchNeighbor ( char* reqData );
void gctParam_lockpci ( char* reqData );
void gctParam_getSimlock ( char* reqData );
void gctParam_getNitzTime ( char* reqData );
void gctParam_setVolteAkaRequest ( struct KT_char_akaRequest *volte_aka_req );
void gctParam_sendSmsRequest ( KT_sms_msg *send_sms );
void gctParam_delSmsRequest ( KT_sms_msg *del_sms );
void gctParam_readSmsRequest ( KT_sms_msg *read_sms );
void gctParam_setSmsCenterNum ( char* reqData );
void gctParam_resendSmsRequest ( KT_sms_msg *resend_sms );
void gctParam_manualPlmnSearchReq ( char* reqData );
void gctParam_plmnSearchManualSelect ( char* reqData );
void gctParam_radioOnOffSet ( char* reqData );
void gctParam_lteTxPowerLimitSet ( char* reqData );
void gctParam_lteRoamingSet ( char* reqData );

#endif
