#ifndef _TELIT_STATUS_REFRESH_H
#define _TELIT_STATUS_REFRESH_H 1

//Telit is only allowed to locked 1 earfcn or 1 pci
#define TELIT_LOCK_EARFCN_MAX "1,1,1"
#define TELIT_LOCK_PCI_MAX "1,1,1"

int telit_module_param_init();
int telitParam_setATEcho ( char* data );
int telitParam_updateModuleVersion ( char* data );
int telitParam_updateSN( char* data );
int telitParam_updateDefaultGateway( char* data );
int telitParam_updateSuppBands ( char* data );
int telitParam_initAPNsetting( char* data );
int telitParam_initNetSelectMode( char* data );
int telitParam_radioLockInit( char* data );
void telit_normal_status_refresh(void);
int telitParam_updatePinStatus(void);
int telitParam_updatePinLockStatus(void);
int telitParam_updatePinPukCount(void);
void telitParam_updateImsi (void);
void telitParam_updateSimSPN (void);
void telitParam_updateUsimMncLen (void);
void telitParam_updateIccid (void);
void telitParam_updatePhoneNumber (void);
void telitParam_updateCesq(void);
void telitParam_updateCops(void);
void telitParam_updateCgatt (void);
void telitParam_updateCereg (void);
void telitParam_updateC5greg (void);
void telitParam_updateCgact (void);
int telitParam_configCgact ( int cid_index, int act_val );
int telitParam_updatePDPreturnFormat ( void );
int telitParam_updateCgcontrdp ( int apn_index );
int telitParam_updateDualSim (void);
void teliParam_updateNeighbourCell(void);

#endif
