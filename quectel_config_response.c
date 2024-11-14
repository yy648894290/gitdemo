#include "comd_share.h"
#include "atchannel.h"
#include "at_tok.h"
#include "quectel_config_response.h"
#include "quectel_status_refresh.h"
#include "quectel_atcmd_parsing.h"
#include "config_key.h"
#include "hal_platform.h"

extern int apns_msg_flag[5];

void quectelParam_lockPin ( char* reqData )
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

    quectelParam_updatePinStatus ();
    quectelParam_updatePinLockStatus ();
    quectelParam_updatePinPukCount ();

END:
    parsing_quectel_lockpin ( at_rep );
}

void quectelParam_enterPin ( char* reqData )
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

    quectelParam_updatePinStatus ();
    quectelParam_updatePinPukCount ();

END:
    parsing_quectel_enterpin ( at_rep );
}

void quectelParam_modifyPin ( char* reqData )
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

    quectelParam_updatePinStatus ();
    quectelParam_updatePinPukCount ();

END:
    parsing_quectel_modifypin ( at_rep );
}

void quectelParam_config_commandsetting ( char* reqData )
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

void quectelParam_enterPuk ( char* reqData )
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

    quectelParam_updatePinStatus ();
    quectelParam_updatePinPukCount ();

END:
    parsing_quectel_enterpuk ( at_rep );
}

void quectelParam_connectNetwork ( char* reqData )
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+COPS?", 3000, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "+COPS: 0" ) )
    {
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( "AT+COPS=0", 10000, at_rep, sizeof ( at_rep ) );
    }

    nv_set ( "connect_network_set", "0" );
}

void quectelParam_disconnectNetwork ( char* reqData )
{
    char at_rep[32] = {0};

    COMD_AT_PROCESS ( "AT+COPS=2", 10000, at_rep, sizeof ( at_rep ) );

    nv_set ( "disconnect_network_set", strstr ( at_rep, "\r\nOK\r\n" ) ? "0" : "1" );
}

void quectelParam_apnSetting ( apn_profile* const apnSetting_data )
{
    if ( NULL == apnSetting_data )
    {
        CLOGD ( ERROR, "apnSetting_data is NULL !!!\n" );
        nv_set ( "apn_set", "1" );
        return;
    }

    quectelParam_initAPNsetting ( ( char * ) apnSetting_data );

    return;
}

int quectelParam_setRatPriority ( char* reqData )
{
    char module_type[32] = {0};
    char at_req[64] = {0};
    char at_rep[64] = {0};
    char uciPreferRadio[8] = {0};

    if ( NULL == reqData )
    {
        sys_get_config ( "lte_param.scanmode.prefer_radio", uciPreferRadio, sizeof ( uciPreferRadio ) );
    }
    else
    {
        snprintf ( uciPreferRadio, sizeof ( uciPreferRadio ), "%s", reqData );
    }

    nv_get ( "modulemodel", module_type, sizeof ( module_type ) );
    if ( strstr ( module_type, "EM160R-GL" ) )
    {
        snprintf ( at_req, sizeof ( at_req ), "AT+QNWPREFCFG=\"rat_acq_order\",%s", "LTE" );
    }
    else
    {
        snprintf ( at_req, sizeof ( at_req ), "AT+QNWPREFCFG=\"rat_acq_order\",%s",
                                            strcmp ( uciPreferRadio, "4G" ) ? "NR5G:LTE" : "LTE:NR5G" );
    }

    COMD_AT_PROCESS ( at_req, 3000, at_rep, sizeof ( at_rep ) );

    return strstr ( at_rep, "\r\nOK\r\n" ) ? 0 : 1;
}

void quectelParam_lockband ( char* reqData, int lock_type )
{
    char band_value[256] = {0};
    char lockearfcn_status[16] = {0};
    char lockpci_status[16] = {0};
    char lockband_cmd[256] = {0};
    char lock5gband_cmd[256] = {0};
    char at_rep[64] = {0};
    char ramRadioPrefer[8] = {0};
    char mode_pref_cmd[64] = {0};
    char lockbands_4g[128] = {0};
    char lockbands_5g[128] = {0};

    if ( NULL == reqData )
    {
        CLOGD ( ERROR, "reqData is NULL !!!\n" );
        nv_set ( "lockband_set", "1" );
        return;
    }

    if ( strcmp ( reqData, "" ) )
    {
        snprintf ( band_value, sizeof ( band_value ), "%s", reqData );

        if ( ',' == band_value[ strlen ( band_value ) - 1 ] || NULL == strstr ( band_value, "," ) ) // 4G only
        {
            sscanf ( band_value, "%[0-9_],", lockbands_4g );
            snprintf ( mode_pref_cmd, sizeof ( mode_pref_cmd ), "AT+QNWPREFCFG=\"mode_pref\",%s", "LTE" );
        }
        else if ( ',' == band_value[0] ) // 5G only
        {
            sscanf ( band_value, ",%[0-9_]", lockbands_5g );
            snprintf ( mode_pref_cmd, sizeof ( mode_pref_cmd ), "AT+QNWPREFCFG=\"mode_pref\",%s", "NR5G" );
        }
        else // 4G & 5G
        {
            sscanf ( band_value, "%[0-9_],%[0-9_]", lockbands_4g, lockbands_5g );
            snprintf ( mode_pref_cmd, sizeof ( mode_pref_cmd ), "AT+QNWPREFCFG=\"mode_pref\",%s", "LTE:NR5G" );
        }

        CLOGD ( FINE, "lockbands_4g -> [%s]\n", lockbands_4g );
        nv_set ( "lockband_list", lockbands_4g );
        if ( strcmp ( lockbands_4g, "" ) )
        {
            comd_strrpl ( lockbands_4g, "_", ":" );
            lockbands_4g [ strlen ( lockbands_4g ) - 1 ] = '\0';
        }
        snprintf ( lockband_cmd, sizeof ( lockband_cmd ), "AT+QNWPREFCFG=\"lte_band\",%s",
                                                                strcmp ( lockbands_4g, "" ) ? lockbands_4g : "0" );

        CLOGD ( FINE, "lockbands_5g -> [%s]\n", lockbands_5g );
        nv_set ( "lockband5g_list", lockbands_5g );
        if ( strcmp ( lockbands_5g, "" ) )
        {
            comd_strrpl ( lockbands_5g, "_", ":" );
            lockbands_5g [ strlen ( lockbands_5g ) - 1 ] = '\0';
        }
        snprintf ( lock5gband_cmd, sizeof ( lock5gband_cmd ), "AT+QNWPREFCFG=\"nr5g_band\",%s",
                                                                strcmp ( lockbands_5g, "" ) ? lockbands_5g : "0" );

        nv_set ( "lockband_status", "locked" );
    }
    else
    {
        snprintf ( mode_pref_cmd, sizeof ( mode_pref_cmd ), "AT+QNWPREFCFG=\"mode_pref\",%s", "LTE:NR5G" );

        nv_get ( "suppband_org", lockbands_4g, sizeof ( lockbands_4g ) );
        snprintf ( lockband_cmd, sizeof ( lockband_cmd ), "AT+QNWPREFCFG=\"lte_band\",%s", lockbands_4g );

        nv_get ( "suppband5g_org", lockbands_5g, sizeof ( lockbands_5g ) );
        snprintf ( lock5gband_cmd, sizeof ( lock5gband_cmd ), "AT+QNWPREFCFG=\"nr5g_band\",%s",
                                                                strcmp ( lockbands_5g, "" ) ? lockbands_5g : "0" );

        nv_set ( "lockband_status", "notlocked" );
        nv_set ( "lockband_list", "" );
        nv_set ( "lockband5g_list", "" );
    }

    if ( 1 == lock_type )
    {
        quectelParam_radioOnOffSet ( "off" );
    }

    if ( strcmp ( lockbands_4g, "" ) && !strcmp ( lockbands_5g, "" ))
    {
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( lockband_cmd, 3000, at_rep, sizeof ( at_rep ) );
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( mode_pref_cmd, 3000, at_rep, sizeof ( at_rep ) );
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( lock5gband_cmd, 3000, at_rep, sizeof ( at_rep ) );
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( "AT+QNWPREFCFG=\"gw_band\",0", 3000, at_rep, sizeof ( at_rep ) );
    }
    else if( !strcmp ( lockbands_4g, "" ) && strcmp ( lockbands_5g, "" ))
    {
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( lock5gband_cmd, 3000, at_rep, sizeof ( at_rep ) );
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( mode_pref_cmd, 3000, at_rep, sizeof ( at_rep ) );
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( lockband_cmd, 3000, at_rep, sizeof ( at_rep ) );
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( "AT+QNWPREFCFG=\"gw_band\",0", 3000, at_rep, sizeof ( at_rep ) );
    }
    else
    {
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( mode_pref_cmd, 3000, at_rep, sizeof ( at_rep ) );

        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( "AT+QNWPREFCFG=\"gw_band\",0", 3000, at_rep, sizeof ( at_rep ) );

        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( lockband_cmd, 3000, at_rep, sizeof ( at_rep ) );

        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( lock5gband_cmd, 3000, at_rep, sizeof ( at_rep ) );
    }

    if ( 1 == lock_type )
    {
        comd_wait_ms ( 1000 );
        quectelParam_radioOnOffSet ( "on" );
        nv_get ( "radio_prefermode", ramRadioPrefer, sizeof ( ramRadioPrefer ) );
        quectelParam_setRatPriority ( ramRadioPrefer );
    }

    nv_get ( "lockearfcn_status", lockearfcn_status, sizeof ( lockearfcn_status ) );
    if ( strlen(lockearfcn_status) == 0 || 0 == strcmp ( lockearfcn_status, "locked" ) )
    {
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( "AT+QNWCFG=\"lte_earfcn_lock\",0", 3000, at_rep, sizeof ( at_rep ) );
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( "AT+QNWCFG=\"nr5g_earfcn_lock\",0", 3000, at_rep, sizeof ( at_rep ) );
        nv_set ( "lockearfcn_status", "notlocked" );
        nv_set ( "lockearfcn_list", "" );
    }

    nv_get ( "lockpci_status", lockpci_status, sizeof ( lockpci_status ) );
    if ( strlen(lockpci_status) == 0 || 0 == strcmp ( lockpci_status, "locked" ) )
    {
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( "AT+QNWLOCK=\"common/4g\",0", 3000, at_rep, sizeof ( at_rep ) );
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( "AT+QNWLOCK=\"common/5g\",0", 3000, at_rep, sizeof ( at_rep ) );
        nv_set ( "lockpci_status", "notlocked" );
        nv_set ( "lockpci_list", "" );
    }

    if( 0 == strcmp ( lockearfcn_status, "locked" ) || 0 == strcmp ( lockpci_status, "locked" ) )
    {
        nv_set ( "module_need_reboot", "1" );
    }

    nv_set ( "lockband_set", "0" );
}

void quectelParam_lockearfcn ( char* reqData )
{
    if ( NULL == reqData )
    {
        CLOGD ( ERROR, "reqData is NULL !!!\n" );
        nv_set ( "lockearfcn_set", "1" );
        return;
    }

    char earfcns_list[256] = {0};
    int earfcns_num = 0;
    char at_rep[64] = {0};
    char lockband_status[16] = {0};
    char lockpci_status[16] = {0};
    char lockearfcn_status[16] = {0};
    char earfcn[64] = {0};
    char earfcn_5g[64] = {0};
    char mode_pref_cmd[64] = {0};
    char lockearfcns_5g[128] = {0};
    char lockearfcn_cmd[64] = {0};
    char lock5gearfcn_cmd[64] = {0};
    char lockband4g_cmd[64] = {0};
    char lockband5g_cmd[64] = {0};
    char scs_type[4] = {0};
    int lockearfcn_4g_flag = 0;
    char band_4g[32] = {0};
    char lockband_list[32] = {0};
    char band_list[32] = {0};
    int earfcn_dl = 0;

    snprintf ( earfcns_list, sizeof ( earfcns_list ), "%s", reqData );
    CLOGD ( FINE, "earfcns_list -> [%s]\n", earfcns_list );

    if ( NULL == strstr ( earfcns_list, "4G") && NULL == strstr ( earfcns_list, "5G") )
    {
        nv_set ( "lockearfcn_set", "1" );
        return;
    }

    char *earfcn_tmp = strtok ( earfcns_list, "_" );
    while ( earfcn_tmp )
    {
        if ( 0 == strncmp ( earfcn_tmp, "4G", 2 ) )
        {
            lockearfcn_4g_flag = 1;
            sscanf ( earfcn_tmp, "4G,%[0-9]", earfcn );
            earfcn_dl = atoi( earfcn );
            calc_4g_band_from_earfcn( earfcn_dl, band_4g, sizeof( band_4g ) );
        }
        else if ( 0 == strncmp ( earfcn_tmp, "5G", 2 ) )
        {
            sscanf ( earfcn_tmp, "5G,%[0-9],%[0-9]", earfcn_5g, scs_type );
            snprintf ( lockearfcns_5g + strlen ( lockearfcns_5g ), sizeof ( lockearfcns_5g ) - strlen ( lockearfcns_5g ), "%s:%s:", earfcn_5g, scs_type );
            earfcn_dl = atoi( earfcn_5g );
            calc_5g_band_from_narfcn( earfcn_dl, band_list, sizeof( band_list ) );
            earfcns_num ++;
        }
        earfcn_tmp = strtok ( NULL, "_" );
    }

    CLOGD ( FINE, "earfcn -> [%s]\n", earfcn );
    lockearfcns_5g [ strlen ( lockearfcns_5g ) - 1 ] = '\0';
    CLOGD ( FINE, "lockearfcns_5g -> [%s]\n", lockearfcns_5g );

    if ( lockearfcn_4g_flag == 1 && earfcns_num != 0 )
    {
        snprintf ( mode_pref_cmd, sizeof ( mode_pref_cmd ), "AT+QNWPREFCFG=\"mode_pref\",%s", "LTE:NR5G" );
        snprintf ( lockearfcn_cmd, sizeof ( lockearfcn_cmd ), "AT+QNWCFG=\"lte_earfcn_lock\",1,%s", earfcn );
        snprintf ( lock5gearfcn_cmd, sizeof ( lock5gearfcn_cmd ), "AT+QNWCFG=\"nr5g_earfcn_lock\",%d,%s", earfcns_num, lockearfcns_5g );
    }
    else if ( lockearfcn_4g_flag == 1 && earfcns_num == 0 )
    {
        snprintf ( mode_pref_cmd, sizeof ( mode_pref_cmd ), "AT+QNWPREFCFG=\"mode_pref\",%s", "LTE" );
        snprintf ( lockearfcn_cmd, sizeof ( lockearfcn_cmd ), "AT+QNWCFG=\"lte_earfcn_lock\",1,%s", earfcn );
        snprintf ( lock5gearfcn_cmd, sizeof ( lock5gearfcn_cmd ), "AT+QNWCFG=\"nr5g_earfcn_lock\",0" );
    }
    else if ( lockearfcn_4g_flag == 0 && earfcns_num != 0 )
    {
        snprintf ( mode_pref_cmd, sizeof ( mode_pref_cmd ), "AT+QNWPREFCFG=\"mode_pref\",%s", "NR5G" );
        snprintf ( lockearfcn_cmd, sizeof ( lockearfcn_cmd ), "AT+QNWCFG=\"lte_earfcn_lock\",0" );
        snprintf ( lock5gearfcn_cmd, sizeof ( lock5gearfcn_cmd ), "AT+QNWCFG=\"nr5g_earfcn_lock\",%d,%s", earfcns_num, lockearfcns_5g );
    }

    nv_get ( "lockband_status", lockband_status, sizeof ( lockband_status ) );
    nv_get ( "lockpci_status", lockpci_status, sizeof ( lockpci_status ) );
    nv_get ( "lockearfcn_status", lockearfcn_status, sizeof ( lockearfcn_status ) );
    CLOGD ( FINE, "lockband_status -> [%s]\n", lockband_status );
    CLOGD ( FINE, "lockpci_status -> [%s]\n", lockpci_status );
    CLOGD ( FINE, "lockearfcn_status -> [%s]\n", lockearfcn_status );

    COMD_AT_PROCESS ( "AT+QNWPREFCFG=\"mode_pref\",LTE:NR5G", 3000, at_rep, sizeof ( at_rep ) );

    if( lockearfcn_4g_flag == 1 )
    {
         band_4g [ strlen ( band_4g ) - 1 ] = '\0';
         snprintf ( lockband4g_cmd, sizeof ( lockband4g_cmd ), "AT+QNWPREFCFG=\"lte_band\",%s", band_4g);
    }
    else
    {
         snprintf ( lockband4g_cmd, sizeof ( lockband4g_cmd ), "AT+QNWPREFCFG=\"lte_band\",%d", 0);
    }

    if( earfcns_num != 0 )
    {
        split_and_remove_duplicates(band_list,lockband_list,sizeof(lockband_list));
        lockband_list [ strlen ( lockband_list ) - 1 ] = '\0';
        snprintf ( lockband5g_cmd, sizeof ( lockband5g_cmd ), "AT+QNWPREFCFG=\"nr5g_band\",%s", lockband_list);
    }
    else
    {
        snprintf ( lockband5g_cmd, sizeof ( lockband5g_cmd ), "AT+QNWPREFCFG=\"nr5g_band\",%d", 0);
    }

    if ( lockearfcn_4g_flag == 1 && earfcns_num == 0 )
    {
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( lockband4g_cmd, 3000, at_rep, sizeof ( at_rep ) );
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( lockband5g_cmd, 3000, at_rep, sizeof ( at_rep ) );
    }
    else
    {
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( lockband5g_cmd, 3000, at_rep, sizeof ( at_rep ) );
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( lockband4g_cmd, 3000, at_rep, sizeof ( at_rep ) );
    }

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( mode_pref_cmd, 3000, at_rep, sizeof ( at_rep ) );

    if ( 0 == strcmp ( lockpci_status, "locked" ) )//clean pci lock
    {
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( "AT+QNWLOCK=\"common/4g\",0", 3000, at_rep, sizeof ( at_rep ) );
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( "AT+QNWLOCK=\"common/5g\",0", 3000, at_rep, sizeof ( at_rep ) );
    }

    if( 0 == strcmp ( lockearfcn_status, "locked" ) || 0 == strcmp ( lockpci_status, "locked" ) || 0 != strlen ( lockband_status ) )
    {
        CLOGD ( FINE, "lock earfcn  module need reboot\n" );
        nv_set ( "module_need_reboot", "1" );
    }

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( lockearfcn_cmd, 3000, at_rep, sizeof ( at_rep ) );
    if( lockearfcn_4g_flag == 1 )
    {
        if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
        {
            nv_set ( "lockearfcn_set", "1" );
            return;
        }
    }

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( lock5gearfcn_cmd, 3000, at_rep, sizeof ( at_rep ) );
    if( earfcns_num != 0 )
    {
        if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
        {
            nv_set ( "lockearfcn_set", "1" );
            return;
        }
    }

    nv_set ( "lockearfcn_status", "locked" );
    nv_set ( "lockearfcn_list", reqData );
    nv_set ( "lockband_status", "notlocked" );
    nv_set ( "lockband_list", "" );
    nv_set ( "lockpci_status", "notlocked" );
    nv_set ( "lockpci_list", "" );

    nv_set ( "lockearfcn_set", "0" );

    return;
}

void quectelParam_setDefaultGateway ( char* reqData )
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
    {
        s_data = "1";
    }

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
        {
            apns_msg_flag[apn_index] = 0;
        }
    }

    nv_set ( "default_gateway_set", "0" );
}

void quectelParam_lockpci ( char* reqData )
{
    if ( NULL == reqData )
    {
        CLOGD ( ERROR, "reqData is NULL !!!\n" );
        nv_set ( "lockpci_set", "1" );
        return;
    }

    char pcis_list[256] = {0};
    int pcis_num = 0;
    char at_tmp[256] = {0};
    char at_rep[64] = {0};
    char lockband_status[16] = {0};
    char lockearfcn_status[16] = {0};
    char lockpci_status[16] = {0};
    char pci_4g[64] = {0};
    char pci_5g[64] = {0};
    char lockband_5g[8] = {0};
    char lockearfcn_5g[8] = {0};
    char scs_type[4] = {0};
    char mode_pref_cmd[64] = {0};
    char lockband4g_cmd[64] = {0};
    char lockband5g_cmd[64] = {0};
    char lockpcis_4g[128] = {0};
    char lockpcis_5g[128] = {0};
    char lockpci_cmd[64] = {0};
    char lock5gpci_cmd[64] = {0};
    int lock5gpci_flag = 0;
    char earfcn[64] = {0};
    char band_list[32] = {0};
    char lockband_list[32] = {0};

    snprintf ( pcis_list, sizeof ( pcis_list ), "%s", reqData );
    CLOGD ( FINE, "pcis_list -> [%s]\n", pcis_list );

    if ( NULL == strstr ( pcis_list, "4G") && NULL == strstr ( pcis_list, "5G") )
    {
        nv_set ( "lockpci_set", "1" );
        return;
    }

    char *pic_tmp = strtok ( pcis_list, "_" );
    while ( pic_tmp )
    {
        if ( 0 == strncmp ( pic_tmp, "4G", 2 ) )
        {
            sscanf ( pic_tmp, "4G,%[0-9],%[0-9]", earfcn, pci_4g );
            snprintf ( lockpcis_4g + strlen ( lockpcis_4g ), sizeof ( lockpcis_4g ) - strlen ( lockpcis_4g ),"%s,%s,", earfcn, pci_4g );
            calc_4g_band_from_earfcn( atoi( earfcn ), band_list, sizeof( band_list ) );
            pcis_num ++;
        }
        else if ( 0 == strncmp ( pic_tmp, "5G", 2 ) )
        {
            lock5gpci_flag = 1;
            sscanf ( pic_tmp, "5G,%[0-9],%[0-9],%[0-9],%[0-9]", lockband_5g, lockearfcn_5g, pci_5g, scs_type );
            snprintf ( lockpcis_5g, sizeof ( lockpcis_5g ), "%s,%s,%s,%s", pci_5g, lockearfcn_5g, scs_type, lockband_5g );
        }
        pic_tmp = strtok ( NULL, "_" );
    }

    CLOGD ( FINE, "lockpcis_4g -> [%s]\n", lockpcis_4g );
    lockpcis_4g [ strlen ( lockpcis_4g ) - 1 ] = '\0';
    CLOGD ( FINE, "lockpcis_5g -> [%s]\n", lockpcis_5g );

    if ( pcis_num != 0 && lock5gpci_flag == 1 )
    {
        snprintf ( mode_pref_cmd, sizeof ( mode_pref_cmd ), "AT+QNWPREFCFG=\"mode_pref\",%s", "LTE:NR5G" );
        snprintf ( lockpci_cmd, sizeof ( lockpci_cmd ), "AT+QNWLOCK=\"common/4g\",%d,%s", pcis_num, lockpcis_4g );
        snprintf ( lock5gpci_cmd, sizeof ( lock5gpci_cmd ), "AT+QNWLOCK=\"common/5g\",%s", lockpcis_5g );
    }
    else if ( pcis_num != 0 && lock5gpci_flag == 0 )
    {
        snprintf ( mode_pref_cmd, sizeof ( mode_pref_cmd ), "AT+QNWPREFCFG=\"mode_pref\",%s", "LTE" );
        snprintf ( lockpci_cmd, sizeof ( lockpci_cmd ), "AT+QNWLOCK=\"common/4g\",%d,%s", pcis_num, lockpcis_4g );
        snprintf ( lock5gpci_cmd, sizeof ( lock5gpci_cmd ), "AT+QNWLOCK=\"common/5g\",0" );
    }
    else if ( pcis_num == 0 && lock5gpci_flag == 1 )
    {
        snprintf ( mode_pref_cmd, sizeof ( mode_pref_cmd ), "AT+QNWPREFCFG=\"mode_pref\",%s", "NR5G" );
        snprintf ( lockpci_cmd, sizeof ( lockpci_cmd ), "AT+QNWLOCK=\"common/4g\",0" );
        snprintf ( lock5gpci_cmd, sizeof ( lock5gpci_cmd ), "AT+QNWLOCK=\"common/5g\",%s", lockpcis_5g );
    }

    nv_get ( "lockband_status", lockband_status, sizeof ( lockband_status ) );
    nv_get ( "lockearfcn_status", lockearfcn_status, sizeof ( lockearfcn_status ) );
    nv_get ( "lockpci_status", lockpci_status, sizeof ( lockpci_status ) );
    CLOGD ( FINE, "lockband_status -> [%s]\n", lockband_status );
    CLOGD ( FINE, "lockearfcn_status -> [%s]\n", lockearfcn_status );
    CLOGD ( FINE, "lockpci_status -> [%s]\n", lockpci_status );

    COMD_AT_PROCESS ( "AT+QNWPREFCFG=\"mode_pref\",LTE:NR5G", 3000, at_rep, sizeof ( at_rep ) );

    if( pcis_num != 0 )
    {
        split_and_remove_duplicates(band_list,lockband_list,sizeof(lockband_list));
        lockband_list [ strlen ( lockband_list ) - 1 ] = '\0';
        snprintf ( lockband4g_cmd, sizeof ( lockband4g_cmd ), "AT+QNWPREFCFG=\"lte_band\",%s", lockband_list);
    }
    else
    {
        snprintf ( lockband4g_cmd, sizeof ( lockband4g_cmd ), "AT+QNWPREFCFG=\"lte_band\",%d", 0);
    }

    if( lock5gpci_flag == 1 )
    {
         snprintf ( lockband5g_cmd, sizeof ( lockband5g_cmd ), "AT+QNWPREFCFG=\"nr5g_band\",%s", lockband_5g);
    }
    else
    {
         snprintf ( lockband5g_cmd, sizeof ( lockband5g_cmd ), "AT+QNWPREFCFG=\"nr5g_band\",%d", 0);
    }

    if ( pcis_num != 0 && lock5gpci_flag == 0 )
    {
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( lockband4g_cmd, 3000, at_rep, sizeof ( at_rep ) );

        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( lockband5g_cmd, 3000, at_rep, sizeof ( at_rep ) );
    }
    else
    {
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( lockband5g_cmd, 3000, at_rep, sizeof ( at_rep ) );

        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( lockband4g_cmd, 3000, at_rep, sizeof ( at_rep ) );
    }

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( mode_pref_cmd, 3000, at_rep, sizeof ( at_rep ) );

    if( 0 == strcmp ( lockearfcn_status, "locked" ) || 0 == strcmp ( lockpci_status, "locked" ) || 0 != strlen ( lockband_status ) )
    {
        CLOGD ( FINE, "lock pci module need reboot\n" );
        nv_set ( "module_need_reboot", "1" );
    }

    memset ( at_tmp, 0, sizeof ( at_tmp ) );
    COMD_AT_PROCESS ( lock5gpci_cmd, 3000, at_tmp, sizeof ( at_tmp ) );

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( lockpci_cmd, 3000, at_rep, sizeof ( at_rep ) );
    if( pcis_num != 0 )
    {
        if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
        {
            nv_set ( "lockpci_set", "1" );
            return;
        }
    }

    if( lock5gpci_flag == 1 )
    {
        if ( NULL == strstr ( at_tmp, "\r\nOK\r\n" ) )
        {
            nv_set ( "lockpci_set", "1" );
            return;
        }
        else
        {
            /* fix 5G pci lock edge frequency point error
             * AT return OK and check if the pci is properly locked
             */
            memset ( at_rep, 0, sizeof ( at_rep ) );
            COMD_AT_PROCESS ( "AT+QNWLOCK=\"common/5g\"", 3000, at_rep, sizeof ( at_rep ) );
            if( check_quectel_pcilock( at_rep, lockearfcn_5g ) )
            {
                nv_set ( "lockpci_set", "1" );
                return;
            }
        }
    }

    if ( 0 == strcmp ( lockearfcn_status, "locked" ) )//clean earfcn lock
    {
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( "AT+QNWCFG=\"lte_earfcn_lock\",0", 3000, at_rep, sizeof ( at_rep ) );
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( "AT+QNWCFG=\"nr5g_earfcn_lock\",0", 3000, at_rep, sizeof ( at_rep ) );
    }

    nv_set ( "lockearfcn_status", "notlocked" );
    nv_set ( "lockearfcn_list", "" );
    nv_set ( "lockband_status", "notlocked" );
    nv_set ( "lockband_list", "" );
    nv_set ( "lockpci_status", "locked" );
    nv_set ( "lockpci_list", reqData );

    nv_set ( "lockpci_set", "0" );

    return;
}

int check_quectel_pcilock( char *data, char *earfcn )
{
    int i = 0;
    int ret = 0;
    char regex_buf[4][REGEX_BUF_ONE];

    char *pattern = NULL;

    for ( i = 0; i < 4; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    pattern = "\\+QNWLOCK:[ ]*\"([^\"]*)\",([^,]*),([^,]*),.*$";

    if ( NULL == strstr ( data, "\r\n+QNWLOCK: \"common/5g\",0" ) )
    {
        ret = at_tok_regular_more ( strstr(data,"+QNWLOCK:"), pattern, regex_buf );

        CLOGD ( FINE,"ret = %d,regex_buf[3] = [%s]\n",ret,regex_buf[3]);
        if ( 0 == ret )
        {
            if( strstr( regex_buf[3], earfcn ) != NULL )
            {
                return  0;
            }
        }
    }
    return -1;
}

void quectelParam_getSimlock ( char* reqData )
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

        sys_get_config ( "lte_param.simlock.plmn", simlock_val, sizeof ( simlock_val ) );
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

void quectelParam_radioOnOffSet ( char* reqData )
{
    char at_rep[64] = {0};

    if ( NULL == reqData )
    {
        CLOGD ( ERROR, "reqData is NULL !!!\n" );
    }
    else if ( 0 == strcmp ( reqData, "off" ) )
    {
        COMD_AT_PROCESS ( "AT+CFUN=4", 3000, at_rep, sizeof ( at_rep ) );
    }
    else
    {
        COMD_AT_PROCESS ( "AT+CFUN=1", 3000, at_rep, sizeof ( at_rep ) );
    }

    if ( 0 == parsing_quectel_cfun_set ( at_rep ) )
        nv_set ( "lte_on_off", reqData );
}

int quectelParam_setServicePSCSMode ( char* reqData )
{
    char at_rep[64] = {0};
    char service_mode_pscs[8] = {0};

    sys_get_config ( "lte_param.parameter.service_mode_pscs", service_mode_pscs, sizeof ( service_mode_pscs ) );
    if ( 0 == strcmp ( service_mode_pscs, "" ) )
    {
        strcpy ( service_mode_pscs, "PSCS" );
    }

    if ( NULL != reqData )
    {
        if ( 0 == strcmp ( service_mode_pscs, reqData ) )
        {
            nv_set ( "set_service_mode_pscs", "0");
            return 0;
        }
        snprintf ( service_mode_pscs, sizeof ( service_mode_pscs ), "%s", reqData );
    }

    if ( 0 == strcmp ( service_mode_pscs, "PSCS" ) )
    {
        COMD_AT_PROCESS ( "AT+QNWPREFCFG=\"srv_domain\",2", 3000, at_rep, sizeof ( at_rep ) );
    }
    else if( 0 == strcmp ( service_mode_pscs, "PS" ))
    {
        COMD_AT_PROCESS ( "AT+QNWPREFCFG=\"srv_domain\",1", 3000, at_rep, sizeof ( at_rep ) );
    }
    else if( 0 == strcmp ( service_mode_pscs, "CS" ))
    {
        COMD_AT_PROCESS ( "AT+QNWPREFCFG=\"srv_domain\",0", 3000, at_rep, sizeof ( at_rep ) );
    }

    if ( strstr ( at_rep, "\r\nOK\r\n" ) )
    {
        nv_set ( "set_service_mode_pscs", "0");

        return 0;
    }
    else
    {
        nv_set ( "set_service_mode_pscs", "1");
        CLOGD ( ERROR, "AT+QNWPREFCFG=\"srv_domain\" is ERROR !!!\n" );

        return 1;
    }
}

void quectelParam_netSelectModeSet ( char* reqData )
{
    char cur_select_mode[32] = {0};

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

    nv_set ( "net_select_mode_set", "0" );
}

void quectelParam_setSearchNeighbor ( char* reqData )
{
    char at_rep[RECV_BUF_SIZE * 2] = {0};
    int ret = 0;

    COMD_AT_PROCESS ( "AT+QENG=\"neighbourcell\"", 3000, at_rep, sizeof ( at_rep ) );

    ret = parsing_quectel_neighbor_cellinfo ( at_rep );

    nv_set ( "search_neighbor_set", ( 0 < ret ) ? "0" : "1" );
}

void quectelParam_dualSimSet ( char* reqData )
{
    if ( NULL == reqData )
    {
        CLOGD ( ERROR, "reqData is NULL !!!\n" );
        nv_set ( "dualsim_set", "1" );
    }
    else
    {
        nv_set ( "dualsim_set", "0" );
    }
}

void quectelParam_getNitzTime ( char* reqData )
{
    char at_rep[128] = {0};

    COMD_AT_PROCESS ( "AT+CTZU=1", 1500, at_rep, sizeof ( at_rep ) );

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( "AT+CCLK?", 1500, at_rep, sizeof ( at_rep ) );

    parsing_quectel_cclk ( at_rep );
}

void quectelParam_setVolteAkaRequest ( struct KT_char_akaRequest *volte_aka_req )
{
    char at_req[256] = {0};
    char at_rep[256] = {0};

    CLOGD ( FINE, "volte_aka_req->KT_simType: [%d]\n", volte_aka_req->KT_simType );
    CLOGD ( FINE, "volte_aka_req->KT_pRand: [%s]\n", volte_aka_req->KT_pRand );
    CLOGD ( FINE, "volte_aka_req->KT_pAutn: [%s]\n", volte_aka_req->KT_pAutn );

    snprintf(at_req, sizeof ( at_req ), "AT+QIMSCFG=\"authenticate\",11,\"10%s10%s\"",
                volte_aka_req->KT_pRand, volte_aka_req->KT_pAutn);

    COMD_AT_PROCESS ( at_req, 3000, at_rep, sizeof ( at_rep ) );

    parsing_quectel_qimscfg ( at_rep );
}

int quectelParam_lteRoamingSet ( char* reqData )
{
    char roaming_set[8] = {0};
    char at_req[32] = {0};
    char at_rep[64] = {0};

    if ( NULL != reqData )
    {
        strncpy ( roaming_set, reqData, sizeof ( roaming_set ) - 1 );
    }
    else
    {
        sys_get_config ( "lte_param.parameter.roaming",
                            roaming_set, sizeof ( roaming_set ) );
    }

    snprintf ( at_req, sizeof ( at_req ), "AT+QNWPREFCFG=\"roam_pref\",%s", strcmp ( roaming_set, "1" ) ? "255" : "1" );

    COMD_AT_PROCESS ( at_req, 3000, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
    {
        nv_set ( "roaming_set", "1" );
        return -1;
    }

    if ( NULL != reqData )
    {
        quectelParam_radioOnOffSet ( "off" );
        comd_wait_ms ( 1000 );
        quectelParam_radioOnOffSet ( "on" );
        quectelParam_setRatPriority ( NULL );
    }

    nv_set ( "roaming_set", "0" );

    return 0;
}

void quectelparsing_txpowlmt ( char* data )
{
    if ( strstr ( data, "\r\nOK\r\n" ) )
    {
        nv_set ( "txpower_limit_set", "0" );
    }
    else
    {
        nv_set ( "txpower_limit_set", "1" );
        CLOGD ( WARNING, "AT TXPOWLMT set error !\n" );
    }
}

void quectelParam_lteTxPowerLimitSet ( char* reqData )
{
    char txpowlmt_val[8] = {0};
    char at_req_4g[64] = {0};
    char at_rep_4g[64] = {0};
    char at_req_5g[64] = {0};
    char at_rep_5g[64] = {0};
    char  *ptr = NULL;

    if ( NULL == reqData )
    {
        nv_get("txpower_limit", txpowlmt_val, sizeof(txpowlmt_val));
    }
    else
    {
        snprintf ( txpowlmt_val, sizeof ( txpowlmt_val ), "%s", reqData );
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
    if ( atoi(txpowlmt_val) > 25 || !strcmp( txpowlmt_val, "") || atoi(txpowlmt_val) == 0  )
        strcpy( txpowlmt_val, "" );
    else if (atoi(txpowlmt_val) > 23 && atoi(txpowlmt_val) <= 25)
        strcpy( txpowlmt_val, "23" );
    else if( atoi(txpowlmt_val) <=4 )
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
}

