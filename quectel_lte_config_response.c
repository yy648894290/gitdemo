#include "comd_share.h"
#include "atchannel.h"
#include "at_tok.h"
#include "quectel_lte_config_response.h"
#include "quectel_lte_status_refresh.h"
#include "quectel_lte_atcmd_parsing.h"
#include "config_key.h"
#include "hal_platform.h"


extern int apns_msg_flag[5];

void quectel_lte_Param_lockPin ( char* reqData )
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

    quectel_lte_Param_updatePinStatus ();
    quectel_lte_Param_updatePinLockStatus ();
    quectel_lte_Param_updatePinPukCount ();

END:
    parsing_quectel_lte_lockpin ( at_rep );
}

void quectel_lte_Param_enterPin ( char* reqData )
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

    quectel_lte_Param_updatePinStatus ();
    quectel_lte_Param_updatePinPukCount ();

END:
    parsing_quectel_lte_enterpin ( at_rep );
}

void quectel_lte_Param_modifyPin ( char* reqData )
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

    quectel_lte_Param_updatePinStatus ();
    quectel_lte_Param_updatePinPukCount ();

END:
    parsing_quectel_lte_modifypin ( at_rep );
}

void quectel_lte_Param_enterPuk ( char* reqData )
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

    quectel_lte_Param_updatePinStatus ();
    quectel_lte_Param_updatePinPukCount ();

END:
    parsing_quectel_lte_enterpuk ( at_rep );
}

void quectel_lte_Param_config_commandsetting ( char* reqData )
{
    char at_rep[RECV_SMS_SIZE] = {0};

    if ( NULL == reqData )
    {
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

void quectel_lte_Param_radioOnOffSet ( char* reqData )
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

    if ( 0 == parsing_quectel_lte_cfun_set ( at_rep ) )
        nv_set ( "lte_on_off", reqData );
}

void quectel_lte_Param_netSelectModeSet ( char* reqData )
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

int quectel_lte_Param_lteRoamingSet ( char* reqData )
{
    char roaming_set[8] = {0};
    char at_req[32] = {0};
    char at_rep[64] = {0};
    char modulemodel[8] = {0};

    nv_get ( "modulemodel", modulemodel, sizeof ( modulemodel ) );

    if ( NULL != reqData )
    {
        strncpy ( roaming_set, reqData, sizeof ( roaming_set ) - 1 );
    }
    else
    {
        sys_get_config ( "lte_param.parameter.roaming", roaming_set, sizeof ( roaming_set ) );
    }

    if ( 0 == strcmp ( modulemodel, "EC200A" ) )
    {
        snprintf ( at_req, sizeof ( at_req ), "AT+QCFG=\"roamservice\",%s", strcmp ( roaming_set, "1" ) ? "2" : "1" );
    }
    else
    {
        snprintf ( at_req, sizeof ( at_req ), "AT+QNWPREFCFG=\"roam_pref\",%s", strcmp ( roaming_set, "1" ) ? "255" : "1" );
    }

    COMD_AT_PROCESS ( at_req, 3000, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
    {
        nv_set ( "roaming_set", "1" );
        return -1;
    }

    if ( NULL != reqData )
    {
        quectel_lte_Param_radioOnOffSet ( "off" );
        comd_wait_ms ( 1000 );
        quectel_lte_Param_radioOnOffSet ( "on" );
    }

    nv_set ( "roaming_set", "0" );

    return 0;
}

void quectel_lte_Param_getSimlock ( char* reqData )
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


void quectel_lte_Param_connectNetwork ( char* reqData )
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

void quectel_lte_Param_disconnectNetwork ( char* reqData )
{
    char at_rep[32] = {0};

    COMD_AT_PROCESS ( "AT+COPS=2", 10000, at_rep, sizeof ( at_rep ) );

    nv_set ( "disconnect_network_set", strstr ( at_rep, "\r\nOK\r\n" ) ? "0" : "1" );
}

void quectel_lte_Param_apnSetting ( apn_profile* const apnSetting_data )
{
    if ( NULL == apnSetting_data )
    {
        CLOGD ( ERROR, "apnSetting_data is NULL !!!\n" );
        nv_set ( "apn_set", "1" );
        return;
    }

    quectel_lte_Param_initAPNsetting ( ( char * ) apnSetting_data );

    return;
}

void quectel_lte_Param_lockband ( char* reqData, int lock_type )
{
    char band_value[256] = {0};
    char lockpci_status[16] = {0};
    char lockband_cmd[256] = {0};
    char at_rep[64] = {0};
    char mode_pref_cmd[64] = {0};
    char lockbands_4g[128] = {0};

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

        CLOGD ( FINE, "lockbands_4g -> [%s]\n", lockbands_4g );
        nv_set ( "lockband_list", lockbands_4g );
        if ( strcmp ( lockbands_4g, "" ) )
        {
            comd_strrpl ( lockbands_4g, "_", ":" );
            lockbands_4g [ strlen ( lockbands_4g ) - 1 ] = '\0';
        }
        snprintf ( lockband_cmd, sizeof ( lockband_cmd ), "AT+QNWPREFCFG=\"lte_band\",%s",
                                                                strcmp ( lockbands_4g, "" ) ? lockbands_4g : "0" );

        nv_set ( "lockband_status", "locked" );
    }
    else
    {
        snprintf ( mode_pref_cmd, sizeof ( mode_pref_cmd ), "AT+QNWPREFCFG=\"mode_pref\",%s", "LTE" );

        nv_get ( "suppband_org", lockbands_4g, sizeof ( lockbands_4g ) );
        snprintf ( lockband_cmd, sizeof ( lockband_cmd ), "AT+QNWPREFCFG=\"lte_band\",%s", lockbands_4g );

        nv_set ( "lockband_status", "notlocked" );
        nv_set ( "lockband_list", "" );
    }

    if ( 1 == lock_type )
    {
        quectel_lte_Param_radioOnOffSet ( "off" );
    }

    if ( strcmp ( lockbands_4g, "" ) )
    {
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( lockband_cmd, 3000, at_rep, sizeof ( at_rep ) );
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( mode_pref_cmd, 3000, at_rep, sizeof ( at_rep ) );
    }
    else
    {
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( mode_pref_cmd, 3000, at_rep, sizeof ( at_rep ) );

        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( lockband_cmd, 3000, at_rep, sizeof ( at_rep ) );
    }

    if ( 1 == lock_type )
    {
        comd_wait_ms ( 1000 );
        quectel_lte_Param_radioOnOffSet ( "on" );
        quectel_lte_Param_setRatPriority ( NULL );
    }

    nv_get ( "lockpci_status", lockpci_status, sizeof ( lockpci_status ) );

    if ( 0 == strcmp ( lockpci_status, "locked" ) )
    {
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( "AT+QNWLOCK=\"common/4g\",0", 3000, at_rep, sizeof ( at_rep ) );

        nv_set ( "lockpci_status", "notlocked" );
        nv_set ( "lockpci_list", "" );
    }

    nv_set ( "lockband_set", "0" );
}

void quectel_lte_Param_lockearfcn ( char* reqData, int lock_type )
{
    CLOGD ( FINE, "EG120K is not support lock earfcn!!!\n" );

    return;
}

void quectel_lte_Param_lockpci ( char* reqData, int lock_type )
{
    char *lock_content = NULL;
    char at_rep[64] = {0};
    int ret = 0;
    int tmp_band = 0;
    int tmp_arfcn = 0;
    int tmp_pci = 0;
    int lock4g_num = 0;
    char arfcn4g_pci_list[128] = {0};
    char mode_pref_cmd[64] = {0};
    char lock4g_cmd[256] = {0};
    char lockband_status[16] = {0};
    char lockpci_status[16] = {0};
    char lockband_cmd[256] = {0};
    char lockbands_4g[128] = {0};
    char lockband_list[128] = {0};
    char lockpci_list[256] = {0};

    if ( NULL == reqData )
    {
        CLOGD ( ERROR, "reqData is NULL !!!\n" );
        nv_set ( "lockpci_set", "1" );
        return;
    }

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
                           sizeof ( arfcn4g_pci_list ) - strlen ( arfcn4g_pci_list ),
                           ( lock4g_num > 1 ) ? ",%d,%d" : "%d,%d", tmp_arfcn, tmp_pci );
            }
        }

        lock_content = strtok ( NULL, ";" );
    }

    CLOGD ( FINE, "lock4g_num -> [%d]\n", lock4g_num );
    CLOGD ( FINE, "arfcn4g_pci_list -> [%s]\n", arfcn4g_pci_list );

    nv_get ( "lockband_status", lockband_status, sizeof ( lockband_status ) );// clean lock band
    sys_get_config ( WAN_LTE_LOCK_BAND, lockband_list, sizeof ( lockband_list ) );

    if ( 0 == strcmp ( lockband_status, "locked" ) )
    {
        if ( strcmp ( lockband_list, "" ) )
        {
            nv_get ( "suppband_org", lockbands_4g, sizeof ( lockbands_4g ) );
            snprintf ( lockband_cmd, sizeof ( lockband_cmd ), "AT+QNWPREFCFG=\"lte_band\",%s", lockbands_4g );
            memset ( at_rep, 0, sizeof ( at_rep ) );
            COMD_AT_PROCESS ( lockband_cmd, 4000, at_rep, sizeof ( at_rep ) );
        }

        nv_set ( "lockband_status", "notlocked" );
        nv_set ( "lockband_status", "" );
    }

    nv_get ( "lockpci_status", lockpci_status, sizeof ( lockpci_status ) );// clean lock pci
    sys_get_config ( WAN_LTE_LOCK_PCI, lockpci_list, sizeof ( lockpci_list ) );

    if ( 0 == strcmp ( lockpci_status, "locked" ) )
    {
        if ( strstr ( lockpci_list, "4G" ) )
        {
            memset ( at_rep, 0, sizeof ( at_rep ) );
            COMD_AT_PROCESS ( "AT+QNWLOCK=\"common/4g\",0", 4000, at_rep, sizeof ( at_rep ) );
        }
    }

    if ( 0 < lock4g_num )
    {
        snprintf ( mode_pref_cmd, sizeof ( mode_pref_cmd ), "AT+QNWPREFCFG=\"mode_pref\",%s", "LTE" );
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( mode_pref_cmd, 4000, at_rep, sizeof ( at_rep ) );

        snprintf ( lock4g_cmd, sizeof ( lock4g_cmd ), "AT+QNWLOCK=\"common/4g\",%d,%s", lock4g_num , arfcn4g_pci_list );
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( lock4g_cmd, 4000, at_rep, sizeof ( at_rep ) );
    }

    if ( 1 == lock_type )
    {
        quectel_lte_Param_radioOnOffSet ( "off" );
        comd_wait_ms ( 1000 );
        quectel_lte_Param_radioOnOffSet ( "on" );
        quectel_lte_Param_setRatPriority ( NULL );
    }

    nv_set ( "lockpci_status", "locked" );
    nv_set ( "lockpci_set", "0" );

    return;
}

void quectel_lte_Param_setDefaultGateway ( char* reqData )
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

void quectel_lte_Param_setSearchNeighbor ( char* reqData )
{
    char at_rep[RECV_BUF_SIZE * 2] = {0};
    int ret = 0;

    COMD_AT_PROCESS ( "AT+QENG=\"neighbourcell\"", 3000, at_rep, sizeof ( at_rep ) );

    ret = parsing_quectel_lte_neighbor_cellinfo ( at_rep );

    nv_set ( "search_neighbor_set", ( 0 < ret ) ? "0" : "1" );
}

void quectel_lte_Param_getNitzTime ( char* reqData )
{
    char at_rep[128] = {0};

    COMD_AT_PROCESS ( "AT+CTZU=1", 1500, at_rep, sizeof ( at_rep ) );

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( "AT+CCLK?", 1500, at_rep, sizeof ( at_rep ) );

    parsing_quectel_lte_cclk ( at_rep );
}

void quectel_lte_Param_dualSimSet ( char* reqData )
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

