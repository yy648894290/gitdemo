#include "comd_share.h"
#include "atchannel.h"
#include "at_tok.h"
#include "comd_ip_helper.h"
#include "tdtek_atcmd_parsing.h"
#include "config_key.h"
#include "hal_platform.h"

extern CELL_EARFCN_FREQ earfcn_freq;
extern int apns_msg_flag[5];

int parsing_tdtek_imei ( char* data )
{
    char regex_buf[2][REGEX_BUF_ONE];
    int ret = 0;
    char *pattern = NULL;

    pattern = "\\IMEI:[ ]*([0-9]*).*OK.*$";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( data, pattern, regex_buf );
    if ( 0 == ret )
    {
        nv_set ( "imei", regex_buf[1] );
    }
    else
    {
        nv_set ( "imei", "" );
        CLOGD ( WARNING, "update module imei failed !\n" );
    }

    return ret;
}

int parsing_tdtek_sn ( char* data )
{
    char regex_buf[2][REGEX_BUF_ONE];
    int ret = 0;
    char *pattern = NULL;

    pattern = "\\getsn:\r\n([^\r\n]*).*$";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "getsn:" ), pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( "SN", regex_buf[1] );
    }
    else
    {
        nv_set ( "SN", "" );
        CLOGD ( WARNING, "update module SN failed !\n" );
    }

    return ret;
}

int parsing_tdtek_moduleModel ( char* data )
{
    char regex_buf[2][REGEX_BUF_ONE];
    int ret = 0;
    char *pattern = NULL;

#if defined(_MTK_7621_) || defined(_MTK_7981_) || defined(_MTK_7628_)
    pattern = "\\Model:[ ]*([^\r\n]*).*OK.*$";
#else
    pattern = "\\Model:[ ]*Modem[ ]*([^\r\n]*).*OK.*$";
#endif

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( data, pattern, regex_buf );
    if ( 0 == ret )
    {
        if ( 0 == strcmp ( regex_buf[1], "V100R100C10B670" ) )
        {
            nv_set ( "modulemodel", "MT5710-CN" );
        }
        else
        {
            if ( 0 == strcmp ( regex_buf[1], "MT5710_CN" ) )
            {
                nv_set ( "modulemodel", "MT5710-CN" );
            }
            else
            {
                nv_set ( "modulemodel", regex_buf[1] );
            }
        }
    }
    else
    {
        nv_set ( "modulemodel", "" );
        CLOGD ( WARNING, "update module model failed !\n" );
    }

    return ret;
}

int parsing_tdtek_moduleVersion ( char* data )
{
    char regex_buf[3][REGEX_BUF_ONE];
    int ret = 0;
    char *pattern = NULL;

    pattern = "\\Revision:[ ]*([^\r\n]*).*OK.*$";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );
    memset ( regex_buf[2], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( data, pattern, regex_buf );
    if ( 0 == ret )
    {
        nv_set ( "moduleversion", regex_buf[1] );
        nv_set ( "moduleVerTime", regex_buf[2] );
    }
    else
    {
        nv_set ( "moduleversion", "" );
        nv_set ( "moduleVerTime", "" );
        CLOGD ( WARNING, "update module version failed !\n" );
    }

    return ret;
}

int parsing_tdtek_apn ( char* data )
{
    char regex_buf[3][REGEX_BUF_ONE];
    int ret = 0;
    int i = 0;
    char *pattern = NULL;

    pattern = "\\+CGDCONT: 1,\"([IPV46]*)\",\"([-0-9a-zA-Z .]*)\",[ .,:+A-Za-z0-9\"]*.*";

    for ( i = 0; i < 3; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "+CGDCONT: 1," ), pattern, regex_buf );

    if ( ret == 0 )
    {
        nv_set ( "pdn1_type", strcmp ( regex_buf[1], "IP" ) ? ( strcmp ( regex_buf[1], "IPV6" ) ? "2" : "1" ) : "0" );
        nv_set ( "apn1_name", regex_buf[2] );
        nv_set ( "apn1_enable", "1" );
        CLOGD ( FINE, "pdn1_type: [%s], apn1_name: [%s]\n", regex_buf[1], regex_buf[2] );
    }
    else
    {
        nv_set ( "pdn1_type", "0" );
        nv_set ( "apn1_name", "" );
        nv_set ( "apn1_enable", "0" );
        CLOGD ( ERROR, "AT+CGDCONT? regex apn1 error !\n" );
    }

    pattern = "\\+CGDCONT: 2,\"([IPV46]*)\",\"([-0-9a-zA-Z .]*)\",[ .,:+A-Za-z0-9\"]*.*";

    for ( i = 0; i < 3; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "+CGDCONT: 2," ), pattern, regex_buf );

    if ( ret == 0 )
    {
        nv_set ( "pdn2_type", strcmp ( regex_buf[1], "IP" ) ? ( strcmp ( regex_buf[1], "IPV6" ) ? "2" : "1" ) : "0" );
        nv_set ( "apn2_name", regex_buf[2] );
        nv_set ( "apn2_enable", "1" );
        CLOGD ( FINE, "pdn2_type: [%s], apn2_name: [%s]\n", regex_buf[1], regex_buf[2] );
    }
    else
    {
        nv_set ( "pdn2_type", "0" );
        nv_set ( "apn2_name", "" );
        nv_set ( "apn2_enable", "0" );
        CLOGD ( ERROR, "AT+CGDCONT? regex apn2 error !\n" );
    }

    pattern = "\\+CGDCONT: 3,\"([IPV46]*)\",\"([-0-9a-zA-Z .]*)\",[ .,:+A-Za-z0-9\"]*.*";

    for ( i = 0; i < 3; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "+CGDCONT: 3," ), pattern, regex_buf );

    if ( ret == 0 )
    {
        nv_set ( "pdn3_type", strcmp ( regex_buf[1], "IP" ) ? ( strcmp ( regex_buf[1], "IPV6" ) ? "2" : "1" ) : "0" );
        nv_set ( "apn3_name", regex_buf[2] );
        nv_set ( "apn3_enable", "1" );
        CLOGD ( FINE, "pdn3_type: [%s], apn3_name: [%s]\n", regex_buf[1], regex_buf[2] );
    }
    else
    {
        nv_set ( "pdn3_type", "0" );
        nv_set ( "apn3_name", "" );
        nv_set ( "apn3_enable", "0" );
        CLOGD ( ERROR, "AT+CGDCONT? regex apn3 error !\n" );
    }

    pattern = "\\+CGDCONT: 4,\"([IPV46]*)\",\"([-0-9a-zA-Z .]*)\",[ .,:+A-Za-z0-9\"]*.*";

    for ( i = 0; i < 3; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "+CGDCONT: 4," ), pattern, regex_buf );

    if ( ret == 0 )
    {
        nv_set ( "pdn4_type", strcmp ( regex_buf[1], "IP" ) ? ( strcmp ( regex_buf[1], "IPV6" ) ? "2" : "1" ) : "0" );
        nv_set ( "apn4_name", regex_buf[2] );
        nv_set ( "apn4_enable", "1" );
        CLOGD ( FINE, "pdn4_type: [%s], apn4_name: [%s]\n", regex_buf[1], regex_buf[2] );
    }
    else
    {
        nv_set ( "pdn4_type", "0" );
        nv_set ( "apn4_name", "" );
        nv_set ( "apn4_enable", "0" );
        CLOGD ( ERROR, "AT+CGDCONT? regex apn4 error !\n" );
    }

    return 0;
}

void parsing_tdtek_cimi ( char* data )
{
    char regex_buf[2][REGEX_BUF_ONE];
    int ret = 0;
    char *pattern = NULL;

    pattern = "[\r\n]*([0-9]*).*OK.*$";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( data, pattern, regex_buf );
    if ( 0 == ret )
    {
        nv_set ( "imsi", regex_buf[1] );
    }
    else
    {
        nv_set ( "imsi", "--" );
        CLOGD ( FINE, "update sim imsi failed !\n" );
    }

    return;
}

void parsing_tdtek_simMncLen ( char* data )
{
    int ret = 0;
    int buf_len = 0;
    char *pattern = NULL;
    char regex_buf[2][REGEX_BUF_ONE];
    char imsi_val[32] = {0};
    char usim_mcc[4] = {0};
    char usim_mnc[4] = {0};

    pattern = "\\+CRSM:[ ]*[0-9]*,[ ]*[0-9]*,[ ]*\"(.*)\".*OK.*$";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "+CRSM:" ), pattern, regex_buf );

    nv_get ( "imsi", imsi_val, sizeof ( imsi_val ) );
    strncpy ( usim_mcc, imsi_val, 3 );

    if ( 0 == ret )
    {
        buf_len = strlen ( regex_buf[1] );
        if ( 0 < buf_len && '3' == regex_buf[1][buf_len - 1] )
        {
            strncpy ( usim_mnc, imsi_val + 3, 3 );
            nv_set ( "mnc_len", "3" );
        }
        else
        {
            strncpy ( usim_mnc, imsi_val + 3, 2 );
            nv_set ( "mnc_len", "2" );
        }
    }
    else
    {
        strncpy ( usim_mnc, imsi_val + 3, 2 );
        nv_set ( "mnc_len", "2" );
        CLOGD ( WARNING, "AT+CRSM get mncLen return error !\n" );
    }

    nv_set ( "usim_mcc", usim_mcc );
    nv_set ( "usim_mnc", usim_mnc );
}

void parsing_tdtek_sim_spn ( char* data )
{
    int ret = 0;
    char *pattern = NULL;
    char SIM_SPN[64] = {0};
    char regex_buf[2][REGEX_BUF_ONE];

    pattern = "\\+CRSM:[^,]*,[^,]*,[ ]*\"([^\"]*)\".*$";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "+CRSM:" ), pattern, regex_buf );

    if ( 0 == ret && 34 <= strlen ( regex_buf[1] ) )
    {
        HexToStr ( SIM_SPN, regex_buf[1] + 2, sizeof ( SIM_SPN ) );
        nv_set ( "SIM_SPN", SIM_SPN );
        nv_set ( "SIM_SPN_HEX", regex_buf[1] );
        CLOGD ( FINE, "SIM_SPN: [%s] -> [%s]\n", regex_buf[1], SIM_SPN );
    }
    else
    {
        nv_set ( "SIM_SPN", "--" );
        nv_set ( "SIM_SPN_HEX", "--" );
        CLOGD ( FINE, "AT+CRSM get SPN return error !\n" );
    }
}

void parsing_tdtek_iccid ( char* data )
{
    char regex_buf[2][REGEX_BUF_ONE];
    int ret = 0;
    char *pattern = NULL;

    pattern = "\\^ICCID:[ ]*([^\r\n]*).*OK.*$";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "^ICCID:" ), pattern, regex_buf );
    if ( 0 == ret )
    {
        nv_set ( "iccid", regex_buf[1] );
    }
    else
    {
        nv_set ( "iccid", "--" );
        CLOGD ( FINE, "update sim iccid failed !\n" );
    }

    return;
}

void parsing_tdtek_operator ( char* data )
{
    char regex_buf[4][REGEX_BUF_ONE];
    int ret = 0;
    char *pattern = NULL;

    pattern = "\\+COPS: ([0-9]*),[0-9]*,\"(.*)\",([0-9]*)";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );
    memset ( regex_buf[2], 0, REGEX_BUF_ONE );
    memset ( regex_buf[3], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "+COPS:" ), pattern, regex_buf );
    if ( ret == 0 )
    {
        nv_set ( "cops", regex_buf[1] );
        nv_set ( "operator", regex_buf[2] );
        nv_set ( "netmode", regex_buf[3] );
    }
    else
    {
        nv_set ( "cops", "--" );
        nv_set ( "operator", "--" );
        nv_set ( "netmode", "--" );
        CLOGD ( FINE, "AT+COPS? return invalid !\n" );
    }

    return;
}

void parsing_tdtek_lockpin ( char* data )
{
    if ( strstr ( data, "\r\nOK\r\n" ) )
    {
        nv_set ( "lockpin_set", "0" );
    }
    else
    {
        nv_set ( "lockpin_set", "1" );
    }

    return;
}

void parsing_tdtek_enterpin ( char* data )
{
    if ( strstr ( data, "\r\nOK\r\n" ) )
    {
        nv_set ( "enterpin_set", "0" );
    }
    else
    {
        nv_set ( "enterpin_set", "1" );
    }

    return;
}

void parsing_tdtek_modifypin ( char* data )
{
    if ( strstr ( data, "\r\nOK\r\n" ) )
    {
        nv_set ( "modifypin_set", "0" );
    }
    else
    {
        nv_set ( "modifypin_set", "1" );
    }

    return;
}

void parsing_tdtek_enterpuk ( char* data )
{
    if ( strstr ( data, "\r\nOK\r\n" ) )
    {
        nv_set ( "enterpuk_set", "0" );
    }
    else
    {
        nv_set ( "enterpuk_set", "1" );
    }

    return;
}

int parsing_tdtek_cfun_set ( char* data )
{
    if ( strstr ( data, "\r\nOK\r\n" ) )
    {
        nv_set ( "setOnOff_lte", "0" );
        return 0;
    }
    else
    {
        nv_set ( "setOnOff_lte", "1" );
        return -1;
    }
}

int parsing_tdtek_cfun_get ( char* data )
{
    if ( strstr ( data, "+CFUN: 1" ) )
    {
        return 1;
    }
    else if( strstr ( data, "+CFUN: 0" ) || strstr ( data, "+CFUN: 4" ))
    {
        return 0;
    }

    return -1;
}

void parsing_tdtek_cpin_get ( char* data )
{
    int ret = 0;
    char *pattern = NULL;
    char regex_buf[2][REGEX_BUF_ONE];

    pattern = "\\+CPIN: ([A-Z1-2 ]*).*OK.*$";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "+CPIN:" ), pattern, regex_buf );

    if ( 0 == ret )
    {
        if ( 0 == strcmp ( regex_buf[1], "SIM PIN2" ) ||
                0 == strcmp ( regex_buf[1], "SIM PUK2" ) )
        {
            nv_set ( "cpin", "READY" );
        }
        else
        {
            nv_set ( "cpin", regex_buf[1] );
        }
    }
    else
    {
        nv_set ( "cpin", "ERROR" );
        nv_set ( "signal_bar", "-1" );
        CLOGD ( WARNING, "AT+CPIN? return ERROR !\n" );
    }

    return;
}

void parsing_tdtek_clck_get ( char* data )
{
    char regex_buf[2][REGEX_BUF_ONE];
    int ret = 0;
    char *pattern = NULL;

    pattern = "\\+CLCK: ([0-9]*).*";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "+CLCK:" ), pattern, regex_buf );
    if ( 0 == ret )
    {
        nv_set ( "lockpin_status", regex_buf[1] );
    }
    else
    {
        nv_set ( "lockpin_status", "" );
        CLOGD ( WARNING, "AT+CLCK=SC,2 return ERROR !\n" );
    }

    return;
}

void parsing_tdtek_cpinr ( char* data )
{
    char regex_buf[3][REGEX_BUF_ONE];
    int ret = 0;
    char *pattern = NULL;

    pattern = "\\^CPIN:[^,]*,[^,]*,([0-9]*),([0-9]*),.*";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );
    memset ( regex_buf[2], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "^CPIN:" ), pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( "pin_times", regex_buf[2] );
        nv_set ( "puk_times", regex_buf[1] );
    }
    else
    {
        nv_set ( "pin_times", "3" );
        nv_set ( "puk_times", "10" );
        CLOGD ( WARNING, "get PinPukTimes failed !\n" );
    }

    return;
}

int parsing_tdtek_cereg ( char* data )
{
    char regex_buf[2][REGEX_BUF_ONE];
    int ret = 0;
    char *pattern = NULL;

    pattern = "\\+CEREG: [0-9]*,([0-9]*).*";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "+CEREG:" ), pattern, regex_buf );
    if ( ret == 0 )
    {
        nv_set ( "cereg_stat", regex_buf[1] );
    }
    else
    {
        CLOGD ( WARNING, "AT+CEREG? regex error !\n" );
    }

    return ret;
}

int parsing_tdtek_c5greg ( char* data )
{
    char regex_buf[2][REGEX_BUF_ONE];
    int ret = 0;
    char *pattern = NULL;

    pattern = "\\+C5GREG: [0-9]*,([0-9]*)";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr(data, "+C5GREG:"), pattern, regex_buf );
    if ( 0 == ret )
    {
        nv_set ( "c5greg_stat", regex_buf[1] );
    }
    else
    {
        CLOGD ( FINE, "AT+C5GREG? return error!\n" );
    }

    return ret;
}

int parsing_tdtek_cireg ( char* data )
{
    char regex_buf[2][REGEX_BUF_ONE];
    int ret = 0;
    char *pattern = NULL;

    pattern = "\\+CIREG: ([0-9]*),.*";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "+CIREG:" ), pattern, regex_buf );
    if ( 0 == ret )
    {
        if ( 0 == strcmp ( regex_buf[1], "1" ) )
        {
            system ( "echo Registered > /tmp/voice/voice-reg-status.txt" );
        }
        else
        {
            system ( "echo Unregistered > /tmp/voice/voice-reg-status.txt" );
        }
    }
    else
    {
        CLOGD ( FINE, "AT+CIREG? return invalid !\n" );
    }

    return ret;
}

int parsing_tdtek_csq ( char* data )
{
    int i = 0;
    int cesq_param[2] = { 99, 99 };
    char strRamOpt[16] = {0};
    char cesq_val[64] = {0};

    if ( NULL == strstr ( data, "+CSQ:" ) )
        return -1;

    sscanf ( strstr ( data, "+CSQ:" ), "+CSQ: %d,%d\r\n", &cesq_param[0], &cesq_param[1] );

    snprintf ( cesq_val, sizeof ( cesq_val ), "%d,%d", cesq_param[0], cesq_param[1] );

    nv_set ( "csq", cesq_val );

    for ( i = 0; i < 2; i++ )
    {
        snprintf ( strRamOpt, sizeof ( strRamOpt ), "csq_param%d", i + 1 );
        snprintf ( cesq_val, sizeof ( cesq_val ), "%d", cesq_param[i] );
        nv_set ( strRamOpt, cesq_val );
    }

    return 0;
}

int parsing_tdtek_cgatt ( char* data )
{
    char regex_buf[2][REGEX_BUF_ONE];
    int ret = 0;
    char *pattern = NULL;

    pattern = "\\+CGATT: ([0-9]*).*";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "+CGATT:" ), pattern, regex_buf );
    if ( ret == 0 )
    {
        nv_set ( "cgatt_val", regex_buf[1] );
    }
    else
    {
        CLOGD ( FINE, "AT+CGATT? return error!\n" );
    }

    return ret;
}

void parsing_tdtek_cgact ( char* data )
{
    if ( strstr ( data, "^DCONNSTAT: 1," ) )
        nv_set ( "cid_1_state", "1" );
    else
        nv_set ( "cid_1_state", "0" );

    if ( strstr ( data, "^DCONNSTAT: 2," ) )
        nv_set ( "cid_2_state", "1" );
    else
        nv_set ( "cid_2_state", "0" );

    if ( strstr ( data, "^DCONNSTAT: 3," ) )
        nv_set ( "cid_3_state", "1" );
    else
        nv_set ( "cid_3_state", "0" );

    if ( strstr ( data, "^DCONNSTAT: 4," ) )
        nv_set ( "cid_4_state", "1" );
    else
        nv_set ( "cid_4_state", "0" );
}

static void clear_tdtek_4g_cellinfo ()
{
    nv_set( "mcc", "--" );
    nv_set( "mnc", "--" );
    nv_set( "tac", "--" );
    nv_set( "globalid", "--" );
    nv_set( "cellid", "--" );
    nv_set( "eNBid", "--" );
    nv_set( "dl_earfcn", "--" );
    nv_set( "ul_earfcn", "--" );
    nv_set( "dl_frequency", "--" );
    nv_set( "ul_frequency", "--" );
    nv_set( "pci", "--" );
    nv_set( "band", "--" );
    nv_set( "bandwidth", "--" );
    nv_set( "sinr", "--" );
    nv_set( "cqi", "--" );
    nv_set( "rxlev", "--" );
    nv_set( "rsrp0", "--" );
    nv_set( "rsrp1", "--" );
    nv_set( "rsrq", "--" );
    nv_set( "rssi", "--" );
    nv_set( "txpower", "--" );
}

static void clear_tdtek_5g_cellinfo ()
{
    nv_set( "5g_mcc", "--" );
    nv_set( "5g_mnc", "--" );
    nv_set( "5g_tac", "--" );
    nv_set( "5g_globalid", "--" );
    nv_set( "5g_cellid", "--" );
    nv_set( "5g_eNBid", "--" );
    nv_set( "5g_dl_earfcn", "--" );
    nv_set( "5g_ul_earfcn", "--" );
    nv_set( "5g_dl_frequency", "--" );
    nv_set( "5g_ul_frequency", "--" );
    nv_set( "5g_dl_earfcn_ssb", "--" );
    nv_set( "5g_ul_earfcn_ssb", "--" );
    nv_set( "5g_dl_frequency_ssb", "--" );
    nv_set( "5g_ul_frequency_ssb", "--" );
    nv_set( "5g_pci", "--" );
    nv_set( "5g_band", "--" );
    nv_set( "5g_bandwidth", "--" );
    nv_set( "5g_sinr", "--" );
    nv_set( "5g_rxlev", "--" );
    nv_set( "5g_rsrp0", "--" );
    nv_set( "5g_rsrp1", "--" );
    nv_set( "5g_rsrq", "--" );
    nv_set( "5g_rssi", "--" );
    nv_set( "5g_txpower", "--" );
}

static int parsing_tdtek_4g_cellinfo ( char* data )
{
    CLOGD ( FINE, "parsing 4G -> [%s]\n", data );

    char regex_buf[6][REGEX_BUF_ONE];
    int ret = 0;
    int i = 0;
    char *pattern = NULL;
    char strRamVal[32] = {0};

    for ( i = 0; i < 6; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    pattern = "\\^MONSC: LTE,([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),.*$";

    ret = at_tok_regular_more ( data, pattern, regex_buf );

    if ( 0 == ret )
    {
        for ( i = 1; i < 6; i++ )
            CLOGD ( FINE, "[%s]\n", regex_buf[i] );

        snprintf ( strRamVal, sizeof ( strRamVal ), "%03d", atoi ( regex_buf[1] ) );
        nv_set ( "mcc", strRamVal );
        nv_set ( "mnc", regex_buf[2] );

        // regex_buf[3] is earfcn

        sscanf ( regex_buf[4], "%x", &i );
        snprintf ( strRamVal, sizeof ( strRamVal ), "%d", i );
        nv_set ( "globalid", strRamVal );
        snprintf ( strRamVal, sizeof ( strRamVal ), "%d", i & 0x000000FF );
        nv_set ( "cellid", strRamVal );
        snprintf ( strRamVal, sizeof ( strRamVal ), "%d", i >> 8 );
        nv_set ( "eNBid", strRamVal );

        sscanf ( regex_buf[5], "%x", &i );
        snprintf ( strRamVal, sizeof ( strRamVal ), "%d", i );
        nv_set ( "pci", strRamVal );
    }
    else
    {
        clear_tdtek_4g_cellinfo ();
        return ret;
    }

    for ( i = 0; i < 6; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    pattern = "\\^MONSC: LTE,[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,([^,]*),([^,]*),([^,]*),([-0-9]*).*$";

    ret = at_tok_regular_more ( data, pattern, regex_buf );

    if ( 0 == ret )
    {
        for ( i = 1; i < 5; i++ )
            CLOGD ( FINE, "[%s]\n", regex_buf[i] );

        sscanf ( regex_buf[1], "%x", &i );
        snprintf ( strRamVal, sizeof ( strRamVal ), "%d", i );
        nv_set ( "tac", strRamVal );

        nv_set ( "rsrp0", regex_buf[2] );
        nv_set ( "rsrp1", regex_buf[2] );
        nv_set ( "rsrq", regex_buf[3] );
        nv_set ( "rssi", regex_buf[4] );
    }
    else
    {
        clear_tdtek_4g_cellinfo ();
        return ret;
    }

    return 0;
}

static int parsing_tdtek_5g_cellinfo ( char* data )
{
    CLOGD ( FINE, "parsing 5G -> [%s]\n", data );

    char regex_buf[6][REGEX_BUF_ONE];
    int ret = 0;
    int i = 0;
    char *pattern = NULL;
    char strRamVal[32] = {0};
    unsigned long long globalid_5g = 0;

    for ( i = 0; i < 6; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    pattern = "\\^MONSC: NR,([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),.*$";

    ret = at_tok_regular_more ( data, pattern, regex_buf );

    if ( 0 == ret )
    {
        for ( i = 1; i < 6; i++ )
            CLOGD ( FINE, "[%s]\n", regex_buf[i] );

        snprintf ( strRamVal, sizeof ( strRamVal ), "%03d", atoi ( regex_buf[1] ) );
        nv_set ( "5g_mcc", strRamVal );
        nv_set ( "5g_mnc", regex_buf[2] );

        // regex_buf[3] is ssb narfcn
        nv_set ( "5g_dl_earfcn_ssb", regex_buf[3] );

        earfcn_freq.dl_earfcn = atoi ( regex_buf[3] );
        if ( 0 == calc_5g_freq_from_narfcn ( &earfcn_freq ) )
        {
            snprintf ( strRamVal, sizeof ( strRamVal ), "%d", earfcn_freq.ul_earfcn );
            nv_set ( "5g_ul_earfcn_ssb", strRamVal );
            snprintf ( strRamVal, sizeof ( strRamVal ), "%d.%03d", earfcn_freq.dl_frequency / 1000, earfcn_freq.dl_frequency % 1000 );
            nv_set ( "5g_dl_frequency_ssb", strRamVal );
            snprintf ( strRamVal, sizeof ( strRamVal ), "%d.%03d", earfcn_freq.ul_frequency / 1000, earfcn_freq.ul_frequency % 1000 );
            nv_set ( "5g_ul_frequency_ssb", strRamVal );
        }

        switch ( atoi ( regex_buf[4] ) )
        {
        case 0:
            nv_set ( "5g_scs", "15" );    // KHz
            break;
        case 1:
            nv_set ( "5g_scs", "30" );    // KHz
            break;
        case 2:
            nv_set ( "5g_scs", "60" );    // KHz
            break;
        case 3:
            nv_set ( "5g_scs", "120" );    // KHz
            break;
        case 4:
            nv_set ( "5g_scs", "240" );    // KHz
            break;
        }

        sscanf ( regex_buf[5], "%llx", &globalid_5g );
        snprintf ( strRamVal, sizeof ( strRamVal ), "%lld", globalid_5g );
        nv_set ( "5g_globalid", strRamVal );
        snprintf ( strRamVal, sizeof ( strRamVal ), "%lld", globalid_5g & 0x000000FF );
        nv_set ( "5g_cellid", strRamVal );
        snprintf ( strRamVal, sizeof ( strRamVal ), "%lld", globalid_5g >> 14 );
        nv_set ( "5g_eNBid", strRamVal );
    }
    else
    {
        clear_tdtek_5g_cellinfo ();
        return ret;
    }

    for ( i = 0; i < 6; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    pattern = "\\^MONSC: NR,[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,([^,]*),([^,]*),([^,]*),([^,]*),([-0-9]*).*$";

    ret = at_tok_regular_more ( data, pattern, regex_buf );

    if ( 0 == ret )
    {
        for ( i = 1; i < 6; i++ )
            CLOGD ( FINE, "[%s]\n", regex_buf[i] );

        sscanf ( regex_buf[1], "%x", &i );
        snprintf ( strRamVal, sizeof ( strRamVal ), "%d", i );
        nv_set ( "5g_pci", strRamVal );

        sscanf ( regex_buf[2], "%x", &i );
        snprintf ( strRamVal, sizeof ( strRamVal ), "%d", i );
        nv_set ( "5g_tac", strRamVal );

        nv_set ( "5g_rsrp0", regex_buf[3] );
        nv_set ( "5g_rsrp1", regex_buf[3] );
        nv_set ( "5g_rsrq", regex_buf[4] );
        nv_set ( "5g_sinr", regex_buf[5] );
    }
    else
    {
        clear_tdtek_5g_cellinfo ();
        return ret;
    }

    nv_set ( "5g_rssi", "--" );

    return 0;
}

void parsing_tdtek_serving_cellinfo ( char* data )
{
    CLOGD ( FINE, "start servingcell parsing ...\n" );

    char *info_line = NULL;
    int lte_info_flag = 0;
    int nr_info_flag = 0;
    char net_mode[8] = {0};

    strcpy ( net_mode, "--" );

    info_line = strtok ( data, "\r\n" );
    while ( info_line )
    {
        CLOGD ( FINE, "Get line:\n%s\n\n", info_line );

        if ( strstr ( info_line, "^MONSC: NR" ) )
        {
            nr_info_flag = 1;
            strcpy ( net_mode, "5G" );
            parsing_tdtek_5g_cellinfo ( info_line );
        }
        else if ( strstr ( info_line, "^MONSC: LTE" ) )
        {
            lte_info_flag = 1;
            strcpy ( net_mode, "4G" );
            parsing_tdtek_4g_cellinfo ( info_line );
        }

        info_line = strtok ( NULL, "\r\n" );
    }

    nv_set ( "mode", net_mode );

    if ( 0 == lte_info_flag )
    {
        clear_tdtek_4g_cellinfo ();
    }

    if ( 0 == nr_info_flag )
    {
        clear_tdtek_5g_cellinfo ();
    }
}

int parsing_tdtek_neighbor_cellinfo ( char* data )
{
    char regex_buf[7][REGEX_BUF_ONE];
    char strRamOpt[32] = {0};
    char strRamVal[32] = {0};
    int neighbor_count = 0;
    int i = 0;
    int ignore_first_cell = 0;
    int ret = 0;
    char *pattern = NULL;
    char *neighbor_content = NULL;

    pattern = "\\^MONNC:[ ]*([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),(.*)";

    neighbor_content = strtok ( data, "\r\n" );
    while ( neighbor_content )
    {
        CLOGD ( FINE, "Get line:\n%s\n\n", neighbor_content );
        if ( NULL == strstr ( neighbor_content, "^MONNC: NONE" ) && NULL != strstr ( neighbor_content, "^MONNC:" ) )
        {
            if ( 1 == ignore_first_cell ) // first cell is serving cell
            {
                ignore_first_cell = 0;
                goto NEXT;
            }

            for ( i = 0; i < 7; i++ )
            {
                memset ( regex_buf[i], 0, REGEX_BUF_ONE );
            }
            ret = at_tok_regular_more ( neighbor_content, pattern, regex_buf );
            if ( 0 == ret )
            {
                CLOGD ( FINE, "neighbor_count -> [%d]\n", ++neighbor_count );

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "neighbor_mode_%d", neighbor_count );
                nv_set ( strRamOpt, regex_buf[1] );

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "neighbor_earfcn_%d", neighbor_count );
                nv_set ( strRamOpt, regex_buf[2] );

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "neighbor_pci_%d", neighbor_count );
                snprintf ( strRamVal, sizeof ( strRamVal ), "%ld", strtol ( regex_buf[3] , NULL, 16 ) );
                nv_set ( strRamOpt, strRamVal );

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "neighbor_rsrp_%d", neighbor_count );
                nv_set ( strRamOpt, regex_buf[4] );

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "neighbor_rsrq_%d", neighbor_count );
                nv_set ( strRamOpt, regex_buf[5] );

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "neighbor_rssi_%d", neighbor_count );
                nv_set ( strRamOpt, strcmp ( regex_buf[1], "NR" ) ? regex_buf[6] : "--" );

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "neighbor_sinr_%d", neighbor_count );
                nv_set ( strRamOpt, strcmp ( regex_buf[1], "NR" ) ? "--" : regex_buf[6] );
            }
        }

NEXT:
        neighbor_content = strtok ( NULL, "\r\n" );
    }

    snprintf ( strRamVal, sizeof ( strRamVal ), "%d", neighbor_count );
    nv_set ( "neighbor_count", strRamVal );

    return neighbor_count;
}

/*
 * +CCLK: "20/11/30,12:03:38+32"
 *      year/mon/day,hour:min:sec(+-)zone
 */
void parsing_tdtek_cclk ( char* data )
{
    unsigned int time_s[6] = {0};
    unsigned int time_zone = 0;
    char zone_type[4] = {0};
    char sCurrentTime[32] = {0};
    time_t gmt;
    struct tm *SwitchLocalTime;
    int ret = 0;
    char *pattern = NULL;
    char regex_buf[2][REGEX_BUF_ONE];

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    pattern = "\\+CCLK: \"([^\"\r\n]*)\".*";

    ret = at_tok_regular_more ( strstr ( data, "+CCLK:" ), pattern, regex_buf );

    if ( 0 != ret )
    {
        CLOGD ( WARNING, "AT+CCLK? regex error !\n" );
        return;
    }

    CLOGD ( FINE, "NITZ_INFO -> [%s]\n", regex_buf[1] );

    sscanf ( regex_buf[1], "%u/%u/%u,%u:%u:%u%[-+]%u",
                            &time_s[0], &time_s[1], &time_s[2],
                            &time_s[3], &time_s[4], &time_s[5],
                            zone_type, &time_zone
                    );

    CLOGD ( FINE, "[%u] [%u] [%u] [%u] [%u] [%u] [%s] [%u]\n",
                    time_s[0], time_s[1], time_s[2],
                    time_s[3], time_s[4], time_s[5],
                    zone_type, time_zone
                );

    if ( 50 < time_s[0] || 0 == strcmp ( zone_type, "" ) )
    {
        nv_set ( "date_time", "" );
        CLOGD ( WARNING, "NITZ time is unreasonable ...\n" );
        return;
    }

    gmt = nitz_time_total_second (
                time_s[0] + 2000, time_s[1], time_s[2],
                time_s[3], time_s[4], time_s[5]
            );

#if 0
    /*
     * QUECTEL module AT+CCLK? return ( UTC_time & time_zone )
     *
     * for example, BeiJing local time is [2020.12.01,18:30:45]
     * then AT+CCLK? will return "20/12/01,10:30:45+32"
     * local_time = UTC_time +/- ( time_zone * 15 * 60 )
     *
     * 1, if use this code:
     *      date_time will be local time.
     *      you need write tm_zone to /tmp/TZ firstly.
     *      and then exe 'date -s date_time' to update local time.
     *
     * 2, if not use this code:
     *      date_time will be UTC time.
     *      you need exe 'date -s date_time -u' to update local time.
     *      but need not write tm_zone to /tmp/TZ.
     */
    if ( '+' == zone_type[0] )
    {
        gmt += ( time_zone * 15 * 60 );
    }
    else if ( '-' == zone_type[0] )
    {
        gmt -= ( time_zone * 15 * 60 );
    }
#endif

    SwitchLocalTime = gmtime ( &gmt );
    snprintf ( sCurrentTime, sizeof ( sCurrentTime ), "%4d-%02d-%02d %02d:%02d:%02d",
                ( SwitchLocalTime->tm_year + 1900 ), ( SwitchLocalTime->tm_mon + 1 ),
                SwitchLocalTime->tm_mday, SwitchLocalTime->tm_hour,
                SwitchLocalTime->tm_min, SwitchLocalTime->tm_sec
        );

    if ( 2024 > ( SwitchLocalTime->tm_year + 1900 ) )
    {
        nv_set ( "date_time", "" );
    }
    else
    {
        nv_set( "date_time", sCurrentTime );
    }

    return;
}

// 930E790A -> 0A.79.0E.93 -> 10.121.14.147
static void tdtek_hexchar2ipaddr ( char* in_hex, char* out_ip, int out_len )
{
    unsigned int ip_parts[4];

    if ( 8 != strlen ( in_hex ) )
    {
        return;
    }

    sscanf ( in_hex, "%2x%2x%2x%2x", &ip_parts[3], &ip_parts[2], &ip_parts[1], &ip_parts[0] );

    snprintf ( out_ip, out_len, "%d.%d.%d.%d", ip_parts[0], ip_parts[1], ip_parts[2], ip_parts[3] );
}

//           IP      MASK      GW      DHCPS    DNS1     DNS2     M_RX b/s   M_TX b/s
// ^DHCP: 930E790A,000000FF,0100000A,0100000A,FFB84170,035416D2, 600000000, 100000000
void parsing_tdtek_dhcpv4_info ( int cid_index, char* data )
{
    static char ipv4_addr[4][32];
    int i = 0, ret = 0;
    char strRamOpt[32] = {0};
    char ipv4_temp[32] = {0};
    char ipv4_mask[32] = {0};
    char ipv4_gway[32] = {0};
    char *pattern = NULL;
    char *ipv4_content = NULL;
    char regex_buf[5][REGEX_BUF_ONE];

    ipv4_content = strstr ( data, "^DHCP:" );

    if ( ipv4_content )
    {
        pattern = "\\^DHCP:[ ]*([^,]*),[^,]*,[^,]*,[^,]*,[ ]*([^,]*),[ ]*([^,]*),.*";

        for ( i = 0; i < 5; i++ )
        {
            memset ( regex_buf[i], 0, REGEX_BUF_ONE );
        }

        ret = at_tok_regular_more ( ipv4_content, pattern, regex_buf );
        if ( 0 == ret )
        {
            tdtek_hexchar2ipaddr ( regex_buf[1], ipv4_temp, sizeof ( ipv4_temp ) );
            snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ip", cid_index );
            nv_set ( strRamOpt, ipv4_temp );

            if ( strcmp ( ipv4_addr [ cid_index - 1 ], ipv4_temp ) )
            {
                snprintf ( ipv4_addr [ cid_index - 1 ], sizeof ( ipv4_addr[0] ), "%s", ipv4_temp );
                if ( 1 == apns_msg_flag [ cid_index ] )
                {
                    apns_msg_flag [ cid_index ] = 0;
                }
            }

            calc_x55_mask_gw_from_ip ( ipv4_temp, ipv4_mask, ipv4_gway );

            snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_mask", cid_index );
            nv_set ( strRamOpt, ipv4_mask );

            snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_gateway", cid_index );
            nv_set ( strRamOpt, ipv4_gway );

            tdtek_hexchar2ipaddr ( regex_buf[2], ipv4_temp, sizeof ( ipv4_temp ) );
            snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_dns1", cid_index );
            nv_set ( strRamOpt, ipv4_temp );

            tdtek_hexchar2ipaddr ( regex_buf[3], ipv4_temp, sizeof ( ipv4_temp ) );
            snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_dns2", cid_index );
            nv_set ( strRamOpt, ipv4_temp );

            snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_state", cid_index );
            nv_set ( strRamOpt, "connect" );

            return;
        }
    }

    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ip", cid_index );
    nv_set ( strRamOpt, "--" );
    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_mask", cid_index );
    nv_set ( strRamOpt, "--" );
    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_gateway", cid_index );
    nv_set ( strRamOpt, "--" );
    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_dns1", cid_index );
    nv_set ( strRamOpt, "--" );
    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_dns2", cid_index );
    nv_set ( strRamOpt, "--" );
    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_state", cid_index );
    nv_set ( strRamOpt, "disconnect" );

    memset ( ipv4_addr [ cid_index - 1 ], 0, sizeof ( ipv4_addr[0] ) );
}

//                          IPV6                                     DNS1               DNS2           M_RX b/s   M_TX b/s
// ^DHCPV6: 2408:840d:1300:30d2:c1dc:a001:2b0f:67f8,::,::,::,2408:8888:0:8888::8,2408:8899:0:8899::8, 600000000, 100000000
void parsing_tdtek_dhcpv6_info ( int cid_index, char* data )
{
    static char ipv6_addr[4][64];
    int i = 0, ret = 0;
    char strRamOpt[32] = {0};
    char *pattern = NULL;
    char *ipv6_content = NULL;
    char regex_buf[5][REGEX_BUF_ONE];

    ipv6_content = strstr ( data, "^DHCPV6:" );

    if ( ipv6_content )
    {
        pattern = "\\^DHCPV6:[ ]*([^,]*),[^,]*,[^,]*,[^,]*,[ ]*([^,]*),[ ]*([^,]*),.*";

        for ( i = 0; i < 5; i++ )
        {
            memset ( regex_buf[i], 0, REGEX_BUF_ONE );
        }

        ret = at_tok_regular_more ( ipv6_content, pattern, regex_buf );
        if ( 0 == ret )
        {
             if ( strcmp ( ipv6_addr [ cid_index - 1 ], regex_buf[1] ) )
             {
                 snprintf ( ipv6_addr [ cid_index - 1 ], sizeof ( ipv6_addr[0] ), "%s", regex_buf[1] );
                 if ( 1 == apns_msg_flag [ cid_index ] )
                 {
                     apns_msg_flag [ cid_index ] = 0;
                 }
             }

             snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_ip", cid_index );
             nv_set ( strRamOpt, regex_buf[1] );
             snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_mask", cid_index );
             nv_set ( strRamOpt, "FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FF00" );
             snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_gateway", cid_index );
             nv_set ( strRamOpt, "fe80::2" );
             snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_dns1", cid_index );
             nv_set ( strRamOpt, regex_buf[2] );
             snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_dns2", cid_index );
             nv_set ( strRamOpt, regex_buf[3] );
             snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_state", cid_index );
             nv_set ( strRamOpt, "connect" );

             return;
        }
    }

    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_ip", cid_index );
    nv_set ( strRamOpt, "--" );
    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_mask", cid_index );
    nv_set ( strRamOpt, "--" );
    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_gateway", cid_index );
    nv_set ( strRamOpt, "--" );
    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_dns1", cid_index );
    nv_set ( strRamOpt, "--" );
    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_dns2", cid_index );
    nv_set ( strRamOpt, "--" );
    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_state", cid_index );
    nv_set ( strRamOpt, "disconnect" );

    memset ( ipv6_addr [ cid_index - 1 ], 0, sizeof ( ipv6_addr[0] ) );
}

void parsing_tdtek_rrc_state ( char* data )
{
    char regex_buf[2][REGEX_BUF_ONE];
    int ret = 0;
    char *pattern = NULL;

    pattern = "\\^RRCSTAT:[ ]*[^,]*,([0-9]*).*";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "^RRCSTAT:" ), pattern, regex_buf );
    if ( ret == 0 )
    {
        switch ( atoi ( regex_buf[1] ) )
        {
        case 0: // idle
        case 2: // inactive
            nv_set ( "rrc_state", "IDLE" );
            break;
        case 1: // connected
            nv_set ( "rrc_state", "CONNECTED" );
            break;
        default:
            nv_set ( "rrc_state", "--" );
            break;
        }
    }
    else
    {
        CLOGD ( WARNING, "AT^RRCSTAT? return invalid !\n" );
    }
}

void parsing_tdtek_hfreqinfo ( char* data )
{
    char regex_buf[4][REGEX_BUF_ONE];
    int i = 0, ret = 0;
    int mode = 6; // 4G
    char tmp_val[16] = {0};
    char *pattern = NULL;

    pattern = "\\^HFREQINFO:[ ]*[^,]*,([^,]*),([^,]*),.*";
    for ( i = 0; i < 4; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );
    ret = at_tok_regular_more ( strstr ( data, "^HFREQINFO:" ), pattern, regex_buf );
    if ( 0 == ret )
    {
        if ( 0 == strcmp ( regex_buf[1], "7" ))
        {
            mode = 7;   // 5G
            nv_set ( "5g_band", regex_buf[2] );
            earfcn_freq.band = atoi ( regex_buf[2] );
        }
        else
        {
            nv_set ( "band", regex_buf[2] );
        }
    }
    else
    {
        CLOGD ( WARNING, "AT^HFREQINFO? return invalid P1 !\n" );
    }

    pattern = "\\^HFREQINFO:[ ]*[^,]*,[^,]*,[^,]*,([^,]*),([^,]*),([^,]*),.*";
    for ( i = 0; i < 4; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );
    ret = at_tok_regular_more ( strstr ( data, "^HFREQINFO:" ), pattern, regex_buf );
    if ( 0 == ret )
    {
        if ( 6 == mode )
        {
            nv_set ( "dl_earfcn", regex_buf[1] );

            snprintf (tmp_val , sizeof ( tmp_val ), "%d.%d",
                                    atoi ( regex_buf[2] ) / 10, atoi ( regex_buf[2] ) % 10 );
            nv_set ( "dl_frequency", tmp_val );

            snprintf ( tmp_val, sizeof ( tmp_val ), "%d", atoi ( regex_buf[3] ) / 1000 );
            if ( 0 == strcmp ( tmp_val, "1" ) )
            {
                snprintf ( tmp_val, sizeof ( tmp_val ), "%s", "1.4" );
            }
            nv_set ( "bandwidth", tmp_val );
            nv_set ( "dl_bandwidth", tmp_val );
        }
        else
        {
            nv_set ( "5g_dl_earfcn", regex_buf[1] );

            snprintf (tmp_val , sizeof ( tmp_val ), "%d.%03d",
                                    atoi ( regex_buf[2] ) / 1000, atoi ( regex_buf[2] ) % 1000 );
            nv_set ( "5g_dl_frequency", tmp_val );

            snprintf ( tmp_val, sizeof ( tmp_val ), "%d", atoi ( regex_buf[3] ) / 1000 );
            if ( 0 == strcmp ( tmp_val, "1" ) )
            {
                snprintf ( tmp_val, sizeof ( tmp_val ), "%s", "1.4" );
            }
            nv_set ( "5g_bandwidth", tmp_val );
            nv_set ( "5g_dl_bandwidth", tmp_val );
        }
    }
    else
    {
        CLOGD ( WARNING, "AT^HFREQINFO? return invalid P2 !\n" );
    }

    pattern = "\\^HFREQINFO:[ ]*[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,([^,]*),([^,]*),([0-9]*).*";
    for ( i = 0; i < 4; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );
    ret = at_tok_regular_more ( strstr ( data, "^HFREQINFO:" ), pattern, regex_buf );
    if ( 0 == ret )
    {
        if ( 6 == mode )
        {
            nv_set ( "ul_earfcn", regex_buf[1] );

            snprintf (tmp_val , sizeof ( tmp_val ), "%d.%d",
                                    atoi ( regex_buf[2] ) / 10, atoi ( regex_buf[2] ) % 10 );
            nv_set ( "ul_frequency", tmp_val );

            snprintf ( tmp_val, sizeof ( tmp_val ), "%d", atoi ( regex_buf[3] ) / 1000 );
            if ( 0 == strcmp ( tmp_val, "1" ) )
            {
                snprintf ( tmp_val, sizeof ( tmp_val ), "%s", "1.4" );
            }
            nv_set ( "ul_bandwidth", tmp_val );
        }
        else
        {
            nv_set ( "5g_ul_earfcn", regex_buf[1] );

            snprintf (tmp_val , sizeof ( tmp_val ), "%d.%03d",
                                    atoi ( regex_buf[2] ) / 1000, atoi ( regex_buf[2] ) % 1000 );
            nv_set ( "5g_ul_frequency", tmp_val );

            snprintf ( tmp_val, sizeof ( tmp_val ), "%d", atoi ( regex_buf[3] ) / 1000 );
            if ( 0 == strcmp ( tmp_val, "1" ) )
            {
                snprintf ( tmp_val, sizeof ( tmp_val ), "%s", "1.4" );
            }
            nv_set ( "5g_ul_bandwidth", tmp_val );
        }
    }
    else
    {
        CLOGD ( WARNING, "AT^HFREQINFO? return invalid P3 !\n" );
    }
}

void parsing_tdtek_hcsq ( char* data )
{
    char regex_buf[3][REGEX_BUF_ONE];
    int ret = 0;
    char tmp_val[4] = {0};
    char *pattern = NULL;

    pattern = "\\^HCSQ: \"(.*)\",[^,]*,[^,]*,([-0-9]*).*";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );
    memset ( regex_buf[2], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "^HCSQ:" ), pattern, regex_buf );
    if ( 0 == ret )
    {
        if ( 0 == strcmp ( regex_buf[1], "LTE" ) )
        {
            snprintf ( tmp_val, sizeof ( tmp_val ), "%d", ( atoi ( regex_buf[2] ) - 101 ) / 5 );
            nv_set ( "sinr", tmp_val );
        }
    }
    else
    {
        CLOGD ( WARNING, "AT^HCSQ? return invalid !\n" );
    }
}

void parsing_tdtek_ulmcs ( char* data )
{
    char regex_buf[3][REGEX_BUF_ONE];
    int ret = 0;
    int code0_mcs = 0;
    int code1_mcs = 0;
    char tmp_mcs[4] = {0};
    char *pattern = NULL;

    pattern = "\\^MCS:[ ]*[^,]*,[^,]*,[^,]*,([0-9]*),([0-9]*).*";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );
    memset ( regex_buf[2], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "^MCS:" ), pattern, regex_buf );
    if ( 0 != ret )
    {
        CLOGD ( WARNING, "AT^MCS=0 return invalid !\n" );
    }

    ret = atoi ( regex_buf[1] );
    if ( 0 <= ret && ret <= 31 )
    {
        code0_mcs = ret;
    }

    ret = atoi ( regex_buf[2] );
    if ( 0 <= ret && ret <= 31 )
    {
        code1_mcs = ret;
    }

    snprintf ( tmp_mcs, sizeof ( tmp_mcs ), "%d", code0_mcs > code1_mcs ? code0_mcs : code1_mcs );

    nv_set ( "ulmcs", tmp_mcs );
    nv_set ( "5g_ulmcs", tmp_mcs );
}

void parsing_tdtek_dlmcs ( char* data )
{
    char regex_buf[3][REGEX_BUF_ONE];
    int ret = 0;
    int code0_mcs = 0;
    int code1_mcs = 0;
    char tmp_mcs[4] = {0};
    char *pattern = NULL;

    pattern = "\\^MCS:[ ]*[^,]*,[^,]*,[^,]*,([0-9]*),([0-9]*).*";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );
    memset ( regex_buf[2], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "^MCS:" ), pattern, regex_buf );
    if ( 0 != ret )
    {
        CLOGD ( WARNING, "AT^MCS=1 return invalid !\n" );
    }

    ret = atoi ( regex_buf[1] );
    if ( 0 <= ret && ret <= 31 )
    {
        code0_mcs = ret;
    }

    ret = atoi ( regex_buf[2] );
    if ( 0 <= ret && ret <= 31 )
    {
        code1_mcs = ret;
    }

    snprintf ( tmp_mcs, sizeof ( tmp_mcs ), "%d", code0_mcs > code1_mcs ? code0_mcs : code1_mcs );

    nv_set ( "dlmcs", tmp_mcs );
    nv_set ( "5g_dlmcs", tmp_mcs );
}

void parsing_tdtek_4g_txpower ( char* data )
{
    char regex_buf[2][REGEX_BUF_ONE];
    int ret = 0;
    char *pattern = NULL;

    pattern = "\\^TXPOWER:[ ]*[^,]*,([^,]*),.*";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "^TXPOWER:" ), pattern, regex_buf );
    if ( ret == 0 )
    {
        nv_set ( "txpower", regex_buf[1] );
    }
    else
    {
        CLOGD ( WARNING, "AT^TXPOWER? return invalid !\n" );
    }
}

void parsing_tdtek_5g_txpower ( char* data )
{
    char regex_buf[2][REGEX_BUF_ONE];
    int ret = 0;
    char *pattern = NULL;

    pattern = "\\^NTXPOWER:[ ]*([^,]*),.*";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "^NTXPOWER:" ), pattern, regex_buf );
    if ( ret == 0 )
    {
        nv_set ( "5g_txpower", regex_buf[1] );
    }
    else
    {
        CLOGD ( WARNING, "AT^NTXPOWER? return invalid !\n" );
    }
}

/*
 * ^NWTIME: 24/05/16,09:21:09+32,00
 *        year/mon/day,hour:min:sec(+-)zone,summer
 */
void parsing_tdtek_nwtime ( char* data )
{
    unsigned int time_s[6] = {0};
    unsigned int time_zone = 0;
    char zone_type[4] = {0};
    char sCurrentTime[32] = {0};
    char ntp_updated[32] = {0};
    char date_cmd_buf[128] = {0};
    time_t gmt;
    struct tm *SwitchLocalTime;
    int ret = 0;
    char *pattern = NULL;
    char regex_buf[2][REGEX_BUF_ONE];

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    pattern = "\\^NWTIME: ([^\r\n]*).*";

    ret = at_tok_regular_more ( strstr ( data, "^NWTIME:" ), pattern, regex_buf );

    if ( 0 != ret )
    {
        CLOGD ( WARNING, "AT^NWTIME? regex error !\n" );
        return;
    }

    CLOGD ( FINE, "NITZ_INFO -> [%s]\n", regex_buf[1] );

    sscanf ( regex_buf[1], "%u/%u/%u,%u:%u:%u%[-+]%u",
                            &time_s[0], &time_s[1], &time_s[2],
                            &time_s[3], &time_s[4], &time_s[5],
                            zone_type, &time_zone
                    );

    CLOGD ( FINE, "[%u] [%u] [%u] [%u] [%u] [%u] [%s] [%u]\n",
                    time_s[0], time_s[1], time_s[2],
                    time_s[3], time_s[4], time_s[5],
                    zone_type, time_zone
                );

    if ( 50 < time_s[0] || 0 == strcmp ( zone_type, "" ) )
    {
        CLOGD ( WARNING, "NITZ time is unreasonable ...\n" );
        return;
    }

    gmt = nitz_time_total_second (
                time_s[0] + 2000, time_s[1], time_s[2],
                time_s[3], time_s[4], time_s[5]
            );

#if 0
    /*
     * AT^NWTIME? return ( UTC_time & time_zone )
     *
     * for example, BeiJing local time is [2020.12.01,18:30:45]
     * then AT^NWTIME? will return "20/12/01,10:30:45+32"
     * local_time = UTC_time +/- ( time_zone * 15 * 60 )
     *
     * 1, if use this code:
     *      date_time will be local time.
     *      you need write tm_zone to /tmp/TZ firstly.
     *      and then exe 'date -s date_time' to update local time.
     *
     * 2, if not use this code:
     *      date_time will be UTC time.
     *      you need exe 'date -s date_time -u' to update local time.
     *      but need not write tm_zone to /tmp/TZ.
     */
    if ( '+' == zone_type[0] )
    {
        gmt += ( time_zone * 15 * 60 );
    }
    else if ( '-' == zone_type[0] )
    {
        gmt -= ( time_zone * 15 * 60 );
    }
#endif

    SwitchLocalTime = gmtime ( &gmt );
    snprintf ( sCurrentTime, sizeof ( sCurrentTime ), "%4d-%02d-%02d %02d:%02d:%02d",
                ( SwitchLocalTime->tm_year + 1900 ), ( SwitchLocalTime->tm_mon + 1 ),
                SwitchLocalTime->tm_mday, SwitchLocalTime->tm_hour,
                SwitchLocalTime->tm_min, SwitchLocalTime->tm_sec
        );

    if ( 2024 <= ( SwitchLocalTime->tm_year + 1900 ) )
    {
        // Set time at UTC
        snprintf ( date_cmd_buf, sizeof ( date_cmd_buf ), "date -s \"%s\" -u", sCurrentTime );
        system ( date_cmd_buf );
        nv_get ( "ntp_updated", ntp_updated, sizeof ( ntp_updated ) );
        if ( strcmp ( ntp_updated, "1" ) )
        {
            nv_set ( "ntp_updated", "1" );
        }
        system ( "which hwclock && hwclock -w && hwclock --systz || date -k" );
    }

    return;
}

