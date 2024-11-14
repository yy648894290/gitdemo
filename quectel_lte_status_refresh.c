#include "comd_share.h"
#include "atchannel.h"
#include "at_tok.h"
#include "quectel_lte_status_refresh.h"
#include "quectel_lte_atcmd_parsing.h"
#include "quectel_lte_config_response.h"
#include "config_key.h"
#include "hal_platform.h"
#include "utils_api.h"

static int abnormal_check_flag[5] = { 1, 1, 1, 1, 1 };
static int cpin_error_check = 0;
static int main_status_check = 0;
static long abnormal_start_time = 0;
static int g_simslot_switch_rec = 0;
static int g_simslot_switch_event = 0;

extern char led_mode[4];
extern char dualsim_en[4];
extern int apns_msg_flag[5];
extern char volte_enable[8];
extern char esim_supp[4];

static void quectel_lte_Param_radioRestart ()
{
    char at_rep[512] = {0};

    COMD_AT_PROCESS ( "AT+CFUN=0", 10000, at_rep, sizeof ( at_rep ) );

    comd_wait_ms ( 1500 );

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( "AT+CFUN=1", 10000, at_rep, sizeof ( at_rep ) );
}

int quectel_lte_Param_setATEcho ( char* data )
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "ATE0", 3000, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
        return -1;

    return 0;
}

int quectel_lte_Param_setCfunMode ( char* mode )
{
    char cfunSet_ret[4] = {0};

    quectel_lte_Param_radioOnOffSet ( mode );

    nv_get ( "setOnOff_lte", cfunSet_ret, sizeof ( cfunSet_ret ) );

    return strcmp ( cfunSet_ret, "0" ) ? -1 : 0;
}

int quectel_lte_Param_updateModuleVersion ( char* data )
{
    unsigned int check_times = 0;
    char at_rep[128] = {0};

    while ( check_times++ < 5 )
    {
        if ( 0 == COMD_AT_PROCESS ( "ATI", 1500, at_rep, sizeof ( at_rep ) ) )
        {
            if ( 0 == parsing_quectel_lte_moduleModel ( at_rep ) &&
                 0 == parsing_quectel_lte_moduleVersion ( at_rep ) )
            {
                return 0;
            }
        }
        comd_wait_ms ( 1000 );
        memset ( at_rep, 0, sizeof ( at_rep ) );
    }

    return 0; // do not return -1, even can not get module info
}

int quectel_lte_Param_updateUsbmode ( char* data )
{
    char at_rep[64] = {0};
    char modulemodel[8] = {0};
    int modem_reboot_flag = 0;

    nv_get ( "modulemodel", modulemodel, sizeof ( modulemodel ) );
    if ( 0 == strcmp ( modulemodel, "EC200A" ) )
    {
        COMD_AT_PROCESS ( "AT+QCFG=\"nat\"", 3000, at_rep, sizeof ( at_rep ) );

        if ( NULL == strstr ( at_rep, "\r\n+QCFG: \"nat\"" ) )
        {
            return -1;
        }

        if ( NULL == strstr ( at_rep, "\r\n+QCFG: \"nat\",1" ) )
        {
            memset ( at_rep, 0, sizeof( at_rep ) );
            COMD_AT_PROCESS ( "AT+QCFG=\"nat\",1", 3000, at_rep, sizeof ( at_rep ) );
            if ( strstr ( at_rep, "\r\nOK\r\n" ) )
            {
                modem_reboot_flag = 1;
            }
        }

        COMD_AT_PROCESS ( "AT+QCFG=\"usbnet\"", 3000, at_rep, sizeof ( at_rep ) );

        if ( NULL == strstr ( at_rep, "\r\n+QCFG: \"usbnet\"" ) )
        {
            return -1;
        }

        if ( NULL == strstr ( at_rep, "\r\n+QCFG: \"usbnet\",1" ) )
        {
            memset ( at_rep, 0, sizeof( at_rep ) );
            COMD_AT_PROCESS ( "AT+QCFG=\"usbnet\",1", 3000, at_rep, sizeof ( at_rep ) );
            if ( strstr ( at_rep, "\r\nOK\r\n" ) )
            {
                modem_reboot_flag = 1;
            }
        }

        if ( 1 == modem_reboot_flag )
        {
            comd_wait_ms ( 5000 );
            CLOGD ( WARNING, "Switch to nat ECM driver, reboot Quectel module !!!\n" );
            COMD_AT_PROCESS ( "AT+CFUN=1,1", 3000, at_rep, sizeof ( at_rep ) );
            return -1;
        }
    }

    return 0;
}

int quectel_lte_Param_setQuimslot ( char* data )
{
    char at_rep[128] = {0};
    char modulemodel[8] = {0};
    char board_bype[16] = {0};

    nv_get ( "modulemodel", modulemodel, sizeof ( modulemodel ) );
    if ( 0 == strcmp ( modulemodel, "EC200A" ) )
    {
        CLOGD ( FINE, "modulemodel is EC200A,exit!\n" );
        return 0;
    }

    COMD_AT_PROCESS ( "AT+QUIMSLOT?", 3000, at_rep, sizeof ( at_rep ) );

    comd_exec ( "/lib/factory_tool.sh show|grep ^CONFIG_HW_BOARD_TYPE_CUST|cut -d '=' -f 2|tr -d '\r\n'", board_bype, sizeof ( board_bype ) );

    if ( 0 == strcmp ( board_bype, "STX5061" ) )
    {
        if ( NULL == strstr ( at_rep, "\r\n+QUIMSLOT: 1" ) )
        {
            memset ( at_rep, 0, sizeof ( at_rep ) );
            COMD_AT_PROCESS ( "AT+QUIMSLOT=1", 3000, at_rep, sizeof ( at_rep ) );
            if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
            {
                return -1;
            }
        }
    }
    else
    {
        if ( NULL == strstr ( at_rep, "\r\n+QUIMSLOT: 2" ) )
        {
            memset ( at_rep, 0, sizeof ( at_rep ) );
            COMD_AT_PROCESS ( "AT+QUIMSLOT=2", 3000, at_rep, sizeof ( at_rep ) );
            if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
            {
                return -1;
            }
        }
    }

    return 0;
}

int quectel_lte_Param_setVlan ( char* data )
{
    char at_req[64] = {0};
    char at_rep[256] = {0};
    int i = 0;
    char vlan_mac[32] = {0};
    char ram_val[16] = {0};
    char modulemodel[8] = {0};

    nv_get ( "modulemodel", modulemodel, sizeof ( modulemodel ) );
    if ( 0 == strcmp ( modulemodel, "EC200A" ) )
    {
        system ( "/lib/rename_wan_if.sh" );

        CLOGD ( FINE, "modulemodel is EC200A, rename wan if and exit!\n" );
        return 0;
    }

    for ( i = 0; i <= 3; i++ )
    {
        snprintf ( at_req, sizeof ( at_req ), "AT+QMAP=\"VLAN\",%d,\"enable\",1", i + 100 );

        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( at_req, 3000, at_rep, sizeof ( at_rep ) );
    }

    for ( i = 3; i >= 0; i-- )
    {
        if ( 0 == i )
        {
            COMD_AT_PROCESS ( "AT+QMAP=\"mPDN_rule\"", 3000, at_rep, sizeof ( at_rep ) );
            if ( NULL == strstr ( at_rep, "\r\n+QMAP: \"MPDN_rule\",0,1,0,1,1\r\n" ) )
            {
                snprintf ( at_req, sizeof ( at_req ), "AT+QMAP=\"mPDN_rule\",0,1,0,1,1,\"FF:FF:FF:FF:FF:FF\"" );
            }
        }
        else
        {
            snprintf ( at_req, sizeof ( at_req ), "AT+QMAP=\"mPDN_rule\",%d", i );
            COMD_AT_PROCESS ( at_req, 3000, at_rep, sizeof ( at_rep ) );

            snprintf ( ram_val, sizeof ( ram_val ), "eth1.10%d_mac", i );
            memset ( vlan_mac, 0, sizeof ( vlan_mac ) );
            nv_get ( ram_val, vlan_mac, sizeof ( vlan_mac ) );

            snprintf ( at_req, sizeof ( at_req ), "AT+QMAP=\"mPDN_rule\",%d,%d,%d,1,%d,\"%s\"",
                              i, i + 1 + APN_OFFSET, i + 100, ( i > 0 ) ? 0 : 1, vlan_mac );
        }

        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( at_req, 3000, at_rep, sizeof ( at_rep ) );

        comd_wait_ms ( 300 );
    }

    return 0;
}

int quectel_lte_Param_setVolte ( char* data )
{
    char at_rep[64] = {0};
    char modulemodel[8] = {0};

    if ( 0 != strcmp ( volte_enable, "VOLTE" ) )
    {
        CLOGD ( FINE, "product no support VOLTE,exit!\n" );
        return 0;
    }

    nv_get ( "modulemodel", modulemodel, sizeof ( modulemodel ) );
    if ( 0 == strcmp ( modulemodel, "EC200A" ) )
    {
        CLOGD ( FINE, "modulemodel is EC200A,exit!\n" );
        return 0;
    }

    COMD_AT_PROCESS ( "AT+QSLIC?", 3000, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "+QSLIC: 1,2" ) )
    {
        memset ( at_rep, 0, sizeof( at_rep ) );
        COMD_AT_PROCESS ( "AT+QSLIC=1,2", 3000, at_rep, sizeof ( at_rep ) );

        if ( NULL == strstr ( at_rep, "OK" ) )
        {
            CLOGD ( WARNING, "set AT+QSLIC=1,2 failed !!!\n" );
        }
    }

    return 0;
}

int quectel_lte_Param_setsfe ( char* data )
{
    char at_rep[64] = {0};
    char modulemodel[8] = {0};

    nv_get ( "modulemodel", modulemodel, sizeof ( modulemodel ) );
    if ( 0 == strcmp ( modulemodel, "EC200A" ) )
    {
        CLOGD ( FINE, "modulemodel is EC200A,exit!\n" );
        return 0;
    }

    COMD_AT_PROCESS ( "AT+QMAP=\"SFE\"", 3000, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "+QMAP: \"SFE\",\"enable\"" ) )
    {
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( "AT+QMAP=\"SFE\",\"enable\"", 3000, at_rep, sizeof ( at_rep ) );

        if ( NULL == strstr ( at_rep, "OK" ) )
        {
            CLOGD ( WARNING, "set AT+QMAP=\"SFE\",\"enable\" failed !!!\n" );
        }
    }

    return 0;
}

int quectel_lte_Param_updateImei ( char* data )
{
    unsigned int check_times = 0;
    char imei_val[32] = {0};
    char at_rep[STR_AT_RSP_LEN] = {0};

    while ( check_times++ < 5 )
    {
        if ( 0 == COMD_AT_PROCESS ( "AT+EGMR=0,7", 1500, at_rep, sizeof ( at_rep ) ) )
        {
            parsing_quectel_lte_imei ( at_rep );
            nv_get ( "imei", imei_val, sizeof ( imei_val ) );
            if ( 15 <= strlen ( imei_val ) )
                return 0;
            memset ( imei_val, 0, sizeof ( imei_val ) );
        }
        comd_wait_ms ( 1000 );
        memset ( at_rep, 0, sizeof ( at_rep ) );
    }

    return 0; // do not return -1, even can not get imei
}

int quectel_lte_Param_updateSN ( char* data )
{
    unsigned int check_times = 0;
    char sn_val[32] = {0};
    char at_rep[STR_AT_RSP_LEN] = {0};

    while ( check_times++ < 5 )
    {
        if (0 == COMD_AT_PROCESS ( "AT+EGMR=0,5", 1500, at_rep, sizeof ( at_rep ) ) )
        {
            parsing_quectel_lte_sn ( at_rep );
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

int quectel_lte_Param_updateDefaultGateway ( char* data )
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

int quectel_lte_Param_updateSuppBands ( char* data )
{
    char module_type[32] = {0};
    char *substr = NULL;
    char at_rep[RECV_BUF_SIZE] = {0};
    char lte_bands[256] = {0};
    char old_suppband[128] = {0};

    char *stored_path = "lte_param.parameter.support_band";

    nv_get ( "modulemodel", module_type, sizeof ( module_type ) );
    if ( 0 == strcmp ( module_type, "EC200A" ) )
    {
        CLOGD ( FINE, "modulemodel is EC200A,exit!\n" );
        return 0;
    }

    snprintf ( lte_bands, sizeof ( lte_bands ), "%s",
                           strstr ( module_type, "EG120K-EA" ) ? QUECTEL_EG120K_EA_4G_BANDS : (
                           strstr ( module_type, "EG120K-NA" ) ? QUECTEL_EG120K_NA_4G_BANDS : (
                           strstr ( module_type, "EC200A" ) ? QUECTEL_EC200A_CN_4G_BANDS : ""
                        ) )
                    );

    if ( strcmp ( lte_bands, "" ) )
    {
        CLOGD ( FINE, "4G_BANDS_ORG -> [%s]\n", lte_bands );
        nv_set ( "suppband_org", lte_bands );
        comd_strrpl ( lte_bands, ":", " " );
        CLOGD ( FINE, "4G_SUPPBAND  -> [%s]\n", lte_bands );
        nv_set ( "suppband", lte_bands );

        sys_get_config ( stored_path, old_suppband, sizeof ( old_suppband ) );

        if ( strcmp ( old_suppband, lte_bands ) )
        {
            CLOGD ( WARNING, "Save supported bands to flash !\n" );
            sys_set_config ( stored_path, lte_bands );
            sys_commit_config ( "lte_param" );
        }

        return 0;
    }

    COMD_AT_PROCESS ( "AT+QNWPREFCFG=\"policy_band\"", 1500, at_rep, sizeof ( at_rep ) );
    substr = strtok ( at_rep, "\r\n" );
    while ( substr )
    {
        CLOGD ( FINE, "policy_band:\n[%s]\n\n", substr );
        if ( strstr ( substr, "+QNWPREFCFG: \"lte_band\"," ) )
        {
            snprintf ( lte_bands, sizeof ( lte_bands ), "%s", substr );
        }
        substr = strtok ( NULL, "\r\n" );
    }

    return ( parsing_quectel_lte_supp4gband ( lte_bands ) );
}

int quectel_lte_Param_initAPNsetting ( char* data )
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
            snprintf ( strUciOpt, sizeof ( strUciOpt ), "lte_param.apn%d.profile_name", i );
            memset ( uci_profilename, 0, sizeof ( uci_profilename ) );
            sys_get_config ( strUciOpt, uci_profilename, sizeof ( uci_profilename ) );
            CLOGD ( FINE, "%s -> [%s]\n", strUciOpt, uci_profilename );
        }

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "profile_name%d", i );
        nv_set ( strRamOpt, uci_profilename );

        if ( apnset_start_index < apnset_end_index )
        {
            snprintf ( strUciOpt, sizeof ( strUciOpt ), "lte_param.apn%d.pdptype", i );
            memset ( uci_pdptype, 0, sizeof ( uci_pdptype ) );
            sys_get_config ( strUciOpt, uci_pdptype, sizeof ( uci_pdptype ) );
            CLOGD ( FINE, "%s -> [%s]\n", strUciOpt, uci_pdptype );
        }

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "pdn%d_type", i );
        nv_set ( strRamOpt, strcmp ( uci_pdptype, "IP" ) ? ( strcmp ( uci_pdptype, "IPV6" ) ? "2" : "1" ) : "0" );

        if ( apnset_start_index < apnset_end_index )
        {
            snprintf ( strUciOpt, sizeof ( strUciOpt ), "lte_param.apn%d.apnname", i );
            memset ( uci_apnname, 0, sizeof ( uci_apnname ) );
            sys_get_config ( strUciOpt, uci_apnname, sizeof ( uci_apnname ) );
            CLOGD ( FINE, "%s -> [%s]\n", strUciOpt, uci_apnname );
        }

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_name", i );
        nv_set ( strRamOpt, uci_apnname );

        if ( apnset_start_index < apnset_end_index )
        {
            snprintf ( strUciOpt, sizeof ( strUciOpt ), "lte_param.apn%d.enable", i );
            memset ( uci_apnenable, 0, sizeof ( uci_apnenable ) );
            sys_get_config ( strUciOpt, uci_apnenable, sizeof ( uci_apnenable ) );
            CLOGD ( FINE, "%s -> [%s]\n", strUciOpt, uci_apnenable );
        }

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_enable", i );
        nv_set ( strRamOpt, uci_apnenable );

        if ( apnset_start_index < apnset_end_index )
        {
            snprintf ( strUciOpt, sizeof ( strUciOpt ), "lte_param.apn%d.auth_type", i );
            memset ( uci_authtype, 0, sizeof ( uci_authtype ) );
            sys_get_config ( strUciOpt, uci_authtype, sizeof ( uci_authtype ) );
            CLOGD ( FINE, "%s -> [%s]\n", strUciOpt, uci_authtype );
            strcpy ( uci_authtype, strcmp ( uci_authtype, "CHAP" ) ? ( strcmp ( uci_authtype, "PAP" ) ? "0" : "1" ) : "2" );
        }

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "auth_type%d", i );
        nv_set ( strRamOpt, uci_authtype );

        if ( apnset_start_index < apnset_end_index )
        {
            snprintf ( strUciOpt, sizeof ( strUciOpt ), "lte_param.apn%d.username", i );
            memset ( uci_username, 0, sizeof ( uci_username ) );
            sys_get_config ( strUciOpt, uci_username, sizeof ( uci_username ) );
            CLOGD ( FINE, "%s -> [%s]\n", strUciOpt, uci_username );
        }

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "user_name%d", i );
        nv_set ( strRamOpt, uci_username );

        if ( apnset_start_index < apnset_end_index )
        {
            snprintf ( strUciOpt, sizeof ( strUciOpt ), "lte_param.apn%d.password", i );
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
                       ( 1 == i ) ? i : ( i + APN_OFFSET ),
                       strcmp ( uci_pdptype, "IP" ) ? ( strcmp ( uci_pdptype, "IPV6" ) ? "3" : "2" ) : "1",
                       uci_apnname,
                       strcmp ( uci_authtype, "0" ) ? uci_username : "",
                       strcmp ( uci_authtype, "0" ) ? uci_password : "",
                       uci_authtype
                );

            memset ( at_rep, 0, sizeof ( at_rep ) );
            COMD_AT_PROCESS ( at_cmd, 3000, at_rep, sizeof ( at_rep ) );

            if ( data )
            {
                if ( 1 == i )
                {
                    snprintf ( at_cmd, sizeof ( at_cmd ), "AT+COPS=2" );
                    memset ( at_rep, 0, sizeof ( at_rep ) );
                    COMD_AT_PROCESS ( at_cmd, 3000, at_rep, sizeof ( at_rep ) );
                }
                else
                {
                    quectel_lte_Param_configQmapConnect ( i , 0 );
                }
            }

        }
        else
        {
            snprintf ( at_cmd, sizeof ( at_cmd ), "AT+CGDCONT=%d", ( 1 == i ) ? i : ( i + APN_OFFSET ) );
            COMD_AT_PROCESS ( at_cmd, 3000, at_rep, sizeof ( at_rep ) );
        }
    }

    return 0;
}

int quectel_lte_Param_initNetSelectMode ( char* data )
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

int quectel_lte_Param_radioLockInit ( char* data )
{
    char uciScanMode[16] = {0};
    char uciLocked4gBand[128] = {0};
    char uciLockedBand[256] = {0};
    char uciLockedFreq[256] = {0};
    char uciLockedPci[256] = {0};
    char modulemodel[8] = {0};

    nv_get ( "modulemodel", modulemodel, sizeof ( modulemodel ) );
    if ( 0 == strcmp ( modulemodel, "EC200A" ) )
    {
        system ( "gpio w 43 0" );
        quectel_lte_Param_radioRestart ();
        CLOGD ( FINE, "modulemodel is EC200A,exit!\n" );
        return 0;
    }

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

        snprintf ( uciLockedBand, sizeof ( uciLockedBand ), "%s", uciLocked4gBand );

        quectel_lte_Param_lockband ( uciLockedBand, 0 );
    }
    else if ( 0 == strcmp ( uciScanMode, "FREQ_LOCK" ) )
    {
        sys_get_config ( WAN_LTE_LOCK_FREQ, uciLockedFreq, sizeof ( uciLockedFreq ) );

        if ( 0 != strcmp ( uciLockedFreq, "" ) )
        {
            // "4G,41,40000;5G,28,156100,15;"
            CLOGD ( FINE, "locked_freq -> [%s]\n", uciLockedFreq );
            quectel_lte_Param_lockearfcn ( uciLockedFreq, 0 );
        }
    }
    else if ( 0 == strcmp ( uciScanMode, "PCI_LOCK" ) )
    {
        sys_get_config ( WAN_LTE_LOCK_PCI, uciLockedPci, sizeof ( uciLockedPci ) );

        if ( 0 != strcmp ( uciLockedPci, "" ) )
        {
            // "4G,41,40000,104;5G,28,156100,505,15;"
            CLOGD ( FINE, "locked_pci -> [%s]\n", uciLockedPci );
            quectel_lte_Param_lockpci ( uciLockedPci, 0 );
        }
    }
    else if ( NULL == data )
    {
        quectel_lte_Param_lockband ( "", 0 );
    }
    else if ( 0 == strcmp ( data, "restore_fullband" ) )
    {
        quectel_lte_Param_lockband ( "", 1 );
    }

    return 0;

}

int quectel_lte_Param_setRatPriority ( char* data )
{
    char at_rep[64] = {0};
    char modulemodel[8] = {0};

    nv_get ( "modulemodel", modulemodel, sizeof ( modulemodel ) );
    if ( 0 == strcmp ( modulemodel, "EC200A" ) )
    {
        COMD_AT_PROCESS ( "AT+QCFG=\"nwscanseq\",3", 3000, at_rep, sizeof ( at_rep ) );
    }
    else
    {
        COMD_AT_PROCESS ( "AT+QNWPREFCFG=\"mode_pref\",LTE", 3000, at_rep, sizeof ( at_rep ) );
    }

    return strstr ( at_rep, "OK" ) ? 0 : 1;
}

void quectel_lte_Param_updatePinStatus ()
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+CPIN?", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "+CPIN:" ) || strstr ( at_rep, "ERROR" ) )
        parsing_quectel_lte_cpin_get ( at_rep );
}

void quectel_lte_Param_updatePinLockStatus ()
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+CLCK=\"SC\",2", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "+CLCK:" ) )
        parsing_quectel_lte_clck_get ( at_rep );
}

void quectel_lte_Param_updatePinPukCount ()
{
    char at_rep[256] = {0};

    COMD_AT_PROCESS ( "AT+QPINC?", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "+QPINC:" ) )
        parsing_quectel_lte_qpinc ( at_rep );
}

int quectel_lte_Param_setIpv6Format ( char* data )
{
    char at_rep[64] = {0};
    COMD_AT_PROCESS ( "AT+CGPIAF=1,1,0,0", 10000, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
        return -1;

    return 0;
}

void quectel_lte_Param_updateImsi ()
{
    char at_rep[64] = {0};
    char ram_val[16] = {0};

    nv_get ( "imsi", ram_val, sizeof ( ram_val ) );

    if ( 0 == strcmp ( ram_val, "" ) || 0 == strcmp ( ram_val, "--" ) )
    {
        COMD_AT_PROCESS ( "AT+CIMI", 3000, at_rep, sizeof ( at_rep ) );

        if ( strcmp ( at_rep, "" ) )
            parsing_quectel_lte_cimi ( at_rep );

        quectel_lte_Param_updateUsimMncLen ();
        quectel_lte_Param_setIpv6Format ( NULL );
    }
}

void quectel_lte_Param_updateIccid ()
{
    char at_rep[64] = {0};
    char ram_val[32] = {0};

    nv_get ( "iccid", ram_val, sizeof ( ram_val ) );

    if ( 0 == strcmp ( ram_val, "" ) || 0 == strcmp ( ram_val, "--" ) )
    {
        COMD_AT_PROCESS ( "AT+CRSM=176,12258,0,0,10", 1500, at_rep, sizeof ( at_rep ) );

        if ( strstr ( at_rep, "+CRSM:" ) )
            parsing_quectel_lte_iccid ( at_rep );
    }
}

void quectel_lte_Param_updatePhoneNumber ()
{
    char at_rep[64] = {0};
    char ram_val[32] = {0};

    nv_get ( "phone_number", ram_val, sizeof ( ram_val ) );

    if ( 0 == strcmp ( ram_val, "" ) || 0 == strcmp ( ram_val, "--" ) )
    {
        COMD_AT_PROCESS ( "AT+CNUM", 1500, at_rep, sizeof ( at_rep ) );

        if ( strcmp ( at_rep, "" ) )
            parsing_quectel_lte_cnum ( at_rep );
    }
}

void quectel_lte_Param_updateUsimMncLen ()
{
    char at_rep[128] = {0};

    COMD_AT_PROCESS ( "AT+CRSM=176,28589,0,0,0", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "+CRSM:" ) )
        parsing_quectel_lte_simMncLen ( at_rep );
}

void quectel_lte_Param_updateSimSPN ()
{
    char at_rep[128] = {0};
    char ram_val[64] = {0};

    nv_get ( "SIM_SPN", ram_val, sizeof ( ram_val ) );

    if ( 0 == strcmp ( ram_val, "" ) || 0 == strcmp ( ram_val, "--" ) )
    {
        COMD_AT_PROCESS ( "AT+CRSM=176,28486,0,0,17", 3000, at_rep, sizeof ( at_rep ) );

        if ( strstr ( at_rep, "+CRSM:" ) )
            parsing_quectel_lte_sim_spn ( at_rep );
    }
}

void quectel_lte_Param_updateClcc ()
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
            ret = parsing_quectel_lte_cgdcont ( substr );
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
        ret = parsing_quectel_lte_clcc ( substr );
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

void quectel_lte_Param_updateCsq ()
{
    char at_rep[STR_AT_RSP_LEN] = {0};

    COMD_AT_PROCESS ( "AT+CSQ", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "+CSQ:" ) )
        parsing_quectel_lte_csq ( at_rep );
}

void quectel_lte_Param_updateCreg ()
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+CREG?", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "\r\n+CREG:" ) )
        parsing_quectel_lte_creg ( at_rep );
}

void quectel_lte_Param_updateCgatt ()
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+CGATT?", 3000, at_rep, sizeof ( at_rep ) );

    if ( strcmp ( at_rep, "" ) )
        parsing_quectel_lte_cgatt ( at_rep );
}

void quectel_lte_Param_updateCops ()
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+COPS?", 3000, at_rep, sizeof ( at_rep ) );
    if ( strcmp ( at_rep, "" ) )
        parsing_quectel_lte_operator ( at_rep );
}

void quectel_lte_Param_updateRrcState ()
{
    char at_rep[RECV_BUF_SIZE] = {0};
    char modulemodel[8] = {0};

    nv_get ( "modulemodel", modulemodel, sizeof ( modulemodel ) );
    if ( 0 == strcmp ( modulemodel, "EC200A" ) )
    {
        CLOGD ( FINE, "modulemodel is EC200A,exit!\n" );
        return;
    }

    COMD_AT_PROCESS ( "AT+QNWCFG=\"rrc_state\"", 3000, at_rep, sizeof ( at_rep ) );

    if ( strcmp ( at_rep, "" ) )
    {
        parsing_quectel_lte_rrc_state ( at_rep );
    }
}

void quectel_lte_Param_updateServingCell ( char* data )
{
    char at_rep[RECV_BUF_SIZE] = {0};

    COMD_AT_PROCESS ( "AT+QENG=\"servingcell\"", 3000, at_rep, sizeof ( at_rep ) );

    if ( strcmp ( at_rep, "" ) )
    {
        parsing_quectel_lte_serving_cellinfo ( at_rep );
    }
}

static void quectel_lte_Param_updateSignalBar ()
{
    char signal_s[8] = {0};
    int signal_i = 0;

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
    nv_get ( "rsrp0" , signal_s, sizeof ( signal_s ) );

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

void quectel_lte_Param_updateQcainfo ()
{
    char at_rep[RECV_BUF_SIZE] = {0};
    int i = 0;
    char strRamOpt[32] = {0};
    char modulemodel[8] = {0};

    nv_get ( "modulemodel", modulemodel, sizeof ( modulemodel ) );
    if ( 0 == strcmp ( modulemodel, "EC200A" ) )
    {
        CLOGD ( FINE, "modulemodel is EC200A,exit!\n" );
        return;
    }

    COMD_AT_PROCESS ( "AT+QCAINFO", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "SCC" ) )
    {
        parsing_quectel_lte_qcainfo ( at_rep );
    }
    else
    {
        nv_set ( "secondary_cell", "0" );

        for ( i = 1; i < 4; i++ )
        {
            snprintf ( strRamOpt, sizeof ( strRamOpt ), "S%d_dl_frequency", i );
            nv_set ( strRamOpt, "" );

            snprintf ( strRamOpt, sizeof ( strRamOpt ), "S%d_ul_frequency", i );
            nv_set ( strRamOpt, "" );
        }
    }
}

void quectel_lte_Param_updateQnetdevctl ()
{
    char at_rep[STR_AT_RSP_LEN] = {0};

    COMD_AT_PROCESS ( "AT+QNETDEVCTL?", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "\r\nOK\r\n" ) )
        parsing_quectel_lte_Qnetdevctl ( at_rep );
}

void quectel_lte_Param_updateCgact ()
{
    char at_rep[STR_AT_RSP_LEN] = {0};

    COMD_AT_PROCESS ( "AT+CGACT?", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "\r\nOK\r\n" ) )
        parsing_quectel_lte_cgact ( at_rep );
}

int quectel_lte_Param_updateCgcontrdp ( int apn_index )
{
    char at_rep[RECV_BUF_SIZE] = {0};
    char at_cmd[32] = {0};

    snprintf ( at_cmd, sizeof ( at_cmd ), "AT+CGCONTRDP=%d", ( 1 == apn_index ) ? apn_index : apn_index + APN_OFFSET );

    COMD_AT_PROCESS ( at_cmd, 8000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "\r\nOK\r\n" ) )
    {
        return parsing_quectel_lte_cgcontrdp ( apn_index, at_rep );
    }

    return 0;
}

int quectel_lte_Param_configQmapConnect ( int cid_index, int act_val )
{
    char at_req[64] = {0};
    char at_rep[64] = {0};
    char modulemodel[8] = {0};

    nv_get ( "modulemodel", modulemodel, sizeof ( modulemodel ) );
    if ( 0 == strcmp ( modulemodel, "EC200A" ) )
    {
        snprintf ( at_req, sizeof ( at_req ), "AT+QNETDEVCTL=%d,1,0", act_val );
    }
    else
    {
        snprintf ( at_req, sizeof ( at_req ), "AT+QMAP=\"connect\",%d,%d", cid_index - 1, act_val );
    }

    COMD_AT_PROCESS ( at_req, 5000, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
        return -1;

    return 0;
}

int quectel_lte_Param_updateDualSim ( void )
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
            quectel_lte_Param_updatePinStatus ();
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

static void quectel_lte_init_apn_netdev ()
{
    nv_set ( "apn1_netdev", "eth1.100" );
    nv_set ( "apn2_netdev", "eth1.101" );
    nv_set ( "apn3_netdev", "eth1.102" );
    nv_set ( "apn4_netdev", "eth1.103" );
}

int quectel_lte_module_param_init ()
{
    reset_apns_msg_flag ( 0 );
    init_abnormal_check_config ();
    quectel_lte_init_apn_netdev ();

    cpin_error_check = 0;
    main_status_check = 0;
    abnormal_start_time = 0;

    if (
        quectel_lte_Param_setATEcho ( NULL ) ||
        quectel_lte_Param_setCfunMode ( "off" ) ||
        quectel_lte_Param_updateModuleVersion ( NULL ) ||
        quectel_lte_Param_updateUsbmode ( NULL ) ||
        quectel_lte_Param_setQuimslot ( NULL ) ||
        quectel_lte_Param_setVlan ( NULL ) ||
        quectel_lte_Param_setVolte ( NULL ) ||
        quectel_lte_Param_setsfe ( NULL ) ||
        quectel_lte_Param_updateImei ( NULL ) ||
        quectel_lte_Param_updateSN ( NULL ) ||
        quectel_lte_Param_updateDefaultGateway ( NULL ) ||
        quectel_lte_Param_updateSuppBands ( NULL ) ||
        quectel_lte_Param_initAPNsetting ( NULL ) ||
        quectel_lte_Param_lteRoamingSet ( NULL ) ||
        quectel_lte_Param_initNetSelectMode ( NULL ) ||
        quectel_lte_Param_setCfunMode ( "on" ) ||
        quectel_lte_Param_radioLockInit ( NULL ) ||
        quectel_lte_Param_setRatPriority ( NULL )
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

static int quectel_lte_Param_checkAbnormal ( char* sim_stat, char* main_stat )
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
    CLOGD ( FINE, "lte_on_off         -> [%s]\n", radio_stat );
    CLOGD ( FINE, "net_select_mode    -> [%s]\n", net_select_mode );

    if ( 0 == strcmp ( radio_stat, "off" ) || 0 == strcmp ( net_select_mode, "manual_select" ) )
    {
        cpin_error_check = 0;
        main_status_check = 0;
        return 0;
    }

    if ( 0 == strcmp ( sim_stat, "ERROR" ) )
    {
        if ( abnormal_more_than_xxx_seconds ( cpin_error_check, 120 ) || cpin_error_check++ > 90 )
        {
            CLOGD ( FINE, "abnormal_check_flag[0]: [%d]\n", abnormal_check_flag[0] );
            if ( abnormal_check_flag[0] )
            {
                quectel_lte_Param_radioRestart ();
                CLOGD ( ERROR, "exe AT+CFUN=0/1 for sim_not_ready !!!\n" );
            }
            cpin_error_check = 0;
            ret = -1;
        }

        main_status_check = 0;
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
                quectel_lte_Param_radioRestart ();
                CLOGD ( ERROR, "exe AT+CFUN=0/1 for main_status not connected !!!\n" );
            }
            main_status_check = 0;
            ret = -1;
        }

        return ret;
    }
    else
    {
        main_status_check = 0;
    }

    return 0;
}

void quectel_lte_normal_status_refresh()
{
    char cpin_value[16] = {0};
    char mStatusVal[32] = {0};
    char csq_value[32] = {0};
    char cgatt_value[4] = {0};
    char time_str[16] = {0};
    char apn_enable[4] = {0};
    char strUciOpt[64] = {0};
    char apn_act_state[4] = {0};
    char strRamOpt[64] = {0};
    int i = 0;
    int ret = 1;
    char radio_stat[8] = {0};
    char simlock_stat[4] = {0};
    char cur_select_mode[32] = {0};
    int cur_cfun_state = 0;
    char at_rep[32] = {0};

    quectel_lte_Param_updatePinStatus ();
    quectel_lte_Param_updatePinLockStatus ();
    quectel_lte_Param_updatePinPukCount ();

    if ( 0 == strncmp ( dualsim_en, "1", 1 ) && 1 == quectel_lte_Param_updateDualSim () && g_simslot_switch_event == 1)
    {
        g_simslot_switch_event = 0;

        comd_wait_ms ( 10 * 1000 );
        quectel_lte_Param_initAPNsetting ( NULL );
        quectel_lte_Param_radioLockInit ( "restore_fullband" );
        nv_set ( "imsi", "--" );
        nv_set ( "SIM_SPN", "--" );
        return;
    }

    nv_get ( "cpin", cpin_value, sizeof ( cpin_value ) );
    nv_get ( "main_status", mStatusVal, sizeof ( mStatusVal ) );

    quectel_lte_Param_checkAbnormal ( cpin_value, mStatusVal );

    if ( 0 == strcmp ( cpin_value, "READY" ) )
    {
        quectel_lte_Param_updateImsi ();
        quectel_lte_Param_updateSimSPN ();
        quectel_lte_Param_getSimlock ( NULL );
        quectel_lte_Param_updateIccid ();
        quectel_lte_Param_updatePhoneNumber ();
        nv_get ( "simlocked", simlock_stat, sizeof ( simlock_stat ) );
        nv_get ( "lte_on_off", radio_stat, sizeof ( radio_stat ) );
        nv_get ( "net_select_mode", cur_select_mode, sizeof ( cur_select_mode ) );
        nv_get ( "setOnOff_lte", strRamOpt, sizeof ( strRamOpt ) );

        COMD_AT_PROCESS ( "AT+CFUN?", 3000, at_rep, sizeof ( at_rep ) );

        cur_cfun_state = parsing_quectel_lte_cfun_get ( at_rep );

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
            quectel_lte_Param_updateClcc ();
        }

        quectel_lte_Param_updateCsq ();
        nv_get ( "csq", csq_value, sizeof ( csq_value ) );

        if ( strcmp ( csq_value, "" ) && strcmp ( csq_value, "99,99" ) )
        {
            quectel_lte_Param_updateCreg ();
            quectel_lte_Param_updateCgatt ();
            quectel_lte_Param_updateCops ();
            quectel_lte_Param_updateRrcState ();
            quectel_lte_Param_updateServingCell ( NULL );
            quectel_lte_Param_updateSignalBar ();
            quectel_lte_Param_updateQcainfo ();

            nv_get ( "cgatt_val", cgatt_value, sizeof ( cgatt_value ) );
            CLOGD ( FINE, "cgatt_value -> [%s]\n", cgatt_value );
            if ( 0 == strcmp ( cgatt_value, "1" ) )
            {
                if ( 0 == strcmp ( simlock_stat, "1" ) )
                {
                    quectel_lte_Param_disconnectNetwork ( NULL );
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
                char modulemodel[8] = {0};
                int apnset_end_index = 4;

                nv_get ( "modulemodel", modulemodel, sizeof ( modulemodel ) );
                if ( 0 == strcmp ( modulemodel, "EC200A" ) )
                {
                    apnset_end_index = 1;
                    quectel_lte_Param_updateQnetdevctl ();
                }
                else
                {
                    quectel_lte_Param_updateCgact ();
                }

                for ( i = 1; i <= apnset_end_index; i++ )
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
                            ret = quectel_lte_Param_updateCgcontrdp ( i );

                            if ( 0 < ret )
                                apnActOrDeactMsgEvent ( i, 1 );
                        }
                        else
                        {
                            quectel_lte_Param_configQmapConnect ( i, 0 );
                        }
                    }
                    else
                    {
                        parsing_quectel_lte_cgcontrdp ( i, "" );
                        apnActOrDeactMsgEvent ( i, 0 );
                        if ( 0 == strcmp ( apn_enable, "1" ) )
                        {
                            if ( 0 == multi_apn_act )
                            {
                                comd_wait_ms ( 1500 );
                                multi_apn_act = 1;
                                quectel_lte_Param_configQmapConnect ( i, 1 );
                            }
                        }
                    }
                }
            }
            else
            {
                if ( 0 == strcmp ( simlock_stat, "0" ) && strcmp ( radio_stat, "off" ) && strcmp ( cur_select_mode, "manual_select" ) )
                {
                    quectel_lte_Param_connectNetwork ( NULL );
                }

                apnActOrDeactMsgEvent ( 0, 0 );

                if( 0 == quectel_lte_Param_updatePlmnSearchStatus() )
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
                quectel_lte_Param_connectNetwork ( NULL );
            }

            apnActOrDeactMsgEvent ( 0, 0 );

            if( strcmp ( radio_stat, "on" ) == 0 && quectel_lte_Param_updatePlmnSearchStatus() == 0 )
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

void quectel_lte_install_mode_refresh ()
{
    return;
}

/******************************************************************************
 *  quectelParam_updatePlmnSearchStatus :  update PlmnSearch Status
 *
 * @param    void
 * @return   int
 * @declare  -1  ---> disconnected
 *           0   ---> searching
 *           1   ---> connected
 ******************************************************************************/
int quectel_lte_Param_updatePlmnSearchStatus(void)
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


