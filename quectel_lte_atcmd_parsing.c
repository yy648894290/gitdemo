#include "comd_share.h"
#include "atchannel.h"
#include "at_tok.h"
#include "comd_ip_helper.h"
#include "quectel_lte_atcmd_parsing.h"
#include "config_key.h"
#include "hal_platform.h"

extern CELL_EARFCN_FREQ earfcn_freq;
extern int apns_msg_flag[5];

int parsing_quectel_lte_cfun_set ( char* data )
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

int parsing_quectel_lte_cfun_get ( char* data )
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

int parsing_quectel_lte_moduleModel ( char* data )
{
    char regex_buf[2][REGEX_BUF_ONE];
    int ret = 0;
    char *pattern = NULL;

    pattern = "\\Quectel[\r\n]*([^\r\n]*)[\r\n]*Revision:.*OK.*$";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "Quectel" ), pattern, regex_buf );
    if ( 0 == ret )
    {
        nv_set ( "modulemodel", regex_buf[1] );
    }
    else
    {
        nv_set ( "modulemodel", "" );
        CLOGD ( WARNING, "ATI return invalid !\n" );
    }

    return ret;
}

int parsing_quectel_lte_moduleVersion ( char* data )
{
    char regex_buf[2][REGEX_BUF_ONE];
    int ret = 0;
    char *pattern = NULL;

    pattern = "\\Revision:[ ]*([^\r\n]*).*OK.*$";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "Revision:" ), pattern, regex_buf );
    if ( 0 == ret )
    {
        nv_set ( "moduleversion", regex_buf[1] );
    }
    else
    {
        nv_set ( "moduleversion", "" );
        CLOGD ( WARNING, "ATI return invalid !\n" );
    }

    return ret;
}

int parsing_quectel_lte_imei ( char* data )
{
    char regex_buf[2][REGEX_BUF_ONE];
    int ret = 0;
    char *pattern = NULL;

    pattern = "\\+EGMR:[ ]*\"([0-9]*)\".*OK.*$";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "+EGMR:" ), pattern, regex_buf );
    if ( 0 == ret )
    {
        nv_set ( "imei", regex_buf[1] );
    }
    else
    {
        nv_set ( "imei", "" );
        CLOGD ( FINE, "AT+EGMR=0,7 return error !\n" );
    }

    return ret;
}

int parsing_quectel_lte_sn ( char* data )
{
    char regex_buf[2][REGEX_BUF_ONE];
    int ret = 0;
    char *pattern = NULL;

    pattern = "\\+EGMR:[ ]*\"(.*)\".*OK.*$";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "+EGMR:" ), pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( "SN", regex_buf[1] );
    }
    else
    {
        nv_set ( "SN", "" );
        CLOGD ( FINE, "AT+EGMR=0,5 return error !\n" );
    }

    return ret;
}

void parsing_quectel_lte_cpin_get ( char* data )
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

void parsing_quectel_lte_clck_get ( char* data )
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

void parsing_quectel_lte_qpinc ( char* data )
{
    char regex_buf[3][REGEX_BUF_ONE];
    int ret = 0;
    char *pattern = NULL;

    pattern = "\\+QPINC: \"SC\",([0-9]*),([0-9]*).*OK.*$";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );
    memset ( regex_buf[2], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "+QPINC: \"SC\"," ), pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( "pin_times", regex_buf[1] );
        nv_set ( "puk_times", regex_buf[2] );
    }
    else
    {
        nv_set ( "pin_times", "3" );
        nv_set ( "puk_times", "10" );
        CLOGD ( WARNING, "getPinPukTimes fail !\n" );
    }

    return;
}

void parsing_quectel_lte_cimi ( char* data )
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
        CLOGD ( FINE, "AT+CIMI? return error !\n" );
    }

    return;
}

void parsing_quectel_lte_iccid ( char* data )
{
    char regex_buf[3][REGEX_BUF_ONE];
    int i = 0;
    int ret = 0;
    char *pattern = NULL;

    pattern = "\\+CRSM: [0-9]*,[0-9]*,\"([^\"]*)\".*$";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );
    memset ( regex_buf[2], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "+CRSM:" ), pattern, regex_buf );
    if ( 0 == ret )
    {
        CLOGD ( FINE, "ICCID_org: [%s]\n", regex_buf[1] );
        for ( ; i < strlen ( regex_buf[1] ); i = i + 2 )
        {
            regex_buf[2][i] = regex_buf[1][i + 1];
            regex_buf[2][i + 1] = regex_buf[1][i];
        }
        CLOGD ( FINE, "ICCID_cal: [%s]\n", regex_buf[2] );
        nv_set ( "iccid", regex_buf[2] );
    }
    else
    {
        CLOGD ( FINE, "Fail to read iccid !\n" );
        nv_set ( "iccid", "" );
    }
}

void parsing_quectel_lte_cnum ( char* data )
{
    char regex_buf[2][REGEX_BUF_ONE];
    int ret = 0;
    char *pattern = NULL;

    pattern = "\\+CNUM:[^,]*,\"([^\"]*)\",.*$";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "+CNUM:" ), pattern, regex_buf );
    if ( 0 == ret )
    {
        CLOGD ( FINE, "PHONE_NUM: [%s]\n", regex_buf[1] );
        nv_set ( "phone_number", regex_buf[1] );
    }
    else
    {
        CLOGD ( FINE, "Fail to read phone number !\n" );
        nv_set ( "phone_number", "" );
    }
}

int parsing_quectel_lte_cgdcont ( char* data )
{
    char regex_buf[3][REGEX_BUF_ONE];
    int ret = 0;
    char *pattern = NULL;

    pattern = "\\+CGDCONT: ([0-9]*),\"[IPV46]*\",\"([-0-9a-zA-Z .]*)\",[ .,:+A-Za-z0-9\"]*.*";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );
    memset ( regex_buf[2], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "+CGDCONT:" ), pattern, regex_buf );
    if ( 0 == ret )
    {
        if ( 0 == strcmp ( regex_buf[2], "ims" ) || 0 == strcmp ( regex_buf[2], "IMS" ) )
        {
            ret = atoi ( regex_buf[1] );
            CLOGD ( FINE, "PID activated by IMS is [%d]\n", ret );
            return ret;
        }
        else
        {
            CLOGD ( FINE, "APN name no IMS!\n" );
            return 0;
        }
    }
    else
    {
        CLOGD ( ERROR, "AT+CGDCONT? return error !\n" );
        return 0;
    }
}

int parsing_quectel_lte_clcc ( char* data )
{
    char regex_buf[3][REGEX_BUF_ONE];
    int ret = 0;
    char *pattern = NULL;

    pattern = "\\+CLCC:[^,]*,[^,]*,([^,]*),[^,]*,[^,]*,\"([^\"]*)\",[0-9]*.*$";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );
    memset ( regex_buf[2], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "+CLCC:" ), pattern, regex_buf );
    if ( 0 == ret )
    {
        CLOGD ( FINE, "regex_buf[1] -> [%s]\n", regex_buf[1] );
        CLOGD ( FINE, "regex_buf[2] -> [%s]\n", regex_buf[2] );
        if ( strcmp ( regex_buf[2], "" ) )
        {
            if ( 0 == strcmp ( regex_buf[1], "2" ) || 0 == strcmp ( regex_buf[1], "4" ) )
            {
                return 1;
            }
            else if ( 0 == strcmp ( regex_buf[1], "0" ) )
            {
                return 2;
            }
            else
            {
                return 0;
            }
        }
        else
        {
            return 0;
        }
    }
    else
    {
        return 0;
    }
}

void parsing_quectel_lte_simMncLen ( char* data )
{
    int ret = 0;
    int buf_len = 0;
    char *pattern = NULL;
    char regex_buf[2][REGEX_BUF_ONE];
    char imsi_val[32] = {0};
    char usim_mcc[4] = {0};
    char usim_mnc[4] = {0};

    pattern = "\\+CRSM: [0-9]*,[0-9]*,\"(.*)\".*OK.*$";

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

void parsing_quectel_lte_sim_spn ( char* data )
{
    char regex_buf[2][REGEX_BUF_ONE];
    int ret = 0;
    char *pattern = NULL;
    char SIM_SPN[64] = {0};

    pattern = "\\+CRSM: [0-9]*,[0-9]*,\"([^\"]*)\".*$";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "+CRSM:" ), pattern, regex_buf );

    if ( 0 == ret && 34 <= strlen ( regex_buf[1] ) )
    {
        HexToStr ( SIM_SPN, regex_buf[1] + 2, sizeof ( SIM_SPN ) );
        nv_set ( "SIM_SPN", SIM_SPN );
        nv_set ( "SIM_SPN_HEX", regex_buf[1] );
        CLOGD ( WARNING, "SIM_SPN: [%s] -> [%s]\n", regex_buf[1], SIM_SPN );
    }
    else
    {
        nv_set ( "SIM_SPN", "--" );
        nv_set ( "SIM_SPN_HEX", "--" );
        CLOGD ( WARNING, "AT+CRSM get SPN return error !\n" );
    }
}

int parsing_quectel_lte_csq ( char* data )
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

int parsing_quectel_lte_creg ( char* data )
{
    char regex_buf[2][REGEX_BUF_ONE];
    int ret = 0;
    char *pattern = NULL;

    pattern = "\\+CREG: [0-9]*,([0-9]*).*";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr(data, "+CREG:"), pattern, regex_buf );
    if ( ret == 0 )
    {
        nv_set ( "cereg_stat", regex_buf[1] );
    }
    else
    {
        CLOGD ( WARNING, "AT+CREG? regex error !\n" );
    }

    return ret;
}

int parsing_quectel_lte_cgatt ( char* data )
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

void parsing_quectel_lte_Qnetdevctl ( char* data )
{
    char regex_buf[2][REGEX_BUF_ONE];
    int ret = 0;
    char *pattern = NULL;

    pattern = "\\+QNETDEVCTL:[ ]*[0-9]*,[0-9]*,[0-9]*,([0-9]*).*";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "+QNETDEVCTL:" ), pattern, regex_buf );
    if ( ret == 0 )
    {
        nv_set ( "cid_1_state", regex_buf[1] );
    }
    else
    {
        CLOGD ( FINE, "AT+QNETDEVCTL? return error!\n" );
    }
}

void parsing_quectel_lte_cgact ( char* data )
{
    if ( strstr ( data, "+CGACT: 1,1\r\n" ) )
        nv_set ( "cid_1_state", "1" );
    else
        nv_set ( "cid_1_state", "0" );

    if ( strstr ( data, "+CGACT: 7,1\r\n" ) )
        nv_set ( "cid_2_state", "1" );
    else
        nv_set ( "cid_2_state", "0" );

    if ( strstr ( data, "+CGACT: 8,1\r\n" ) )
        nv_set ( "cid_3_state", "1" );
    else
        nv_set ( "cid_3_state", "0" );

    if ( strstr ( data, "+CGACT: 9,1\r\n" ) )
        nv_set ( "cid_4_state", "1" );
    else
        nv_set ( "cid_4_state", "0" );
}

void parsing_quectel_lte_operator ( char* data )
{
    char regex_buf[3][REGEX_BUF_ONE];
    int ret = 0;
    char *pattern = NULL;

    pattern = "\\+COPS: [0-9]*,[0-9]*,\"(.*)\",([0-9]*)";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );
    memset ( regex_buf[2], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "+COPS:" ), pattern, regex_buf );
    if ( ret == 0 )
    {
        nv_set ( "operator", regex_buf[1] );
        nv_set ( "cops", regex_buf[2] );
    }
    else
    {
        nv_set ( "operator", "--" );
        nv_set ( "cops", "--" );
        CLOGD ( FINE, "AT+COPS? return error !\n" );
    }

    return;
}

void parsing_quectel_lte_lockpin ( char* data )
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

void parsing_quectel_lte_enterpin ( char* data )
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

void parsing_quectel_lte_modifypin ( char* data )
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

void parsing_quectel_lte_enterpuk ( char* data )
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

void parsing_quectel_lte_rrc_state ( char* data )
{
    char regex_buf[2][REGEX_BUF_ONE];
    int ret = 0;
    int i = 0;
    char *pattern = NULL;

    for ( i = 0; i < 2; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    pattern = "\\+QNWCFG:[ ]*\"rrc_state\",\"LTE\",([0-9]*).*$";

    ret = at_tok_regular_more ( data, pattern, regex_buf );

    if ( 0 == ret )
    {
        if ( 0 == strcmp ( regex_buf[1], "3" ) || 0 == strcmp ( regex_buf[1], "4" ) )
        {
            nv_set ( "rrc_state", "CONNECTED" );
            CLOGD ( FINE, "rrc_state -> [%s]\n", regex_buf[1] );
        }
        else if ( 0 == strcmp ( regex_buf[1], "1" ) || 0 == strcmp ( regex_buf[1], "2" ) )
        {
            nv_set ( "rrc_state", "IDLE" );
            CLOGD ( FINE, "rrc_state -> [%s]\n", regex_buf[1] );
        }
        else
        {
            nv_set ( "rrc_state", "--" );
            CLOGD ( FINE, "rrc_state -> [%s]\n", regex_buf[1] );
        }
    }
    else
    {
        nv_set ( "rrc_state", "--" );
        CLOGD ( FINE, "AT+QNWCFG? return invalid !\n" );
    }
}

static void clear_quectel_4g_cellinfo ()
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
    nv_set( "dlmcs", "--" );
    nv_set( "ulmcs", "--" );
}

static int parsing_quectel_lte_cellinfo ( char* data )
{
    CLOGD ( FINE, "parsing 4G -> [%s]\n", data );

    char regex_buf[5][REGEX_BUF_ONE];
    int ret = 0;
    int i = 0;
    char *pattern = NULL;
    char strRamVal[32] = {0};

    for ( i = 0; i < 5; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    pattern = "\\+QENG:[ ]*\"servingcell\",\"(.*)\",\"(.*)\",\"(.*)\",.*$";

    ret = at_tok_regular_more ( data, pattern, regex_buf );

    if ( 0 == ret )
    {
        for ( i = 1; i < 4; i++ )
            CLOGD ( FINE, "[%s]\n", regex_buf[i] );

        if ( strcmp ( regex_buf[1], "ENDC" ) )
            nv_set ( "ue_state", regex_buf[1] );

        nv_set ( "cell_type", regex_buf[2] );
        nv_set ( "duplex_mode", regex_buf[3] );
    }
    else
    {
        nv_set ( "ue_state", "--" );
        nv_set ( "cell_type", "--" );
        nv_set ( "duplex_mode", "--" );
    }

    for ( i = 0; i < 5; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    pattern = "\\+QENG:[ ]*\"servingcell\",\".*\",\".*\",\".*\",[ ]*"
                "([^,]*),([^,]*),([^,]*),([^,]*),.*$";

    ret = at_tok_regular_more ( data, pattern, regex_buf );

    if ( 0 == ret )
    {
        for ( i = 1; i < 5; i++ )
            CLOGD ( FINE, "[%s]\n", regex_buf[i] );

        snprintf ( strRamVal, sizeof ( strRamVal ), "%03d", atoi ( regex_buf[1] ) );
        nv_set ( "mcc", strRamVal );
        nv_set ( "mnc", regex_buf[2] );

        sscanf ( regex_buf[3], "%x", &i );
        snprintf ( strRamVal, sizeof ( strRamVal ), "%d", i );
        nv_set ( "globalid", strRamVal );
        snprintf ( strRamVal, sizeof ( strRamVal ), "%d", i & 0x000000FF );
        nv_set ( "cellid", strRamVal );
        snprintf ( strRamVal, sizeof ( strRamVal ), "%d", i >> 8 );
        nv_set ( "eNBid", strRamVal );

        nv_set ( "pci", regex_buf[4] );
    }
    else
    {
        clear_quectel_4g_cellinfo ();
        return ret;
    }

    for ( i = 0; i < 5; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    pattern = "\\+QENG:[ ]*\"servingcell\",\".*\",\".*\",\".*\",[ ]*"
                "[^,]*,[^,]*,[^,]*,[^,]*,([^,]*),([^,]*),([^,]*),([^,]*),.*$";

    ret = at_tok_regular_more ( data, pattern, regex_buf );

    if ( 0 == ret )
    {
        for ( i = 1; i < 5; i++ )
            CLOGD ( FINE, "[%s]\n", regex_buf[i] );

        nv_set ( "dl_earfcn", regex_buf[1] );
        earfcn_freq.dl_earfcn = atoi ( regex_buf[1] );
        nv_set ( "band", regex_buf[2] );
        earfcn_freq.band = atoi ( regex_buf[2] );

        if ( 0 == calc_freq_from_earfcn ( &earfcn_freq ) )
        {
            snprintf ( strRamVal, sizeof ( strRamVal ), "%d", earfcn_freq.ul_earfcn );
            nv_set ( "ul_earfcn", strRamVal );
            snprintf ( strRamVal, sizeof ( strRamVal ), "%d.%03d",
                                    earfcn_freq.dl_frequency / 1000, earfcn_freq.dl_frequency % 1000 );
            nv_set ( "dl_frequency", strRamVal );
            snprintf ( strRamVal, sizeof ( strRamVal ), "%d.%03d",
                                    earfcn_freq.ul_frequency / 1000, earfcn_freq.ul_frequency % 1000 );
            nv_set ( "ul_frequency", strRamVal );
        }

        switch ( atoi ( regex_buf[3] ) )
        {
        case 0:
            snprintf ( strRamVal, sizeof ( strRamVal ), "%s", "1.4" );
            break;
        case 1:
            snprintf ( strRamVal, sizeof ( strRamVal ), "%s", "3" );
            break;
        case 2:
        case 3:
        case 4:
        case 5:
            snprintf ( strRamVal, sizeof ( strRamVal ), "%d", ( atoi ( regex_buf[3] ) - 1 ) * 5 );
            break;
        default:
            CLOGD ( WARNING, "Invalid DL bandwidth value -> [%d]\n", atoi ( regex_buf[3] ) );
            snprintf ( strRamVal, sizeof ( strRamVal ), "%s", "--" );
            break;
        }

        nv_set ( "bandwidth", strRamVal );
        nv_set ( "dl_bandwidth", strRamVal );

        switch ( atoi ( regex_buf[4] ) )
        {
        case 0:
            snprintf ( strRamVal, sizeof ( strRamVal ), "%s", "1.4" );
            break;
        case 1:
            snprintf ( strRamVal, sizeof ( strRamVal ), "%s", "3" );
            break;
        case 2:
        case 3:
        case 4:
        case 5:
            snprintf ( strRamVal, sizeof ( strRamVal ), "%d", ( atoi ( regex_buf[4] ) - 1 ) * 5 );
            break;
        default:
            CLOGD ( WARNING, "Invalid UL bandwidth value -> [%d]\n", atoi ( regex_buf[4] ) );
            snprintf ( strRamVal, sizeof ( strRamVal ), "%s", "--" );
            break;
        }

        nv_set ( "ul_bandwidth", strRamVal );
    }
    else
    {
        clear_quectel_4g_cellinfo ();
        return ret;
    }

    for ( i = 0; i < 5; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    pattern = "\\+QENG:[ ]*\"servingcell\",\".*\",\".*\",\".*\",[ ]*"
                "[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,"
                "([^,]*),([^,]*),([^,]*),([^,]*),.*$";

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
        clear_quectel_4g_cellinfo ();
        return ret;
    }

    for ( i = 0; i < 5; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    pattern = "\\+QENG:[ ]*\"servingcell\",\".*\",\".*\",\".*\",[ ]*"
                "[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,"
                "[^,]*,[^,]*,[^,]*,[^,]*,([^,]*),([^,]*),([^,]*)$";

    ret = at_tok_regular_more ( data, pattern, regex_buf );

    if ( 0 == ret )
    {
        for ( i = 1; i < 4; i++ )
            CLOGD ( FINE, "[%s]\n", regex_buf[i] );

        snprintf ( strRamVal, sizeof ( strRamVal ), "%d", atoi ( regex_buf[1] ) * 2 - 20 );
        nv_set ( "sinr", strRamVal );

        nv_set ( "cqi", regex_buf[2] );

        if ( 0 == strcmp ( regex_buf[3], "-" ) || 0 == strcmp ( regex_buf[3], "-32768" ) )
        {
            snprintf ( strRamVal, sizeof ( strRamVal ), "%s", "--" );
        }
        else
        {
            snprintf ( strRamVal, sizeof ( strRamVal ), "%d.%d",
                                    atoi ( regex_buf[3] ) / 10, atoi ( regex_buf[3] ) % 10 );
        }
        nv_set ( "txpower", strRamVal );

    }
    else
    {
        pattern = "\\+QENG:[ ]*\"servingcell\",\".*\",\".*\",\".*\",([^,]*,){12}([^,]*),([^,]*)$";

        ret = at_tok_regular_more ( data, pattern, regex_buf );

        if ( 0 == ret )
        {
            for ( i = 2; i < 4; i++ )
                CLOGD ( FINE, "[%s]\n", regex_buf[i] );

            snprintf ( strRamVal, sizeof ( strRamVal ), "%d", atoi ( regex_buf[2] ) * 2 - 20 );
            nv_set ( "sinr", strRamVal );

            nv_set ( "rxlev", strcmp ( regex_buf[3], "-" ) ? regex_buf[3] : "--" );
        }
    }

    return 0;
}

void parsing_quectel_lte_serving_cellinfo ( char* data )
{
    CLOGD ( FINE, "start servingcell parsing ...\n" );

    char *info_line = NULL;

    info_line = strtok ( data, "\r\n" );
    while ( info_line )
    {
        CLOGD ( FINE, "Get line:\n%s\n\n", info_line );

        if ( strstr ( info_line, ",\"LTE\"," ) )
        {
            nv_set ( "mode", "4G" );
            parsing_quectel_lte_cellinfo ( info_line );
        }

        info_line = strtok ( NULL, "\r\n" );
    }

}

int parsing_quectel_lte_neighbor_cellinfo ( char* data )
{
    char regex_buf[7][REGEX_BUF_ONE];
    char strRamOpt[32] = {0};
    char strRamVal[32] = {0};
    int neighbor_count = 0;
    int i = 0;
    int ignore_first_cell = 1;
    int ret = 0;
    char *pattern = NULL;
    char *neighbor_content = NULL;

    pattern = "\\+QENG: \".*\",\".*\",([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),.*";

    neighbor_content = strtok ( data, "\r\n" );
    while ( neighbor_content )
    {
        CLOGD ( FINE, "Get line:\n%s\n\n", neighbor_content );
        if ( strstr ( neighbor_content, "neighbourcell" ) )
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

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "neighbor_earfcn_%d", neighbor_count );
                nv_set ( strRamOpt, regex_buf[1] );

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "neighbor_pci_%d", neighbor_count );
                nv_set ( strRamOpt, regex_buf[2] );

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "neighbor_rsrq_%d", neighbor_count );
                nv_set ( strRamOpt, regex_buf[3] );

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "neighbor_rsrp_%d", neighbor_count );
                nv_set ( strRamOpt, regex_buf[4] );

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "neighbor_rssi_%d", neighbor_count );
                nv_set ( strRamOpt, regex_buf[5] );

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "neighbor_sinr_%d", neighbor_count );
                nv_set ( strRamOpt, regex_buf[6] );
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
void parsing_quectel_lte_cclk ( char* data )
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
    char ntp_updated[32] = {0};
    char date_cmd_buf[256] = {0};

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

    if ( 2024 <= ( SwitchLocalTime->tm_year + 1900 ) )
    {
        snprintf ( date_cmd_buf, sizeof(date_cmd_buf), "date -s \"%s\" -u", sCurrentTime );//Set time at UTC
        system( date_cmd_buf );

        nv_get("ntp_updated", ntp_updated, sizeof(ntp_updated));
        if( strcmp( ntp_updated, "1" ) )
        {
            nv_set("ntp_updated","1");
            system ("which hwclock && hwclock -w && hwclock --systz || date -k");
        }
    }

    return;
}

void parsing_quectel_lte_qcainfo ( char* data )
{
    char regex_buf[6][REGEX_BUF_ONE];
    char strRamOpt[32] = {0};
    char strRamVal[32] = {0};
    char *cc_content = NULL;
    char *pattern = NULL;
    int cc_count = 1;
    int j = 0;
    int i = 0;
    int ret = 0;
    char band[4] = {0};

    cc_content = strtok ( data, "\r\n" );
    while ( cc_content )
    {
        CLOGD ( FINE, "Get line:\n%s\n\n", cc_content );

        if ( strstr ( cc_content, "PCC" ) )
        {
            CLOGD ( FINE, "PCC does not require parsing!\n" );
        }
        else if ( strstr ( cc_content, "SCC" ) )
        {
            if ( strstr ( cc_content, "LTE" ) )
            {
                for ( j = 0; j < 5; j++ )
                {
                    memset ( regex_buf[j], 0, REGEX_BUF_ONE );
                }

                pattern = "\\+QCAINFO:[ ]*\"SCC\",([^,]*),([^,]*),\"([^\"]*)\",[^,]*,([^,]*),.*$";
                ret = at_tok_regular_more ( cc_content, pattern, regex_buf );
                if ( 0 == ret )
                {
                    for ( j = 1; j < 5; j++ )
                    {
                        CLOGD ( FINE, "[%s]\n", regex_buf[j] );
                    }

                    nv_set ( "scc_type", "LTE" );

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "S%d_dl_earfcn", cc_count );
                    nv_set ( strRamOpt, regex_buf[1] );

                    sscanf( regex_buf[1], "%d", &i );
                    earfcn_freq.dl_earfcn = i;

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "S%d_bandwidth", cc_count );
                    if ( 0 == strcmp ( regex_buf[2], "6" ) )
                        snprintf ( strRamVal, sizeof ( strRamVal ), "%s", "1.4" );
                    else
                        snprintf ( strRamVal, sizeof ( strRamVal ), "%d", atoi ( regex_buf[2] ) / 5 );
                    nv_set ( strRamOpt, strRamVal );

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "S%d_band", cc_count );
                    sscanf ( regex_buf[3], "LTE BAND %s", band );
                    nv_set ( strRamOpt, band );
                    earfcn_freq.band = atoi ( band );

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "S%d_pci", cc_count );
                    nv_set ( strRamOpt, regex_buf[4] );
                }

                for ( j = 0; j < 5; j++ )
                {
                    memset ( regex_buf[j], 0, REGEX_BUF_ONE );
                }

                pattern = "\\+QCAINFO:[ ]*\"SCC\",[^,]*,[^,]*,\".*\",[^,]*,[^,]*,([^,]*),([^,]*),([^,]*),([^,]*),.*$";
                ret = at_tok_regular_more ( cc_content, pattern, regex_buf );
                if ( 0 == ret )
                {
                    for ( j = 1; j < 5; j++ )
                    {
                        CLOGD ( FINE, "[%s]\n", regex_buf[j] );
                    }

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "S%d_rsrp", cc_count );
                    nv_set ( strRamOpt, regex_buf[1] );

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "S%d_rsrq", cc_count );
                    nv_set ( strRamOpt, regex_buf[2] );

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "S%d_rssi", cc_count );
                    nv_set ( strRamOpt, regex_buf[3] );

                    if ( 0 == strcmp ( regex_buf[4], "-" ) )
                    {
                        snprintf ( strRamVal, sizeof ( strRamVal ), "%s", "--" );
                    }
                    else
                    {
                        snprintf ( strRamVal, sizeof ( strRamVal ), "%s", regex_buf[4] );
                    }
                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "S%d_sinr", cc_count );
                    nv_set ( strRamOpt, strRamVal );
                }

                for ( j = 0; j < 3; j++ )
                {
                    memset ( regex_buf[j], 0, REGEX_BUF_ONE );
                }

                pattern = "\\+QCAINFO: \"SCC\",[^,]*,[^,]*,\"[^\"]*\",[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,([^,]*),([^,]*)$";
                ret = at_tok_regular_more ( strstr ( cc_content, "+QCAINFO:" ), pattern, regex_buf );
                if ( 0 == ret )
                {
                    for ( j = 1; j < 3; j++ )
                    {
                        CLOGD ( FINE, "[%s]\n", regex_buf[j] );
                    }

                    if ( 0 == strcmp ( regex_buf[1], "-" ) )
                    {
                        snprintf ( strRamVal, sizeof ( strRamVal ), "%s", "--" );
                    }
                    else
                    {
                        if ( 0 == strcmp ( regex_buf[1], "6" ) )
                            snprintf ( strRamVal, sizeof ( strRamVal ), "%s", "1.4" );
                        else
                            snprintf ( strRamVal, sizeof ( strRamVal ), "%d", atoi ( regex_buf[2] ) / 5 );
                    }
                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "S%d_ul_bandwidth", cc_count );
                    nv_set ( strRamOpt, strRamVal );

                    if ( 0 == strcmp ( regex_buf[2], "-" ) )
                    {
                        snprintf ( strRamVal, sizeof ( strRamVal ), "%s", "--" );
                    }
                    else
                    {
                        snprintf ( strRamVal, sizeof ( strRamVal ), "%s", regex_buf[2] );
                    }
                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "S%d_ul_earfcn", cc_count );
                    nv_set ( strRamOpt, strRamVal );
                }

                if ( 0 == calc_freq_from_earfcn ( &earfcn_freq ) )
                {
                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "S%d_dl_frequency", cc_count );
                    snprintf ( strRamVal, sizeof ( strRamVal ), "%d.%03d", earfcn_freq.dl_frequency / 1000, earfcn_freq.dl_frequency % 1000 );
                    nv_set ( strRamOpt, strRamVal );
                    CLOGD ( FINE, "%s [%d.%03d]\n", strRamOpt, earfcn_freq.dl_frequency / 1000, earfcn_freq.dl_frequency % 1000 );

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "S%d_ul_frequency", cc_count );
                    snprintf ( strRamVal, sizeof ( strRamVal ), "%d.%03d", earfcn_freq.ul_frequency / 1000, earfcn_freq.ul_frequency % 1000 );
                    nv_set ( strRamOpt, strRamVal );
                }
            }
            cc_count++;
        }
        cc_content = strtok ( NULL, "\r\n" );
    }

    snprintf ( strRamVal, sizeof ( strRamVal ), "%d", cc_count - 1 );
    nv_set ( "secondary_cell", strRamVal );

    for ( i = cc_count + 1; i < 4; i++ )
    {
        snprintf ( strRamOpt, sizeof ( strRamOpt ), "S%d_dl_frequency", i );
        nv_set ( strRamOpt, "" );

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "S%d_ul_frequency", i );
        nv_set ( strRamOpt, "" );
    }
}

int parsing_quectel_lte_cgcontrdp ( int cid_index, char* data )
{
    char regex_buf[7][REGEX_BUF_ONE];
    char ipv4v6_tmp[REGEX_BUF_ONE] = {0};
    int ret = 0;
    int i = 0;
    int j = 0;
    char strRamOpt[32] = {0};
    char ipv4_gway[32] = {0};
    char ipv4_mask[32] = {0};
    char *pattern = NULL;
    char *ipv4v6_content = NULL;
    int ipv4_updated = 0;
    int ipv6_updated = 0;
    static char ipv4_addr[4][32];
    static char ipv6_addr[4][64];
    int c_count = 0;
    char splice_ipv6[64] = {0};
    char ipv6_prefix[64] = {0};
    char ipv6_suffix[64] = {0};
    char ipv4v6_str[RECV_BUF_SIZE] = {0};

    ipv4v6_content = strtok ( data, "\r\n" );
    while ( ipv4v6_content && i < 2 )
    {
        snprintf ( ipv4v6_str, RECV_BUF_SIZE, "%s", ipv4v6_content );
        CLOGD ( FINE, "Get line:\n%s\n\n", ipv4v6_str );
        if ( strstr ( ipv4v6_str, "+CGCONTRDP: " ) )
        {
            delchar ( ipv4v6_str, '\"' );

            c_count = calc_spec_char_counts ( ipv4v6_str, ',' );
            if ( 6 == c_count )
            {
                pattern = "\\+CGCONTRDP: [0-9]*,[0-9]*,[^,]*,([^,]*),[ ]*([^,]*),[ ]*([^,]*),[ ]*(.*)$";
            }
            else if ( 7 == c_count )
            {
                pattern = "\\+CGCONTRDP: [0-9]*,[0-9]*,[^,]*,([^,]*),[ ]*([^,]*),[ ]*([^,]*),[ ]*([^,]*),[ ]*(.*)$";
            }
            else if ( 8 <= c_count )
            {
                pattern = "\\+CGCONTRDP: [0-9]*,[0-9]*,[^,]*,([^,]*),[ ]*([^,]*),[ ]*([^,]*),[ ]*([^,]*),[ ]*([^,]*),[ ]*([^,]*).*$";
            }
            else
            {
                CLOGD ( ERROR, "',' num is %d, unknown format !!!\n", c_count );
                break;
            }

            for ( j = 0; j < 7; j++ )
            {
                memset ( regex_buf[j], 0, REGEX_BUF_ONE );
            }
            ret = at_tok_regular_more ( ipv4v6_str, pattern, regex_buf );
            if ( 0 == ret )
            {
                for ( j = 1; j < 7; j++ )
                {
                    CLOGD ( FINE, "regex_buf[%d] -> [%s]\n", j, regex_buf[j] );
                }

                if ( strstr ( regex_buf[1], ":" ) ) // ipv6
                {
                    if ( strcmp ( ipv6_addr[ cid_index - 1 ], regex_buf[1] ) )
                    {
                        snprintf ( ipv6_addr[ cid_index - 1 ], sizeof ( ipv6_addr[0] ), "%s", regex_buf[1] );
                        if ( 1 == apns_msg_flag[ cid_index ] )
                        {
                            apns_msg_flag[ cid_index ] = 0;
                        }
                    }

                    memset ( splice_ipv6, 0, sizeof ( splice_ipv6 ) );
                    memset ( ipv6_prefix, 0, sizeof ( ipv6_prefix ) );
                    if ( 0 == calc_ipv6_addr_prefix ( regex_buf[1], 64, ipv6_prefix ) )
                    {
                        if ( 5 == calc_spec_char_counts ( ipv6_prefix, ':' ) )
                        {
                            ipv6_prefix [ strlen ( ipv6_prefix ) - 1 ] = '\0';
                        }
                        CLOGD ( FINE, "ipv6_pref->[%s]\n", ipv6_prefix );

                        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_suffix", cid_index );
                        memset ( ipv6_suffix, 0, sizeof ( ipv6_suffix ) );
                        nv_get ( strRamOpt, ipv6_suffix, sizeof ( ipv6_suffix ) );
                        CLOGD ( FINE, "ipv6_suff->[%s]\n", ipv6_suffix );

                        if ( strcmp ( ipv6_suffix, "" ) )
                        {
                            snprintf ( splice_ipv6, sizeof ( splice_ipv6 ), "%s%s", ipv6_prefix, ipv6_suffix );
                        }
                        CLOGD ( FINE, "spli_ipv6->[%s]\n", splice_ipv6 );
                    }

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_ip", cid_index );
                    nv_set ( strRamOpt, strcmp ( splice_ipv6, "" ) ? splice_ipv6 : regex_buf[1] );

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_mask", cid_index );
                    nv_set ( strRamOpt, "FFFF:FFFF:FFFF:FFFF::" );

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_gateway", cid_index );
                    nv_set ( strRamOpt, /* regex_buf[2] */ "fe80::1" );

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_dns1", cid_index );
                    nv_set ( strRamOpt, regex_buf[3] );

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_dns2", cid_index );
                    nv_set ( strRamOpt, regex_buf[4] );

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_state", cid_index );
                    nv_set ( strRamOpt, "connect" );

                    ipv6_updated = 1;
                }
                else if ( strstr ( regex_buf[2], ":" ) ) // ipv4v6
                {
                    if ( strcmp ( ipv4_addr[ cid_index - 1 ], regex_buf[1] ) )
                    {
                        snprintf ( ipv4_addr[ cid_index - 1 ], sizeof ( ipv4_addr[0] ), "%s", regex_buf[1] );
                        if ( 1 == apns_msg_flag[ cid_index ] )
                        {
                            apns_msg_flag[ cid_index ] = 0;
                        }
                    }

                    if ( strcmp ( ipv6_addr[ cid_index - 1 ], regex_buf[2] ) )
                    {
                        snprintf ( ipv6_addr[ cid_index - 1 ], sizeof ( ipv6_addr[0] ), "%s", regex_buf[2] );
                        if ( 1 == apns_msg_flag[ cid_index ] )
                        {
                            apns_msg_flag[ cid_index ] = 0;
                        }
                    }

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ip", cid_index );
                    nv_set ( strRamOpt, regex_buf[1] );

                    calc_x55_mask_gw_from_ip ( regex_buf[1], ipv4_mask, ipv4_gway );
                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_mask", cid_index );
                    nv_set ( strRamOpt, ipv4_mask );

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_gateway", cid_index );
                    nv_set ( strRamOpt, ipv4_gway );

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_state", cid_index );
                    nv_set ( strRamOpt, "connect" );

                    ipv4_updated = 1;

                    memset ( splice_ipv6, 0, sizeof ( splice_ipv6 ) );
                    memset ( ipv6_prefix, 0, sizeof ( ipv6_prefix ) );
                    if ( 0 == calc_ipv6_addr_prefix ( regex_buf[2], 64, ipv6_prefix ) )
                    {
                        if ( 5 == calc_spec_char_counts ( ipv6_prefix, ':' ) )
                        {
                            ipv6_prefix [ strlen ( ipv6_prefix ) - 1 ] = '\0';
                        }
                        CLOGD ( FINE, "ipv6_pref->[%s]\n", ipv6_prefix );

                        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_suffix", cid_index );
                        memset ( ipv6_suffix, 0, sizeof ( ipv6_suffix ) );
                        nv_get ( strRamOpt, ipv6_suffix, sizeof ( ipv6_suffix ) );
                        CLOGD ( FINE, "ipv6_suff->[%s]\n", ipv6_suffix );

                        if ( strcmp ( ipv6_suffix, "" ) )
                        {
                            snprintf ( splice_ipv6, sizeof ( splice_ipv6 ), "%s%s", ipv6_prefix, ipv6_suffix );
                        }
                        CLOGD ( FINE, "spli_ipv6->[%s]\n", splice_ipv6 );
                    }

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_ip", cid_index );
                    nv_set ( strRamOpt, strcmp ( splice_ipv6, "" ) ? splice_ipv6 : regex_buf[2] );

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_mask", cid_index );
                    nv_set ( strRamOpt, "FFFF:FFFF:FFFF:FFFF::" );

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_gateway", cid_index );
                    nv_set ( strRamOpt, /* regex_buf[3] */ "fe80::1" );

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_state", cid_index );
                    nv_set ( strRamOpt, "connect" );

                    ipv6_updated = 1;

                    snprintf ( ipv4v6_tmp, sizeof ( ipv4v6_tmp ), "%s,%s", regex_buf[4], regex_buf[5] );
                    pattern = "([0-9.]*)[ ]*([0-9A-Fa-f:]*),([0-9.]*)[ ]*([0-9A-Fa-f:]*)";
                    for ( j = 0; j < 5; j++ )
                    {
                        memset ( regex_buf[j], 0, REGEX_BUF_ONE );
                    }
                    ret = at_tok_regular_more ( ipv4v6_tmp, pattern, regex_buf );
                    if ( 0 == ret )
                    {
                        for ( j = 1; j < 5; j++ )
                        {
                            CLOGD ( FINE, "regex_buf[%d] -> [%s]\n", j, regex_buf[j] );
                        }
                        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_dns1", cid_index );
                        nv_set ( strRamOpt, regex_buf[1] );

                        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_dns1", cid_index );
                        nv_set ( strRamOpt, regex_buf[2] );

                        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_dns2", cid_index );
                        nv_set ( strRamOpt, regex_buf[3] );

                        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_dns2", cid_index );
                        nv_set ( strRamOpt, regex_buf[4] );
                    }
                }
                else // ipv4
                {
                    if ( strcmp ( ipv4_addr[ cid_index - 1 ], regex_buf[1] ) )
                    {
                        snprintf ( ipv4_addr[ cid_index - 1 ], sizeof ( ipv4_addr[0] ), "%s", regex_buf[1] );
                        if ( 1 == apns_msg_flag[ cid_index ] )
                        {
                            apns_msg_flag[ cid_index ] = 0;
                        }
                    }

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ip", cid_index );
                    nv_set ( strRamOpt, regex_buf[1] );

                    calc_x55_mask_gw_from_ip ( regex_buf[1], ipv4_mask, ipv4_gway );
                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_mask", cid_index );
                    nv_set ( strRamOpt, ipv4_mask );

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_gateway", cid_index );
                    nv_set ( strRamOpt, ipv4_gway );

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_dns1", cid_index );
                    nv_set ( strRamOpt, regex_buf[3] );

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_dns2", cid_index );
                    nv_set ( strRamOpt, regex_buf[4] );

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_state", cid_index );
                    nv_set ( strRamOpt, "connect" );

                    ipv4_updated = 1;
                }
            }
            i++;
        }
        ipv4v6_content = strtok ( NULL, "\r\n" );
    }

    if ( 0 == ipv4_updated )
    {
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

        memset ( ipv4_addr[ cid_index - 1 ], 0, sizeof ( ipv4_addr[0] ) );
    }

    if ( 0 == ipv6_updated )
    {
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

        memset ( ipv6_addr[ cid_index - 1 ], 0, sizeof ( ipv6_addr[0] ) );
    }

    return ( ipv4_updated | ipv6_updated );
}

int parsing_quectel_lte_supp4gband ( char* data )
{
    char regex_buf[2][REGEX_BUF_ONE];
    int ret = 0;
    char *pattern = NULL;

    pattern = "\\+QNWPREFCFG: \"lte_band\",([0-9:]*).*$";

    memset( regex_buf[0], 0, REGEX_BUF_ONE );
    memset( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "+QNWPREFCFG: \"lte_band\"," ), pattern, regex_buf );
    if ( 0 == ret )
    {
        CLOGD ( FINE, "4G_BANDS_ORG -> [%s]\n", regex_buf[1] );
        nv_set ( "suppband_org", regex_buf[1] );
        comd_strrpl ( regex_buf[1], ":", " " );
        CLOGD ( FINE, "4G_SUPPBAND  -> [%s]\n", regex_buf[1] );
        nv_set ( "suppband", regex_buf[1] );
    }
    else
    {
        nv_set ( "suppband", "" );
        nv_set ( "suppband_org", "" );
        CLOGD ( WARNING, "GET 4G SUPPBAND -> invalid value !\n" );
    }

    return ret;
}

