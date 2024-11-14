#ifndef _TELIT_CONFIG_RESPONSE_H
#define _TELIT_CONFIG_RESPONSE_H 1

void telitParam_radioOnOffSet ( char* reqData );
int telit_bandlist_hexstr_to_strlist(char* band_4g_list_hexstr, char* band_5g_list_hexstr,
                                                char* band_4g_list_str, int size_band_4g,
                                                char* band_5g_list_str, int size_band_5g);
void telitParam_getSimlock ( char* reqData );
void telitParam_apnSetting ( apn_profile* const apnSetting_data );
void telitParam_config_commandsetting ( char* reqData );
void telitParam_lockband ( char* reqData, int lock_type );
int telit_band_strlist_to_hexstr(char* band_str_list, char* band_hexstr_list, int band_hexstr_list_size);
void telitParam_lockearfcn ( char* reqData );
void telitParam_lockpci ( char* reqData );
void telitParam_connectNetwork ( char* reqData );
void telitParam_disconnectNetwork ( char* reqData );
void telitParam_netSelectModeSet ( char* reqData );
void telitParam_setDefaultGateway ( char* reqData );
void telitParam_dualSimSet ( char* reqData );
void telitParam_lockPin ( char* reqData );
void telitParam_enterPin ( char* reqData );
void telitParam_modifyPin ( char* reqData );
void telitParam_enterPuk ( char* reqData );
void telitParam_getNitzTime ( char* reqData );

#endif
