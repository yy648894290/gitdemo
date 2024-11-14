#ifndef _TELIT_PLS_CONFIG_RESPONSE_H
#define _TELIT_PLS_CONFIG_RESPONSE_H 1

void telitPlsParam_radioOnOffSet ( char* reqData );
void telitPlsParam_getSimlock ( char* reqData );
void telitPlsParam_apnSetting ( apn_profile* const apnSetting_data );
void telitPlsParam_config_commandsetting ( char* reqData );
void telitPlsParam_lockband ( char* reqData, int lock_type );
int telit_pls_band_strlist_to_hexstr(char* band_str_list, char* band_hexstr_list, int band_hexstr_list_size);
void telitPlsParam_lockearfcn ( char* reqData );
void telitPlsParam_lockpci ( char* reqData );
void telitPlsParam_connectNetwork ( char* reqData );
void telitPlsParam_disconnectNetwork ( char* reqData );
void telitPlsParam_netSelectModeSet ( char* reqData );
void telitPlsParam_setDefaultGateway ( char* reqData );
void telitPlsParam_dualSimSet ( char* reqData );
void telitPlsParam_lockPin ( char* reqData );
void telitPlsParam_enterPin ( char* reqData );
void telitPlsParam_modifyPin ( char* reqData );
void telitPlsParam_enterPuk ( char* reqData );
void telitPlsParam_getNitzTime ( char* reqData );
void telitPlsParam_setSearchNeighbor (void);

#endif
