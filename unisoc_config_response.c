#include "comd_share.h"
#include "comd_sms.h"
#include "atchannel.h"
#include "at_tok.h"
#include "unisoc_config_response.h"
#include "unisoc_status_refresh.h"
#include "unisoc_atcmd_parsing.h"
#include "config_key.h"
#include "hal_platform.h"

extern int apns_msg_flag[5];

static int scs2index ( int scs )
{
    switch ( scs )
    {
    case 15:
        return 0;
    case 30:
        return 1;
    case 60:
        return 2;
    case 120:
        return 3;
    case 240:
        return 4;
    default:
        return 0;
    }
}

void unisocParam_lockPin ( char* reqData )
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

    unisocParam_updatePinStatus ();
    unisocParam_updatePinLockStatus ();
    unisocParam_updatePinPukCount ();

END:
    parsing_unisoc_lockpin ( at_rep );
}

void unisocParam_enterPin ( char* reqData )
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

    unisocParam_updatePinStatus ();
    unisocParam_updatePinPukCount ();

END:
    parsing_unisoc_enterpin ( at_rep );
}

void unisocParam_modifyPin ( char* reqData )
{
    char oldCode[16] = {0};
    char *newCode = NULL;
    char at_rep[64] = {0};
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

    unisocParam_updatePinStatus ();
    unisocParam_updatePinPukCount ();
END:
    parsing_unisoc_modifypin ( at_rep );
}

void unisocParam_enterPuk ( char* reqData )
{
    char pukCode[16] = {0};
    char *newPinCode = NULL;
    char at_rep[64] = {0};
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

    unisocParam_updatePinStatus ();
    unisocParam_updatePinPukCount ();

END:
    parsing_unisoc_enterpuk ( at_rep );
}

void unisocParam_netSelectModeSet ( char* reqData )
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

void unisocParam_connectNetwork ( char* reqData )
{
    char at_rep[64] = {0};

    if ( NULL == reqData )
    {
        COMD_AT_PROCESS ( "AT+COPS?", 3000, at_rep, sizeof ( at_rep ) );

        if ( NULL == strstr ( at_rep, "+COPS: 0" ) )
        {
            memset ( at_rep, 0, sizeof ( at_rep ) );
            COMD_AT_PROCESS ( "AT+COPS=0", 10000, at_rep, sizeof ( at_rep ) );
        }
    }
    else
    {
        COMD_AT_PROCESS ( "AT+COPS=0", 3000, at_rep, sizeof ( at_rep ) );
    }

    nv_set ( "connect_network_set", "0" );
}

void unisocParam_disconnectNetwork ( char* reqData )
{
    char at_rep[32] = {0};

    COMD_AT_PROCESS ( "AT+COPS=2", 10000, at_rep, sizeof ( at_rep ) );

    nv_set ( "disconnect_network_set", strstr ( at_rep, "OK" ) ? "0" : "1" );
}

void unisocParam_radioOnOffSet ( char* reqData )
{
    char at_rep[64] = {0};

    if ( NULL == reqData )
    {
        CLOGD ( ERROR, "reqData is NULL !!!\n" );
    }
    else if ( 0 == strcmp ( reqData, "off" ) )
    {
        COMD_AT_PROCESS ( "AT+CFUN=4", 5000, at_rep, sizeof ( at_rep ) );
    }
    else
    {
        COMD_AT_PROCESS ( "AT+CFUN=1", 10000, at_rep, sizeof ( at_rep ) );
    }

    if ( 0 == parsing_unisoc_cfun_set ( at_rep ) )
        nv_set ( "lte_on_off", reqData );
}

void unisocParam_getSimlock ( char* reqData )
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

void unisocParam_apnSetting ( apn_profile* const apnSetting_data )
{
    if ( NULL == apnSetting_data )
    {
        CLOGD ( ERROR, "apnSetting_data is NULL !!!\n" );
        nv_set ( "apn_set", "1" );
        return;
    }

    unisocParam_initAPNsetting ( ( char * ) apnSetting_data );

    return;
}

void unisocParam_config_commandsetting ( char* reqData )
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

void unisocParam_lockband ( char* reqData, int lock_type )
{
    char band_value[256] = {0};
    char lockband_cmd[256] = {0};
    char lock5gband_cmd[256] = {0};
    char at_rep[128] = {0};
    char mode_pref_cmd[64] = {0};
    char lockbands_4g[128] = {0};
    char lockbands_5g[128] = {0};
    char ramRadioPrefer[8] = {0};
    char lockearfcn_status[16] = {0};
    char lockpci_status[16] = {0};
    char lockearfcn_list[256] = {0};
    char lockpci_list[256] = {0};

    if ( NULL == reqData )
    {
        CLOGD ( ERROR, "reqData is NULL !!!\n" );
        nv_set ( "lockband_set", "1" );
        return;
    }

    nv_get ( "radio_prefermode", ramRadioPrefer, sizeof ( ramRadioPrefer ) );
    if ( 0 == strcmp ( ramRadioPrefer, "" ) )
    {
        sys_get_config ( WAN_LTE_LOCK_PREFER_RADIO, ramRadioPrefer, sizeof ( ramRadioPrefer ) );
        nv_set ( "radio_prefermode", ramRadioPrefer );
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
            snprintf ( mode_pref_cmd, sizeof ( mode_pref_cmd ), "AT+QNWPREFCFG=\"mode_pref\",%s", "NR5G-SA" );
        }
        else // 4G & 5G
        {
            sscanf ( band_value, "%[0-9_],%[0-9_]", lockbands_4g, lockbands_5g );
            snprintf ( mode_pref_cmd, sizeof ( mode_pref_cmd ), "AT+QNWPREFCFG=\"mode_pref\",%s", strcmp ( ramRadioPrefer, "4G" ) ? "NR5G:LTE" : "LTE:NR5G" );
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
        snprintf ( mode_pref_cmd, sizeof ( mode_pref_cmd ), "AT+QNWPREFCFG=\"mode_pref\",%s", strcmp ( ramRadioPrefer, "4G" ) ? "NR5G:LTE" : "LTE:NR5G" );

        nv_get ( "suppband_org", lockbands_4g, sizeof ( lockbands_4g ) );
        snprintf ( lockband_cmd, sizeof ( lockband_cmd ), "AT+QNWPREFCFG=\"lte_band\",%s", lockbands_4g );

        nv_get ( "suppband5g_org", lockbands_5g, sizeof ( lockbands_5g ) );
        snprintf ( lock5gband_cmd, sizeof ( lock5gband_cmd ), "AT+QNWPREFCFG=\"nr5g_band\",%s", lockbands_5g );

        nv_set ( "lockband_status", "notlocked" );
        nv_set ( "lockband_list", "" );
        nv_set ( "lockband5g_list", "" );
    }

    nv_get ( "lockearfcn_status", lockearfcn_status, sizeof ( lockearfcn_status ) );
    sys_get_config ( WAN_LTE_LOCK_FREQ, lockearfcn_list, sizeof ( lockearfcn_list ) );

    if ( 0 == strcmp ( lockearfcn_status, "locked" ) )
    {
        if ( strstr ( lockearfcn_list, "4G" ) )
        {
            memset ( at_rep, 0, sizeof ( at_rep ) );
            COMD_AT_PROCESS ( "AT+QNWLOCKFREQ=\"common/lte\",0", 4000, at_rep, sizeof ( at_rep ) );
        }

        if ( strstr ( lockearfcn_list, "5G" ) )
        {
            memset ( at_rep, 0, sizeof ( at_rep ) );
            COMD_AT_PROCESS ( "AT+QNWLOCKFREQ=\"common/5g\",0", 4000, at_rep, sizeof ( at_rep ) );
        }

        nv_set ( "lockearfcn_status", "notlocked" );
        nv_set ( "lockearfcn_list", "" );
    }

    nv_get ( "lockpci_status", lockpci_status, sizeof ( lockpci_status ) );
    sys_get_config ( WAN_LTE_LOCK_PCI, lockpci_list, sizeof ( lockpci_list ) );

    if ( 0 == strcmp ( lockpci_status, "locked" ) )
    {
        if ( strstr ( lockpci_list, "4G" ) )
        {
            memset ( at_rep, 0, sizeof ( at_rep ) );
            COMD_AT_PROCESS ( "AT+QNWLOCK=\"common/lte\",0", 4000, at_rep, sizeof ( at_rep ) );
        }

        if ( strstr ( lockpci_list, "5G" ) )
        {
            memset ( at_rep, 0, sizeof ( at_rep ) );
            COMD_AT_PROCESS ( "AT+QNWLOCK=\"common/5g\",0", 4000, at_rep, sizeof ( at_rep ) );
        }

        nv_set ( "lockpci_status", "notlocked" );
        nv_set ( "lockpci_list", "" );
    }

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( mode_pref_cmd, 4000, at_rep, sizeof ( at_rep ) );

    if ( strcmp ( lockbands_4g, "" ) && !strcmp ( lockbands_5g, "" ) )
    {
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( lockband_cmd, 4000, at_rep, sizeof ( at_rep ) );
    }
    else if ( !strcmp ( lockbands_4g, "" ) && strcmp ( lockbands_5g, "" ) )
    {
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( lock5gband_cmd, 4000, at_rep, sizeof ( at_rep ) );
    }
    else
    {
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( lockband_cmd, 4000, at_rep, sizeof ( at_rep ) );

        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( lock5gband_cmd, 4000, at_rep, sizeof ( at_rep ) );
    }

    nv_set ( "lockband_set", "0" );

    if ( 1 == lock_type )
    {
        unisocParam_radioOnOffSet ( "off" );
        comd_wait_ms ( 1000 );
        unisocParam_radioOnOffSet ( "on" );
    }
}

void unisocParam_lockearfcn ( char* reqData, int lock_type )
{
    char ramRadioPrefer[8] = {0};
    char *lock_content = NULL;
    char at_rep[64] = {0};
    int ret = 0;
    int tmp_band = 0;
    int tmp_arfcn = 0;
    int tmp_scs = 0;
    int lock4g_num = 0;
    char arfcn4g_list[128] = {0};
    int lock5g_num = 0;
    char arfcn5g_list[128] = {0};
    char scs_list[32] = {0};
    char mode_pref_cmd[64] = {0};
    char lock4g_cmd[256] = {0};
    char lock5g_cmd[256] = {0};
    char lockband_status[16] = {0};
    char lockband_cmd[256] = {0};
    char lock5gband_cmd[256] = {0};
    char lockbands_4g[128] = {0};
    char lockbands_5g[128] = {0};
    char lockpci_status[16] = {0};
    char lockearfcn_status[16] = {0};
    char lockband_list[128] = {0};
    char lockband5g_list[128] = {0};
    char lockearfcn_list[256] = {0};
    char lockpci_list[256] = {0};

    if ( NULL == reqData )
    {
        CLOGD ( ERROR, "reqData is NULL !!!\n" );
        nv_set ( "lockearfcn_set", "1" );
        return;
    }

    nv_get ( "radio_prefermode", ramRadioPrefer, sizeof ( ramRadioPrefer ) );
    if ( 0 == strcmp ( ramRadioPrefer, "" ) )
    {
        sys_get_config ( WAN_LTE_LOCK_PREFER_RADIO, ramRadioPrefer, sizeof ( ramRadioPrefer ) );
        nv_set ( "radio_prefermode", ramRadioPrefer );
    }

    lock_content = strtok ( reqData, ";" );
    while ( lock_content )
    {
        if ( strstr ( lock_content, "4G," ) )
        {
            ret = sscanf ( lock_content, "4G,%d,%d", &tmp_band, &tmp_arfcn );
            if ( 2 == ret )
            {
                lock4g_num++;
                snprintf ( arfcn4g_list + strlen ( arfcn4g_list ),
                           sizeof ( arfcn4g_list ) - strlen ( arfcn4g_list ),
                           ( lock4g_num > 1 ) ? "-%d" : "%d", tmp_arfcn );
            }
        }
        else if ( strstr ( lock_content, "5G," ) )
        {
            ret = sscanf ( lock_content, "5G,%d,%d,%d", &tmp_band, &tmp_arfcn, &tmp_scs );
            if ( 3 == ret )
            {
                lock5g_num++;
                snprintf ( arfcn5g_list + strlen ( arfcn5g_list ),
                           sizeof ( arfcn5g_list ) - strlen ( arfcn5g_list ),
                           ( lock5g_num > 1 ) ? "-%d" : "%d", tmp_arfcn );
                snprintf ( scs_list + strlen ( scs_list ),
                           sizeof ( scs_list ) - strlen ( scs_list ),
                           ( lock5g_num > 1 ) ? ",%d" : "%d", scs2index ( tmp_scs ) );
            }
        }

        lock_content = strtok ( NULL, ";" );
    }

    CLOGD ( FINE, "lock4g_num -> [%d]\n", lock4g_num );
    CLOGD ( FINE, "arfcn4g_list -> [%s]\n", arfcn4g_list );

    CLOGD ( FINE, "lock5g_num -> [%d]\n", lock5g_num );
    CLOGD ( FINE, "arfcn5g_list -> [%s]\n", arfcn5g_list );

    nv_get ( "lockband_status", lockband_status, sizeof ( lockband_status ) );// clean lock band
    sys_get_config ( WAN_LTE_LOCK_BAND, lockband_list, sizeof ( lockband_list ) );
    sys_get_config ( WAN_LTE_LOCK_BAND5G, lockband5g_list, sizeof ( lockband5g_list ) );

    if ( 0 == strcmp ( lockband_status, "locked" ) )
    {
        if ( strcmp ( lockband_list, "" ) )
        {
            nv_get ( "suppband_org", lockbands_4g, sizeof ( lockbands_4g ) );
            snprintf ( lockband_cmd, sizeof ( lockband_cmd ), "AT+QNWPREFCFG=\"lte_band\",%s", lockbands_4g );
            memset ( at_rep, 0, sizeof ( at_rep ) );
            COMD_AT_PROCESS ( lockband_cmd, 4000, at_rep, sizeof ( at_rep ) );
        }

        if ( strcmp ( lockband5g_list, "" ) )
        {
            nv_get ( "suppband5g_org", lockbands_5g, sizeof ( lockbands_5g ) );
            snprintf ( lock5gband_cmd, sizeof ( lock5gband_cmd ), "AT+QNWPREFCFG=\"nr5g_band\",%s", lockbands_5g );
            memset ( at_rep, 0, sizeof ( at_rep ) );
            COMD_AT_PROCESS ( lock5gband_cmd, 4000, at_rep, sizeof ( at_rep ) );
        }

        nv_set ( "lockband_status", "notlocked" );
        nv_set ( "lockband_status", "" );
    }

    nv_get ( "lockearfcn_status", lockearfcn_status, sizeof ( lockearfcn_status ) );// clean lock earfcn
    sys_get_config ( WAN_LTE_LOCK_FREQ, lockearfcn_list, sizeof ( lockearfcn_list ) );

    if ( 0 == strcmp ( lockearfcn_status, "locked" ) )
    {
        if ( strstr ( lockearfcn_list, "4G" ) )
        {
            memset ( at_rep, 0, sizeof ( at_rep ) );
            COMD_AT_PROCESS ( "AT+QNWLOCKFREQ=\"common/lte\",0", 4000, at_rep, sizeof ( at_rep ) );
        }

        if ( strstr ( lockearfcn_list, "5G" ) )
        {
            memset ( at_rep, 0, sizeof ( at_rep ) );
            COMD_AT_PROCESS ( "AT+QNWLOCKFREQ=\"common/5g\",0", 4000, at_rep, sizeof ( at_rep ) );
        }
    }

    nv_get ( "lockpci_status", lockpci_status, sizeof ( lockpci_status ) );// clean lock pci
    sys_get_config ( WAN_LTE_LOCK_PCI, lockpci_list, sizeof ( lockpci_list ) );

    if ( 0 == strcmp ( lockpci_status, "locked" ) )
    {
        if ( strstr ( lockpci_list, "4G" ) )
        {
            memset ( at_rep, 0, sizeof ( at_rep ) );
            COMD_AT_PROCESS ( "AT+QNWLOCK=\"common/lte\",0", 4000, at_rep, sizeof ( at_rep ) );
        }

        if ( strstr ( lockpci_list, "5G" ) )
        {
            memset ( at_rep, 0, sizeof ( at_rep ) );
            COMD_AT_PROCESS ( "AT+QNWLOCK=\"common/5g\",0", 4000, at_rep, sizeof ( at_rep ) );
        }

        nv_set ( "lockpci_status", "notlocked" );
        nv_set ( "lockpci_list", "" );
    }

    if ( 0 < lock4g_num && 0 < lock5g_num )
    {
        snprintf ( mode_pref_cmd, sizeof ( mode_pref_cmd ), "AT+QNWPREFCFG=\"mode_pref\",%s", strcmp ( ramRadioPrefer, "4G" ) ? "NR5G:LTE" : "LTE:NR5G" );
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( mode_pref_cmd, 4000, at_rep, sizeof ( at_rep ) );

        snprintf ( lock4g_cmd, sizeof ( lock4g_cmd ), "AT+QNWLOCKFREQ=\"common/lte\",1,%s", arfcn4g_list );
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( lock4g_cmd, 4000, at_rep, sizeof ( at_rep ) );

        snprintf ( lock5g_cmd, sizeof ( lock5g_cmd ), "AT+QNWLOCKFREQ=\"common/5g\",1,%s", arfcn5g_list );
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( lock5g_cmd, 4000, at_rep, sizeof ( at_rep ) );
    }
    else if ( 0 < lock4g_num )
    {
        snprintf ( mode_pref_cmd, sizeof ( mode_pref_cmd ), "AT+QNWPREFCFG=\"mode_pref\",%s", "LTE" );
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( mode_pref_cmd, 4000, at_rep, sizeof ( at_rep ) );

        snprintf ( lock4g_cmd, sizeof ( lock4g_cmd ), "AT+QNWLOCKFREQ=\"common/lte\",1,%s", arfcn4g_list );
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( lock4g_cmd, 4000, at_rep, sizeof ( at_rep ) );
    }
    else if ( 0 < lock5g_num )
    {
        snprintf ( mode_pref_cmd, sizeof ( mode_pref_cmd ), "AT+QNWPREFCFG=\"mode_pref\",%s", "NR5G" );
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( mode_pref_cmd, 4000, at_rep, sizeof ( at_rep ) );

        snprintf ( lock5g_cmd, sizeof ( lock5g_cmd ), "AT+QNWLOCKFREQ=\"common/5g\",1,%s", arfcn5g_list );
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( lock5g_cmd, 4000, at_rep, sizeof ( at_rep ) );
    }

    nv_set ( "lockearfcn_status", "locked" );
    nv_set ( "lockearfcn_set", "0" );

    if ( 1 == lock_type )
    {
        unisocParam_radioOnOffSet ( "off" );
        comd_wait_ms ( 1000 );
        unisocParam_radioOnOffSet ( "on" );
    }

    return;
}

void unisocParam_lockpci ( char* reqData, int lock_type )
{
    char ramRadioPrefer[8] = {0};
    char *lock_content = NULL;
    char at_rep[64] = {0};
    int ret = 0;
    int tmp_band = 0;
    int tmp_arfcn = 0;
    int tmp_pci = 0;
    int tmp_scs = 0;
    int lock4g_num = 0;
    char arfcn4g_pci_list[128] = {0};
    int lock5g_num = 0;
    char arfcn5g_pci_list[256] = {0};
    char mode_pref_cmd[64] = {0};
    char lock4g_cmd[256] = {0};
    char lock5g_cmd[256] = {0};
    char lockband_status[16] = {0};
    char lockpci_status[16] = {0};
    char lockearfcn_status[16] = {0};
    char lockband_cmd[256] = {0};
    char lock5gband_cmd[256] = {0};
    char lockbands_4g[128] = {0};
    char lockbands_5g[128] = {0};
    char lockband_list[128] = {0};
    char lockband5g_list[128] = {0};
    char lockearfcn_list[256] = {0};
    char lockpci_list[256] = {0};

    if ( NULL == reqData )
    {
        CLOGD ( ERROR, "reqData is NULL !!!\n" );
        nv_set ( "lockpci_set", "1" );
        return;
    }

    nv_get ( "radio_prefermode", ramRadioPrefer, sizeof ( ramRadioPrefer ) );
    if ( 0 == strcmp ( ramRadioPrefer, "" ) )
    {
        sys_get_config ( WAN_LTE_LOCK_PREFER_RADIO, ramRadioPrefer, sizeof ( ramRadioPrefer ) );
        nv_set ( "radio_prefermode", ramRadioPrefer );
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
        else if ( strstr ( lock_content, "5G," ) )
        {
            ret = sscanf ( lock_content, "5G,%d,%d,%d,%d", &tmp_band, &tmp_arfcn, &tmp_pci, &tmp_scs );
            if ( 4 == ret )
            {
                lock5g_num++;
                snprintf ( arfcn5g_pci_list + strlen ( arfcn5g_pci_list ),
                           sizeof ( arfcn5g_pci_list ) - strlen ( arfcn5g_pci_list ),
                           ( lock5g_num > 1 ) ? ",%d,%d" : "%d,%d", tmp_arfcn, tmp_pci );
            }
        }

        lock_content = strtok ( NULL, ";" );
    }

    CLOGD ( FINE, "lock4g_num -> [%d]\n", lock4g_num );
    CLOGD ( FINE, "arfcn4g_pci_list -> [%s]\n", arfcn4g_pci_list );

    CLOGD ( FINE, "lock5g_num -> [%d]\n", lock5g_num );
    CLOGD ( FINE, "arfcn5g_pci_list -> [%s]\n", arfcn5g_pci_list );

    nv_get ( "lockband_status", lockband_status, sizeof ( lockband_status ) );// clean lock band
    sys_get_config ( WAN_LTE_LOCK_BAND, lockband_list, sizeof ( lockband_list ) );
    sys_get_config ( WAN_LTE_LOCK_BAND5G, lockband5g_list, sizeof ( lockband5g_list ) );

    if ( 0 == strcmp ( lockband_status, "locked" ) )
    {
        if ( strcmp ( lockband_list, "" ) )
        {
            nv_get ( "suppband_org", lockbands_4g, sizeof ( lockbands_4g ) );
            snprintf ( lockband_cmd, sizeof ( lockband_cmd ), "AT+QNWPREFCFG=\"lte_band\",%s", lockbands_4g );
            memset ( at_rep, 0, sizeof ( at_rep ) );
            COMD_AT_PROCESS ( lockband_cmd, 4000, at_rep, sizeof ( at_rep ) );
        }

        if ( strcmp ( lockband5g_list, "" ) )
        {
            nv_get ( "suppband5g_org", lockbands_5g, sizeof ( lockbands_5g ) );
            snprintf ( lock5gband_cmd, sizeof ( lock5gband_cmd ), "AT+QNWPREFCFG=\"nr5g_band\",%s", lockbands_5g );
            memset ( at_rep, 0, sizeof ( at_rep ) );
            COMD_AT_PROCESS ( lock5gband_cmd, 4000, at_rep, sizeof ( at_rep ) );
        }

        nv_set ( "lockband_status", "notlocked" );
        nv_set ( "lockband_status", "" );
    }

    nv_get ( "lockearfcn_status", lockearfcn_status, sizeof ( lockearfcn_status ) );// clean lock earfcn
    sys_get_config ( WAN_LTE_LOCK_FREQ, lockearfcn_list, sizeof ( lockearfcn_list ) );

    if ( 0 == strcmp ( lockearfcn_status, "locked" ) )
    {
        if ( strstr ( lockearfcn_list, "4G" ) )
        {
            memset ( at_rep, 0, sizeof ( at_rep ) );
            COMD_AT_PROCESS ( "AT+QNWLOCKFREQ=\"common/lte\",0", 4000, at_rep, sizeof ( at_rep ) );
        }

        if ( strstr ( lockearfcn_list, "5G" ))
        {
            memset ( at_rep, 0, sizeof ( at_rep ) );
            COMD_AT_PROCESS ( "AT+QNWLOCKFREQ=\"common/5g\",0", 4000, at_rep, sizeof ( at_rep ) );
        }

        nv_set ( "lockearfcn_status", "notlocked" );
        nv_set ( "lockearfcn_list", "" );
    }

    nv_get ( "lockpci_status", lockpci_status, sizeof ( lockpci_status ) );// clean lock pci
    sys_get_config ( WAN_LTE_LOCK_PCI, lockpci_list, sizeof ( lockpci_list ) );

    if ( 0 == strcmp ( lockpci_status, "locked" ) )
    {
        if ( strstr ( lockpci_list, "4G" ) )
        {
            memset ( at_rep, 0, sizeof ( at_rep ) );
            COMD_AT_PROCESS ( "AT+QNWLOCK=\"common/lte\",0", 4000, at_rep, sizeof ( at_rep ) );
        }

        if ( strstr ( lockpci_list, "5G" ) )
        {
            memset ( at_rep, 0, sizeof ( at_rep ) );
            COMD_AT_PROCESS ( "AT+QNWLOCK=\"common/5g\",0", 4000, at_rep, sizeof ( at_rep ) );
        }
    }

    if ( 0 < lock4g_num && 0 < lock5g_num )
    {
        snprintf ( mode_pref_cmd, sizeof ( mode_pref_cmd ), "AT+QNWPREFCFG=\"mode_pref\",%s", strcmp ( ramRadioPrefer, "4G" ) ? "NR5G:LTE" : "LTE:NR5G" );
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( mode_pref_cmd, 4000, at_rep, sizeof ( at_rep ) );

        snprintf ( lock4g_cmd, sizeof ( lock4g_cmd ), "AT+QNWLOCK=\"common/lte\",%d,%s", lock4g_num , arfcn4g_pci_list );
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( lock4g_cmd, 4000, at_rep, sizeof ( at_rep ) );

        snprintf ( lock5g_cmd, sizeof ( lock5g_cmd ), "AT+QNWLOCK=\"common/5g\",%d,%s", lock5g_num , arfcn5g_pci_list );
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( lock5g_cmd, 4000, at_rep, sizeof ( at_rep ) );
    }
    else if ( 0 < lock4g_num )
    {
        snprintf ( mode_pref_cmd, sizeof ( mode_pref_cmd ), "AT+QNWPREFCFG=\"mode_pref\",%s", "LTE" );
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( mode_pref_cmd, 4000, at_rep, sizeof ( at_rep ) );

        snprintf ( lock4g_cmd, sizeof ( lock4g_cmd ), "AT+QNWLOCK=\"common/lte\",%d,%s", lock4g_num , arfcn4g_pci_list );
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( lock4g_cmd, 4000, at_rep, sizeof ( at_rep ) );
    }
    else if ( 0 < lock5g_num )
    {
        snprintf ( mode_pref_cmd, sizeof ( mode_pref_cmd ), "AT+QNWPREFCFG=\"mode_pref\",%s", "NR5G" );
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( mode_pref_cmd, 4000, at_rep, sizeof ( at_rep ) );

        snprintf ( lock5g_cmd, sizeof ( lock5g_cmd ), "AT+QNWLOCK=\"common/5g\",%d,%s", lock5g_num , arfcn5g_pci_list );
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( lock5g_cmd, 4000, at_rep, sizeof ( at_rep ) );
    }

    nv_set ( "lockpci_status", "locked" );
    nv_set ( "lockpci_set", "0" );

    if ( 1 == lock_type )
    {
        unisocParam_radioOnOffSet ( "off" );
        comd_wait_ms ( 1000 );
        unisocParam_radioOnOffSet ( "on" );
    }

    return;
}

void unisocParam_setDefaultGateway ( char* reqData )
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

int unisocParam_setRatPriority ( char* reqData )
{

    return 0;

    char at_req[64] = {0};
    char at_rep[64] = {0};
    char uciPreferRadio[8] = {0};

    if ( NULL == reqData )
    {
        sys_get_config ( WAN_LTE_LOCK_PREFER_RADIO, uciPreferRadio, sizeof ( uciPreferRadio ) );
    }
    else
    {
        snprintf ( uciPreferRadio, sizeof ( uciPreferRadio ), "%s", reqData );
    }

    snprintf ( at_req, sizeof ( at_req ), "AT+QNWPREFCFG=\"mode_pref\",%s", strcmp ( uciPreferRadio, "4G" ) ? "NR5G:LTE" : "LTE:NR5G" );

    COMD_AT_PROCESS ( at_req, 3000, at_rep, sizeof ( at_rep ) );

    return strstr ( at_rep, "OK" ) ? 0 : 1;
}

void unisocParam_setSearchNeighbor ( char* reqData )
{
    char at_rep[RECV_BUF_SIZE * 2] = {0};
    int ret = 0;

    COMD_AT_PROCESS ( "AT+QENG=\"neighbourcell\"", 3000, at_rep, sizeof ( at_rep ) );

    ret = parsing_unisoc_neighbor_cellinfo ( at_rep );

    nv_set ( "search_neighbor_set", ( 0 < ret ) ? "0" : "1" );
}

void unisocParam_getNitzTime ( char* reqData )
{
    char at_rep[128] = {0};

    COMD_AT_PROCESS ( "AT+CTZU=1", 1500, at_rep, sizeof ( at_rep ) );

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( "AT+CCLK?", 1500, at_rep, sizeof ( at_rep ) );

    parsing_unisoc_cclk ( at_rep );
}

/*
// 0 开启国内国际漫游
// 1 开启国内漫游，关闭国际漫游
// 2 关闭国内漫游，开启国际漫游
// 3 关闭国内国际漫游
*/
int unisocParam_lteRoamingSet ( char* reqData )
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
        sys_get_config ( LTE_PARAMETER_ROAMING, roaming_set, sizeof ( roaming_set ) );
    }

    snprintf ( at_req, sizeof ( at_req ), "AT+QNWPREFCFG=\"roam_pref\",%s", strcmp ( roaming_set, "1" ) ? "0" : "3" );

    COMD_AT_PROCESS ( at_req, 3000, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "OK" ) )
    {
        nv_set ( "roaming_set", "1" );
        return -1;
    }

    nv_set ( "roaming_set", "0" );

    if ( NULL != reqData )
    {
        unisocParam_radioOnOffSet ( "off" );
        comd_wait_ms ( 1000 );
        unisocParam_radioOnOffSet ( "on" );
        unisocParam_setRatPriority ( NULL );
    }

    return 0;
}

void unisocParam_dualSimSet ( char* reqData )
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

void unisocParam_sendSmsRequest ( KT_sms_msg *send_sms )
{
    char at_req[SEND_MAX_SIZE] = {0};
    char at_rep[RECV_SMS_SIZE] = {0};

    CLOGD ( FINE, "send_sms->send_len: [%d]\n", send_sms->num_len );
    CLOGD ( FINE, "send_sms->send_pdu: [%s]\n", send_sms->sms_msg );

    snprintf ( at_req, sizeof ( at_req ), "AT+CMGS=%d;%s",
                                send_sms->num_len, send_sms->sms_msg );

    COMD_AT_PROCESS ( at_req, 5000, at_rep, sizeof ( at_rep ) );
    parsing_unisoc_gcmgs ( at_rep );
}

void unisocParam_sendUssdCode( char* reqData )
{
    int ret_mode = 0;
    char at_req[256] = {0};
    char at_rep[64] = {0};

    if ( NULL == reqData )
    {
        CLOGD ( ERROR, "reqData is NULL !!!\n" );
        nv_set ( "ussd_code_set", "1" );
        return;
    }

    if ( 0 == strcmp ( reqData, "*2*2*2*2*2*2*2#" ) )
    {
        ret_mode = 1;
        snprintf ( at_req, sizeof ( at_req ), "%s", "AT+CUSD=2" );
    }
    else
    {
        snprintf ( at_req, sizeof ( at_req ), "AT+CUSD=1,\"%s\",15", reqData );
    }

    COMD_AT_PROCESS ( at_req, 1500, at_rep, sizeof ( at_rep ) );

    parsing_unisoc_cusd_set ( at_rep, ret_mode );
}

void unisocParam_setModemLog ( char* reqData )
{
    if ( NULL == reqData )
    {
        CLOGD ( ERROR, "reqData is NULL !!!\n" );
        nv_set ( "modem_log_set", "1" );
        return;
    }


}
