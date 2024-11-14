#include "comd_share.h"
#include "comd_sms.h"
#include "atchannel.h"
#include "mtk_config_response.h"
#include "mtk_status_refresh.h"
#include "mtk_atcmd_parsing.h"
#include "config_key.h"
#include "hal_platform.h"

extern int manual_conn;
extern int apns_msg_flag[5];
extern char celllock_enable[4];
extern char pinlock_enable[4];
extern char simlock_enable[4];

#if defined(MTK_T750) && CONFIG_QUECTEL_T750
#include "ql_nw.h"
#include "ql_sms.h"
#endif

void mtkParam_lockPin ( char* reqData )
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

    mtkParam_updatePinStatus ();
    mtkParam_updatePinLockStatus ();
    mtkParam_updatePinPukCount ();

END:
    parsing_mtk_lockpin ( at_rep );
}

void mtkParam_enterPin ( char* reqData )
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

    comd_wait_ms ( 2000 );

    mtkParam_updatePinStatus ();
    mtkParam_updatePinPukCount ();

END:
    parsing_mtk_enterpin ( at_rep );
}

void mtkParam_modifyPin ( char* reqData )
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

    mtkParam_updatePinStatus ();
    mtkParam_updatePinPukCount ();

END:
    parsing_mtk_modifypin ( at_rep );
}

void mtkParam_config_commandsetting ( char* reqData )
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

void mtkParam_enterPuk ( char* reqData )
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

    mtkParam_updatePinStatus ();
    mtkParam_updatePinPukCount ();

END:
    parsing_mtk_enterpuk ( at_rep );
}

void mtkParam_connectNetwork ( char* reqData )
{
    char at_rep[32] = {0};

    if ( 0 == manual_conn )
    {
        manual_conn = 1;
        COMD_AT_PROCESS ( "AT+CGATT=1", 10000, at_rep, sizeof ( at_rep ) );
    }

    nv_set ( "connect_network_set", "0" );
}

void mtkParam_disconnectNetwork ( char* reqData )
{
    char at_rep[32] = {0};

    manual_conn = 0;

    COMD_AT_PROCESS ( "AT+CGATT=0", 10000, at_rep, sizeof ( at_rep ) );

#if defined(MTK_T750) && CONFIG_QUECTEL_T750
    mtkParam_deInitDataCall ( 1 );
#endif

    nv_set ( "disconnect_network_set", strstr ( at_rep, "\r\nOK\r\n" ) ? "0" : "1" );
}

void mtkParam_apnSetting ( apn_profile* const apnSetting_data )
{
    if ( NULL == apnSetting_data )
    {
        CLOGD ( ERROR, "apnSetting_data is NULL !!!\n" );
        nv_set ( "apn_set", "1" );
        return;
    }

    mtkParam_initAPNsetting ( ( char * ) apnSetting_data );

    return;
}

int mtkParam_setRatPriority ( char* reqData )
{
#if defined(MTK_T750)
    return 0;
#endif

    return 0;
}

#define Q_EVERY_PART    32
#define Q_PART_NUM      4

static void StrBandsToHex ( char* lockbands, char* delim, char* lockbands_hex, int hex_len )
{
    char *band_str = NULL;
    int band_dec = 0;
    int j = 0;
    unsigned long long int llu_one = 1;
    unsigned long long int bands_dec[Q_PART_NUM] = {0};

    band_str = strtok ( lockbands, delim );
    while ( band_str )
    {
        band_dec = atoi ( band_str );
        if ( 0 < band_dec )
        {
            j = ( band_dec - 1 ) / Q_EVERY_PART;
            if ( 0 <= j && j < Q_PART_NUM )
            {
                bands_dec[j] += ( llu_one << ( ( band_dec - 1 ) % Q_EVERY_PART ) );
            }
        }
        band_str = strtok ( NULL, delim );
    }

    for ( j = 0; j < Q_PART_NUM; j++ )
    {
        CLOGD ( FINE, "bands_%d_dec -> [%llu]\n", ( j + 1 ) * Q_EVERY_PART, bands_dec[j] );
        CLOGD ( FINE, "bands_%d_hex -> [%0*llx]\n", ( j + 1 ) * Q_EVERY_PART, Q_EVERY_PART / 4, bands_dec[j] );

        snprintf ( lockbands_hex + strlen ( lockbands_hex ),
                   hex_len - strlen ( lockbands_hex ), "%0*llx", Q_EVERY_PART / 4, bands_dec[j] );
    }
}

int mtkParam_getSupportBand ()
{
    char at_rep[256] = {0};

    COMD_AT_PROCESS ( "AT+KSBAND?", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "\r\nOK\r\n" ) )
        parsing_mtk_supportband ( at_rep );

    return 0;
}

void mtkParam_lockband ( char* reqData, int lock_type )
{
    char band_value[256] = {0};
    char lockearfcn_status[16] = {0};
    char lockpci_status[16] = {0};
    char lockband_cmd[256] = {0};
    char at_rep[64] = {0};
    char ramRadioPrefer[8] = {0};
    char mode_pref_cmd[64] = {0};
    char lockbands_4g[128] = {0};
    char lockbands_4g_hex[Q_EVERY_PART / 4 * Q_PART_NUM + 1] = {0};
    char lockbands_5g[128] = {0};
    char lockbands_5g_hex[Q_EVERY_PART / 4 * Q_PART_NUM  + 1] = {0};

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
            snprintf ( mode_pref_cmd, sizeof ( mode_pref_cmd ), "%s", "AT+ERAT=3,4,1" );
        }
        else if ( ',' == band_value[0] ) // 5G only
        {
            sscanf ( band_value, ",%[0-9_]", lockbands_5g );
            snprintf ( mode_pref_cmd, sizeof ( mode_pref_cmd ), "%s", "AT+ERAT=15,128,1" );
        }
        else // 4G & 5G
        {
            sscanf ( band_value, "%[0-9_],%[0-9_]", lockbands_4g, lockbands_5g );
            snprintf ( mode_pref_cmd, sizeof ( mode_pref_cmd ), "AT+ERAT=19,%d,1", strcmp ( ramRadioPrefer, "4G" ) ? 128 : 4 );
        }

        CLOGD ( FINE, "lockbands_4g -> [%s]\n", lockbands_4g );
        nv_set ( "lockband_list", lockbands_4g );
        StrBandsToHex ( lockbands_4g, "_", lockbands_4g_hex, sizeof ( lockbands_4g_hex ) );
        CLOGD ( FINE, "lockbands_4g_hex -> [%s]\n", lockbands_4g_hex );

        CLOGD ( FINE, "lockbands_5g -> [%s]\n", lockbands_5g );
        nv_set ( "lockband5g_list", lockbands_5g );
        StrBandsToHex ( lockbands_5g, "_", lockbands_5g_hex, sizeof ( lockbands_5g_hex ) );
        CLOGD ( FINE, "lockbands_5g_hex -> [%s]\n", lockbands_5g_hex );

        nv_set ( "lockband_status", "locked" );
    }
    else
    {
        snprintf ( mode_pref_cmd, sizeof ( mode_pref_cmd ), "AT+ERAT=19,%d,1", strcmp ( ramRadioPrefer, "4G" ) ? 128 : 4 );

        nv_get ( "suppband_org", lockbands_4g, sizeof ( lockbands_4g ) );
        StrBandsToHex ( lockbands_4g, ":", lockbands_4g_hex, sizeof ( lockbands_4g_hex ) );
        CLOGD ( FINE, "lockbands_4g_hex -> [%s]\n", lockbands_4g_hex );

        nv_get ( "suppband5g_org", lockbands_5g, sizeof ( lockbands_5g ) );
        StrBandsToHex ( lockbands_5g, ":", lockbands_5g_hex, sizeof ( lockbands_5g_hex ) );
        CLOGD ( FINE, "lockbands_5g_hex -> [%s]\n", lockbands_5g_hex );

        nv_set ( "lockband_status", "notlocked" );
        nv_set ( "lockband_list", "" );
        nv_set ( "lockband5g_list", "" );
    }

    if ( 1 == lock_type )
    {
        mtkParam_radioOnOffSet ( "off" );
    }

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( mode_pref_cmd, 3000, at_rep, sizeof ( at_rep ) );

    snprintf ( lockband_cmd, sizeof ( lockband_cmd ), "AT+EPBSEH=,,\"%s\",\"%s\"", lockbands_4g_hex, lockbands_5g_hex );
    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( lockband_cmd, 3000, at_rep, sizeof ( at_rep ) );

    nv_get ( "lockearfcn_status", lockearfcn_status, sizeof ( lockearfcn_status ) );
    nv_get ( "lockpci_status", lockpci_status, sizeof ( lockpci_status ) );

    if ( 0 == strcmp ( lockearfcn_status, "locked" ) || 0 == strcmp ( lockpci_status, "locked" ) )
    {
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( "AT+KLOCKEARFCN=0", 3000, at_rep, sizeof ( at_rep ) );

        if ( strstr ( at_rep, "\r\nOK\r\n" ) )
        {
            nv_set ( "lockearfcn_status", "notlocked" );
            nv_set ( "lockearfcn_list", "" );
            CLOGD ( FINE, "unlock earfcn is success!\n" );

            nv_set ( "lockpci_status", "notlocked" );
            nv_set ( "lockpci_list", "" );
            CLOGD ( FINE, "unlock pci is success!\n" );
        }
        else
        {
            CLOGD ( FINE, "unlock earfcn is error!\n" );
        }
    }

    if ( 1 == lock_type )
    {
        comd_wait_ms ( 1000 );
        mtkParam_radioOnOffSet ( "on" );
    }

    nv_set ( "lockband_set", "0" );
}

static int mtkParam_earfcn_to_band(int lte_list_index, int nr_list_index, char* lockbands_band_5g_list,char* lockbands_band_4g_list)
{
    char mode_pref_cmd[256] = {0};
    char at_rep[32] = {0};

    char lockbands_4g[128] = {0};
    char lockbands_4g_hex[Q_EVERY_PART / 4 * Q_PART_NUM + 1] = {0};
    char lockbands_5g_hex[Q_EVERY_PART / 4 * Q_PART_NUM  + 1] = {0};

    if(lte_list_index > 0 && nr_list_index == 0)
    {
        memset ( mode_pref_cmd, 0, sizeof ( mode_pref_cmd ) );
        snprintf ( mode_pref_cmd, sizeof ( mode_pref_cmd ), "%s", "AT+ERAT=3,4,1" );
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( mode_pref_cmd, 3000, at_rep, sizeof ( at_rep ) );
        if(lockbands_band_4g_list != NULL && strlen(lockbands_band_4g_list) > 0 && strcmp(lockbands_band_4g_list,"NONE") != 0)
        {
            memset ( mode_pref_cmd, 0, sizeof ( mode_pref_cmd ) );
            memset ( at_rep, 0, sizeof ( at_rep ) );
            StrBandsToHex ( lockbands_band_4g_list, "_", lockbands_4g_hex, sizeof ( lockbands_4g_hex ) );
            CLOGD ( FINE, "lockbands_4g_hex -> [%s]\n", lockbands_4g_hex );
            snprintf ( mode_pref_cmd, sizeof ( mode_pref_cmd ), "AT+EPBSEH=,,\"%s\",\"%s\"", lockbands_4g_hex, lockbands_5g_hex );
            COMD_AT_PROCESS ( mode_pref_cmd, 3000, at_rep, sizeof ( at_rep ) );
        }
    }
    else if (lte_list_index == 0 && nr_list_index > 0)
    {
        memset ( mode_pref_cmd, 0, sizeof ( mode_pref_cmd ) );
        memset ( at_rep, 0, sizeof ( at_rep ) );
        snprintf ( mode_pref_cmd, sizeof ( mode_pref_cmd ), "%s", "AT+ERAT=15,128,1" );
        COMD_AT_PROCESS ( mode_pref_cmd, 3000, at_rep, sizeof ( at_rep ) );

        //The input content <lockbands_band_5g_list> is "NONE" or strlen == 0
        //Means we are processing lockearfcn, therefore no need lock 5g band
        if(lockbands_band_5g_list != NULL && strlen(lockbands_band_5g_list) > 0 && strcmp(lockbands_band_5g_list,"NONE") != 0)
        {
            memset ( mode_pref_cmd, 0, sizeof ( mode_pref_cmd ) );
            memset ( at_rep, 0, sizeof ( at_rep ) );
            StrBandsToHex ( lockbands_band_5g_list, "_", lockbands_5g_hex, sizeof ( lockbands_5g_hex ) );
            CLOGD ( FINE, "lockbands_5g_hex -> [%s]\n", lockbands_5g_hex );
            snprintf ( mode_pref_cmd, sizeof ( mode_pref_cmd ), "AT+EPBSEH=,,\"%s\",\"%s\"", lockbands_4g_hex, lockbands_5g_hex );
            COMD_AT_PROCESS ( mode_pref_cmd, 3000, at_rep, sizeof ( at_rep ) );
        }
    }
    else
    {
        memset ( mode_pref_cmd, 0, sizeof ( mode_pref_cmd ) );
        memset ( at_rep, 0, sizeof ( at_rep ) );
        snprintf ( mode_pref_cmd, sizeof ( mode_pref_cmd ), "%s", "AT+ERAT=19,128,1" );
        COMD_AT_PROCESS ( mode_pref_cmd, 3000, at_rep, sizeof ( at_rep ) );

        if(lockbands_band_5g_list != NULL && strlen(lockbands_band_5g_list) > 0 && strcmp(lockbands_band_5g_list,"NONE") != 0)
        {
            nv_get ( "suppband_org", lockbands_4g, sizeof ( lockbands_4g ) );
            StrBandsToHex ( lockbands_4g, ":", lockbands_4g_hex, sizeof ( lockbands_4g_hex ) );
            CLOGD ( FINE, "lockbands_4g_hex -> [%s]\n", lockbands_4g_hex );
            memset ( mode_pref_cmd, 0, sizeof ( mode_pref_cmd ) );
            memset ( at_rep, 0, sizeof ( at_rep ) );
            StrBandsToHex ( lockbands_band_5g_list, "_", lockbands_5g_hex, sizeof ( lockbands_5g_hex ) );
            CLOGD ( FINE, "lockbands_5g_hex -> [%s]\n", lockbands_5g_hex );
            snprintf ( mode_pref_cmd, sizeof ( mode_pref_cmd ), "AT+EPBSEH=,,\"%s\",\"%s\"", lockbands_4g_hex, lockbands_5g_hex );
            COMD_AT_PROCESS ( mode_pref_cmd, 3000, at_rep, sizeof ( at_rep ) );
        }
    }

    CLOGD ( FINE, "[%s] rep ->[%s]\n",mode_pref_cmd,at_rep);

    return 0;
}

void mtkParam_lockearfcn ( char* reqData, int lock_type )
{
    char earfcns_list[256] = {0};
    char earfcn[16] = {0};
    char earfcn_5g[16] = {0};
    int lte_list_index = 0;
    int nr_list_index = 0;
    int tmp_scs = 0;
    int ret = 0;
    char at_rep[128] = {0};
    char lockbands_band_4g[8] = {0};
    char lockbands_band_4g_list[128] = {0};
    char lockbands_band_5g[8] = {0};
    char lockbands_band_5g_list[128] = {0};
    char new_earfcn_tmp[256] = {0};
    char scs_tmp[8] = {0};
    char lockearfcn_cmd[256] = {0};

    if ( NULL == reqData )
    {
        CLOGD ( ERROR, "reqData is NULL !!!\n" );
        nv_set ( "lockearfcn_set", "1" );
        return;
    }

    snprintf ( earfcns_list, sizeof ( earfcns_list ), "%s", reqData );
    CLOGD ( FINE, "earfcns_list -> [%s]\n", earfcns_list );

    char *earfcn_tmp = strtok ( earfcns_list, ";" );
    while ( earfcn_tmp )
    {
        if ( 0 == strncmp ( earfcn_tmp, "4G", 2 ) )
        {
            memset(lockbands_band_4g,0,sizeof(lockbands_band_4g));
            ret = sscanf ( earfcn_tmp, "4G,%[0-9],%[0-9]", lockbands_band_4g, earfcn );

            strcat(new_earfcn_tmp,earfcn_tmp);
            strcat(new_earfcn_tmp,",65535_");//Use 65535(0XFFFF) when this pci is not locked

            if ( 2 == ret && NULL == strstr ( lockbands_band_4g_list, lockbands_band_4g ) )
            {
                lte_list_index ++;
                strcat(lockbands_band_4g,"_");
                strcat(lockbands_band_4g_list,lockbands_band_4g);
            }
        }
        else if ( 0 == strncmp ( earfcn_tmp, "5G", 2 ) )
        {
            memset(lockbands_band_5g,0,sizeof(lockbands_band_5g));
            ret = sscanf ( earfcn_tmp, "5G,%[0-9],%[0-9],%d", lockbands_band_5g, earfcn_5g, &tmp_scs );

            strcat(new_earfcn_tmp,"5G,");
            strcat(new_earfcn_tmp,lockbands_band_5g);
            strcat(new_earfcn_tmp,",");
            strcat(new_earfcn_tmp,earfcn_5g);
            strcat(new_earfcn_tmp,",65535,");//Use 65535(0XFFFF) when this pci is not locked
            sprintf( scs_tmp, "%d", tmp_scs );
            strcat(new_earfcn_tmp,scs_tmp);
            strcat(new_earfcn_tmp,"_");

            if ( 3 == ret && NULL == strstr ( lockbands_band_5g_list, lockbands_band_5g ) )
            {
                nr_list_index ++;
                strcat(lockbands_band_5g,"_");
                strcat(lockbands_band_5g_list,lockbands_band_5g);
            }
        }
        earfcn_tmp = strtok ( NULL, ";" );
    }

    CLOGD ( FINE, "lte_list_index -> [%d]\n", lte_list_index );
    CLOGD ( FINE, "nr_list_index -> [%d]\n", nr_list_index );

    CLOGD ( FINE, "lockbands_band_4g_list -> [%s]\n", lockbands_band_4g_list );
    CLOGD ( FINE, "lockbands_band_5g_list -> [%s]\n", lockbands_band_5g_list );

    mtkParam_earfcn_to_band(lte_list_index,nr_list_index,lockbands_band_5g_list,lockbands_band_4g_list);

    sprintf ( lockearfcn_cmd, "AT+KLOCKEARFCN=%s", new_earfcn_tmp );
    CLOGD ( FINE, "lockearfcn_cmd -> [%s]\n", lockearfcn_cmd );

    COMD_AT_PROCESS ( lockearfcn_cmd, 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "\r\nOK\r\n" ) )
    {
        if ( 1 == lock_type )
        {
            mtkParam_radioOnOffSet ( "radio_off" );
            comd_wait_ms ( 1000 );
            mtkParam_radioOnOffSet ( "on" );
        }

        nv_set ( "lockearfcn_status", "locked" );
        //nv_set ( "lockearfcn_list", reqData );
        nv_set ( "lockband_status", "notlocked" );
        nv_set ( "lockband_list", "" );
        nv_set ( "lockband5g_list", "" );
        nv_set ( "lockpci_status", "notlocked" );
        nv_set ( "lockpci_list", "" );

        nv_set ( "lockearfcn_set", "0" );
        CLOGD ( FINE, "lock arfcn is success!\n" );
    }
    else
    {
        CLOGD ( FINE, "ret -> [%d]\n", ret );
        nv_set ( "lockearfcn_set", "1" );
        CLOGD ( FINE, "lock arfcn is error!\n" );
    }

    return;
}

void mtkParam_lockpci ( char* reqData, int lock_type)
{
    char earfcns_list[256] = {0};
    char earfcn[16] = {0};
    char earfcn_5g[16] = {0};
    char pci_4g[8] = {0};
    char pci_5g[8] = {0};
    int lte_list_index = 0;
    int nr_list_index = 0;
    int ret = 0;
    int tmp_scs = 0;
    char lockbands_band_4g[8] = {0};
    char lockbands_band_4g_list[128] = {0};
    char lockbands_band_5g[8] = {0};
    char lockbands_band_5g_list[128] = {0};
    char lockearfcn_cmd[256] = {0};
    char at_rep[64] = {0};
    char new_earfcn_tmp[256] = {0};

    if ( NULL == reqData )
    {
        CLOGD ( ERROR, "reqData is NULL !!!\n" );
        nv_set ( "lockpci_set", "1" );
        return;
    }

    snprintf ( earfcns_list, sizeof ( earfcns_list ), "%s", reqData );
    CLOGD ( FINE, "pci_list -> [%s]\n", earfcns_list );

    char *earfcn_tmp = strtok ( earfcns_list, ";" );
    while ( earfcn_tmp )
    {
        if ( 0 == strncmp ( earfcn_tmp, "4G", 2 ) )
        {
            memset(lockbands_band_4g,0,sizeof(lockbands_band_4g));
            ret = sscanf ( earfcn_tmp, "4G,%[0-9],%[0-9],%[0-9]", lockbands_band_4g, earfcn, pci_4g );

            strcat(new_earfcn_tmp,earfcn_tmp);
            strcat(new_earfcn_tmp,"_");

            if ( 3 == ret && NULL == strstr ( lockbands_band_4g_list, lockbands_band_4g ) )
            {
                CLOGD ( FINE, "earfcn -> [%s][%s]\n", earfcn,pci_4g );
                lte_list_index ++;
                strcat(lockbands_band_4g,"_");
                strcat(lockbands_band_4g_list,lockbands_band_4g);
            }
        }
        else if ( 0 == strncmp ( earfcn_tmp, "5G", 2 ) )
        {
            memset(lockbands_band_5g,0,sizeof(lockbands_band_5g));

            ret = sscanf ( earfcn_tmp, "5G,%[0-9],%[0-9],%[0-9],%d", lockbands_band_5g, earfcn_5g, pci_5g, &tmp_scs );

            strcat(new_earfcn_tmp,earfcn_tmp);
            strcat(new_earfcn_tmp,"_");

            if ( 4 == ret && NULL == strstr ( lockbands_band_5g_list, lockbands_band_5g ) )
            {
                CLOGD ( FINE, "earfcn -> [%s][%s][%s]\n", lockbands_band_5g, earfcn_5g,pci_5g );
                ret = verify_5g_narfcn_by_band(lockbands_band_5g,earfcn_5g);
                if(ret == 0)
                {
                    CLOGD ( ERROR, "Verify earfcn[%s] to band[%s] failed!\n",earfcn_5g, lockbands_band_5g );
                    nv_set ( "lockpci_set", "1" );
                    return;
                }
                strcat(lockbands_band_5g,"_");
                strcat(lockbands_band_5g_list,lockbands_band_5g);
                nr_list_index ++;
            }
        }
        earfcn_tmp = strtok ( NULL, ";" );
    }

    CLOGD ( FINE, "lte_list_index -> [%d]\n", lte_list_index );
    CLOGD ( FINE, "nr_list_index -> [%d]\n", nr_list_index );

    CLOGD ( FINE, "lockbands_band_4g_list -> [%s]\n", lockbands_band_4g_list );
    CLOGD ( FINE, "lockbands_band_5g_list -> [%s]\n", lockbands_band_5g_list );

    mtkParam_earfcn_to_band(lte_list_index,nr_list_index, lockbands_band_5g_list,lockbands_band_4g_list);

    sprintf ( lockearfcn_cmd, "AT+KLOCKEARFCN=%s", new_earfcn_tmp );
    CLOGD ( FINE, "lockearfcn_cmd -> [%s]\n", lockearfcn_cmd );

    COMD_AT_PROCESS ( lockearfcn_cmd, 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "\r\nOK\r\n" ) )
    {
        if ( 1 == lock_type )
        {
            mtkParam_radioOnOffSet ( "radio_off" );
            comd_wait_ms ( 1000 );
            mtkParam_radioOnOffSet ( "on" );
        }

        nv_set ( "lockpci_status", "locked" );
        //nv_set ( "lockpci_list", reqData );
        nv_set ( "lockband_status", "notlocked" );
        nv_set ( "lockband_list", "" );
        nv_set ( "lockearfcn_status", "notlocked" );
        nv_set ( "lockearfcn_list", "" );

        nv_set ( "lockpci_set", "0" );
        CLOGD ( FINE, "lock pci is success!\n" );
    }
    else
    {
        nv_set ( "lockpci_set", "1" );
        CLOGD ( FINE, "lock pci is error!\n" );
    }

    return;
}

void mtkParam_setDefaultGateway ( char* reqData )
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
#if defined(MTK_T750) && CONFIG_QUECTEL_T750
            apnActOrDeactMsgEvent ( apn_index, 1 );
#endif
        }
    }

    nv_set ( "default_gateway_set", "0" );
}

void mtkParam_getSimlock ( char* reqData )
{
#if defined (CONFIG_SW_STC_SIMLOCK)
    if ( 0 == strcmp ( simlock_enable, "0" ) )//disable sim lock
    {
        nv_set ( "simlocked", "0" );

        CLOGD ( FINE, "sim lock is disable !!!\n" );
        return;
    }
#endif

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

void mtkParam_radioOnOffSet ( char* reqData )
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
    else if ( 0 == strcmp ( reqData, "radio_off" ) )
    {
        COMD_AT_PROCESS ( "AT+CFUN=0", 15000, at_rep, sizeof ( at_rep ) );
    }
    else
    {
        COMD_AT_PROCESS ( "AT+CFUN=1", 3000, at_rep, sizeof ( at_rep ) );
    }

    if ( 0 == parsing_mtk_cfun_set ( at_rep ) )
        nv_set ( "lte_on_off", reqData );
}

void mtkParam_netSelectModeSet ( char* reqData )
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
        mtkParam_connectNetwork ( NULL );
    }
#if 0
    else
    {
        mtkParam_disconnectNetwork ( NULL );
    }
#endif
    nv_set ( "net_select_mode_set", "0" );
}

void mtkParam_setSearchNeighbor ( char* reqData )
{
    // mtkParam_updateServNeighInfo will refresh

    return;
}

void mtkParam_dualSimSet ( char* reqData )
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

void mtkParam_getNitzTime ( char* reqData )
{
    char at_rep[128] = {0};

    COMD_AT_PROCESS ( "AT+CTZU=1", 1500, at_rep, sizeof ( at_rep ) );

    memset ( at_rep, 0, sizeof ( at_rep ) );

    COMD_AT_PROCESS ( "AT+EMTSI=2", 1500, at_rep, sizeof ( at_rep ) );
    parsing_mtk_emtsi( at_rep );
}

void mtkParam_sendUssdCode ( char* reqData )
{
#if defined(MTK_T750) && CONFIG_QUECTEL_T750
    if ( NULL == reqData )
    {
        CLOGD ( ERROR, "reqData is NULL !!!\n" );
        nv_set ( "ussd_code_set", "1" );
    }
    else
    {
        int ret = 0;
        int ussd_len = 0;
        ql_ussd_send_req_t send_req;

        memset((void *)&send_req,0,sizeof(ql_ussd_send_req_t));

        snprintf( send_req.ussd_str, QL_USSD_MAX_SEND_LENGTH + 1, "%s", reqData );

        ussd_len = strlen(send_req.ussd_str);
        if ('\r' == send_req.ussd_str[ussd_len - 1] || '\n' == send_req.ussd_str[ussd_len - 1])
        {
            send_req.ussd_str[ussd_len - 1] = 0;
            ussd_len--;
        }

        CLOGD( FINE, "ussd_len:%d ussd str:%s\n", ussd_len,send_req.ussd_str);

        ret = ql_ussd_send(&send_req);
        if ( ret == QL_SMS_SUCCESS )
        {
            CLOGD( FINE, "send ussd str success! str:%s\n", send_req.ussd_str);
        }
        else
        {
            CLOGD( ERROR, "send ussd str failed! ret:%d\n", ret);
            nv_set ( "ussd_code_set", "1" );
        }
    }
    return;
#endif
}

int mtkParam_lteRoamingSet ( char* reqData )
{
    nv_set ( "roaming_set", "0" );

    return 0;
}

//PIN LOCK type 00,00000000 : web disable PIN LOCK;
//10,00000000 : SMS disable PIN LOCK;
//11,00000000 : TR069 disable PIN LOCK;
//12,00000000 : TR069 enable PIN LOCK;
//13,new_pin_code : TR069 modify or replace pin code;
void mtkParam_setPinLock ( char* reqData )
{
#if defined (CONFIG_SW_STC_PINLOCK)
    char *pEnable = NULL;
    char *pinCode = NULL;
    char pin_info[32] = {0};
    char pin_cpwd[64] = {0};
    char pinlock_code[16] = {0};
    char strPinCode[64] = {0};
    char strPinLockEnable[64] = {0};
    char at_rep[32] = {0};
    char pin_clck[64] = {0};
    char strPinIccid[64] = {0};
    char lockpin_status[4] = {0};
    char unlock_code[16] = {0};

    if ( NULL == reqData )
    {
        CLOGD ( ERROR, "reqData is NULL !!!\n" );
        nv_set ( "lte_stc_pinlock", "1" );
        return;
    }

    snprintf ( pin_info, sizeof ( pin_info ), "%s", reqData );

    pEnable = strtok ( pin_info, "," );
    if ( NULL == pEnable )
    {
        CLOGD ( ERROR, "pEnable is NULL !!!\n" );
        nv_set ( "lte_stc_pinlock", "1" );
        return;
    }

    comd_exec ( FACTORY_STC_PIN_CODE, pinlock_code, sizeof ( pinlock_code ) );
    CLOGD ( FINE, "pinlock_code [%s]\n", pinlock_code );

    if ( 0 == strcmp ( pEnable, "00" ) )
    {
        pinCode = strtok ( NULL, "," );
        if ( NULL == pinCode )
        {
            CLOGD ( ERROR, "pinCode is NULL !!!\n" );
            nv_set ( "lte_stc_pinlock", "1" );
            return;
        }

        nv_get ( "unlock_code2", unlock_code, sizeof ( unlock_code ) );
        if ( 0 == strcmp ( pinCode, unlock_code ) )
        {
            snprintf ( pinlock_enable, sizeof ( pinlock_enable ), "0" );

            nv_get ( "lockpin_status", lockpin_status, sizeof ( lockpin_status ) );
            if ( 0 == strcmp ( lockpin_status, "1" ) )
            {
                snprintf ( pin_cpwd, sizeof ( pin_cpwd ),
                    "AT+CPWD=\"SC\",\"%s\",\"%s\"", pinlock_code, STC_PINLOCK_CODE );
                COMD_AT_PROCESS ( pin_cpwd, 1500, at_rep, sizeof ( at_rep ) );//Modify the SIM pin code to the default "0000"
                CLOGD ( FINE, "pin_cpwd is [%s]\n", pin_cpwd );

                memset ( at_rep, 0, sizeof ( at_rep ) );
                snprintf ( pin_clck, sizeof ( pin_clck ), "AT+CLCK=\"SC\",0,\"%s\"", STC_PINLOCK_CODE );//disable PIN LOCK
                COMD_AT_PROCESS ( pin_clck, 1500, at_rep, sizeof ( at_rep ) );
            }

            snprintf ( strPinLockEnable, sizeof ( strPinLockEnable ),
                "/lib/factory_tool.sh CONFIG_HW_STC_PINLOCK=0" );
            system ( strPinLockEnable );//modify flash

            snprintf ( strPinCode, sizeof ( strPinCode ),
                "/lib/factory_tool.sh CONFIG_HW_STC_PIN_CODE=" );
            system ( strPinCode );//Clear the random pin code saved to flash

            snprintf ( strPinIccid, sizeof ( strPinIccid ),
                "/lib/factory_tool.sh CONFIG_HW_STC_ICCID=" );
            system ( strPinIccid );//Clear the ICCID saved to flash

            mtkParam_connectNetwork ( NULL );

            nv_set ( "pinlocked", "0" );
            nv_set ( "lte_stc_pinlock", "0" );
            CLOGD ( FINE, "web Unlock PIN LOCK is success !!!\n" );
        }
        else
        {
            CLOGD ( ERROR, "pinCode is ERROR !!!\n" );
            nv_set ( "lte_stc_pinlock", "1" );
            return;
        }
    }
    else if ( 0 == strcmp ( pEnable, "10" ) || 0 == strcmp ( pEnable, "11" ) )
    {
        snprintf ( pin_cpwd, sizeof ( pin_cpwd ),
            "AT+CPWD=\"SC\",\"%s\",\"%s\"", pinlock_code, STC_PINLOCK_CODE );
        COMD_AT_PROCESS ( pin_cpwd, 1500, at_rep, sizeof ( at_rep ) );//Modify the SIM pin code to the default "0000"
        CLOGD ( FINE, "pin_cpwd is [%s]\n", pin_cpwd );

        memset ( at_rep, 0, sizeof ( at_rep ) );
        snprintf ( pin_clck, sizeof ( pin_clck ), "AT+CLCK=\"SC\",0,\"%s\"", STC_PINLOCK_CODE );//disable PIN LOCK
        COMD_AT_PROCESS ( pin_clck, 1500, at_rep, sizeof ( at_rep ) );

        snprintf ( strPinLockEnable, sizeof ( strPinLockEnable ),
            "/lib/factory_tool.sh CONFIG_HW_STC_PINLOCK=0" );
        system ( strPinLockEnable );//modify flash

        snprintf ( pinlock_enable, sizeof ( pinlock_enable ), "0" );

        snprintf ( strPinCode, sizeof ( strPinCode ),
            "/lib/factory_tool.sh CONFIG_HW_STC_PIN_CODE=" );
        system ( strPinCode );//Clear the random pin code saved to flash
        snprintf ( strPinIccid, sizeof ( strPinIccid ),
            "/lib/factory_tool.sh CONFIG_HW_STC_ICCID=" );
        system ( strPinIccid );//Clear the ICCID saved to flash

        nv_set ( "pinlocked", "0" );
        nv_set ( "lte_stc_pinlock", "0" );
        CLOGD ( FINE, "SMS or TR069 Unlock PIN LOCK is success !!!\n" );
    }
    else if ( 0 == strcmp ( pEnable, "12" ) )
    {
        snprintf ( strPinLockEnable, sizeof ( strPinLockEnable ),
            "/lib/factory_tool.sh CONFIG_HW_STC_PINLOCK=1" );
        system ( strPinLockEnable );//modify flash

        snprintf ( pinlock_enable, sizeof ( pinlock_enable ), "1" );

        nv_set ( "lte_stc_pinlock", "0" );
        CLOGD ( FINE, "TR069 enable PIN LOCK is success !!!\n" );
    }
    else if ( 0 == strcmp ( pEnable, "13" ) )
    {
        pinCode = strtok ( NULL, "," );
        if ( NULL == pinCode )
        {
            CLOGD ( ERROR, "pinCode is NULL !!!\n" );
            nv_set ( "lte_stc_pinlock", "1" );
            return;
        }

        snprintf ( strPinCode, sizeof ( strPinCode ),
            "/lib/factory_tool.sh CONFIG_HW_STC_PIN_CODE=%s", pinCode );
        system ( strPinCode );

        snprintf ( pin_cpwd, sizeof ( pin_cpwd ),
            "AT+CPWD=\"SC\",\"%s\",\"%s\"", pinlock_code, pinCode );
        COMD_AT_PROCESS ( pin_cpwd, 1500, at_rep, sizeof ( at_rep ) );
        CLOGD ( FINE, "pin_cpwd is [%s]\n", pin_cpwd );

        nv_set ( "lte_stc_pinlock", "0" );
    }
    else
    {
        nv_set ( "lte_stc_pinlock", "1" );
        CLOGD ( ERROR, "PIN LOCK type is ERROR !!!\n" );
    }
#else
    nv_set ( "lte_stc_pinlock", "1" );
    CLOGD ( ERROR, "No defined CONFIG_SW_STC_PINLOCK !!!\n" );
#endif

}

//SIM LOCK type 00,unlock_code1 : web disable SIM LOCK;
//10,00000000 : SMS disable SIM LOCK;
//11,00000000 : TR069 disable SIM LOCK;
//12,00000000 : TR069 enable SIM LOCK;
void mtkParam_setSimLock ( char* reqData )
{
#if defined (CONFIG_SW_STC_SIMLOCK)
    char *pEnable = NULL;
    char *simCode = NULL;
    char sim_info[16] = {0};
    char simlock_code[16] = {0};
    char strSimLock[64] = {0};

    if ( NULL == reqData )
    {
        CLOGD ( ERROR, "reqData is NULL !!!\n" );
        nv_set ( "lte_stc_simlock", "1" );
        return;
    }

    snprintf ( sim_info, sizeof ( sim_info ), "%s", reqData );
    pEnable = strtok ( sim_info, "," );
    if ( NULL == pEnable )
    {
        CLOGD ( ERROR, "pEnable is NULL !!!\n" );
        nv_set ( "lte_stc_simlock", "1" );
        return;
    }

    if ( 0 == strcmp ( pEnable, "00" ) )
    {
        simCode = strtok ( NULL, "," );
        if ( NULL == simCode )
        {
            CLOGD ( ERROR, "simCode is NULL !!!\n" );
            nv_set ( "lte_stc_simlock", "1" );
            return;
        }

        nv_get ( "unlock_code1", simlock_code, sizeof ( simlock_code ) );
        if ( 0 == strcmp ( simCode, simlock_code ) )
        {
            snprintf ( strSimLock, sizeof ( strSimLock ),
                "/lib/factory_tool.sh CONFIG_HW_STC_SIMLOCK=0" );
            system ( strSimLock );
            snprintf ( simlock_enable, sizeof ( simlock_enable ), "0" );

            mtkParam_setCfunMode ( "on" );//enable scan

            nv_set ( "lte_stc_simlock", "0" );
            CLOGD ( FINE, "web Unlock SIM LOCK is success !!!\n" );
        }
        else
        {
            CLOGD ( ERROR, "simCode is ERROR !!!\n" );
            nv_set ( "lte_stc_simlock", "1" );
            return;
        }
    }
    else if ( 0 == strcmp ( pEnable, "10" ) || 0 == strcmp ( pEnable, "11" ) )
    {
        snprintf ( strSimLock, sizeof ( strSimLock ),
            "/lib/factory_tool.sh CONFIG_HW_STC_SIMLOCK=0" );
        system ( strSimLock );
        snprintf ( simlock_enable, sizeof ( simlock_enable ), "0" );

        mtkParam_setCfunMode ( "on" );//enable scan

        nv_set ( "lte_stc_simlock", "0" );
        CLOGD ( FINE, "SMS or TR069 Unlock SIM LOCK is success !!!\n" );
    }
    else if (  0 == strcmp ( pEnable, "12" ) )
    {
        snprintf ( strSimLock, sizeof ( strSimLock ),
            "/lib/factory_tool.sh CONFIG_HW_STC_SIMLOCK=1" );
        system ( strSimLock );
        snprintf ( simlock_enable, sizeof ( simlock_enable ), "1" );

        nv_set ( "lte_stc_simlock", "0" );
        CLOGD ( FINE, "TR069 lock SIM LOCK is success !!!\n" );
    }
    else
    {
        nv_set ( "lte_stc_simlock", "1" );
        CLOGD ( ERROR, "SIM LOCK type is ERROR !!!\n" );
    }
#else
    nv_set ( "lte_stc_simlock", "1" );
    CLOGD ( ERROR, "No defined CONFIG_HW_STC_SIMLOCK !!!\n" );
#endif
}

#if defined (CONFIG_SW_STC_CELLLOCK)
static int mtk_stc_cell_lock ( int en_flag )
{
    char at_req[32] = {0};
    char at_rep[32] = {0};

    //snprintf ( at_req, sizeof ( at_req ), "AT+ZCWLC=%d", en_flag );

    //COMD_AT_PROCESS ( at_req, 1500, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
    {
        return 1;
    }

    if ( 0 == en_flag )
    {
        system ( "/lib/factory_tool.sh CONFIG_HW_STC_CELLLOCK=0" );
        strcpy ( celllock_enable, "0" );
    }
    else if ( 1 == en_flag )
    {
        system ( "/lib/factory_tool.sh CONFIG_HW_STC_CELLLOCK=1" );
        strcpy ( celllock_enable, "1" );
    }

    return 0;
}
#endif

void mtkParam_setCellLock ( char* reqData )
{
#if defined (CONFIG_SW_STC_CELLLOCK)
    int ret = 1;
    char *cmd_type = NULL;
    char *code_val = NULL;
    char req_info[32] = {0};
    char unlock_code3[16] = {0};
    char unlock_code4[16] = {0};

    if ( NULL == reqData )
    {
        CLOGD ( ERROR, "reqData is NULL !!!\n" );
        goto END;
    }

    snprintf ( req_info, sizeof ( req_info ), "%s", reqData );

    cmd_type = strtok ( req_info, "," );
    if ( NULL == cmd_type )
    {
        CLOGD ( ERROR, "cmd_type is NULL !!!\n" );
        goto END;
    }

    code_val = strtok ( NULL, "," );
    if ( NULL == code_val )
    {
        CLOGD ( ERROR, "code_val is NULL !!!\n" );
        goto END;
    }

    /*
     * disable cell lock function
     * used for web request, check unlock code
     */
    if ( 0 == strcmp ( cmd_type, "00" ) )
    {
        nv_get ( "unlock_code4", unlock_code4, sizeof ( unlock_code4 ) );
        if ( 0 == strcmp ( code_val, unlock_code4 ) )
        {
            ret = mtk_stc_cell_lock ( 0 );
        }
        else
        {
            CLOGD ( ERROR, "unlock_code4 match failed !!!\n" );
        }
    }
    /*
     * enable cell lock function
     * used for web request, check unlock code
     */
    else if ( 0 == strcmp ( cmd_type, "01" ) )
    {
        nv_get ( "unlock_code4", unlock_code4, sizeof ( unlock_code4 ) );
        if ( 0 == strcmp ( code_val, unlock_code4 ) )
        {
            ret = mtk_stc_cell_lock ( 1 );
        }
        else
        {
            CLOGD ( ERROR, "unlock_code4 match failed !!!\n" );
        }
    }
    /*
     * reset cell lock list
     * used for web request, check unlock code
     */
    else if ( 0 == strcmp ( cmd_type, "02" ) )
    {
        nv_get ( "unlock_code3", unlock_code3, sizeof ( unlock_code3 ) );
        if ( 0 == strcmp ( code_val, unlock_code3 ) )
        {
            ret = mtk_stc_cell_lock ( 2 );
        }
        else
        {
            CLOGD ( ERROR, "unlock_code3 match failed !!!\n" );
        }
    }
    /*
     * disable cell lock function
     * used for sms or tr069 request, no need check unlock code
     */
    else if ( 0 == strcmp ( cmd_type, "10" ) )
    {
        ret = mtk_stc_cell_lock ( 0 );
    }
    /*
     * enable cell lock function
     * used for sms or tr069 request, no need check unlock code
     */
    else if ( 0 == strcmp ( cmd_type, "11" ) )
    {
        ret = mtk_stc_cell_lock ( 1 );
    }
    /*
     * reset cell lock list
     * used for sms or tr069 request, no need check unlock code
     */
    else if ( 0 == strcmp ( cmd_type, "12" ) )
    {
        ret = mtk_stc_cell_lock ( 2 );
    }
    else
    {
        CLOGD ( ERROR, "Unknown cmd_type -> [%s] !!!\n", cmd_type );
    }

END:
    CLOGD ( FINE, "ret -> [%d]\n", ret );
    nv_set ( "lte_stc_celllock", ( 0 == ret ) ? "0" : "1" );
#else
    nv_set ( "lte_stc_celllock", "1" );
    CLOGD ( ERROR, "No defined CONFIG_SW_STC_CELLLOCK !!!\n" );
#endif
}

void mtkParam_sendSmsRequest ( KT_sms_msg *send_sms )
{
    char at_req[SEND_MAX_SIZE] = {0};
    char at_rep[RECV_SMS_SIZE] = {0};

    CLOGD ( FINE, "send_sms->send_len: [%d]\n", send_sms->num_len );
    CLOGD ( FINE, "send_sms->send_pdu: [%s]\n", send_sms->sms_msg );

    snprintf ( at_req, sizeof ( at_req ), "AT+CMGS=%d,\"%s\"",
                                send_sms->num_len, send_sms->sms_msg );

    COMD_AT_PROCESS ( at_req, 3000, at_rep, sizeof ( at_rep ) );
    parsing_mtk_gcmgs ( at_rep );
}


