#include "comd_share.h"
#include "comd_sms.h"
#include "atchannel.h"
#include "unisoc_status_refresh.h"
#include "unisoc_atcmd_parsing.h"
#include "unisoc_config_response.h"
#include "config_key.h"
#include "hal_platform.h"

static int abnormal_check_flag[5] = { 1, 1, 1, 1, 1 };
static int cpin_error_check = 0;
static int main_status_check = 0;
static int ping_status_check = 0;
static long abnormal_start_time = 0;
static int g_simslot_switch_rec = 0;
static int g_simslot_switch_event = 0;
static int param_init_done = 0;

extern char led_mode[4];
extern char volte_enable[8];
extern char dualsim_en[4];
extern char esim_supp[4];

static void unisocParam_radioRestart ()
{
    char at_rep[512] = {0};

    COMD_AT_PROCESS ( "AT+CFUN=0", 10000, at_rep, sizeof ( at_rep ) );

    comd_wait_ms ( 1500 );

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( "AT+CFUN=1", 10000, at_rep, sizeof ( at_rep ) );
}

int unisocParam_setATEcho ( char* data )
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "ATE0", 3000, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "OK" ) )
        return -1;

    return 0;
}

int unisocParam_setCfunMode ( char* mode )
{
    char cfunSet_ret[4] = {0};

    unisocParam_radioOnOffSet ( mode );

    nv_get ( "setOnOff_lte", cfunSet_ret, sizeof ( cfunSet_ret ) );

    return strcmp ( cfunSet_ret, "0" ) ? -1 : 0;
}

int unisocParam_updateModuleModel ( char* data )
{
    unsigned int check_times = 0;
    char at_rep[128] = {0};

    while ( check_times++ < 5 )
    {
        if ( 0 == COMD_AT_PROCESS ( "ATI", 1500, at_rep, sizeof ( at_rep ) ) )
        {
            if ( 0 == parsing_unisoc_moduleModel ( at_rep ) )
            {
                return 0;
            }
        }
        comd_wait_ms ( 1000 );
        memset ( at_rep, 0, sizeof ( at_rep ) );
    }

    return 0; // do not return -1, even can not get module info
}

int unisocParam_updateModuleVersion ( char* data )
{
    unsigned int check_times = 0;
    char at_rep[128] = {0};

    while ( check_times++ < 5 )
    {
        if ( 0 == COMD_AT_PROCESS ( "AT+QGMR", 1500, at_rep, sizeof ( at_rep ) ) )
        {
            if ( 0 == parsing_unisoc_moduleVersion ( at_rep ) )
            {
                return 0;
            }
        }
        comd_wait_ms ( 1000 );
        memset ( at_rep, 0, sizeof ( at_rep ) );
    }

    return 0;
}

int unisocParam_updateImei ( char* data )
{
    unsigned int check_times = 0;
    char imei_val[32] = {0};
    char at_rep[STR_AT_RSP_LEN] = {0};

    while ( check_times++ < 5 )
    {
        if ( 0 == COMD_AT_PROCESS ( "AT+GSN", 1500, at_rep, sizeof ( at_rep ) ) )
        {
            parsing_unisoc_imei ( at_rep );
            nv_get ( "imei", imei_val, sizeof ( imei_val ) );
            if ( 15 <= strlen ( imei_val ) )
            {
                return 0;
            }
            memset ( imei_val, 0, sizeof ( imei_val ) );
        }
        comd_wait_ms ( 1000 );
        memset ( at_rep, 0, sizeof ( at_rep ) );
    }

    return 0; // do not return -1, even can not get imei
}

int unisocParam_updateSN ( char* data )
{
    unsigned int check_times = 0;
    char sn_val[32] = {0};
    char at_rep[STR_AT_RSP_LEN] = {0};

    while ( check_times++ < 5 )
    {
        if (0 == COMD_AT_PROCESS ( "AT+EGMR=0,5", 1500, at_rep, sizeof ( at_rep ) ) )
        {
            parsing_unisoc_sn ( at_rep );
            nv_get ( "SN", sn_val, sizeof ( sn_val ) );
            if ( strcmp ( sn_val, "" ) )
                return 0;
            memset ( sn_val, 0, sizeof ( sn_val ) );
        }
        comd_wait_ms ( 1000 );
        memset ( at_rep, 0, sizeof ( at_rep ) );
    }

    return 0; // do not return -1, even can not get sn
}

int unisocParam_updateNat ( char* data )
{
#if defined(_MTK_7621_) || defined(_MTK_7981_) || defined(_MTK_7628_)
    char at_rep[64] = {0};
    char modulemodel[16] = {0};
    int modem_reboot_flag = 0;

    nv_get ( "modulemodel", modulemodel, sizeof ( modulemodel ) );

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( "AT+QCFG=\"nat\"", 3000, at_rep, sizeof ( at_rep ) );
    if ( strstr ( modulemodel, "RM500U" ) || strstr ( modulemodel, "RG500U" ) )
    {
        if ( NULL == strstr ( at_rep, "+QCFG: \"nat\",0" ) )
        {
            memset ( at_rep, 0, sizeof ( at_rep ) );
            COMD_AT_PROCESS ( "AT+QCFG=\"nat\",0", 3000, at_rep, sizeof ( at_rep ) );
            modem_reboot_flag = 1;
        }
    }

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( "AT+QCFG=\"usbnet\"", 3000, at_rep, sizeof ( at_rep ) );
    if ( strstr ( modulemodel, "RM500U" ) || strstr ( modulemodel, "RG500U" ) )
    {
        if ( NULL == strstr ( at_rep, "+QCFG: \"usbnet\",5" ) )
        {
            memset ( at_rep, 0, sizeof ( at_rep ) );
            COMD_AT_PROCESS ( "AT+QCFG=\"usbnet\",5", 3000, at_rep, sizeof ( at_rep ) );
            modem_reboot_flag = 1;
        }
    }

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( "AT+QCFG=\"vlan\"", 3000, at_rep, sizeof ( at_rep ) );
    if ( strstr ( modulemodel, "RM500U" ) || strstr ( modulemodel, "RG500U" ) )
    {
        if ( NULL == strstr ( at_rep, "+QCFG: \"vlan\",1" ) )
        {
            memset ( at_rep, 0, sizeof ( at_rep ) );
            COMD_AT_PROCESS ( "AT+QCFG=\"vlan\",1", 3000, at_rep, sizeof ( at_rep ) );
            modem_reboot_flag = 1;
        }
    }

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( "AT+QCFG=\"proxyarp\"", 3000, at_rep, sizeof ( at_rep ) );
    if ( strstr ( modulemodel, "RM500U" ) || strstr ( modulemodel, "RG500U" ) )
    {
        if ( NULL == strstr ( at_rep, "+QCFG: \"proxyarp\",1" ) )
        {
            memset ( at_rep, 0, sizeof ( at_rep ) );
            COMD_AT_PROCESS ( "AT+QCFG=\"proxyarp\",1", 3000, at_rep, sizeof ( at_rep ) );
            modem_reboot_flag = 1;
        }
    }

    if ( strstr ( modulemodel, "RM500U" ) || strstr ( modulemodel, "RG500U" ) )
    {
        if ( 1 == modem_reboot_flag )
        {
            memset ( at_rep, 0, sizeof ( at_rep ) );
            COMD_AT_PROCESS ( "AT+CFUN=1,1", 3000, at_rep, sizeof ( at_rep ) );
            CLOGD ( ERROR, "exe AT+CFUN=1,1 for nat or usbnet setting!!!\n" );
            modem_reboot_flag = 0;
        }
    }
#endif

    return 0;
}

int unisocParam_setVolte ( char* data )
{
    char at_rep[64] = {0};
    char slic_type[16] = {0};

    if ( 0 != strcmp ( volte_enable, "VOLTE" ) )
    {
        CLOGD ( FINE, "This platform does not support VOLTE, exit !!!\n" );
        return 0;
    }

    comd_exec ( "/lib/factory_tool.sh show | grep ^CONFIG_HW_SLIC_TYPE | cut -d '=' -f 2 | tr -d '\r\n'", slic_type, sizeof ( slic_type ) );
    CLOGD ( FINE, "CONFIG_HW_SLIC_TYPE is %s\n", slic_type );

    memset ( at_rep, 0, sizeof( at_rep ) );
    COMD_AT_PROCESS ( "AT+QSLIC?", 3000, at_rep, sizeof ( at_rep ) );

    if ( 0 == strcmp ( slic_type, "Maxlinear" ) )
    {
        if ( NULL == strstr ( at_rep, "+QSLIC: 1,5" ) )
        {
            if ( strstr ( at_rep, "+QSLIC: 1,2" ) )
            {
                memset ( at_rep, 0, sizeof( at_rep ) );
                COMD_AT_PROCESS ( "AT+QSLIC=0,2", 3000, at_rep, sizeof ( at_rep ) );
            }

            memset ( at_rep, 0, sizeof( at_rep ) );
            COMD_AT_PROCESS ( "AT+QSLIC=1,5,2", 3000, at_rep, sizeof ( at_rep ) );
        }
    }
    else if ( 0 == strcmp ( slic_type, "SI32185" ) )
    {
        if ( NULL == strstr ( at_rep, "+QSLIC: 1,2" ) )
        {
            if ( strstr ( at_rep, "+QSLIC: 1,5" ) )
            {
                memset ( at_rep, 0, sizeof( at_rep ) );
                COMD_AT_PROCESS ( "AT+QSLIC=0,5", 3000, at_rep, sizeof ( at_rep ) );
            }

            memset ( at_rep, 0, sizeof( at_rep ) );
            COMD_AT_PROCESS ( "AT+QSLIC=1,2", 3000, at_rep, sizeof ( at_rep ) );
        }
    }

    return 0;
}

int unisocParam_updateWanif ( char* data )
{
    int i = 0;
    char ram_val[16] = {0};
    char apn_netdev[16] = {0};
    char wanif[16] = {0};

#if defined(_MTK_7621_) || defined(_MTK_7981_)
    snprintf ( wanif, sizeof ( wanif ), "%s", "eth1.10" );
#else
    snprintf ( wanif, sizeof ( wanif ), "%s", "sipa_eth" );
#endif

    for ( i = 1; i <= 4; i++ )
    {
        snprintf ( ram_val, sizeof ( ram_val ), "apn%d_netdev", i );
        snprintf ( apn_netdev, sizeof ( apn_netdev ), "%s%d", wanif, i - 1 );
        nv_set ( ram_val, apn_netdev );
    }

    return 0;
}

int unisocParam_setQnetdevctl ( char* data )
{
#if 0
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+QNETDEVCTL=1", 3000, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "+QNETDEVCTL: 1,3,1" ) )
    {
        memset ( at_rep, 0, sizeof( at_rep ) );
        COMD_AT_PROCESS ( "AT+QNETDEVCTL=1,3,1", 3000, at_rep, sizeof ( at_rep ) );

        if ( NULL == strstr ( at_rep, "OK" ) )
        {
            CLOGD ( WARNING, "set AT+QNETDEVCTL=1,3,1 failed !!!\n" );
            return -1;
        }
    }
#endif

    return 0;
}

int unisocParam_updateDefaultGateway ( char* data )
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

int unisocParam_updateSuppBands ( char* data )
{
    char module_type[32] = {0};
    char *substr = NULL;
    char at_rep[RECV_BUF_SIZE] = {0};
    char lte_bands[256] = {0};
    char nr5g_bands[256] = {0};

    char old_suppband[128] = {0};
    char old_suppband5g[128] = {0};

    char *stored_path = LTE_PARAMETER_SUPPORT_BAND;
    char *stored_path5g = LTE_PARAMETER_SUPPORT_BAND5G;

    nv_get ( "modulemodel", module_type, sizeof ( module_type ) );

    snprintf ( lte_bands, sizeof ( lte_bands ), "%s",
                            strstr ( module_type, "RG500U-EA" ) ? QUECTEL_RG500U_EA_4G_BANDS : (
                            strstr ( module_type, "RG500U-EB" ) ? QUECTEL_RG500U_EB_4G_BANDS : ""
                        )
                    );

    snprintf ( nr5g_bands, sizeof ( nr5g_bands ), "%s",
                            strstr ( module_type, "RG500U-EA" ) ? QUECTEL_RG500U_EA_5G_BANDS : (
                            strstr ( module_type, "RG500U-EB" ) ? QUECTEL_RG500U_EB_5G_BANDS : ""
                        )
                    );

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

        sys_get_config ( stored_path, old_suppband, sizeof ( old_suppband ) );
        sys_get_config ( stored_path5g, old_suppband5g, sizeof ( old_suppband5g ) );
        if ( strcmp ( old_suppband, lte_bands ) )
        {
            CLOGD ( WARNING, "Save supported bands to flash !\n" );
            sys_set_config ( stored_path, lte_bands );
            sys_commit_config ( "lte_param" );
        }
        if ( strcmp ( old_suppband5g, nr5g_bands ) )
        {
            CLOGD ( WARNING, "Save supported bands to flash !\n" );
            sys_set_config ( stored_path5g, nr5g_bands );
            sys_commit_config ( "lte_param" );
        }

        return 0;
    }

    COMD_AT_PROCESS ( "AT+QNWPREFCFG=?", 1500, at_rep, sizeof ( at_rep ) );
    substr = strtok ( at_rep, "\r\n" );
    while ( substr )
    {
        CLOGD ( FINE, "policy_band:\n[%s]\n\n", substr );
        if ( strstr ( substr, "+QNWPREFCFG: \"lte_band\"," ) )
        {
            snprintf ( lte_bands, sizeof ( lte_bands ), "%s", substr );
        }
        else if ( strstr ( substr, "+QNWPREFCFG: \"nr5g_band\"," ) )
        {
            snprintf ( nr5g_bands, sizeof ( nr5g_bands ), "%s", substr );
        }
        substr = strtok ( NULL, "\r\n" );
    }

    return ( parsing_unisoc_supp4gband ( lte_bands ) & parsing_unisoc_supp5gband ( nr5g_bands ) );
}

int unisocParam_initAPNsetting ( char* data )
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
            memset ( uci_profilename, 0, sizeof ( uci_profilename ) );
            sys_get_config ( strUciOpt, uci_profilename, sizeof ( uci_profilename ) );
            CLOGD ( FINE, "%s -> [%s]\n", strUciOpt, uci_profilename );
        }

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "profile_name%d", i );
        nv_set ( strRamOpt, uci_profilename );

        if ( apnset_start_index < apnset_end_index )
        {
            snprintf ( strUciOpt, sizeof ( strUciOpt ), LTE_APN_PDPTYPE, i );
            memset ( uci_pdptype, 0, sizeof ( uci_pdptype ) );
            sys_get_config ( strUciOpt, uci_pdptype, sizeof ( uci_pdptype ) );
            CLOGD ( FINE, "%s -> [%s]\n", strUciOpt, uci_pdptype );
        }

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "pdn%d_type", i );
        nv_set ( strRamOpt, strcmp ( uci_pdptype, "IP" ) ? ( strcmp ( uci_pdptype, "IPV6" ) ? "2" : "1" ) : "0" );

        if ( apnset_start_index < apnset_end_index )
        {
            snprintf ( strUciOpt, sizeof ( strUciOpt ), LTE_APN_APNNAME, i );
            memset ( uci_apnname, 0, sizeof ( uci_apnname ) );
            sys_get_config ( strUciOpt, uci_apnname, sizeof ( uci_apnname ) );
            CLOGD ( FINE, "%s -> [%s]\n", strUciOpt, uci_apnname );
        }

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_name", i );
        nv_set ( strRamOpt, uci_apnname );

        if ( apnset_start_index < apnset_end_index )
        {
            snprintf ( strUciOpt, sizeof ( strUciOpt ), LTE_APN_ENABLE, i );
            memset ( uci_apnenable, 0, sizeof ( uci_apnenable ) );
            sys_get_config ( strUciOpt, uci_apnenable, sizeof ( uci_apnenable ) );
            CLOGD ( FINE, "%s -> [%s]\n", strUciOpt, uci_apnenable );
        }

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_enable", i );
        nv_set ( strRamOpt, uci_apnenable );

        if ( apnset_start_index < apnset_end_index )
        {
            snprintf ( strUciOpt, sizeof ( strUciOpt ), LTE_APN_AUTH_TYPE, i );
            memset ( uci_authtype, 0, sizeof ( uci_authtype ) );
            sys_get_config ( strUciOpt, uci_authtype, sizeof ( uci_authtype ) );
            CLOGD ( FINE, "%s -> [%s]\n", strUciOpt, uci_authtype );
            strcpy ( uci_authtype, strcmp ( uci_authtype, "CHAP" ) ? ( strcmp ( uci_authtype, "PAP" ) ? "0" : "1" ) : "2" );
        }

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "auth_type%d", i );
        nv_set ( strRamOpt, uci_authtype );

        if ( apnset_start_index < apnset_end_index )
        {
            snprintf ( strUciOpt, sizeof ( strUciOpt ), LTE_APN_USERNAME, i );
            memset ( uci_username, 0, sizeof ( uci_username ) );
            sys_get_config ( strUciOpt, uci_username, sizeof ( uci_username ) );
            CLOGD ( FINE, "%s -> [%s]\n", strUciOpt, uci_username );
        }

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "user_name%d", i );
        nv_set ( strRamOpt, uci_username );

        if ( apnset_start_index < apnset_end_index )
        {
            snprintf ( strUciOpt, sizeof ( strUciOpt ), LTE_APN_PASSWORD, i );
            memset ( uci_password, 0, sizeof ( uci_password ) );
            sys_get_config ( strUciOpt, uci_password, sizeof ( uci_password ) );
            CLOGD ( FINE, "%s -> [%s]\n", strUciOpt, uci_password );
        }

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "password%d", i );
        nv_set ( strRamOpt, uci_password );

        memset ( at_rep, 0, sizeof ( at_rep ) );
        if ( 0 == strcmp ( uci_apnenable, "1" ) )
        {
            snprintf ( at_cmd, sizeof ( at_cmd ), "AT+QICSGP=%d,%s,\"%s\",\"%s\",\"%s\",%s",
                       i,
                       strcmp ( uci_pdptype, "IP" ) ? ( strcmp ( uci_pdptype, "IPV6" ) ? "3" : "2" ) : "1",
                       uci_apnname,
                       strcmp ( uci_authtype, "0" ) ? uci_username : "",
                       strcmp ( uci_authtype, "0" ) ? uci_password : "",
                       uci_authtype
                );
        }
        else
        {
            snprintf ( at_cmd, sizeof ( at_cmd ), "AT+CGDCONT=%d", i );
        }
        COMD_AT_PROCESS ( at_cmd, 3000, at_rep, sizeof ( at_rep ) );

        unisocParam_updateQnetdevctl ( i, 0 );
    }

    return 0;
}


int unisocParam_initNetSelectMode ( char* data )
{
    char m_select_mode[4] = {0};

    sys_get_config ( WAN_LTE_SELECT_MODE, m_select_mode, sizeof ( m_select_mode ) );

    if ( strcmp ( m_select_mode, "0" ) )
    {
        nv_set ( "net_select_mode", "auto_select" );
    }
    else
    {
        nv_set ( "net_select_mode", "manual_select" );
    }

    return 0;
}

int unisocParam_radioLockInit ( char* data )
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

        unisocParam_lockband ( uciLockedBand, 0 );
    }
    else if ( 0 == strcmp ( uciScanMode, "FREQ_LOCK" ) )
    {
        sys_get_config ( WAN_LTE_LOCK_FREQ, uciLockedFreq, sizeof ( uciLockedFreq ) );

        if ( 0 != strcmp ( uciLockedFreq, "" ) )
        {
            // "4G,41,40000;5G,28,156100,15;"
            CLOGD ( FINE, "locked_freq -> [%s]\n", uciLockedFreq );
            unisocParam_lockearfcn ( uciLockedFreq, 0 );
        }
    }
    else if ( 0 == strcmp ( uciScanMode, "PCI_LOCK" ) )
    {
        sys_get_config ( WAN_LTE_LOCK_PCI, uciLockedPci, sizeof ( uciLockedPci ) );

        if ( 0 != strcmp ( uciLockedPci, "" ) )
        {
            // "4G,41,40000,104;5G,28,156100,505,15;"
            CLOGD ( FINE, "locked_pci -> [%s]\n", uciLockedPci );
            unisocParam_lockpci ( uciLockedPci, 0 );
        }
    }
    else if ( NULL == data )
    {
        unisocParam_lockband ( "", 0 );
    }
    else if ( 0 == strcmp ( data, "restore_fullband" ) )
    {
        unisocParam_lockband ( "", 1 );
    }

    return 0;
}

void unisocParam_updatePinStatus ()
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+CPIN?", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "+CPIN:" ) || strstr ( at_rep, "ERROR" ) )
        parsing_unisoc_cpin_get ( at_rep );
}

void unisocParam_updatePinLockStatus ()
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+CLCK=\"SC\",2", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "+CLCK:" ) )
        parsing_unisoc_clck_get ( at_rep );
}

void unisocParam_updatePinPukCount ()
{
    char at_rep[256] = {0};

    COMD_AT_PROCESS ( "AT+QPINC?", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "+QPINC:" ) )
        parsing_unisoc_qpinc ( at_rep );
}

void unisocParam_updateUsimMncLen ()
{
    char at_rep[128] = {0};

    COMD_AT_PROCESS ( "AT+CRSM=176,28589,0,0,0", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "+CRSM:" ) )
        parsing_unisoc_simMncLen ( at_rep );
}

int unisocParam_setIpv6Format ( char* data )
{
    char at_rep[64] = {0};
    COMD_AT_PROCESS ( "AT+CGPIAF=1,1,0,0", 10000, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "OK" ) )
        return -1;

    return 0;
}

void unisocParam_updateImsi ()
{
    char at_rep[64] = {0};
    char ram_val[16] = {0};

    nv_get ( "imsi", ram_val, sizeof ( ram_val ) );

    if ( 0 == strcmp ( ram_val, "" ) || 0 == strcmp ( ram_val, "--" ) )
    {
        COMD_AT_PROCESS ( "AT+CIMI", 3000, at_rep, sizeof ( at_rep ) );

        if ( strcmp ( at_rep, "" ) )
            parsing_unisoc_cimi ( at_rep );

        unisocParam_updateUsimMncLen ();
        unisocParam_setIpv6Format ( NULL );
    }
}

void unisocParam_updateSimSPN ()
{
    char at_rep[128] = {0};
    char ram_val[64] = {0};

    nv_get ( "SIM_SPN", ram_val, sizeof ( ram_val ) );

    if ( 0 == strcmp ( ram_val, "" ) || 0 == strcmp ( ram_val, "--" ) )
    {
        COMD_AT_PROCESS ( "AT+CRSM=176,28486,0,0,17", 3000, at_rep, sizeof ( at_rep ) );

        if ( strstr ( at_rep, "+CRSM:" ) )
            parsing_unisoc_sim_spn ( at_rep );
    }
}

void unisocParam_updateIccid ()
{
    char at_rep[64] = {0};
    char ram_val[32] = {0};

    nv_get ( "iccid", ram_val, sizeof ( ram_val ) );

    if ( 0 == strcmp ( ram_val, "" ) || 0 == strcmp ( ram_val, "--" ) )
    {
        COMD_AT_PROCESS ( "AT+CRSM=176,12258,0,0,10", 1500, at_rep, sizeof ( at_rep ) );

        if ( strstr ( at_rep, "+CRSM:" ) )
            parsing_unisoc_iccid ( at_rep );
    }
}

void unisocParam_updatePhoneNumber ()
{
    char at_rep[64] = {0};
    char ram_val[32] = {0};

    nv_get ( "phone_number", ram_val, sizeof ( ram_val ) );

    if ( 0 == strcmp ( ram_val, "" ) || 0 == strcmp ( ram_val, "--" ) )
    {
        COMD_AT_PROCESS ( "AT+CNUM", 1500, at_rep, sizeof ( at_rep ) );

        if ( strcmp ( at_rep, "" ) )
            parsing_unisoc_cnum ( at_rep );
    }
}

void unisocParam_updateClcc ()
{
    char at_rep[RECV_BUF_SIZE * 2] = {0};
    char *substr = NULL;
    int ret = 0;
    char cgact_tmp[32] = {0};

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( "AT+CGDCONT?", 1500, at_rep, sizeof ( at_rep ) );
    if ( NULL == strstr ( at_rep, "ims" ) && NULL == strstr ( at_rep, "IMS" ) )
    {
        CLOGD ( FINE, "Unregistered\n" );
        system ( "echo Unregistered > /tmp/voice/voice-reg-status.txt" );
        return;
    }
    else
    {
        substr = strtok ( at_rep, "\r\n" );
        while ( substr )
        {
            CLOGD ( FINE, "[%s]\n\n", substr );
            ret = parsing_unisoc_cgdcont ( substr );
            if ( 0 != ret )
            {
                memset ( at_rep, 0, sizeof ( at_rep ) );
                COMD_AT_PROCESS ( "AT+CGACT?", 1500, at_rep, sizeof ( at_rep ) );
                snprintf ( cgact_tmp, sizeof ( cgact_tmp ), "+CGACT: %d,1", ret );
                if ( NULL == strstr ( at_rep, cgact_tmp ) )
                {
                    CLOGD ( FINE, "Unregistered\n" );
                    system ( "echo Unregistered > /tmp/voice/voice-reg-status.txt" );
                    return;
                }
                else
                {
                    CLOGD ( FINE, "Registered\n" );
                    system ( "echo Registered > /tmp/voice/voice-reg-status.txt" );
                }
            }
            substr = strtok ( NULL, "\r\n" );
        }
    }

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( "AT+CLCC", 1500, at_rep, sizeof ( at_rep ) );

    substr = strtok ( at_rep, "\r\n" );
    while ( substr )
    {
        CLOGD ( FINE, "[%s]\n\n", substr );
        ret = parsing_unisoc_clcc ( substr );
        if ( 1 == ret )
        {
            CLOGD ( FINE, "Ringing\n" );
            system ( "echo Ringing > /tmp/voice/voice-call-status.txt" );
            return;
        }
        else if ( 2 == ret )
        {
            CLOGD ( FINE, "Connect\n" );
            system ( "echo Connect > /tmp/voice/voice-call-status.txt" );
            return;
        }
        substr = strtok ( NULL, "\r\n" );
    }

    CLOGD ( FINE, "Idle\n" );
    system ( "echo Idle > /tmp/voice/voice-call-status.txt" );

    return;
}


void unisocParam_updateCesq ()
{
    char at_rep[STR_AT_RSP_LEN] = {0};

    COMD_AT_PROCESS ( "AT+CESQ", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "+CESQ:" ) )
        parsing_unisoc_cesq ( at_rep );
}

void unisocParam_updateCops ()
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+COPS?", 3000, at_rep, sizeof ( at_rep ) );
    if ( strcmp ( at_rep, "" ) )
        parsing_unisoc_operator ( at_rep );
}

void unisocParam_updateServingCell ()
{
    char at_rep[RECV_BUF_SIZE] = {0};

    COMD_AT_PROCESS ( "AT+QENG=\"servingcell\"", 3000, at_rep, sizeof ( at_rep ) );

    if ( strcmp ( at_rep, "" ) )
        parsing_unisoc_serving_cellinfo ( at_rep );
}

void unisocParam_updateMcs ()
{
    char at_rep[128] = {0};

    COMD_AT_PROCESS ( "AT+QNWCFG=\"lte_ulMCS\"", 3000, at_rep, sizeof ( at_rep ) );
    if ( strcmp ( at_rep, "" ) )
        parsing_unisoc_lte_ulmcs ( at_rep );

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( "AT+QNWCFG=\"lte_dlMCS\"", 3000, at_rep, sizeof ( at_rep ) );
    if ( strcmp ( at_rep, "" ) )
        parsing_unisoc_lte_dlmcs ( at_rep );

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( "AT+QNWCFG=\"nr5g_ulMCS\"", 3000, at_rep, sizeof ( at_rep ) );
    if ( strcmp ( at_rep, "" ) )
        parsing_unisoc_nr5g_ulmcs ( at_rep );

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( "AT+QNWCFG=\"nr5g_dlMCS\"", 3000, at_rep, sizeof ( at_rep ) );
    if ( strcmp ( at_rep, "" ) )
        parsing_unisoc_nr5g_dlmcs ( at_rep );
}

void unisocParam_updateQcainfo ()
{
    char at_rep[RECV_BUF_SIZE] = {0};
    int i = 0;
    char strRamOpt[32] = {0};
    char mode[8] = {0};

    nv_get ( "mode", mode, sizeof ( mode ) );
    CLOGD ( FINE, "mode : [%s]\n", mode );

    COMD_AT_PROCESS ( "AT+QCAINFO", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "SCC" ) )
    {
        if ( 0 == strcmp ( mode, "ENDC" ) )
        {
            parsing_unisoc_endc_qcainfo ( at_rep );
        }
        else
        {
            parsing_unisoc_qcainfo ( at_rep );
        }
    }
    else
    {
        nv_set ( "secondary_cell", "0" );

        for ( i = 1; i < 4; i++ )
        {
            snprintf ( strRamOpt, sizeof ( strRamOpt ), "S%d_dl_frequency", i );
            nv_set ( strRamOpt, "" );

            snprintf ( strRamOpt, sizeof ( strRamOpt ), "5g_S%d_dl_frequency", i );
            nv_set ( strRamOpt, "" );
        }
    }
}

void unisocParam_updateSignalBar ()
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

void unisocParam_updateCgatt ()
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+CGATT?", 3000, at_rep, sizeof ( at_rep ) );

    if ( strcmp ( at_rep, "" ) )
        parsing_unisoc_cgatt ( at_rep );
}

void unisocParam_updateCgact ()
{
    char at_rep[STR_AT_RSP_LEN] = {0};

    COMD_AT_PROCESS ( "AT+CGACT?", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "OK" ) )
        parsing_unisoc_cgact ( at_rep );
}

int unisocParam_updateQnetdevstatus ( int apn_index )
{
    char at_rep[RECV_BUF_SIZE] = {0};
    char at_cmd[32] = {0};

    snprintf ( at_cmd, sizeof ( at_cmd ), "AT+QNETDEVSTATUS=%d", apn_index );

    COMD_AT_PROCESS ( at_cmd, 8000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "OK" ) )
        return parsing_unisoc_qnetdevstatus ( apn_index, at_rep );

    if ( strstr ( at_rep, "ERROR" ) )
        unisocParam_updateQnetdevctl ( apn_index, 1 );

    return 0;
}

int unisocParam_updateQnetdevctl ( int apn_index, int act_val )
{
    char at_rep[RECV_BUF_SIZE] = {0};
    char at_cmd[32] = {0};

    snprintf ( at_cmd, sizeof ( at_cmd ), "AT+QNETDEVCTL=%d,%d,0", apn_index, act_val );

    COMD_AT_PROCESS ( at_cmd, 8000, at_rep, sizeof ( at_rep ) );

    return 0;
}

void unisocParam_updateCsiInfo( void )
{
    char at_rep[64] = {0};
    char ram_val[8] = {0};

    nv_get ( "mode", ram_val, sizeof ( ram_val ) );

    if( strcmp(ram_val,"5G") == 0 )
    {
        COMD_AT_PROCESS ( "AT+QNWCFG=\"nr5g_csi\"", 3000, at_rep, sizeof ( at_rep ) );
    }
    else
    {
        COMD_AT_PROCESS ( "AT+QNWCFG=\"lte_csi\"", 3000, at_rep, sizeof ( at_rep ) );
    }

    if ( strcmp ( at_rep, "" ) )
    {
        unisoc_quectel_csiinfo ( at_rep );
    }
}

int unisocParam_updateDualSim ( void )
{
    char cur_sim_mode[4] = {0};
    char uci_sim_slot[4] = {0};
    char auto_sim_slot[4] = {0};
    char use_esim[4] = {0};
    char ram_val[16] = {0};
    char at_req[16] = {0};
    char at_rep[128] = {0};
    int i = 0;

    nv_get ( "use_esim", use_esim, sizeof ( use_esim ) );

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

        if(strcmp(use_esim,"1") == 0)
        {
            g_simslot_switch_rec = 2;
        }
    }
    else
    {
        return 0;
    }

    sys_get_config ( "lte_param.parameter.simslot", uci_sim_slot, sizeof ( uci_sim_slot ) );
    sys_get_config ( "lte_param.parameter.auto_sim", auto_sim_slot, sizeof ( auto_sim_slot ) );
    CLOGD ( FINE, "sim_slot uci value -> [%s], g_simslot_switch_rec -> [%d]\n", uci_sim_slot, g_simslot_switch_rec );

    if ( 0 == strcmp ( auto_sim_slot, "1" ) )
    {
        while ( i++ < 8 )
        {
            nv_get ( "cpin", ram_val, sizeof ( ram_val ) );
            if ( strcmp ( ram_val, "ERROR" ) )
            {
                return 1;
            }
            comd_wait_ms ( 2000 );
            unisocParam_updatePinStatus ();
        }

        memset ( at_rep, 0, sizeof ( at_rep ) );

        if(strcmp ( cur_sim_mode, "0" ) == 0)
        {
            if(strcmp ( esim_supp, "1" ) == 0)
            {
                if(g_simslot_switch_rec == 0)
                {
                    system("/lib/simslot_switch.sh sim");
                    comd_wait_ms ( 1000 );
                    snprintf(at_req, sizeof ( at_req ), "AT+QUIMSLOT=2");
                    g_simslot_switch_rec = 1;
                }
                else if(g_simslot_switch_rec == 1)
                {
                    system("/lib/simslot_switch.sh esim");
                    comd_wait_ms ( 1000 );
                    snprintf(at_req, sizeof ( at_req ), "AT+QUIMSLOT=2");
                    g_simslot_switch_rec = 2;
                }
            }
            else
            {
                snprintf(at_req, sizeof ( at_req ), "AT+QUIMSLOT=2");
            }
        }
        else if(strcmp ( cur_sim_mode, "1" ) == 0)
        {
            if(strcmp ( esim_supp, "1" ) == 0)
            {
                if(g_simslot_switch_rec == 0)
                {
                    system("/lib/simslot_switch.sh sim");
                    comd_wait_ms ( 1000 );
                    snprintf(at_req, sizeof ( at_req ), "AT+QUIMSLOT=1");
                    g_simslot_switch_rec = 1;
                }
                else if(g_simslot_switch_rec == 1)
                {
                    system("/lib/simslot_switch.sh esim");
                    comd_wait_ms ( 1000 );
                    snprintf(at_req, sizeof ( at_req ), "AT+QUIMSLOT=2");
                    g_simslot_switch_rec = 2;
                }
                else if(g_simslot_switch_rec == 2)
                {
                    system("/lib/simslot_switch.sh sim");
                    comd_wait_ms ( 1000 );
                    snprintf(at_req, sizeof ( at_req ), "AT+QUIMSLOT=1");
                    g_simslot_switch_rec = 0;
                }
            }
            else
            {
                snprintf(at_req, sizeof ( at_req ), "AT+QUIMSLOT=1");
            }
        }

        COMD_AT_PROCESS ( at_req, 5000, at_rep, sizeof ( at_rep ) );
        g_simslot_switch_event = 1;
    }
    else
    {
        /*
        *   0 --> sim slot 1
        *   1 --> sim slot 2
        *   2 --> esim slot
        */

        memset ( at_req, 0, sizeof ( at_req ) );
        memset ( at_rep, 0, sizeof ( at_rep ) );
        switch(atoi(uci_sim_slot))
        {
            case 0:
                if(strncmp(cur_sim_mode,"0",1) != 0)
                {
                    snprintf(at_req, sizeof ( at_req ), "AT+QUIMSLOT=1");
                }
                break;
            case 1:
                if(strncmp(cur_sim_mode,"1",1) != 0 || strncmp(use_esim,"0",1) != 0)
                {
                    system("/lib/simslot_switch.sh sim");
                    comd_wait_ms ( 1000 );
                    snprintf(at_req, sizeof ( at_req ), "AT+QUIMSLOT=2");
                }
                break;
            case 2:
                if(strncmp(cur_sim_mode,"1",1) != 0 || strncmp(use_esim,"1",1) != 0)
                {
                    system("/lib/simslot_switch.sh esim");
                    comd_wait_ms ( 1000 );
                    snprintf(at_req, sizeof ( at_req ), "AT+QUIMSLOT=2");
                }
                break;
        }

        if(strlen(at_req) > 0)
        {
            COMD_AT_PROCESS ( at_req, 5000, at_rep, sizeof ( at_rep ) );
            g_simslot_switch_event = 1;
            if ( strstr ( at_rep, "\r\nOK\r\n" ) )
                return 1;
        }
    }

    return 0;
}

static int unisocParam_setSmsCmgf ( char* data )
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+CMGF=0", 1500, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
        return -1;

    return 0;
}

static int unisocParam_setSmsCnmi ( char* data )
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+CNMI=2,1,0,1,0", 1500, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
        return -1;

    return 0;
}

static int unisocParam_setSmsCpms ( char* data )
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+CPMS=\"ME\",\"ME\",\"ME\"", 1500, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
        return -1;

    return 0;
}

static int unisocParam_getSmsMaxNum ( char* data )
{
    char at_rep[128] = {0};

    COMD_AT_PROCESS ( "AT+CPMS?", 1500, at_rep, sizeof ( at_rep ) );

    parsing_unisoc_cpms_get ( at_rep );

    if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
        return -1;

    return 0;
}

static int unisocParam_setSmsCsmp ( char* data )
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+CSMP=0,255,0,8", 1500, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
        return -1;

    return 0;
}

static void unisocParam_initSmsSimCardMessage ()
{
    char at_rep[RECV_SMS_SIZE] = {0};

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( "AT+CMGL=4", 5000, at_rep, sizeof ( at_rep ) );

    if ( strcmp ( at_rep, "" ) )
        parsing_unisoc_cmgl_four_sim_card ( at_rep );
}

void unisocParam_getSimBookMaxNumber ( char* data )
{
    char at_rep[32] = {0};

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( "AT+CSCS=\"GSM\"", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "\r\nOK\r\n" ) )
    {
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( "AT+CPBS?", 3000, at_rep, sizeof ( at_rep ) );

        if( !strstr( at_rep, "CPBS: \"SM\"" ) )
        {
            COMD_AT_PROCESS ( "AT+CPBS=\"SM\"", 3000, at_rep, sizeof ( at_rep ) );
            memset ( at_rep, 0, sizeof ( at_rep ) );
            COMD_AT_PROCESS ( "AT+CPBS?", 3000, at_rep, sizeof ( at_rep ) );
        }

        if ( strcmp ( at_rep, "" ) )
        {
            parsing_unisoc_cpbs ( at_rep );
        }
    }
}

void unisocParam_updateCmglInfo ()
{

    char at_rep[RECV_SMS_SIZE] = {0};
    int ret = 0;
    char sms_all_number_buf[8] = {0};

    nv_get ( "all_sms_number", sms_all_number_buf, sizeof ( sms_all_number_buf) );
    if(strcmp(sms_all_number_buf,""))
    {
        if(atoi(sms_all_number_buf)>200)
            return;
    }

    COMD_AT_PROCESS ( "AT+CMGL=4", 5000, at_rep, sizeof ( at_rep ) );

    if ( strcmp ( at_rep, "" ) )
    {
        ret = parsing_unisoc_cmgl_zero ( at_rep );
    }

    memset(at_rep,0,sizeof(at_rep));

    if(ret == 0)
    {
        COMD_AT_PROCESS ( "AT+CMGD=1,4", 5000, at_rep, sizeof ( at_rep ) );
    }

    //long_message_table_check();
}

void unisocParam_updateSmsCenterNum ()
{
    char at_rep[64] = {0};
    char ram_val[32] = {0};

    nv_get ( "sms_center_num", ram_val, sizeof ( ram_val ) );

    if ( 0 == strcmp ( ram_val, "" ) || 0 == strcmp ( ram_val, "--" ) )
    {
        COMD_AT_PROCESS ( "AT+CSCA?", 3000, at_rep, sizeof ( at_rep ) );

        if ( strstr ( at_rep, "+CSCA:" ) )
            parsing_unisoc_csca_get ( at_rep );
    }
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

int unisoc_module_param_init ()
{
    reset_apns_msg_flag ( 0 );
    init_abnormal_check_config ();

    cpin_error_check = 0;
    main_status_check = 0;
    ping_status_check = 0;
    abnormal_start_time = 0;
    param_init_done = 0;
    if (
        unisocParam_setATEcho ( NULL ) ||
        unisocParam_setCfunMode ( "off" ) ||
        unisocParam_updateModuleModel ( NULL ) ||
        unisocParam_updateModuleVersion ( NULL ) ||
        unisocParam_updateImei ( NULL ) ||
        unisocParam_updateSN ( NULL ) ||
        unisocParam_updateNat ( NULL ) ||
        unisocParam_setVolte ( NULL ) ||
        unisocParam_updateWanif ( NULL ) ||
        unisocParam_setQnetdevctl ( NULL ) ||
        unisocParam_updateDefaultGateway ( NULL ) ||
        unisocParam_updateSuppBands ( NULL ) ||
        unisocParam_initAPNsetting ( NULL ) ||
        unisocParam_lteRoamingSet ( NULL ) ||
        unisocParam_initNetSelectMode ( NULL ) ||
        unisocParam_radioLockInit ( NULL ) ||
        unisocParam_setCfunMode ( "on" ) ||
        unisocParam_setRatPriority ( NULL )
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

static int unisocParam_checkAbnormal ( char* sim_stat, char* main_stat )
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
    CLOGD ( FINE, "ping_status_check  -> [%d]\n", ping_status_check );
    CLOGD ( FINE, "lte_on_off         -> [%s]\n", radio_stat );
    CLOGD ( FINE, "net_select_mode    -> [%s]\n", net_select_mode );

    if ( 0 == strcmp ( radio_stat, "off" ) || 0 == strcmp ( net_select_mode, "manual_select" ) )
    {
        cpin_error_check = 0;
        main_status_check = 0;
        ping_status_check = 0;
        return 0;
    }

    if ( 0 == strcmp ( sim_stat, "ERROR" ) )
    {
        if ( abnormal_more_than_xxx_seconds ( cpin_error_check, 120 ) || cpin_error_check++ > 90 )
        {
            CLOGD ( FINE, "abnormal_check_flag[0]: [%d]\n", abnormal_check_flag[0] );
            if ( abnormal_check_flag[0] )
            {
                unisocParam_radioRestart ();
                CLOGD ( ERROR, "exe AT+CFUN=0/1 for sim_not_ready !!!\n" );
            }
            cpin_error_check = 0;
            ret = -1;
        }

        main_status_check = 0;
        ping_status_check = 0;
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
                unisocParam_radioRestart ();
                CLOGD ( ERROR, "exe AT+CFUN=0/1 for main_status not connected !!!\n" );
            }
            main_status_check = 0;
            ret = -1;
        }

        ping_status_check = 0;
        return ret;
    }
    else
    {
        main_status_check = 0;
    }

    char ping_stat[8] = {0};
    char at_rep[64] = {0};

    nv_get ( "reachable", ping_stat, sizeof ( ping_stat ) );

    if ( 0 == strcmp ( ping_stat, "1" ) )
    {
        if ( abnormal_more_than_xxx_seconds ( ping_status_check, 60 ) || ping_status_check++ > 40 )
        {
            CLOGD ( FINE, "abnormal_check_flag[3]: [%d]\n", abnormal_check_flag[2] );
            if ( abnormal_check_flag[2] )
            {
                COMD_AT_PROCESS ( "AT+CFUN=4", 3000, at_rep, sizeof ( at_rep ) );
                comd_wait_ms ( 1500 );
                COMD_AT_PROCESS ( "AT+CFUN=1", 3000, at_rep, sizeof ( at_rep ) );

                CLOGD ( ERROR, "exe AT+CFUN=4/1 for ping_status not 0 !!!\n" );
            }
            ping_status_check = 0;
            ret = -1;
        }

        return ret;
    }
    else
    {
        ping_status_check = 0;
    }

    return 0;
}

void unisoc_normal_status_refresh ()
{
    char cpin_value[16] = {0};
    char mStatusVal[32] = {0};
    char cesq_value[64] = {0};
    char cgatt_value[4] = {0};
    char strUciOpt[64] = {0};
    char apn_enable[4] = {0};
    char time_str[16] = {0};
    char apn_act_state[4] = {0};
    char strRamOpt[64] = {0};
    int ret = 1;
    int i = 0;
    char simlock_stat[4] = {0};
    char radio_stat[8] = {0};
    char cur_select_mode[32] = {0};
    int cur_cfun_state = 0;
    char at_rep[32] = {0};
    char signal_value[8] = {0};

#if defined(_MTK_7981_)
    char socket_cmd[64] = {0};
#endif

    unisocParam_updatePinStatus ();
    unisocParam_updatePinLockStatus ();
    unisocParam_updatePinPukCount ();

    if ( 0 == strncmp ( dualsim_en, "1", 1 ) && unisocParam_updateDualSim () && g_simslot_switch_event == 1)
    {
        g_simslot_switch_event = 0;

        comd_wait_ms ( 10 * 1000 );
        unisocParam_initAPNsetting ( NULL );
        unisocParam_radioLockInit ( "restore_fullband" );
        nv_set ( "imsi", "--" );
        nv_set ( "SIM_SPN", "--" );
        return;
    }

    nv_get ( "cpin", cpin_value, sizeof ( cpin_value ) );
    nv_get ( "main_status", mStatusVal, sizeof ( mStatusVal ) );

#if defined(_MTK_7981_)
    snprintf ( socket_cmd, sizeof ( socket_cmd ), "ilte_mon \"main_status=%s\"", mStatusVal );
    CLOGD ( FINE, "socket_cmd -> [%s]\n", socket_cmd );
    system ( socket_cmd );
#endif

    unisocParam_checkAbnormal ( cpin_value, mStatusVal );

    if ( 0 == strcmp ( cpin_value, "READY" ) )
    {
        unisocParam_updateImsi ();
        unisocParam_updateSimSPN ();
        unisocParam_getSimlock ( NULL );
        unisocParam_updateIccid ();
        unisocParam_updatePhoneNumber ();

        if(CONFIG_SW_SMS == 1)
        {
            if(param_init_done == 0)
            {
                comd_wait_ms ( 5000 ); // wait for SIM really ready
                unisocParam_setSmsCmgf ( NULL );
                unisocParam_setSmsCnmi ( NULL );
                unisocParam_setSmsCpms ( NULL );
                unisocParam_getSmsMaxNum ( NULL );
                unisocParam_setSmsCsmp ( NULL );
                initSmsSqlite3 ();
                unisocParam_initSmsSimCardMessage ( NULL );
                unisocParam_getSimBookMaxNumber ( NULL );

                param_init_done = 1;
            }

            unisocParam_updateSmsCenterNum();
        }

        nv_get ( "simlocked", simlock_stat, sizeof ( simlock_stat ) );
        nv_get ( "lte_on_off", radio_stat, sizeof ( radio_stat ) );
        nv_get ( "net_select_mode", cur_select_mode, sizeof ( cur_select_mode ) );
        nv_get ( "signal_bar", signal_value, sizeof ( signal_value ) );
        nv_get ( "setOnOff_lte", strRamOpt, sizeof ( strRamOpt ) );
#if defined(_MTK_7981_)
        snprintf ( socket_cmd, sizeof ( socket_cmd ), "ilte_mon \"simlocked=%s\"", simlock_stat );
        CLOGD ( FINE, "socket_cmd -> [%s]\n", socket_cmd );
        system ( socket_cmd );

        snprintf ( socket_cmd, sizeof ( socket_cmd ), "ilte_mon \"signal_bar=%s\"", signal_value );
        CLOGD ( FINE, "socket_cmd -> [%s]\n", socket_cmd );
        system ( socket_cmd );
#endif

        COMD_AT_PROCESS ( "AT+CFUN?", 3000, at_rep, sizeof ( at_rep ) );

        cur_cfun_state = parsing_unisoc_cfun_get(at_rep);

        if ( strncmp(strRamOpt,"-1",2) != 0 && strcmp ( simlock_stat, "1" ) == 0 && cur_cfun_state == 1)
        {
            COMD_AT_PROCESS ( "AT+CFUN=4", 3000, at_rep, sizeof ( at_rep ) );
        }
        else if ( strncmp(strRamOpt,"-1",2) != 0 && strcmp ( simlock_stat, "0" ) == 0 && cur_cfun_state == 0 && strcmp ( radio_stat, "on" ) == 0 )
        {
            COMD_AT_PROCESS ( "AT+CFUN=1", 3000, at_rep, sizeof ( at_rep ) );
        }


        if ( 0 == strcmp ( volte_enable, "VOLTE" ) )
        {
            unisocParam_updateClcc ();
        }

        unisocParam_updateCesq ();
        nv_get ( "cesq", cesq_value, sizeof ( cesq_value ) );

        if ( strcmp ( cesq_value, "" ) && strcmp ( cesq_value, "99,99,255,255,255,255,255,255,255" ) )
        {
            unisocParam_updateCops ();
            unisocParam_updateServingCell ();
            unisocParam_updateSignalBar ();
            unisocParam_updateCgatt ();

            nv_get ( "cgatt_val", cgatt_value, sizeof ( cgatt_value ) );
            CLOGD ( FINE, "cgatt_value -> [%s]\n", cgatt_value );
            if ( 0 == strcmp ( cgatt_value, "1" ) )
            {
                if ( 0 == strcmp ( simlock_stat, "1" ) )
                {
                    unisocParam_disconnectNetwork ( NULL );
                    return;
                }
                unisocParam_updateCsiInfo();
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

                if ( 1 == CONFIG_SW_SMS )
                {
                    unisocParam_updateCmglInfo ();
                }

                unisocParam_updateMcs ();
                unisocParam_updateQcainfo ();
                unisocParam_updateCgact ();

                int multi_apn_act = 0;

                for ( i = 1; i <= 4; i++ )
                {
                    snprintf ( strUciOpt, sizeof ( strUciOpt ), "lte_param.apn%d.enable", i );
                    memset ( apn_enable, 0, sizeof ( apn_enable ) );
                    sys_get_config ( strUciOpt, apn_enable, sizeof ( apn_enable ) );
                    CLOGD ( FINE, "%s -> [%s]\n", strUciOpt, apn_enable );

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "cid_%d_state", i );
                    memset ( apn_act_state, 0, sizeof ( apn_act_state ) );
                    nv_get ( strRamOpt, apn_act_state, sizeof ( apn_act_state ) );
                    CLOGD ( FINE, "%s -> [%s]\n", strRamOpt, apn_act_state );

                    if ( 0 == strcmp ( apn_act_state, "1" ) )
                    {
                        if ( 0 == strcmp ( apn_enable, "1" ) )
                        {
                            ret = unisocParam_updateQnetdevstatus ( i );

                            if ( 0 < ret )
                                apnActOrDeactMsgEvent ( i, 1 );
                        }
                        else
                        {
                            unisocParam_updateQnetdevctl ( i, 0 );
                        }
                    }
                    else
                    {
                        parsing_unisoc_qnetdevstatus( i, "" );
                        apnActOrDeactMsgEvent ( i, 0 );
                        if ( 0 == strcmp ( apn_enable, "1" ) )
                        {
                            if ( 0 == multi_apn_act )
                            {
                                comd_wait_ms ( 1500 );
                                multi_apn_act = 1;
                                unisocParam_updateQnetdevctl ( i, 1 );
                            }
                        }
                    }
                }
            }
            else
            {
                if ( 0 == strcmp ( simlock_stat, "0" ) && strcmp ( radio_stat, "off" ) && strcmp ( cur_select_mode, "manual_select" ) )
                {
                    unisocParam_connectNetwork ( NULL );
                }

                apnActOrDeactMsgEvent ( 0, 0 );

                if( 0 == unisocParam_updatePlmnSearchStatus() )
                {
                    goto END_S;
                }

                if ( strcmp ( mStatusVal, "disconnected" ) )
                {
                    nv_set ( "main_status", "disconnected" );
                    goto END_S;
                }
            }
        }
        else
        {
            if ( 0 == strcmp ( simlock_stat, "0" ) && strcmp ( radio_stat, "off" ) && strcmp ( cur_select_mode, "manual_select" ) )
            {
                unisocParam_connectNetwork ( NULL );
            }
            apnActOrDeactMsgEvent ( 0, 0 );

            if( strcmp ( radio_stat, "off" ) != 0 && 0 == unisocParam_updatePlmnSearchStatus() )
            {
                goto END;
            }

            if ( strcmp ( mStatusVal, "no_service" ) )
            {
                nv_set ( "main_status", "no_service" );
                goto END;
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

    ipv4v6_data_restore ( 0 );
}

void unisoc_install_mode_refresh ()
{
    return;
}

/******************************************************************************
 *  unisocParam_updatePlmnSearchStatus :  update PlmnSearch Status
 *
 * @param    void
 * @return   int
 * @declare  -1  ---> disconnected
 *           0   ---> searching
 *           1   ---> connected
 ******************************************************************************/
int unisocParam_updatePlmnSearchStatus()
{
    int ret = -1;
    char at_rep[64] = {0};

    memset(at_rep, 0, sizeof(at_rep));
    COMD_AT_PROCESS("AT+COPS?", 3000, at_rep, sizeof(at_rep));
    if ( strstr ( at_rep, "+COPS: 2" ) )
    {
        ret = -1;
        goto END;
    }
    else if ( strstr ( at_rep, "+COPS: 0" ) && !strstr ( at_rep, "+COPS: 0," ) )
    {
        ret = 0;
        goto END;
    }

    memset(at_rep, 0, sizeof(at_rep));
    COMD_AT_PROCESS("AT+C5GREG?", 3000, at_rep, sizeof(at_rep));
    if ( strstr ( at_rep, ",1" ) || strstr ( at_rep, ",5" ) || strstr ( at_rep, ",8") )
    {
        ret = 1;
        goto END;
    }
    else if ( strstr ( at_rep, ",0" ) || strstr ( at_rep, ",3") || strstr ( at_rep, ",4") )
    {
        ret = -1;
    }
    else
    {
        ret = 0;
        goto END;
    }

    memset(at_rep, 0, sizeof(at_rep));
    COMD_AT_PROCESS("AT+CEREG?", 3000, at_rep, sizeof(at_rep));
    if ( strstr ( at_rep, ",1" ) || strstr ( at_rep, ",5" ) || strstr ( at_rep, ",8") )
    {
        ret = 1;
        goto END;
    }
    else if ( strstr ( at_rep, ",0" ) || strstr ( at_rep, ",3") || strstr ( at_rep, ",4") )
    {
        ret = -1;
    }
    else
    {
        ret = 0;
        goto END;
    }

END:
    CLOGD(FINE, "PlmnSearchStatus: [%d]\n", ret);
    switch(ret)
    {
        case -1:
            nv_set ( "main_status", "disconnected" );
            break;
        case 0:
            nv_set ( "main_status", "searching" );
            break;
        case 1:
            nv_set ( "main_status", "connected" );
            break;
        default:
            break;
    }
    return ret;
}

