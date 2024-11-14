#include "comd_share.h"
#include "atchannel.h"
#include "tdtek_config_response.h"
#include "tdtek_status_refresh.h"
#include "tdtek_atcmd_parsing.h"
#include "config_key.h"
#include "hal_platform.h"

extern int manual_conn;
extern int apns_msg_flag[5];
extern char SYSCFGEX_F[32];
extern char SYSCFGEX_S[32];

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

/*
 * n_type:
 *      "03": 4G only
 *      "08": 5G only
 *      "0308": 4G & 5G, pref 4G
 *      "0803": 4G & 5G, pref 5G
 *      "99": no change
 * p_type:
 *      0: CS only
 *      1: PS only
 *      2: CS & PS
 *      99: no change
 * n_type:
 *      0: 启用国内国际漫游
 *      1: 只启用国内漫游
 *      2: 只启用国际漫游
 *      3: 禁用国内国际漫游
 *      99: no change
 */
static void create_syscfgex ( char *o_cmd, char *n_type, int r_type, int p_type )
{
    static char s_n_type[16] = "0803";
    static int s_r_type = 1;
    static int s_p_type = 2;

    if ( strcmp ( n_type, "99" ) )
        snprintf ( s_n_type, sizeof ( s_n_type ), "%s", n_type );

    if ( r_type < 99 )
        s_r_type = r_type;

    if ( p_type < 99 )
        s_p_type = p_type;

    sprintf ( o_cmd, "AT^SYSCFGEX=\"%s\",%s,%d,%d,%s,,", s_n_type, SYSCFGEX_F, s_r_type, s_p_type, SYSCFGEX_S );
}

void tdtekParam_lockPin ( char* reqData )
{
    char pEnable[4] = {0};
    char *pinCode = NULL;
    char at_rep[RECV_BUF_SIZE * 2] = {0};
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

    tdtekParam_updatePinStatus ();
    tdtekParam_updatePinLockStatus ();
    tdtekParam_updatePinPukCount ();

END:
    parsing_tdtek_lockpin ( at_rep );
}

void tdtekParam_enterPin ( char* reqData )
{
    char at_rep[RECV_BUF_SIZE * 2] = {0};
    char pin_unlock[32] = {0};

    if ( NULL == reqData )
    {
        CLOGD ( ERROR, "reqData is NULL !!!\n" );
        goto END;
    }

    snprintf ( pin_unlock, sizeof(pin_unlock), "AT+CPIN=\"%s\"", reqData );

    COMD_AT_PROCESS ( pin_unlock, 1500, at_rep, sizeof ( at_rep ) );

    tdtekParam_updatePinStatus ();
    tdtekParam_updatePinPukCount ();

END:
    parsing_tdtek_enterpin ( at_rep );
}

void tdtekParam_modifyPin ( char* reqData )
{
    char oldCode[16] = {0};
    char *newCode = NULL;
    char pin_cpwd[64] = {0};
    char at_rep[RECV_BUF_SIZE * 2] = {0};

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

    tdtekParam_updatePinStatus ();
    tdtekParam_updatePinPukCount ();

END:
    parsing_tdtek_modifypin ( at_rep );
}

void tdtekParam_config_commandsetting ( char* reqData )
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

void tdtekParam_enterPuk ( char* reqData )
{
    char pukCode[16] = {0};
    char *newPinCode = NULL;
    char pin_update[64] = {0};
    char at_rep[RECV_BUF_SIZE * 2] = {0};

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

    tdtekParam_updatePinStatus ();
    tdtekParam_updatePinPukCount ();

END:
    parsing_tdtek_enterpuk ( at_rep );
}

void tdtekParam_connectNetwork ( char* reqData )
{
    char at_rep[32] = {0};

    if ( 0 == manual_conn )
    {
        manual_conn = 1;
        COMD_AT_PROCESS ( "AT+CGATT=1", 10000, at_rep, sizeof ( at_rep ) );
    }

    nv_set ( "connect_network_set", "0" );
}

void tdtekParam_disconnectNetwork ( char* reqData )
{
    char at_rep[32] = {0};

    manual_conn = 0;

    COMD_AT_PROCESS ( "AT+CGATT=0", 10000, at_rep, sizeof ( at_rep ) );

    nv_set ( "disconnect_network_set", strstr ( at_rep, "\r\nOK\r\n" ) ? "0" : "1" );
}

void tdtekParam_apnSetting ( apn_profile* const apnSetting_data )
{
    if ( NULL == apnSetting_data )
    {
        CLOGD ( ERROR, "apnSetting_data is NULL !!!\n" );
        nv_set ( "apn_set", "1" );
        return;
    }

    tdtekParam_initAPNsetting ( ( char * ) apnSetting_data );

    return;
}

int tdtekParam_setRatPriority ( char* reqData )
{
#if 0
    char at_req[128] = {0};
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

    create_syscfgex ( at_req, strcmp ( uciPreferRadio, "4G" ) ? "0803" : "0308", 99, 99 );

    COMD_AT_PROCESS ( at_req, 3000, at_rep, sizeof ( at_rep ) );

    return strstr ( at_rep, "\r\nOK\r\n" ) ? 0 : 1;
#endif

    return 0;
}

void tdtekParam_lockband ( char* reqData, int lock_type )
{
    int ret = 0;
    char at_rep[64] = {0};
    char band_value[256] = {0};
    char ramRadioPrefer[8] = {0};
    char lockbands_4g[128] = {0};
    char lockbands_5g[128] = {0};
    char mode_pref_cmd[64] = {0};
    char lockband_cmd[256] = {0};
    char lock5gband_cmd[256] = {0};

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
            create_syscfgex ( mode_pref_cmd, "03", 99, 99 );
        }
        else if ( ',' == band_value[0] ) // 5G only
        {
            sscanf ( band_value, ",%[0-9_]", lockbands_5g );
            create_syscfgex ( mode_pref_cmd, "08", 99, 99 );
        }
        else // 4G & 5G
        {
            sscanf ( band_value, "%[0-9_],%[0-9_]", lockbands_4g, lockbands_5g );
            create_syscfgex ( mode_pref_cmd, strcmp ( ramRadioPrefer, "4G" ) ? "0803" : "0308", 99, 99 );
        }

        CLOGD ( FINE, "lockbands_4g -> [%s]\n", lockbands_4g );
        nv_set ( "lockband_list", lockbands_4g );
        ret = comd_strrpl ( lockbands_4g, "_", "," );
        if ( 0 < ret )
        {
            lockbands_4g [ strlen ( lockbands_4g ) - 1 ] = '\0';
            snprintf ( lockband_cmd, sizeof ( lockband_cmd ), "AT^LTEFREQLOCK=3,0,%d,\"%s\"", ret, lockbands_4g );
        }
        else
        {
            snprintf ( lockband_cmd, sizeof ( lockband_cmd ), "%s", "AT^LTEFREQLOCK=0" );
        }

        CLOGD ( FINE, "lockbands_5g -> [%s]\n", lockbands_5g );
        nv_set ( "lockband5g_list", lockbands_5g );
        ret = comd_strrpl ( lockbands_5g, "_", "," );
        if ( 0 < ret )
        {
            lockbands_5g [ strlen ( lockbands_5g ) - 1 ] = '\0';
            snprintf ( lock5gband_cmd, sizeof ( lock5gband_cmd ), "AT^NRFREQLOCK=3,0,%d,\"%s\"", ret, lockbands_5g );
        }
        else
        {
            snprintf ( lock5gband_cmd, sizeof ( lock5gband_cmd ), "%s", "AT^NRFREQLOCK=0" );
        }

        nv_set ( "lockband_status", "locked" );
    }
    else
    {
        create_syscfgex ( mode_pref_cmd, strcmp ( ramRadioPrefer, "4G" ) ? "0803" : "0308", 99, 99 );
        snprintf ( lockband_cmd, sizeof ( lockband_cmd ), "%s", "AT^LTEFREQLOCK=0" );
        snprintf ( lock5gband_cmd, sizeof ( lock5gband_cmd ), "%s", "AT^NRFREQLOCK=0" );

        nv_set ( "lockband_status", "notlocked" );
        nv_set ( "lockband_list", "" );
        nv_set ( "lockband5g_list", "" );
    }

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( lockband_cmd, 3000, at_rep, sizeof ( at_rep ) );

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( lock5gband_cmd, 3000, at_rep, sizeof ( at_rep ) );

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( mode_pref_cmd, 3000, at_rep, sizeof ( at_rep ) );

    nv_set ( "lockearfcn_status", "notlocked" );
    nv_set ( "lockearfcn_list", "" );

    nv_set ( "lockpci_status", "notlocked" );
    nv_set ( "lockpci_list", "" );

    if ( 1 == lock_type )
    {
        tdtekParam_radioOnOffSet ( "off" );
        comd_wait_ms ( 1000 );
        tdtekParam_radioOnOffSet ( "on" );
    }

    nv_set ( "lockband_set", "0" );
}

void tdtekParam_lockearfcn ( char* reqData, int lock_type )
{
    char ramRadioPrefer[8] = {0};
    char *lock_content = NULL;
    char at_rep[64] = {0};
    int ret = 0;
    int tmp_band = 0;
    int tmp_arfcn = 0;
    int tmp_scs = 0;
    int lock4g_num = 0;
    char band4g_list[64] = {0};
    char arfcn4g_list[128] = {0};
    int lock5g_num = 0;
    char band5g_list[64] = {0};
    char arfcn5g_list[128] = {0};
    char scs_list[32] = {0};
    char mode_pref_cmd[64] = {0};
    char lock4g_cmd[256] = {0};
    char lock5g_cmd[256] = {0};

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
                snprintf ( band4g_list + strlen ( band4g_list ),
                           sizeof ( band4g_list ) - strlen ( band4g_list ),
                           ( lock4g_num > 1 ) ? ",%d" : "%d", tmp_band );
                snprintf ( arfcn4g_list + strlen ( arfcn4g_list ),
                           sizeof ( arfcn4g_list ) - strlen ( arfcn4g_list ),
                           ( lock4g_num > 1 ) ? ",%d" : "%d", tmp_arfcn );
            }
        }
        else if ( strstr ( lock_content, "5G," ) )
        {
            ret = sscanf ( lock_content, "5G,%d,%d,%d", &tmp_band, &tmp_arfcn, &tmp_scs );
            if ( 3 == ret )
            {
                lock5g_num++;
                snprintf ( band5g_list + strlen ( band5g_list ),
                           sizeof ( band5g_list ) - strlen ( band5g_list ),
                           ( lock5g_num > 1 ) ? ",%d" : "%d", tmp_band );
                snprintf ( arfcn5g_list + strlen ( arfcn5g_list ),
                           sizeof ( arfcn5g_list ) - strlen ( arfcn5g_list ),
                           ( lock5g_num > 1 ) ? ",%d" : "%d", tmp_arfcn );
                snprintf ( scs_list + strlen ( scs_list ),
                           sizeof ( scs_list ) - strlen ( scs_list ),
                           ( lock5g_num > 1 ) ? ",%d" : "%d", scs2index ( tmp_scs ) );
            }
        }

        lock_content = strtok ( NULL, ";" );
    }

    if ( 0 < lock4g_num && 0 < lock5g_num )
    {
        create_syscfgex ( mode_pref_cmd, strcmp ( ramRadioPrefer, "4G" ) ? "0803" : "0308", 99, 99 );
        snprintf ( lock4g_cmd, sizeof ( lock4g_cmd ), "AT^LTEFREQLOCK=1,0,%d,\"%s\",\"%s\"",
                   lock4g_num, band4g_list, arfcn4g_list );
        snprintf ( lock5g_cmd, sizeof ( lock5g_cmd ), "AT^NRFREQLOCK=1,0,%d,\"%s\",\"%s\",\"%s\"",
                   lock5g_num, band5g_list, arfcn5g_list, scs_list );
    }
    else if ( 0 < lock4g_num )
    {
        create_syscfgex ( mode_pref_cmd, "03", 99, 99 );
        snprintf ( lock4g_cmd, sizeof ( lock4g_cmd ), "AT^LTEFREQLOCK=1,0,%d,\"%s\",\"%s\"",
                   lock4g_num, band4g_list, arfcn4g_list );
        snprintf ( lock5g_cmd, sizeof ( lock5g_cmd ), "%s", "AT^NRFREQLOCK=0" );
    }
    else if ( 0 < lock5g_num )
    {
        create_syscfgex ( mode_pref_cmd, "08", 99, 99 );
        snprintf ( lock4g_cmd, sizeof ( lock4g_cmd ), "%s", "AT^LTEFREQLOCK=0" );
        snprintf ( lock5g_cmd, sizeof ( lock5g_cmd ), "AT^NRFREQLOCK=1,0,%d,\"%s\",\"%s\",\"%s\"",
                   lock5g_num, band5g_list, arfcn5g_list, scs_list );
    }

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( lock4g_cmd, 3000, at_rep, sizeof ( at_rep ) );

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( lock5g_cmd, 3000, at_rep, sizeof ( at_rep ) );

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( mode_pref_cmd, 3000, at_rep, sizeof ( at_rep ) );

    if ( 1 == lock_type )
    {
        tdtekParam_radioOnOffSet ( "off" );
        comd_wait_ms ( 1000 );
        tdtekParam_radioOnOffSet ( "on" );
    }

    nv_set ( "lockband_status", "notlocked" );
    nv_set ( "lockband_list", "" );
    nv_set ( "lockband5g_list", "" );

    nv_set ( "lockearfcn_status", "locked" );

    nv_set ( "lockpci_status", "notlocked" );
    nv_set ( "lockpci_list", "" );

    nv_set ( "lockearfcn_set", "0" );
}

void tdtekParam_lockpci ( char* reqData, int lock_type )
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
    char band4g_list[64] = {0};
    char arfcn4g_list[128] = {0};
    char pci4g_list[64] = {0};
    int lock5g_num = 0;
    char band5g_list[64] = {0};
    char arfcn5g_list[128] = {0};
    char pci5g_list[64] = {0};
    char scs_list[32] = {0};
    char mode_pref_cmd[64] = {0};
    char lock4g_cmd[256] = {0};
    char lock5g_cmd[256] = {0};

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
                snprintf ( band4g_list + strlen ( band4g_list ),
                           sizeof ( band4g_list ) - strlen ( band4g_list ),
                           ( lock4g_num > 1 ) ? ",%d" : "%d", tmp_band );
                snprintf ( arfcn4g_list + strlen ( arfcn4g_list ),
                           sizeof ( arfcn4g_list ) - strlen ( arfcn4g_list ),
                           ( lock4g_num > 1 ) ? ",%d" : "%d", tmp_arfcn );
                snprintf ( pci4g_list + strlen ( pci4g_list ),
                           sizeof ( pci4g_list ) - strlen ( pci4g_list ),
                           ( lock4g_num > 1 ) ? ",%d" : "%d", tmp_pci );
            }
        }
        else if ( strstr ( lock_content, "5G," ) )
        {
            ret = sscanf ( lock_content, "5G,%d,%d,%d,%d", &tmp_band, &tmp_arfcn, &tmp_pci, &tmp_scs );
            if ( 4 == ret )
            {
                lock5g_num++;
                snprintf ( band5g_list + strlen ( band5g_list ),
                           sizeof ( band5g_list ) - strlen ( band5g_list ),
                           ( lock5g_num > 1 ) ? ",%d" : "%d", tmp_band );
                snprintf ( arfcn5g_list + strlen ( arfcn5g_list ),
                           sizeof ( arfcn5g_list ) - strlen ( arfcn5g_list ),
                           ( lock5g_num > 1 ) ? ",%d" : "%d", tmp_arfcn );
                snprintf ( pci5g_list + strlen ( pci5g_list ),
                           sizeof ( pci5g_list ) - strlen ( pci5g_list ),
                           ( lock5g_num > 1 ) ? ",%d" : "%d", tmp_pci );
                snprintf ( scs_list + strlen ( scs_list ),
                           sizeof ( scs_list ) - strlen ( scs_list ),
                           ( lock5g_num > 1 ) ? ",%d" : "%d", scs2index ( tmp_scs ) );
            }
        }

        lock_content = strtok ( NULL, ";" );
    }

    if ( 0 < lock4g_num && 0 < lock5g_num )
    {
        create_syscfgex ( mode_pref_cmd, strcmp ( ramRadioPrefer, "4G" ) ? "0803" : "0308", 99, 99 );
        snprintf ( lock4g_cmd, sizeof ( lock4g_cmd ), "AT^LTEFREQLOCK=2,0,%d,\"%s\",\"%s\",\"%s\"",
                   lock4g_num, band4g_list, arfcn4g_list, pci4g_list );
        snprintf ( lock5g_cmd, sizeof ( lock5g_cmd ), "AT^NRFREQLOCK=2,0,%d,\"%s\",\"%s\",\"%s\",\"%s\"",
                   lock5g_num, band5g_list, arfcn5g_list, scs_list, pci5g_list );
    }
    else if ( 0 < lock4g_num )
    {
        create_syscfgex ( mode_pref_cmd, "03", 99, 99 );
        snprintf ( lock4g_cmd, sizeof ( lock4g_cmd ), "AT^LTEFREQLOCK=2,0,%d,\"%s\",\"%s\",\"%s\"",
                   lock4g_num, band4g_list, arfcn4g_list, pci4g_list );
        snprintf ( lock5g_cmd, sizeof ( lock5g_cmd ), "%s", "AT^NRFREQLOCK=0" );
    }
    else if ( 0 < lock5g_num )
    {
        create_syscfgex ( mode_pref_cmd, "08", 99, 99 );
        snprintf ( lock4g_cmd, sizeof ( lock4g_cmd ), "%s", "AT^LTEFREQLOCK=0" );
        snprintf ( lock5g_cmd, sizeof ( lock5g_cmd ), "AT^NRFREQLOCK=2,0,%d,\"%s\",\"%s\",\"%s\",\"%s\"",
                   lock5g_num, band5g_list, arfcn5g_list, scs_list, pci5g_list );
    }

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( lock4g_cmd, 3000, at_rep, sizeof ( at_rep ) );

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( lock5g_cmd, 3000, at_rep, sizeof ( at_rep ) );

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( mode_pref_cmd, 3000, at_rep, sizeof ( at_rep ) );

    if ( 1 == lock_type )
    {
        tdtekParam_radioOnOffSet ( "off" );
        comd_wait_ms ( 1000 );
        tdtekParam_radioOnOffSet ( "on" );
    }

    nv_set ( "lockband_status", "notlocked" );
    nv_set ( "lockband_list", "" );
    nv_set ( "lockband5g_list", "" );

    nv_set ( "lockearfcn_status", "notlocked" );
    nv_set ( "lockearfcn_list", "" );

    nv_set ( "lockpci_status", "locked" );

    nv_set ( "lockpci_set", "0" );
}

void tdtekParam_setDefaultGateway ( char* reqData )
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

void tdtekParam_getSimlock ( char* reqData )
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

void tdtekParam_radioOnOffSet ( char* reqData )
{
    char at_rep[512] = {0};

    if ( NULL == reqData )
    {
        CLOGD ( ERROR, "reqData is NULL !!!\n" );
    }
    else if ( 0 == strcmp ( reqData, "off" ) )
    {
        COMD_AT_PROCESS ( "AT+CFUN=4", 5000, at_rep, sizeof ( at_rep ) );
    }
    else if ( 0 == strcmp ( reqData, "radio_off" ) )
    {
        COMD_AT_PROCESS ( "AT+CFUN=0", 15000, at_rep, sizeof ( at_rep ) );
    }
    else
    {
        COMD_AT_PROCESS ( "AT+CFUN=1", 3000, at_rep, sizeof ( at_rep ) );
    }

    if ( 0 == parsing_tdtek_cfun_set ( at_rep ) )
        nv_set ( "lte_on_off", reqData );
}

void tdtekParam_netSelectModeSet ( char* reqData )
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

    nv_set ( "net_select_mode", reqData );
    if ( strcmp ( reqData, "manual_select" ) )
    {
        tdtekParam_connectNetwork ( NULL );
    }
#if 0
    else
    {
        tdtekParam_disconnectNetwork ( NULL );
    }
#endif
    nv_set ( "net_select_mode_set", "0" );
}

void tdtekParam_setSearchNeighbor ( char* reqData )
{
    tdtekParam_updateServNeighInfo ( NULL );

    return;
}

void tdtekParam_dualSimSet ( char* reqData )
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

void tdtekParam_getNitzTime ( char* reqData )
{
    char at_rep[256] = {0};

#if 0
    COMD_AT_PROCESS ( "AT+CTZU=1", 1500, at_rep, sizeof ( at_rep ) );

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( "AT+CCLK?", 1500, at_rep, sizeof ( at_rep ) );
    parsing_tdtek_cclk ( at_rep );
#else
    COMD_AT_PROCESS ( "AT^NWTIME?", 1500, at_rep, sizeof ( at_rep ) );
    parsing_tdtek_nwtime ( at_rep );
#endif
}

void tdtekParam_sendUssdCode ( char* reqData )
{
    return;
}

// AT^SYSCFGEX
int tdtekParam_lteRoamingSet ( char* reqData )
{
#if 0
    char roaming_set[8] = {0};
    char at_req[128] = {0};
    char at_rep[64] = {0};

    if ( NULL != reqData )
    {
        strncpy ( roaming_set, reqData, sizeof ( roaming_set ) - 1 );
    }
    else
    {
        sys_get_config ( LTE_PARAMETER_ROAMING, roaming_set, sizeof ( roaming_set ) );
    }

    create_syscfgex ( at_req, "99", strcmp ( roaming_set, "0" ) ? 1 : 3, 99 );

    COMD_AT_PROCESS ( at_req, 3000, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
    {
        nv_set ( "roaming_set", "1" );
        return -1;
    }

    if ( NULL != reqData )
    {
        tdtekParam_radioOnOffSet ( "off" );
        comd_wait_ms ( 1000 );
        tdtekParam_radioOnOffSet ( "on" );
    }
#endif

    nv_set ( "roaming_set", "0" );

    return 0;
}

void tdtekParam_setVolteAkaRequest ( struct KT_char_akaRequest *volte_aka_req )
{
    return;
}

void tdtekParam_sendSmsRequest ( KT_sms_msg *send_sms )
{
    return;
}

void tdtekParam_delSmsRequest ( KT_sms_msg *del_sms )
{
    return;
}

void tdtekParam_readSmsRequest ( KT_sms_msg *read_sms )
{
    return;
}

void tdtekParam_setSmsCenterNum ( char* reqData )
{
    return;
}

void tdtekParam_resendSmsRequest ( KT_sms_msg *resend_sms )
{
    return;
}

void tdtekParam_manualPlmnSearchReq ( char* reqData )
{
    return;
}

void tdtekParam_plmnSearchManualSelect ( char* reqData )
{
    return;
}

void tdtekParam_lteTxPowerLimitSet ( char* reqData )
{
    return;
}

void tdtekParam_lteEventNotify ( char* reqData )
{
    return;
}

