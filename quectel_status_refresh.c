#include "comd_share.h"
#include "atchannel.h"
#include "at_tok.h"
#include "quectel_status_refresh.h"
#include "quectel_atcmd_parsing.h"
#include "quectel_config_response.h"
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
extern char esim_supp[4];

static void quectelParam_radioRestart ()
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+CFUN=0", 5000, at_rep, sizeof ( at_rep ) );

    comd_wait_ms ( 1500 );

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( "AT+CFUN=1", 5000, at_rep, sizeof ( at_rep ) );
}

int quectelParam_GpsInit ( char* data )
{
    char at_req[16] = {0};
    char at_rep[64] = {0};
    char gps_enable[16] = {0};
    comd_exec ( "/lib/hw_def.sh get CONFIG_HW_GPS", gps_enable, sizeof ( gps_enable ) );
    CLOGD ( FINE, "CONFIG_HW_GPS -> %s\n", gps_enable );

    if( strncmp(gps_enable,"1",1) == 0 )
    {
        snprintf ( at_req, sizeof ( at_req ), "AT+QGPS?");
        COMD_AT_PROCESS ( at_req, 3000, at_rep, sizeof ( at_rep ) );
        if ( strstr ( at_rep, "+QGPS: 1" ) )
        {
            CLOGD ( FINE, "Already init Gps\n");
            return 0;
        }
        else
        {
            snprintf ( at_req, sizeof ( at_req ), "AT+QGPS=1" );

            COMD_AT_PROCESS ( at_req, 3000, at_rep, sizeof ( at_rep ) );

            if ( strstr ( at_rep, "\r\nOK\r\n" ) )
            {
                CLOGD ( FINE, "ATCMD GPS return OK\n");
                return 0;
            }
            else
            {
                CLOGD ( FINE, "ATCMD GPS return NOK\n" );
                return -1;
            }
        }
    }
    else
    {
        CLOGD ( FINE, "CPE is not supported GPS\n");
        return 0;
    }

    CLOGD ( ERROR, "Should not arrived here\n");
    return -1;
}

int quectelParam_setATEcho ( char* data )
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "ATE0", 3000, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
        return -1;

    return 0;
}

int quectelParam_setIpv6Format ( char* data )
{
    char at_rep[64] = {0};
    COMD_AT_PROCESS ( "AT+CGPIAF=1,1,0,0", 10000, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
        return -1;

    return 0;
}

int  quectelParam_updateUsbmode ( char* data )
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+QCFG=\"usbnet\"", 3000, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "\r\n+QCFG: \"usbnet\"" ) )
    {
        return -1;
    }

    if ( NULL == strstr ( at_rep, "\r\n+QCFG: \"usbnet\",0" ) )
    {
        memset ( at_rep, 0, sizeof( at_rep ) );
        COMD_AT_PROCESS ( "AT+QCFG=\"usbnet\",0", 3000, at_rep, sizeof ( at_rep ) );
        if ( strstr ( at_rep, "\r\nOK\r\n" ) )
        {
            comd_wait_ms ( 5000 );
            CLOGD ( WARNING, "Switch to GobiNet driver, reboot Quectel module !!!\n" );
            COMD_AT_PROCESS ( "AT+CFUN=1,1", 3000, at_rep, sizeof ( at_rep ) );
        }
        return -1;
    }

    return 0;
}

int  quectelParam_updatePciemode ( char* data )
{
    char module_type[32] = { 0 };
    nv_get("modulemodel", module_type, sizeof(module_type));

    if ( strcmp ( module_type, "RM505Q-AE" ) || strcmp ( module_type, "EG120K-EA" ) )
    {
        return 0;
    }

#if defined(_QCA_IPQ_)
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+QCFG=\"pcie_mbim\"", 3000, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "\r\n+QCFG: \"pcie_mbim\"" ) )
    {
        return -1;
    }

    if ( NULL == strstr ( at_rep, "\r\n+QCFG: \"pcie_mbim\",0" ) )
    {
        memset ( at_rep, 0, sizeof( at_rep ) );
        COMD_AT_PROCESS ( "AT+QCFG=\"pcie_mbim\",0", 3000, at_rep, sizeof ( at_rep ) );
        if ( strstr ( at_rep, "\r\nOK\r\n" ) )
        {
            comd_wait_ms ( 5000 );
            CLOGD ( WARNING, "Switch to GobiNet driver, reboot Quectel module !!!\n" );
            COMD_AT_PROCESS ( "AT+CFUN=1,1", 3000, at_rep, sizeof ( at_rep ) );
        }
        return -1;
    }
#endif

    return 0;
}

int quectelParam_radioLockCheck( char *earfcn )
{
    int i = 0;
    int ret = 0;
    char at_rep[64] = {0};
    char regex_buf[4][REGEX_BUF_ONE];

    char *pattern = NULL;

    for ( i = 0; i < 4; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    pattern = "\\+QNWCFG:[ ]*\"([^\"]*)\",([^,]*),([0-9]*).*$";

    for( i = 0; i < 2; i++ )
    {
        COMD_AT_PROCESS ( "AT+QNWCFG=\"lte_earfcn_lock\"", 3000, at_rep, sizeof ( at_rep ) );

        if( strstr ( at_rep, "\r\nOK\r\n" ) )
        {
            break;
        }
        memset ( at_rep, 0, sizeof( at_rep ) );
    }

    if( i == 2 )
    {
        CLOGD ( FINE,"module not support earfcn lock!\n");
        return 0;
    }

    if ( NULL == strstr ( at_rep, "\r\n+QNWCFG: \"lte_earfcn_lock\",0" ) )
    {
        if( earfcn != NULL )
        {
            ret = at_tok_regular_more ( strstr(at_rep,"+QNWCFG:"), pattern, regex_buf );
            CLOGD ( FINE,"ret = %d,regex_buf[2] = %s,regex_buf[3] = [%s]\n",ret,regex_buf[2],regex_buf[3]);

            if ( 0 == ret )
            {
                lib_replace_crlf_character(regex_buf[3]);

                if( strncmp( regex_buf[2], "1", 1 ) == 0 && strstr( earfcn, regex_buf[3] ) != NULL )
                {
                    return 1;
                }
            }
        }
        COMD_AT_PROCESS ( "AT+QNWCFG=\"lte_earfcn_lock\",0", 3000, at_rep, sizeof ( at_rep ) );
        return -1;
    }

    if( earfcn != NULL && strstr( earfcn, "4G" ) != NULL )
    {
        return -1;
    }

    return 0;
}

int quectelParam_radioLockInit ( char* data )
{
    char uciScanMode[16] = {0};
    char uciLocked5gBand[128] = {0};
    char uciLocked4gBand[128] = {0};
    char uciLockedBand[256] = {0};
    char uciLockedFreq[256] = {0};
    char uciLockedPci[256] = {0};

    char at_rep[32] = {0};
    char *ptr = NULL;
    int ret = 0;

    sys_get_config ( WAN_LTE_LOCK_MODE, uciScanMode, sizeof ( uciScanMode ) );

    if( 0 != strcmp ( uciScanMode, "FREQ_LOCK" ) )
    {
        ret = quectelParam_radioLockCheck( NULL );
    }

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

        quectelParam_lockband ( uciLockedBand, 0 );
    }
    else if ( 0 == strcmp ( uciScanMode, "FREQ_LOCK" ) )
    {
        sys_get_config ( WAN_LTE_LOCK_FREQ, uciLockedFreq, sizeof ( uciLockedFreq ) );

        if ( 0 != strcmp ( uciLockedFreq, "" ) )
        {
            // "42001 42003" -> "42001_42003_"
            CLOGD ( FINE, "locked_freq -> [%s]\n", uciLockedFreq );
            ptr = strchr( uciLockedFreq, ' ');
            if ( NULL != ptr )
            {
                uciLockedFreq[ ptr - uciLockedFreq ] = '\0';
                CLOGD ( FINE, "locked_freq -> [%s]\n", uciLockedFreq );
                sys_set_config ( WAN_LTE_LOCK_FREQ, uciLockedFreq );
                system("uci commit lte_param 2>/dev/null");
            }
            strcat ( uciLockedFreq, "_" );
            ret = quectelParam_radioLockCheck( uciLockedFreq );
            if( ret == 1 )
            {
                nv_set ( "lockearfcn_status", "locked" );
                return 0;
            }

            quectelParam_lockearfcn ( uciLockedFreq );
        }
    }
    else if ( 0 == strcmp ( uciScanMode, "PCI_LOCK" ) )
    {
        sys_get_config ( WAN_LTE_LOCK_PCI, uciLockedPci, sizeof ( uciLockedPci ) );

        if ( 0 != strcmp ( uciLockedPci, "" ) )
        {
            // "42005,3 43012,1" -> "42005,3_43012,1_"
            CLOGD ( FINE, "locked_pci -> [%s]\n", uciLockedPci );
            ptr = strchr( uciLockedPci, ' ');
            if ( NULL != ptr )
            {
                uciLockedPci[ ptr - uciLockedPci ] = '\0';
                CLOGD ( FINE, "locked_pci -> [%s]\n", uciLockedPci );
                sys_set_config ( WAN_LTE_LOCK_PCI, uciLockedPci );
                system("uci commit lte_param 2>/dev/null");
            }
            strcat ( uciLockedPci, "_" );

            quectelParam_lockpci ( uciLockedPci );
        }
    }
    else if ( NULL == data )
    {
        quectelParam_lockband ( "", 0 );
    }
    else if ( 0 == strcmp ( data, "restore_fullband" ) )
    {
        quectelParam_lockband ( "", 1 );
    }

    if( ret )
    {
        COMD_AT_PROCESS ( "AT+CFUN=1,1", 3000, at_rep, sizeof ( at_rep ) );
    }

    return 0;
}

int quectelParam_initNetSelectMode ( char* data )
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

int quectelParam_updateImei ( char* data )
{
    unsigned int check_times = 0;
    char imei_val[32] = {0};
    char at_rep[STR_AT_RSP_LEN] = {0};

    while ( check_times++ < 5 )
    {
        if ( 0 == COMD_AT_PROCESS ( "AT+EGMR=0,7", 1500, at_rep, sizeof ( at_rep ) ) )
        {
            parsing_quectel_imei ( at_rep );
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

int quectelParam_updateSN ( char* data )
{
    unsigned int check_times = 0;
    char sn_val[32] = {0};
    char at_rep[STR_AT_RSP_LEN] = {0};

    while ( check_times++ < 5 )
    {
        if (0 == COMD_AT_PROCESS ( "AT+EGMR=0,5", 1500, at_rep, sizeof ( at_rep ) ) )
        {
            parsing_quectel_sn ( at_rep );
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

int quectelParam_updateModuleModel ( char* data )
{
    char at_rep[STR_AT_RSP_LEN] = {0};

    COMD_AT_PROCESS ( "AT+CGMM", 1500, at_rep, sizeof ( at_rep ) );

    return parsing_quectel_moduleModel ( at_rep );
}

int quectelParam_updateModuleVersion ( char* data )
{
    unsigned int check_times = 0;
    char at_rep[128] = {0};

    while ( check_times++ < 5 )
    {
        if ( 0 == COMD_AT_PROCESS ( "ATI", 1500, at_rep, sizeof ( at_rep ) ) )
        {
            if ( 0 == parsing_quectel_moduleModel ( at_rep ) &&
                 0 == parsing_quectel_moduleVersion ( at_rep ) )
            {
                return 0;
            }
        }
        comd_wait_ms ( 1000 );
        memset ( at_rep, 0, sizeof ( at_rep ) );
    }

    return 0; // do not return -1, even can not get module info
}

int quectelParam_updateSuppBands ( char* data )
{

    char module_type[32] = {0};
    char *substr = NULL;
    char at_rep[RECV_BUF_SIZE] = {0};
    char lte_bands[256] = {0};
    char nr5g_bands[256] = {0};

    char old_suppband[128] = {0};
    char old_suppband5g[128] = {0};

    char *stored_path = "lte_param.parameter.support_band";
    char *stored_path5g = "lte_param.parameter.support_band5g";

    nv_get ( "modulemodel", module_type, sizeof ( module_type ) );

    snprintf ( lte_bands, sizeof ( lte_bands ), "%s",
                            strstr ( module_type, "RM500Q-GL" ) ? QUECTEL_RM500Q_GL_4G_BANDS : (
                            strstr ( module_type, "RM505Q-AE" ) ? QUECTEL_RM505Q_AE_4G_BANDS : (
                            strstr ( module_type, "RM520N-GL" ) ? QUECTEL_RM520N_GL_4G_BANDS : (
                            strstr ( module_type, "RG520F-EU" ) ? QUECTEL_RG520F_EU_4G_BANDS : (
                            strstr ( module_type, "RG520F-NA" ) ? QUECTEL_RG520F_NA_4G_BANDS : (
                            strstr ( module_type, "RG500Q-EA" ) ? QUECTEL_RG500Q_EA_4G_BANDS : (
                            strstr ( module_type, "RG500Q-NA" ) ? QUECTEL_RG500Q_NA_4G_BANDS : (
                            strstr ( module_type, "EM160R-GL" ) ? QUECTEL_EM160R_GL_4G_BANDS : ""
                        ) ) ) ) ) ) )
                    );

    snprintf ( nr5g_bands, sizeof ( nr5g_bands ), "%s",
                            strstr ( module_type, "RM500Q-GL" ) ? QUECTEL_RM500Q_GL_5G_BANDS : (
                            strstr ( module_type, "RM505Q-AE" ) ? QUECTEL_RM505Q_AE_5G_BANDS : (
                            strstr ( module_type, "RM520N-GL" ) ? QUECTEL_RM520N_GL_5G_BANDS : (
                            strstr ( module_type, "RG520F-EU" ) ? QUECTEL_RG520F_EU_5G_BANDS : (
                            strstr ( module_type, "RG520F-NA" ) ? QUECTEL_RG520F_NA_5G_BANDS : (
                            strstr ( module_type, "RG500Q-EA" ) ? QUECTEL_RG500Q_EA_5G_BANDS : (
                            strstr ( module_type, "RG500Q-NA" ) ? QUECTEL_RG500Q_NA_5G_BANDS : (
                            strstr ( module_type, "EM160R-GL" ) ? QUECTEL_EM160R_GL_5G_BANDS : ""
                        ) ) ) ) ) ) )
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
            system ( "uci commit lte_param 2>/dev/null" );
        }
        if ( strcmp ( old_suppband5g, nr5g_bands ) )
        {
            CLOGD ( WARNING, "Save supported bands to flash !\n" );
            sys_set_config ( stored_path5g, nr5g_bands );
            system ( "uci commit lte_param 2>/dev/null" );
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
        else if ( strstr ( substr, "+QNWPREFCFG: \"nr5g_band\"," ) )
        {
            snprintf ( nr5g_bands, sizeof ( nr5g_bands ), "%s", substr );
        }
        substr = strtok ( NULL, "\r\n" );
    }

    return ( parsing_quectel_supp4gband ( lte_bands ) & parsing_quectel_supp5gband ( nr5g_bands ) );
}

int quectelParam_updateCGDCONT ( char* data )
{
    char at_rep[RECV_BUF_SIZE * 2] = {0};

    COMD_AT_PROCESS ( "AT+CGDCONT?", 3000, at_rep, sizeof ( at_rep ) );
    if ( strstr ( at_rep, "\r\n+CGDCONT:" ) )
    {
        return parsing_quectel_apn ( at_rep );
    }

    return -1;
}

void quectelParam_updateQICSGP ( char* data )
{
    int i = 1;
    char at_req[16] = {0};
    char at_rep[256] = {0};

    for ( ; i <= 4; i++ )
    {
        snprintf ( at_req, sizeof ( at_req ), "AT+QICSGP=%d", ( 1 == i ) ? i : ( i + APN_OFFSET ) );
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( at_req, 3000, at_rep, sizeof ( at_rep ) );
        parsing_quectel_apnAndAuth ( i, at_rep );
    }

    return;
}

static void quectelParam_updateApnConfig ( int update_type )
{
    switch ( update_type )
    {
    case 0:
        quectelParam_updateCGDCONT ( NULL );
        break;
    case 1:
        quectelParam_updateQICSGP ( NULL );
        break;
    default:
        CLOGD ( ERROR, "unknown update_type -> [%d]\n", update_type );
        break;
    }

    return;
}

int quectelParam_initAPNsetting ( char* data )
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
    char module_type[64]={0};

    nv_get("modulemodel", module_type, sizeof(module_type));
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

#if defined(_QCA_X55_) && defined(RG500)
        if ( 1 == i )
        {
            snprintf ( at_cmd, sizeof ( at_cmd ), "AT+COPS=2" );
        }
        else
        {
#if defined(QUECTEL_X55_QCMAP_EN)
            snprintf ( at_cmd, sizeof ( at_cmd ), "AT+QETH=\"rgmii\",\"disable\",0,0,%d", i );
#else
            snprintf ( at_cmd, sizeof ( at_cmd ), "AT+CGACT=0,%d", i );
#endif
        }

        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( at_cmd, 3000, at_rep, sizeof ( at_rep ) );
#elif defined(CONFIG_SW_QUECTEL_X62)
        CLOGD( FINE, "CONFIG_SW_QUECTEL_X62 do nothing before APN set.\n" );
#else
        snprintf ( strRamOpt, sizeof ( strRamOpt ), "/usr/sbin/KT_x55_CM.sh stop %d quectel-CM", i );
        system ( strRamOpt );
#endif

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

            if(strstr(module_type,"EM160R-GL"))
            {
                memset(at_cmd,0,sizeof(at_cmd));
                snprintf ( at_cmd, sizeof ( at_cmd ), "AT+CGDCONT=%d,\"%s\",\"%s\"",
                           ( 1 == i ) ? i : ( i + APN_OFFSET ),
                           uci_pdptype,
                           uci_apnname
                    );

                memset ( at_rep, 0, sizeof ( at_rep ) );
                COMD_AT_PROCESS ( at_cmd, 3000, at_rep, sizeof ( at_rep ) );
            }

#if defined(_QCA_X55_) && defined(RG500)
#elif defined(CONFIG_SW_QUECTEL_X62)
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
                    quectelParam_configQmapConnect ( i , 0 );
                }
            }

            if ( 2 == i )
            {
                quectelParam_setQCPDPIMSCFGE ( NULL );
            }
#else
            if ( 1 == i )
            {
                snprintf ( at_cmd, sizeof ( at_cmd ), "AT+COPS=2" );
                memset ( at_rep, 0, sizeof ( at_rep ) );
                COMD_AT_PROCESS ( at_cmd, 3000, at_rep, sizeof ( at_rep ) );

                snprintf ( at_cmd, sizeof ( at_cmd ), "AT+COPS=0" );
                memset ( at_rep, 0, sizeof ( at_rep ) );
                COMD_AT_PROCESS ( at_cmd, 3000, at_rep, sizeof ( at_rep ) );
            }
            #if 0
            snprintf ( strRamOpt, sizeof ( strRamOpt ), "/usr/sbin/KT_x55_CM.sh start %d quectel-CM", i );
            system ( strRamOpt );
            #endif
#endif
        }
        else
        {
            snprintf ( at_cmd, sizeof ( at_cmd ), "AT+CGDCONT=%d", ( 1 == i ) ? i : ( i + APN_OFFSET ) );
            COMD_AT_PROCESS ( at_cmd, 3000, at_rep, sizeof ( at_rep ) );
        }
    }

    return 0;
}

int quectelParam_updateDualSim ()
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
            quectelParam_updatePinStatus ();
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

void quectelParam_updatePinStatus ()
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+CPIN?", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "+CPIN:" ) || strstr ( at_rep, "ERROR" ) )
        parsing_quectel_cpin_get ( at_rep );
}

void quectelParam_updatePinLockStatus ()
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+CLCK=\"SC\",2", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "+CLCK:" ) )
        parsing_quectel_clck_get ( at_rep );
}

void quectelParam_updatePinPukCount ()
{
    char at_rep[256] = {0};

    COMD_AT_PROCESS ( "AT+QPINC?", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "+QPINC:" ) )
        parsing_quectel_qpinc ( at_rep );
}

void quectelParam_updateImsi ()
{
    char at_rep[64] = {0};
    char ram_val[16] = {0};

    nv_get ( "imsi", ram_val, sizeof ( ram_val ) );

    if ( 0 == strcmp ( ram_val, "" ) || 0 == strcmp ( ram_val, "--" ) )
    {
        COMD_AT_PROCESS ( "AT+CIMI", 3000, at_rep, sizeof ( at_rep ) );

        if ( strcmp ( at_rep, "" ) )
            parsing_quectel_cimi ( at_rep );

        quectelParam_updateUsimMncLen ();
        quectelParam_setIpv6Format ( NULL );
    }
}

void quectelParam_updateIccid ()
{
    char at_rep[64] = {0};
    char ram_val[32] = {0};

    nv_get ( "iccid", ram_val, sizeof ( ram_val ) );

    if ( 0 == strcmp ( ram_val, "" ) || 0 == strcmp ( ram_val, "--" ) )
    {
        COMD_AT_PROCESS ( "AT+CRSM=176,12258,0,0,10", 1500, at_rep, sizeof ( at_rep ) );

        if ( strstr ( at_rep, "+CRSM:" ) )
            parsing_quectel_iccid ( at_rep );
    }
}

void quectelParam_updatePhoneNumber ()
{
    char at_rep[64] = {0};
    char ram_val[32] = {0};

    nv_get ( "phone_number", ram_val, sizeof ( ram_val ) );

    if ( 0 == strcmp ( ram_val, "" ) || 0 == strcmp ( ram_val, "--" ) )
    {
        COMD_AT_PROCESS ( "AT+CNUM", 1500, at_rep, sizeof ( at_rep ) );

        if ( strcmp ( at_rep, "" ) )
            parsing_quectel_cnum ( at_rep );
    }
}

void quectelParam_updateUsimMncLen ()
{
    char at_rep[128] = {0};

    COMD_AT_PROCESS ( "AT+CRSM=176,28589,0,0,0", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "+CRSM:" ) )
        parsing_quectel_simMncLen ( at_rep );
}

void quectelParam_updateSimSPN ()
{
    char at_rep[128] = {0};
    char ram_val[64] = {0};

    nv_get ( "SIM_SPN", ram_val, sizeof ( ram_val ) );

    if ( 0 == strcmp ( ram_val, "" ) || 0 == strcmp ( ram_val, "--" ) )
    {
        COMD_AT_PROCESS ( "AT+CRSM=176,28486,0,0,17", 3000, at_rep, sizeof ( at_rep ) );

        if ( strstr ( at_rep, "+CRSM:" ) )
            parsing_quectel_sim_spn ( at_rep );
    }
}

void quectelParam_updateCreg ()
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+CREG?", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "\r\n+CREG:" ) )
        parsing_quectel_creg ( at_rep );
}

void quectelParam_updateC5greg ()
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+C5GREG?", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "\r\n+C5GREG:" ) )
        parsing_quectel_c5greg (at_rep);
}

void quectelParam_updateCsq ()
{
    char at_rep[STR_AT_RSP_LEN] = {0};

    COMD_AT_PROCESS ( "AT+CSQ", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "+CSQ:" ) )
        parsing_quectel_csq ( at_rep );
}

void quectelParam_updateCgatt ()
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+CGATT?", 3000, at_rep, sizeof ( at_rep ) );

    if ( strcmp ( at_rep, "" ) )
        parsing_quectel_cgatt ( at_rep );
}

void quectelParam_updateEthRgmii ()
{
    char at_rep[STR_AT_RSP_LEN_2X] = {0};

    COMD_AT_PROCESS ( "AT+QETH=\"rgmii\"", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "\r\nOK\r\n" ) )
        parsing_quectel_ethrgmii ( at_rep );
}

int quectelParam_configEthRgmii ( int cid_index, int act_val )
{
    char at_req[64] = {0};
    char at_rep[64] = {0};

    snprintf ( at_req, sizeof ( at_req ), "AT+QETH=\"rgmii\",\"%s\",0,0,%d",
                                                    ( 1 == act_val ) ? "enable" : "disable", cid_index );

    COMD_AT_PROCESS ( at_req, 5000, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
        return -1;

    return 0;
}

void quectelParam_updateCgact ()
{
    char at_rep[STR_AT_RSP_LEN] = {0};

    COMD_AT_PROCESS ( "AT+CGACT?", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "\r\nOK\r\n" ) )
        parsing_quectel_cgact ( at_rep );
}

int quectelParam_configCgact ( int cid_index, int act_val )
{
    char at_req[16] = {0};
    char at_rep[128] = {0};

    snprintf ( at_req, sizeof ( at_req ), "AT+CGACT=%d,%d", act_val, cid_index );
    COMD_AT_PROCESS ( at_req, 3000, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
        return -1;

    return 0;
}

void quectelParam_updateCops ()
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+COPS?", 3000, at_rep, sizeof ( at_rep ) );
    if ( strcmp ( at_rep, "" ) )
        parsing_quectel_operator ( at_rep );
}

int quectelParam_updateCgcontrdp ( int apn_index )
{
    char at_rep[RECV_BUF_SIZE] = {0};
    char at_cmd[32] = {0};

    snprintf ( at_cmd, sizeof ( at_cmd ), "AT+CGCONTRDP=%d", apn_index );

    COMD_AT_PROCESS ( at_cmd, 8000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "\r\nOK\r\n" ) )
        return parsing_quectel_cgcontrdp ( apn_index, at_rep );

    return 0;
}

void quectelParam_updateRrcState ()
{
    char at_rep[RECV_BUF_SIZE] = {0};

    COMD_AT_PROCESS ( "AT+QNWCFG=\"rrc_state\"", 3000, at_rep, sizeof ( at_rep ) );

    if ( strcmp ( at_rep, "" ) )
    {
        parsing_quectel_rrc_state ( at_rep );
    }
}

void quectelParam_updateServingCell ( char* data )
{
    char at_rep[RECV_BUF_SIZE] = {0};
    char mode_type[64] = {0};

    COMD_AT_PROCESS ( "AT+QENG=\"servingcell\"", 3000, at_rep, sizeof ( at_rep ) );

    if ( strcmp ( at_rep, "" ) )
    {
        parsing_quectel_serving_cellinfo ( at_rep );
    }

    nv_get ( "mode", mode_type, sizeof ( mode_type ) );
    if( 0 == strcmp ( mode_type, "5G" ) || 0 == strcmp ( mode_type, "ENDC" ) )
    {
        quectelParam_updateNR5Gtxpower ( NULL );
    }else{
        nv_set ( "5g_txpower", "--" );
    }
}

int quectelParam_setCfunMode ( char* mode )
{
    char cfunSet_ret[4] = {0};

    quectelParam_radioOnOffSet ( mode );

    nv_get ( "setOnOff_lte", cfunSet_ret, sizeof ( cfunSet_ret ) );

    return strcmp ( cfunSet_ret, "0" ) ? -1 : 0;
}

int quectelParam_routerModeInit ( char* data )
{
    char at_rep[RECV_BUF_SIZE] = {0};
    char module_model[16] = {0};
    char lte_mode[16] = {0};
    char router_type[4] = {0};

    nv_get ( "modulemodel", module_model, sizeof ( module_model ) );

    if ( 0 == strcmp ( module_model, "RM505Q-AE" ) || 0 == strcmp ( module_model, "RM500Q-GL" ) )
    {
        COMD_AT_PROCESS ( "AT+QLOGMASK=\"DPL\"", 3000, at_rep, sizeof ( at_rep ) );
        parsing_quectel_qlogmask ( at_rep );

        nv_get ( "router_type", router_type, sizeof ( router_type ) );
        sys_get_config ( "lte_param.parameter.lte_mode", lte_mode, sizeof ( lte_mode ) );

        if ( 0 == strcmp ( lte_mode, "router" ) && 0 == strcmp ( router_type, "0" ) )
        {
            COMD_AT_PROCESS ( "AT+QLOGMASK=\"DPL\",1", 3000, at_rep, sizeof ( at_rep ) );
            comd_wait_ms ( 5000 );
            CLOGD ( WARNING, "Switch to router mode, reboot Quectel module !!!\n" );
            COMD_AT_PROCESS ( "AT+CFUN=1,1", 3000, at_rep, sizeof ( at_rep ) );
        }
        else if ( 0 != strcmp ( lte_mode, "router" ) && 0 == strcmp ( router_type, "1" ) )
        {
            COMD_AT_PROCESS ( "AT+QLOGMASK=\"DPL\",0", 3000, at_rep, sizeof ( at_rep ) );
            comd_wait_ms ( 5000 );
            CLOGD ( WARNING, "Switch to router mode, reboot Quectel module !!!\n" );
            COMD_AT_PROCESS ( "AT+CFUN=1,1", 3000, at_rep, sizeof ( at_rep ) );
        }
    }
    else if ( 0 == strcmp ( module_model, "RM520N-GL" ) )
    {
        COMD_AT_PROCESS ( "AT+QROUTINGBH?", 3000, at_rep, sizeof ( at_rep ) );
        if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
        {
            CLOGD ( INFO, "module not support QROUTINGBH!!!\n" );
            return 0;
        }

        sys_get_config ( "lte_param.parameter.lte_mode", lte_mode, sizeof ( lte_mode ) );
        if ( 0 == strcmp ( lte_mode, "router" ) && NULL != strstr ( at_rep, "QROUTINGBH: 0\r\n" ) )
        {
            COMD_AT_PROCESS ( "AT+QROUTINGBH=1", 3000, at_rep, sizeof ( at_rep ) );
            comd_wait_ms ( 1000 );
            CLOGD ( WARNING, "Switch to router mode, reboot Quectel module !!!\n" );
            COMD_AT_PROCESS ( "AT+CFUN=1,1", 3000, at_rep, sizeof ( at_rep ) );
        }
        else if ( 0 != strcmp ( lte_mode, "router" ) && NULL != strstr ( at_rep, "QROUTINGBH: 1\r\n" ) )
        {
            COMD_AT_PROCESS ( "AT+QROUTINGBH=0", 3000, at_rep, sizeof ( at_rep ) );
            comd_wait_ms ( 1000 );
            CLOGD ( WARNING, "Switch to other mode, reboot Quectel module !!!\n" );
            COMD_AT_PROCESS ( "AT+CFUN=1,1", 3000, at_rep, sizeof ( at_rep ) );
        }
    }

    return 0;
}

#if defined ( CONFIG_SW_QUECTEL_X62 )
int quectelParam_setVlanMac ( char* data )
{
    char at_req[128] = {0};
    char at_rep[512] = {0};
    char ram_val[16] = {0};
    char vlan_mac[32] = {0};

    int i = 3;

    for ( ; i >= 0; i-- )
    {
        snprintf ( at_req, sizeof ( at_req ), "AT+QMAP=\"mPDN_rule\",%d", i );
        COMD_AT_PROCESS ( at_req, 8000, at_rep, sizeof ( at_rep ) );

        snprintf ( ram_val, sizeof ( ram_val ), "eth1.10%d_mac", i );
        memset ( vlan_mac, 0, sizeof ( vlan_mac ) );
        nv_get ( ram_val, vlan_mac, sizeof ( vlan_mac ) );

        snprintf ( at_req, sizeof ( at_req ), "AT+QMAP=\"mPDN_rule\",%d,%d,%d,1,%d,\"%s\"",
                          i, i + 1, i + 100, ( i > 0 ) ? 0 : 1, vlan_mac );
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( at_req, 8000, at_rep, sizeof ( at_rep ) );

        comd_wait_ms ( 300 );
    }

    return 0;
}

int quectelParam_setVolteIms ( char* data )
{
    char at_rep[512] = {0};

    COMD_AT_PROCESS ( "AT+QCFG=\"ims\"", 3000, at_rep, sizeof ( at_rep ) );
    if ( NULL == strstr ( at_rep, "+QCFG: \"ims\"" ) )
    {
        return -1;
    }

    if ( NULL == strstr ( at_rep, "+QCFG: \"ims\",2" ) )
    {
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( "AT+QCFG=\"ims\",2", 3000, at_rep, sizeof ( at_rep ) );
        if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
        {
            return -1;
        }
    }

    return 0;
}

int quectelParam_setQCPDPIMSCFGE ( char* data )
{
    char at_req[32] = {0};
    char at_rep[64] = {0};

    /*
     * ims need request pcscf address
     * here APN2 is used for ims as default
     */
    snprintf ( at_req, sizeof ( at_req ), "AT\$QCPDPIMSCFGE=%d,1", 2 );
    COMD_AT_PROCESS ( at_req, 3000, at_rep, sizeof ( at_rep ) );

    return strstr ( at_rep, "\r\nOK\r\n" ) ? 0 : 1;
}

int quectelParam_configQmapConnect ( int cid_index, int act_val )
{
    char at_req[64] = {0};
    char at_rep[64] = {0};

    snprintf ( at_req, sizeof ( at_req ), "AT+QMAP=\"connect\",%d,%d", cid_index - 1, act_val );

    COMD_AT_PROCESS ( at_req, 5000, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
        return -1;

    return 0;
}
#endif

int quectelParam_updateDefaultGateway ( char* data )
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

void quectelParam_updateQcainfo ()
{
    char at_rep[RECV_BUF_SIZE] = {0};
    int i = 0;
    char strRamOpt[32] = {0};

    COMD_AT_PROCESS ( "AT+QCAINFO", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "SCC" ) )
    {
        parsing_quectel_qcainfo ( at_rep );
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

            snprintf ( strRamOpt, sizeof ( strRamOpt ), "5g_S%d_dl_frequency", i );
            nv_set ( strRamOpt, "" );

            snprintf ( strRamOpt, sizeof ( strRamOpt ), "5g_S%d_ul_frequency", i );
            nv_set ( strRamOpt, "" );
        }

    }

    return;
}
static void quectelParam_updateSignalBar ()
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

int quectelParam_initCsiCtrl( void )
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+QNWCFG=\"csi_ctrl\",1,1", 3000, at_rep, sizeof ( at_rep ) );
    if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
    {
        CLOGD ( ERROR, "Initial CSI CTRL failed\n" );
        return -1;
    }
    return 0;
}

void quectelParam_updateCsiInfo( void )
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
        parsing_quectel_csiinfo ( at_rep );
    }
}

int quectelParam_initTxPowerLimitSet ( char* data )
{
    char txpowlmt_val[8] = {0};
    char at_req_4g[64] = {0};
    char at_rep_4g[64] = {0};
    char at_req_5g[64] = {0};
    char at_rep_5g[64] = {0};
    char  *ptr = NULL;

    if ( NULL == data )
    {
        nv_get("txpower_limit", txpowlmt_val, sizeof(txpowlmt_val));
    }
    else
    {
        snprintf ( txpowlmt_val, sizeof ( txpowlmt_val ), "%s", data );
    }

    ptr = strtok(txpowlmt_val,",");
    if( ptr != NULL)
    {
        ptr=strtok(NULL,",");
    }

    if( ptr != NULL)
    {
        snprintf ( txpowlmt_val, sizeof ( txpowlmt_val ), "%s", ptr );
    }

    /**
     * AT+QMTPLCFG :the range of txpower control is 4-23.
     * the range of module txpower is 0-26
     */
    CLOGD ( FINE, "setTxPowerLimit: [%s]\n", txpowlmt_val );
    if ( atoi(txpowlmt_val) > 25 || !strcmp( txpowlmt_val, "") || atoi(txpowlmt_val) == 0 )
        strcpy( txpowlmt_val, "" );
    else if (atoi(txpowlmt_val) > 23 && atoi(txpowlmt_val) <= 25)
        strcpy( txpowlmt_val, "23" );
    else if( atoi(txpowlmt_val) <=4 && atoi(txpowlmt_val) >=1 )
        strcpy( txpowlmt_val, "4" );


    CLOGD ( FINE, "setTxPowerLimit: [%s]\n", txpowlmt_val );

    /**
    * 6:NR;4:lte
    * 0:disable;1:enable
    */
    if ( !strcmp ( txpowlmt_val, "" ) )
    {
        snprintf ( at_req_5g, sizeof ( at_req_5g ), "%s%s,%s,%s", "AT+QMTPLCFG=", "6","0","230" );
        snprintf ( at_req_4g, sizeof ( at_req_4g ), "%s%s,%s,%s", "AT+QMTPLCFG=", "4","0","230" );
    }
    else
    {
        snprintf ( at_req_5g, sizeof ( at_req_5g ), "%s%s,%s,%s0", "AT+QMTPLCFG=", "6","1",txpowlmt_val );
        snprintf ( at_req_4g, sizeof ( at_req_4g ), "%s%s,%s,%s0", "AT+QMTPLCFG=", "4","1",txpowlmt_val );
    }

    COMD_AT_PROCESS ( at_req_4g, 1500, at_rep_4g, sizeof ( at_rep_4g ) );
    quectelparsing_txpowlmt( at_rep_4g );

    COMD_AT_PROCESS ( at_req_5g, 1500, at_rep_5g, sizeof ( at_rep_5g ) );
    quectelparsing_txpowlmt( at_rep_5g );

    if ( NULL == strstr ( at_rep_4g, "\r\nOK\r\n" ) && NULL == strstr ( at_rep_5g, "\r\nOK\r\n" ) )
        //return -1;
        return 0;

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

static void quectel_init_apn_netdev ()
{
#if defined(_MTK_7981_)
    char wan_if[64] = {0};

    sys_get_config ( LTE_PARAMETER_WAN_IF, wan_if, sizeof ( wan_if ) );

    if ( strstr ( wan_if, "usb0" ) )
    {
        nv_set ( "apn1_netdev", "usb0.1" );
        nv_set ( "apn2_netdev", "usb0.2" );
        nv_set ( "apn3_netdev", "usb0.3" );
        nv_set ( "apn4_netdev", "usb0.4" );
    }
    else if ( strstr ( wan_if, "eth1" ) )
    {
        nv_set ( "apn1_netdev", "eth1.100" );
        nv_set ( "apn2_netdev", "eth1.101" );
        nv_set ( "apn3_netdev", "eth1.102" );
        nv_set ( "apn4_netdev", "eth1.103" );
    }
#endif
}

int quectel_module_param_init ()
{
    quectel_init_apn_netdev ();
    reset_apns_msg_flag ( 0 );

    init_abnormal_check_config ();

    cpin_error_check = 0;
    main_status_check = 0;
    abnormal_start_time = 0;

    if (
        quectelParam_setATEcho ( NULL ) ||
        quectelParam_setCfunMode ( "off" ) ||
        quectelParam_updateModuleVersion ( NULL ) ||
        quectelParam_routerModeInit ( NULL ) ||
#if defined(CONFIG_SW_QUECTEL_X62)
        quectelParam_setVolteIms ( NULL ) ||
        quectelParam_setVlanMac ( NULL ) ||
#endif
        quectelParam_updateImei ( NULL ) ||
        quectelParam_updateSN ( NULL ) ||
        quectelParam_updateDefaultGateway ( NULL ) ||
        quectelParam_updateSuppBands ( NULL ) ||
        quectelParam_updateUsbmode ( NULL ) ||
        quectelParam_updatePciemode ( NULL ) ||
        quectelParam_initAPNsetting ( NULL ) ||
        quectelParam_lteRoamingSet ( NULL ) ||
        quectelParam_initNetSelectMode ( NULL ) ||
        quectelParam_setCfunMode ( "on" ) ||
        quectelParam_setServicePSCSMode ( NULL ) ||
        quectelParam_GpsInit ( NULL ) ||
        quectelParam_initTxPowerLimitSet( NULL ) ||
        quectelParam_initCsiCtrl () ||
        quectelParam_radioLockInit ( NULL ) ||
        quectelParam_setRatPriority ( NULL ) ||
        quectelParam_initNR5Gtxpower ( NULL )
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

static int quectelParam_checkAbnormal ( char* sim_stat, char* main_stat )
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
                quectelParam_radioRestart ();
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

    if ( 0 == strcmp ( sim_stat, "READY" ) && strcmp ( main_stat, "connected" ) && atoi(sim_lock_status) == 0)
    {
        if ( abnormal_more_than_xxx_seconds ( main_status_check, 120 ) || main_status_check++ > 90 )
        {
            CLOGD ( FINE, "abnormal_check_flag[1]: [%d]\n", abnormal_check_flag[1] );
            if ( abnormal_check_flag[1] )
            {
                quectelParam_radioRestart ();
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

void quectel_normal_status_refresh()
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
    int ret = 1;
    char radio_stat[8] = {0};
    char simlock_stat[4] = {0};
    char apns_status[4] = {0};
    char cur_select_mode[32] = {0};
    char module_type[64] = { 0 };
    int cur_cfun_state = 0;
    char at_rep[32] = {0};

    quectelParam_updatePinStatus ();
    quectelParam_updatePinLockStatus ();
    quectelParam_updatePinPukCount ();

    nv_get ( "cpin", cpin_value, sizeof ( cpin_value ) );
    nv_get ( "main_status", mStatusVal, sizeof ( mStatusVal ) );

    if ( 0 == strncmp ( dualsim_en, "1", 1 ) && 1 == quectelParam_updateDualSim () && g_simslot_switch_event == 1)
    {
        g_simslot_switch_event = 0;
        comd_wait_ms ( 10 * 1000 );
        quectelParam_initAPNsetting ( NULL );
        quectelParam_radioLockInit ( "restore_fullband" );
        nv_set ( "imsi", "--" );
        nv_set ( "SIM_SPN", "--" );
        return;
    }

    quectelParam_checkAbnormal ( cpin_value, mStatusVal );

    if ( 0 == strcmp ( cpin_value, "READY" ) )
    {
        quectelParam_updateImsi ();
        quectelParam_updateSimSPN ();
        quectelParam_getSimlock ( NULL );
        quectelParam_updateIccid ();
        quectelParam_updatePhoneNumber ();
        nv_get ( "simlocked", simlock_stat, sizeof ( simlock_stat ) );
        nv_get ( "lte_on_off", radio_stat, sizeof ( radio_stat ) );
        nv_get ( "net_select_mode", cur_select_mode, sizeof ( cur_select_mode ) );
        nv_get ( "setOnOff_lte", strRamOpt, sizeof ( strRamOpt ) );

        COMD_AT_PROCESS ( "AT+CFUN?", 3000, at_rep, sizeof ( at_rep ) );

        cur_cfun_state = parsing_quectel_cfun_get(at_rep);

        if ( strncmp(strRamOpt,"-1",2) != 0 && strcmp ( simlock_stat, "1" ) == 0 && cur_cfun_state == 1)
        {
            COMD_AT_PROCESS ( "AT+CFUN=4", 3000, at_rep, sizeof ( at_rep ) );
        }
        else if ( strncmp(strRamOpt,"-1",2) != 0 && strcmp ( simlock_stat, "0" ) == 0 && cur_cfun_state == 0 && strcmp ( radio_stat, "on" ) == 0 )
        {
            COMD_AT_PROCESS ( "AT+CFUN=1", 3000, at_rep, sizeof ( at_rep ) );
        }


        quectelParam_updateCsq ();
        nv_get ( "csq", csq_value, sizeof ( csq_value ) );

        if ( strcmp ( csq_value, "" ) && strcmp ( csq_value, "99,99" ) )
        {
            quectelParam_updateCreg ();
            quectelParam_updateCgatt ();
            nv_get ( "cgatt_val", cgatt_value, sizeof ( cgatt_value ) );
C5GREG_LOOP:
            quectelParam_updateCops ();
            quectelParam_updateRrcState ();
            quectelParam_updateServingCell ( NULL );
            quectelParam_updateSignalBar ();
            quectelParam_updateQcainfo ();
            nv_get("modulemodel", module_type, sizeof(module_type));
            if(strstr(module_type,"EM160R-GL"))
            {
                quectelParam_updateApnConfig ( 0 );
            }
            else
                quectelParam_updateApnConfig ( 1 );

            CLOGD ( FINE, "cgatt_value -> [%s]\n", cgatt_value );
            if ( 0 == strcmp ( cgatt_value, "1" ) )
            {
                if ( 0 == strcmp ( simlock_stat, "1" ) )
                {
                    quectelParam_disconnectNetwork ( NULL );
                    return;
                }

                quectelParam_updateCsiInfo();

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

#if defined(CONFIG_SW_QUECTEL_X62)
                quectelParam_updateCgact ();
#else
#endif
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
#if defined(CONFIG_SW_QUECTEL_X62)
                            ret = quectelParam_updateCgcontrdp ( i );
#else
                            snprintf ( strRamOpt, sizeof ( strRamOpt ), "usb0.%d_net_down", i );
                            nv_get ( strRamOpt, apns_status, sizeof ( apns_status ) );
                            CLOGD ( FINE, "usb0.%d_net_down [%s]\n", i, apns_status );

                            if ( 0 == strcmp ( apns_status, "1" ) )
                            {
                                apns_msg_flag[i] = 0;
                                nv_set ( strRamOpt, "0" );
                            }
#endif
                            if ( 0 < ret )
                                apnActOrDeactMsgEvent ( i, 1 );
                        }
                        else
                        {
#if defined(CONFIG_SW_QUECTEL_X62)
#else
                            snprintf ( strRamOpt, sizeof ( strRamOpt ), "/usr/sbin/KT_x55_CM.sh stop %d quectel-CM", i );
                            system ( strRamOpt );
#endif
                        }
                    }
                    else
                    {
#if defined(CONFIG_SW_QUECTEL_X62)
                        parsing_quectel_cgcontrdp ( i, "" );
#else
#endif
                        apnActOrDeactMsgEvent ( i, 0 );
                        if ( 0 == strcmp ( apn_enable, "1" ) )
                        {
                            if ( 0 == multi_apn_act )
                            {
                                comd_wait_ms ( 1500 );
                                multi_apn_act = 1;
#if defined(CONFIG_SW_QUECTEL_X62)
                                quectelParam_configQmapConnect ( i , 1 );
#else
                                snprintf ( strRamOpt, sizeof ( strRamOpt ), "/usr/sbin/KT_x55_CM.sh start %d quectel-CM", i );
                                system ( strRamOpt );
#endif
                            }
                        }
                        else
                        {
#if defined(CONFIG_SW_QUECTEL_X62)
#else
                            snprintf ( strRamOpt, sizeof ( strRamOpt ), "/usr/sbin/KT_x55_CM.sh stop %d quectel-CM", i );
                            system ( strRamOpt );
#endif
                        }
                    }
                }
            }
            else
            {
                if ( 0 == strcmp ( simlock_stat, "0" ) && strcmp ( radio_stat, "off" ) && strcmp ( cur_select_mode, "manual_select" ) )
                {
                    quectelParam_connectNetwork ( NULL );
                }

                apnActOrDeactMsgEvent ( 0, 0 );


                if( 0 == quectelParam_updatePlmnSearchStatus() )
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
            quectelParam_updateC5greg ();
            nv_get ( "c5greg_stat", c5greg_stat, sizeof ( c5greg_stat ) );
            CLOGD ( FINE, "c5greg_stat -> [%s]\n", c5greg_stat );
            quectelParam_updateCgatt ();
            nv_get ( "cgatt_val", cgatt_value, sizeof ( cgatt_value ) );
            CLOGD ( FINE, "cgatt_value -> [%s]\n", cgatt_value );
            if ( 0 == strcmp ( cgatt_value, "1" ) || 0 == strcmp ( c5greg_stat, "1" ) || 0 == strcmp ( c5greg_stat, "5" ) )
            {
                goto C5GREG_LOOP;
            }

            if ( 0 == strcmp ( simlock_stat, "0" ) && strcmp ( radio_stat, "off" ) && strcmp ( cur_select_mode, "manual_select" ) )
            {
                quectelParam_connectNetwork ( NULL );
            }

            apnActOrDeactMsgEvent ( 0, 0 );

            if( strcmp ( radio_stat, "on" ) == 0 && quectelParam_updatePlmnSearchStatus() == 0 )
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

void quectel_install_mode_refresh ()
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
int quectelParam_updatePlmnSearchStatus(void)
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

int quectelParam_initNR5Gtxpower ( char* data )
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+QNWCFG=\"nr5g_tx_pwr\",1", 3000, at_rep, sizeof ( at_rep ) );
    if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
    {
        CLOGD ( ERROR, "Initial 5G TxPower failed\n" );
    }
    return 0;
}

int quectelParam_updateNR5Gtxpower ( char* data )
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+QNWCFG=\"nr5g_tx_pwr\"", 3000, at_rep, sizeof ( at_rep ) );
    if ( strstr ( at_rep, "\r\nOK\r\n" ) )
    {
        parsing_quectel_NR5Gtxpower ( at_rep );
    }

    return 0;
}
