#ifndef _TDTEK_STATUS_REFRESH_H
#define _TDTEK_STATUS_REFRESH_H 1

int tdtekParam_setATEcho ( char* data );
int tdtekParam_setNetnum ( char* data );
int tdtekParam_setIpv6Format ( char* data );
int tdtekParam_radioLockInit ( char* data );
int tdtekParam_updateImei ( char* data );
int tdtekParam_updateSN ( char* data );
int tdtekParam_updateModuleModel ( char* data );
int tdtekParam_updateModuleVersion ( char* data );
int tdtekParam_updateATI ( char* data );
int tdtekParam_updateSuppBands ( char* data );
int tdtekParam_updateCGDCONT ( char* data );
int tdtekParam_initAPNsetting ( char* data );
void tdtekParam_updatePinStatus ();
void tdtekParam_updatePinLockStatus ();
void tdtekParam_updatePinPukCount ();
void tdtekParam_updateImsi ();
void tdtekParam_updateSimSPN ();
void tdtekParam_updateIccid ();
void tdtekParam_updateCereg ();
void tdtekParam_updateC5greg ();
void tdtekParam_updateCireg ();
void tdtekParam_updateCsq ();
void tdtekParam_updateCgatt ();
void tdtekParam_updateCgact ();
int tdtekParam_configCgact ( int cid_index, int act_val );
void tdtekParam_updateCops ();
void tdtekParam_updateCgcontrdp ( int apn_index );
void tdtekParam_updatePccSccInfo ( char* data );
void tdtekParam_updateServNeighInfo ( char* data );
int tdtekParam_setCfunMode ( char* mode );
int tdtekParam_updateDefaultGateway ( char* data );
int tdtek_module_param_init ();
void tdtek_normal_status_refresh ();
void tdtek_install_mode_refresh ();
void tdtekParam_updateUsimMncLen ();
int tdtekParam_updateDualSim ();

#endif
