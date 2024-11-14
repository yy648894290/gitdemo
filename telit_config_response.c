#include "comd_share.h"
#include "atchannel.h"
#include "telit_status_refresh.h"
#include "telit_atcmd_parsing.h"
#include "telit_config_response.h"
#include "config_key.h"
#include "hal_platform.h"

extern int apns_msg_flag[5];
extern COMD_DEV_INFO at_dev;

int telit_bandlist_hexstr_to_strlist(char* band_4g_list_hexstr, char* band_5g_list_hexstr,
                                                char* band_4g_list_str, int size_band_4g,
                                                char* band_5g_list_str, int size_band_5g)
{

    char band_range_1_to_33[16] = {0};
    char band_range_34_to_65[16] = {0};
    char band_range_66_to_end[16] = {0};
    char local_band_4g_list_str[256] = {0};
    char local_band_5g_list_str[256] = {0};
    unsigned int band_in_decimal = 0;
    int i = 0;
    int flag = 0;

    snprintf(band_range_1_to_33, 9, "%s", band_4g_list_hexstr + strlen(band_4g_list_hexstr) - 8);
    band_in_decimal = strtol(band_range_1_to_33, NULL, 16);
    CLOGD ( FINE, "band_4g_range_1_to_33 -> [%s] to decimal -> [%d]\n",band_range_1_to_33, band_in_decimal );
    for(i = 0; i < 32; i++)
    {
        flag = band_in_decimal >> i & 0x01;
        if(flag == 1)
        {
            snprintf(local_band_4g_list_str + strlen(local_band_4g_list_str),
                    sizeof(local_band_4g_list_str) - strlen(local_band_4g_list_str), "%d ", i+1);
        }
    }

    snprintf(band_range_34_to_65, 9, "%s", band_4g_list_hexstr + strlen(band_4g_list_hexstr) - 16);
    band_in_decimal = strtol(band_range_34_to_65, NULL, 16);
    CLOGD ( FINE, "band_4g_range_34_to_65 -> [%s] to decimal -> [%d]\n", band_range_34_to_65, band_in_decimal );
    for(i = 0; i < 32; i++)
    {
        flag = band_in_decimal >> i & 0x01;
        if(flag == 1)
        {
            snprintf(local_band_4g_list_str + strlen(local_band_4g_list_str),
                    sizeof(local_band_4g_list_str) - strlen(local_band_4g_list_str), "%d ", i+32+1);
        }
    }

    snprintf(band_range_66_to_end, strlen(band_4g_list_hexstr)-16+1, "%s", band_4g_list_hexstr);
    band_in_decimal = strtol(band_range_66_to_end, NULL, 16);
    CLOGD ( FINE, "band_4g_range_66_to_end -> [%s] to decimal -> [%d]\n", band_range_66_to_end, band_in_decimal );
    for(i = 0; i < strlen(band_range_66_to_end)*4; i++)
    {
        flag = band_in_decimal >> i & 0x01;
        if(flag == 1)
        {
            snprintf(local_band_4g_list_str + strlen(local_band_4g_list_str),
                    sizeof(local_band_4g_list_str) - strlen(local_band_4g_list_str), "%d ", i+64+1);
        }
    }

    local_band_4g_list_str[strlen(local_band_4g_list_str)-1] = '\0';
    CLOGD ( FINE, "local_band_4g_list_str -> [%s] \n", local_band_4g_list_str );

    if( at_dev.comd_mVendor == M_VENDOR_TELIT_5G )
    {
        snprintf(band_range_1_to_33, 9 ,"%s",band_5g_list_hexstr + strlen(band_5g_list_hexstr) - 8);
        band_in_decimal = strtol(band_range_1_to_33, NULL, 16);
        CLOGD ( FINE, "band_5g_range_1_to_33 -> [%s] to decimal -> [%d]\n",band_range_1_to_33, band_in_decimal );
        for(i = 0; i < 32; i++)
        {
            flag = band_in_decimal >> i & 0x01;
            if(flag == 1)
            {
                snprintf(local_band_5g_list_str + strlen(local_band_5g_list_str),
                        sizeof(local_band_5g_list_str) - strlen(local_band_5g_list_str), "%d ", i+1);
            }
        }

        snprintf(band_range_34_to_65, 9, "%s", band_5g_list_hexstr + strlen(band_5g_list_hexstr) - 16);
        band_in_decimal = strtol(band_range_34_to_65, NULL, 16);
        CLOGD ( FINE, "band_5g_range_34_to_65 -> [%s] to decimal -> [%d]\n",band_range_34_to_65, band_in_decimal );
        for(i = 0; i < 32; i++)
        {
            flag = band_in_decimal >> i & 0x01;
            if(flag == 1)
            {
                snprintf(local_band_5g_list_str + strlen(local_band_5g_list_str),
                        sizeof(local_band_5g_list_str) - strlen(local_band_5g_list_str), "%d ", i+32+1);
            }
        }

        snprintf(band_range_66_to_end, strlen(band_5g_list_hexstr)-16+1, "%s", band_5g_list_hexstr);
        band_in_decimal = strtol(band_range_66_to_end, NULL, 16);
        CLOGD ( FINE, "band_5g_range_66_to_end -> [%s] to decimal -> [%d]\n",band_range_66_to_end, band_in_decimal );
        for(i = 0; i < strlen(band_range_66_to_end)*4; i++)
        {
            flag = band_in_decimal >> i & 0x01;
            if(flag == 1)
            {
                snprintf(local_band_5g_list_str + strlen(local_band_5g_list_str),
                        sizeof(local_band_5g_list_str) - strlen(local_band_5g_list_str), "%d ", i+64+1);
            }
        }

        local_band_5g_list_str[strlen(local_band_5g_list_str)-1] = '\0';
        CLOGD ( FINE, "local_band_5g_list_str -> [%s] \n", local_band_5g_list_str );
    }

    snprintf(band_4g_list_str, size_band_4g, "%s", local_band_4g_list_str);
    snprintf(band_5g_list_str, size_band_5g, "%s", local_band_5g_list_str);
    return 0;
}

int telit_band_strlist_to_hexstr(char* band_str_list, char* band_hexstr_list, int band_hexstr_list_size)
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
    snprintf(local_band_hexstr_list,sizeof(local_band_hexstr_list),"%08x%08x,%04x",band_34_to_65,band_1_to_33,band_66_to_end);


    CLOGD(FINE, "Check --> [%s]",local_band_hexstr_list);
    snprintf(band_hexstr_list, band_hexstr_list_size,"%s",local_band_hexstr_list);
    return 0;
}

void telitParam_radioOnOffSet ( char* reqData )
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

    if ( 0 == parsing_telit_cfun_set ( at_rep ) )
        nv_set ( "lte_on_off", reqData );

}

void telitParam_getSimlock ( char* reqData )
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

void telitParam_apnSetting ( apn_profile* const apnSetting_data )
{
    if ( NULL == apnSetting_data )
    {
        CLOGD ( ERROR, "apnSetting_data is NULL !!!\n" );
        nv_set ( "apn_set", "1" );
        return;
    }

    telitParam_initAPNsetting ( ( char * ) apnSetting_data );

    return;
}

void telitParam_config_commandsetting ( char* reqData )
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

static void telit_nsa_lockband_calc(char* lockband_5g_hexstr, char* recover_lockband_5gnsa_hexstr, char* lockband_nsa5g_hexstr, int lockband_nsa5g_hexstr_size)
{
    char band_5g_range_1_to_33[16] = {0};
    char band_5g_range_34_to_65[16] = {0};
    char band_5g_range_66_to_end[16] = {0};
    char band_5gnsa_range_1_to_33[16] = {0};
    char band_5gnsa_range_34_to_65[16] = {0};
    char band_5gnsa_range_66_to_end[16] = {0};

    char band_5g_range_1_to_65[32] = {0};
    char band_5gnsa_range_1_to_65[32] = {0};

    char local_lockband_nsa5g_hexstr[256] = {0};

    sscanf(lockband_5g_hexstr,"%[^,],%s",band_5g_range_1_to_65,band_5g_range_66_to_end);

    sscanf(recover_lockband_5gnsa_hexstr,"%[^,],%s",band_5gnsa_range_1_to_65,band_5gnsa_range_66_to_end);

    if(strlen(band_5g_range_1_to_65) < 8)
    {
        snprintf(band_5gnsa_range_1_to_33, 9, "%s", band_5gnsa_range_1_to_65 + strlen(band_5gnsa_range_1_to_65) - 8);

        long band_5gnsa_range_1_to_33_in_long = strtol(band_5gnsa_range_1_to_33,NULL,16);
        long band_5g_range_1_to_65_in_long = strtol(band_5g_range_1_to_65,NULL,16);

        long band_5gnsa_range_66_to_end_in_long = strtol(band_5gnsa_range_66_to_end,NULL,16);
        long band_5g_range_66_to_end_in_long = strtol(band_5g_range_66_to_end,NULL,16);

        snprintf(local_lockband_nsa5g_hexstr,sizeof(local_lockband_nsa5g_hexstr),"%08lx,%04lx",
            (band_5gnsa_range_1_to_33_in_long & band_5g_range_1_to_65_in_long),
            (band_5gnsa_range_66_to_end_in_long & band_5g_range_66_to_end_in_long));

    }
    else
    {
        snprintf(band_5gnsa_range_1_to_33, 9, "%s", band_5gnsa_range_1_to_65 + strlen(band_5gnsa_range_1_to_65) - 8);
        snprintf(band_5gnsa_range_34_to_65, 9 + strlen(band_5gnsa_range_1_to_65) - 16, "%s", band_5gnsa_range_1_to_65);

        snprintf(band_5g_range_1_to_33, 9, "%s", band_5g_range_1_to_65 + strlen(band_5g_range_1_to_65) - 8);
        snprintf(band_5g_range_34_to_65, 9 + strlen(band_5g_range_1_to_65) - 16, "%s", band_5g_range_1_to_65);

        long band_5gnsa_range_1_to_33_in_long = strtol(band_5gnsa_range_1_to_33,NULL,16);
        long band_5g_range_1_to_33_in_long = strtol(band_5g_range_1_to_33,NULL,16);

        long band_5gnsa_range_34_to_65_in_long = strtol(band_5gnsa_range_34_to_65,NULL,16);
        long band_5g_range_34_to_65_in_long = strtol(band_5g_range_34_to_65,NULL,16);

        long band_5gnsa_range_66_to_end_in_long = strtol(band_5gnsa_range_66_to_end,NULL,16);
        long band_5g_range_66_to_end_in_long = strtol(band_5g_range_66_to_end,NULL,16);

        snprintf(local_lockband_nsa5g_hexstr,sizeof(local_lockband_nsa5g_hexstr),"%08lx%08lx,%04lx",
            (band_5gnsa_range_34_to_65_in_long & band_5g_range_34_to_65_in_long),
            (band_5gnsa_range_1_to_33_in_long & band_5g_range_1_to_33_in_long),
            (band_5gnsa_range_66_to_end_in_long & band_5g_range_66_to_end_in_long));
    }


    CLOGD(FINE, "Check 5g NSA lockband in hexstr -> [%s]",local_lockband_nsa5g_hexstr);
    snprintf(lockband_nsa5g_hexstr,lockband_nsa5g_hexstr_size,"%s",local_lockband_nsa5g_hexstr);

}

void telitParam_lockband ( char* reqData, int lock_type )
{
    char band_value[256] = {0};
    char lockband_cmd[256] = {0};

    char at_rep[64] = {0};
    char mode_pref_cmd[64] = {0};
    char lockbands_4g[128] = {0};
    char lockbands_5g[128] = {0};

    char lockbands_hex_str_4g[256] = {0};
    char lockbands_hex_str_5g[256] = {0};
    char lockbands_hex_str_nsa5g[256] = {0};
    char band_3g_recover_str[32] = {0};
    char band_4g_recover_str[32] = {0};
    char band_5g_nsa_recover_str[32] = {0};
    char band_5g_sa_recover_str[32] = {0};

    //int ret = 0;
    if ( NULL == reqData )
    {
        CLOGD ( ERROR, "reqData is NULL !!!\n" );
        nv_set ( "lockband_set", "1" );
        return;
    }

    CLOGD ( FINE, "band_list -> [%s]\n", reqData );

    nv_get("band_3g_recover_str", band_3g_recover_str, sizeof(band_3g_recover_str));
    nv_get("band_4g_recover_str", band_4g_recover_str, sizeof(band_4g_recover_str));
    nv_get("band_5g_nsa_recover_str", band_5g_nsa_recover_str, sizeof(band_5g_nsa_recover_str));
    nv_get("band_5g_sa_recover_str", band_5g_sa_recover_str, sizeof(band_5g_sa_recover_str));

    sprintf(lockband_cmd,"AT#BND=%s,",band_3g_recover_str);

    if ( strcmp ( reqData, "" ) )
    {
        snprintf ( band_value, sizeof ( band_value ), "%s", reqData );

        if ( ',' == band_value[ strlen ( band_value ) - 1 ] || NULL == strstr ( band_value, "," ) ) // 4G only
        {
            sscanf ( band_value, "%[0-9_],", lockbands_4g );
            snprintf ( mode_pref_cmd, sizeof ( mode_pref_cmd ), "AT+WS46=28" );
        }
        else if ( ',' == band_value[0] ) // 5G only
        {
            sscanf ( band_value, ",%[0-9_]", lockbands_5g );
            snprintf ( mode_pref_cmd, sizeof ( mode_pref_cmd ), "AT+WS46=36" );
        }
        else // 4G & 5G
        {
            sscanf ( band_value, "%[0-9_],%[0-9_]", lockbands_4g, lockbands_5g );
            snprintf ( mode_pref_cmd, sizeof ( mode_pref_cmd ), "AT+WS46=37" );
        }

        CLOGD ( FINE, "lockbands_4g -> [%s]\n", lockbands_4g );
        nv_set ( "lockband_list", lockbands_4g );
        if ( strcmp ( lockbands_4g, "" ) )
        {
            lockbands_4g [ strlen ( lockbands_4g ) - 1 ] = '\0';

            telit_band_strlist_to_hexstr(lockbands_4g,lockbands_hex_str_4g,sizeof(lockbands_hex_str_4g));
            if( at_dev.comd_mVendor == M_VENDOR_TELIT_5G )
            {
                snprintf(lockband_cmd+strlen(lockband_cmd),sizeof(lockband_cmd)-strlen(lockband_cmd),"%s,",lockbands_hex_str_4g);
            }
            else
            {
                snprintf(lockband_cmd+strlen(lockband_cmd),sizeof(lockband_cmd)-strlen(lockband_cmd),"%s",lockbands_hex_str_4g);
            }
        }
        else
        {
            if( at_dev.comd_mVendor == M_VENDOR_TELIT_5G )
            {
                snprintf(lockband_cmd+strlen(lockband_cmd),sizeof(lockband_cmd)-strlen(lockband_cmd),"%s,",band_4g_recover_str);
            }
            else
            {
                snprintf(lockband_cmd+strlen(lockband_cmd),sizeof(lockband_cmd)-strlen(lockband_cmd),"%s",band_4g_recover_str);
            }
        }

        CLOGD ( FINE, "lockbands_5g -> [%s]\n", lockbands_5g );
        nv_set ( "lockband5g_list", lockbands_5g );
        if ( strcmp ( lockbands_5g, "" ) )
        {
            lockbands_5g [ strlen ( lockbands_5g ) - 1 ] = '\0';

            telit_band_strlist_to_hexstr(lockbands_5g,lockbands_hex_str_5g,sizeof(lockbands_hex_str_5g));

            telit_nsa_lockband_calc(lockbands_hex_str_5g,band_5g_nsa_recover_str,lockbands_hex_str_nsa5g,sizeof(lockbands_hex_str_nsa5g));

            snprintf(lockband_cmd+strlen(lockband_cmd),sizeof(lockband_cmd)-strlen(lockband_cmd),"%s,%s",lockbands_hex_str_nsa5g,lockbands_hex_str_5g);
        }
        else
        {
            if( at_dev.comd_mVendor == M_VENDOR_TELIT_5G )
            {
                snprintf(lockband_cmd+strlen(lockband_cmd),sizeof(lockband_cmd)-strlen(lockband_cmd),"%s,",band_5g_nsa_recover_str);
                snprintf(lockband_cmd+strlen(lockband_cmd),sizeof(lockband_cmd)-strlen(lockband_cmd),"%s",band_5g_sa_recover_str);
            }
        }

        CLOGD ( FINE, "mode_pref_cmd -> [%s]\n", mode_pref_cmd );
        CLOGD ( FINE, "lockband_cmd -> [%s]\n", lockband_cmd );

        nv_set ( "lockband_status", "locked" );
    }
    else
    {
        if( at_dev.comd_mVendor == M_VENDOR_TELIT_5G )
        {
            snprintf ( mode_pref_cmd, sizeof ( mode_pref_cmd ), "AT+WS46=38" );
            snprintf( lockband_cmd, sizeof ( lockband_cmd ),"AT#BND=%s,%s,%s,%s",band_3g_recover_str,band_4g_recover_str,
                        band_5g_nsa_recover_str, band_5g_sa_recover_str);
        }
        else
        {
            snprintf ( mode_pref_cmd, sizeof ( mode_pref_cmd ), "AT+WS46=28" );
            snprintf( lockband_cmd, sizeof ( lockband_cmd ),"AT#BND=%s,%s",band_3g_recover_str,band_4g_recover_str);

        }
        CLOGD ( FINE, "mode_pref_cmd -> [%s]\n", mode_pref_cmd );
        CLOGD ( FINE, "lockband_cmd -> [%s]\n", lockband_cmd );

        nv_set ( "lockband_status", "notlocked" );
        nv_set ( "lockband_list", "" );
        nv_set ( "lockband5g_list", "" );
    }

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( lockband_cmd, 3000, at_rep, sizeof ( at_rep ) );
    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( mode_pref_cmd, 3000, at_rep, sizeof ( at_rep ) );
    memset ( at_rep, 0, sizeof ( at_rep ) );

    if ( 1 == lock_type )
    {
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( "AT#BCCHLOCK=1024,0,65535,0,0", 3000, at_rep, sizeof ( at_rep ) );

        if( at_dev.comd_mVendor == M_VENDOR_TELIT_5G )
        {
            memset ( at_rep, 0, sizeof ( at_rep ) );
            COMD_AT_PROCESS ( "AT#5GBCCHLOCK=2", 3000, at_rep, sizeof ( at_rep ) );
        }

        nv_set ( "lockearfcn_status", "notlocked" );
        nv_set ( "lockearfcn_list", "" );
        nv_set ( "lockpci_status", "notlocked" );
        nv_set ( "lockpci_list", "" );

        if ( strcmp ( lockbands_5g, "" ) )
        {
            nv_set ( "module_need_reboot", "1" );
        }
    }

    nv_set ( "lockband_set", "0" );
}

void telitParam_lockearfcn ( char* reqData )
{
    if ( NULL == reqData )
    {
        CLOGD ( ERROR, "reqData is NULL !!!\n" );
        nv_set ( "lockearfcn_set", "1" );
        return;
    }

    char earfcns_list[256] = {0};
    int earfcns_num = 0;
    int earfcn_dl = 0;
    char at_rep[64] = {0};
    char lockband_status[16] = {0};
    char lockpci_status[16] = {0};
    char lockearfcn_status[16] = {0};
    char earfcn[64] = {0};
    char earfcn_5g[64] = {0};

    char lockearfcns_5g[128] = {0};
    char lockearfcn_cmd[64] = {0};
    char lock5gearfcn_cmd[64] = {0};
    char lockband4g_cmd[64] = {0};
    char lockband_cmd[64] = {0};
    char lockband5g_cmd[64] = {0};
    char scs_type[8] = {0};
    int lockearfcn_4g_flag = 0;
    char band_4g[32] = {0};
    char band_5g[32] = {0};
    char scs_cmd[8] = {0};

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
            sscanf ( earfcn_tmp, "4G,%[0-9],%[0-9]", band_4g, earfcn );
            earfcn_dl = atoi( earfcn );
            sprintf(lockband4g_cmd,"%s_",band_4g);
        }
        else if ( 0 == strncmp ( earfcn_tmp, "5G", 2 ) )
        {
            sscanf ( earfcn_tmp, "5G,%[0-9],%[0-9],%[0-9]", band_5g, earfcn_5g, scs_type );
            switch(atoi(scs_type))
            {
                case 15:
                    snprintf(scs_cmd,sizeof(scs_cmd),"0");
                    break;
                case 30:
                    snprintf(scs_cmd,sizeof(scs_cmd),"1");
                    break;
                case 60:
                    snprintf(scs_cmd,sizeof(scs_cmd),"2");
                    break;
                case 120:
                    snprintf(scs_cmd,sizeof(scs_cmd),"3");
                    break;
                case 240:
                    snprintf(scs_cmd,sizeof(scs_cmd),"4");
                    break;
                default:
                    snprintf(scs_cmd,sizeof(scs_cmd),"1");
                    break;
            }

            snprintf (lockearfcns_5g + strlen ( lockearfcns_5g ), sizeof ( lockearfcns_5g ) - strlen ( lockearfcns_5g ), "%s,%s,0,%d,",scs_cmd,earfcn_5g,(atoi(band_5g)-1));

            earfcn_dl = atoi( earfcn_5g );
            sprintf(lockband5g_cmd,",%s_",band_5g);
            earfcns_num ++;
        }
        earfcn_tmp = strtok ( NULL, "_" );
    }

    CLOGD ( FINE, "earfcn -> [%s]\n", earfcn );
    lockearfcns_5g [ strlen ( lockearfcns_5g ) - 1 ] = '\0';
    CLOGD ( FINE, "lockearfcns_5g -> [%s]\n", lockearfcns_5g );

    if ( lockearfcn_4g_flag == 1 && earfcns_num != 0 )
    {
        snprintf ( lockearfcn_cmd, sizeof ( lockearfcn_cmd ), "AT#BCCHLOCK=1024,0,65535,%s,0", earfcn );
        snprintf ( lock5gearfcn_cmd, sizeof ( lock5gearfcn_cmd ), "AT#5GBCCHLOCK=1,%s", lockearfcns_5g );
        sprintf(lockband_cmd,"%s%s",lockband4g_cmd,lockband5g_cmd);
    }
    else if ( lockearfcn_4g_flag == 1 && earfcns_num == 0 )
    {
        snprintf ( lockearfcn_cmd, sizeof ( lockearfcn_cmd ), "AT#BCCHLOCK=1024,0,65535,%s,0", earfcn );
        snprintf ( lock5gearfcn_cmd, sizeof ( lock5gearfcn_cmd ), "AT#5GBCCHLOCK=2" );
        sprintf(lockband_cmd,"%s",lockband4g_cmd);
    }
    else if ( lockearfcn_4g_flag == 0 && earfcns_num != 0 )
    {
        snprintf ( lockearfcn_cmd, sizeof ( lockearfcn_cmd ), "AT#BCCHLOCK=1024,0,65535,0,0" );
        snprintf ( lock5gearfcn_cmd, sizeof ( lock5gearfcn_cmd ), "AT#5GBCCHLOCK=1,%s", lockearfcns_5g );
        sprintf(lockband_cmd,"%s",lockband5g_cmd);

    }


    nv_get ( "lockband_status", lockband_status, sizeof ( lockband_status ) );
    nv_get ( "lockpci_status", lockpci_status, sizeof ( lockpci_status ) );
    nv_get ( "lockearfcn_status", lockearfcn_status, sizeof ( lockearfcn_status ) );
    CLOGD ( FINE, "lockband_status -> [%s]\n", lockband_status );
    CLOGD ( FINE, "lockpci_status -> [%s]\n", lockpci_status );
    CLOGD ( FINE, "lockearfcn_status -> [%s]\n", lockearfcn_status );


    telitParam_radioOnOffSet("off");
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

    if( at_dev.comd_mVendor == M_VENDOR_TELIT_5G )
    {
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
    }
    telitParam_lockband ( lockband_cmd, 0 );

    nv_set ( "module_need_reboot", "1" );

    nv_set ( "lockearfcn_status", "locked" );
    nv_set ( "lockearfcn_list", reqData );
    nv_set ( "lockband_status", "notlocked" );
    nv_set ( "lockband_list", "" );
    nv_set ( "lockpci_status", "notlocked" );
    nv_set ( "lockpci_list", "" );

    nv_set ( "lockearfcn_set", "0" );

}

void telitParam_lockpci ( char* reqData )
{
    if ( NULL == reqData )
    {
        CLOGD ( ERROR, "reqData is NULL !!!\n" );
        nv_set ( "lockpci_set", "1" );
        return;
    }

    char pcis_list[256] = {0};
    int pcis_num = 0;
    char at_req[256] = {0};
    char at_rep[64] = {0};
    char lockband_status[16] = {0};
    char lockearfcn_status[16] = {0};
    char lockpci_status[16] = {0};
    char pci_4g[64] = {0};
    char pci_5g[64] = {0};
    char lockearfcn_5g[8] = {0};
    char band_5g[8] = {0};
    char band_4g[8] = {0};
    char scs_type[4] = {0};
    char scs_cmd[4] = {0};
    char mode_pref_cmd[64] = {0};
    char lockpcis_4g[128] = {0};
    char lockpcis_5g[128] = {0};
    char lockband_cmd[64] = {0};
    char lockband4g_cmd[32] = {0};
    char lockband5g_cmd[32] = {0};
    char lockpci_cmd[256] = {0};
    char lock5gpci_cmd[64] = {0};
    int lock5gpci_flag = 0;
    char earfcn[64] = {0};

    snprintf ( pcis_list, sizeof ( pcis_list ), "%s", reqData );
    CLOGD ( FINE, "pcis_list -> [%s]\n", pcis_list );

    if ( NULL == strstr ( pcis_list, "4G") && NULL == strstr ( pcis_list, "5G") )
    {
        nv_set ( "lockpci_set", "1" );
        return;
    }

    char *pci_tmp = strtok ( pcis_list, "_" );
    while ( pci_tmp )
    {
        if ( 0 == strncmp ( pci_tmp, "4G", 2 ) )
        {
            sscanf ( pci_tmp, "4G,%[0-9],%[0-9],%[0-9]", band_4g, earfcn, pci_4g );
            snprintf ( lockpcis_4g + strlen ( lockpcis_4g ), sizeof ( lockpcis_4g ) - strlen ( lockpcis_4g ),"%s,%s", earfcn, pci_4g );
            sprintf(lockband4g_cmd,"%s_",band_4g);
            pcis_num ++;
        }
        else if ( 0 == strncmp ( pci_tmp, "5G", 2 ) )
        {
            lock5gpci_flag = 1;
            sscanf ( pci_tmp, "5G,%[0-9],%[0-9],%[0-9],%[0-9]", band_5g, lockearfcn_5g, pci_5g, scs_type );
            switch(atoi(scs_type))
            {
                case 15:
                    snprintf(scs_cmd,sizeof(scs_cmd),"0");
                    break;
                case 30:
                    snprintf(scs_cmd,sizeof(scs_cmd),"1");
                    break;
                case 60:
                    snprintf(scs_cmd,sizeof(scs_cmd),"2");
                    break;
                case 120:
                    snprintf(scs_cmd,sizeof(scs_cmd),"3");
                    break;
                case 240:
                    snprintf(scs_cmd,sizeof(scs_cmd),"4");
                    break;
                default:
                    snprintf(scs_cmd,sizeof(scs_cmd),"1");
                    break;
            }

            snprintf ( lockpcis_5g, sizeof ( lockpcis_5g ), "%s,%s,%s,%d", scs_cmd,lockearfcn_5g,pci_5g,(atoi(band_5g)-1) );
            sprintf(lockband5g_cmd,",%s_",band_5g);
        }
        pci_tmp = strtok ( NULL, "_" );
    }


    CLOGD ( FINE, "lockpcis_4g -> [%s]\n", lockpcis_4g );
    CLOGD ( FINE, "lockpcis_5g -> [%s]\n", lockpcis_5g );

    if ( pcis_num != 0 && lock5gpci_flag == 1 )
    {
        snprintf ( lockpci_cmd, sizeof ( lockpci_cmd ), "AT#BCCHLOCK=1024,0,65535,%s", lockpcis_4g );
        snprintf ( lock5gpci_cmd, sizeof ( lock5gpci_cmd ), "AT#5GBCCHLOCK=0,%s", lockpcis_5g );
        sprintf(lockband_cmd,"%s%s",lockband4g_cmd,lockband5g_cmd);
    }
    else if ( pcis_num != 0 && lock5gpci_flag == 0 )
    {
        snprintf ( lockpci_cmd, sizeof ( lockpci_cmd ), "AT#BCCHLOCK=1024,0,65535,%s", lockpcis_4g );
        snprintf ( lock5gpci_cmd, sizeof ( lock5gpci_cmd ), "AT#5GBCCHLOCK=2" );
        sprintf(lockband_cmd,"%s",lockband4g_cmd);

    }
    else if ( pcis_num == 0 && lock5gpci_flag == 1 )
    {
        snprintf ( lockpci_cmd, sizeof ( lockpci_cmd ), "AT#BCCHLOCK=1024,0,65535,0,0" );
        snprintf ( lock5gpci_cmd, sizeof ( lock5gpci_cmd ), "AT#5GBCCHLOCK=0,%s", lockpcis_5g );
        sprintf(lockband_cmd,"%s",lockband5g_cmd);
    }

    nv_get ( "lockband_status", lockband_status, sizeof ( lockband_status ) );
    nv_get ( "lockpci_status", lockpci_status, sizeof ( lockpci_status ) );
    nv_get ( "lockearfcn_status", lockearfcn_status, sizeof ( lockearfcn_status ) );
    CLOGD ( FINE, "lockband_status -> [%s]\n", lockband_status );
    CLOGD ( FINE, "lockpci_status -> [%s]\n", lockpci_status );
    CLOGD ( FINE, "lockearfcn_status -> [%s]\n", lockearfcn_status );


    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( lockpci_cmd, 3000, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
    {
        nv_set ( "lockpci_set", "1" );
        return;
    }

    if( at_dev.comd_mVendor == M_VENDOR_TELIT_5G )
    {
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( lock5gpci_cmd, 3000, at_rep, sizeof ( at_rep ) );

        if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
        {
            nv_set ( "lockpci_set", "1" );
            return;
        }
    }
    telitParam_lockband ( lockband_cmd, 0 );

    nv_set ( "module_need_reboot", "1" );
    nv_set ( "lockearfcn_status", "notlocked" );
    nv_set ( "lockearfcn_list", "" );
    nv_set ( "lockband_status", "notlocked" );
    nv_set ( "lockband_list", "" );
    nv_set ( "lockpci_status", "locked" );
    nv_set ( "lockpci_list", reqData );

    nv_set ( "lockpci_set", "0" );


}

void telitParam_connectNetwork ( char* reqData )
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

void telitParam_disconnectNetwork ( char* reqData )
{
    char at_rep[32] = {0};

    COMD_AT_PROCESS ( "AT+COPS=2", 10000, at_rep, sizeof ( at_rep ) );

    nv_set ( "disconnect_network_set", strstr ( at_rep, "\r\nOK\r\n" ) ? "0" : "1" );
}

void telitParam_netSelectModeSet ( char* reqData )
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

void telitParam_setDefaultGateway ( char* reqData )
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

void telitParam_dualSimSet ( char* reqData )
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

void telitParam_lockPin ( char* reqData )
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

    telitParam_updatePinStatus ();
    telitParam_updatePinLockStatus ();
    telitParam_updatePinPukCount ();

END:
    parsing_telit_lockpin ( at_rep );
}

void telitParam_enterPin ( char* reqData )
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

    telitParam_updatePinStatus ();
    telitParam_updatePinPukCount ();

END:
    parsing_telit_enterpin ( at_rep );
}

void telitParam_modifyPin ( char* reqData )
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

    telitParam_updatePinStatus ();
    telitParam_updatePinPukCount ();

END:
    parsing_telit_modifypin ( at_rep );
}

void telitParam_enterPuk ( char* reqData )
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

    telitParam_updatePinStatus ();
    telitParam_updatePinPukCount ();

END:
    parsing_telit_enterpuk ( at_rep );
}

void telitParam_getNitzTime ( char* reqData )
{
    char at_rep[128] = {0};

    COMD_AT_PROCESS ( "AT+CTZU=1", 1500, at_rep, sizeof ( at_rep ) );

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( "AT+CCLK?", 1500, at_rep, sizeof ( at_rep ) );

    parsing_telit_cclk ( at_rep );
}

