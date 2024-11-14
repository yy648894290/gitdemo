#ifndef _UNISOC_STATUS_REFRESH_H
#define _UNISOC_STATUS_REFRESH_H 1

int unisocParam_setCfunMode ( char* data );
int unisocParam_updateModuleModel ( char* data );
int unisocParam_updateModuleVersion ( char* data );
int unisocParam_updateImei ( char* data );
int unisocParam_updateNat ( char* data );
int unisocParam_setVolte ( char* data );
int unisocParam_updateWanif ( char* data );
int unisocParam_setQnetdevctl ( char* data );
int unisocParam_updateDefaultGateway ( char* data );
int unisocParam_updateSuppBands ( char* data );
int unisocParam_initAPNsetting ( char* data );
int unisocParam_initNetSelectMode ( char* data );
int unisocParam_radioLockInit ( char* data );
int unisocParam_setRatPriority ( char* data );

void unisocParam_updatePinStatus ();
void unisocParam_updatePinLockStatus ();
void unisocParam_updatePinPukCount ();
void unisocParam_updateUsimMncLen ();
int unisocParam_setIpv6Format ( char* data );
void unisocParam_updateImsi ();
void unisocParam_updateSimSPN ();
void unisocParam_getSimlock ( char* reqData );
void unisocParam_updateIccid ();
void unisocParam_updatePhoneNumber ();
void unisocParam_updateClcc ();
void unisocParam_updateCesq ();
void unisocParam_updateCops ();
void unisocParam_updateServingCell ();
void unisocParam_updateMcs ();
void unisocParam_updateQcainfo ();
void unisocParam_updateSignalBar ();
void unisocParam_updateCgatt ();
void unisocParam_updateCgact ();
int unisocParam_updateQnetdevstatus ( int apn_index );
int unisocParam_updateQnetdevctl ( int apn_index, int act_val );
void unisocParam_updateCsiInfo( void );


int unisoc_module_param_init ();
void unisoc_normal_status_refresh ();
void unisoc_install_mode_refresh ();
int unisocParam_updatePlmnSearchStatus();

#endif
