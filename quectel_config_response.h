#ifndef _QUECTEL_CONFIG_RESPONSE_H
#define _QUECTEL_CONFIG_RESPONSE_H 1

void quectelParam_lockPin ( char* reqData );
void quectelParam_enterPin ( char* reqData );
void quectelParam_modifyPin ( char* reqData );
void quectelParam_enterPuk ( char* reqData );
void quectelParam_connectNetwork ( char* reqData );
void quectelParam_disconnectNetwork ( char* reqData );
void quectelParam_apnSetting ( apn_profile* const apnSetting_data );
int quectelParam_setRatPriority ( char* reqData );
void quectelParam_lockband ( char* reqData, int lock_type );
void quectelParam_lockearfcn ( char* reqData );
void quectelParam_setDefaultGateway ( char* reqData );
void quectelParam_lockpci ( char* reqData );
void quectelParam_getSimlock ( char* reqData );
void quectelParam_radioOnOffSet ( char* reqData );
void quectelParam_config_commandsetting ( char* reqData );
void quectelParam_netSelectModeSet ( char* reqData );
void quectelParam_setSearchNeighbor ( char* reqData );
void quectelParam_dualSimSet ( char* reqData );
void quectelParam_getNitzTime ( char* reqData );
void quectelParam_setVolteAkaRequest ( struct KT_char_akaRequest *volte_aka_req );
int quectelParam_setServicePSCSMode ( char* reqData );
int quectelParam_lteRoamingSet ( char* reqData );
void quectelparsing_txpowlmt ( char* data );
void quectelParam_lteTxPowerLimitSet ( char* reqData );
int check_quectel_pcilock( char *data, char *earfcn );

#endif
