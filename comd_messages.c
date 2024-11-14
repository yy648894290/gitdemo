#include "comd_share.h"
#include "gct_config_response.h"
#include "comd_messages.h"
#include "mtk_config_response.h"
#include "tdtek_config_response.h"
#include "quectel_config_response.h"
#include "unisoc_config_response.h"
#include "quectel_lte_config_response.h"
#include "telit_config_response.h"
#include "telit_pls_config_response.h"

extern COMD_DEV_INFO at_dev;

static int requestLockPin ( ST_MESSAGE* msg )
{
    char *pinLockStr = NULL;

    pinLockStr = GET_MESSAGE_DATA_POINTER ( msg );

    switch ( at_dev.comd_mVendor )
    {
        case M_VENDOR_GCT:
            gctParam_lockPin ( pinLockStr );
            break;
        case M_VENDOR_FIBOCOM_X55:
            break;
        case M_VENDOR_QUECTEL_X55:
            quectelParam_lockPin ( pinLockStr );
            break;
        case M_VENDOR_QUECTEL_T750:
            mtkParam_lockPin ( pinLockStr );
            break;
        case M_VENDOR_ZTE_X55:
            break;
        case M_VENDOR_SQNS:

            break;
        case M_VENDOR_TD_MT5710:
            tdtekParam_lockPin ( pinLockStr );
            break;
        case M_VENDOR_QUECTEL_UNISOC:
            unisocParam_lockPin ( pinLockStr );
            break;
        case M_VENDOR_QUECTEL_LTE:
            quectel_lte_Param_lockPin ( pinLockStr );
            break;
        case M_VENDOR_TELIT_4G:
        case M_VENDOR_TELIT_5G:
            telitParam_lockPin ( pinLockStr );
            break;
        case M_VENDOR_TELIT_PLS:
            telitPlsParam_lockPin ( pinLockStr );
            break;
    }

    return 0;
}

static int requestEnterPin ( ST_MESSAGE* msg )
{
    char *pinEnterStr = NULL;

    pinEnterStr = GET_MESSAGE_DATA_POINTER ( msg );

    switch ( at_dev.comd_mVendor )
    {
        case M_VENDOR_GCT:
            gctParam_enterPin ( pinEnterStr );
            break;
        case M_VENDOR_FIBOCOM_X55:
            break;
        case M_VENDOR_QUECTEL_X55:
            quectelParam_enterPin ( pinEnterStr );
            break;
        case M_VENDOR_QUECTEL_T750:
            mtkParam_enterPin ( pinEnterStr );
            break;
        case M_VENDOR_ZTE_X55:
            break;
        case M_VENDOR_SQNS:

            break;
        case M_VENDOR_TD_MT5710:
            tdtekParam_enterPin ( pinEnterStr );
            break;
        case M_VENDOR_QUECTEL_UNISOC:
            unisocParam_enterPin ( pinEnterStr );
            break;
        case M_VENDOR_QUECTEL_LTE:
            quectel_lte_Param_enterPin ( pinEnterStr );
            break;
        case M_VENDOR_TELIT_4G:
        case M_VENDOR_TELIT_5G:
            telitParam_enterPin ( pinEnterStr );
            break;
        case M_VENDOR_TELIT_PLS:
            telitPlsParam_enterPin ( pinEnterStr );
            break;

    }

    return 0;
}

static int requestModifyPin ( ST_MESSAGE* msg )
{
    char *pinModStr = NULL;

    pinModStr = GET_MESSAGE_DATA_POINTER ( msg );

    switch ( at_dev.comd_mVendor )
    {
        case M_VENDOR_GCT:
            gctParam_modifyPin ( pinModStr );
            break;
        case M_VENDOR_FIBOCOM_X55:
            break;
        case M_VENDOR_QUECTEL_X55:
            quectelParam_modifyPin ( pinModStr );
            break;
        case M_VENDOR_QUECTEL_T750:
            mtkParam_modifyPin ( pinModStr );
            break;
        case M_VENDOR_ZTE_X55:
            break;
        case M_VENDOR_SQNS:

            break;
        case M_VENDOR_TD_MT5710:
            tdtekParam_modifyPin ( pinModStr );
            break;
        case M_VENDOR_QUECTEL_UNISOC:
            unisocParam_modifyPin ( pinModStr );
            break;
        case M_VENDOR_QUECTEL_LTE:
            quectel_lte_Param_modifyPin ( pinModStr );
            break;
        case M_VENDOR_TELIT_4G:
        case M_VENDOR_TELIT_5G:
            telitParam_modifyPin ( pinModStr );
            break;
        case M_VENDOR_TELIT_PLS:
            telitPlsParam_modifyPin ( pinModStr );
            break;
    }

    return 0;
}

static int requestEnterPuk ( ST_MESSAGE* msg )
{
    char *pukEnterStr = NULL;

    pukEnterStr = GET_MESSAGE_DATA_POINTER ( msg );

    switch ( at_dev.comd_mVendor )
    {
        case M_VENDOR_GCT:
            gctParam_enterPuk ( pukEnterStr );
            break;
        case M_VENDOR_FIBOCOM_X55:
            break;
        case M_VENDOR_QUECTEL_X55:
            quectelParam_enterPuk ( pukEnterStr );
            break;
        case M_VENDOR_QUECTEL_T750:
            mtkParam_enterPuk ( pukEnterStr );
            break;
        case M_VENDOR_ZTE_X55:
            break;
        case M_VENDOR_SQNS:

            break;
        case M_VENDOR_TD_MT5710:
            tdtekParam_enterPuk ( pukEnterStr );
            break;
        case M_VENDOR_QUECTEL_UNISOC:
            unisocParam_enterPuk ( pukEnterStr );
            break;
        case M_VENDOR_QUECTEL_LTE:
            quectel_lte_Param_enterPuk ( pukEnterStr );
            break;
        case M_VENDOR_TELIT_4G:
        case M_VENDOR_TELIT_5G:
            telitParam_enterPuk ( pukEnterStr );
            break;
        case M_VENDOR_TELIT_PLS:
            telitPlsParam_enterPuk ( pukEnterStr );
            break;
    }

    return 0;
}

static int requestNetSelectMode ( ST_MESSAGE* msg )
{
    char *netSelectMode = NULL;

    netSelectMode = GET_MESSAGE_DATA_POINTER ( msg );

    switch ( at_dev.comd_mVendor )
    {
        case M_VENDOR_GCT:
            gctParam_netSelectModeSet ( netSelectMode );
            break;
        case M_VENDOR_FIBOCOM_X55:
            break;
        case M_VENDOR_QUECTEL_X55:
            quectelParam_netSelectModeSet ( netSelectMode );
            break;
        case M_VENDOR_QUECTEL_T750:
            mtkParam_netSelectModeSet ( netSelectMode );
            break;
        case M_VENDOR_ZTE_X55:
            break;
        case M_VENDOR_SQNS:

            break;
        case M_VENDOR_TD_MT5710:
            tdtekParam_netSelectModeSet ( netSelectMode );
            break;
        case M_VENDOR_QUECTEL_UNISOC:
            unisocParam_netSelectModeSet ( netSelectMode );
            break;
        case M_VENDOR_QUECTEL_LTE:
            quectel_lte_Param_netSelectModeSet ( netSelectMode );
            break;
        case M_VENDOR_TELIT_4G:
        case M_VENDOR_TELIT_5G:
            telitParam_netSelectModeSet ( netSelectMode );
            break;
        case M_VENDOR_TELIT_PLS:
            telitPlsParam_netSelectModeSet ( netSelectMode );
            break;
    }

    return 0;
}

static int requestConnectNetwork ( ST_MESSAGE* msg )
{
    switch ( at_dev.comd_mVendor )
    {
        case M_VENDOR_GCT:
            gctParam_connectNetwork ( NULL );
            break;
        case M_VENDOR_FIBOCOM_X55:
            break;
        case M_VENDOR_QUECTEL_X55:
            quectelParam_connectNetwork ( NULL );
            break;
        case M_VENDOR_QUECTEL_T750:
            mtkParam_connectNetwork ( NULL );
            break;
        case M_VENDOR_ZTE_X55:
            break;
        case M_VENDOR_SQNS:

            break;
        case M_VENDOR_TD_MT5710:
            tdtekParam_connectNetwork ( NULL );
            break;
        case M_VENDOR_QUECTEL_UNISOC:
            unisocParam_connectNetwork ( "0" );
            break;
        case M_VENDOR_QUECTEL_LTE:
            quectel_lte_Param_connectNetwork ( NULL );
            break;
        case M_VENDOR_TELIT_4G:
        case M_VENDOR_TELIT_5G:
            telitParam_connectNetwork ( NULL );
            break;
        case M_VENDOR_TELIT_PLS:
            telitPlsParam_connectNetwork ( NULL );
            break;
    }

    return 0;
}

static int requestDisConnectNetwork ( ST_MESSAGE* msg )
{
    switch ( at_dev.comd_mVendor )
    {
        case M_VENDOR_GCT:
            gctParam_disconnectNetwork ( NULL );
            break;
        case M_VENDOR_FIBOCOM_X55:
            break;
        case M_VENDOR_QUECTEL_X55:
            quectelParam_disconnectNetwork ( NULL );
            break;
        case M_VENDOR_QUECTEL_T750:
            mtkParam_disconnectNetwork ( NULL );
            break;
        case M_VENDOR_ZTE_X55:
            break;
        case M_VENDOR_SQNS:

            break;
        case M_VENDOR_TD_MT5710:
            tdtekParam_disconnectNetwork ( NULL );
            break;
        case M_VENDOR_QUECTEL_UNISOC:
            unisocParam_disconnectNetwork ( NULL );
            break;
        case M_VENDOR_QUECTEL_LTE:
            quectel_lte_Param_disconnectNetwork ( NULL );
            break;
        case M_VENDOR_TELIT_4G:
        case M_VENDOR_TELIT_5G:
            telitParam_disconnectNetwork ( NULL );
            break;
        case M_VENDOR_TELIT_PLS:
            telitPlsParam_disconnectNetwork ( NULL );
            break;

    }

    return 0;
}

static int requestApnSetting ( ST_MESSAGE* msg )
{
    apn_profile* apnSetting_data = NULL;

    apnSetting_data = GET_MESSAGE_DATA_POINTER ( msg );

    switch ( at_dev.comd_mVendor )
    {
        case M_VENDOR_GCT:
            gctParam_apnSetting ( apnSetting_data );
            break;
        case M_VENDOR_FIBOCOM_X55:
            break;
        case M_VENDOR_QUECTEL_X55:
            quectelParam_apnSetting( apnSetting_data );
            break;
        case M_VENDOR_QUECTEL_T750:
            mtkParam_apnSetting ( apnSetting_data );
            break;
        case M_VENDOR_ZTE_X55:
            break;
        case M_VENDOR_SQNS:

            break;
        case M_VENDOR_TD_MT5710:
            tdtekParam_apnSetting ( apnSetting_data );
            break;
        case M_VENDOR_QUECTEL_UNISOC:
            unisocParam_apnSetting ( apnSetting_data );
            break;
        case M_VENDOR_QUECTEL_LTE:
            quectel_lte_Param_apnSetting( apnSetting_data );
            break;
        case M_VENDOR_TELIT_4G:
        case M_VENDOR_TELIT_5G:
            telitParam_apnSetting ( apnSetting_data );
        case M_VENDOR_TELIT_PLS:
            telitPlsParam_apnSetting ( apnSetting_data );
            break;

    }

    return 0;
}

static int requestATCommandSet ( ST_MESSAGE* msg )
{
    char *ATCommandStr = NULL;

    ATCommandStr = GET_MESSAGE_DATA_POINTER ( msg );

    switch ( at_dev.comd_mVendor )
    {
        case M_VENDOR_GCT:
            gctParam_config_commandsetting ( ATCommandStr );
            break;
        case M_VENDOR_FIBOCOM_X55:
            break;
        case M_VENDOR_QUECTEL_X55:
            quectelParam_config_commandsetting ( ATCommandStr );
            break;
        case M_VENDOR_QUECTEL_T750:
            mtkParam_config_commandsetting ( ATCommandStr );
            break;
        case M_VENDOR_ZTE_X55:
            break;
        case M_VENDOR_SQNS:

            break;
        case M_VENDOR_TD_MT5710:
            tdtekParam_config_commandsetting ( ATCommandStr );
            break;
        case M_VENDOR_QUECTEL_UNISOC:
            unisocParam_config_commandsetting ( ATCommandStr );
            break;
        case M_VENDOR_QUECTEL_LTE:
            quectel_lte_Param_config_commandsetting ( ATCommandStr );
            break;
        case M_VENDOR_TELIT_4G:
        case M_VENDOR_TELIT_5G:
            telitParam_config_commandsetting ( ATCommandStr );
            break;
        case M_VENDOR_TELIT_PLS:
            telitPlsParam_config_commandsetting ( ATCommandStr );
            break;

    }

    return 0;
}
static int requestLogSteamEnableSet ( ST_MESSAGE* msg )
{
    switch ( at_dev.comd_mVendor )
    {
        case M_VENDOR_GCT:
            break;
        case M_VENDOR_FIBOCOM_X55:
            break;
        case M_VENDOR_QUECTEL_X55:
            break;
        case M_VENDOR_QUECTEL_T750:
            break;
        case M_VENDOR_ZTE_X55:
            break;
        case M_VENDOR_SQNS:

            break;
        case M_VENDOR_TD_MT5710:
            break;
    }

    return 0;
}

static int requestLteSAEnableSet ( ST_MESSAGE* msg )
{
    switch ( at_dev.comd_mVendor )
    {
        case M_VENDOR_GCT:
            break;
        case M_VENDOR_FIBOCOM_X55:
            break;
        case M_VENDOR_QUECTEL_X55:
            break;
        case M_VENDOR_QUECTEL_T750:
            break;
        case M_VENDOR_ZTE_X55:
            break;
        case M_VENDOR_SQNS:

            break;
        case M_VENDOR_TD_MT5710:
            break;
    }

    return 0;
}
static int requestLockBand ( ST_MESSAGE* msg )
{
    char *bandLockStr = NULL;

    bandLockStr = GET_MESSAGE_DATA_POINTER ( msg );

    switch ( at_dev.comd_mVendor )
    {
        case M_VENDOR_GCT:
            gctParam_lockband ( bandLockStr );
            break;
        case M_VENDOR_FIBOCOM_X55:
            break;
        case M_VENDOR_QUECTEL_X55:
            quectelParam_lockband ( bandLockStr, 1 );
            break;
        case M_VENDOR_QUECTEL_T750:
            mtkParam_lockband ( bandLockStr, 1 );
            break;
        case M_VENDOR_ZTE_X55:
            break;
        case M_VENDOR_SQNS:

            break;
        case M_VENDOR_TD_MT5710:
            tdtekParam_lockband ( bandLockStr, 1 );
            break;
        case M_VENDOR_QUECTEL_UNISOC:
            unisocParam_lockband ( bandLockStr, 1 );
            break;
        case M_VENDOR_QUECTEL_LTE:
            quectel_lte_Param_lockband ( bandLockStr, 1 );
            break;
        case M_VENDOR_TELIT_4G:
        case M_VENDOR_TELIT_5G:
            telitParam_lockband ( bandLockStr, 1 );
            break;
        case M_VENDOR_TELIT_PLS:
            telitPlsParam_lockband ( bandLockStr, 1 );
            break;

    }

    return 0;
}

static int requestLockEarfcn ( ST_MESSAGE* msg )
{
    char *earfcnLockStr = NULL;

    earfcnLockStr = GET_MESSAGE_DATA_POINTER ( msg );

    switch ( at_dev.comd_mVendor )
    {
        case M_VENDOR_GCT:
            gctParam_lockearfcn ( earfcnLockStr );
            break;
        case M_VENDOR_FIBOCOM_X55:
            break;
        case M_VENDOR_QUECTEL_X55:
            quectelParam_lockearfcn ( earfcnLockStr );
            break;
        case M_VENDOR_QUECTEL_T750:
            mtkParam_lockearfcn ( earfcnLockStr, 1 );
            break;
        case M_VENDOR_ZTE_X55:
            break;
        case M_VENDOR_SQNS:

            break;
        case M_VENDOR_TD_MT5710:
            tdtekParam_lockearfcn ( earfcnLockStr, 1 );
            break;
        case M_VENDOR_QUECTEL_UNISOC:
            unisocParam_lockearfcn ( earfcnLockStr, 1 );
            break;
        case M_VENDOR_QUECTEL_LTE:
            quectel_lte_Param_lockearfcn ( earfcnLockStr, 1 );
            break;
        case M_VENDOR_TELIT_4G:
        case M_VENDOR_TELIT_5G:
            telitParam_lockearfcn ( earfcnLockStr );
            break;
    }

    return 0;
}

static int requestSetDefaultGateway ( ST_MESSAGE* msg )
{
    char *default_gw = NULL;

    default_gw = GET_MESSAGE_DATA_POINTER ( msg );

    switch ( at_dev.comd_mVendor )
    {
        case M_VENDOR_GCT:
            gctParam_setDefaultGateway ( default_gw );
            break;
        case M_VENDOR_FIBOCOM_X55:
            break;
        case M_VENDOR_QUECTEL_X55:
            quectelParam_setDefaultGateway( default_gw );
            break;
        case M_VENDOR_QUECTEL_T750:
            mtkParam_setDefaultGateway ( default_gw );
            break;
        case M_VENDOR_ZTE_X55:
            break;
        case M_VENDOR_SQNS:

            break;
        case M_VENDOR_TD_MT5710:
            tdtekParam_setDefaultGateway ( default_gw );
            break;
        case M_VENDOR_QUECTEL_UNISOC:
            unisocParam_setDefaultGateway ( default_gw );
            break;
        case M_VENDOR_QUECTEL_LTE:
            quectel_lte_Param_setDefaultGateway( default_gw );
            break;
        case M_VENDOR_TELIT_4G:
        case M_VENDOR_TELIT_5G:
            telitParam_setDefaultGateway ( default_gw );
            break;
        case M_VENDOR_TELIT_PLS:
            telitPlsParam_setDefaultGateway ( default_gw );
            break;
    }

    return 0;
}

static int requestSearchNeighborCells ( ST_MESSAGE* msg )
{
    switch ( at_dev.comd_mVendor )
    {
        case M_VENDOR_GCT:
            gctParam_setSearchNeighbor ( NULL );
            break;
        case M_VENDOR_FIBOCOM_X55:
            break;
        case M_VENDOR_QUECTEL_X55:
            quectelParam_setSearchNeighbor ( NULL );
            break;
        case M_VENDOR_QUECTEL_T750:
            mtkParam_setSearchNeighbor ( NULL );
            break;
        case M_VENDOR_ZTE_X55:
            break;
        case M_VENDOR_SQNS:

            break;
        case M_VENDOR_TD_MT5710:
            tdtekParam_setSearchNeighbor ( NULL );
            break;
        case M_VENDOR_QUECTEL_UNISOC:
            unisocParam_setSearchNeighbor ( NULL );
            break;
        case M_VENDOR_QUECTEL_LTE:
            quectel_lte_Param_setSearchNeighbor ( NULL );
            break;
        case M_VENDOR_TELIT_4G:
        case M_VENDOR_TELIT_5G:
            break;
        case M_VENDOR_TELIT_PLS:
            telitPlsParam_setSearchNeighbor ( );
            break;
    }

    return 0;
}

static int requestLockPci ( ST_MESSAGE* msg )
{
    char *pciLockStr = NULL;

    pciLockStr = GET_MESSAGE_DATA_POINTER ( msg );

    switch ( at_dev.comd_mVendor )
    {
        case M_VENDOR_GCT:
            gctParam_lockpci ( pciLockStr );
            break;
        case M_VENDOR_FIBOCOM_X55:
            break;
        case M_VENDOR_QUECTEL_X55:
            quectelParam_lockpci ( pciLockStr );
            break;
        case M_VENDOR_QUECTEL_T750:
            mtkParam_lockpci ( pciLockStr, 1 );
            break;
        case M_VENDOR_ZTE_X55:
            break;
        case M_VENDOR_SQNS:

            break;
        case M_VENDOR_TD_MT5710:
            tdtekParam_lockpci ( pciLockStr, 1 );
            break;
        case M_VENDOR_QUECTEL_UNISOC:
            unisocParam_lockpci ( pciLockStr, 1 );
            break;
        case M_VENDOR_QUECTEL_LTE:
            quectel_lte_Param_lockpci ( pciLockStr, 1 );
            break;
        case M_VENDOR_TELIT_4G:
        case M_VENDOR_TELIT_5G:
            telitParam_lockpci ( pciLockStr );
            break;
    }

    return 0;
}

static int requestSetSimLock ( ST_MESSAGE* msg )
{
    char *simLockStr = NULL;

    simLockStr = GET_MESSAGE_DATA_POINTER ( msg );

    switch ( at_dev.comd_mVendor )
    {
        case M_VENDOR_GCT:
            gctParam_getSimlock ( simLockStr );
            break;
        case M_VENDOR_FIBOCOM_X55:
            break;
        case M_VENDOR_QUECTEL_X55:
            quectelParam_getSimlock( simLockStr );
            break;
        case M_VENDOR_QUECTEL_T750:
            mtkParam_getSimlock ( simLockStr );
            break;
        case M_VENDOR_ZTE_X55:
            break;
        case M_VENDOR_SQNS:

            break;
        case M_VENDOR_TD_MT5710:
            tdtekParam_getSimlock ( simLockStr );
            break;
        case M_VENDOR_QUECTEL_UNISOC:
            unisocParam_getSimlock ( simLockStr );
            break;
        case M_VENDOR_QUECTEL_LTE:
            quectel_lte_Param_getSimlock( simLockStr );
            break;
        case M_VENDOR_TELIT_4G:
        case M_VENDOR_TELIT_5G:
            telitParam_getSimlock ( simLockStr );
            break;
        case M_VENDOR_TELIT_PLS:
            telitPlsParam_getSimlock ( simLockStr );
            break;
    }

    return 0;
}

static int requestSetPlmnLock ( ST_MESSAGE* msg )
{
    switch ( at_dev.comd_mVendor )
    {
        case M_VENDOR_GCT:
            break;
        case M_VENDOR_FIBOCOM_X55:
            break;
        case M_VENDOR_QUECTEL_X55:
            break;
        case M_VENDOR_QUECTEL_T750:
            break;
        case M_VENDOR_ZTE_X55:
            break;
        case M_VENDOR_SQNS:
            break;
        case M_VENDOR_TD_MT5710:
            break;
    }

    return 0;
}

static int requestGetNitzTime ( ST_MESSAGE* msg )
{
    switch ( at_dev.comd_mVendor )
    {
        case M_VENDOR_GCT:
            gctParam_getNitzTime ( NULL );
            break;
        case M_VENDOR_FIBOCOM_X55:
            break;
        case M_VENDOR_QUECTEL_X55:
            quectelParam_getNitzTime ( NULL );
            break;
        case M_VENDOR_QUECTEL_T750:
            mtkParam_getNitzTime ( NULL );
            break;
        case M_VENDOR_ZTE_X55:
            break;
        case M_VENDOR_SQNS:
            break;
        case M_VENDOR_TD_MT5710:
            tdtekParam_getNitzTime ( NULL );
            break;
        case M_VENDOR_QUECTEL_UNISOC:
            unisocParam_getNitzTime ( NULL );
            break;
        case M_VENDOR_QUECTEL_LTE:
            quectel_lte_Param_getNitzTime ( NULL );
            break;
        case M_VENDOR_TELIT_4G:
        case M_VENDOR_TELIT_5G:
            telitParam_getNitzTime ( NULL );
            break;
        case M_VENDOR_TELIT_PLS:
            telitPlsParam_getNitzTime ( NULL );
            break;
    }

    return 0;
}

static int requestSetVolteAka ( ST_MESSAGE* msg )
{
    struct KT_char_akaRequest *volte_aka_req;

    volte_aka_req = GET_MESSAGE_DATA_POINTER ( msg );

    switch ( at_dev.comd_mVendor )
    {
        case M_VENDOR_GCT:
            gctParam_setVolteAkaRequest ( volte_aka_req );
            break;
        case M_VENDOR_FIBOCOM_X55:

            break;
        case M_VENDOR_QUECTEL_X55:
            break;
        case M_VENDOR_QUECTEL_T750:
            break;
        case M_VENDOR_ZTE_X55:
            break;
        case M_VENDOR_SQNS:

            break;
        case M_VENDOR_TD_MT5710:
            tdtekParam_setVolteAkaRequest ( volte_aka_req );
            break;
    }

    return 0;
}

static int requestSendSms ( ST_MESSAGE* msg )
{
    KT_sms_msg *send_sms;

    send_sms = GET_MESSAGE_DATA_POINTER ( msg );

    switch ( at_dev.comd_mVendor )
    {
        case M_VENDOR_GCT:
            gctParam_sendSmsRequest ( send_sms );
            break;
        case M_VENDOR_FIBOCOM_X55:
            break;
        case M_VENDOR_QUECTEL_X55:
            break;
        case M_VENDOR_QUECTEL_T750:
            mtkParam_sendSmsRequest ( send_sms );
            break;
        case M_VENDOR_ZTE_X55:
            break;
        case M_VENDOR_SQNS:
            break;
        case M_VENDOR_TD_MT5710:
            tdtekParam_sendSmsRequest ( send_sms );
            break;
        case M_VENDOR_QUECTEL_UNISOC:
            unisocParam_sendSmsRequest ( send_sms );
            break;
    }

    return 0;
}

static int requestDeleteSms ( ST_MESSAGE* msg )
{
    KT_sms_msg *del_sms;

    del_sms = GET_MESSAGE_DATA_POINTER ( msg );

    switch ( at_dev.comd_mVendor )
    {
        case M_VENDOR_GCT:
            gctParam_delSmsRequest ( del_sms );
            break;
        case M_VENDOR_FIBOCOM_X55:
            break;
        case M_VENDOR_QUECTEL_X55:
            break;
        case M_VENDOR_QUECTEL_T750:
            break;
        case M_VENDOR_ZTE_X55:
            break;
        case M_VENDOR_SQNS:

            break;
        case M_VENDOR_TD_MT5710:
            tdtekParam_delSmsRequest ( del_sms );
            break;
    }

    return 0;
}

static int requestReadSms ( ST_MESSAGE* msg )
{
    KT_sms_msg *read_sms;

    read_sms = GET_MESSAGE_DATA_POINTER ( msg );

    switch ( at_dev.comd_mVendor )
    {
        case M_VENDOR_GCT:
            gctParam_readSmsRequest ( read_sms );
            break;
        case M_VENDOR_FIBOCOM_X55:
            break;
        case M_VENDOR_QUECTEL_X55:
            break;
        case M_VENDOR_QUECTEL_T750:
            break;
        case M_VENDOR_ZTE_X55:
            break;
        case M_VENDOR_SQNS:

            break;
        case M_VENDOR_TD_MT5710:
            tdtekParam_readSmsRequest ( read_sms );
            break;
    }

    return 0;
}

static int requestSetSmsCenterNumber ( ST_MESSAGE* msg )
{
    char *smsCenterNum = NULL;

    smsCenterNum = GET_MESSAGE_DATA_POINTER ( msg );

    switch ( at_dev.comd_mVendor )
    {
        case M_VENDOR_GCT:
            gctParam_setSmsCenterNum ( smsCenterNum );
            break;
        case M_VENDOR_FIBOCOM_X55:
            break;
        case M_VENDOR_QUECTEL_X55:
            break;
        case M_VENDOR_QUECTEL_T750:
            break;
        case M_VENDOR_ZTE_X55:
            break;
        case M_VENDOR_SQNS:

            break;
        case M_VENDOR_TD_MT5710:
            tdtekParam_setSmsCenterNum ( smsCenterNum );
            break;
    }

    return 0;
}

static int requestResendSms ( ST_MESSAGE* msg )
{
    KT_sms_msg *resend_sms;

    resend_sms = GET_MESSAGE_DATA_POINTER ( msg );

    switch ( at_dev.comd_mVendor )
    {
        case M_VENDOR_GCT:
            gctParam_resendSmsRequest ( resend_sms );
            break;
        case M_VENDOR_FIBOCOM_X55:
            break;
        case M_VENDOR_QUECTEL_X55:
            break;
        case M_VENDOR_QUECTEL_T750:
            break;
        case M_VENDOR_ZTE_X55:
            break;
        case M_VENDOR_SQNS:

            break;
        case M_VENDOR_TD_MT5710:
            tdtekParam_resendSmsRequest ( resend_sms );
            break;
    }

    return 0;
}

static int eventManualPlmnSearch ( ST_MESSAGE* msg )
{
    switch ( at_dev.comd_mVendor )
    {
        case M_VENDOR_GCT:
            gctParam_manualPlmnSearchReq ( NULL );
            break;
        case M_VENDOR_FIBOCOM_X55:
            break;
        case M_VENDOR_QUECTEL_X55:
            break;
        case M_VENDOR_QUECTEL_T750:
            break;
        case M_VENDOR_ZTE_X55:
            break;
        case M_VENDOR_SQNS:

            break;
        case M_VENDOR_TD_MT5710:
            tdtekParam_manualPlmnSearchReq ( NULL );
            break;
    }

    return 0;
}

static int eventManualPlmnConnect ( ST_MESSAGE* msg )
{
    char *operPlmn = NULL;

    operPlmn = GET_MESSAGE_DATA_POINTER ( msg );

    switch ( at_dev.comd_mVendor )
    {
        case M_VENDOR_GCT:
            gctParam_plmnSearchManualSelect ( operPlmn );
            break;
        case M_VENDOR_FIBOCOM_X55:
            break;
        case M_VENDOR_QUECTEL_X55:
            break;
        case M_VENDOR_QUECTEL_T750:
            break;
        case M_VENDOR_ZTE_X55:
            break;
        case M_VENDOR_SQNS:

            break;
        case M_VENDOR_TD_MT5710:
            tdtekParam_plmnSearchManualSelect ( operPlmn );
            break;
    }

    return 0;
}

static int requestRadioOnOff ( ST_MESSAGE* msg )
{
    char *cfunStr = NULL;

    cfunStr = GET_MESSAGE_DATA_POINTER ( msg );

    switch ( at_dev.comd_mVendor )
    {
        case M_VENDOR_GCT:
            gctParam_radioOnOffSet ( cfunStr );
            break;
        case M_VENDOR_FIBOCOM_X55:
            break;
        case M_VENDOR_QUECTEL_X55:
            quectelParam_radioOnOffSet( cfunStr );
            break;
        case M_VENDOR_QUECTEL_T750:
            mtkParam_radioOnOffSet ( cfunStr );
            break;
        case M_VENDOR_ZTE_X55:
            break;
        case M_VENDOR_SQNS:

            break;
        case M_VENDOR_TD_MT5710:
            tdtekParam_radioOnOffSet ( cfunStr );
            break;
        case M_VENDOR_QUECTEL_UNISOC:
            unisocParam_radioOnOffSet ( cfunStr );
            break;
        case M_VENDOR_QUECTEL_LTE:
            quectel_lte_Param_radioOnOffSet( cfunStr );
            break;
        case M_VENDOR_TELIT_4G:
        case M_VENDOR_TELIT_5G:
            telitParam_radioOnOffSet ( cfunStr );
            break;
        case M_VENDOR_TELIT_PLS:
            telitPlsParam_radioOnOffSet ( cfunStr );
            break;
    }

    return 0;
}

static int requestTxpowerLimitSet ( ST_MESSAGE* msg )
{
    char *txPowLimit = NULL;

    txPowLimit = GET_MESSAGE_DATA_POINTER ( msg );

    switch ( at_dev.comd_mVendor )
    {
        case M_VENDOR_GCT:
            gctParam_lteTxPowerLimitSet ( txPowLimit );
            break;
        case M_VENDOR_FIBOCOM_X55:
            break;
        case M_VENDOR_QUECTEL_X55:
            quectelParam_lteTxPowerLimitSet ( txPowLimit );
            break;
        case M_VENDOR_QUECTEL_T750:
            break;
        case M_VENDOR_ZTE_X55:
            break;
        case M_VENDOR_SQNS:

            break;
        case M_VENDOR_TD_MT5710:
            tdtekParam_lteTxPowerLimitSet ( txPowLimit );
            break;
    }

    return 0;
}

static int requestImsOnOff ( ST_MESSAGE* msg )
{
    switch ( at_dev.comd_mVendor )
    {
        case M_VENDOR_GCT:
            break;
        case M_VENDOR_FIBOCOM_X55:
            break;
        case M_VENDOR_QUECTEL_X55:
            break;
        case M_VENDOR_QUECTEL_T750:
            break;
        case M_VENDOR_ZTE_X55:
            break;
        case M_VENDOR_SQNS:

            break;
        case M_VENDOR_TD_MT5710:
            break;
    }

    return 0;
}

static int requestRoamingSet ( ST_MESSAGE* msg )
{
    char *roamStr = NULL;

    roamStr = GET_MESSAGE_DATA_POINTER ( msg );

    switch ( at_dev.comd_mVendor )
    {
        case M_VENDOR_GCT:
            gctParam_lteRoamingSet ( roamStr );
            break;
        case M_VENDOR_FIBOCOM_X55:
            break;
        case M_VENDOR_QUECTEL_X55:
            quectelParam_lteRoamingSet ( roamStr );
            break;
        case M_VENDOR_QUECTEL_T750:
            mtkParam_lteRoamingSet ( roamStr );
            break;
        case M_VENDOR_ZTE_X55:
            break;
        case M_VENDOR_SQNS:

            break;
        case M_VENDOR_TD_MT5710:
            tdtekParam_lteRoamingSet ( roamStr );
            break;
        case M_VENDOR_QUECTEL_UNISOC:
            unisocParam_lteRoamingSet ( roamStr );
            break;
        case M_VENDOR_QUECTEL_LTE:
            quectel_lte_Param_lteRoamingSet ( roamStr );
            break;
    }

    return 0;
}

static int eventNotifyComd ( ST_MESSAGE* msg )
{
    char *eventStr = NULL;

    eventStr = GET_MESSAGE_DATA_POINTER ( msg );

    switch ( at_dev.comd_mVendor )
    {
        case M_VENDOR_GCT:
            gctParam_lteEventNotify ( eventStr );
            break;
        case M_VENDOR_FIBOCOM_X55:
            break;
        case M_VENDOR_QUECTEL_X55:
            break;
        case M_VENDOR_QUECTEL_T750:
            break;
        case M_VENDOR_ZTE_X55:
            break;
        case M_VENDOR_SQNS:
            break;
        case M_VENDOR_TD_MT5710:
            tdtekParam_lteEventNotify ( eventStr );
            break;
    }

    return 0;
}

static int requestDualsimSet ( ST_MESSAGE* msg )
{
    char *simslot_str = NULL;

    simslot_str = GET_MESSAGE_DATA_POINTER ( msg );

    switch ( at_dev.comd_mVendor )
    {
        case M_VENDOR_GCT:
            break;
        case M_VENDOR_FIBOCOM_X55:
            break;
        case M_VENDOR_QUECTEL_X55:
            quectelParam_dualSimSet ( simslot_str );
            break;
        case M_VENDOR_QUECTEL_T750:
            mtkParam_dualSimSet ( simslot_str );
            break;
        case M_VENDOR_ZTE_X55:
            break;
        case M_VENDOR_SQNS:
            break;
        case M_VENDOR_TD_MT5710:
            tdtekParam_dualSimSet ( simslot_str );
            break;
        case M_VENDOR_QUECTEL_UNISOC:
            unisocParam_dualSimSet ( simslot_str );
            break;
        case M_VENDOR_QUECTEL_LTE:
            quectel_lte_Param_dualSimSet ( simslot_str );
            break;
        case M_VENDOR_TELIT_4G:
        case M_VENDOR_TELIT_5G:
            telitParam_dualSimSet ( simslot_str );
            break;

    }

    return 0;
}

static int requestMconfBackUp ( ST_MESSAGE* msg )
{
    switch ( at_dev.comd_mVendor )
    {
        case M_VENDOR_GCT:
            break;
        case M_VENDOR_FIBOCOM_X55:
            break;
        case M_VENDOR_QUECTEL_X55:
            break;
        case M_VENDOR_QUECTEL_T750:
            break;
        case M_VENDOR_ZTE_X55:
            break;
        case M_VENDOR_SQNS:
            break;
        case M_VENDOR_TD_MT5710:
            break;
    }

    return 0;
}

static int requestSetSTCPinLock ( ST_MESSAGE* msg )
{
    char *setPinLock = NULL;

    setPinLock = GET_MESSAGE_DATA_POINTER ( msg );

    switch ( at_dev.comd_mVendor )
    {
        case M_VENDOR_GCT:
            break;
        case M_VENDOR_FIBOCOM_X55:
            break;
        case M_VENDOR_QUECTEL_X55:
            break;
        case M_VENDOR_QUECTEL_T750:
            mtkParam_setPinLock ( setPinLock );
            break;
        case M_VENDOR_SQNS:
            break;
        case M_VENDOR_TD_MT5710:
            break;
    }

    return 0;
}

static int requestSetSTCSimLock ( ST_MESSAGE* msg )
{
    char *setSimLock = NULL;

    setSimLock = GET_MESSAGE_DATA_POINTER ( msg );

    switch ( at_dev.comd_mVendor )
    {
        case M_VENDOR_GCT:
            break;
        case M_VENDOR_FIBOCOM_X55:
            break;
        case M_VENDOR_QUECTEL_X55:
            break;
        case M_VENDOR_QUECTEL_T750:
            mtkParam_setSimLock ( setSimLock );
            break;
        case M_VENDOR_SQNS:
            break;
        case M_VENDOR_TD_MT5710:
            break;
    }

    return 0;
}

static int requestSetSTCCellLock ( ST_MESSAGE* msg )
{
    char *setCellLock = NULL;

    setCellLock = GET_MESSAGE_DATA_POINTER ( msg );

    switch ( at_dev.comd_mVendor )
    {
        case M_VENDOR_GCT:
            break;
        case M_VENDOR_FIBOCOM_X55:
            break;
        case M_VENDOR_QUECTEL_X55:
            break;
        case M_VENDOR_QUECTEL_T750:
            mtkParam_setCellLock ( setCellLock );
            break;
        case M_VENDOR_SQNS:
            break;
        case M_VENDOR_TD_MT5710:
            break;
    }

    return 0;
}

static int requestSetModemLog ( ST_MESSAGE* msg )
{
    char *setModemLog = NULL;

    setModemLog = GET_MESSAGE_DATA_POINTER ( msg );

    switch ( at_dev.comd_mVendor )
    {
        case M_VENDOR_GCT:
            break;
        case M_VENDOR_FIBOCOM_X55:
            break;
        case M_VENDOR_QUECTEL_X55:
            break;
        case M_VENDOR_QUECTEL_T750:
            break;
        case M_VENDOR_SQNS:
            break;
        case M_VENDOR_TD_MT5710:
            break;
        case M_VENDOR_QUECTEL_UNISOC:
            unisocParam_setModemLog ( setModemLog );
            break;
    }

    return 0;
}

static int requestUssdCodeSend ( ST_MESSAGE* msg )
{
    char *ussd_code = NULL;

    ussd_code = GET_MESSAGE_DATA_POINTER ( msg );

    switch ( at_dev.comd_mVendor )
    {
        case M_VENDOR_GCT:
            break;
        case M_VENDOR_FIBOCOM_X55:
            break;
        case M_VENDOR_QUECTEL_X55:
            break;
        case M_VENDOR_QUECTEL_T750:
            mtkParam_sendUssdCode ( ussd_code );
            break;
        case M_VENDOR_SQNS:
            break;
        case M_VENDOR_TD_MT5710:
            tdtekParam_sendUssdCode ( ussd_code );
            break;
        case M_VENDOR_QUECTEL_UNISOC:
            unisocParam_sendUssdCode ( ussd_code );
            break;
    }

    return 0;
}

ST_MESSAGE_PROCCESS comdMessageProccess[] =
{
    PROCCESS_METHOD ( "EventNotifyComd", MSG_TYPE_EVENT, MSG_EVT_NOTIFY_COMD, eventNotifyComd ),
    PROCCESS_METHOD ( "ReqDualSim", MSG_TYPE_EVENT, MSG_EVT_DUALSIM_SET, requestDualsimSet ),
    PROCCESS_METHOD ( "ReqMconfBackUp", MSG_TYPE_EVENT, MSG_EVT_LANIP_BACKUP, requestMconfBackUp ),

    PROCCESS_METHOD ( "ReqLockPIN", MSG_TYPE_EVENT, MSG_EVT_LOCK_PIN, requestLockPin ),
    PROCCESS_METHOD ( "ReqEnterPIN", MSG_TYPE_EVENT, MSG_EVT_ENTER_PIN, requestEnterPin ),
    PROCCESS_METHOD ( "ReqModifyPIN", MSG_TYPE_EVENT, MSG_EVT_MODIFY_PIN, requestModifyPin ),
    PROCCESS_METHOD ( "ReqEnterPUK", MSG_TYPE_EVENT, MSG_EVT_ENTER_PUK, requestEnterPuk ),
    PROCCESS_METHOD ( "ReqNetSelectMode", MSG_TYPE_EVENT, MSG_EVT_NET_SELECT_MODE_SET, requestNetSelectMode ),
    PROCCESS_METHOD ( "EvtConnectNetwork", MSG_TYPE_EVENT, MSG_EVT_CONNECT_NETWORK, requestConnectNetwork ),
    PROCCESS_METHOD ( "ReqDisConnectNetwork", MSG_TYPE_EVENT, MSG_EVT_DISCONNECT_NETWORK, requestDisConnectNetwork ),
    PROCCESS_METHOD ( "ReqApnSetting", MSG_TYPE_EVENT, MSG_EVT_APN_SET, requestApnSetting ),
    PROCCESS_METHOD ( "ReqLockBand", MSG_TYPE_EVENT, MSG_EVT_LOCKBAND_SET, requestLockBand ),
    PROCCESS_METHOD ( "ReqLockEarfcn", MSG_TYPE_EVENT, MSG_EVT_LOCKEARFCN_SET, requestLockEarfcn ),
    PROCCESS_METHOD ( "ReqSetDefaultGateway", MSG_TYPE_EVENT, MSG_EVT_DEFAULT_GATEWAY_SET, requestSetDefaultGateway ),
    PROCCESS_METHOD ( "ReqSearchNeighborCells", MSG_TYPE_EVENT, MSG_EVT_SEARCH_NEIGHBOR_SET, requestSearchNeighborCells ),
    PROCCESS_METHOD ( "ReqLockPci", MSG_TYPE_EVENT, MSG_EVT_LOCKPCI_SET, requestLockPci ),
    PROCCESS_METHOD ( "ReqSetSimLock", MSG_TYPE_EVENT, MSG_EVT_SIMLOCK_SET, requestSetSimLock ),
    PROCCESS_METHOD ( "ReqSetPlmnLock", MSG_TYPE_EVENT, MSG_EVT_PLMNLOCK_SET, requestSetPlmnLock ),
    PROCCESS_METHOD ( "ReqGetNitzTime", MSG_TYPE_EVENT, MSG_EVT_GET_NITZ_TIME, requestGetNitzTime ),
    PROCCESS_METHOD ( "ReqSetVolteAka", MSG_TYPE_EVENT, MSG_EVT_SET_VOLTE_AKA_REQ, requestSetVolteAka ),
    PROCCESS_METHOD ( "ReqSendSms", MSG_TYPE_EVENT, MSG_EVT_SEND_SMS_REQ, requestSendSms ),
    PROCCESS_METHOD ( "ReqDeleteSms", MSG_TYPE_EVENT, MSG_EVT_DEL_SMS_REQ, requestDeleteSms ),
    PROCCESS_METHOD ( "ReqReadSms", MSG_TYPE_EVENT, MSG_EVT_READ_SMS_REQ, requestReadSms ),
    PROCCESS_METHOD ( "ReqSetSmsCenterNumber", MSG_TYPE_EVENT, MSG_EVT_SET_SMS_C_NUM, requestSetSmsCenterNumber ),
    PROCCESS_METHOD ( "ReqResendSms", MSG_TYPE_EVENT, MSG_EVT_RESEND_SMS_REQ, requestResendSms ),
    PROCCESS_METHOD ( "EvtManualPlmnSearch", MSG_TYPE_EVENT, MSG_EVT_PLMN_SEARCH_REQ, eventManualPlmnSearch ),
    PROCCESS_METHOD ( "EvtManualPlmnConnect", MSG_TYPE_EVENT, MSG_EVT_PLMN_SEARCH_NET_SET, eventManualPlmnConnect ),
    PROCCESS_METHOD ( "ReqRadioOnOff", MSG_TYPE_EVENT, MSG_EVT_LTE_ON_OFF_SET, requestRadioOnOff ),
    PROCCESS_METHOD ( "ReqTxpowerLimitSet", MSG_TYPE_EVENT, MSG_EVT_TXPOWER_LIMIT_SET, requestTxpowerLimitSet ),
    PROCCESS_METHOD ( "ReqImsOnOff", MSG_TYPE_EVENT, MSG_EVT_IMS_ON_OFF_SET, requestImsOnOff ),
    PROCCESS_METHOD ( "ReqRoaminSet", MSG_TYPE_EVENT, MSG_EVT_ROAMING_SET, requestRoamingSet ),
    PROCCESS_METHOD ( "ReqLogSteamEnableSet", MSG_TYPE_EVENT, MSG_EVT_LOG_STEAM_ENABLE_SET, requestLogSteamEnableSet ),
    PROCCESS_METHOD ( "ReqLteSAEnableSet", MSG_TYPE_EVENT, MSG_EVT_LTESAENABLE_SET, requestLteSAEnableSet ),
    PROCCESS_METHOD ( "ReqATCommandSet", MSG_TYPE_EVENT, MSG_CMD_AT_COMMAND, requestATCommandSet ),
    PROCCESS_METHOD ( "ReqSetSTCPinLock", MSG_TYPE_EVENT, MSG_EVT_SET_STC_PINLOCK, requestSetSTCPinLock ),
    PROCCESS_METHOD ( "ReqSetSTCSimLock", MSG_TYPE_EVENT, MSG_EVT_SET_STC_SIMLOCK, requestSetSTCSimLock ),
    PROCCESS_METHOD ( "ReqSetSTCCellLock", MSG_TYPE_EVENT, MSG_EVT_SET_STC_CELLLOCK, requestSetSTCCellLock ),
    PROCCESS_METHOD ( "ReqSetModemLog", MSG_TYPE_EVENT, MSG_EVT_SET_MODEM_LOG, requestSetModemLog ),

    PROCCESS_METHOD ( "ReqUssdCodeSend", MSG_TYPE_EVENT, MSG_EVT_USSD_CODE_SEND, requestUssdCodeSend ),

    PROCCESS_METHOD ( 0, 0, 0, 0 ),
};

int comdMessageInit()
{
    MessageProccessRegister ( comdMessageProccess );
    MessageInitialize ( MESSAGE_ID_COMD );

    return 0;
}


