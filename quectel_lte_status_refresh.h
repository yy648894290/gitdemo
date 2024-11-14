#ifndef _QUECTEL_LTE_STATUS_REFRESH_H
#define _QUECTEL_LTE_STATUS_REFRESH_H 1

int quectel_lte_Param_setATEcho ( char* data );
int quectel_lte_Param_setCfunMode ( char* mode );
int quectel_lte_Param_updateModuleVersion ( char* data );
int quectel_lte_Param_updateUsbmode ( char* data );
int quectel_lte_Param_setQuimslot ( char* data );
int quectel_lte_Param_setVlan ( char* data );
int quectel_lte_Param_setVolte ( char* data );
int quectel_lte_Param_updateImei ( char* data );
int quectel_lte_Param_updateSN ( char* data );
int quectel_lte_Param_updateDefaultGateway ( char* data );
int quectel_lte_Param_updateSuppBands ( char* data );
int quectel_lte_Param_initAPNsetting ( char* data );
int quectel_lte_Param_initNetSelectMode ( char* data );
int quectel_lte_Param_radioLockInit ( char* data );
int quectel_lte_Param_setRatPriority ( char* data );
void quectel_lte_Param_updatePinStatus ();
void quectel_lte_Param_updatePinLockStatus ();
void quectel_lte_Param_updatePinPukCount ();
int quectel_lte_Param_setIpv6Format ( char* data );
void quectel_lte_Param_updateImsi ();
void quectel_lte_Param_updateIccid ();
void quectel_lte_Param_updatePhoneNumber ();
void quectel_lte_Param_updateUsimMncLen ();
void quectel_lte_Param_updateSimSPN ();
void quectel_lte_Param_updateClcc ();
void quectel_lte_Param_updateCsq ();
void quectel_lte_Param_updateCreg ();
void quectel_lte_Param_updateCgatt ();
void quectel_lte_Param_updateCops ();
void quectel_lte_Param_updateRrcState ();
void quectel_lte_Param_updateServingCell ( char* data );
void quectel_lte_Param_updateQcainfo ();
void quectel_lte_Param_updateQnetdevctl ();
void quectel_lte_Param_updateCgact ();
int quectel_lte_Param_updateCgcontrdp ( int apn_index );
int quectel_lte_Param_configQmapConnect ( int cid_index, int act_val );
int quectel_lte_module_param_init ();
void quectel_lte_normal_status_refresh();
void quectel_lte_install_mode_refresh ();
int quectel_lte_Param_updatePlmnSearchStatus(void);

#endif
