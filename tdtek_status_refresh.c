#include "comd_share.h"
#include "atchannel.h"
#include "tdtek_status_refresh.h"
#include "tdtek_atcmd_parsing.h"
#include "tdtek_config_response.h"
#include "config_key.h"
#include "hal_platform.h"

static int abnormal_check_flag[5] = { 1, 1, 1, 1, 1 };
static int cpin_error_check = 0;
static int main_status_check = 0;
static int dial_status_check = 0;
static long abnormal_start_time = 0;

extern int manual_conn;
extern char led_mode[4];
extern char dualsim_en[4];
extern int apns_msg_flag[5];
extern char volte_enable[8];

static void tdtekParam_radioRestart ()
{
    char at_rep[512] = {0};

    COMD_AT_PROCESS ( "AT+CFUN=0", 10000, at_rep, sizeof ( at_rep ) );

    comd_wait_ms ( 1500 );

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( "AT+CFUN=1", 10000, at_rep, sizeof ( at_rep ) );
}

int tdtekParam_setATEcho ( char* data )
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "ATE0", 3000, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
        return -1;

    return 0;
}

int tdtekParam_setIpv6Format ( char* data )
{
    char at_rep[64] = {0};
    COMD_AT_PROCESS ( "AT+CGPIAF=1,1,0,1", 10000, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
        return -1;

    return 0;
}

int tdtekParam_setNetnum ( char* data )
{
#if defined(_MTK_7621_) || defined(_MTK_7981_) || defined(_MTK_7628_)
    char at_rep[256] = {0};

    COMD_AT_PROCESS ( "AT^SETNETNUM?", 3000, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "^SETNETNUM:1" ) )
    {
        memset ( at_rep, 0, sizeof( at_rep ) );
        COMD_AT_PROCESS ( "AT^SETNETNUM=1", 3000, at_rep, sizeof ( at_rep ) );

        if ( strstr ( at_rep, "OK" ) )
        {
            comd_wait_ms ( 5000 );
            CLOGD ( WARNING, "Switch to bridge mode, reboot unisoc module !!!\n" );
            COMD_AT_PROCESS ( "AT+CFUN=1,1", 3000, at_rep, sizeof ( at_rep ) );
        }
        return -1;
    }

    memset ( at_rep, 0, sizeof( at_rep ) );
    COMD_AT_PROCESS ( "AT^SETMODE?", 3000, at_rep, sizeof ( at_rep ) );

    memset ( at_rep, 0, sizeof( at_rep ) );
    COMD_AT_PROCESS ( "AT^TDCFG?", 3000, at_rep, sizeof ( at_rep ) );

    system ( "/lib/rename_wan_if.sh" );

    CLOGD ( FINE, "rename wan if and exit!\n" );
#endif

    return 0;
}

int tdtekParam_radioLockInit ( char* data )
{
    char uciScanMode[16] = {0};
    char uciLocked5gBand[128] = {0};
    char uciLocked4gBand[128] = {0};
    char uciLockedBand[256] = {0};
    char uciLockedFreq[256] = {0};
    char uciLockedPci[256] = {0};

    sys_get_config ( WAN_LTE_LOCK_MODE, uciScanMode, sizeof ( uciScanMode ) );

    if ( 0 == strcmp ( uciScanMode, "BAND_LOCK" ) )
    {
        sys_get_config ( WAN_LTE_LOCK_BAND, uciLocked4gBand, sizeof ( uciLocked4gBand ) );

        if ( 0 != strcmp ( uciLocked4gBand, "" ) )
        {
            // "42 43" -> "42_43_"
            comd_strrpl ( uciLocked4gBand, " ", "_" );
            strcat ( uciLocked4gBand, "_" );
        }

        sys_get_config ( WAN_LTE_LOCK_BAND5G, uciLocked5gBand, sizeof ( uciLocked5gBand ) );

        if ( 0 != strcmp ( uciLocked5gBand, "" ) )
        {
            // "42 43" -> "42_43_"
            comd_strrpl ( uciLocked5gBand, " ", "_" );
            strcat ( uciLocked5gBand, "_" );
        }

        snprintf ( uciLockedBand, sizeof ( uciLockedBand ), "%s,%s", uciLocked4gBand, uciLocked5gBand );

        tdtekParam_lockband ( uciLockedBand, 0 );
    }
    else if ( 0 == strcmp ( uciScanMode, "FREQ_LOCK" ) )
    {
        sys_get_config ( WAN_LTE_LOCK_FREQ, uciLockedFreq, sizeof ( uciLockedFreq ) );

        if ( 0 != strcmp ( uciLockedFreq, "" ) )
        {
            // "4G,41,40000;5G,28,156100,15;"
            tdtekParam_lockearfcn ( uciLockedFreq, 0 );
        }
    }
    else if ( 0 == strcmp ( uciScanMode, "PCI_LOCK" ) )
    {
        sys_get_config ( WAN_LTE_LOCK_PCI, uciLockedPci, sizeof ( uciLockedPci ) );

        if ( 0 != strcmp ( uciLockedPci, "" ) )
        {
            // "4G,41,40000,104;5G,28,156100,505,15;"
            tdtekParam_lockpci ( uciLockedPci, 0 );
        }
    }
    else if ( NULL == data )
    {
        tdtekParam_lockband ( "", 0 );
    }
    else if ( 0 == strcmp ( data, "restore_fullband" ) )
    {
        tdtekParam_lockband ( "", 1 );
    }

    return 0;
}

int tdtekParam_updateImei ( char* data )
{
    return parsing_tdtek_imei ( strstr ( data, "IMEI:" ) );
}

int tdtekParam_updateSN ( char* data )
{
#if defined(_MTK_7621_) || defined(_MTK_7981_) || defined(_MTK_7628_)
    return 0;
#endif
    char ram_val[64] = {0};

    nv_get ( "SN", ram_val, sizeof ( ram_val ) );

    if ( 0 == strcmp ( ram_val, "" ) || 0 == strcmp ( ram_val, "--" ) )
    {
        memset ( ram_val, 0, sizeof ( ram_val ) );
        comd_exec ( "attools -t getsn", ram_val, sizeof ( ram_val ) );
        parsing_tdtek_sn ( ram_val );
    }

    return 0;
}

int tdtekParam_updateModuleModel ( char* data )
{
    return parsing_tdtek_moduleModel ( strstr ( data, "Model:" ) );
}

int tdtekParam_updateModuleVersion ( char* data )
{
    return parsing_tdtek_moduleVersion ( strstr ( data, "Revision:" ) );
}

/*
 * Manufacturer: TD Tech Ltd.
 * Model: Modem V100R100C10B670
 * Revision: 22C10B660S000C000
 * IMEI: 863495060066501
 * +GCAP: +CGSM,+DS,+ES
 */
int tdtekParam_updateATI ( char* data )
{
    int ret = 0;
    char at_rep[256] = {0};

    COMD_AT_PROCESS ( "ATI", 1500, at_rep, sizeof ( at_rep ) );

    ret |= tdtekParam_updateImei ( at_rep );
    ret |= tdtekParam_updateSN ( at_rep );
    ret |= tdtekParam_updateModuleModel ( at_rep );
    ret |= tdtekParam_updateModuleVersion ( at_rep );

    return ret;
}

char SYSCFGEX_F[32] = "2000000680380";  // need default ??
char SYSCFGEX_S[32] = "1E200000095";    // need default ??

int tdtekParam_updateSuppBands ( char* data )
{
    char module_type[32] = {0};
    char lte_bands[256] = {0};
    char nr5g_bands[256] = {0};

    char old_suppband[128] = {0};
    char old_suppband5g[128] = {0};

    nv_get ( "modulemodel", module_type, sizeof ( module_type ) );

    snprintf ( lte_bands, sizeof ( lte_bands ), "%s",
                            strstr ( module_type, "MT5710-CN" ) ? TDTEK_MT5710_CN_4G_BANDS : ""
                    );

    snprintf ( nr5g_bands, sizeof ( nr5g_bands ), "%s",
                            strstr ( module_type, "MT5710-CN" ) ? TDTEK_MT5710_CN_5G_BANDS : ""
                    );

    if ( strstr ( module_type, "MT5710-CN" ) )
    {
        strcpy ( SYSCFGEX_F, "2000000680380" );
        strcpy ( SYSCFGEX_S, "1E200000095" );
    }

    if ( strcmp ( lte_bands, "" ) || strcmp ( nr5g_bands, "" ) )
    {
        CLOGD ( FINE, "4G_BANDS_ORG -> [%s]\n", lte_bands );
        nv_set ( "suppband_org", lte_bands );
        comd_strrpl ( lte_bands, ":", " " );
        CLOGD ( FINE, "4G_SUPPBAND  -> [%s]\n", lte_bands );
        nv_set ( "suppband", lte_bands );

        CLOGD ( FINE, "5G_BANDS_ORG -> [%s]\n", nr5g_bands );
        nv_set ( "suppband5g_org", nr5g_bands );
        comd_strrpl ( nr5g_bands, ":", " " );
        CLOGD ( FINE, "5G_SUPPBAND  -> [%s]\n", nr5g_bands );
        nv_set ( "suppband5g", nr5g_bands );

        sys_get_config ( LTE_PARAMETER_SUPPORT_BAND, old_suppband, sizeof ( old_suppband ) );
        sys_get_config ( LTE_PARAMETER_SUPPORT_BAND5G, old_suppband5g, sizeof ( old_suppband5g ) );
        if ( strcmp ( old_suppband, lte_bands ) )
        {
            CLOGD ( WARNING, "Save supported bands to flash !\n" );
            sys_set_config ( LTE_PARAMETER_SUPPORT_BAND, lte_bands );
            sys_commit_config ( "lte_param" );
        }
        if ( strcmp ( old_suppband5g, nr5g_bands ) )
        {
            CLOGD ( WARNING, "Save supported bands to flash !\n" );
            sys_set_config ( LTE_PARAMETER_SUPPORT_BAND5G, nr5g_bands );
            sys_commit_config ( "lte_param" );
        }

        return 0;
    }

    CLOGD ( ERROR, "No support band list for [%s]\n", module_type );

    return -1;
}

int tdtekParam_updateCGDCONT ( char* data )
{
    char at_rep[RECV_BUF_SIZE * 2] = {0};

    COMD_AT_PROCESS ( "AT+CGDCONT?", 3000, at_rep, sizeof ( at_rep ) );

    if ( NULL == data && strstr ( at_rep, "\r\n+CGDCONT:" ) )
    {
        return parsing_tdtek_apn ( at_rep );
    }

    return -1;
}

int tdtekParam_initAPNsetting ( char* data )
{
    char uci_apnenable[4] = {0};
    char uci_pdptype[8] = {0};
    char uci_apnname[64] = {0};
    char uci_profilename[64] = {0};
    char uci_authtype[8] = {0};
    char uci_username[64] = {0};
    char uci_password[64] = {0};
    char strUciOpt[128] = {0};
    char strRamOpt[128] = {0};
    char at_rep[128] = {0};
    char at_cmd[128] = {0};
    int i = 0;
    int apnset_start_index = 1;
    int apnset_end_index = 4;

    if ( data )
    {
        apn_profile* apn_settingdata = ( apn_profile* ) data;
        apnset_start_index = apn_settingdata->index + 1;
        apnset_end_index = apn_settingdata->index + 1;

        snprintf ( uci_profilename, sizeof ( uci_profilename ), "%s", apn_settingdata->profile_name );
        CLOGD ( FINE, "apn_msg_profilename -> [%s]\n", uci_profilename );

        if ( 0 == strcmp ( apn_settingdata->pdn_type, "0" ) )
        {
            strcpy ( uci_pdptype, "IP" );
        }
        else if ( 0 == strcmp ( apn_settingdata->pdn_type, "1" ) )
        {
            strcpy ( uci_pdptype, "IPV6" );
        }
        else
        {
            strcpy ( uci_pdptype, "IPV4V6" );
        }
        CLOGD ( FINE, "apn_msg_pdptype -> [%s]\n", uci_pdptype );

        snprintf ( uci_apnname, sizeof ( uci_apnname ), "%s", apn_settingdata->apn_name );
        CLOGD ( FINE, "apn_msg_apnname -> [%s]\n", uci_apnname );

        snprintf ( uci_apnenable, sizeof ( uci_apnenable ), "%s", apn_settingdata->apn_enable );
        CLOGD ( FINE, "apn_msg_enabled -> [%s]\n", uci_apnenable );

        snprintf ( uci_authtype, sizeof ( uci_authtype ), "%s", apn_settingdata->auth_type );
        CLOGD ( FINE, "apn_msg_authtype -> [%s]\n", uci_authtype );

        snprintf ( uci_username, sizeof ( uci_username ), "%s", apn_settingdata->user_name );
        CLOGD ( FINE, "apn_msg_username -> [%s]\n", uci_username );

        snprintf ( uci_password, sizeof ( uci_password ), "%s", apn_settingdata->password );
        CLOGD ( FINE, "apn_msg_password -> [%s]\n", uci_password );

        nv_set ( "apn_set", "0" );
    }

    for ( i = apnset_start_index; i <= apnset_end_index; i++ )
    {
        if ( apnset_start_index < apnset_end_index )
        {
            snprintf ( strUciOpt, sizeof ( strUciOpt ), LTE_APN_PROFILE_NAME, i );
            sys_get_config ( strUciOpt, uci_profilename, sizeof ( uci_profilename ) );
            CLOGD ( FINE, "%s -> [%s]\n", strUciOpt, uci_profilename );
        }

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "profile_name%d", i );
        nv_set ( strRamOpt, uci_profilename );

        if ( apnset_start_index < apnset_end_index )
        {
            snprintf ( strUciOpt, sizeof ( strUciOpt ), LTE_APN_PDPTYPE, i );
            sys_get_config ( strUciOpt, uci_pdptype, sizeof ( uci_pdptype ) );
            CLOGD ( FINE, "%s -> [%s]\n", strUciOpt, uci_pdptype );
        }

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "pdn%d_type", i );
        nv_set ( strRamOpt, strcmp ( uci_pdptype, "IP" ) ? ( strcmp ( uci_pdptype, "IPV6" ) ? "2" : "1" ) : "0" );

        if ( apnset_start_index < apnset_end_index )
        {
            snprintf ( strUciOpt, sizeof ( strUciOpt ), LTE_APN_APNNAME, i );
            sys_get_config ( strUciOpt, uci_apnname, sizeof ( uci_apnname ) );
            CLOGD ( FINE, "%s -> [%s]\n", strUciOpt, uci_apnname );
        }

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_name", i );
        nv_set ( strRamOpt, uci_apnname );

        if ( apnset_start_index < apnset_end_index )
        {
            snprintf ( strUciOpt, sizeof ( strUciOpt ), LTE_APN_ENABLE, i );
            sys_get_config ( strUciOpt, uci_apnenable, sizeof ( uci_apnenable ) );
            CLOGD ( FINE, "%s -> [%s]\n", strUciOpt, uci_apnenable );
        }

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_enable", i );
        nv_set ( strRamOpt, uci_apnenable );

        if ( apnset_start_index < apnset_end_index )
        {
            snprintf ( strUciOpt, sizeof ( strUciOpt ), LTE_APN_AUTH_TYPE, i );
            sys_get_config ( strUciOpt, uci_authtype, sizeof ( uci_authtype ) );
            CLOGD ( FINE, "%s -> [%s]\n", strUciOpt, uci_authtype );
            strcpy ( uci_authtype, strcmp ( uci_authtype, "CHAP" ) ? ( strcmp ( uci_authtype, "PAP" ) ? "0" : "1" ) : "2" );
        }

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "auth_type%d", i );
        nv_set ( strRamOpt, uci_authtype );

        if ( apnset_start_index < apnset_end_index )
        {
            snprintf ( strUciOpt, sizeof ( strUciOpt ), LTE_APN_USERNAME, i );
            sys_get_config ( strUciOpt, uci_username, sizeof ( uci_username ) );
            CLOGD ( FINE, "%s -> [%s]\n", strUciOpt, uci_username );
        }

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "user_name%d", i );
        nv_set ( strRamOpt, uci_username );

        if ( apnset_start_index < apnset_end_index )
        {
            snprintf ( strUciOpt, sizeof ( strUciOpt ), LTE_APN_PASSWORD, i );
            sys_get_config ( strUciOpt, uci_password, sizeof ( uci_password ) );
            CLOGD ( FINE, "%s -> [%s]\n", strUciOpt, uci_password );
        }

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "password%d", i );
        nv_set ( strRamOpt, uci_password );

        snprintf ( at_cmd, sizeof ( at_cmd ), "AT^NDISDUP=%d,0", i );
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( at_cmd, 5000, at_rep, sizeof ( at_rep ) );

        memset ( at_rep, 0, sizeof ( at_rep ) );
        if ( 0 == strcmp ( uci_apnenable, "1" ) )
        {
            snprintf ( at_cmd, sizeof ( at_cmd ), "AT+CGDCONT=%d,\"%s\",\"%s\"", i, uci_pdptype, uci_apnname );
            COMD_AT_PROCESS ( at_cmd, 3000, at_rep, sizeof ( at_rep ) );

            if ( 0 == strcmp ( uci_authtype, "0" ) )
                snprintf ( at_cmd, sizeof ( at_cmd ), "AT^AUTHDATA=%d,0", i );
            else
                snprintf ( at_cmd, sizeof ( at_cmd ), "AT^AUTHDATA=%d,%s,\"\",\"%s\",\"%s\"", i, uci_authtype, uci_password, uci_username );

            memset ( at_rep, 0, sizeof ( at_rep ) );
            COMD_AT_PROCESS ( at_cmd, 3000, at_rep, sizeof ( at_rep ) );
        }
        else
        {
            snprintf ( at_cmd, sizeof ( at_cmd ), "AT+CGDCONT=%d", i );
            COMD_AT_PROCESS ( at_cmd, 3000, at_rep, sizeof ( at_rep ) );

            snprintf ( at_cmd, sizeof ( at_cmd ), "AT^AUTHDATA=%d", i );
            memset ( at_rep, 0, sizeof ( at_rep ) );
            COMD_AT_PROCESS ( at_cmd, 3000, at_rep, sizeof ( at_rep ) );
        }
    }

    return 0;
}

int tdtekParam_updateDualSim ()
{
    char cur_sim_mode[4] = {0};
    char uci_sim_mode[4] = {0};
    char ram_val[16] = {0};
    char at_req[16] = {0};
    char at_rep[128] = {0};
    int i = 0;
    static int autosim_have_checked = 0;

    COMD_AT_PROCESS ( "AT+QUIMSLOT?", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "\r\n+QUIMSLOT: 1" ) )
    {
        strcpy ( cur_sim_mode, "0" );
        nv_set ( "sim_slot_used", "0" );
    }
    else if ( strstr ( at_rep, "\r\n+QUIMSLOT: 2" ) )
    {
        strcpy ( cur_sim_mode, "1" );
        nv_set ( "sim_slot_used", "1" );
    }
    else
    {
        return 0;
    }

    sys_get_config ( "lte_param.parameter.simslot", uci_sim_mode, sizeof ( uci_sim_mode ) );
    CLOGD ( FINE, "sim_slot uci value -> [%s]\n", uci_sim_mode );

    if ( 0 == strcmp ( uci_sim_mode, "2" ) )
    {
        if ( 1 == autosim_have_checked  )
        {
            return 0;
        }

        autosim_have_checked = 1;

        while ( i++ < 3 )
        {
            nv_get ( "cpin", ram_val, sizeof ( ram_val ) );
            if ( strcmp ( ram_val, "ERROR" ) )
            {
                return 0;
            }
            comd_wait_ms ( 3000 );
            tdtekParam_updatePinStatus ();
        }

        snprintf ( at_req, sizeof ( at_req ), "AT+QUIMSLOT=%s",
                                                    strcmp ( cur_sim_mode, "0" ) ? "1" : "2" );
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( at_req, 5000, at_rep, sizeof ( at_rep ) );

        if ( strstr ( at_rep, "\r\nOK\r\n" ) )
            return 1;
    }
    else
    {
        autosim_have_checked = 0;

        if ( strcmp ( uci_sim_mode, cur_sim_mode ) )
        {
            snprintf ( at_req, sizeof ( at_req ), "AT+QUIMSLOT=%s",
                                                        strcmp ( cur_sim_mode, "0" ) ? "1" : "2" );
            memset ( at_rep, 0, sizeof ( at_rep ) );
            COMD_AT_PROCESS ( at_req, 5000, at_rep, sizeof ( at_rep ) );

            if ( strstr ( at_rep, "\r\nOK\r\n" ) )
                return 1;
        }
    }

    return 0;
}

void tdtekParam_updatePinStatus ()
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+CPIN?", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "+CPIN:" ) || strstr ( at_rep, "ERROR" ) )
        parsing_tdtek_cpin_get ( at_rep );
}

void tdtekParam_updatePinLockStatus ()
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+CLCK=\"SC\",2", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "+CLCK:" ) )
    {
        parsing_tdtek_clck_get ( at_rep );
    }
    else if ( strstr ( at_rep, "+CME ERROR: SIM P" ) )
    {
        // if SIM already PIN or PUK, clck must have been enabled
        nv_set ( "lockpin_status", "1" );
    }
}

void tdtekParam_updatePinPukCount ()
{
    char at_rep[128] = {0};

    COMD_AT_PROCESS ( "AT^CPIN?", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "^CPIN:" ) )
        parsing_tdtek_cpinr ( at_rep );
}

void tdtekParam_updateImsi ()
{
    char at_rep[64] = {0};
    char ram_val[16] = {0};

    nv_get ( "imsi", ram_val, sizeof ( ram_val ) );

    if ( 0 == strcmp ( ram_val, "" ) || 0 == strcmp ( ram_val, "--" ) )
    {
        COMD_AT_PROCESS ( "AT+CIMI", 3000, at_rep, sizeof ( at_rep ) );

        if ( strstr ( at_rep, "\r\nOK\r\n" ) )
            parsing_tdtek_cimi ( at_rep );

        tdtekParam_updateUsimMncLen ();
        tdtekParam_setIpv6Format ( NULL );
    }
}

void tdtekParam_updateUsimMncLen ()
{
    char at_rep[128] = {0};

    COMD_AT_PROCESS ( "AT+CRSM=176,28589,0,0,4", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "+CRSM:" ) )
        parsing_tdtek_simMncLen ( at_rep );
}

void tdtekParam_updateSimSPN ()
{
    char at_rep[256] = {0};
    char ram_val[64] = {0};

    nv_get ( "SIM_SPN", ram_val, sizeof ( ram_val ) );

    if ( 0 == strcmp ( ram_val, "" ) || 0 == strcmp ( ram_val, "--" ) )
    {
        COMD_AT_PROCESS ( "AT+CRSM=176,28486,0,0,17", 3000, at_rep, sizeof ( at_rep ) );

        if ( strstr ( at_rep, "+CRSM:" ) )
            parsing_tdtek_sim_spn ( at_rep );
    }
}

void tdtekParam_updateIccid ()
{
    char at_rep[64] = {0};
    char ram_val[32] = {0};

    nv_get ( "iccid", ram_val, sizeof ( ram_val ) );

    if ( 0 == strcmp ( ram_val, "" ) || 0 == strcmp ( ram_val, "--" ) )
    {
        COMD_AT_PROCESS ( "AT^ICCID?", 3000, at_rep, sizeof ( at_rep ) );

        if ( strstr ( at_rep, "^ICCID:" ) )
            parsing_tdtek_iccid ( at_rep );
    }
}

void tdtekParam_updateCereg ()
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+CEREG?", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "\r\n+CEREG:" ) )
        parsing_tdtek_cereg ( at_rep );
}

void tdtekParam_updateC5greg ()
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+C5GREG?", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "\r\n+C5GREG:" ) )
        parsing_tdtek_c5greg (at_rep);
}

void tdtekParam_updateCireg ()
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+CIREG?", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "+CIREG:" ) )
        parsing_tdtek_cireg ( at_rep );
}

void tdtekParam_updateCsq ()
{
    char at_rep[128] = {0};

    COMD_AT_PROCESS ( "AT+CSQ", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "+CSQ:" ) )
        parsing_tdtek_csq ( at_rep );

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( "AT+CESQ", 3000, at_rep, sizeof ( at_rep ) );
}

void tdtekParam_updateCgatt ()
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+CGATT?", 3000, at_rep, sizeof ( at_rep ) );

    if ( strcmp ( at_rep, "" ) )
        parsing_tdtek_cgatt ( at_rep );
}

void tdtekParam_updateCgact ()
{
    char at_rep[512] = {0};

    COMD_AT_PROCESS ( "AT^DCONNSTAT?", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "\r\nOK\r\n" ) )
        parsing_tdtek_cgact ( at_rep );
}

int tdtekParam_configCgact ( int cid_index, int act_val )
{
    char at_req[16] = {0};
    char at_rep[128] = {0};

    snprintf ( at_req, sizeof ( at_req ), "AT^CHDATA=%d,%d", cid_index, cid_index + 1 );
    COMD_AT_PROCESS ( at_req, 3000, at_rep, sizeof ( at_rep ) );

    snprintf ( at_req, sizeof ( at_req ), "AT^NDISDUP=%d,%d", cid_index, act_val );
    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( at_req, 3000, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
        return -1;

    return 0;
}

void tdtekParam_updateCops ()
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+COPS?", 3000, at_rep, sizeof ( at_rep ) );

    if ( strcmp ( at_rep, "" ) )
        parsing_tdtek_operator ( at_rep );
}

void tdtekParam_updateCgcontrdp ( int apn_index )
{
    char at_cmd[32] = {0};
    char at_rep[RECV_BUF_SIZE] = {0};

    snprintf ( at_cmd, sizeof ( at_cmd ), "AT^DHCP=%d", apn_index );
    COMD_AT_PROCESS ( at_cmd, 3000, at_rep, sizeof ( at_rep ) );
    parsing_tdtek_dhcpv4_info ( apn_index, at_rep );

    snprintf ( at_cmd, sizeof ( at_cmd ), "AT^DHCPV6=%d", apn_index );
    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( at_cmd, 3000, at_rep, sizeof ( at_rep ) );
    parsing_tdtek_dhcpv6_info ( apn_index, at_rep );
}

void tdtekParam_updatePccSccInfo ( char* data )
{
    char at_rep[RECV_BUF_SIZE] = {0};
    char net_mode[8] = {0};

    COMD_AT_PROCESS ( "AT^RRCSTAT?", 3000, at_rep, sizeof ( at_rep ) );
    parsing_tdtek_rrc_state ( at_rep );

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( "AT^HFREQINFO?", 3000, at_rep, sizeof ( at_rep ) );
    parsing_tdtek_hfreqinfo ( at_rep );

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( "AT^MONSC", 3000, at_rep, sizeof ( at_rep ) );
    parsing_tdtek_serving_cellinfo ( at_rep );

    nv_get ( "mode", net_mode, sizeof ( net_mode) );

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( "AT^HCSQ?", 3000, at_rep, sizeof ( at_rep ) );
    parsing_tdtek_hcsq ( at_rep );

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( "AT^MCS=0", 3000, at_rep, sizeof ( at_rep ) );
    parsing_tdtek_ulmcs ( at_rep );

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( "AT^MCS=1", 3000, at_rep, sizeof ( at_rep ) );
    parsing_tdtek_dlmcs ( at_rep );

    memset ( at_rep, 0, sizeof ( at_rep ) );
    if ( 0 != strcmp ( net_mode, "5G" ))
    {
        COMD_AT_PROCESS ( "AT^TXPOWER?", 3000, at_rep, sizeof ( at_rep ) );
        parsing_tdtek_4g_txpower ( at_rep );
    }
    else
    {
        COMD_AT_PROCESS ( "AT^NTXPOWER?", 3000, at_rep, sizeof ( at_rep ) );
        parsing_tdtek_5g_txpower ( at_rep );
    }
}

void tdtekParam_updateServNeighInfo ( char* data )
{
    char at_rep[RECV_BUF_SIZE] = {0};

    COMD_AT_PROCESS ( "AT^MONNC", 3000, at_rep, sizeof ( at_rep ) );

    parsing_tdtek_neighbor_cellinfo ( at_rep );

    nv_set ( "search_neighbor_set", "0" );

    return;
}

int tdtekParam_setCfunMode ( char* mode )
{
    char cfunSet_ret[4] = {0};

    tdtekParam_radioOnOffSet ( mode );

    nv_get ( "setOnOff_lte", cfunSet_ret, sizeof ( cfunSet_ret ) );

    return strcmp ( cfunSet_ret, "0" ) ? -1 : 0;
}

int tdtekParam_updateDefaultGateway ( char* data )
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

static void tdtekParam_updateSignalBar ()
{
    char signal_s[8] = {0};
    int signal_i = 0;
    char ram_val[8] = {0};

#if 0
    nv_get ( "sinr", signal_s, sizeof ( signal_s ) );
    sscanf ( signal_s, "%d", &signal_i );

    if (signal_i < -3)
    {
        nv_set("signal_bar", (char *)"0");
    }
    else if (signal_i < 1)
    {
        nv_set("signal_bar", (char *)"1");
    }
    else if (signal_i < 7)
    {
        nv_set("signal_bar", (char *)"2");
    }
    else if (signal_i < 13)
    {
        nv_set("signal_bar", (char *)"3");
    }
    else
    {
        nv_set("signal_bar", (char *)"4");
    }
#else
    nv_get ( "mode", ram_val, sizeof ( ram_val ) );

    nv_get ( strcmp ( ram_val, "5G" ) ? "rsrp0" : "5g_rsrp0", signal_s, sizeof ( signal_s ) );

    sscanf ( signal_s, "%d", &signal_i );

    if ( signal_i <= -135 )
    {
        nv_set ( "signal_bar", "0" );
    }
    else if ( signal_i <= -120 )
    {
        nv_set ( "signal_bar", "1" );
    }
    else if ( signal_i <= -100 )
    {
        nv_set ( "signal_bar", "2" );
    }
    else if ( signal_i <= -30 )
    {
        if ( signal_i <= -85 || 0 == strncmp ( led_mode, "1", 1 ) )
        {
            nv_set ( "signal_bar", "3" );
        }
        else
        {
            nv_set ( "signal_bar", "4" );
        }
    }
    else
    {
        nv_set ( "signal_bar", "-1" );
    }
#endif
}

static int tdtekParam_initNetSelectMode ( char* data )
{
    char uci_conn_mode[4] = {0};

    sys_get_config ( WAN_LTE_SELECT_MODE, uci_conn_mode, sizeof ( uci_conn_mode ) );

    if ( 0 == strcmp ( uci_conn_mode, "0" ) )
    {
        manual_conn = 0;
        nv_set ( "net_select_mode", "manual_select" );
    }
    else
    {
        manual_conn = 1;
        nv_set ( "net_select_mode", "auto_select" );
    }

    return 0;
}

static void init_abnormal_check_config ()
{
    char conf_val[8] = {0};
    int i = 0;
    int conf_mask = 0;

    sys_get_config ( LTE_PARAMETER_ABNORMAL_CHECK, conf_val, sizeof ( conf_val ) );

    if ( strcmp ( conf_val, "" ) )
    {
        conf_mask = atoi ( conf_val );

        for ( i = 0; i < 5; i++ )
        {
            abnormal_check_flag[i] = ( conf_mask & ( 1 << i ) );
            CLOGD ( FINE, "abnormal_check_flag[%d]: [%d]\n", i, abnormal_check_flag[i] );
        }
    }
    else
    {
        for ( i = 0; i < 5; i++ )
        {
            abnormal_check_flag[i] = 1;
            CLOGD ( FINE, "abnormal_check_flag[%d]: [%d]\n", i, abnormal_check_flag[i] );
        }
    }
}

static void tdtek_init_apn_netdev ()
{
#if defined(_MTK_7621_) || defined(_MTK_7981_) || defined(_MTK_7628_)
    nv_set ( "apn1_netdev", "eth1.100" );

    nv_set ( "apn2_netdev", "eth1.101" );

    nv_set ( "apn3_netdev", "eth1.102" );

    nv_set ( "apn4_netdev", "eth1.103" );
#else
    nv_set ( "apn1_netdev", "eth_x1" );
    system ( "ifconfig eth_x1 -arp" );

    nv_set ( "apn2_netdev", "eth_x2" );
    system ( "ifconfig eth_x2 -arp" );

    nv_set ( "apn3_netdev", "eth_x3" );
    system ( "ifconfig eth_x3 -arp" );

    nv_set ( "apn4_netdev", "eth_x4" );
    system ( "ifconfig eth_x4 -arp" );
#endif
}

int tdtek_module_param_init ()
{
    tdtek_init_apn_netdev ();
    reset_apns_msg_flag ( 0 );
    init_abnormal_check_config ();

    cpin_error_check = 0;
    main_status_check = 0;
    abnormal_start_time = 0;
    dial_status_check = 0;

    if (
        tdtekParam_setATEcho ( NULL ) ||
        tdtekParam_setNetnum ( NULL ) ||
        tdtekParam_setCfunMode ( "off" ) ||
        tdtekParam_updateATI ( NULL ) ||
        tdtekParam_updateDefaultGateway ( NULL ) ||
        tdtekParam_updateSuppBands ( NULL ) ||
        tdtekParam_initAPNsetting ( NULL ) ||
        tdtekParam_radioLockInit ( NULL ) ||
        tdtekParam_setRatPriority ( NULL ) ||
        tdtekParam_lteRoamingSet ( NULL ) ||
        tdtekParam_setCfunMode ( "on" ) ||
        tdtekParam_initNetSelectMode ( NULL )
      )
        return -1;

    return 0;
}

static int abnormal_more_than_xxx_seconds ( int check_times, int xxx )
{
    struct sysinfo info;

    sysinfo ( &info );

    if ( 0 == check_times || 0 == abnormal_start_time )
    {
        abnormal_start_time = info.uptime;
    }
    else if ( xxx < ( info.uptime - abnormal_start_time ) )
    {
        CLOGD ( ERROR, "abnormal have lasted more than %d seconds\n", xxx );
        return 1;
    }

    return 0;
}

static int tdtekParam_checkAbnormal ( char* sim_stat, char* main_stat )
{
    int ret = 1;
    char radio_stat[8] = {0};
    char net_select_mode[32] = {0};
    char sim_lock_status[8] = {0};

    nv_get ( "simlocked", sim_lock_status, sizeof ( sim_lock_status ) );
    nv_get ( "lte_on_off", radio_stat, sizeof ( radio_stat ) );
    nv_get ( "net_select_mode", net_select_mode, sizeof ( net_select_mode ) );

    CLOGD ( FINE, "sim_stat: [%s], main_stat: [%s]\n", sim_stat, main_stat );
    CLOGD ( FINE, "sim_lock_status   -> [%s]\n", sim_lock_status );
    CLOGD ( FINE, "cpin_error_check   -> [%d]\n", cpin_error_check );
    CLOGD ( FINE, "main_status_check  -> [%d]\n", main_status_check );
    CLOGD ( FINE, "dial_status_check  -> [%d]\n", dial_status_check );
    CLOGD ( FINE, "lte_on_off         -> [%s]\n", radio_stat );
    CLOGD ( FINE, "net_select_mode    -> [%s]\n", net_select_mode );

    if ( 0 == strcmp ( radio_stat, "off" ) || 0 == strcmp ( net_select_mode, "manual_select" ) )
    {
        cpin_error_check = 0;
        main_status_check = 0;
        dial_status_check = 0;
        return 0;
    }

    if ( 0 == strcmp ( sim_stat, "ERROR" ) )
    {
        if ( abnormal_more_than_xxx_seconds ( cpin_error_check, 120 ) || cpin_error_check++ > 90 )
        {
            CLOGD ( FINE, "abnormal_check_flag[0]: [%d]\n", abnormal_check_flag[0] );
            if ( abnormal_check_flag[0] )
            {
                tdtekParam_radioRestart ();
                CLOGD ( ERROR, "exe AT+CFUN=0/1 for sim_not_ready !!!\n" );
            }
            cpin_error_check = 0;
            ret = -1;
        }

        main_status_check = 0;
        dial_status_check = 0;
        return ret;
    }
    else
    {
        cpin_error_check = 0;
    }

    if ( 0 == strcmp ( sim_stat, "READY" ) && strcmp ( main_stat, "connected" ) && atoi(sim_lock_status) == 0)
    {
        if ( abnormal_more_than_xxx_seconds ( main_status_check, 120 ) || main_status_check++ > 90 )
        {
            CLOGD ( FINE, "abnormal_check_flag[1]: [%d]\n", abnormal_check_flag[1] );
            if ( abnormal_check_flag[1] )
            {
                tdtekParam_radioRestart ();
                CLOGD ( ERROR, "exe AT+CFUN=0/1 for main_status not connected !!!\n" );
            }
            main_status_check = 0;
            ret = -1;
        }

        dial_status_check = 0;
        return ret;
    }
    else
    {
        main_status_check = 0;
    }

    char apn1_state[16] = {0};
    char apn1_ipv6_state[16] = {0};

    nv_get ( "apn1_state", apn1_state, sizeof ( apn1_state ) );
    nv_get ( "apn1_ipv6_state", apn1_ipv6_state, sizeof ( apn1_ipv6_state ) );

    if ( 0 == strcmp ( main_stat, "connected" ) && strcmp ( apn1_state, "connect" ) && strcmp ( apn1_ipv6_state, "connect" ) )
    {
        if ( abnormal_more_than_xxx_seconds ( dial_status_check, 60 ) || dial_status_check++ > 40 )
        {
            CLOGD ( FINE, "abnormal_check_flag[1]: [%d]\n", abnormal_check_flag[2] );
            if ( abnormal_check_flag[2] )
            {
                tdtekParam_radioRestart ();
                CLOGD ( ERROR, "exe AT+CFUN=0/1 for main_status not connected !!!\n" );
            }
            dial_status_check = 0;
            ret = -1;
        }

        return ret;
    }
    else
    {
        dial_status_check = 0;
    }

    return 0;
}

void tdtek_normal_status_refresh()
{
    char cpin_value[16] = {0};
    char mStatusVal[32] = {0};
    char csq_value[32] = {0};
    char cgatt_value[4] = {0};
    char c5greg_stat[4] = {0};
    char time_str[16] = {0};
    char apn_enable[4] = {0};
    char strUciOpt[64] = {0};
    char apn_act_state[4] = {0};
    char strRamOpt[64] = {0};
    int i = 0;
    char radio_stat[8] = {0};
    char simlock_stat[4] = {0};
    char ram_conn_mode[16] = {0};

    int cur_cfun_state = 0;
    char at_rep[32] = {0};

    tdtekParam_updatePinStatus ();
    tdtekParam_updatePinLockStatus ();
    tdtekParam_updatePinPukCount ();

    if ( 0 == strncmp ( dualsim_en, "1", 1 ) && 1 == tdtekParam_updateDualSim () )
    {
        comd_wait_ms ( 10 * 1000 );
        tdtekParam_initAPNsetting ( NULL );
        tdtekParam_radioLockInit ( "restore_fullband" );
        nv_set ( "imsi", "--" );
        return;
    }

    nv_get ( "cpin", cpin_value, sizeof ( cpin_value ) );
    nv_get ( "main_status", mStatusVal, sizeof ( mStatusVal ) );

    tdtekParam_checkAbnormal ( cpin_value, mStatusVal );

    if ( 0 == strcmp ( cpin_value, "READY" ) )
    {
        tdtekParam_updateImsi ();
        tdtekParam_updateSimSPN ();
        tdtekParam_updateIccid ();
        tdtekParam_getSimlock ( NULL );

        nv_get ( "simlocked", simlock_stat, sizeof ( simlock_stat ) );
        nv_get ( "lte_on_off", radio_stat, sizeof ( radio_stat ) );
        nv_get ( "net_select_mode", ram_conn_mode, sizeof ( ram_conn_mode ) );
        nv_get ( "setOnOff_lte", strRamOpt, sizeof ( strRamOpt ) );

        COMD_AT_PROCESS ( "AT+CFUN?", 3000, at_rep, sizeof ( at_rep ) );

        cur_cfun_state = parsing_tdtek_cfun_get(at_rep);

        if ( strncmp(strRamOpt,"-1",2) != 0 && strcmp ( simlock_stat, "1" ) == 0 && cur_cfun_state == 1)
        {
            COMD_AT_PROCESS ( "AT+CFUN=4", 3000, at_rep, sizeof ( at_rep ) );
        }
        else if ( strncmp(strRamOpt,"-1",2) != 0 && strcmp ( simlock_stat, "0" ) == 0 && cur_cfun_state == 0 && strcmp ( radio_stat, "on" ) == 0 )
        {
            COMD_AT_PROCESS ( "AT+CFUN=1", 3000, at_rep, sizeof ( at_rep ) );
        }


        tdtekParam_updateCsq ();
        nv_get ( "csq", csq_value, sizeof ( csq_value ) );

        if ( strcmp ( csq_value, "" ) && strcmp ( csq_value, "99,99" ) )
        {
            tdtekParam_updateCereg ();
C5GREG_LOOP:
            tdtekParam_updateCops ();
            tdtekParam_updatePccSccInfo ( NULL );
            tdtekParam_updateSignalBar ();
            tdtekParam_updateCgatt ();
            tdtekParam_updateCGDCONT ( "no_parse" );

            nv_get ( "cgatt_val", cgatt_value, sizeof ( cgatt_value ) );
            CLOGD ( FINE, "cgatt_value -> [%s]\n", cgatt_value );
            if ( 0 == strcmp ( cgatt_value, "1" ) )
            {
                if ( 0 == strcmp ( volte_enable, "VOLTE" ) )
                {
                    tdtekParam_updateCireg ();
                }

                if ( 0 == strcmp ( simlock_stat, "1" ) )
                {
                    tdtekParam_disconnectNetwork ( NULL );
                    return;
                }

                nv_set ( "main_status", "connected" );
                apnActOrDeactMsgEvent ( 0, 1 );

                nv_get ( "connected_time", time_str, sizeof( time_str ) );
                CLOGD ( FINE, "connected_time -> [%s]\n", time_str );
                if ( 0 == strcmp ( time_str, "0" ) )
                {
                    struct sysinfo info;
                    sysinfo ( &info );
                    snprintf ( time_str, sizeof ( time_str ), "%ld", info.uptime );
                    nv_set ( "connected_time", time_str );
                    CLOGD ( FINE, "update connected_time -> [%s]\n", time_str );
                }

                int multi_apn_act = 0;

                tdtekParam_updateCgact ();

                for ( i = 1; i <= 4; i++ )
                {
                    snprintf ( strUciOpt, sizeof ( strUciOpt ), "apn%d_enable", i );
                    memset ( apn_enable, 0, sizeof ( apn_enable ) );
                    nv_get ( strUciOpt, apn_enable, sizeof ( apn_enable ) );
                    CLOGD ( FINE, "%s -> [%s]\n", strUciOpt, apn_enable );

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "cid_%d_state", i );
                    memset ( apn_act_state, 0, sizeof ( apn_act_state ) );
                    nv_get ( strRamOpt, apn_act_state, sizeof ( apn_act_state ) );
                    CLOGD ( FINE, "%s -> [%s]\n", strRamOpt, apn_act_state );

                    if ( 0 == strcmp ( apn_act_state, "1" ) )
                    {
                        if ( 0 == strcmp ( apn_enable, "1" ) )
                        {
                            tdtekParam_updateCgcontrdp ( i );
                            apnActOrDeactMsgEvent ( i, 1 );
                        }
                        else
                        {
                            tdtekParam_configCgact ( i, 0 );
                        }
                    }
                    else
                    {
                        parsing_tdtek_dhcpv4_info ( i, "" );
                        parsing_tdtek_dhcpv6_info ( i, "" );
                        apnActOrDeactMsgEvent ( i, 0 );
                        if ( 0 == strcmp ( apn_enable, "1" ) )
                        {
                            if ( 0 == multi_apn_act )
                            {
                                comd_wait_ms ( 1500 );
                                multi_apn_act = 1;
                                tdtekParam_configCgact ( i, 1 );
                            }
                        }
                    }
                }
            }
            else
            {
                if ( 0 == strcmp ( simlock_stat, "0" ) &&
                    strcmp ( radio_stat, "off" ) &&
                    strcmp ( ram_conn_mode, "manual_select" ) )
                {
                    tdtekParam_connectNetwork ( NULL );
                }

                apnActOrDeactMsgEvent ( 0, 0 );
                if ( strcmp ( mStatusVal, "disconnected" ) )
                {
                    nv_set ( "main_status", "disconnected" );
                    goto END_S;
                }
            }
        }
        else
        {
            tdtekParam_updateC5greg ();
            nv_get ( "c5greg_stat", c5greg_stat, sizeof ( c5greg_stat ) );
            if ( 0 == strcmp ( c5greg_stat, "1" ) || 0 == strcmp ( c5greg_stat, "5" ) )
            {
                goto C5GREG_LOOP;
            }

            if ( 0 == strcmp ( simlock_stat, "0" ) &&
                strcmp ( radio_stat, "off" ) &&
                strcmp ( ram_conn_mode, "manual_select" ) )
            {
                tdtekParam_connectNetwork ( NULL );
            }

            apnActOrDeactMsgEvent ( 0, 0 );
            if(strcmp ( ram_conn_mode, "manual_select" ) == 0)
            {
                if ( strcmp ( mStatusVal, "disconnected" ) )
                {
                    nv_set ( "main_status", "disconnected" );
                    goto END;
                }
            }
            else if(strcmp ( radio_stat, "off" ) == 0)
            {
                if ( strcmp ( mStatusVal, "no_service" ) )
                {
                    nv_set ( "main_status", "no_service" );
                    goto END;
                }
            }
            else
            {
                if ( strcmp ( mStatusVal, "searching" ) )
                {
                    nv_set ( "main_status", "searching" );
                    goto END;
                }
            }
        }
    }
    else if ( 0 == strcmp ( cpin_value, "SIM PIN" ) )
    {
        apnActOrDeactMsgEvent ( 0, 0 );
        if ( strcmp ( mStatusVal, "need_pin" ) )
        {
            nv_set ( "main_status", "need_pin" );
            goto END;
        }
    }
    else if ( 0 == strcmp ( cpin_value, "SIM PUK" ) )
    {
        apnActOrDeactMsgEvent ( 0, 0 );
        if ( strcmp ( mStatusVal, "need_puk" ) )
        {
            nv_set ( "main_status", "need_puk" );
            goto END;
        }
    }
    else
    {
        apnActOrDeactMsgEvent ( 0, 0 );
        if ( strcmp ( mStatusVal, "sim_not_ready" ) )
        {
            nv_set ( "main_status", "sim_not_ready" );
            nv_set ( "imsi", "--" );
            nv_set ( "iccid", "--" );
            nv_set ( "SIM_SPN", "--" );
            nv_set ( "phone_number", "--" );
            goto END;
        }
    }

    return;

END:
    lteinfo_data_restore ();

END_S:
    nv_set ( "connected_time",  "0" );
    nv_set ( "cereg_stat",      "0" );
    nv_set ( "cgatt_val",       "0" );
    nv_set ( "cid_1_state",     "0" );
    nv_set ( "cid_2_state",     "0" );
    nv_set ( "cid_3_state",     "0" );
    nv_set ( "cid_4_state",     "0" );

    if ( 0 == strcmp ( volte_enable, "VOLTE" ) )
    {
        system ( "echo Unregistered > /tmp/voice/voice-reg-status.txt" );
    }

    ipv4v6_data_restore ( 0 );
}

void tdtek_install_mode_refresh ()
{
    return;
}

