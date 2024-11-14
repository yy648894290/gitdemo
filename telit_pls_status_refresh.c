#include "comd_share.h"
#include "atchannel.h"
#include "telit_pls_status_refresh.h"
#include "telit_pls_atcmd_parsing.h"
#include "telit_pls_config_response.h"
#include "config_key.h"
#include "hal_platform.h"

extern char dualsim_en[4];
static int abnormal_check_flag[5] = { 1, 1, 1, 1, 1 };
static int cpin_error_check = 0;
static int main_status_check = 0;
static long abnormal_start_time = 0;
static int lock_init_flag = 0;
static int g_simslot_switch_rec = 0;
static int g_simslot_switch_event = 0;

extern char led_mode[4];
extern char dualsim_en[4];
extern int apns_msg_flag[5];
extern char volte_enable[8];
extern char esim_supp[4];

extern COMD_DEV_INFO at_dev;

int telitPlsParam_setATEcho ( char* data )
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "ATE0", 3000, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
        return -1;

    return 0;
}

int telitPlsParam_updatePDPreturnFormat ( void )
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+CGPIAF?", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "+CGPIAF: 0," ) )
    {
        COMD_AT_PROCESS ( "AT+CGPIAF=1,0,0,1", 3000, at_rep, sizeof ( at_rep ) );
    }

    return 0;
}

int telitPlsParam_setCfunMode( char* mode )
{
    char cfunSet_ret[4] = {0};

    telitPlsParam_radioOnOffSet ( mode );

    nv_get ( "setOnOff_lte", cfunSet_ret, sizeof ( cfunSet_ret ) );

    return strcmp ( cfunSet_ret, "0" ) ? -1 : 0;

}

int telitPlsParam_updateModuleVersion ( char* data )
{
    unsigned int check_times = 0;
    char at_rep[128] = {0};

    while ( check_times++ < 5 )
    {
        memset(at_rep, 0, sizeof ( at_rep ));
        comd_wait_ms ( 1000 );
        if ( 0 == COMD_AT_PROCESS ( "AT+CGMM", 1500, at_rep, sizeof ( at_rep ) ) )
        {
            if ( 0 != parsing_telit_pls_moduleModel ( at_rep ) )
            {
                continue;
            }
        }

        memset ( at_rep, 0, sizeof ( at_rep ) );
        if ( 0 == COMD_AT_PROCESS ( "AT+CGMR", 1500, at_rep, sizeof ( at_rep ) ) )
        {
            if ( 0 != parsing_telit_pls_moduleVersion ( at_rep ) )
            {
                continue;
            }
        }
        break;
    }

    return 0; // do not return -1, even can not get module info
}

int telitPlsParam_updateImei ( char* data )
{
    unsigned int check_times = 0;
    char imei_val[32] = {0};
    char at_rep[STR_AT_RSP_LEN] = {0};

    while ( check_times++ < 5 )
    {
        if ( 0 == COMD_AT_PROCESS ( "AT+CGSN", 1500, at_rep, sizeof ( at_rep ) ) )
        {
            parsing_telit_pls_imei ( at_rep );
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

int telitPlsParam_updateSN( char* data )
{
    unsigned int check_times = 0;
    char sn_val[32] = {0};
    char at_rep[STR_AT_RSP_LEN] = {0};

    while ( check_times++ < 5 )
    {
        if (0 == COMD_AT_PROCESS ( "AT+GSN", 1500, at_rep, sizeof ( at_rep ) ) )
        {
            parsing_telit_pls_sn ( at_rep );
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

int telitPlsParam_updateDefaultGateway( char* data )
{
    char nv_gateway[8] = {0};
    char config_gw[8] = {0};

    sys_get_config ( "lte_param.apn.gw", config_gw, sizeof ( config_gw ) );

    nv_get ( "default_gateway", nv_gateway, sizeof ( nv_gateway ) );
    if ( strcmp ( nv_gateway, "9" ) )
    {
        nv_set ( "default_gateway",
                        strcmp ( config_gw, "" ) ? config_gw : "1" );
    }

    return 0;
}

int telitPlsParam_updateSuppBands ( char* data )
{
    char *stored_path = LTE_PARAMETER_SUPPORT_BAND;
    char at_rep[RECV_BUF_SIZE] = {0};
    char lte_bands[256] = {0};
    char old_suppband[128] = {0};

    sys_get_config ( stored_path, old_suppband, sizeof ( old_suppband ) );

    snprintf(lte_bands,sizeof(lte_bands),"%s",TELIT_PLS_4G_BANDS);
    comd_strrpl ( lte_bands, ":", " " );
    CLOGD ( FINE, "4G_SUPPBAND  -> [%s]\n", lte_bands );
    nv_set ( "suppband", lte_bands );

    CLOGD ( FINE, "4G_BANDS_ORG -> [%s]\n", TELIT_PLS_4G_BANDS );
    nv_set ( "suppband_org", TELIT_PLS_4G_BANDS );

    sys_get_config ( stored_path, old_suppband, sizeof ( old_suppband ) );

    if ( strcmp ( old_suppband, lte_bands ) )
    {
        CLOGD ( WARNING, "Save supported bands to flash !\n" );
        sys_set_config ( stored_path, lte_bands );
        system ( "uci commit lte_param 2>/dev/null" );
    }

    nv_set("arfcnlock_type", TELIT_PLS_LOCK_EARFCN_MAX);
    nv_set("pcilock_type", TELIT_PLS_LOCK_PCI_MAX);

    return 0;
}

int telitPlsParam_initAPNsetting( char* data )
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
    int apnset_end_index = TELIT_PLS_MAX_APN_NUM + 1;

    if( data )
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


    for ( i = apnset_start_index; i <= TELIT_PLS_MAX_APN_NUM; i++ )
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

        if ( 0 == strcmp ( uci_apnenable, "1" ) )
        {
            memset(at_cmd,0,sizeof(at_cmd));
            snprintf ( at_cmd, sizeof ( at_cmd ), "AT+CGDCONT=%d,\"%s\",\"%s\"",
                       i,uci_pdptype,uci_apnname);

            memset ( at_rep, 0, sizeof ( at_rep ) );
            COMD_AT_PROCESS ( at_cmd, 3000, at_rep, sizeof ( at_rep ) );
        }
        else
        {
            snprintf ( at_cmd, sizeof ( at_cmd ), "AT+CGDCONT=%d", i );
            COMD_AT_PROCESS ( at_cmd, 3000, at_rep, sizeof ( at_rep ) );
        }


        if ( apnset_start_index == apnset_end_index )
        {
            snprintf ( at_cmd, sizeof ( at_cmd ), "AT+COPS=2" );
            memset ( at_rep, 0, sizeof ( at_rep ) );
            COMD_AT_PROCESS ( at_cmd, 3000, at_rep, sizeof ( at_rep ) );

            comd_wait_ms(3000);
            snprintf ( at_cmd, sizeof ( at_cmd ), "AT+COPS=0" );
            memset ( at_rep, 0, sizeof ( at_rep ) );
            COMD_AT_PROCESS ( at_cmd, 3000, at_rep, sizeof ( at_rep ) );
        }

        if ( 0 == strcmp ( uci_authtype, "0" ) )
        {
            snprintf ( at_cmd, sizeof ( at_cmd ), "AT^SGAUTH=%d,0", i );
        }
        else
        {
            snprintf ( at_cmd, sizeof ( at_cmd ), "AT^SGAUTH=%d,%s,\"%s\",\"%s\"",
                                                        i, uci_authtype, uci_password, uci_username );
        }

        memset ( at_rep, 0, sizeof ( at_rep ) );

        COMD_AT_PROCESS ( at_cmd, 3000, at_rep, sizeof ( at_rep ) );

    }
    return 0;
}

int telitPlsParam_initNetSelectMode( char* data )
{
    char m_select_mode[4] = {0};

    sys_get_config ( "lte_param.parameter.auto_connect", m_select_mode, sizeof ( m_select_mode ) );

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

static int telitPlsParam_init_apn_netdev (void)
{
    nv_set ( "apn1_netdev", "eth1.100" );
    return 0;
}

int telitPlsParam_radioLockInit( char* data )
{
    char uciScanMode[16] = {0};
    char uciLocked4gBand[128] = {0};
    char at_rep[64] = {0};

    sys_get_config ( "lte_param.scanmode.scanmode",uciScanMode, sizeof ( uciScanMode ) );
    sys_get_config ( "lte_param.scanmode.locked_band", uciLocked4gBand, sizeof ( uciLocked4gBand ) );

    COMD_AT_PROCESS ( "AT^SCFG=\"Radio/Band/2G\",0", 3000, at_rep, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( "AT^SCFG=\"Radio/Band/3G\",0", 3000, at_rep, sizeof ( at_rep ) );
    if ( 0 == strcmp ( uciScanMode, "BAND_LOCK" ) )
    {
        if ( 0 != strcmp ( uciLocked4gBand, "" ) )
        {
            comd_strrpl ( uciLocked4gBand, " ", "_" );
            strcat ( uciLocked4gBand, "_" );
        }

        telitPlsParam_lockband ( uciLocked4gBand, 1 );
    }
    return 0;
}

int telitPlsParam_updatePinStatus(void)
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+CPIN?", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "+CPIN:" ) || strstr ( at_rep, "ERROR" ) )
        parsing_telit_pls_cpin_get ( at_rep );

    return 0;
}

int telitPlsParam_updatePinLockStatus(void)
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+CLCK=\"SC\",2", 3000, at_rep, sizeof ( at_rep ) );

    parsing_telit_pls_clck_get ( at_rep );

    return 0;
}

int telitPlsParam_updatePinPukCount(void)
{
    char at_rep[512] = {0};

    COMD_AT_PROCESS ( "AT^SPIC=\"SC\"", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "^SPIC: " ) )
        parsing_telit_pls_spic ( at_rep );

    return 0;
}

void telitPlsParam_updateImsi (void)
{
    char at_rep[64] = {0};
    char ram_val[16] = {0};

    nv_get ( "imsi", ram_val, sizeof ( ram_val ) );

    if ( 0 == strcmp ( ram_val, "" ) || 0 == strcmp ( ram_val, "--" ) )
    {
        COMD_AT_PROCESS ( "AT+CIMI", 3000, at_rep, sizeof ( at_rep ) );
        parsing_telit_pls_cimi ( at_rep );
        telitPlsParam_updateUsimMncLen();
    }
}

void telitPlsParam_updateSimSPN (void)
{
    char at_rep[128] = {0};
    char ram_val[64] = {0};

    nv_get ( "SIM_SPN", ram_val, sizeof ( ram_val ) );

    if ( 0 == strcmp ( ram_val, "" ) || 0 == strcmp ( ram_val, "--" ) )
    {
        COMD_AT_PROCESS ( "AT+CRSM=176,28486,0,0,17", 3000, at_rep, sizeof ( at_rep ) );

        if ( strstr ( at_rep, "+CRSM:" ) )
            parsing_telit_pls_sim_spn ( at_rep );
    }
}

void telitPlsParam_updateUsimMncLen (void)
{
    char at_rep[128] = {0};

    COMD_AT_PROCESS ( "AT+CRSM=176,28589,0,0,0", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "+CRSM:" ) )
        parsing_telit_pls_simMncLen ( at_rep );
}

void telitPlsParam_updateIccid (void)
{
    char at_rep[64] = {0};
    char ram_val[32] = {0};

    nv_get ( "iccid", ram_val, sizeof ( ram_val ) );

    if ( 0 == strcmp ( ram_val, "" ) || 0 == strcmp ( ram_val, "--" ) )
    {
        COMD_AT_PROCESS ( "AT+CRSM=176,12258,0,0,10", 1500, at_rep, sizeof ( at_rep ) );

        if ( strstr ( at_rep, "+CRSM:" ) )
            parsing_telit_pls_iccid ( at_rep );
    }
}

void telitPlsParam_updatePhoneNumber (void)
{
    char at_rep[64] = {0};
    char ram_val[32] = {0};

    nv_get ( "phone_number", ram_val, sizeof ( ram_val ) );

    if ( 0 == strcmp ( ram_val, "" ) || 0 == strcmp ( ram_val, "--" ) )
    {
        COMD_AT_PROCESS ( "AT+CNUM", 1500, at_rep, sizeof ( at_rep ) );

        if ( strcmp ( at_rep, "" ) )
            parsing_telit_pls_cnum ( at_rep );
    }
}

void telitPlsParam_updateCesq(void)
{
    char at_rep[STR_AT_RSP_LEN] = {0};

    COMD_AT_PROCESS ( "AT+CESQ", 3000, at_rep, sizeof ( at_rep ) );

    parsing_telit_pls_cesq ( at_rep );
}

void telitPlsParam_updateCops(void)
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+COPS?", 3000, at_rep, sizeof ( at_rep ) );
    if ( strcmp ( at_rep, "" ) )
        parsing_telit_pls_operator ( at_rep );
}

void telitPlsParam_updateCgatt (void)
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+CGATT?", 3000, at_rep, sizeof ( at_rep ) );

    if ( strcmp ( at_rep, "" ) )
        parsing_telit_pls_cgatt ( at_rep );
}

void telitPlsParam_updateServingCell ( char* data )
{
    char at_rep[RECV_BUF_SIZE] = {0};
    char network_state[8] = {0};
    int ret;

    //Note: Modify here after test endc mode

    COMD_AT_PROCESS ( "AT^SMONI", 3000, at_rep, sizeof ( at_rep ) );

    parsing_telit_pls_serving_4gcellinfo ( at_rep );

    memset(at_rep, 0, sizeof(at_rep));
    COMD_AT_PROCESS ( "AT+CSQ", 3000, at_rep, sizeof ( at_rep ) );
    parsing_telit_pls_csq ( at_rep );
}

static void telitPlsParam_updateSignalBar (void)
{
    char signal_s[8] = {0};
    int signal_i = 0;
    char ram_val[8] = {0};

    nv_get ( "mode", ram_val, sizeof ( ram_val ) );

    nv_get ( "rsrp0", signal_s, sizeof ( signal_s ) );

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
        if ( signal_i <= -85 )
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

}

void telitPlsParam_updateCgact (void)
{
    char at_rep[STR_AT_RSP_LEN] = {0};

    COMD_AT_PROCESS ( "AT+CGACT?", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "\r\nOK\r\n" ) )
        parsing_telit_pls_cgact ( at_rep );
}

int telitPlsParam_updateCgcontrdp ( int apn_index )
{
    char at_rep[RECV_BUF_SIZE * 2] = {0};
    char at_cmd[128] = {0};

    int ret = 0;
    if ( apn_index <= 0 )
    {
        COMD_AT_PROCESS ( "AT+CGCONTRDP", 3000, at_rep, sizeof ( at_rep ) );
        return;
    }
    else
    {
        snprintf(at_cmd, sizeof(at_cmd), "AT+CGCONTRDP=%d", apn_index);
        COMD_AT_PROCESS ( at_cmd, 3000, at_rep, sizeof ( at_rep ) );
    }
    ret = parsing_telit_pls_cgcontrdp ( apn_index, at_rep );
    return ret;
}

static void telitPlsParam_UpdateSwwan(void)
{
    char at_rep[RECV_BUF_SIZE] = {0};
    char at_cmd[128] = {0};

    int ret = 0;

    COMD_AT_PROCESS ( "AT^SWWAN?", 3000, at_rep, sizeof ( at_rep ) );
    if(!strstr(at_rep, "^SWWAN: 1,1,1"))
    {
        COMD_AT_PROCESS ( "AT^SWWAN=1,1", 3000, at_rep, sizeof ( at_rep ) );
    }

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

static void init_abnormal_check_config ()
{
    char conf_val[8] = {0};
    int i = 0;
    int conf_mask = 0;

    sys_get_config ( "lte_param.parameter.abnormal_check", conf_val, sizeof ( conf_val ) );

    if ( strcmp ( conf_val, "" ) )
    {
        conf_mask = atoi ( conf_val );

        for ( i = 0; i < TELIT_PLS_MAX_APN_NUM; i++ )
        {
            abnormal_check_flag[i] = ( conf_mask & ( 1 << i ) );
            CLOGD ( FINE, "abnormal_check_flag[%d]: [%d]\n", i, abnormal_check_flag[i] );
        }
    }
    else
    {
        for ( i = 0; i < TELIT_PLS_MAX_APN_NUM; i++ )
        {
            abnormal_check_flag[i] = 1;
            CLOGD ( FINE, "abnormal_check_flag[%d]: [%d]\n", i, abnormal_check_flag[i] );
        }
    }
}

static void telitPlsParam_radioRestart (void)
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+CFUN=0", 5000, at_rep, sizeof ( at_rep ) );

    comd_wait_ms ( 1500 );

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( "AT+CFUN=1", 5000, at_rep, sizeof ( at_rep ) );
}

static int telitPlsParam_checkAbnormal ( char* sim_stat, char* main_stat )
{
    int ret = 1;
    char radio_stat[8] = {0};
    char net_select_mode[32] = {0};
    char sim_lock_status[16] = {0};

    nv_get ( "lte_on_off", radio_stat, sizeof ( radio_stat ) );
    nv_get ( "net_select_mode", net_select_mode, sizeof ( net_select_mode ) );
    nv_get ( "simlocked", sim_lock_status, sizeof ( sim_lock_status ) );

    CLOGD ( FINE, "sim_stat: [%s], main_stat: [%s]\n", sim_stat, main_stat );
    CLOGD ( FINE, "cpin_error_check   -> [%d]\n", cpin_error_check );
    CLOGD ( FINE, "sim_lock_status   -> [%s]\n", sim_lock_status );
    CLOGD ( FINE, "main_status_check  -> [%d]\n", main_status_check );
    CLOGD ( FINE, "lte_on_off         -> [%s]\n", radio_stat );
    CLOGD ( FINE, "net_select_mode    -> [%s]\n", net_select_mode );

    if ( ( 0 == strcmp ( radio_stat, "off" ) ) || ( 0 == strcmp ( net_select_mode, "manual_select" ) ) )
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
                telitPlsParam_radioRestart();
                CLOGD ( ERROR, "exe AT+CFUN=0/1 for sim_not_ready !!!\n" );
                system ("/etc/syslog_save/criticalEvent.sh \"exe AT+CFUN=0/1 for sim_not_ready !!!\"");
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

    if ( 0 == strcmp ( sim_stat, "READY" ) && strcmp ( main_stat, "connected" ) && atoi(sim_lock_status) == 0 )
    {
        if ( abnormal_more_than_xxx_seconds ( main_status_check, 120 ) || main_status_check++ > 90 )
        {
            CLOGD ( FINE, "abnormal_check_flag[1]: [%d]\n", abnormal_check_flag[1] );
            if ( abnormal_check_flag[1] )
            {
                telitPlsParam_radioRestart ();
                CLOGD ( ERROR, "exe AT+CFUN=0/1 for main_status not connected !!!\n" );
                system ("/etc/syslog_save/criticalEvent.sh \"exe AT+CFUN=0/1 for main_status not connected !!!\"");
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

void telit_pls_normal_status_refresh (void)
{
    char cpin_value[16] = {0};
    char mStatusVal[32] = {0};
    char radio_stat[8] = {0};
    char simlock_stat[8] = {0};
    char apns_status[8] = {0};
    char cur_select_mode[32] = {0};
    char cesq_value[64] = {0};
    char cgatt_value[8] = {0};

    char time_str[16] = {0};
    char apn_enable[4] = {0};
    char strUciOpt[64] = {0};
    char apn_act_state[4] = {0};
    char apn_pdptype[8] = {0};
    char strRamOpt[64] = {0};
    char apn_connect_state_v4[16] = {0};
    char apn_connect_state_v6[16] = {0};

    char at_rep[256] = {0};

    int i = 0;
    int ret = 0;
    int cur_cfun_state = 0;
    telitPlsParam_updatePinStatus();
    telitPlsParam_updatePinLockStatus ();
    telitPlsParam_updatePinPukCount ();

    nv_get ( "cpin", cpin_value, sizeof ( cpin_value ) );
    nv_get ( "main_status", mStatusVal, sizeof ( mStatusVal ) );

    telitPlsParam_checkAbnormal ( cpin_value, mStatusVal );

    if ( 0 == strcmp ( cpin_value, "READY" ) )
    {
        telitPlsParam_updateImsi ();
        telitPlsParam_updateSimSPN ();
        telitPlsParam_getSimlock ( NULL );
        telitPlsParam_updateIccid ();
        telitPlsParam_updatePhoneNumber ();
        telitPlsParam_updatePDPreturnFormat();

        nv_get ( "simlocked", simlock_stat, sizeof ( simlock_stat ) );
        nv_get ( "lte_on_off", radio_stat, sizeof ( radio_stat ) );
        nv_get ( "net_select_mode", cur_select_mode, sizeof ( cur_select_mode ) );
        nv_get ( "setOnOff_lte", strRamOpt, sizeof ( strRamOpt ) );

        COMD_AT_PROCESS ( "AT+CFUN?", 3000, at_rep, sizeof ( at_rep ) );

        cur_cfun_state = parsing_telit_pls_cfun_get ( at_rep );

        nv_get ( "setOnOff_lte", strRamOpt, sizeof ( strRamOpt ) );
        if ( strncmp(strRamOpt,"-1",2) != 0 && strcmp ( simlock_stat, "1" ) == 0 && cur_cfun_state == 1)
        {
            COMD_AT_PROCESS ( "AT+CFUN=4", 3000, at_rep, sizeof ( at_rep ) );
        }
        else if ( strncmp(strRamOpt,"-1",2) != 0 && strcmp ( simlock_stat, "0" ) == 0 && cur_cfun_state == 0 && strcmp ( radio_stat, "on" ) == 0 )
        {
            COMD_AT_PROCESS ( "AT+CFUN=1", 3000, at_rep, sizeof ( at_rep ) );
        }

        telitPlsParam_updateCesq ();

        nv_get ( "cesq", cesq_value, sizeof ( cesq_value ) );

        if ( strcmp ( cesq_value, "" ) && strcmp ( cesq_value, "99,99,255,255,255,255" ) )
        {
            telitPlsParam_updateCops ();
            telitPlsParam_updateCgatt ();

            telitPlsParam_updateServingCell ( NULL );
            //teliParam_updateNeighbourCell ( );
            telitPlsParam_updateSignalBar ();

            nv_get ( "cgatt_val", cgatt_value, sizeof ( cgatt_value ) );
            CLOGD ( FINE, "cgatt_value -> [%s]\n", cgatt_value );
            if ( 0 == strcmp ( cgatt_value, "1" ) )
            {
                if ( 0 == strcmp ( simlock_stat, "1" ) )
                {
                    telitPlsParam_radioOnOffSet ( "off" );
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

                telitPlsParam_updateCgact ();

                int multi_apn_act = 0;


                for ( i = 1; i <= TELIT_PLS_MAX_APN_NUM; i++ )
                {
                    memset ( apn_enable, 0, sizeof ( apn_enable ) );
                    snprintf ( strUciOpt, sizeof ( strUciOpt ), "lte_param.apn%d.enable", i );
                    sys_get_config ( strUciOpt, apn_enable, sizeof ( apn_enable ) );
                    CLOGD ( FINE, "%s -> [%s]\n", strUciOpt, apn_enable );

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "cid_%d_state", i );
                    memset ( apn_act_state, 0, sizeof ( apn_act_state ) );
                    nv_get ( strRamOpt, apn_act_state, sizeof ( apn_act_state ) );
                    CLOGD ( FINE, "%s -> [%s]\n", strRamOpt, apn_act_state );

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_state", i );
                    nv_get ( strRamOpt, apn_connect_state_v4, sizeof ( apn_connect_state_v4 ) );
                    CLOGD ( FINE, "%s -> [%s]\n", strRamOpt, apn_connect_state_v4 );

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_state", i );
                    nv_get ( strRamOpt, apn_connect_state_v6, sizeof ( apn_connect_state_v6 ) );
                    CLOGD ( FINE, "%s -> [%s]\n", strRamOpt, apn_connect_state_v6 );

                    if ( 0 == strcmp ( apn_act_state, "1" ) )
                    {
                        if ( 0 == strcmp ( apn_enable, "1" ) )
                        {
                            COMD_AT_PROCESS ( "AT^SWWAN=1,1", 1500, strRamOpt, sizeof ( strRamOpt ) );
                            telitPlsParam_UpdateSwwan();
                            snprintf ( strUciOpt, sizeof ( strUciOpt ), LTE_APN_PDPTYPE, i );
                            sys_get_config ( strUciOpt, apn_pdptype, sizeof ( apn_pdptype ) );
                            if(((strcmp(apn_pdptype,"IP") == 0 || strcmp(apn_pdptype,"IPV4V6") == 0)) &&
                                strcmp(apn_connect_state_v4,"connect") != 0)
                            {
                                system("udhcpc -s /lib/netifd/telit-dhcp.script -i eth1.100 -q -n -A 5 -t 3 -T 5");
                            }
                            /*if (((strcmp(apn_pdptype,"IPV6") == 0 || strcmp(apn_pdptype,"IPV4V6") == 0)) &&
                                strcmp(apn_connect_state_v6,"connect") != 0)
                            {
                                system("odhcp6c -s /lib/netifd/telit-dhcpv6.script eth1");
                            }*/
                            ret = telitPlsParam_updateCgcontrdp ( i );
                            if(ret > 0)
                                apnActOrDeactMsgEvent ( i, 1 );
                        }
                        else
                        {
                            apnActOrDeactMsgEvent ( i, 0 );
                        }
                    }
                    else
                    {
                        telitPlsParam_updateCgcontrdp ( i );
                        apnActOrDeactMsgEvent ( i, 0 );
                        if ( 0 == strcmp ( apn_enable, "1" ) )
                        {
                            if ( 0 == multi_apn_act )
                            {
                                multi_apn_act = 1;
                                telitPlsParam_UpdateSwwan();
                                snprintf ( strUciOpt, sizeof ( strUciOpt ), LTE_APN_PDPTYPE, i );
                                sys_get_config ( strUciOpt, apn_pdptype, sizeof ( apn_pdptype ) );

                                if(strcmp(apn_pdptype,"IP") == 0 || strcmp(apn_pdptype,"IPV4V6") == 0)
                                {
                                    system("udhcpc -s /lib/netifd/telit-dhcp.script -i eth1 -q -n -A 5 -t 3 -T 5");
                                }
                                /*if (strcmp(apn_pdptype,"IPV6") == 0 || strcmp(apn_pdptype,"IPV4V6") == 0)
                                {
                                    system("odhcp6c -s /lib/netifd/telit-dhcpv6.script eth1");
                                }*/
                            }
                        }
                        else
                        {
                            apnActOrDeactMsgEvent ( i, 0 );
                        }
                    }
                }
            }
            else
            {
                apnActOrDeactMsgEvent ( 0, 0 );
                if ( 0 == strcmp ( simlock_stat, "0" ) && strcmp ( radio_stat, "off" ) && strcmp ( cur_select_mode, "manual_select" ) )
                {
                    telitPlsParam_connectNetwork ( NULL );
                    if ( strcmp ( mStatusVal, "searching" ) )
                    {
                        nv_set ( "main_status", "searching" );
                    }
                }
                else
                {
                    if ( strcmp ( mStatusVal, "disconnected" ) )
                    {
                        nv_set ( "main_status", "disconnected" );
                    }
                }
                goto END_S;
            }
        }
        else
        {
            apnActOrDeactMsgEvent ( 0, 0 );
            if ( 0 == strcmp ( simlock_stat, "0" ) && strcmp ( radio_stat, "off" ) && strcmp ( cur_select_mode, "manual_select" ) )
            {
                telitPlsParam_connectNetwork ( NULL );
                if ( strcmp ( mStatusVal, "searching" ) )
                {
                    nv_set ( "main_status", "searching" );
                }
            }
            else
            {
                if ( strcmp ( mStatusVal, "no_service" ) )
                {
                    nv_set ( "main_status", "no_service" );
                }
            }
            goto END;
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

    nv_set ( "secondary_cell",  "0" );

    ipv4v6_data_restore ( 0 );

}

int telit_pls_module_param_init()
{
    reset_apns_msg_flag ( 0 );
    init_abnormal_check_config ();
    char strRamOpt[64] = {0};
    int i = 0;
    cpin_error_check = 0;
    main_status_check = 0;
    abnormal_start_time = 0;

    if(
        telitPlsParam_setATEcho ( NULL ) ||
        telitPlsParam_setCfunMode ( "off" ) ||
        telitPlsParam_updateModuleVersion ( NULL ) ||
        telitPlsParam_updateImei ( NULL ) ||
        telitPlsParam_updateSN ( NULL ) ||
        telitPlsParam_updateDefaultGateway ( NULL ) ||
        telitPlsParam_updateSuppBands ( NULL ) ||
        telitPlsParam_updatePinStatus() ||
        telitPlsParam_updatePinLockStatus () ||
        telitPlsParam_updatePinPukCount () ||
        telitPlsParam_init_apn_netdev () ||
        telitPlsParam_initAPNsetting ( NULL ) ||
        telitPlsParam_initNetSelectMode ( NULL ) ||
        telitPlsParam_setCfunMode ( "on" ) ||
        telitPlsParam_radioLockInit ( NULL )
        )
    {
        return -1;
    }

    return 0;
}

