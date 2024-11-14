#include "comd_share.h"
#include "atchannel.h"
#include "gct_status_refresh.h"
#include "gct_atcmd_parsing.h"
#include "gct_config_response.h"
#include "config_key.h"
#include "hal_platform.h"

extern int apns_msg_flag[5];
static short check_phoneNum_flag = 0;
static short sim_unlock_flag = 0;
static short connected_flag = 0;
static short simlock_disconnect = 0;

int gctParam_enableRRC ( char* data )
{
    char at_rep[RECV_BUF_SIZE] = {0};

    if ( COMD_AT_PROCESS ( "AT%GDMCNT=1,\"RRC\"", 1500, at_rep, sizeof ( at_rep ) ) )
        return -1;

    return 0;
}

int gctParam_updateImei ( char* data )
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT%IMEI", 1500, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "%IMEI:" ) )
        return parsing_gct_imei ( at_rep );

    return -1;
}

int gctParam_updateSuppBands ( char* data )
{
    char old_suppband[128] = {0};
    char at_rep[RECV_BUF_SIZE] = {0};

    nv_get ( "suppband_org", old_suppband, sizeof ( old_suppband ) );
    if ( strcmp ( old_suppband, "" ) )
    {
        return 0;
    }

    COMD_AT_PROCESS ( "AT%GGETBAND?", 1500, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "%GGETBAND:" ) )
        return parsing_gct_suppband ( at_rep );

    return -1;
}

int gctParam_updateModuleModel ( char* data )
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+CGMM", 1500, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "+CGMM:" ) )
        return parsing_gct_moduleModel ( at_rep );

    return -1;
}

int gctParam_updateModuleType ( char* data )
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+KTCGMM", 1500, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "+KTCGMM:" ) )
        return parsing_gct_moduleType ( at_rep );

    return -1;
}

int gctParam_updateModuleVersion ( char* data )
{
    char at_rep[128] = {0};

    COMD_AT_PROCESS ( "AT+KTCGMR", 1500, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "+KTCGMR:" ) )
        return parsing_gct_moduleVersion ( at_rep );

    return -1;
}

int gctParam_updateFwVersion ( char* data )
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT%SWV1", 1500, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "ARM0:" ) )
        return parsing_gct_FwVersion ( at_rep );

    return -1;
}

int gctParam_updateDefaultGateway ( char* data )
{
    char nv_gateway[8] = {0};
    char config_gw[8] = {0};

    sys_get_config ( LTE_PARAMETER_APN_GW, config_gw, sizeof ( config_gw ) );

    nv_get ( "default_gateway", nv_gateway, sizeof ( nv_gateway ) );
    if ( strcmp ( nv_gateway, "9" ) )
    {
        nv_set ( "default_gateway", strcmp ( config_gw, "" ) ? config_gw : "1" );
    }

    return 0;
}

static int gctParam_updateAttachType ( char* data )
{
    char buffer[4] = {0};

    ucfg_info_get ( "config wan lte autocm attach_type", "attach_type",
                        buffer, sizeof ( buffer ) );

    if ( 0 != strcmp ( buffer, "1" ) && 0 != strcmp ( buffer, "" ) )
    {
        ucfg_info_set ( "config wan lte autocm attach_type", "1" );
        comd_wait_ms ( 1000 );

        CLOGD ( WARNING, "set attach_type to 1 for SMS, reboot GCT module !!!\n" );

        comd_reboot_module();

        return -1;
    }

    return 0;
}

static int gctParam_setIpv6Format ( char* data )
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+CGPIAF=1,1,0,0", 1500, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
        return -1;

    return 0;
}

static int gctParam_setAtEcho ( char* data )
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "ATE0", 1500, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
        return -1;

    return 0;
}

static int gctParam_setCgerep ( char* data )
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+CGEREP=1,0", 1500, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
        return -1;

    return 0;
}

static int gctParam_setSmsCmgf ( char* data )
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+CMGF=0", 1500, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
        return -1;

    return 0;
}

static int gctParam_setSmsCnmi ( char* data )
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+CNMI=2,1,0,0,0", 1500, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
        return -1;

    return 0;
}

static int gctParam_setSmsSmsims ( char* data )
{
    char at_rep[64] = {0};

#if defined(CONFIG_SW_MIFI)
    COMD_AT_PROCESS ( "AT+SMSIMS=0,\"3GPP\",\"OFF\",\"ON\"",
                                1500, at_rep, sizeof ( at_rep ) );
#else
    COMD_AT_PROCESS ( "AT+SMSIMS=0,\"3GPP\",\"OFF\",\"OFF\"",
                                1500, at_rep, sizeof ( at_rep ) );
#endif

    if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
        return -1;

    return 0;
}

static int gctParam_setSmsCpms ( char* data )
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+CPMS=\"ME\",\"ME\",\"ME\"",
                                1500, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
        return -1;

    return 0;
}

static int gctParam_setSmsCsmp ( char* data )
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+CSMP=0,0,0,8", 1500, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
        return -1;

    return 0;
}

#if 0
static int gctParam_getSmsMaxNum ( char* data )
{
    char at_rep[128] = {0};

    COMD_AT_PROCESS ( "AT+CPMS?", 1500, at_rep, sizeof ( at_rep ) );

    parsing_gct_cpms_get ( at_rep );

    if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
        return -1;

    return 0;
}
#endif

static int gctParam_initSmsCenterNum ( char* data )
{
    char center_num[32] = {0};
    char at_req[32] = {0};
    char at_rep[64] = {0};

    sys_get_config ( "lte_param.parameter.sms_center_num",
                            center_num, sizeof ( center_num ) );

    snprintf ( at_req, sizeof ( at_req ), "AT+CSCA=\"%s\"", center_num );
    COMD_AT_PROCESS ( at_req, 1500, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
        return -1;

    return 0;
}

static int gctParam_initTxPowerLimitSet ( char* data )
{
    char txpowlmt_val[8] = {0};
    char at_req[32] = {0};
    char at_rep[64] = {0};

    nv_get("txpower_limit", txpowlmt_val, sizeof(txpowlmt_val));

    CLOGD ( FINE, "setTxPowerLimit: [%s]\n", txpowlmt_val );

    if ( 0 == strcmp ( txpowlmt_val, "" ) )
    {
        snprintf ( at_req, sizeof ( at_req ), "%s%s", "AT%TXPOWLMT=", "0" );
    }
    else
    {
        snprintf ( at_req, sizeof ( at_req ), "%s%s", "AT%TXPOWLMT=", txpowlmt_val );
    }

    COMD_AT_PROCESS ( at_req, 1500, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
        return -1;

    return 0;
}

static int gctParam_initRoamingSet ( char* data )
{
    char roaming_set[8] = {0};
    char module_val[8] = {0};

    sys_get_config ( LTE_PARAMETER_ROAMING, roaming_set, sizeof ( roaming_set ) );

    ucfg_info_get ( "config wan lte plmn_search_param roaming", "roaming", module_val, sizeof ( module_val ) );

    if ( 0 == strcmp ( roaming_set, module_val ) )
    {
        return 0;
    }

    ucfg_info_set ( "config wan lte plmn_search_param roaming", roaming_set );

    return 0;
}

static int gctParam_initNetSelectMode ( char* data )
{
    char m_select_mode[4] = {0};
    char gateway[4] = {0};
    char uciVal[4] = {0};
    char dualwan_mode[16] = {0};

    nv_get ( "default_gateway", gateway, sizeof ( gateway ) );

    ucfg_info_get ( "config wan lte autocm manual", "manual", m_select_mode, sizeof ( m_select_mode ) );

    if ( 0 == strcmp ( gateway, "9" ) )
    {
        if ( strcmp ( m_select_mode, "1" ) )
        {
            strcpy ( m_select_mode, "1" );
            ucfg_info_set ( "config wan lte autocm manual", m_select_mode );
        }
        goto END;
    }


    sys_get_config ( WAN_LTE_SELECT_MODE, uciVal, sizeof ( uciVal ) );

    if ( 0 == strcmp ( uciVal, m_select_mode ) )
    {
        strcpy ( m_select_mode, strcmp ( uciVal, "1" ) ? "1" : "0" );
        ucfg_info_set ( "config wan lte autocm manual", m_select_mode );
    }

    sys_get_config ( DUALWAN_CONFIG_MODE, dualwan_mode, sizeof ( dualwan_mode ) );

    CLOGD ( FINE, "DualWanMode: [%s]\n", dualwan_mode );

END:
    nv_set ( "net_select_mode",
                    strcmp ( m_select_mode, "1" ) ? "auto_select" : "manual_select" );

    return 0;
}

static int gctParam_initBandEarfcnPciLock ( char* data )
{
    char uciScanMode[64] = {0};
    char uciLockedBand[128] = {0};
    char uciLockedFreq[128] = {0};
    char uciLockedPci[128] = {0};
    int i = 0;

    sys_get_config ( WAN_LTE_LOCK_MODE, uciScanMode, sizeof ( uciScanMode ) );

    if ( 0 == strcmp ( uciScanMode, "BAND_LOCK" ) )
    {
        sys_get_config ( WAN_LTE_LOCK_BAND, uciLockedBand, sizeof ( uciLockedBand ) );

        if ( 0 != strcmp ( uciLockedBand, "" ) )
        {
            // "42 43" -> "42_43_"
            for ( i = 0; i < strlen ( uciLockedBand ); i++ )
            {
                if ( ' ' == uciLockedBand[i] )
                    uciLockedBand[i] = '_';
            }
            strcat ( uciLockedBand, "_" );

            gctParam_lockband ( uciLockedBand );
        }
    }
    else if ( 0 == strcmp ( uciScanMode, "FREQ_LOCK" ) )
    {
        sys_get_config ( WAN_LTE_LOCK_FREQ, uciLockedFreq, sizeof ( uciLockedFreq ) );

        if ( 0 != strcmp ( uciLockedFreq, "" ) )
        {
            // "42001 42003" -> "42001_42003_"
            for ( i = 0; i < strlen ( uciLockedFreq ); i++ )
            {
                if ( ' ' == uciLockedFreq[i] )
                    uciLockedFreq[i] = '_';
            }
            strcat ( uciLockedFreq, "_" );

            gctParam_lockearfcn ( uciLockedFreq );
        }
    }
    else if ( 0 == strcmp ( uciScanMode, "PCI_LOCK" ) )
    {
        sys_get_config ( WAN_LTE_LOCK_PCI, uciLockedPci, sizeof ( uciLockedPci ) );

        if ( 0 != strcmp ( uciLockedPci, "" ) )
        {
            // "42005,3 43012,1" -> "42005,3_43012,1_"
            for ( i = 0; i < strlen ( uciLockedPci ); i++ )
            {
                if ( ' ' == uciLockedPci[i] )
                    uciLockedPci[i] = '_';
            }
            strcat ( uciLockedPci, "_" );

            gctParam_lockpci ( uciLockedPci );
        }
    }

    return 0;
}

static int gctParam_initApnSetting ( char* data )
{
    char apnEnabled[4] = {0};
    char apnPdpType[8] = {0};
    char apnProfileName[32] = {0};
    char apnName[32] = {0};
    char apnAuthType[4] = {0};
    char authUserName[32] = {0};
    char authPasswd[32] = {0};
    char apnBandMac[32] = {0};
    int i = 1;
    int cid_index = 3;
    char ucfgSetNode[64] = {0};
    char nvSetNode[32] = {0};
    char at_req[128] = {0};
    char at_rep[64] = {0};
    char strUciOpt[64] = {0};

    for ( i = 1; i <= 4; i++ )
    {
        cid_index = get_cid_from_apn_index ( i );

        snprintf ( strUciOpt, sizeof ( strUciOpt ), LTE_APN_ENABLE, i );
        sys_get_config ( strUciOpt, apnEnabled, sizeof ( apnEnabled ) );
        snprintf ( nvSetNode, sizeof ( nvSetNode ), "apn%d_enable", i );
        nv_set ( nvSetNode, apnEnabled );

        snprintf ( strUciOpt, sizeof ( strUciOpt ), LTE_APN_PDPTYPE, i );
        sys_get_config ( strUciOpt, apnPdpType, sizeof ( apnPdpType ) );
        snprintf ( nvSetNode, sizeof ( nvSetNode ), "pdn%d_type", i );
        nv_set ( nvSetNode, strcmp ( apnPdpType, "IP" ) ? ( strcmp ( apnPdpType, "IPV6" ) ? "2" : "1" ) : "0" );

        snprintf ( strUciOpt, sizeof ( strUciOpt ), LTE_APN_APNNAME, i );
        sys_get_config ( strUciOpt, apnName, sizeof ( apnName ) );
        snprintf ( nvSetNode, sizeof ( nvSetNode ), "apn%d_name", i );
        nv_set ( nvSetNode, apnName );

        memset ( at_rep, 0, sizeof ( at_rep ) );
        if ( 0 == strcmp ( apnEnabled, "0" ) )
        {
            //send AT+CGDCONT=cid_index
            snprintf ( at_req, sizeof (at_req), "AT+CGDCONT=%d", cid_index );
            COMD_AT_PROCESS ( at_req, 1500, at_rep, sizeof ( at_rep ) );
        }
        else
        {
            //send AT+CGDCONT=cid_index,"apnPdpType","apnName"
            snprintf ( at_req, sizeof (at_req), "AT+CGDCONT=%d,\"%s\",\"%s\"", cid_index, apnPdpType, apnName );
            COMD_AT_PROCESS ( at_req, 1500, at_rep, sizeof ( at_rep ) );
        }

        snprintf ( strUciOpt, sizeof ( strUciOpt ), LTE_APN_USERNAME, i );
        sys_get_config ( strUciOpt, authUserName, sizeof ( authUserName ) );
        snprintf ( nvSetNode, sizeof ( nvSetNode ), "user_name%d", i );
        nv_set ( nvSetNode, authUserName );

        snprintf ( strUciOpt, sizeof ( strUciOpt ), LTE_APN_PASSWORD, i );
        sys_get_config ( strUciOpt, authPasswd, sizeof ( authPasswd ) );
        snprintf ( nvSetNode, sizeof ( nvSetNode ), "password%d", i );
        nv_set ( nvSetNode, authPasswd );

        snprintf ( strUciOpt, sizeof ( strUciOpt ), LTE_APN_AUTH_TYPE, i );
        sys_get_config ( strUciOpt, apnAuthType, sizeof ( apnAuthType ) );
        snprintf ( nvSetNode, sizeof ( nvSetNode ), "auth_type%d", i );

        memset ( at_rep, 0, sizeof ( at_rep ) );
        if ( 0 == strcmp ( apnAuthType, "CHAP" ) )
        {
            //send AT+CGAUTH=cid_index,2,"authUserName","authPasswd"
            snprintf ( at_req, sizeof (at_req), "AT+CGAUTH=%d,2,\"%s\",\"%s\"", cid_index, authUserName, authPasswd );
            COMD_AT_PROCESS ( at_req, 1500, at_rep, sizeof ( at_rep ) );
            nv_set ( nvSetNode, "2" );
        }
        else if ( 0 == strcmp ( apnAuthType, "PAP" ) )
        {
            //send AT+CGAUTH=cid_index,1,"authUserName","authPasswd"
            snprintf ( at_req, sizeof (at_req), "AT+CGAUTH=%d,1,\"%s\",\"%s\"", cid_index, authUserName, authPasswd );
            COMD_AT_PROCESS ( at_req, 1500, at_rep, sizeof ( at_rep ) );
            nv_set ( nvSetNode, "1" );
        }
        else
        {
            //send AT+CGAUTH=cid_index
            snprintf ( at_req, sizeof (at_req), "AT+CGAUTH=%d", cid_index );
            COMD_AT_PROCESS ( at_req, 1500, at_rep, sizeof ( at_rep ) );
            nv_set ( nvSetNode, "0" );
        }

        snprintf ( strUciOpt, sizeof ( strUciOpt ), LTE_APN_PROFILE_NAME, i );
        sys_get_config ( strUciOpt, apnProfileName, sizeof ( apnProfileName ) );
        snprintf ( nvSetNode, sizeof ( nvSetNode ), "profile_name%d", i );
        nv_set ( nvSetNode, apnProfileName );

        snprintf ( ucfgSetNode, sizeof ( ucfgSetNode ), LTE_APN_BAND_MAC, i );
        sys_get_config ( ucfgSetNode, apnBandMac, sizeof ( apnBandMac ) );
        snprintf ( nvSetNode, sizeof ( nvSetNode ), "band_mac%d", i );
        nv_set ( nvSetNode, apnBandMac );
    }

    return 0;
}

void gctParam_updatePinStatus ()
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+CPIN?", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "+CPIN:" ) || strstr ( at_rep, "\r\nERROR\r\n" ) )
        parsing_gct_cpin_get ( at_rep );
}

void gctParam_updatePinLockStatus ()
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+CLCK=\"SC\",2", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "+CLCK:" ) )
        parsing_gct_clck_get ( at_rep );
}

void gctParam_updateImsi ()
{
    char at_rep[64] = {0};
    char ram_val[16] = {0};

    nv_get ( "imsi", ram_val, sizeof ( ram_val ) );

    if ( 0 == strcmp ( ram_val, "" ) || 0 == strcmp ( ram_val, "--" ) )
    {
        COMD_AT_PROCESS ( "AT+CIMI", 3000, at_rep, sizeof ( at_rep ) );

        if ( strcmp ( at_rep, "" ) )
            parsing_gct_cimi ( at_rep );

        gctParam_updateUsimMncLen ();
    }
}

void gctParam_updateUsimMncLen ()
{
    char at_rep[128] = {0};

    COMD_AT_PROCESS ( "AT+CRSM=176,28589,0,0,0,0", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "+CRSM:" ) )
        parsing_gct_simMncLen ( at_rep );
}

void gctParam_updateSimSPN ()
{
    char at_rep[128] = {0};
    char ram_val[64] = {0};

    nv_get ( "SIM_SPN", ram_val, sizeof ( ram_val ) );

    if ( 0 == strcmp ( ram_val, "" ) || 0 == strcmp ( ram_val, "--" ) )
    {
        COMD_AT_PROCESS ( "AT+CRSM=176,28486,0,0,17", 3000, at_rep, sizeof ( at_rep ) );

        if ( strstr ( at_rep, "+CRSM:" ) )
            parsing_gct_sim_spn ( at_rep );
    }
}

void gctParam_updateIccid ()
{
    char at_rep[64] = {0};
    char ram_val[32] = {0};

    nv_get ( "iccid", ram_val, sizeof ( ram_val ) );

    if ( 0 == strcmp ( ram_val, "" ) || 0 == strcmp ( ram_val, "--" ) )
    {
        COMD_AT_PROCESS ( "AT%GICCID", 3000, at_rep, sizeof ( at_rep ) );

        if ( strstr ( at_rep, "%GICCID" ) )
            parsing_gct_iccid ( at_rep );
    }
}

void gctParam_updatePinPukCount ()
{
    char at_rep[256] = {0};

    COMD_AT_PROCESS ( "AT+CPINR", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "+CPINR:" ) )
        parsing_gct_cpinr ( at_rep );
}

void gctParam_updatePhoneNumber ()
{
    char at_rep[64] = {0};

    if ( 0 == check_phoneNum_flag )
    {
        COMD_AT_PROCESS ( "AT+CNUM", 3000, at_rep, sizeof ( at_rep ) );

        if ( strcmp ( at_rep, "" ) )
        {
            check_phoneNum_flag = 1;
            parsing_gct_cnum ( at_rep );
        }
    }
}

void gctParam_updateSmsCenterNum ()
{
    char at_rep[64] = {0};
    char ram_val[32] = {0};

    nv_get ( "sms_center_num", ram_val, sizeof ( ram_val ) );

    if ( 0 == strcmp ( ram_val, "" ) || 0 == strcmp ( ram_val, "--" ) )
    {
        COMD_AT_PROCESS ( "AT+CSCA?", 3000, at_rep, sizeof ( at_rep ) );

        if ( strstr ( at_rep, "+CSCA:" ) )
            parsing_gct_csca_get ( at_rep );
    }
}

void gctParam_updateCesq ()
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+CESQ", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "+CESQ:" ) )
        parsing_gct_cesq ( at_rep );
}

void gctParam_updateRrcState ()
{
    char at_rep[512] = {0};

    COMD_AT_PROCESS ( "AT!GSTATUS?", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "!GSTATUS:" ) )
    {
        if ( strstr ( at_rep, "RRC CONNECTED" ) )
        {
            nv_set ( "rrc_state", "CONNECTED" );
            CLOGD ( FINE, "rrc_state -> [CONNECTED]\n" );
        }
        else if ( strstr ( at_rep, "RRC IDLE" ) )
        {
            nv_set ( "rrc_state", "IDLE" );
            CLOGD ( FINE, "rrc_state -> [IDLE]\n" );
        }
    }
    else
    {
        nv_set ( "rrc_state", "--" );
    }
}

void gctParam_updateCereg ()
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+CEREG?", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "+CEREG:" ) )
        parsing_gct_cereg ( at_rep );
}

void gctParam_updateCgatt ()
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+CGATT?", 5000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "+CGATT:" ) )
        parsing_gct_cgatt_get ( at_rep );
}

void gctParam_updateCgact ()
{
    char at_rep[128] = {0};

    COMD_AT_PROCESS ( "AT+CGACT?", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "\r\nOK\r\n" ) )
        parsing_gct_cgact_get ( at_rep );
}

int gctParam_configCgact ( int cid_index, int act_val )
{
    char at_req[16] = {0};
    char at_rep[128] = {0};

    snprintf ( at_req, sizeof ( at_req ), "AT+CGACT=%d,%d", act_val, cid_index );
    COMD_AT_PROCESS ( at_req, 3000, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
        return -1;

    return 0;
}

void gctParam_updateKtlteinfo ()
{
    char at_rep[RECV_BUF_SIZE] = {0};

    COMD_AT_PROCESS ( "AT%KTLTEINFO", 3000, at_rep, sizeof ( at_rep ) );

    if ( strcmp ( at_rep, "" ) )
        parsing_gct_ktlteinfo ( at_rep );
}

void gctParam_updateGdmitem ()
{
    char at_rep[RECV_BUF_SIZE * 3] = {0};

    COMD_AT_PROCESS ( "AT%GDMITEM?", 3000, at_rep, sizeof ( at_rep ) );

    if ( strcmp ( at_rep, "" ) )
        parsing_gct_gdmitem ( at_rep );
}

void gctParam_updateGlteconnstatus ()
{
    char at_rep[RECV_BUF_SIZE] = {0};

    COMD_AT_PROCESS ( "AT%GLTECONNSTATUS", 3000, at_rep, sizeof ( at_rep ) );

    if ( strcmp ( at_rep, "" ) )
        parsing_gct_glteconnstatus ( at_rep );
}

void gctParam_updateGdmmodtc ()
{
    char at_rep[RECV_BUF_SIZE] = {0};

    COMD_AT_PROCESS ( "AT%GDMMODTC", 3000, at_rep, sizeof ( at_rep ) );

    if ( strcmp ( at_rep, "" ) )
        parsing_gct_gdmmodtc ( at_rep );
}

void gctParam_updateGbler ()
{
/*
    char at_rep[RECV_BUF_SIZE] = {0};

    COMD_AT_PROCESS ( "AT%GBLER", 3000, at_rep, sizeof ( at_rep ) );

    if ( strcmp ( at_rep, "" ) )
        parsing_gct_gbler ( at_rep );
*/
}

void gctParam_updateGharq ()
{
/*
    char at_rep[RECV_BUF_SIZE] = {0};

    COMD_AT_PROCESS ( "AT%GHARQ", 3000, at_rep, sizeof ( at_rep ) );

    if ( strcmp ( at_rep, "" ) )
        parsing_gct_gharq ( at_rep );
*/
}

void gctParam_updateDefbrdp ( int cid_index )
{
    char at_req[16] = {0};
    char at_rep[RECV_BUF_SIZE] = {0};

    snprintf ( at_req, sizeof ( at_req ), "%s=%d", "AT%DEFBRDP", cid_index );
    COMD_AT_PROCESS ( at_req, 3000, at_rep, sizeof ( at_rep ) );

    if ( strcmp ( at_rep, "" ) )
        parsing_gct_defbrdp ( cid_index, at_rep );
}

void gctParam_updateGdmcqi ()
{
    char at_rep[RECV_BUF_SIZE] = {0};

    COMD_AT_PROCESS ( "AT%GDMCQI", 3000, at_rep, sizeof ( at_rep ) );

    if ( strcmp ( at_rep, "" ) )
        parsing_gct_gdmcqi ( at_rep );
}

void gctParam_updateGrankindex ()
{
    char at_rep[RECV_BUF_SIZE] = {0};

    COMD_AT_PROCESS ( "AT%GRANKINDEX", 3000, at_rep, sizeof ( at_rep ) );

    if ( strcmp ( at_rep, "" ) )
        parsing_gct_grankindex ( at_rep );
}

int gctParam_updateCgcontrdp ( int cid_index )
{
    char at_req[16] = {0};
    char at_rep[RECV_BUF_SIZE * 2] = {0};

    snprintf ( at_req, sizeof ( at_req ), "%s=%d", "AT+CGCONTRDP", cid_index );
    COMD_AT_PROCESS ( at_req, 3000, at_rep, sizeof ( at_rep ) );

    return strstr ( at_rep, "+CGCONTRDP:" ) ? parsing_gct_cgcontrdp ( cid_index, at_rep ) : 0;
}

void gctParam_updateCmglFour ()
{
    char at_rep[RECV_SMS_SIZE] = {0};

    COMD_AT_PROCESS ( "AT+CMGL=4", 5000, at_rep, sizeof ( at_rep ) );
    if ( strcmp ( at_rep, "" ) )
        parsing_gct_cmgl_four ( at_rep );
}

void gctParam_updateWanTraffic ()
{
    char at_rep[RECV_BUF_SIZE * 4] = {0};

    COMD_AT_PROCESS ( "AT%SYSCMD=\"cat /proc/net/dev | grep lte0pdn[0123]:\"",
                                                    5000, at_rep, sizeof ( at_rep ) );
    if ( strcmp ( at_rep, "" ) )
        parsing_gct_wanTraffic ( at_rep );
}

int gctParam_updateNetworkSearch()
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT%PSSTATUS", 3000, at_rep, sizeof ( at_rep ) );
    /*  declaration from ltetype.h -> _PLMN_SEARCH_STATUS_READ_RSP_INFO
        - 0 : PLMN Search Success status(Camping Status)
        - 1 : Not Camping Status and GDM is searching PLMN
        - 2 : Not Camping Status and GDM is not searching PLMN
    */
    CLOGD(FINE, "<updateNetworkSearch>: PASSTATUS=%s\n",at_rep);
    if ( ( strstr ( at_rep, "PSSTATUS: 2" ) ) )
    {
        return 0;
    }

    return 1;
}

static void gct_init_apn_netdev ()
{
#if defined(_GDM7243_)
    nv_set ( "apn1_netdev", "lte0pdn0" );
    nv_set ( "apn2_netdev", "lte0pdn1" );
    nv_set ( "apn3_netdev", "lte0pdn2" );
    nv_set ( "apn4_netdev", "lte0pdn3" );
#else
    char wan_if[64] = {0};

    sys_get_config ( LTE_PARAMETER_WAN_IF, wan_if, sizeof ( wan_if ) );

    if ( strstr ( wan_if, "eth2" ) )
    {
        nv_set ( "apn1_netdev", "eth2.100" );
        nv_set ( "apn2_netdev", "eth2.101" );
        nv_set ( "apn3_netdev", "eth2.102" );
        nv_set ( "apn4_netdev", "eth2.103" );
    }
    else
    {
        nv_set ( "apn1_netdev", "eth1.100" );
        nv_set ( "apn2_netdev", "eth1.101" );
        nv_set ( "apn3_netdev", "eth1.102" );
        nv_set ( "apn4_netdev", "eth1.103" );
    }
#endif
}

int gct_module_param_init()
{
    /* tac lock iptables chain clean */
    system ( "/usr/sbin/limit_internet.sh del" );

    gct_init_apn_netdev();

    reset_apns_msg_flag ( 0 );
    check_phoneNum_flag = 0;
    sim_unlock_flag = 0;
    connected_flag = 0;
    simlock_disconnect = 0;
    memsetGctParsingStr();

    if (
        gctParam_setAtEcho ( NULL ) ||
        gctParam_setCgerep ( NULL ) ||
        gctParam_setIpv6Format ( NULL ) ||
        gctParam_enableRRC ( NULL ) ||
        gctParam_updateImei ( NULL ) ||
        gctParam_updateSuppBands ( NULL ) ||
        gctParam_updateModuleType ( NULL ) ||
        gctParam_updateModuleModel ( NULL ) ||
        gctParam_updateModuleVersion ( NULL ) ||
        gctParam_updateFwVersion ( NULL ) ||
        gctParam_updateDefaultGateway ( NULL )
    )
        return -1;

    if ( 1 == CONFIG_SW_SMS &&
        (
        gctParam_updateAttachType ( NULL ) ||
        gctParam_setSmsCmgf ( NULL ) ||
        gctParam_setSmsCnmi ( NULL ) ||
        gctParam_setSmsSmsims ( NULL ) ||
        gctParam_setSmsCpms ( NULL ) ||
#if 0
        gctParam_getSmsMaxNum ( NULL ) ||
#endif
        gctParam_setSmsCsmp ( NULL ) ||
        gctParam_initSmsCenterNum ( NULL )
        )
    )
        return -1;

    gctParam_radioOnOffSet ( "off" );

    if (
        gctParam_initTxPowerLimitSet ( NULL ) ||
        gctParam_initRoamingSet ( NULL ) ||
        gctParam_initNetSelectMode ( NULL ) ||
        gctParam_initApnSetting ( NULL ) ||
        gctParam_initBandEarfcnPciLock ( NULL )
    )
        return -1;

    comd_wait_ms ( 1000 );
    gctParam_radioOnOffSet ( "on" );

    return 0;
}

static void reset_gct_lte_status ( int flag )
{
    switch ( flag )
    {
    case 0:
        lteinfo_data_restore();
    case 1:
        memsetGctParsingStr();
        nv_set ( "cqi",              "" );
        nv_set ( "rankIndex",        "" );
        nv_set ( "dl_qam",       "QPSK" );
        nv_set ( "ul_qam",       "QPSK" );
        nv_set ( "cereg_stat",      "0" );
        nv_set ( "cgatt_val",       "0" );
        nv_set ( "cid_1_state",     "0" );
        nv_set ( "cid_2_state",     "0" );
        nv_set ( "cid_3_state",     "0" );
        nv_set ( "cid_4_state",     "0" );
        nv_set ( "apn1_qci",         "" );
        nv_set ( "apn2_qci",         "" );
        nv_set ( "apn3_qci",         "" );
        nv_set ( "apn4_qci",         "" );
        nv_set ( "bler_err1",       "0" );
        nv_set ( "bler_err2",       "0" );
        nv_set ( "bler_total1",     "0" );
        nv_set ( "bler_total2",     "0" );
        nv_set ( "initial_harq1",   "0" );
        nv_set ( "initial_harq2",   "0" );
        nv_set ( "initial_bleq1",   "0" );
        nv_set ( "initial_bleq2",   "0" );
        nv_set ( "retx_harq1",      "0" );
        nv_set ( "retx_harq2",      "0" );
        nv_set ( "secondary_cell",  "0" );
        ipv4v6_data_restore ( 0 );
        break;
    default:
        break;
    }
}

void gct_normal_status_refresh()
{
    char gateway[4] = {0};
    char cpin_value[16] = {0};
    char mStatusVal[32] = {0};
    char cesq_rsrq_val[16] = {0};
    char cesq_rsrp_val[16] = {0};
    char cereg_stat_val[4] = {0};
    char cgatt_stat_val[4] = {0};
    char radio_stat[8] = {0};
    char simlock_stat[4] = {0};
    char uptime[128] = {0};
    char connectTime[32]= {0};
    char ram_val[32] = {0};
    char apn_act_state[4] = {0};
    char apn_enable_state[4] = {0};
    int id_num = 0;
    int cid_index = 0;
    static short refresh_config = 0;

    gctParam_updatePinStatus ();
    gctParam_updatePinLockStatus ();
    gctParam_updatePinPukCount ();

    nv_get ( "cpin", cpin_value, sizeof ( cpin_value ) );
    nv_get ( "main_status", mStatusVal, sizeof ( mStatusVal ) );
    nv_get ( "default_gateway", gateway, sizeof ( gateway ) );

    if ( 0 == strcmp ( cpin_value, "READY" ) )
    {
        gctParam_updateImsi ();
        gctParam_updateSimSPN ();
        gctParam_updateIccid ();
        gctParam_updatePhoneNumber ();
        if ( 1 == CONFIG_SW_SMS )
        {
            gctParam_updateSmsCenterNum ();
        }
        gctParam_getSimlock ( NULL );
        gctParam_updateCesq ();
        nv_get ( "cesq_rsrq", cesq_rsrq_val, sizeof ( cesq_rsrq_val ) );
        nv_get ( "cesq_rsrp", cesq_rsrp_val, sizeof ( cesq_rsrp_val ) );
        nv_get ( "simlocked", simlock_stat, sizeof ( simlock_stat ) );
        nv_get ( "lte_on_off", radio_stat, sizeof ( radio_stat ) );
        if ( 0 == strcmp ( cesq_rsrq_val, "255" ) && 0 == strcmp ( cesq_rsrp_val, "255" ) )
        {
            if ( gctParam_updateNetworkSearch() )
            {
                nv_set( "main_status", "searching" );

            }
            else if ( strcmp ( mStatusVal, "no_service" ) )
            {
                nv_set ( "main_status", "no_service" );
            }
            reset_gct_lte_status ( 0 );
            apnActOrDeactMsgEvent ( 0, 0 );
            if ( 0 == strcmp ( simlock_stat, "0" ) && 1 == simlock_disconnect &&
                    0 != strcmp ( radio_stat, "off" ) && 0 != strcmp ( gateway, "9" ) )
            {
                gctParam_plmnSearchManualSelect ( "" );
                simlock_disconnect = 0;
            }
        }
        else
        {
            gctParam_updateRrcState ();
            gctParam_updateCereg ();
            nv_get ( "cereg_stat", cereg_stat_val, sizeof ( cereg_stat_val ) );
            gctParam_updateCgatt ();
            nv_get ( "cgatt_val", cgatt_stat_val, sizeof ( cgatt_stat_val ) );
            if ( 0 == strcmp ( cgatt_stat_val, "1" ) )
            {
                if ( 0 == strcmp ( simlock_stat, "1" ) )
                {
                    gctParam_disconnectNetwork ( "" );
                    simlock_disconnect = 1;
                    return;
                }

                nv_set ( "main_status", "connected" );
                apnActOrDeactMsgEvent ( 0, 1 );

                nv_get ( "connected_time", connectTime, sizeof ( connectTime ) );
                if ( 0 == connected_flag || 0 == strcmp ( connectTime, "0" ) )
                {
                    struct sysinfo info;
                    sysinfo ( &info );
                    snprintf ( uptime, sizeof ( uptime ), "%ld", info.uptime );
                    nv_set ( "connected_time", uptime );
                    connected_flag = 1;
                }

                gctParam_updateCgact ();

                int multi_apn_act = 0;

                for ( id_num = 1; id_num <= 4; id_num++ )
                {
                    cid_index = get_cid_from_apn_index ( id_num );
                    snprintf ( ram_val, sizeof ( ram_val ), "cid_%d_state", cid_index );
                    nv_get ( ram_val, apn_act_state, sizeof ( apn_act_state ) );
                    snprintf ( ram_val, sizeof ( ram_val ), "apn%d_enable", id_num );
                    nv_get ( ram_val, apn_enable_state, sizeof ( apn_enable_state ) );
                    if ( 0 == strcmp ( apn_act_state, "1" ) )
                    {
                        gctParam_updateDefbrdp ( cid_index );
                        if ( gctParam_updateCgcontrdp ( cid_index ) )
                        {
                            apnActOrDeactMsgEvent ( id_num, 1 );
                        }
                    }
                    else
                    {
                        parsing_gct_defbrdp ( cid_index, "" );
                        parsing_gct_cgcontrdp ( cid_index, "" );
                        apnActOrDeactMsgEvent ( id_num, 0 );
                        if ( 0 == multi_apn_act && 0 == strcmp ( apn_enable_state, "1" ) )
                        {
                            multi_apn_act = 1;
                            gctParam_configCgact ( cid_index, 1 );
                        }
                    }
                }

                if ( 0 == refresh_config )
                {
                    gctParam_updateGdmcqi ();
                    gctParam_updateGrankindex ();
                }
                else
                {
                    gctParam_updateGbler ();
                    gctParam_updateGharq ();
                    if ( 1 == CONFIG_SW_SMS )
                    {
                        gctParam_updateCmglFour ();
                    }
                }
                refresh_config = ( refresh_config ? 0 : 1 );
#if defined(_MTK_7981_)
                gctParam_updateWanTraffic ();
#endif
                gctParam_updateGdmmodtc ();
            }
            else
            {
                if ( strcmp ( mStatusVal, "disconnected" ) )
                {
                    reset_gct_lte_status ( 1 );
                    nv_set ( "main_status", "disconnected" );
                }
                apnActOrDeactMsgEvent ( 0, 0 );
            }

            gctParam_updateKtlteinfo ();
        }
    }
    else if ( 0 == strcmp ( cpin_value, "SIM PIN" ) )
    {
        if ( strcmp ( mStatusVal, "need_pin" ) )
        {
            reset_gct_lte_status ( 0 );
            nv_set ( "main_status", "need_pin" );
        }
        apnActOrDeactMsgEvent ( 0, 0 );
        if ( 0 == sim_unlock_flag )
        {
            char pin_code[128] = {0};
            ucfg_info_get ( "config wan lte sim_pincode", "sim_pincode",
                                    pin_code, sizeof ( pin_code ) );
            if ( strcmp ( pin_code, "" ) )
            {
                gctParam_enterPin ( pin_code );
            }
            sim_unlock_flag = 1;
        }
    }
    else if ( 0 == strcmp ( cpin_value, "SIM PUK" ) )
    {
        if ( strcmp ( mStatusVal, "need_puk" ) )
        {
            gctParam_disconnectNetwork ( NULL );
            reset_gct_lte_status ( 0 );
            nv_set ( "main_status", "need_puk" );
        }
        apnActOrDeactMsgEvent ( 0, 0 );
    }
    else
    {
        if ( strcmp ( mStatusVal, "sim_not_ready" ) )
        {
            reset_gct_lte_status ( 0 );
            nv_set ( "main_status", "sim_not_ready" );
        }
        apnActOrDeactMsgEvent ( 0, 0 );
    }
}

void gct_install_mode_refresh()
{
    char cur_select_mode[32] = {0};

    nv_get ( "net_select_mode", cur_select_mode, sizeof ( cur_select_mode ) );

    if ( 0 == strcmp ( cur_select_mode, "manual_select" ) )
    {
        gctParam_updateGlteconnstatus ();
        return;
    }

    gctParam_updateKtlteinfo ();
    comd_wait_ms ( 1000 );
#if !defined(gdm7243st)
    gctParam_updateGdmitem ();
#endif
}

