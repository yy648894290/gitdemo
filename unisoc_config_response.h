#ifndef _UNISOC_CONFIG_RESPONSE_H
#define _UNISOC_CONFIG_RESPONSE_H 1

void unisocParam_lockPin ( char* reqData );
void unisocParam_enterPin ( char* reqData );
void unisocParam_modifyPin ( char* reqData );
void unisocParam_enterPuk ( char* reqData );
void unisocParam_netSelectModeSet ( char* reqData );
void unisocParam_connectNetwork ( char* reqData );
void unisocParam_disconnectNetwork ( char* reqData );
void unisocParam_radioOnOffSet ( char* reqData );
void unisocParam_getSimlock ( char* reqData );
void unisocParam_apnSetting ( apn_profile* const apnSetting_data );
void unisocParam_config_commandsetting ( char* reqData );
void unisocParam_lockband ( char* reqData, int lock_type );
void unisocParam_lockearfcn ( char* reqData, int lock_type );
void unisocParam_lockpci ( char* reqData, int lock_type );
void unisocParam_setDefaultGateway ( char* reqData );
int unisocParam_setRatPriority ( char* reqData );
void unisocParam_setSearchNeighbor ( char* reqData );
void unisocParam_getNitzTime ( char* reqData );
int unisocParam_lteRoamingSet ( char* data );
void unisocParam_dualSimSet ( char* reqData );
void unisocParam_sendSmsRequest ( KT_sms_msg *send_sms );
void unisocParam_sendUssdCode( char* reqData );

#endif
