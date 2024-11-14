#ifndef _TDTEK_CONFIG_RESPONSE_H
#define _TDTEK_CONFIG_RESPONSE_H 1

void tdtekParam_lockPin ( char* reqData );
void tdtekParam_enterPin ( char* reqData );
void tdtekParam_modifyPin ( char* reqData );
void tdtekParam_enterPuk ( char* reqData );
void tdtekParam_connectNetwork ( char* reqData );
void tdtekParam_disconnectNetwork ( char* reqData );
void tdtekParam_apnSetting ( apn_profile* const apnSetting_data );
int tdtekParam_setRatPriority ( char* reqData );
void tdtekParam_lockband ( char* reqData, int lock_type );
void tdtekParam_lockearfcn ( char* reqData, int lock_type );
void tdtekParam_setDefaultGateway ( char* reqData );
void tdtekParam_lockpci ( char* reqData, int lock_type );
void tdtekParam_getSimlock ( char* reqData );
void tdtekParam_radioOnOffSet ( char* reqData );
void tdtekParam_config_commandsetting ( char* reqData );
void tdtekParam_netSelectModeSet ( char* reqData );
void tdtekParam_setSearchNeighbor ( char* reqData );
void tdtekParam_dualSimSet ( char* reqData );
void tdtekParam_getNitzTime ( char* reqData );
int tdtekParam_lteRoamingSet ( char* reqData );
void tdtekParam_sendUssdCode ( char* reqData );

void tdtekParam_setVolteAkaRequest ( struct KT_char_akaRequest *volte_aka_req );

void tdtekParam_sendSmsRequest ( KT_sms_msg *send_sms );
void tdtekParam_delSmsRequest ( KT_sms_msg *del_sms );
void tdtekParam_readSmsRequest ( KT_sms_msg *read_sms );
void tdtekParam_setSmsCenterNum ( char* reqData );
void tdtekParam_resendSmsRequest ( KT_sms_msg *resend_sms );

void tdtekParam_manualPlmnSearchReq ( char* reqData );
void tdtekParam_plmnSearchManualSelect ( char* reqData );
void tdtekParam_lteTxPowerLimitSet ( char* reqData );
void tdtekParam_lteEventNotify ( char* reqData );

#endif
