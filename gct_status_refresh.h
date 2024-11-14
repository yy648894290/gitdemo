#ifndef _GCT_STATUS_REFRESH_H
#define _GCT_STATUS_REFRESH_H 1

int gctParam_enableRRC ( char* data );
int gctParam_updateImei ( char* data );
int gctParam_updateSuppBands ( char* data );
int gctParam_updateModuleModel ( char* data );
int gctParam_updateModuleType ( char* data );
int gctParam_updateModuleVersion ( char* data );
int gctParam_updateFwVersion ( char* data );
int gctParam_updateDefaultGateway ( char* data );
void gctParam_updatePinStatus ();
void gctParam_updatePinLockStatus ();
void gctParam_updateImsi ();
void gctParam_updateSimSPN ();
void gctParam_updateIccid ();
void gctParam_updatePinPukCount ();
void gctParam_updatePhoneNumber ();
void gctParam_updateSmsCenterNum ();
void gctParam_updateUsimMncLen ();
void gctParam_updateCesq ();
void gctParam_updateRrcState ();
void gctParam_updateCereg ();
void gctParam_updateCgatt ();
void gctParam_updateCgact ();
void gctParam_updateKtlteinfo ();
void gctParam_updateGdmitem ();
void gctParam_updateGlteconnstatus ();
void gctParam_updateGdmmodtc ();
void gctParam_updateGbler ();
void gctParam_updateGharq ();
void gctParam_updateDefbrdp ( int cid_index );
void gctParam_updateGdmcqi ();
void gctParam_updateGrankindex ();
int gctParam_updateCgcontrdp ( int cid_index );
void gctParam_updateCmglFour ();
void gctParam_updateWanTraffic ();

int gctParam_configCgact ( int cid_index, int act_val );

int gct_module_param_init();
void gct_normal_status_refresh();
void gct_install_mode_refresh();

#endif
