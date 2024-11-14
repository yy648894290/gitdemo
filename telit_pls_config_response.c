#include "comd_share.h"
#include "atchannel.h"
#include "telit_pls_status_refresh.h"
#include "telit_pls_atcmd_parsing.h"
#include "telit_pls_config_response.h"
#include "config_key.h"
#include "hal_platform.h"

extern int apns_msg_flag[5];
extern COMD_DEV_INFO at_dev;

int telit_pls_band_strlist_to_hexstr(char* band_str_list, char* band_hexstr_list, int band_hexstr_list_size)
{

    unsigned int band_1_to_33 = 0;
    unsigned int band_34_to_65 = 0;
    unsigned int band_66_to_end = 0;

    char* band_ptr = NULL;
    char local_band_hexstr_list[256] = {0};

    band_ptr = strtok(band_str_list,"_");
    if(1 <= atoi(band_ptr) &&  atoi(band_ptr) <= 32 )
    {
         band_1_to_33 |= 1 << (atoi(band_ptr)-1);
    }
    else if(33 <= atoi(band_ptr) &&  atoi(band_ptr) <= 65 )
    {
         band_34_to_65 |= 1 << (atoi(band_ptr)-33);
    }
    else
    {
        band_66_to_end |= 1 << (atoi(band_ptr)-65);
    }

    band_ptr = strtok(NULL ,"_");
    while(band_ptr != NULL)
    {

        if(1 <= atoi(band_ptr) &&  atoi(band_ptr) <= 32 )
        {
             band_1_to_33 |= 1 << (atoi(band_ptr)-1);
        }
        else if(33 <= atoi(band_ptr) &&  atoi(band_ptr) <= 65 )
        {
             band_34_to_65 |= 1 << (atoi(band_ptr)-33);
        }
        else
        {
            band_66_to_end |= 1 << (atoi(band_ptr)-65);
        }

        band_ptr = strtok(NULL ,"_");
    }

    snprintf(local_band_hexstr_list,sizeof(local_band_hexstr_list),"%08x,%04x%08x",band_1_to_33,band_66_to_end,band_34_to_65);

    CLOGD(FINE, "Check --> [%s]",local_band_hexstr_list);
    snprintf(band_hexstr_list, band_hexstr_list_size,"%s",local_band_hexstr_list);
    return 0;
}

void telitPlsParam_radioOnOffSet ( char* reqData )
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
    else if ( 0 == strcmp ( reqData, "radio_off" ) )
    {
        COMD_AT_PROCESS ( "AT+CFUN=0", 3000, at_rep, sizeof ( at_rep ) );
    }
    else
    {
        COMD_AT_PROCESS ( "AT+CFUN=1", 3000, at_rep, sizeof ( at_rep ) );
    }

    if ( 0 == parsing_telit_pls_cfun_set ( at_rep ) )
        nv_set ( "lte_on_off", reqData );

}

void telitPlsParam_getSimlock ( char* reqData )
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

void telitPlsParam_apnSetting ( apn_profile* const apnSetting_data )
{
    if ( NULL == apnSetting_data )
    {
        CLOGD ( ERROR, "apnSetting_data is NULL !!!\n" );
        nv_set ( "apn_set", "1" );
        return;
    }

    telitPlsParam_initAPNsetting ( ( char * ) apnSetting_data );

    return;
}

void telitPlsParam_config_commandsetting ( char* reqData )
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

    LogWebShellSet ( at_rep );
    LogWebShellSet ( "\r\n" );

    CLOGD( FINE, "web_shell_command OK\n" );

END:
    nv_set ( "web_command_flag", "0" );
}

void telitPlsParam_lockband ( char* reqData, int lock_type )
{
    char band_value[256] = {0};
    char lockband_cmd[256] = {0};

    char at_rep[64] = {0};
    char lockbands_4g[128] = {0};

    char lockbands_hex_str_4g[256] = {0};
    char band_4g_recover_str[32] = {0};

    //int ret = 0;
    if ( NULL == reqData )
    {
        CLOGD ( ERROR, "reqData is NULL !!!\n" );
        nv_set ( "lockband_set", "1" );
        return;
    }

    CLOGD ( FINE, "band_list -> [%s]\n", reqData );

    snprintf(band_4g_recover_str, sizeof(band_4g_recover_str),"%s,%s",TELIT_PLS_4G_BANDLIST_B1_B32_HEX,TELIT_PLS_4G_BANDLIST_B33_B66_HEX);

    if ( strcmp ( reqData, "" ) )
    {
        snprintf ( band_value, sizeof ( band_value ), "%s", reqData );

        if ( ',' == band_value[ strlen ( band_value ) - 1 ] || NULL == strstr ( band_value, "," ) ) // 4G only
        {
            sscanf ( band_value, "%[0-9_],", lockbands_4g );
        }
        else if ( ',' == band_value[0] ) // 5G only
        {
            CLOGD ( ERROR, "Not support 5G\n" );
            nv_set ( "lockband_set", "1" );
            return;
        }
        else // 4G & 5G
        {
            CLOGD ( ERROR, "Not support 5G\n" );
            nv_set ( "lockband_set", "1" );
            return;
        }

        CLOGD ( FINE, "lockbands_4g -> [%s]\n", lockbands_4g );
        nv_set ( "lockband_list", lockbands_4g );
        if ( strcmp ( lockbands_4g, "" ) )
        {
            lockbands_4g [ strlen ( lockbands_4g ) - 1 ] = '\0';
            telit_pls_band_strlist_to_hexstr(lockbands_4g,lockbands_hex_str_4g,sizeof(lockbands_hex_str_4g));
            snprintf(lockband_cmd,sizeof(lockband_cmd),"AT^SCFG=\"Radio/Band/4G\",%s",lockbands_hex_str_4g);
        }
        else
        {
            snprintf(lockband_cmd,sizeof(lockband_cmd),"AT^SCFG=\"Radio/Band/4G\",%s",band_4g_recover_str);
        }

        CLOGD ( FINE, "lockband_cmd -> [%s]\n", lockband_cmd );

        nv_set ( "lockband_status", "locked" );
    }
    else
    {

        snprintf( lockband_cmd, sizeof ( lockband_cmd ),"AT^SCFG=\"Radio/Band/4G\",%s",band_4g_recover_str);

        CLOGD ( FINE, "lockband_cmd -> [%s]\n", lockband_cmd );

        nv_set ( "lockband_status", "notlocked" );
        nv_set ( "lockband_list", "" );
        nv_set ( "lockband5g_list", "" );
    }

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( lockband_cmd, 3000, at_rep, sizeof ( at_rep ) );

    nv_set ( "lockband_set", "0" );
}

void telitPlsParam_connectNetwork ( char* reqData )
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

void telitPlsParam_disconnectNetwork ( char* reqData )
{
    char at_rep[32] = {0};

    COMD_AT_PROCESS ( "AT+COPS=2", 10000, at_rep, sizeof ( at_rep ) );

    nv_set ( "disconnect_network_set", strstr ( at_rep, "\r\nOK\r\n" ) ? "0" : "1" );
}


void telitPlsParam_netSelectModeSet ( char* reqData )
{
    char cur_select_mode[32] = {0};
    char at_req[64] = {0};
    char at_rep[32] = {0};
    char strRamOpt[64] = {0};

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

void telitPlsParam_setDefaultGateway ( char* reqData )
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
            apnActOrDeactMsgEvent ( apn_index, 1 );
        }
    }

    nv_set ( "default_gateway_set", "0" );
}

void telitPlsParam_lockPin ( char* reqData )
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

    telitPlsParam_updatePinStatus ();
    telitPlsParam_updatePinLockStatus ();
    telitPlsParam_updatePinPukCount ();

END:
    parsing_telit_pls_lockpin ( at_rep );
}

void telitPlsParam_enterPin ( char* reqData )
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

    telitPlsParam_updatePinStatus ();
    telitPlsParam_updatePinPukCount ();

END:
    parsing_telit_enterpin ( at_rep );
}

void telitPlsParam_modifyPin ( char* reqData )
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

    telitPlsParam_updatePinStatus ();
    telitPlsParam_updatePinPukCount ();

END:
    parsing_telit_pls_modifypin ( at_rep );
}

void telitPlsParam_enterPuk ( char* reqData )
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

    telitPlsParam_updatePinStatus ();
    telitPlsParam_updatePinPukCount ();

END:
    parsing_telit_pls_enterpuk ( at_rep );
}

void telitPlsParam_getNitzTime ( char* reqData )
{
    char at_rep[128] = {0};

    COMD_AT_PROCESS ( "AT+CTZU=1", 1500, at_rep, sizeof ( at_rep ) );

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( "AT+CCLK?", 1500, at_rep, sizeof ( at_rep ) );

    parsing_telit_pls_cclk ( at_rep );
}

void telitPlsParam_setSearchNeighbor (void)
{
    char at_rep[RECV_BUF_SIZE] = {0};

    COMD_AT_PROCESS ( "AT^SMONP", 1500, at_rep, sizeof ( at_rep ) );

    parsing_telit_pls_smonp(at_rep);
}

