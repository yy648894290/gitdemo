#ifndef _TELIT_PLS_STATUS_REFRESH_H
#define _TELIT_PLS_STATUS_REFRESH_H 1

//Telit PLS CAT 1 not support lock earfcn & pci
#define TELIT_PLS_LOCK_EARFCN_MAX "1,0,0"
#define TELIT_PLS_LOCK_PCI_MAX "1,0,0"
#define TELIT_PLS_MAX_APN_NUM 1
#define TELIT_PLS_4G_BANDLIST_B1_B32_HEX "a0e18df"
#define TELIT_PLS_4G_BANDLIST_B33_B66_HEX "2000001a0"

int telit_pls_module_param_init();
int telitPlsParam_setATEcho ( char* data );
int telitPlsParam_updateModuleVersion ( char* data );
int telitPlsParam_updateSN( char* data );
int telitPlsParam_updateDefaultGateway( char* data );
int telitPlsParam_updateSuppBands ( char* data );
int telitPlsParam_initAPNsetting( char* data );
int telitPlsParam_initNetSelectMode( char* data );
int telitPlsParam_radioLockInit( char* data );
void telit_pls_normal_status_refresh(void);
int telitPlsParam_updatePinStatus(void);
int telitPlsParam_updatePinLockStatus(void);
int telitPlsParam_updatePinPukCount(void);
void telitPlsParam_updateImsi (void);
void telitPlsParam_updateSimSPN (void);
void telitPlsParam_updateUsimMncLen (void);
void telitPlsParam_updateIccid (void);
void telitPlsParam_updatePhoneNumber (void);
void telitPlsParam_updateCesq(void);
void telitPlsParam_updateCops(void);
void telitPlsParam_updateCgatt (void);
void telitPlsParam_updateCgact (void);
int telitPlsParam_updatePDPreturnFormat ( void );
int telitPlsParam_updateCgcontrdp ( int apn_index );
int telitPlsParam_updateDualSim (void);
void telitPlsParam_updateNeighbourCell(void);

#endif
