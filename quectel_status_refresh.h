#ifndef _QUECTEL_STATUS_REFRESH_H
#define _QUECTEL_STATUS_REFRESH_H 1

int quectelParam_setATEcho ( char* data );
int quectelParam_setIpv6Format ( char* data );
int quectelParam_updateUsbmode ( char* data );
int quectelParam_radioLockInit ( char* data );
int quectelParam_updateImei ( char* data );
int quectelParam_updateSN ( char* data );
int quectelParam_updateModuleModel ( char* data );
int quectelParam_updateModuleVersion ( char* data );
int quectelParam_updateSuppBands ( char* data );
int quectelParam_updateCGDCONT ( char* data );
void quectelParam_updateQICSGP ( char* data );
int quectelParam_initAPNsetting ( char* data );
void quectelParam_updatePinStatus ();
void quectelParam_updatePinLockStatus ();
void quectelParam_updatePinPukCount ();
void quectelParam_updateImsi ();
void quectelParam_updateIccid ();
void quectelParam_updatePhoneNumber ();
void quectelParam_updateCreg ();
void quectelParam_updateC5greg ();
void quectelParam_updateCsq ();
void quectelParam_updateCgatt ();
void quectelParam_updateEthRgmii ();
int quectelParam_configEthRgmii ( int cid_index, int act_val );
void quectelParam_updateCgact ();
int quectelParam_configCgact ( int cid_index, int act_val );
void quectelParam_updateCops ();
int quectelParam_updateCgcontrdp ( int apn_index );
void quectelParam_updateRrcState ();
void quectelParam_updateServingCell ( char* data );
void quectelParam_updateQcainfo ();
int quectelParam_setCfunMode ( char* mode );
int quectelParam_routerModeInit ( char* data );
int quectelParam_updateDefaultGateway ( char* data );
int quectel_module_param_init ();
void quectel_normal_status_refresh ();
void quectel_install_mode_refresh ();
void quectelParam_updateUsimMncLen ();
int quectelParam_updateDualSim ();
int quectelParam_GpsInit( char* data );
int quectelParam_initNetSelectMode ( char* data );
int quectelParam_initCsiCtrl( void );
void quectelParam_updateCsiInfo ( void );
void quectelParam_updateSimSPN ();
int quectelParam_radioLockCheck( char *earfcn );

#if defined ( CONFIG_SW_QUECTEL_X62 )
int quectelParam_setVlanMac ( char* data );
int quectelParam_setVolteIms ( char* data );
int quectelParam_setQCPDPIMSCFGE ( char* data );
int quectelParam_configQmapConnect ( int cid_index, int act_val );
#endif

#if defined(_QCA_X55_) && defined(RG500)
int quectelParam_initQmapwac ( char* data );
int quectelParam_setupDataCall ( int apn_index );
int quectelParam_initPhyDriver ( char* data );
#endif

int quectelParam_updatePlmnSearchStatus(void);
int quectelParam_initNR5Gtxpower ( char* data );
int quectelParam_updateNR5Gtxpower ( char* data );

#endif
