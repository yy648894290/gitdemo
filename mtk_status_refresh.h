#ifndef _MTK_STATUS_REFRESH_H
#define _MTK_STATUS_REFRESH_H 1

int mtkParam_setATEcho ( char* data );
int mtkParam_setIpv6Format ( char* data );
int mtkParam_updateUsbmode ( char* data );
int mtkParam_SetIms ( char* data );
int mtkParam_radioLockInit ( char* data );
int mtkParam_updateImei ( char* data );
int mtkParam_updateSN ( char* data );
int mtkParam_updateModuleModel ( char* data );
int mtkParam_updateModuleVersion ( char* data );
int mtkParam_updateSuppBands ( char* data );
int mtkParam_updateRrcState ();
int mtkParam_updateCGDCONT ( char* data );
int mtkParam_initAPNsetting ( char* data );
void mtkParam_updatePinStatus ();
void mtkParam_updatePinLockStatus ();
void mtkParam_updatePinPukCount ();
void mtkParam_updateImsi ();
void mtkParam_updateSimSPN ();
void mtkParam_updateIccid ();
void mtkParam_updateCereg ();
void mtkParam_updateC5greg ();
void mtkParam_updateCireg ();
void mtkParam_updateCesq ();
void mtkParam_updateCgatt ();
void mtkParam_updateCgact ();
int mtkParam_configCgact ( int cid_index, char* act_val );
void mtkParam_updateCops ();
void mtkParam_updateKcops ();
int mtkParam_updateIPV4V6 ( int apn_index );
void mtkParam_updatePccSccInfo ( char* data );
void mtkParam_updateServNeighInfo ( char* data );
int mtkParam_setCfunMode ( char* mode );
int mtkParam_updateDefaultGateway ( char* data );
int mtk_module_param_init ();
void mtk_normal_status_refresh ();
void mtk_install_mode_refresh ();
void mtkParam_updateUsimMncLen ();
int mtkParam_updateDualSim ();

#if defined(MTK_T750) && CONFIG_QUECTEL_T750
int mtkParam_setupDataCall ( int apn_index, int del_flag );
int mtkParam_initDataCallBk ();
void mtkParam_deInitDataCall ( int deInitType );
#endif

#endif
