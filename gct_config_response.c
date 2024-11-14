#include "comd_share.h"
#include "atchannel.h"
#include "gct_config_response.h"
#include "gct_status_refresh.h"
#include "gct_atcmd_parsing.h"
#include "config_key.h"
#include "hal_platform.h"

extern int apns_msg_flag[5];

enum
{
    PDN_MANAGER_EVENT,
    MAX_EVENT_NUM
};

static void pdn_manager_event_process ( char* evtCMD )
{
#if defined(_GDM7243_)
    char nic_name[16] = {0};
    char pdn_state[4] = {0};
    int apn_index = 0;
    char *evtParam = NULL;

    evtParam = strtok ( evtCMD, "," );
    if ( NULL == evtParam )
    {
        CLOGD ( ERROR, "evtParam is NULL !!!\n" );
        return;
    }
    snprintf ( nic_name, sizeof ( nic_name ), "%s", evtParam );

    evtParam = strtok ( NULL, "," );
    if ( NULL == evtParam )
    {
        CLOGD ( ERROR, "evtParam is NULL !!!\n" );
        return;
    }
    snprintf ( pdn_state, sizeof ( pdn_state ), "%s", evtParam );

    if ( 0 == strcmp ( nic_name, "lte0pdn0" ) )
    {
        apn_index = 1;
    }
    else if ( 0 == strcmp ( nic_name, "lte0pdn1" ) )
    {
        apn_index = 2;
    }
    else if ( 0 == strcmp ( nic_name, "lte0pdn2" ) )
    {
        apn_index = 3;
    }
    else if ( 0 == strcmp ( nic_name, "lte0pdn3" ) )
    {
        apn_index = 4;
    }
    else
    {
        CLOGD ( ERROR, "Unknown nic_name [%s] !!!\n", nic_name );
        return;
    }

    CLOGD ( FINE, "nic_name [%s], pdn_state [%s]\n", nic_name, pdn_state );

    if ( 0 == strcmp ( pdn_state, "0" ) )
    {
        if ( 1 == apn_index )
        {
            apnActOrDeactMsgEvent ( 0, 0 );
        }
        else
        {
            apnActOrDeactMsgEvent ( apn_index, 0 );
        }
    }
#endif
}

void gctParam_lteEventNotify ( char* reqData )
{
    if ( NULL == reqData )
    {
        CLOGD ( ERROR, "reqData is NULL !!!\n" );
        return;
    }

    char evtContent[64] = {0};
    char *evtParam = NULL;
    char evtID[4] = {0};
    char evtCMD[64] = {0};

    snprintf ( evtContent, sizeof ( evtContent ), "%s", reqData );

    evtParam = strtok ( evtContent, "/" );
    if ( NULL == evtParam )
    {
        CLOGD ( ERROR, "evtParam is NULL !!!\n" );
        return;
    }
    snprintf ( evtID, sizeof ( evtID ), "%s", evtParam );

    evtParam = strtok ( NULL, "/" );
    if ( NULL == evtParam )
    {
        CLOGD ( ERROR, "evtParam is NULL !!!\n" );
        return;
    }
    snprintf ( evtCMD, sizeof ( evtCMD ), "%s", evtParam );

    switch ( atoi ( evtID )  )
    {
    case PDN_MANAGER_EVENT:
        pdn_manager_event_process ( evtCMD );
        break;
    default:
        break;
    }
}

void gctParam_lockPin ( char* reqData )
{
    char pEnable[4] = {0};
    char *pinCode = NULL;
    char at_rep[32] = {0};
    char pin_clck[64] = {0};

    if ( NULL == reqData )
    {
        CLOGD ( ERROR, "reqData is NULL !!!\n" );
        goto END;
    }

    pinCode = strstr ( reqData, "," );
    if ( NULL == pinCode )
    {
        CLOGD ( ERROR, "pinCode is NULL !!!\n" );
        goto END;
    }

    strncpy ( pEnable, reqData, pinCode - reqData );
    snprintf ( pin_clck, sizeof(pin_clck),
                "AT+CLCK=\"SC\",%s,\"%s\"", pEnable, pinCode + 1 );

    COMD_AT_PROCESS ( pin_clck, 1500, at_rep, sizeof ( at_rep ) );

    gctParam_updatePinStatus ();
    gctParam_updatePinLockStatus ();
    gctParam_updatePinPukCount ();

END:
    parsing_gct_lockpin ( at_rep );
}

void gctParam_enterPin ( char* reqData )
{
    char at_rep[32] = {0};
    char pin_unlock[32] = {0};

    if ( NULL == reqData )
    {
        CLOGD ( ERROR, "reqData is NULL !!!\n" );
        goto END;
    }

    snprintf ( pin_unlock, sizeof(pin_unlock), "AT+CPIN=\"%s\"", reqData );

    COMD_AT_PROCESS ( pin_unlock, 1500, at_rep, sizeof ( at_rep ) );

    gctParam_updatePinStatus ();
    gctParam_updatePinPukCount ();

END:
    parsing_gct_enterpin ( at_rep );

    // manual search after unlock PIN ok
    if ( strstr ( at_rep, "\r\nOK\r\n" ) )
    {
        gctParam_plmnSearchManualSelect ( "" );
    }
}

void gctParam_modifyPin ( char* reqData )
{
    char oldCode[16] = {0};
    char *newCode = NULL;
    char at_rep[32] = {0};
    char pin_cpwd[64] = {0};

    if ( NULL == reqData )
    {
        CLOGD ( ERROR, "reqData is NULL !!!\n" );
        goto END;
    }

    newCode = strstr ( reqData, "," );
    if ( NULL == newCode )
    {
        CLOGD ( ERROR, "newCode is NULL !!!\n" );
        goto END;
    }

    strncpy ( oldCode, reqData, newCode - reqData );
    snprintf ( pin_cpwd, sizeof(pin_cpwd),
                "AT+CPWD=\"SC\",\"%s\",\"%s\"", oldCode, newCode + 1 );

    COMD_AT_PROCESS ( pin_cpwd, 1500, at_rep, sizeof ( at_rep ) );

    gctParam_updatePinStatus ();
    gctParam_updatePinPukCount ();

END:
    parsing_gct_modifypin ( at_rep );
}

void gctParam_enterPuk ( char* reqData )
{
    char pukCode[16] = {0};
    char *newPinCode = NULL;
    char at_rep[32] = {0};
    char pin_update[64] = {0};

    if ( NULL == reqData )
    {
        CLOGD ( ERROR, "reqData is NULL !!!\n" );
        goto END;
    }

    newPinCode = strstr ( reqData, "," );
    if ( NULL == newPinCode )
    {
        CLOGD ( ERROR, "newPinCode is NULL !!!\n" );
        goto END;
    }

    strncpy ( pukCode, reqData, newPinCode - reqData );
    snprintf ( pin_update, sizeof(pin_update),
                "AT+CPIN=\"%s\",\"%s\"", pukCode, newPinCode + 1 );

    COMD_AT_PROCESS ( pin_update, 1500, at_rep, sizeof ( at_rep ) );

    gctParam_updatePinStatus ();
    gctParam_updatePinPukCount ();

END:
    parsing_gct_enterpuk ( at_rep );

    // manual search after unlock PUK ok
    if ( strstr ( at_rep, "\r\nOK\r\n" ) )
    {
        gctParam_plmnSearchManualSelect ( "" );
    }
}

void gctParam_config_commandsetting ( char* reqData )
{
    char at_rep[RECV_SMS_SIZE] = {0};

    if ( NULL == reqData ) {
        CLOGD ( ERROR, "reqData is NULL !!!\n" );
        goto END;
    }

    LogWebShellSet ( reqData );
    LogWebShellSet ( "\r\n" );

    if ( 0 == strncasecmp ( reqData, "AT", 2 ) )
    {
        COMD_AT_PROCESS ( reqData, 10000, at_rep, sizeof ( at_rep ) );
    }
#if 0
    else
    {
        comd_exec ( reqData, at_rep, sizeof ( at_rep ) );
    }
#endif
    LogWebShellSet ( at_rep );
    LogWebShellSet ( "\r\n" );

    CLOGD( FINE, "web_shell_command OK\n" );

END:
    nv_set ( "web_command_flag", "0" );
}

void gctParam_netSelectModeSet ( char* reqData )
{
    char cur_select_mode[32] = {0};
    char engineer_mode[4] = {0};
    char at_req[64] = {0};
    char at_rep[32] = {0};

    if ( NULL == reqData )
    {
        CLOGD ( ERROR, "reqData is NULL !!!\n" );
        nv_set ( "net_select_mode_set", "1" );
        return;
    }

    nv_get ( "net_select_mode", cur_select_mode, sizeof ( cur_select_mode ) );
    if ( 0 == strcmp ( reqData, cur_select_mode ) )
    {
        nv_set ( "net_select_mode_set", "0" );
        return;
    }

    nv_set("net_select_mode", reqData);
    nv_get("is_project_page", engineer_mode, sizeof(engineer_mode));

    if ( strcmp ( engineer_mode, "yes" ) )
    {
        ucfg_info_set ( "config wan lte autocm manual",
                        strcmp ( reqData, "auto_select" ) ? "1" : "0" );
    }

    snprintf( at_req, sizeof ( at_req ), "AT+CGATT=%s",
                strcmp ( reqData, "auto_select" ) ? "0" : "1" );

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( at_req, 5000, at_rep, sizeof ( at_rep ) );

    nv_set ( "net_select_mode_set", "0" );

    gctParam_plmnSearchManualSelect ( "" );
}

void gctParam_connectNetwork ( char* reqData )
{
    char at_rep[32] = {0};

    COMD_AT_PROCESS ( "AT+CGATT=1", 10000, at_rep, sizeof ( at_rep ) );

    nv_set ( "connect_network_set", strstr ( at_rep, "\r\nOK\r\n" ) ? "0" : "1" );
}

void gctParam_disconnectNetwork ( char* reqData )
{
    char at_rep[32] = {0};

    COMD_AT_PROCESS ( "AT+CGATT=0", 5000, at_rep, sizeof ( at_rep ) );

    nv_set ( "disconnect_network_set", strstr ( at_rep, "\r\nOK\r\n" ) ? "0" : "1" );

    if ( NULL == reqData )
        gctParam_plmnSearchManualSelect ( "" );
}

static int save_apn_value ( apn_profile* const apnSetting_data )
{
    char apnEnabled[4] = {0};
    char apnProfileName[32] = {0};
    char apnName[32] = {0};
    char apnPdpType[8] = {0};
    char apnAuthType[4] = {0};
    char authUserName[32] = {0};
    char authPasswd[32] = {0};
    char apnBandMac[32] = {0};
    char apn_config_file[128] = {0};
    char apn_ram_file[64] = {0};
    int ret = 0;
    int cid_index = 0;

    cid_index = get_cid_from_apn_index ( apnSetting_data->index + 1 );

    snprintf ( apn_ram_file, sizeof ( apn_ram_file ),
                    "apn%d_enable", apnSetting_data->index + 1 );
    nv_get ( apn_ram_file, apnEnabled, sizeof ( apnEnabled ) );
    if ( strcmp ( apnEnabled, apnSetting_data->apn_enable ) )
    {
        ret = 1;
        /*
         * apn_enable nvram value update is moved to gctParam_apnSetting
         * if update too early, some confusion will happen
         * gct_normal_status_refresh may act it before APN config
         */
        //nv_set ( apn_ram_file, apnSetting_data->apn_enable );
        snprintf ( apn_config_file, sizeof ( apn_config_file ),
                        "config wan lte apntable apn%d ENABLE", cid_index );
        ucfg_info_set ( apn_config_file, apnSetting_data->apn_enable );
    }

    snprintf ( apn_ram_file, sizeof ( apn_ram_file ),
                    "profile_name%d", apnSetting_data->index + 1 );
    nv_get ( apn_ram_file, apnProfileName, sizeof ( apnProfileName ) );
    if ( strcmp ( apnProfileName, apnSetting_data->profile_name ) )
    {
        nv_set ( apn_ram_file, apnSetting_data->profile_name );
        snprintf ( apn_config_file, sizeof ( apn_config_file ),
                        "config wan lte apntable apn%d profile_name", cid_index );
        ucfg_info_set ( apn_config_file, apnSetting_data->profile_name );
    }

    snprintf ( apn_ram_file, sizeof ( apn_ram_file ),
                    "apn%d_name", apnSetting_data->index + 1 );
    nv_get ( apn_ram_file, apnName, sizeof ( apnName ) );
    if ( strcmp ( apnName, apnSetting_data->apn_name ) )
    {
        ret = 1;
        nv_set ( apn_ram_file, apnSetting_data->apn_name );
        snprintf ( apn_config_file, sizeof ( apn_config_file ),
                        "config wan lte apntable apn%d apn_name", cid_index );
        ucfg_info_set ( apn_config_file, apnSetting_data->apn_name );
    }

    snprintf ( apn_ram_file, sizeof ( apn_ram_file ),
                    "pdn%d_type", apnSetting_data->index + 1 );
    nv_get ( apn_ram_file, apnPdpType, sizeof ( apnPdpType ) );
    if ( strcmp ( apnPdpType, apnSetting_data->pdn_type ) )
    {
        ret = 1;
        nv_set ( apn_ram_file, apnSetting_data->pdn_type );
        snprintf ( apn_config_file, sizeof ( apn_config_file ),
                        "config wan lte apntable apn%d pdn_type", cid_index );
        ucfg_info_set ( apn_config_file, apnSetting_data->pdn_type );
    }

    snprintf ( apn_ram_file, sizeof ( apn_ram_file ),
                    "auth_type%d", apnSetting_data->index + 1 );
    nv_get ( apn_ram_file, apnAuthType, sizeof ( apnAuthType ) );
    if ( strcmp ( apnAuthType, apnSetting_data->auth_type ) )
    {
        ret = 1;
        nv_set ( apn_ram_file, apnSetting_data->auth_type );
        snprintf ( apn_config_file, sizeof ( apn_config_file ),
                        "config wan lte apntable apn%d auth_flag", cid_index );
        if ( strcmp ( apnSetting_data->auth_type, "2" ) )
            ucfg_info_set ( apn_config_file, "0" );
        else
            ucfg_info_set ( apn_config_file, "1" );
    }

    snprintf ( apn_ram_file, sizeof ( apn_ram_file ),
                    "user_name%d", apnSetting_data->index + 1 );
    nv_get ( apn_ram_file, authUserName, sizeof ( authUserName ) );
    if ( strcmp ( authUserName, apnSetting_data->user_name ) )
    {
        ret = 1;
        nv_set ( apn_ram_file, apnSetting_data->user_name );
        snprintf ( apn_config_file, sizeof ( apn_config_file ),
                        "config wan lte apntable apn%d username", cid_index );
        ucfg_info_set ( apn_config_file, apnSetting_data->user_name );
    }

    snprintf ( apn_ram_file, sizeof ( apn_ram_file ),
                    "password%d", apnSetting_data->index + 1 );
    nv_get ( apn_ram_file, authPasswd, sizeof ( authPasswd ) );
    if ( strcmp ( authPasswd, apnSetting_data->password ) )
    {
        ret = 1;
        nv_set ( apn_ram_file, apnSetting_data->password );
        snprintf ( apn_config_file, sizeof ( apn_config_file ),
                        "config wan lte apntable apn%d password", cid_index );
        ucfg_info_set ( apn_config_file, apnSetting_data->password );
    }

    snprintf ( apn_ram_file, sizeof ( apn_ram_file ),
                    "band_mac%d", apnSetting_data->index + 1 );
    nv_get ( apn_ram_file, apnBandMac, sizeof ( apnBandMac ) );
    if ( strcmp ( apnBandMac, apnSetting_data->band_mac ) )
    {
        nv_set ( apn_ram_file, apnSetting_data->band_mac );
        snprintf ( apn_config_file, sizeof ( apn_config_file ),
                        "config wan lte apntable apn%d band_mac", cid_index );
        ucfg_info_set ( apn_config_file, apnSetting_data->band_mac );
    }

    return ret;
}

void gctParam_apnSetting ( apn_profile* const apnSetting_data )
{
    int apn_index = 0;
    int cid_index = 0;
    char at_req[128] = {0};
    char at_rep[64] = {0};
    char apn_ram_file[64] = {0};

    if ( NULL == apnSetting_data )
    {
        CLOGD ( ERROR, "apnSetting_data is NULL !!!\n" );
        nv_set ( "apn_set", "1" );
        return;
    }

    if ( 0 == save_apn_value ( apnSetting_data ) )
    {
        nv_set ( "apn_set", "0" );
        return;
    }

    apn_index = apnSetting_data->index + 1;
    cid_index = get_cid_from_apn_index ( apn_index );

    if ( 3 == cid_index )
    {
        COMD_AT_PROCESS ( "AT+CGATT=0", 5000, at_rep, sizeof ( at_rep ) );
    }
    else
    {
        snprintf ( at_req, sizeof ( at_req ), "AT+CGACT=0,%d", cid_index );
        COMD_AT_PROCESS ( at_req, 3000, at_rep, sizeof ( at_rep ) );
    }

    if ( 0 == strcmp ( apnSetting_data->apn_enable, "1" ) )
    {
        int pdntype = atoi ( apnSetting_data->pdn_type );
        snprintf ( at_req, sizeof ( at_req ), "AT+CGDCONT=%d,\"%s\",\"%s\"",
                    cid_index,
                    ( 0 == pdntype ) ? "IP" : ( ( 1 == pdntype ) ? "IPV6" : "IPV4V6" ),
                    apnSetting_data->apn_name
            );

        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( at_req, 3000, at_rep, sizeof ( at_rep ) );

        snprintf ( at_req, sizeof ( at_req ), "AT+CGAUTH=%d,%s,\"%s\",\"%s\"",
                    cid_index, apnSetting_data->auth_type,
                    apnSetting_data->user_name, apnSetting_data->password
            );

        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( at_req, 3000, at_rep, sizeof ( at_rep ) );

        if ( 3 == cid_index )
        {
            COMD_AT_PROCESS ( "AT+CGATT=1", 3000, at_rep, sizeof ( at_rep ) );
        }
    }
    else
    {
        snprintf ( at_req, sizeof ( at_req ), "AT+CGDCONT=%d", cid_index );
        COMD_AT_PROCESS ( at_req, 3000, at_rep, sizeof ( at_rep ) );
    }

    snprintf ( apn_ram_file, sizeof ( apn_ram_file ), "apn%d_enable", apn_index );
    nv_set ( apn_ram_file, apnSetting_data->apn_enable );

    nv_set ( "apn_set", "0" );
}

#define MAX_LOCK_BAND_NUM 16
#define MAX_LOCK_EARFCN_NUM 8
#define MAX_LOCK_PCI_NUM 8
#define MAX_RESTORE_BAND_NUM 8

/*
 * reqData -> NULL
 *      restore to all supported bands
 *
 * reqData -> pcilock_value or earfcnlock_value
 *      earfcnlock_value: "3340_43190_"
 *      pcilock_value: "3340,7_43190,42_"
 *      restore to bands which earfcns locked belong
 */
static void gctRestoreToFullBand ( char* reqData )
{
    int i = 0;
    int j = 0;
    char at_rep[64] = {0};
    char set_full_band[128] = "AT%GSETSBD=0";
    char supp_band_val[128] = {0};
    char supp_band_num[4] = {0};
    int suppband_count = 0;
    int suppband_i_array[32] = {0};
    char lockearfcn_s_array[128] = {0};
    int lockearfcn_count = 0;
    int lockearfcn_i_array[MAX_RESTORE_BAND_NUM] = {0};
    char lockband_s_array[128] = {0};
    int lockband_count = 0;
    int lockband_i_array[MAX_RESTORE_BAND_NUM] = {0};

    nv_get ( "suppband_org", supp_band_val, sizeof ( supp_band_val ) );
    nv_get ( "suppband_num", supp_band_num, sizeof ( supp_band_num ) );

    if ( strcmp ( supp_band_val, "" ) && strcmp ( supp_band_num, "0" ) )
    {
        if ( NULL == reqData )
        {
            snprintf ( set_full_band, sizeof ( set_full_band ), "%s=%s,%s",
                                        "AT%GSETSBD", supp_band_num, supp_band_val );
        }
        else
        {
            char* tmp = strtok ( supp_band_val, "," );
            while ( tmp && suppband_count < sizeof ( suppband_i_array ) / sizeof ( int ) )
            {
                suppband_i_array[suppband_count++] = atoi ( tmp );
                tmp = strtok ( NULL, "," );
            }

            snprintf ( lockearfcn_s_array, sizeof ( lockearfcn_s_array ), "%s", reqData );
            tmp = strtok ( lockearfcn_s_array, "_" );
            while ( tmp && lockearfcn_count < MAX_RESTORE_BAND_NUM )
            {
                lockearfcn_i_array[lockearfcn_count++] = atoi ( tmp );
                tmp = strtok ( NULL, "_" );
            }

            for ( i = 0; i < lockearfcn_count; i++ )
            {
                lockband_i_array[lockband_count] = calc_band_from_earfcn ( lockearfcn_i_array[i],
                                                                (int*)&suppband_i_array, suppband_count );

                if ( 0 == lockband_i_array[lockband_count] )
                    continue;

                for ( j = 0; j < lockband_count; j++ )
                {
                    if ( lockband_i_array[lockband_count] == lockband_i_array[j] )
                        break;
                }

                if ( j == lockband_count )
                {
                    snprintf ( lockband_s_array + strlen ( lockband_s_array ),
                                    sizeof ( lockband_s_array ) - strlen ( lockband_s_array ),
                                                    ",%d", lockband_i_array[lockband_count++] );
                }
            }

            snprintf ( set_full_band, sizeof ( set_full_band ), "%s=%d%s",
                                            "AT%GSETSBD", lockband_count, lockband_s_array );
        }
    }

    COMD_AT_PROCESS ( set_full_band, 1500, at_rep, sizeof ( at_rep ) );
}

void gctParam_lockband ( char* reqData )
{
    char band_value[MAX_LOCK_BAND_NUM * 4] = {0};
    char lockearfcn_status[16] = {0};
    char lockpci_status[16] = {0};
    char lockband_cmd[MAX_LOCK_BAND_NUM * 4 + 16] = {0};
    char at_rep[64] = {0};

    if ( NULL == reqData )
    {
        CLOGD ( ERROR, "reqData is NULL !!!\n" );
        nv_set ( "lockband_set", "1" );
        return;
    }

    gctParam_radioOnOffSet ( "off" );
    nv_set ( "lockband_list", reqData );

    if ( strcmp ( reqData, "" ) )
    {
        nv_set ( "lockband_status", "locked" );
        strncpy ( band_value, reqData, sizeof ( band_value ) );

        char *tmp = strchr ( band_value, '_' );
        int num = 0;
        for ( ; NULL != tmp && num < MAX_LOCK_BAND_NUM; num++ )
        {
            band_value [ tmp - band_value ] = ',';
            tmp = strchr ( tmp + 1, '_' );
        }
        band_value [ strlen ( band_value ) - 1 ] = '\0';

        snprintf ( lockband_cmd, sizeof ( lockband_cmd ), "%s=%d,%s", "AT%GSETSBD", num, band_value );
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( lockband_cmd, 1500, at_rep, sizeof ( at_rep ) );
    }
    else
    {
        nv_set ( "lockband_status", "notlocked" );
        gctRestoreToFullBand ( NULL );
    }

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( "AT%GLOCKCELL=1,0,-1", 1500, at_rep, sizeof ( at_rep ) );

    nv_get ( "lockearfcn_status", lockearfcn_status, sizeof ( lockearfcn_status ) );
    nv_get ( "lockpci_status", lockpci_status, sizeof ( lockpci_status ) );
    if ( 0 == strcmp ( lockearfcn_status, "locked" ) ||
            0 == strcmp ( lockpci_status, "locked" ) )
    {
        nv_set ( "lockearfcn_status", "notlocked" );
        nv_set ( "lockearfcn_list", "" );
        nv_set ( "lockpci_status", "notlocked" );
        nv_set ( "lockpci_list", "" );
    }

    comd_wait_ms ( 1000 );

    gctParam_radioOnOffSet ( "on" );

    nv_set ( "lockband_set", "0" );
}

void gctParam_lockearfcn ( char* reqData )
{
    if ( NULL == reqData )
    {
        CLOGD ( ERROR, "reqData is NULL !!!\n" );
        nv_set ( "lockearfcn_set", "1" );
        return;
    }

    char earfcns_list[256] = {0};
    char earfcnlock_value[MAX_LOCK_EARFCN_NUM * 8] = {0};
    char lockband_status[16] = {0};
    char lockpci_status[16] = {0};
    char lockearfcn_cmd[MAX_LOCK_EARFCN_NUM * 11 + 16] = {0};
    char at_rep[64] = {0};
    char *lock_content = NULL;
    int tmp_band = 0;
    int tmp_arfcn = 0;
    int lock4g_num = 0;
    int ret = 0;

    CLOGD ( FINE, "earfcns_list -> [%s]\n", reqData );

    gctParam_radioOnOffSet ( "off" );

    lock_content = strtok ( reqData, ";" );
    while ( lock_content )
    {
        if ( strstr ( lock_content, "4G," ) )
        {
            ret = sscanf ( lock_content, "4G,%d,%d", &tmp_band, &tmp_arfcn );
            if ( 2 == ret )
            {
                lock4g_num++;
                snprintf ( earfcns_list + strlen ( earfcns_list ),
                                sizeof ( earfcns_list ) - strlen ( earfcns_list ), ",%d,-1", tmp_arfcn );

                snprintf ( earfcnlock_value + strlen ( earfcnlock_value ),
                                sizeof ( earfcnlock_value ) - strlen ( earfcnlock_value ), "%d_", tmp_arfcn );
            }
        }
        lock_content = strtok ( NULL, ";" );
    }

    CLOGD ( FINE, "earfcnlock_value -> [%s]\n", earfcnlock_value );
    gctRestoreToFullBand ( earfcnlock_value );

    snprintf ( lockearfcn_cmd, sizeof ( lockearfcn_cmd ), "%s=%d%s", "AT%GLOCKCELL" , lock4g_num, earfcns_list );
    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( lockearfcn_cmd, 1500, at_rep, sizeof ( at_rep ) );

    nv_get ( "lockband_status", lockband_status, sizeof ( lockband_status ) );
    nv_get ( "lockpci_status", lockpci_status, sizeof ( lockpci_status ) );
    if ( 0 == strcmp ( lockband_status, "locked" ) ||
            0 == strcmp ( lockpci_status, "locked" ) )
    {
        nv_set ( "lockband_status", "notlocked" );
        nv_set ( "lockband_list", "" );
        nv_set ( "lockpci_status", "notlocked" );
        nv_set ( "lockpci_list", "" );
    }

    comd_wait_ms ( 1000 );

    gctParam_radioOnOffSet ( "on" );

    nv_set ( "lockearfcn_set", "0" );
}

void gctParam_setDefaultGateway ( char* reqData )
{
    char* s_data = reqData;
    int apn_index = 0;
    char cur_defaultGW[4] = {0};
    char apn_enable[16] = {0};
    char apn_enableState[4] = {0};

    if ( NULL == reqData )
    {
        CLOGD ( ERROR, "reqData is NULL !!!\n" );
        nv_set ( "default_gateway_set", "1" );
        return;
    }

    snprintf ( apn_enable, sizeof ( apn_enable ), "apn%s_enable", s_data );
    nv_get ( apn_enable, apn_enableState, sizeof ( apn_enableState ) );
    if ( 0 == strcmp ( apn_enableState, "0" ) )
        s_data = "1";

    nv_get ( "default_gateway", cur_defaultGW, sizeof ( cur_defaultGW ) );
    if ( 0 == strcmp ( s_data, cur_defaultGW ) )
    {
        nv_set ( "default_gateway_set", "0" );
        return;
    }

    if ( strcmp ( cur_defaultGW, "9" ) )
    {
        nv_set ( "default_gateway", s_data );
        apn_index = atoi ( s_data );
        if ( 1 <= apn_index && apn_index <= 4 && 1 == apns_msg_flag[apn_index] )
            apns_msg_flag[apn_index] = 0;
    }

    nv_set ( "default_gateway_set", "0" );
}

void gctParam_setSearchNeighbor ( char* reqData )
{
    char at_rep[RECV_BUF_SIZE * 2] = {0};

    COMD_AT_PROCESS ( "AT!LTEINFO", 3000, at_rep, sizeof ( at_rep ) );

    parsing_gct_lteinfo ( at_rep );

    nv_set ( "search_neighbor_set", "0" );
}

void gctParam_lockpci ( char* reqData )
{
    if ( NULL == reqData )
    {
        CLOGD ( ERROR, "reqData is NULL !!!\n" );
        nv_set ( "lockpci_set", "1" );
        return;
    }

    char pcilock_value[MAX_LOCK_PCI_NUM * 8] = {0};
    char lockband_status[16] = {0};
    char lockearfcn_status[16] = {0};
    char lockpci_cmd[MAX_LOCK_PCI_NUM * 11 + 16] = {0};
    char at_rep[64] = {0};
    char *lock_content = NULL;
    int tmp_band = 0;
    int tmp_arfcn = 0;
    int tmp_pci = 0;
    char arfcn4g_pci_list[128] = {0};
    int lock4g_num = 0;
    int ret = 0;

    gctParam_radioOnOffSet ( "off" );

    lock_content = strtok ( reqData, ";" );
    while ( lock_content )
    {
        if ( strstr ( lock_content, "4G," ) )
        {
            ret = sscanf ( lock_content, "4G,%d,%d,%d", &tmp_band, &tmp_arfcn, &tmp_pci );
            if ( 3 == ret )
            {
                lock4g_num++;
                snprintf ( arfcn4g_pci_list + strlen ( arfcn4g_pci_list ),
                           sizeof ( arfcn4g_pci_list ) - strlen ( arfcn4g_pci_list ),",%d,%d", tmp_arfcn, tmp_pci );

               snprintf ( pcilock_value + strlen ( pcilock_value ),
                          sizeof ( pcilock_value ) - strlen ( pcilock_value ),"%d,%d_", tmp_arfcn, tmp_pci );
            }
        }
        lock_content = strtok ( NULL, ";" );
    }

    CLOGD ( FINE, "earfcnlock_value -> [%s]\n", pcilock_value );
    gctRestoreToFullBand ( pcilock_value );

    snprintf ( lockpci_cmd, sizeof ( lockpci_cmd ), "%s=%d%s", "AT%GLOCKCELL", lock4g_num, arfcn4g_pci_list );
    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( lockpci_cmd, 1500, at_rep, sizeof ( at_rep ) );

    nv_get ( "lockband_status", lockband_status, sizeof ( lockband_status ) );
    nv_get ( "lockearfcn_status", lockearfcn_status, sizeof ( lockearfcn_status ) );
    if ( 0 == strcmp ( lockband_status, "locked" ) ||
            0 == strcmp ( lockearfcn_status, "locked" ) )
    {
        nv_set ( "lockband_status", "notlocked" );
        nv_set ( "lockband_list", "" );
        nv_set ( "lockearfcn_status", "notlocked" );
        nv_set ( "lockearfcn_list", "" );
    }

    comd_wait_ms ( 1000 );

    gctParam_radioOnOffSet ( "on" );

    nv_set ( "lockpci_set", "0" );
}

void gctParam_getSimlock ( char* reqData )
{
    char simlock_val[256] = {0};
    char simlock_buf[256] = {0};
    char usim_mcc[4] = {0};
    char usim_mnc[4] = {0};
    char usim_plmn_buf[16] = {0};
    char lock_set[4] = {0};

    nv_get ( "usim_mcc", usim_mcc, sizeof ( usim_mcc ) );
    nv_get ( "usim_mnc", usim_mnc, sizeof ( usim_mnc ) );

    if ( 0 == strcmp ( usim_mcc, "" ) || 0 == strcmp ( usim_mnc, "" ) )
    {
        nv_set ( "simlock_set", "1" );
        return;
    }

    if ( NULL == reqData )
    {
        nv_get ( "simlock_set", lock_set, sizeof ( lock_set ) );
        if ( 0 == strcmp ( lock_set, "-1" ) )
            return;

        sys_get_config ( WAN_LTE_SIMLOCK, simlock_val, sizeof ( simlock_val ) );
    }
    else
    {
        strcpy ( simlock_val, reqData );
    }

    nv_set ( "simlock_val", simlock_val );

    if ( strcmp ( simlock_val, "" ) )
    {
        snprintf ( usim_plmn_buf, sizeof ( usim_plmn_buf ), ";%s%s;", usim_mcc, usim_mnc );
        snprintf ( simlock_buf, sizeof ( simlock_buf ), ";%s", simlock_val );
        CLOGD ( FINE, "usim_plmn: [%s], simlock_buf: [%s]\n", usim_plmn_buf, simlock_buf);
        if ( NULL == strstr ( simlock_buf, usim_plmn_buf ) )
        {
            nv_set ( "simlocked", "1" );
            goto END;
        }
    }

    nv_set ( "simlocked", "0" );

END:
    nv_set ( "simlock_set", "0" );
}

void gctParam_getNitzTime ( char* reqData )
{
    char at_rep[128] = {0};

    COMD_AT_PROCESS ( "AT+CTZU=1", 1500, at_rep, sizeof ( at_rep ) );

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( "AT+CCLK?", 1500, at_rep, sizeof ( at_rep ) );

    parsing_gct_cclk ( at_rep );
}

void gctParam_setVolteAkaRequest ( struct KT_char_akaRequest *volte_aka_req )
{
    char at_req[128] = {0};
    char at_rep[256] = {0};

    CLOGD ( FINE, "volte_aka_req->KT_simType: [%d]\n", volte_aka_req->KT_simType );
    CLOGD ( FINE, "volte_aka_req->KT_pRand: [%s]\n", volte_aka_req->KT_pRand );
    CLOGD ( FINE, "volte_aka_req->KT_pAutn: [%s]\n", volte_aka_req->KT_pAutn );

    snprintf(at_req, sizeof ( at_req ), "AT%%GSIMAUTH=%d,\"%s\",\"%s\"",
                volte_aka_req->KT_simType, volte_aka_req->KT_pRand, volte_aka_req->KT_pAutn);

    COMD_AT_PROCESS ( at_req, 3000, at_rep, sizeof ( at_rep ) );

    parsing_gct_gsimauth ( at_rep );
}

void gctParam_sendSmsRequest ( KT_sms_msg *send_sms )
{
    char at_req[SEND_MAX_SIZE] = {0};
    char at_rep[RECV_SMS_SIZE] = {0};

    CLOGD ( FINE, "send_sms->send_len: [%d]\n", send_sms->num_len );
    CLOGD ( FINE, "send_sms->send_pdu: [%s]\n", send_sms->sms_msg );

    snprintf ( at_req, sizeof ( at_req ), "AT+GCMGS=%d,\"%s\"",
                send_sms->num_len, send_sms->sms_msg );

    COMD_AT_PROCESS ( at_req, 3000, at_rep, sizeof ( at_rep ) );
    parsing_gct_gcmgs ( at_rep );

    if ( 1 == send_sms->req_type )
    {
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( "AT+CMGL=4", 5000, at_rep, sizeof ( at_rep ) );
        parsing_gct_cmgl_four ( at_rep );
    }
}

void gctParam_delSmsRequest ( KT_sms_msg *del_sms )
{
    int i = 0;
    char at_req[32] = {0};
    char at_rep[64] = {0};
    unsigned short del_ind[MAX_SMS_NUM];

    CLOGD ( FINE, "del_sms->del_cout: [%d]\n", del_sms->num_len );
    CLOGD ( FINE, "del_sms->req_type: [%d]\n", del_sms->req_type );

    memcpy ( del_ind, del_sms->sms_msg, sizeof ( del_ind ) );

    for ( i = 0; i < del_sms->num_len; i++ )
    {
        snprintf ( at_req, sizeof ( at_req ), "AT+CMGD=%d,%d", del_ind[i], del_sms->req_type );
        CLOGD ( FINE, "del_sms->del_ind[%d]: [%d]\n", i, del_ind[i] );
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( at_req, 1500, at_rep, sizeof ( at_rep ) );
    }

    parsing_gct_cmgd ( at_rep );
}

void gctParam_readSmsRequest ( KT_sms_msg *read_sms )
{
    int i = 0;
    char at_req[32] = {0};
    char at_rep[64] = {0};
    unsigned short read_ind[MAX_SMS_NUM];

    CLOGD ( FINE, "read_sms->read_cout: [%d]\n", read_sms->num_len );
    CLOGD ( FINE, "read_sms->req_type: [%d]\n", read_sms->req_type );

    memcpy ( read_ind, read_sms->sms_msg, sizeof ( read_ind ) );

    for ( i = 0; i < read_sms->num_len; i++ )
    {
        snprintf ( at_req, sizeof ( at_req ), "AT+CMGR=%d", read_ind[i] );
        CLOGD ( FINE, "read_sms->read_ind[%d]: [%d]\n", i, read_ind[i] );
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( at_req, 1500, at_rep, sizeof ( at_rep ) );
    }

    parsing_gct_cmgr ( at_rep );
}

void gctParam_setSmsCenterNum ( char* reqData )
{
    char at_req[32] = {0};
    char at_rep[64] = {0};

    if ( NULL == reqData )
    {
        CLOGD ( ERROR, "reqData is NULL !!!\n" );
        nv_set ( "set_smsc", "1" );
        return;
    }

    snprintf ( at_req, sizeof ( at_req ), "AT+CSCA=\"%s\"", reqData );
    CLOGD ( FINE, "Set SMS Center Number: [%s]\n", reqData );
    COMD_AT_PROCESS ( at_req, 1500, at_rep, sizeof ( at_rep ) );
    parsing_gct_csca_set ( at_rep );

    if ( 0 == strcmp ( reqData, "" ) )
    {
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( "AT+CSCA?", 1500, at_rep, sizeof ( at_rep ) );
        parsing_gct_csca_get ( at_rep );
    }
}

void gctParam_resendSmsRequest ( KT_sms_msg *resend_sms )
{
    int i = 0;
    char at_req[32] = {0};
    char at_rep[64] = {0};
    unsigned short resend_ind[MAX_SMS_NUM];

    CLOGD ( FINE, "resend_sms->resend_cout: [%d]\n", resend_sms->num_len );
    CLOGD ( FINE, "resend_sms->req_type: [%d]\n", resend_sms->req_type );

    memcpy ( resend_ind, resend_sms->sms_msg, sizeof ( resend_ind ) );

    for ( i = 0; i < resend_sms->num_len; i++ )
    {
        snprintf ( at_req, sizeof ( at_req ), "AT+CMSS=%d", resend_ind[i] );
        CLOGD ( FINE, "resend_sms->resend_ind[%d] : [%d]\n", i, resend_ind[i] );
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( at_req, 1500, at_rep, sizeof ( at_rep ) );
    }

    parsing_gct_cmss ( at_rep );
}

void gctParam_manualPlmnSearchReq ( char* reqData )
{
    char at_rep[RECV_BUF_SIZE] = {0};

    COMD_AT_PROCESS ( "AT+CGATT=0", 5000, at_rep, sizeof ( at_rep ) );

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( "AT+COPS=?", 5 * 60 * 1000, at_rep, sizeof ( at_rep ) );

    parsing_gct_cops_plmnSearch ( at_rep );
}

void gctParam_plmnSearchManualSelect ( char* reqData )
{
    char at_req[32] = {0};
    char at_rep[64] = {0};

    if ( NULL == reqData )
    {
        nv_set ( "plmn_search_net_set", "1" );
    }
    else if ( 0 == strcmp ( reqData, "" ) )
    {
        COMD_AT_PROCESS ( "AT+COPS=0", 3000, at_rep, sizeof ( at_rep ) );
        nv_set ( "plmn_search_net_set", "0" );
    }
    else
    {
        snprintf ( at_req, sizeof ( at_req ), "AT+COPS=1,2,%s", reqData );
        COMD_AT_PROCESS ( at_req, 10 * 1000, at_rep, sizeof ( at_rep ) );
        parsing_gct_cops_select ( at_rep );
    }
}

void gctParam_radioOnOffSet ( char* reqData )
{
    char at_rep[64] = {0};

    if ( NULL == reqData )
    {
        CLOGD ( ERROR, "reqData is NULL !!!\n" );
    }
    else if ( 0 == strcmp ( reqData, "off" ) )
    {
        COMD_AT_PROCESS ( "AT+CFUN=0", 3000, at_rep, sizeof ( at_rep ) );
    }
    else
    {
        COMD_AT_PROCESS ( "AT+CFUN=1", 3000, at_rep, sizeof ( at_rep ) );
    }

    if ( 0 == parsing_gct_cfun_set ( at_rep ) )
        nv_set ( "lte_on_off", reqData );
}

void gctParam_lteTxPowerLimitSet ( char* reqData )
{
    char txpowlmt_val[8] = {0};
    char at_req[32] = {0};
    char at_rep[64] = {0};

    if ( NULL == reqData )
    {
        nv_get("txpower_limit", txpowlmt_val, sizeof(txpowlmt_val));
    }
    else
    {
        snprintf ( txpowlmt_val, sizeof ( txpowlmt_val ), "%s", reqData );
    }

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

    parsing_gct_txpowlmt ( at_rep );
}

void gctParam_lteRoamingSet ( char* reqData )
{
    char roaming_set[8] = {0};
    char module_val[8] = {0};
    char at_rep[64] = {0};

    if ( reqData != NULL )
    {
        strncpy ( roaming_set, reqData, sizeof ( roaming_set ) - 1 );
    }
    else
    {
        sys_get_config ( LTE_PARAMETER_ROAMING, roaming_set, sizeof ( roaming_set ) );
    }

    ucfg_info_get ( "config wan lte plmn_search_param roaming", "roaming",
                        module_val, sizeof ( module_val ) );

    if ( 0 == strcmp ( roaming_set, module_val ) )
    {
        nv_set ( "roaming_set", "0" );
        return;
    }

    ucfg_info_set ( "config wan lte plmn_search_param roaming", roaming_set );

    gctParam_radioOnOffSet ( "off" );
    comd_wait_ms ( 1000 );
    gctParam_radioOnOffSet ( "on" );

    nv_set ( "roaming_set", "0" );
    return;
}

