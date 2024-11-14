#ifndef _MTK_CONFIG_RESPONSE_H
#define _MTK_CONFIG_RESPONSE_H 1

void mtkParam_lockPin ( char* reqData );
void mtkParam_enterPin ( char* reqData );
void mtkParam_modifyPin ( char* reqData );
void mtkParam_enterPuk ( char* reqData );
void mtkParam_connectNetwork ( char* reqData );
void mtkParam_disconnectNetwork ( char* reqData );
void mtkParam_apnSetting ( apn_profile* const apnSetting_data );
int mtkParam_setRatPriority ( char* reqData );
void mtkParam_lockband ( char* reqData, int lock_type );
void mtkParam_lockearfcn ( char* reqData, int lock_type );
void mtkParam_setDefaultGateway ( char* reqData );
void mtkParam_lockpci ( char* reqData, int lock_type);
void mtkParam_getSimlock ( char* reqData );
void mtkParam_radioOnOffSet ( char* reqData );
void mtkParam_config_commandsetting ( char* reqData );
void mtkParam_netSelectModeSet ( char* reqData );
void mtkParam_setSearchNeighbor ( char* reqData );
void mtkParam_dualSimSet ( char* reqData );
void mtkParam_getNitzTime ( char* reqData );
int mtkParam_lteRoamingSet ( char* reqData );
void mtkParam_sendUssdCode ( char* reqData );
int mtkParam_getSupportBand ();

void mtkParam_setPinLock ( char* reqData );
void mtkParam_setSimLock ( char* reqData );
void mtkParam_setCellLock ( char* reqData );

void mtkParam_sendSmsRequest ( KT_sms_msg *send_sms );

#endif
