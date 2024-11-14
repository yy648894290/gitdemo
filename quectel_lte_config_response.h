#ifndef _QUECTEL_LTE_CONFIG_RESPONSE_H
#define _QUECTEL_LTE_CONFIG_RESPONSE_H 1

void quectel_lte_Param_lockPin ( char* reqData );
void quectel_lte_Param_enterPin ( char* reqData );
void quectel_lte_Param_modifyPin ( char* reqData );
void quectel_lte_Param_enterPuk ( char* reqData );
void quectel_lte_Param_config_commandsetting ( char* reqData );
void quectel_lte_Param_radioOnOffSet ( char* reqData );
void quectel_lte_Param_netSelectModeSet ( char* reqData );
int quectel_lte_Param_lteRoamingSet ( char* reqData );
void quectel_lte_Param_getSimlock ( char* reqData );
void quectel_lte_Param_connectNetwork ( char* reqData );
void quectel_lte_Param_disconnectNetwork ( char* reqData );
void quectel_lte_Param_apnSetting ( apn_profile* const apnSetting_data );
void quectel_lte_Param_lockband ( char* reqData, int lock_type );
void quectel_lte_Param_lockearfcn ( char* reqData, int lock_type );
void quectel_lte_Param_lockpci ( char* reqData, int lock_type );
void quectel_lte_Param_setDefaultGateway ( char* reqData );
void quectel_lte_Param_setSearchNeighbor ( char* reqData );
void quectel_lte_Param_getNitzTime ( char* reqData );
void quectel_lte_Param_dualSimSet ( char* reqData );
#endif