#include "comd_share.h"
#include "comd_sms.h"
#include "atchannel.h"
#include "at_tok.h"
#include "comd_ip_helper.h"
#include "mtk_atcmd_parsing.h"
#include "config_key.h"
#include "hal_platform.h"

extern CELL_EARFCN_FREQ earfcn_freq;
extern COMD_DEV_INFO at_dev;
extern int apns_msg_flag[5];
extern int allSms_sem_id;

extern int sms_push_in_sqlite3 ( int init_type );

int parsing_mtk_imei ( char* data )
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

int parsing_mtk_sn ( char* data )
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

int parsing_mtk_moduleModel ( char* data )
{
    char regex_buf[2][REGEX_BUF_ONE];
    int ret = 0;
    char *pattern = NULL;
    pattern = "[\r\n]*([^\r\n]*).*OK.*$";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( data, pattern, regex_buf );
    if ( 0 == ret )
    {
        nv_set ( "modulemodel", regex_buf[1] );
    }
    else
    {
        nv_set ( "modulemodel", "" );
        CLOGD ( WARNING, "AT+CGMM return error !\n" );
    }

    return ret;
}

int parsing_mtk_moduleVersion ( char* data )
{
    char regex_buf[3][REGEX_BUF_ONE];
    int ret = 0;
    char *pattern = NULL;

    if ( M_VENDOR_QUECTEL_T750 == at_dev.comd_mVendor )
    {
        pattern = "[\r\n]*+CGMR:[ ]*([^,]*),[ ]*([^\r\n]*).*";
    }
    else
    {
        pattern = "[\r\n]*([^\r\n]*).*OK.*$";
    }

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
        CLOGD ( WARNING, "AT+CGMR return error!\n" );
    }

    return ret;
}

int parsing_mtk_rrc_state ( char* data )
{
    char regex_buf[2][REGEX_BUF_ONE];
    int ret = 0;
    char *pattern = NULL;

    pattern = "\\+KRRCSTATE:([^\r]*)[\r\n ]*OK.*";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "+KRRCSTATE:" ), pattern, regex_buf );

    if ( ret == 0 )
    {
        nv_set ( "rrc_state", regex_buf[1] );
        CLOGD ( FINE, "rrc_state: [%s]\n", regex_buf[1] );
    }
    else
    {
        nv_set ( "rrc_state", "--" );
        CLOGD ( FINE, "rrc_state: --\n" );
    }

    return 0;
}

int parsing_mtk_apn ( char* data )
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

void parsing_mtk_apnAndAuth ( int cid_index, char* data )
{
    if ( NULL == strstr ( data, "\r\n+QICSGP:" ) )
    {
        goto END;
    }

    char regex_buf[6][REGEX_BUF_ONE];
    int ret = 0;
    int i = 0;
    char *pattern = NULL;
    char strRamOpt[16] = {0};

    pattern = "\\+QICSGP: ([1-3]*),\"(.*)\",\"(.*)\",\"(.*)\",([0-3]*).*";

    for ( i = 0; i < 6; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "+QICSGP: " ), pattern, regex_buf );
    if ( 0 == ret )
    {
        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_enable", cid_index );
        nv_set ( strRamOpt, "1" );

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "pdn%d_type", cid_index );
        CLOGD ( FINE, "%s:      [%d]\n", strRamOpt, atoi ( regex_buf[1] ) - 1 );
        nv_set ( strRamOpt, strcmp ( regex_buf[1], "1" ) ? ( strcmp ( regex_buf[1], "2" ) ? "2" : "1" ) : "0" );

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_name", cid_index );
        CLOGD ( FINE, "%s:      [%s]\n", strRamOpt, regex_buf[2] );
        nv_set ( strRamOpt, regex_buf[2] );

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_username", cid_index );
        CLOGD ( FINE, "%s:  [%s]\n", strRamOpt, regex_buf[3] );
        nv_set ( strRamOpt, regex_buf[3] );

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_password", cid_index );
        CLOGD ( FINE, "%s:  [%s]\n", strRamOpt, regex_buf[4] );
        nv_set ( strRamOpt, regex_buf[4] );

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_authtype", cid_index );
        CLOGD ( FINE, "%s:  [%s]\n", strRamOpt, regex_buf[5] );
        nv_set ( strRamOpt, regex_buf[5] );

        return;
    }

END:
    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_enable", cid_index );
    nv_set ( strRamOpt, "0" );
    snprintf ( strRamOpt, sizeof ( strRamOpt ), "pdn%d_type", cid_index );
    nv_set ( strRamOpt, "0" );
    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_name", cid_index );
    nv_set ( strRamOpt, "" );
    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_username", cid_index );
    nv_set ( strRamOpt, "" );
    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_password", cid_index );
    nv_set ( strRamOpt, "" );
    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_authtype", cid_index );
    nv_set ( strRamOpt, "0" );
}

int parsing_mtk_supp4gband ( char* data )
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

int parsing_mtk_supp5gband ( char* data )
{
    char regex_buf[2][REGEX_BUF_ONE];
    int ret = 0;
    char *pattern = NULL;

    pattern = "\\+QNWPREFCFG: \"nr5g_band\",([0-9:]*).*$";

    memset( regex_buf[0], 0, REGEX_BUF_ONE );
    memset( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "+QNWPREFCFG: \"nr5g_band\"," ), pattern, regex_buf );
    if ( 0 == ret )
    {
        CLOGD ( FINE, "5G_BANDS_ORG -> [%s]\n", regex_buf[1] );
        nv_set ( "suppband5g_org", regex_buf[1] );
        comd_strrpl ( regex_buf[1], ":", " " );
        CLOGD ( FINE, "5G_SUPPBAND  -> [%s]\n", regex_buf[1] );
        nv_set ( "suppband5g", regex_buf[1] );
    }
    else
    {
        nv_set ( "suppband5g", "" );
        nv_set ( "suppband5g_org", "" );
        CLOGD ( WARNING, "GET 5G SUPPBAND -> invalid value !\n" );
    }

    return ret;
}

void parsing_mtk_cimi ( char* data )
{
    char regex_buf[2][REGEX_BUF_ONE];
    int ret = 0;
    char *pattern = NULL;

    pattern = "\\+CIMI:[ ]*\"(.*)\".*OK.*$";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "+CIMI:" ), pattern, regex_buf );
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

void parsing_mtk_simMncLen ( char* data )
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

void parsing_mtk_sim_spn ( char* data )
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

void parsing_mtk_iccid ( char* data )
{
    char regex_buf[2][REGEX_BUF_ONE];
    int ret = 0;
    char *pattern = NULL;

    pattern = "\\+ICCID:[ ]*([^\r\n]*).*OK.*$";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "+ICCID:" ), pattern, regex_buf );
    if ( 0 == ret )
    {
        nv_set ( "iccid", regex_buf[1] );
    }
    else
    {
        nv_set ( "iccid", "--" );
        CLOGD ( FINE, "AT+ICCID? return error !\n" );
    }

    return;
}

void parsing_mtk_operator ( char* data )
{
    char regex_buf[3][REGEX_BUF_ONE];
    int ret = 0;
    char *pattern = NULL;

    pattern = "\\+COPS: ([0-9]*),[0-9]*,\".*\",([0-9]*)";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );
    memset ( regex_buf[2], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "+COPS:" ), pattern, regex_buf );
    if ( ret == 0 )
    {
        nv_set ( "cops", regex_buf[1] );
        nv_set ( "netmode", regex_buf[2] );
    }
    else
    {
        nv_set ( "cops", "--" );
        nv_set ( "netmode", "--" );
        CLOGD ( FINE, "AT+COPS? return error !\n" );
    }

    return;
}

void parsing_mtk_ktoperator ( char* data )
{
    char regex_buf[2][REGEX_BUF_ONE];
    int ret = 0;
    char *pattern = NULL;

    pattern = "\\+KCOPS: \"(.*)\".*$";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "+KCOPS:" ), pattern, regex_buf );
    if ( ret == 0 )
    {
        nv_set ( "operator", regex_buf[1] );
    }
    else
    {
        nv_set ( "operator", "--" );
        CLOGD ( FINE, "AT+KCOPS? return error !\n" );
    }

    return;
}

void parsing_mtk_lockpin ( char* data )
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

void parsing_mtk_enterpin ( char* data )
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

void parsing_mtk_modifypin ( char* data )
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

void parsing_mtk_enterpuk ( char* data )
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

int parsing_mtk_cfun_set ( char* data )
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

int parsing_mtk_cfun_get ( char* data )
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

void parsing_mtk_cpin_get ( char* data )
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

void parsing_mtk_clck_get ( char* data )
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

void parsing_mtk_qpinc ( char* data )
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

int parsing_mtk_ipv4v6 ( int cid_index, char* data )
{
    char regex_buf[11][REGEX_BUF_ONE];
    int ret = 0;
    int i = 0;
    int j = 0;
    char strRamOpt[32] = {0};
    char *pattern = NULL;
    char *ipv4v6_content = NULL;
    int ipv4_updated = 0;
    int ipv6_updated = 0;
    static char ipv4_addr[4][32];
    static char ipv6_addr[4][64];
    char splice_ipv6[64] = {0};
    char ipv6_prefix[64] = {0};
    char ipv6_suffix[64] = {0};

    ipv4v6_content = strtok ( data, "\r\n" );
    while ( ipv4v6_content && i < 2 )
    {
        CLOGD ( FINE, "Get line:\n%s\n\n", ipv4v6_content );
        if ( strstr ( ipv4v6_content, "+KAPNCFG:" ) )
        {
            pattern = "\\+KAPNCFG: \"WWAN\",[0-9]*,[0-9]*,([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),[0-9]*,([^,]*),([^,]*),([^,]*),([^,]*),(.*)$";

            for ( j = 0; j < 11; j++ )
            {
                memset ( regex_buf[j], 0, REGEX_BUF_ONE );
            }

            ret = at_tok_regular_more ( ipv4v6_content, pattern, regex_buf );
            if ( 0 == ret )
            {
                if ( strcmp ( regex_buf[1], "" ) )
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
                    CLOGD ( FINE, "%s->[%s]\n", strRamOpt, regex_buf[1] );

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_mask", cid_index );
                    nv_set ( strRamOpt, regex_buf[2] );
                    CLOGD ( FINE, "%s->[%s]\n", strRamOpt, regex_buf[2] );

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_gateway", cid_index );
                    nv_set ( strRamOpt, regex_buf[3] );
                    CLOGD ( FINE, "%s->[%s]\n", strRamOpt, regex_buf[3] );

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_dns1", cid_index );
                    nv_set ( strRamOpt, regex_buf[4] );
                    CLOGD ( FINE, "%s->[%s]\n", strRamOpt, regex_buf[4] );

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_dns2", cid_index );
                    nv_set ( strRamOpt, regex_buf[5] );
                    CLOGD ( FINE, "%s->[%s]\n", strRamOpt, regex_buf[5] );

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_state", cid_index );
                    nv_set ( strRamOpt, "connect" );

                    ipv4_updated = 1;
                }

                if ( strstr ( regex_buf[6], ":" ) )
                {
                    if ( strcmp ( ipv6_addr[ cid_index - 1 ], regex_buf[6] ) )
                    {
                        snprintf ( ipv6_addr[ cid_index - 1 ], sizeof ( ipv6_addr[0] ), "%s", regex_buf[6] );
                        if ( 1 == apns_msg_flag[ cid_index ] )
                        {
                            apns_msg_flag[ cid_index ] = 0;
                        }
                    }

                    memset ( splice_ipv6, 0, sizeof ( splice_ipv6 ) );
                    memset ( ipv6_prefix, 0, sizeof ( ipv6_prefix ) );
                    if ( 0 == calc_ipv6_addr_prefix ( regex_buf[6], 64, ipv6_prefix ) )
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
                    nv_set ( strRamOpt, strcmp ( splice_ipv6, "" ) ? splice_ipv6 : regex_buf[6] );
                    CLOGD ( FINE, "%s->[%s]\n", strRamOpt, strcmp ( splice_ipv6, "" ) ? splice_ipv6 : regex_buf[6] );

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_mask", cid_index );
                    nv_set ( strRamOpt, regex_buf[7] );

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_gateway", cid_index );
                    nv_set ( strRamOpt, regex_buf[8] );

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_dns1", cid_index );
                    nv_set ( strRamOpt, regex_buf[9] );

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_dns2", cid_index );
                    nv_set ( strRamOpt, regex_buf[10] );

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_state", cid_index );
                    nv_set ( strRamOpt, "connect" );

                    ipv6_updated = 1;
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

int parsing_mtk_cereg ( char* data )
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

int parsing_mtk_c5greg ( char* data )
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

int parsing_mtk_cireg ( char* data )
{
    char regex_buf[2][REGEX_BUF_ONE];
    int ret = 0;
    char *pattern = NULL;

    pattern = "\\+CIREG: [0-9]*,([0-9]*),[0-9]*";

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

int parsing_mtk_cesq ( char* data )
{
    int i = 0;
    int cesq_param[9] = { 99, 99, 255, 255, 255, 255, 255, 255, 255 };
    char strRamOpt[32] = {0};
    char cesq_val[64] = {0};

    if ( NULL == strstr ( data, "+CESQ:" ) )
        return -1;

    sscanf ( strstr ( data, "+CESQ:" ), "+CESQ: %d,%d,%d,%d,%d,%d,%d,%d,%d\r\n",
                    &cesq_param[0], &cesq_param[1], &cesq_param[2],
                    &cesq_param[3], &cesq_param[4], &cesq_param[5],
                    &cesq_param[6], &cesq_param[7], &cesq_param[8] );

    snprintf ( cesq_val, sizeof ( cesq_val ), "%d,%d,%d,%d,%d,%d,%d,%d,%d",
                    cesq_param[0], cesq_param[1], cesq_param[2],
                    cesq_param[3], cesq_param[4], cesq_param[5],
                    cesq_param[6], cesq_param[7], cesq_param[8] );

    nv_set ( "cesq", cesq_val );

    for ( i = 0; i < 9; i++ )
    {
        snprintf ( strRamOpt, sizeof ( strRamOpt ), "cesq_param%d", i + 1 );
        snprintf ( cesq_val, sizeof ( cesq_val ), "%d", cesq_param[i] );
        nv_set ( strRamOpt, cesq_val );
    }

    return 0;
}

int parsing_mtk_cgatt ( char* data )
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

void parsing_mtk_cgact ( char* data )
{
    if ( strstr ( data, "+CGACT: 1,1\r\n" ) )
        nv_set ( "cid_1_state", "1" );
    else
        nv_set ( "cid_1_state", "0" );

    if ( strstr ( data, "+CGACT: 2,1\r\n" ) )
        nv_set ( "cid_2_state", "1" );
    else
        nv_set ( "cid_2_state", "0" );

    if ( strstr ( data, "+CGACT: 3,1\r\n" ) )
        nv_set ( "cid_3_state", "1" );
    else
        nv_set ( "cid_3_state", "0" );

    if ( strstr ( data, "+CGACT: 4,1\r\n" ) )
        nv_set ( "cid_4_state", "1" );
    else
        nv_set ( "cid_4_state", "0" );
}

static void clear_mtk_4g_cellinfo ()
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

static void clear_mtk_5g_cellinfo ()
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

static int parsing_mtk_4g_cellinfo ( char* data )
{
    CLOGD ( FINE, "parsing 4G -> [%s]\n", data );

    char regex_buf[6][REGEX_BUF_ONE];
    int ret = 0;
    int i = 0;
    char *pattern = NULL;
    char strRamVal[32] = {0};

    for ( i = 0; i < 5; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    pattern = "\\+KENG:[ ]*\"servingcell\",\"(.*)\",\"(.*)\",\"(.*)\",.*$";

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

    pattern = "\\+KENG:[ ]*\"servingcell\",\".*\",\".*\",\".*\","
                "([^,]*),([^,]*),([^,]*),([^,]*),.*$";

    ret = at_tok_regular_more ( data, pattern, regex_buf );

    if ( 0 == ret )
    {
        for ( i = 1; i < 5; i++ )
            CLOGD ( FINE, "[%s]\n", regex_buf[i] );

        nv_set ( "mcc", regex_buf[1] );
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
        clear_mtk_4g_cellinfo ();
        return ret;
    }

    for ( i = 0; i < 5; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    pattern = "\\+KENG:[ ]*\"servingcell\",\".*\",\".*\",\".*\","
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

        nv_set ( "ul_bandwidth", regex_buf[3] );

        nv_set ( "bandwidth", regex_buf[4] );
        nv_set ( "dl_bandwidth", regex_buf[4] );
    }
    else
    {
        clear_mtk_4g_cellinfo ();
        return ret;
    }

    for ( i = 0; i < 6; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    pattern = "\\+KENG:[ ]*\"servingcell\",\".*\",\".*\",\".*\","
                "[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,"
                "([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),[^,]*,[^,]*,.*$";

    ret = at_tok_regular_more ( data, pattern, regex_buf );

    if ( 0 == ret )
    {
        for ( i = 1; i < 6; i++ )
            CLOGD ( FINE, "[%s]\n", regex_buf[i] );

        sscanf ( regex_buf[1], "%x", &i );
        snprintf ( strRamVal, sizeof ( strRamVal ), "%d", i );
        nv_set ( "tac", strRamVal );

        nv_set ( "rsrp0", regex_buf[2] );
        nv_set ( "rsrp1", regex_buf[2] );
        nv_set ( "rsrq", regex_buf[3] );
        nv_set ( "rssi", regex_buf[4] );
        nv_set ( "sinr", regex_buf[5] );
    }
    else
    {
        clear_mtk_4g_cellinfo ();
        return ret;
    }

    return 0;
}

static int parsing_mtk_endc_4g_cellinfo ( char* data )
{
    char tmp_data[256] = {0};

    snprintf ( tmp_data, sizeof ( tmp_data ), "+KENG: \"servingcell\",\"ENDC\",%s", strstr ( data, "\"LTE\"," ) );

    return parsing_mtk_4g_cellinfo ( tmp_data );
}

static int parsing_mtk_5g_cellinfo ( char* data )
{
    CLOGD ( FINE, "parsing 5G -> [%s]\n", data );

    char regex_buf[5][REGEX_BUF_ONE];
    int ret = 0;
    int i = 0;
    char *pattern = NULL;
    char strRamVal[32] = {0};

    for ( i = 0; i < 4; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    pattern = "\\+KENG:[ ]*\"servingcell\",\"(.*)\",\"(.*)\",\"(.*)\",.*$";

    ret = at_tok_regular_more ( data, pattern, regex_buf );

    if ( 0 == ret )
    {
        for ( i = 1; i < 4; i++ )
            CLOGD ( FINE, "[%s]\n", regex_buf[i] );

        nv_set ( "5g_ue_state", regex_buf[1] );
        nv_set ( "5g_cell_type", regex_buf[2] );
        nv_set ( "5g_duplex_mode", regex_buf[3] );
    }
    else
    {
        nv_set ( "5g_ue_state", "--" );
        nv_set ( "5g_cell_type", "--" );
        nv_set ( "5g_duplex_mode", "--" );
    }

    for ( i = 0; i < 5; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    pattern = "\\+KENG:[ ]*\"servingcell\",\".*\",\".*\",\".*\",([^,]*),([^,]*),([^,]*),([^,]*),.*$";

    ret = at_tok_regular_more ( data, pattern, regex_buf );

    if ( 0 == ret )
    {
        for ( i = 1; i < 5; i++ )
            CLOGD ( FINE, "[%s]\n", regex_buf[i] );

        nv_set ( "5g_mcc", regex_buf[1] );
        nv_set ( "5g_mnc", regex_buf[2] );

        sscanf ( regex_buf[3], "%d", &i );
        snprintf ( strRamVal, sizeof ( strRamVal ), "%d", i );
        nv_set ( "5g_globalid", strRamVal );
        snprintf ( strRamVal, sizeof ( strRamVal ), "%d", i & 0x000000FF );
        nv_set ( "5g_cellid", strRamVal );
        snprintf ( strRamVal, sizeof ( strRamVal ), "%d", i >> 8 );
        nv_set ( "5g_eNBid", strRamVal );

        nv_set ( "5g_pci", regex_buf[4] );
    }
    else
    {
        clear_mtk_5g_cellinfo ();
        return ret;
    }

    for ( i = 0; i < 5; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    pattern = "\\+KENG:[ ]*\"servingcell\",\".*\",\".*\",\".*\","
                            "[^,]*,[^,]*,[^,]*,[^,]*,([^,]*),([^,]*),([^,]*),([^,]*),.*$";

    ret = at_tok_regular_more ( data, pattern, regex_buf );

    if ( 0 == ret )
    {
        for ( i = 1; i < 5; i++ )
            CLOGD ( FINE, "[%s]\n", regex_buf[i] );

        sscanf ( regex_buf[1], "%x", &i );
        snprintf ( strRamVal, sizeof ( strRamVal ), "%d", i );
        nv_set ( "5g_tac", strRamVal );

        nv_set ( "5g_dl_earfcn", regex_buf[2] );
        earfcn_freq.dl_earfcn = atoi ( regex_buf[2] );
        nv_set ( "5g_band", regex_buf[3] );
        earfcn_freq.band = atoi ( regex_buf[3] );

        if ( 0 == calc_5g_freq_from_narfcn ( &earfcn_freq ) )
        {
            snprintf ( strRamVal, sizeof ( strRamVal ), "%d", earfcn_freq.ul_earfcn );
            nv_set ( "5g_ul_earfcn", strRamVal );
            snprintf ( strRamVal, sizeof ( strRamVal ), "%d.%03d", earfcn_freq.dl_frequency / 1000, earfcn_freq.dl_frequency % 1000 );
            nv_set ( "5g_dl_frequency", strRamVal );
            snprintf ( strRamVal, sizeof ( strRamVal ), "%d.%03d", earfcn_freq.ul_frequency / 1000, earfcn_freq.ul_frequency % 1000 );
            nv_set ( "5g_ul_frequency", strRamVal );
        }

        nv_set ( "5g_bandwidth", regex_buf[4] );
        nv_set ( "5g_dl_bandwidth", regex_buf[4] );
        nv_set ( "5g_ul_bandwidth", regex_buf[4] );
    }
    else
    {
        clear_mtk_5g_cellinfo ();
        return ret;
    }

    for ( i = 0; i < 5; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    pattern = "\\+KENG:[ ]*\"servingcell\",\".*\",\".*\",\".*\",[ ]*[^,]*,[^,]*,[^,]*,[^,]*,"
                                            "[^,]*,[^,]*,[^,]*,[^,]*,([^,]*),([^,]*),([^,]*),([^,]*),.*$";

    ret = at_tok_regular_more ( data, pattern, regex_buf );

    if ( 0 == ret )
    {
        for ( i = 1; i < 5; i++ )
            CLOGD ( FINE, "[%s]\n", regex_buf[i] );

        nv_set ( "5g_rsrp0", regex_buf[1] );
        nv_set ( "5g_rsrp1", regex_buf[1] );
        nv_set ( "5g_rsrq", regex_buf[2] );
        nv_set ( "5g_rssi", regex_buf[3] );
        nv_set ( "5g_sinr", regex_buf[4] );
    }
    else
    {
        clear_mtk_5g_cellinfo ();
        return ret;
    }

    return 0;
}

static int parsing_mtk_endc_5g_cellinfo ( char* data )
{
    CLOGD ( FINE, "parsing ENDC_5G -> [%s]\n", data );

    char regex_buf[6][REGEX_BUF_ONE];
    int ret = 0;
    int i = 0;
    char *pattern = NULL;
    char strRamVal[32] = {0};

    nv_set ( "5g_ue_state", "--" );
    nv_set ( "5g_cell_type", "NR5G-NSA" );
    nv_set ( "5g_duplex_mode", "--" );

    for ( i = 0; i < 6; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    pattern = "\\+KENG:\"NR5G-NSA\",([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),.*$";

    ret = at_tok_regular_more ( data, pattern, regex_buf );

    if ( 0 == ret )
    {
        for ( i = 1; i < 6; i++ )
            CLOGD ( FINE, "[%s]\n", regex_buf[i] );

        nv_set ( "5g_mcc", regex_buf[1] );
        nv_set ( "5g_mnc", regex_buf[2] );
        nv_set ( "5g_pci", regex_buf[3] );
        nv_set ( "5g_rsrp0", regex_buf[4] );
        nv_set ( "5g_rsrp1", regex_buf[4] );
        nv_set ( "5g_sinr", regex_buf[5] );
    }
    else
    {
        clear_mtk_5g_cellinfo ();
        return ret;
    }

    for ( i = 0; i < 5; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    pattern = "\\+KENG:\"NR5G-NSA\",[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,([^,]*),([^,]*),([^,]*),([^,]*),$";

    ret = at_tok_regular_more ( data, pattern, regex_buf );

    if ( 0 == ret )
    {
        for ( i = 1; i < 5; i++ )
            CLOGD ( FINE, "[%s]\n", regex_buf[i] );

        nv_set ( "5g_rsrq", regex_buf[1] );
        nv_set ( "5g_dl_earfcn", regex_buf[2] );
        earfcn_freq.dl_earfcn = atoi ( regex_buf[2] );
        nv_set ( "5g_band", regex_buf[3] );
        earfcn_freq.band = atoi ( regex_buf[3] );

        if ( 0 == calc_5g_freq_from_narfcn ( &earfcn_freq ) )
        {
            snprintf ( strRamVal, sizeof ( strRamVal ), "%d", earfcn_freq.ul_earfcn );
            nv_set ( "5g_ul_earfcn", strRamVal );
            snprintf ( strRamVal, sizeof ( strRamVal ), "%d.%03d", earfcn_freq.dl_frequency / 1000, earfcn_freq.dl_frequency % 1000 );
            nv_set ( "5g_dl_frequency", strRamVal );
            snprintf ( strRamVal, sizeof ( strRamVal ), "%d.%03d", earfcn_freq.ul_frequency / 1000, earfcn_freq.ul_frequency % 1000 );
            nv_set ( "5g_ul_frequency", strRamVal );
        }

        nv_set ( "5g_bandwidth", regex_buf[4] );
        nv_set ( "5g_dl_bandwidth", regex_buf[4] );
    }
    else
    {
        clear_mtk_4g_cellinfo ();
        return ret;
    }

    return 0;
}

static int parsing_mtk_ue_state ( char* data )
{
    char regex_buf[2][REGEX_BUF_ONE];
    int ret = 0;
    int i = 0;
    char *pattern = NULL;

    for ( i = 0; i < 2; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    pattern = "\\+KENG:[ ]*\"servingcell\",\"([^\"]*)\".*$";

    ret = at_tok_regular_more ( data, pattern, regex_buf );

    nv_set ( "ue_state", ( 0 == ret ) ? regex_buf[1] : "--" );

    return ret;
}

void parsing_mtk_serving_cellinfo ( char* data )
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

        if ( strstr ( info_line, "NR5G-SA" ) )
        {
            nr_info_flag = 1;
            strcpy ( net_mode, "5G" );
            parsing_mtk_5g_cellinfo ( info_line );
        }
        else if ( strstr ( info_line, "NR5G-NSA" ) )
        {
            nr_info_flag = 1;
            strcpy ( net_mode, "ENDC" );
            parsing_mtk_endc_5g_cellinfo ( info_line );
        }
        else if ( strstr ( info_line, ",\"LTE\"," ) )
        {
            lte_info_flag = 1;
            strcpy ( net_mode, "4G" );
            parsing_mtk_4g_cellinfo ( info_line );
        }
        else if ( strstr ( info_line, "\"LTE\"," ) )
        {
            lte_info_flag = 1;
            strcpy ( net_mode, "ENDC" );
            parsing_mtk_endc_4g_cellinfo ( info_line );
        }
        else if ( strstr ( info_line, "servingcell" ) )
        {
            parsing_mtk_ue_state ( info_line );
        }

        info_line = strtok ( NULL, "\r\n" );
    }

    nv_set ( "mode", net_mode );

    if ( 0 == lte_info_flag )
    {
        clear_mtk_4g_cellinfo ();
    }

    if ( 0 == nr_info_flag )
    {
        clear_mtk_5g_cellinfo ();
    }
}

int parsing_mtk_neighbor_cellinfo ( char* data )
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

    pattern = "\\+KENG: \".*\",\".*\",([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),.*";

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
void parsing_mtk_cclk ( char* data )
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

    if ( 2024 <= ( SwitchLocalTime->tm_year + 1900 ) )
    {
        snprintf ( date_cmd_buf, sizeof(date_cmd_buf), "date -s \"%s\" -u", sCurrentTime );//Set time at UTC
        system( date_cmd_buf );

        nv_get("ntp_updated", ntp_updated, sizeof(ntp_updated));
        if( strcmp( ntp_updated, "1" ) )
        {
            nv_set("ntp_updated","1");
        }
        system ("which hwclock && hwclock -w && hwclock --systz || date -k");
    }

    return;
}

void parsing_mtk_emtsi ( char* data )
{
    char sCurrentTime[32] = {0};
    time_t gmt;
    struct tm *SwitchLocalTime;
    int ret = 0;
    char *pattern = NULL;
    char regex_buf[10][REGEX_BUF_ONE];
    char ntp_updated[32] = {0};
    char date_cmd_buf[256] = {0};
    int i = 0;

    pattern = "\\+EMTSI: (.*),(.*),(.*),([0-9]*),([0-9]*),([0-9]*),([0-9]*),([0-9]*),([0-9]*)";

    for ( i = 0; i < 10; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "+EMTSI:" ), pattern, regex_buf );

    if ( 0 != ret )
    {
        CLOGD ( WARNING, "AT+EMTSI=2 regex error !\n" );
        return;
    }

    CLOGD ( FINE, "EMTSI_INFO -> [%s][%s][%s][%s][%s][%s][%s][%s][%s]\n", regex_buf[1], regex_buf[2],regex_buf[3],regex_buf[4],
           regex_buf[5],regex_buf[6],regex_buf[7],regex_buf[8],regex_buf[9]);

    gmt = nitz_time_total_second (
                atoi(regex_buf[4]), atoi(regex_buf[5]), atoi(regex_buf[6]),
                atoi(regex_buf[7]), atoi(regex_buf[8]), atoi(regex_buf[9])
            );

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
        }
        system ("which hwclock && hwclock -w && hwclock --systz || date -k");
    }

    return;
}

static char* get_modulation ( char *mod_id )
{
    switch ( atoi ( mod_id ) )
    {
    case 0: return "BPSK";
    case 1: return "QPSK";
    case 2: return "16QAM";
    case 3: return "64QAM";
    case 4: return "256QAM";
    }

    return "";
}

void mtk_sccinfo_data_restort ( int lte_scc_num, int nr_scc_num  )
{
    int i = 0;
    char strRamOpt[32] = {0};

    for ( i = lte_scc_num + 1; i < 4; i++ )
    {
        snprintf ( strRamOpt, sizeof ( strRamOpt ), "S%d_band", i );
        nv_set ( strRamOpt, "" );

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "S%d_dl_earfcn", i );
        nv_set ( strRamOpt, "" );

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "S%d_ul_earfcn", i );
        nv_set ( strRamOpt, "" );

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "S%d_bandwidth", i );
        nv_set ( strRamOpt, "" );

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "S%d_dl_frequency", i );
        nv_set ( strRamOpt, "" );

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "S%d_ul_frequency", i );
        nv_set ( strRamOpt, "" );

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "S%d_dl_mimo", i );
        nv_set ( strRamOpt, "" );

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "S%d_ul_mimo", i );
        nv_set ( strRamOpt, "" );

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "S%d_dl_mcs", i );
        nv_set ( strRamOpt, "" );

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "S%d_ul_mcs", i );
        nv_set ( strRamOpt, "" );
    }

    for ( i = nr_scc_num + 1; i < 4; i++ )
    {
        snprintf ( strRamOpt, sizeof ( strRamOpt ), "5g_S%d_band", i );
        nv_set ( strRamOpt, "" );

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "5g_S%d_dl_earfcn", i );
        nv_set ( strRamOpt, "" );

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "5g_S%d_ul_earfcn", i );
        nv_set ( strRamOpt, "" );

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "5g_S%d_bandwidth", i );
        nv_set ( strRamOpt, "" );

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "5g_S%d_dl_frequency", i );
        nv_set ( strRamOpt, "" );

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "5g_S%d_ul_frequency", i );
        nv_set ( strRamOpt, "" );

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "5g_S%d_dl_mimo", i );
        nv_set ( strRamOpt, "" );

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "5g_S%d_ul_mimo", i );
        nv_set ( strRamOpt, "" );

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "5g_S%d_dl_mcs", i );
        nv_set ( strRamOpt, "" );

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "5g_S%d_ul_mcs", i );
        nv_set ( strRamOpt, "" );
    }

}

void parsing_mtk_edmfapp_six_four ( char* data )
{
    char regex_buf[8][REGEX_BUF_ONE];
    int ret = 0;
    int i = 0;
    int lte_scc_num = 0;
    int nr_scc_num = 0;
    char *pattern = NULL;
    char *cc_content = NULL;
    char strRamOpt[32] = {0};
    char strRamVal[32] = {0};

    cc_content = strtok ( data, "\r\n" );
    while ( cc_content )
    {
        CLOGD ( FINE, "Get line:\n%s\n\n", cc_content );

        if ( strstr ( cc_content, "LTE PCC" ) )
        {
            pattern = "\\+EDMFAPP: [^,]*,[^,]*,[^,]*,"
                "[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,"            // band,pci,earfcn,dl_bandwidth,ul_bandwidth
                "([^,]*),([^,]*),([^,]*),([^,]*),"          // dl_mimo,ul_mimo,dl_modulation,ul_modulation
                "[^,]*,[^,]*,([^,]*),([^,]*),[^,]*,"        // dl_rb,ul_rb,dl_mcs,ul_mcs,pucch_tx_power
                "[^,]*,[^,]*,([-0-9]*),.*";                 // dl_tm,ul_tm,pusch_tx_power

            for ( i = 0; i < 8; i++ )
                memset ( regex_buf[i], 0, REGEX_BUF_ONE );

            ret = at_tok_regular_more ( cc_content, pattern, regex_buf );
            if ( 0 == ret )
            {
                for ( i = 1; i < 8; i++ )
                    CLOGD ( FINE, "LTE PCC regex_buf[%d] -> [%s]\n", i, regex_buf[i] );

                nv_set ( "rankIndex", regex_buf[1] );
                nv_set ( "ul_rankIndex", regex_buf[2] );
                nv_set ( "dl_qam", get_modulation ( regex_buf[3] ) );
                nv_set ( "ul_qam", get_modulation ( regex_buf[4] ) );
                nv_set ( "dlmcs", regex_buf[5] );
                nv_set ( "ulmcs", regex_buf[6] );
                nv_set ( "txpower", regex_buf[7] );
            }
            else
            {
                CLOGD ( ERROR, "regex LTE PCC fail !!!\n" );
            }
        }
        else if ( strstr ( cc_content, "NR PCC" ) )
        {
            pattern = "\\+EDMFAPP: [^,]*,[^,]*,[^,]*,"
                "[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,"            // band,pci,narfcn,dl_bandwidth,ul_bandwidth
                "([^,]*),([^,]*),([^,]*),([^,]*),"          // dl_mimo,ul_mimo,dl_modulation,ul_modulation
                "[^,]*,[^,]*,([^,]*),([^,]*),[^,]*,"        // dl_rb,ul_rb,dl_mcs,ul_mcs,pucch_tx_power
                "[^,]*,[^,]*,([-0-9]*),.*";                 // dl_tm,ul_tm,pusch_tx_power

            for ( i = 0; i < 8; i++ )
                memset ( regex_buf[i], 0, REGEX_BUF_ONE );

            ret = at_tok_regular_more ( cc_content, pattern, regex_buf );
            if ( 0 == ret )
            {
                for ( i = 1; i < 8; i++ )
                    CLOGD ( FINE, "NR PCC regex_buf[%d] -> [%s]\n", i, regex_buf[i] );

                nv_set ( "5g_rankIndex", regex_buf[1] );
                nv_set ( "5g_ul_rankIndex", regex_buf[2] );
                nv_set ( "5g_dl_qam", get_modulation ( regex_buf[3] ) );
                nv_set ( "5g_ul_qam", get_modulation ( regex_buf[4] ) );
                nv_set ( "5g_dlmcs", regex_buf[5] );
                nv_set ( "5g_ulmcs", regex_buf[6] );
                nv_set ( "5g_txpower", regex_buf[7] );
            }
            else
            {
                CLOGD ( ERROR, "regex NR PCC fail !!!\n" );
            }
        }
        else if ( strstr ( cc_content, "LTE SCC" ) )
        {
            pattern = "\\+EDMFAPP: [^,]*,[^,]*,[^,]*,"
                "([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),"  // scc_state,scc_ul_enable,band,pci,earfcn
                "([^,]*),([^,]*),"                          // dl_bandwidth,ul_bandwidth
                "[^,]*,[^,]*,[^,]*,[^,]*,"                  // dl_mimo,ul_mimo,dl_modulation,ul_modulation
                "[^,]*,[^,]*,[^,]*,[^,]*,[-0-9]*.*";        // dl_rb,ul_rb,dl_mcs,ul_mcs,pucch_tx_power

            for ( i = 0; i < 8; i++ )
                memset ( regex_buf[i], 0, REGEX_BUF_ONE );

            ret = at_tok_regular_more ( cc_content, pattern, regex_buf );
            if ( 0 == ret )
            {
                lte_scc_num++;

                for ( i = 1; i < 8; i++ )
                    CLOGD ( FINE, "LTE SCC regex_buf[%d] -> [%s]\n", i, regex_buf[i] );

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "S%d_state", lte_scc_num );
                nv_set ( strRamOpt, regex_buf[1] );

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "S%d_ul_enable", lte_scc_num );
                nv_set ( strRamOpt, regex_buf[2] );

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "S%d_band", lte_scc_num );
                nv_set ( strRamOpt, regex_buf[3] );
                sscanf( regex_buf[3], "%d", &i );
                earfcn_freq.band = i;

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "S%d_pci", lte_scc_num );
                nv_set ( strRamOpt, regex_buf[4] );

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "S%d_earfcn", lte_scc_num );
                nv_set ( strRamOpt, regex_buf[5] );
                sscanf( regex_buf[5], "%d", &i );
                earfcn_freq.dl_earfcn = i;

                if ( 0 == calc_freq_from_earfcn ( &earfcn_freq ) )
                {
                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "S%d_dl_frequency", lte_scc_num );
                    snprintf ( strRamVal, sizeof ( strRamVal ), "%d.%03d", earfcn_freq.dl_frequency / 1000, earfcn_freq.dl_frequency % 1000 );
                    nv_set ( strRamOpt, strRamVal );
                    CLOGD ( FINE, "%s [%d.%03d]\n", strRamOpt, earfcn_freq.dl_frequency / 1000, earfcn_freq.dl_frequency % 1000 );

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "S%d_ul_frequency", lte_scc_num );
                    snprintf ( strRamVal, sizeof ( strRamVal ), "%d.%03d", earfcn_freq.ul_frequency / 1000, earfcn_freq.ul_frequency % 1000 );
                    nv_set ( strRamOpt, strRamVal );
                    CLOGD ( FINE, "%s [%d.%03d]\n", strRamOpt, earfcn_freq.ul_frequency / 1000, earfcn_freq.ul_frequency % 1000 );
                }

                switch ( atoi ( regex_buf[6] ) )
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
                    snprintf ( strRamVal, sizeof ( strRamVal ), "%d", ( atoi ( regex_buf[6] ) - 1 ) * 5 );
                    break;
                default:
                    CLOGD ( WARNING, "Invalid DL bandwidth value -> [%d]\n", atoi ( regex_buf[6] ) );
                    snprintf ( strRamVal, sizeof ( strRamVal ), "%s", "--" );
                    break;
                }

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "S%d_bandwidth", lte_scc_num );
                nv_set ( strRamOpt, strRamVal );

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "S%d_dl_bandwidth", lte_scc_num );
                nv_set ( strRamOpt, strRamVal );

                switch ( atoi ( regex_buf[7] ) )
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
                    snprintf ( strRamVal, sizeof ( strRamVal ), "%d", ( atoi ( regex_buf[7] ) - 1 ) * 5 );
                    break;
                default:
                    CLOGD ( WARNING, "Invalid UL bandwidth value -> [%d]\n", atoi ( regex_buf[7] ) );
                    snprintf ( strRamVal, sizeof ( strRamVal ), "%s", "--" );
                    break;
                }

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "S%d_ul_bandwidth", lte_scc_num );
                nv_set ( strRamOpt, strRamVal );
            }
            else
            {
                CLOGD ( ERROR, "regex LTE SCC PART1 fail !!!\n" );
            }

            pattern = "\\+EDMFAPP: [^,]*,[^,]*,[^,]*,"
                "[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,"            // scc_state,scc_ul_enable,band,pci,earfcn
                "[^,]*,[^,]*,"                              // dl_bandwidth,ul_bandwidth
                "([^,]*),([^,]*),([^,]*),([^,]*),"          // dl_mimo,ul_mimo,dl_modulation,ul_modulation
                "[^,]*,[^,]*,([^,]*),([^,]*),[^,]*,"        // dl_rb,ul_rb,dl_mcs,ul_mcs,pucch_tx_power
                "[^,]*,[^,]*,([-0-9]*),.*";                 // dl_tm,ul_tm,pusch_tx_power

            for ( i = 0; i < 8; i++ )
                memset ( regex_buf[i], 0, REGEX_BUF_ONE );

            ret = at_tok_regular_more ( cc_content, pattern, regex_buf );
            if ( 0 == ret )
            {
                for ( i = 1; i < 8; i++ )
                    CLOGD ( FINE, "LTE SCC regex_buf[%d] -> [%s]\n", i, regex_buf[i] );

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "S%d_dl_mimo", lte_scc_num );
                nv_set ( strRamOpt, regex_buf[1] );

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "S%d_ul_mimo", lte_scc_num );
                if ( 0 == strcmp ( regex_buf[2], "255" ) )
                {
                    nv_set ( strRamOpt, "--" );
                }
                else
                {
                    nv_set ( strRamOpt, regex_buf[2] );
                }

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "S%d_dl_modulation", lte_scc_num );
                nv_set ( strRamOpt, regex_buf[3] );

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "S%d_ul_modulation", lte_scc_num );
                if ( 0 == strcmp ( regex_buf[4], "255" ) )
                {
                    nv_set ( strRamOpt, "--" );
                }
                else
                {
                    nv_set ( strRamOpt, regex_buf[4] );
                }

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "S%d_dl_mcs", lte_scc_num );
                nv_set ( strRamOpt, regex_buf[5] );

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "S%d_ul_mcs", lte_scc_num );
                if ( 0 == strcmp ( regex_buf[6], "255" ) )
                {
                    nv_set ( strRamOpt, "--" );
                }
                else
                {
                    nv_set ( strRamOpt, regex_buf[6] );
                }

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "S%d_txpower", lte_scc_num );
                if ( 0 == strcmp ( regex_buf[7], "255" ) )
                {
                    nv_set ( strRamOpt, "--" );
                }
                else
                {
                    nv_set ( strRamOpt, regex_buf[7] );
                }
            }
            else
            {
                CLOGD ( ERROR, "regex LTE SCC PART2 fail !!!\n" );
            }

            if ( 0 < lte_scc_num )
            {
                snprintf ( strRamVal, sizeof ( strRamVal ), "%d", lte_scc_num );
                nv_set ( "secondary_cell", strRamVal );

                nv_set ( "scc_type", "LTE" );
            }
        }
        else if ( strstr ( cc_content, "NR SCC" ) )
        {
            pattern = "\\+EDMFAPP: [^,]*,[^,]*,[^,]*,"
                "([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),"  // scc_state,scc_ul_enable,band,pci,narfcn
                "([^,]*),([^,]*),"                          // dl_bandwidth,ul_bandwidth
                "[^,]*,[^,]*,[^,]*,[^,]*,"                  // dl_mimo,ul_mimo,dl_modulation,ul_modulation
                "[^,]*,[^,]*,[^,]*,[^,]*,[-0-9]*.*";        // dl_rb,ul_rb,dl_mcs,ul_mcs,pucch_tx_power

            for ( i = 0; i < 8; i++ )
                memset ( regex_buf[i], 0, REGEX_BUF_ONE );

            ret = at_tok_regular_more ( cc_content, pattern, regex_buf );
            if ( 0 == ret )
            {
                nr_scc_num++;

                for ( i = 1; i < 8; i++ )
                    CLOGD ( FINE, "NR SCC regex_buf[%d] -> [%s]\n", i, regex_buf[i] );

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "5g_S%d_state", nr_scc_num );
                nv_set ( strRamOpt, regex_buf[1] );

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "5g_S%d_ul_enable", nr_scc_num );
                nv_set ( strRamOpt, regex_buf[2] );

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "5g_S%d_band", nr_scc_num );
                nv_set ( strRamOpt, regex_buf[3] );
                sscanf( regex_buf[3], "%d", &i );
                earfcn_freq.band = i;

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "5g_S%d_pci", nr_scc_num );
                nv_set ( strRamOpt, regex_buf[4] );

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "5g_S%d_earfcn", nr_scc_num );
                nv_set ( strRamOpt, regex_buf[5] );
                sscanf( regex_buf[5], "%d", &i );
                earfcn_freq.dl_earfcn = i;

                if ( 0 == calc_5g_freq_from_narfcn ( &earfcn_freq ) )
                {
                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "5g_S%d_dl_frequency", nr_scc_num );
                    snprintf ( strRamVal, sizeof ( strRamVal ), "%d.%03d", earfcn_freq.dl_frequency / 1000, earfcn_freq.dl_frequency % 1000 );
                    nv_set ( strRamOpt, strRamVal );
                    CLOGD ( FINE, "%s [%d.%03d]\n", strRamOpt, earfcn_freq.dl_frequency / 1000, earfcn_freq.dl_frequency % 1000 );

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "5g_S%d_ul_frequency", nr_scc_num );
                    snprintf ( strRamVal, sizeof ( strRamVal ), "%d.%03d", earfcn_freq.ul_frequency / 1000, earfcn_freq.ul_frequency % 1000 );
                    nv_set ( strRamOpt, strRamVal );
                    CLOGD ( FINE, "%s [%d.%03d]\n", strRamOpt, earfcn_freq.ul_frequency / 1000, earfcn_freq.ul_frequency % 1000 );
                }

                int bw_index = atoi ( regex_buf[6] );
                switch ( bw_index )
                {
                case 0:     // 5M
                case 1:     // 10M
                case 2:     // 15M
                case 3:     // 20M
                case 4:     // 25M
                case 5:     // 30M
                    snprintf ( strRamVal, sizeof ( strRamVal ), "%d", ( bw_index + 1 ) * 5 );
                    break;
                case 6:     // 40M
                case 7:     // 50M
                case 8:     // 60M
                case 9:     // 70M
                case 10:    // 80M
                case 11:    // 90M
                case 12:    // 100M
                    snprintf ( strRamVal, sizeof ( strRamVal ), "%d", ( bw_index - 2 ) * 10 );
                    break;
                case 13:    // 200M
                case 14:    // 400M
                    snprintf ( strRamVal, sizeof ( strRamVal ), "%d", ( bw_index - 12 ) * 200 );
                    break;
                default:
                    CLOGD ( WARNING, "Invalid DL bandwidth value -> [%d]\n", regex_buf[6] );
                    snprintf ( strRamVal, sizeof ( strRamVal ), "%s", "--" );
                    break;
                }

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "5g_S%d_bandwidth", nr_scc_num );
                nv_set ( strRamOpt, strRamVal );

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "5g_S%d_dl_bandwidth", nr_scc_num );
                nv_set ( strRamOpt, strRamVal );

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "5g_S%d_ul_bandwidth", nr_scc_num );
                if ( 0 == strcmp ( regex_buf[7], "255" ) )
                {
                    nv_set ( strRamOpt, "--" );
                }
                else
                {
                    nv_set ( strRamOpt, regex_buf[7] );
                }
            }
            else
            {
                CLOGD ( ERROR, "regex NR SCC PART1 fail !!!\n" );
            }

            pattern = "\\+EDMFAPP: [^,]*,[^,]*,[^,]*,"
                "[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,"            // scc_state,scc_ul_enable,band,pci,narfcn
                "[^,]*,[^,]*,"                              // dl_bandwidth,ul_bandwidth
                "([^,]*),([^,]*),([^,]*),([^,]*),"          // dl_mimo,ul_mimo,dl_modulation,ul_modulation
                "[^,]*,[^,]*,([^,]*),([^,]*),[^,]*,"        // dl_rb,ul_rb,dl_mcs,ul_mcs,pucch_tx_power
                "[^,]*,[^,]*,([-0-9]*),.*";                 // dl_tm,ul_tm,pusch_tx_power

            for ( i = 0; i < 8; i++ )
                memset ( regex_buf[i], 0, REGEX_BUF_ONE );

            ret = at_tok_regular_more ( cc_content, pattern, regex_buf );
            if ( 0 == ret )
            {
                for ( i = 1; i < 8; i++ )
                    CLOGD ( FINE, "NR SCC regex_buf[%d] -> [%s]\n", i, regex_buf[i] );

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "5g_S%d_dl_mimo", nr_scc_num );
                nv_set ( strRamOpt, regex_buf[1] );

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "5g_S%d_ul_mimo", nr_scc_num );
                if ( 0 == strcmp ( regex_buf[2], "255" ) )
                {
                    nv_set ( strRamOpt, "--" );
                }
                else
                {
                    nv_set ( strRamOpt, regex_buf[2] );
                }

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "5g_S%d_dl_modulation", nr_scc_num );
                nv_set ( strRamOpt, regex_buf[3] );

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "5g_S%d_ul_modulation", nr_scc_num );
                if ( 0 == strcmp ( regex_buf[4], "255" ) )
                {
                    nv_set ( strRamOpt, "--" );
                }
                else
                {
                    nv_set ( strRamOpt, regex_buf[4] );
                }

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "5g_S%d_dl_mcs", nr_scc_num );
                nv_set ( strRamOpt, regex_buf[5] );

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "5g_S%d_ul_mcs", nr_scc_num );
                if ( 0 == strcmp ( regex_buf[6], "255" ) )
                {
                    nv_set ( strRamOpt, "--" );
                }
                else
                {
                    nv_set ( strRamOpt, regex_buf[6] );
                }

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "5g_S%d_txpower", nr_scc_num );
                if ( 0 == strcmp ( regex_buf[7], "255" ) )
                {
                    nv_set ( strRamOpt, "--" );
                }
                else
                {
                    nv_set ( strRamOpt, regex_buf[7] );
                }
            }
            else
            {
                CLOGD ( ERROR, "regex NR SCC PART2 fail !!!\n" );
            }

            if ( 0 < nr_scc_num )
            {
                snprintf ( strRamVal, sizeof ( strRamVal ), "%d", nr_scc_num );
                nv_set ( "secondary_cell", strRamVal );

                nv_set ( "scc_type", "NR5G" );
            }
        }

        cc_content = strtok ( NULL, "\r\n" );
    }

    CLOGD ( FINE, "LTE_SCC_NUM: [%d], NR_SCC_NUM: [%d]\n", lte_scc_num, nr_scc_num );

    if ( 0 == lte_scc_num && 0 == nr_scc_num )
    {
        nv_set ( "secondary_cell", "0" );

        nv_set ( "scc_type", "" );
    }

    mtk_sccinfo_data_restort ( lte_scc_num, nr_scc_num );
}

void parsing_mtk_edmfapp_six_ten ( char* data )
{
    char regex_buf[5][REGEX_BUF_ONE];
    int ret = 0;
    int i = 0;
    int lte_scc_num = 0;
    int nr_scc_num = 0;
    char *pattern = NULL;
    char *cc_content = NULL;
    char strRamOpt[32] = {0};
    char lte_rsrp[16] = {0};
    char lte_rsrq[16] = {0};
    char nr_rsrp[16] = {0};
    char nr_rsrq[16] = {0};

    cc_content = strtok ( data, "\r\n" );
    while ( cc_content )
    {
        CLOGD ( FINE, "Get line:\n%s\n\n", cc_content );

        if ( strstr ( cc_content, "LTE PCC" ) )
        {
            pattern = "\\+EDMFAPP: [^,]*,[^,]*,[^,]*,"
                "([^,]*),([^,]*),([^,]*),([-0-9]*).*";  // path_loss,rank,rsrp,rsrq

            for ( i = 0; i < 5; i++ )
                memset ( regex_buf[i], 0, REGEX_BUF_ONE );

            ret = at_tok_regular_more ( cc_content, pattern, regex_buf );
            if ( 0 == ret )
            {
                for ( i = 1; i < 5; i++ )
                    CLOGD ( FINE, "LTE PCC regex_buf[%d] -> [%s]\n", i, regex_buf[i] );
            }
            else
            {
                CLOGD ( ERROR, "regex LTE PCC fail !!!\n" );
            }
        }
        else if ( strstr ( cc_content, "NR PCC" ) )
        {
            pattern = "\\+EDMFAPP: [^,]*,[^,]*,[^,]*,"
                "([^,]*),([^,]*),[^,]*,([^,]*),([-0-9]*).*";    // path_loss,rank,beam_rsrp,rsrp,rsrq

            for ( i = 0; i < 5; i++ )
                memset ( regex_buf[i], 0, REGEX_BUF_ONE );

            ret = at_tok_regular_more ( cc_content, pattern, regex_buf );
            if ( 0 == ret )
            {
                for ( i = 1; i < 5; i++ )
                    CLOGD ( FINE, "NR PCC regex_buf[%d] -> [%s]\n", i, regex_buf[i] );
            }
            else
            {
                CLOGD ( ERROR, "regex NR PCC fail !!!\n" );
            }
        }
        else if ( strstr ( cc_content, "LTE SCC" ) )
        {
            pattern = "\\+EDMFAPP: [^,]*,[^,]*,[^,]*,"
                "([^,]*),([^,]*),([^,]*),([-0-9]*).*";  // path_loss,rank,rsrp,rsrq

            for ( i = 0; i < 5; i++ )
                memset ( regex_buf[i], 0, REGEX_BUF_ONE );

            ret = at_tok_regular_more ( cc_content, pattern, regex_buf );
            if ( 0 == ret )
            {
                lte_scc_num++;

                for ( i = 1; i < 5; i++ )
                    CLOGD ( FINE, "LTE SCC regex_buf[%d] -> [%s]\n", i, regex_buf[i] );

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "S%d_path_loss", lte_scc_num );
                nv_set ( strRamOpt, regex_buf[1] );

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "S%d_rank", lte_scc_num );
                nv_set ( strRamOpt, regex_buf[2] );

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "S%d_rsrp", lte_scc_num );
                snprintf ( lte_rsrp, sizeof ( lte_rsrp ), "%d", atoi ( regex_buf[3] ) - 141 );
                CLOGD ( FINE, "lte_rsrp -> [%s]\n", lte_rsrp );
                nv_set ( strRamOpt, lte_rsrp );

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "S%d_rsrq", lte_scc_num );
                snprintf ( lte_rsrq, sizeof ( lte_rsrq ), "%d", atoi ( regex_buf[4] ) / 2 - 20 );
                CLOGD ( FINE, "lte_rsrq -> [%s]\n", lte_rsrq );
                nv_set ( strRamOpt, lte_rsrq );
            }
            else
            {
                CLOGD ( ERROR, "regex LTE SCC fail !!!\n" );
            }
        }
        else if ( strstr ( cc_content, "NR SCC" ) )
        {
            pattern = "\\+EDMFAPP: [^,]*,[^,]*,[^,]*,"
                "([^,]*),([^,]*),[^,]*,([^,]*),([-0-9]*).*";    // path_loss,rank,beam_rsrp,rsrp,rsrq

            for ( i = 0; i < 5; i++ )
                memset ( regex_buf[i], 0, REGEX_BUF_ONE );

            ret = at_tok_regular_more ( cc_content, pattern, regex_buf );
            if ( 0 == ret )
            {
                nr_scc_num++;

                for ( i = 1; i < 5; i++ )
                    CLOGD ( FINE, "NR SCC regex_buf[%d] -> [%s]\n", i, regex_buf[i] );

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "5g_S%d_path_loss", nr_scc_num );
                nv_set ( strRamOpt, regex_buf[1] );

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "5g_S%d_rank", nr_scc_num );
                nv_set ( strRamOpt, regex_buf[2] );

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "5g_S%d_rsrp", nr_scc_num );
                snprintf ( nr_rsrp, sizeof ( nr_rsrp ), "%d", atoi ( regex_buf[3] ) - 157 );
                nv_set ( strRamOpt, nr_rsrp );

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "5g_S%d_rsrq", nr_scc_num );
                snprintf ( nr_rsrq, sizeof ( nr_rsrq ), "%d", atoi ( regex_buf[4] ) / 2 - 43 );
                nv_set ( strRamOpt, nr_rsrq );
            }
            else
            {
                CLOGD ( ERROR, "regex NR SCC fail !!!\n" );
            }
        }

        cc_content = strtok ( NULL, "\r\n" );
    }

    CLOGD ( FINE, "LTE_SCC_NUM: [%d], NR_SCC_NUM: [%d]\n", lte_scc_num, nr_scc_num );
}

void parsing_mtk_supportband ( char* data )
{
    char regex_buf[3][REGEX_BUF_ONE];
    int ret = 0;
    char *pattern = NULL;

    pattern = "\\+KSBAND:([^,]*),([^,]*).*";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );
    memset ( regex_buf[2], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "+KSBAND:" ), pattern, regex_buf );
    if ( ret == 0 )
    {
        nv_set ( "band_4g_list", regex_buf[1] );
        nv_set ( "band_5g_list", regex_buf[2] );
    }
    else
    {
        nv_set ( "band_4g_list", "" );
        nv_set ( "band_5g_list", "" );
        CLOGD ( FINE, "AT+CGATT? return error!\n" );
    }

    return;
}

void parsing_mtk_cnum ( char* data )
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

void parsing_mtk_cpms_get ( char* data )
{
    int ret = 0;
    char *pattern = NULL;
    char regex_buf[2][REGEX_BUF_ONE];

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    pattern = "\\+CPMS: \"[A-Z]*\",[0-9]*,([0-9]*),.*OK.*$";

    ret = at_tok_regular_more ( strstr ( data, "+CPMS:" ), pattern, regex_buf );
    if ( 0 == ret )
    {
        nv_set ( "sms_max_num", regex_buf[1] );
    }
    else
    {
        nv_set ( "sms_max_num", "15" );
        CLOGD ( WARNING, "AT+CPMS? return error !\n" );
    }
}

void parsing_mtk_cmgl_four_sim_card ( char* data )
{
    static char old_allSms[RECV_SMS_SIZE] = {0};

    if ( 0 == strcmp ( old_allSms, data ) || !strstr(data, "+CMGL: ") )
    {
        return;
    }

    comd_semaphore_p ( allSms_sem_id );

    FILE *fp = NULL;

    fp = fopen ( "/tmp/SimInCard.txt", "w+" );

    if ( fp )
    {
        fprintf ( fp, "%s", data );
        strcpy ( old_allSms, data );
        fclose ( fp );

        nv_set ( "sms_sim_card_need_update", "1" );
    }

    comd_semaphore_v ( allSms_sem_id );
}

int parsing_mtk_cmgl_zero ( char* data )
{
    int ret = 0;

    if(!strstr(data, "+CMGL: "))
    {
        return -1;
    }

    comd_semaphore_p ( allSms_sem_id );

    FILE *fp = NULL;

    fp = fopen ( "/tmp/unreadSms.txt", "w+" );

    if ( fp )
    {
        fprintf ( fp, "%s", data );
        fclose ( fp );
    }

    ret = sms_push_in_sqlite3(1);
    comd_semaphore_v ( allSms_sem_id );
    nv_set ( "sms_need_update", "1" );
    return ret;
}

void parsing_mtk_cpbs ( char* data )
{
    int ret = 0;
    char *pattern = NULL;
    char regex_buf[3][REGEX_BUF_ONE];

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );
    memset ( regex_buf[2], 0, REGEX_BUF_ONE );

    pattern = "\\+CPBS: \"[A-Z]*\", ([0-9]*), ([0-9]*).*";

    ret = at_tok_regular_more ( strstr ( data, "+CPBS:" ), pattern, regex_buf );
    if ( 0 == ret )
    {
        nv_set ( "simbook_curnum", regex_buf[1] );
        nv_set ( "simbook_maxnum", regex_buf[2] );
        CLOGD ( FINE, "simbook_curnum [%s]\n", regex_buf[1] );
        CLOGD ( FINE, "simbook_maxnum [%s]\n", regex_buf[2] );
    }
    else
    {
        nv_set ( "simbook_curnum", "" );
        nv_set ( "simbook_maxnum", "" );
        CLOGD ( WARNING, "AT+CPBS? get simbook_curnum & simbook_maxnum fail !\n" );
    }
}

void parsing_mtk_csca_get ( char* data )
{
    int ret = 0;
    char *pattern = NULL;
    char regex_buf[2][REGEX_BUF_ONE];

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    pattern = "\\+CSCA: \"([0-9+]*)\",.*OK.*$";

    ret = at_tok_regular_more ( strstr ( data, "+CSCA:" ), pattern, regex_buf );
    if ( 0 == ret )
    {
        nv_set ( "sms_center_num", regex_buf[1] );
    }
    else
    {
        nv_set ( "sms_center_num", "" );
        CLOGD ( WARNING, "AT+CSCA? return nothing !\n" );
    }
}

void parsing_mtk_cpbr ( char* data )
{
    int ret = 0;
    char *pattern = NULL;
    char regex_buf[5][REGEX_BUF_ONE];
    char sql[4096]={0};
    char *errmsg = NULL;
    sqlite3 *db;
    char name[64] = {0};
    char mobile_true[48] = {0};

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );
    memset ( regex_buf[2], 0, REGEX_BUF_ONE );
    memset ( regex_buf[3], 0, REGEX_BUF_ONE );

    ret = sqlite3_open ( SMS_DIR_TMP, &db );
    if ( ret != SQLITE_OK )
    {
        perror("sqlite open :");
        return;
    }

    pattern = "\\+CPBR:[ ]*([0-9]*),\"([^\"]*)\",[^,]*,\"([^\"]*)\",.*$";

    CLOGD ( FINE, "data : [%s]\n", data );
    //idx INTEGER PRIMARY KEY,name text,mobile text,home text,office text,email text,location text,panel text,mobile_true
    //                   NULL,     name,   phonenum,     NULL,       NULL,     index,          '1',      NULL,       NULL
    ret = at_tok_regular_more ( strstr ( data, "+CPBR:" ), pattern, regex_buf );
    if ( 0 == ret )
    {
        unicode2chars ( regex_buf[3], name, sizeof ( name ) );
        CLOGD ( FINE, "name [%s]\n", name );

        if ( 0 == strncasecmp ( regex_buf[2], "+234", 4 ) )
        {
            lib_str_intercept ( regex_buf[2], mobile_true, 4, -1 );
        }
        else if ( 0 == strncasecmp ( regex_buf[2], "+86", 3 ) )
        {
            lib_str_intercept ( regex_buf[2], mobile_true, 3, -1 );
        }
        else if ( 0 == strncasecmp ( regex_buf[2], "+56", 3 ) )
        {
            lib_str_intercept ( regex_buf[2], mobile_true, 3, -1 );
        }
        else
        {
            strcpy ( mobile_true, regex_buf[2] );
        }

        snprintf ( sql, sizeof ( sql ), "insert into sms_book values (NULL,'%s','%s','','','%s','1','','%s');",
            name, regex_buf[2], regex_buf[1], mobile_true );

        CLOGD ( FINE, "sql [%s]", sql );

        if ( SQLITE_OK != sqlite3_exec ( db, sql, NULL, NULL, &errmsg ) )
        {
            CLOGD ( FINE, "fail:%s\n", errmsg );
            CLOGD ( FINE, "\n" );
        }

        if(errmsg)
        {
            sqlite3_free(errmsg);
            errmsg=NULL;
        }
    }
    else
    {
        CLOGD ( WARNING, "AT+CPBR? get simbook fail !\n" );
    }
    sqlite3_close ( db );
}

void parsing_mtk_gcmgs ( char* data )
{
    if ( strstr ( data, "+CMGS:" ) )
    {
        nv_set ( "send_sms", "0" );
    }
    else
    {
        nv_set ( "send_sms", "1" );
    }
}


