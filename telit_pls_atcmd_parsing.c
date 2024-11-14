#include "comd_share.h"
#include "atchannel.h"
#include "at_tok.h"
#include "comd_ip_helper.h"
#include "telit_pls_atcmd_parsing.h"
#include "config_key.h"
#include "hal_platform.h"

extern CELL_EARFCN_FREQ earfcn_freq;
extern int apns_msg_flag[5];
extern COMD_DEV_INFO at_dev;

static void clear_telit_pls_4g_cellinfo (void)
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
    nv_set ( "rrc_state", "--" );
}

int parsing_telit_pls_cfun_set ( char* data )
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

int parsing_telit_pls_moduleModel ( char* data )
{
    char regex_buf[2][REGEX_BUF_ONE] = {0};
    int ret = 0;
    char *pattern = NULL;

    //pattern = "[\r\n]*(*).*OK.*$";
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
        CLOGD ( WARNING, "AT+CGMM return invalid !\n" );
    }

    return ret;
}

int parsing_telit_pls_moduleVersion ( char* data )
{
    char regex_buf[4][REGEX_BUF_ONE] = {0};
    int ret = 0;
    char *pattern = NULL;

    pattern = "\r\nREVISION ([A-z0-9]+\.[A-z0-9]+)";
    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( data, pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( "moduleversion", regex_buf[1] );

    }
    else
    {
        nv_set ( "moduleversion", "" );
        CLOGD ( WARNING, "AT+CGMR return invalid !\n" );
    }

    return ret;
}

int parsing_telit_pls_imei ( char* data )
{
    char regex_buf[2][REGEX_BUF_ONE] = {0};
    int ret = 0;
    char *pattern = NULL;

    pattern = "[\r\n]([0-9]+)";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "\r\n" ), pattern, regex_buf );


    if ( 0 == ret )
    {
        nv_set ( "imei", regex_buf[1] );
    }
    else
    {
        nv_set ( "imei", "" );
        CLOGD ( FINE, "AT+CGSN return error !\n" );
    }

    return ret;
}

int parsing_telit_pls_sn ( char* data )
{
    char regex_buf[2][REGEX_BUF_ONE] = {0};
    int ret = 0;
    char *pattern = NULL;

    pattern = "[\r\n]([0-9]+)";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "\r\n" ), pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( "SN", regex_buf[1] );
    }
    else
    {
        nv_set ( "SN", "" );
        CLOGD ( FINE, "AT+GSN return error !\n" );
    }

    return ret;
}

void parsing_telit_pls_spic ( char* data )
{
    int ret = 0;
    char *pattern = NULL;
    char regex_buf[8][REGEX_BUF_ONE] = {0};

    pattern = "\\^SPIC: ([0-9]).*OK.*$";

    ret = at_tok_regular_more ( strstr ( data, "^SPIC: " ), pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( "pin_times", regex_buf[1] );
        nv_set ( "puk_times", regex_buf[1] );
    }
    else
    {
        nv_set ( "pin_times", "3" );
        nv_set ( "puk_times", "3" );
        CLOGD ( WARNING, "getPinPukTimes fail !\n" );
    }

}

void parsing_telit_pls_cimi ( char* data )
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

}

void parsing_telit_pls_sim_spn ( char* data )
{
    char regex_buf[2][REGEX_BUF_ONE];
    int ret = 0;
    char *pattern = NULL;
    char SIM_SPN[64] = {0};

    pattern = "\\+CRSM: [0-9]+,[0-9]+,\"([A-z0-9]+)\".*$";

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

void parsing_telit_pls_simMncLen ( char* data )
{
    int ret = 0;
    int buf_len = 0;
    char *pattern = NULL;
    char regex_buf[2][REGEX_BUF_ONE];
    char imsi_val[32] = {0};
    char usim_mcc[4] = {0};
    char usim_mnc[4] = {0};

    pattern = "\\+CRSM: [0-9]+,[0-9]+,\"([0-9]+)\".*OK.*$";

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

void parsing_telit_pls_iccid ( char* data )
{
    char regex_buf[4][REGEX_BUF_ONE] = {0};
    int i = 0;
    int ret = 0;
    char *pattern = NULL;

    pattern = "\\+CRSM: [0-9]+,[0-9]+,\"([0-9]+)\".*OK.*$";

    ret = at_tok_regular_more ( strstr ( data, "+CRSM: " ), pattern, regex_buf );
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

void parsing_telit_pls_cnum ( char* data )
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

void parsing_telit_pls_cesq( char* data )
{
    int cesq_param[6] = { 99, 99, 255, 255, 255, 255 };
    char cesq_val[64] = {0};

    if ( NULL == strstr ( data, "+CESQ:" ) )
        goto DEFAULT_CESQ;

    sscanf ( strstr ( data, "+CESQ:" ), "+CESQ: %d,%d,%d,%d,%d,%d\r\n",
                    &cesq_param[0], &cesq_param[1], &cesq_param[2],
                    &cesq_param[3], &cesq_param[4], &cesq_param[5] );

DEFAULT_CESQ:
    snprintf ( cesq_val, sizeof ( cesq_val ), "%d,%d,%d,%d,%d,%d",
                    cesq_param[0], cesq_param[1], cesq_param[2],
                    cesq_param[3], cesq_param[4], cesq_param[5] );

    nv_set ( "cesq", cesq_val );
    snprintf(cesq_val, sizeof(cesq_val), "%d", cesq_param[8]);

}

void parsing_telit_pls_operator ( char* data )
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

}

int parsing_telit_pls_cgatt ( char* data )
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

}

//Might need develop LTE-CA info
void parsing_telit_pls_serving_4gcellinfo ( char* data )
{
    CLOGD ( FINE, "start 4g servingcell parsing ...\n" );

    char network_mode[8] = {0};
    char lte_arfcn[8] = {0};
    char lte_band[8] = {0};
    char lte_dl_bandwidth[8] = {0};
    char lte_ul_bandwidth[8] = {0};
    char lte_duplex_mode[8] = {0};
    char lte_mcc[8] = {0};
    char lte_mnc[8] = {0};
    char lte_tac[16] = {0};
    char lte_global_cellid[16] = {0};
    char lte_phy_cellid[16] = {0};
    char lte_rsrq[16] = {0};
    char lte_rsrp[16] = {0};
    char lte_sinr[16] = {0};
    char strRamVal[32] = {0};

    char output_str[256] = {0};
    char* pad_char = '0';
    char* symbol_char = ',';

    int cellid_int = 0;
    if(strstr(data, "^SMONI: 4G,") && !strstr(data, "^SMONI: 4G,SEARCH"))
    {
        padding_char_between_two_same_symbol(pad_char, symbol_char, data, output_str);
        //                        mode  arfcn band  dl_bw  ul_bw duplex mcc  mnc  tac    g_id  p_id         rsrp   rsrq
        sscanf(output_str, "\r\n^SMONI: %[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%*[^,],%[^,],%[^,],%*[^,], \
                                  %*[^,],%*[^,],%*[^,],%[^,],*",
        //                                              sinr
                                  network_mode,lte_arfcn,lte_band,lte_dl_bandwidth,lte_ul_bandwidth,lte_duplex_mode,lte_mcc,
                                  lte_mnc,lte_tac,lte_global_cellid,lte_phy_cellid,lte_rsrp,lte_rsrq,lte_sinr);

        cellid_int = strtol(lte_global_cellid, NULL, 16);
        cellid_int = cellid_int & 0xFF;
        snprintf(lte_global_cellid, sizeof(lte_global_cellid), "%d", cellid_int);
        nv_set ( "cellid", lte_global_cellid );
        nv_set ( "band", lte_band );
        nv_set ( "mcc", lte_mcc );
        nv_set ( "mnc", lte_mnc );
        nv_set ( "dl_earfcn", lte_arfcn );
        nv_set ( "ul_earfcn", lte_arfcn );
        nv_set ( "pci", lte_phy_cellid );
        nv_set ( "rrc_state", "IDLE" );

        nv_set ( "bandwidth", lte_dl_bandwidth );
        nv_set ( "dl_bandwidth", lte_dl_bandwidth );

        nv_set ( "ul_bandwidth", lte_ul_bandwidth );

        nv_set ( "tac", lte_tac );

        nv_set ( "rsrp0", lte_rsrp );
        nv_set ( "rsrp1", lte_rsrp );
        nv_set ( "rsrq", lte_rsrq );
        nv_set ( "mode", lte_duplex_mode );
        nv_set( "sinr", lte_sinr );
        earfcn_freq.dl_earfcn = atoi ( lte_arfcn );
        earfcn_freq.band = atoi ( lte_band );

        if ( 0 == calc_freq_from_earfcn ( &earfcn_freq ) )
        {
            snprintf ( strRamVal, sizeof ( strRamVal ), "%d.%03d",
                                    earfcn_freq.dl_frequency / 1000, earfcn_freq.dl_frequency % 1000 );
            nv_set ( "dl_frequency", strRamVal );
            snprintf ( strRamVal, sizeof ( strRamVal ), "%d.%03d",
                                    earfcn_freq.ul_frequency / 1000, earfcn_freq.ul_frequency % 1000 );
            nv_set ( "ul_frequency", strRamVal );
        }
    }
    else
    {
        clear_telit_pls_4g_cellinfo();
    }

}

int parsing_telti_pls_cereg ( char* data )
{
    char regex_buf[2][REGEX_BUF_ONE];
    int ret = 0;
    char *pattern = NULL;

    pattern = "\\+CEREG: [0-9]*,([0-9]*).*";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr(data, "+CEREG:"), pattern, regex_buf );
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

void parsing_telit_pls_cgact ( char* data )
{
    if ( strstr ( data, "+CGACT: 1,1\r\n" ) )
        nv_set ( "cid_1_state", "1" );
    else
        nv_set ( "cid_1_state", "0" );

    nv_set ( "cid_2_state", "0" );
    nv_set ( "cid_3_state", "0" );
    nv_set ( "cid_4_state", "0" );
}

int parsing_telit_pls_cgcontrdp ( int cid_index, char* at_rep )
{
    static char ipv4_addr[8][REGEX_BUF_ONE];
    static char ipv6_addr[8][REGEX_BUF_ONE];

    char nvram_ipv4_addr[32] = {0};
    char nvram_ipv6_addr[128] = {0};
    char regex_buf[8][REGEX_BUF_ONE] = {0};
    char apn_pdp_type[8] = {0};
    char strRamOpt[128] = {0};
    char splice_ipv6[64] = {0};
    char ipv6_prefix[64] = {0};
    char ipv6_suffix[64] = {0};

    char *ipv4v6_content = NULL;
    char* pattern_v4 = "\\+CGCONTRDP: 1,5,\".*\",\".*\",\"(.*)\",\"(.*)\".*$";
    char* pattern_v6 = "\\+CGCONTRDP: 1,5,\".*\",\"(.*)\",\"(.*)\",\"(.*)\",\"(.*)\".*$";
    char ipv4v6_str[RECV_BUF_SIZE] = {0};

    int ret = 0;
    int ipv4_updated = 0;
    int ipv6_updated = 0;
    snprintf ( strRamOpt, sizeof ( strRamOpt ), LTE_APN_PDPTYPE, cid_index );
    sys_get_config(strRamOpt, apn_pdp_type, sizeof(apn_pdp_type));

    ipv4v6_content = strtok ( at_rep, "\r\n" );

    snprintf ( ipv4v6_str, sizeof(ipv4v6_str), "%s", ipv4v6_content );
    CLOGD ( FINE, "Get line:\n%s\n\n", ipv4v6_str );
    if ( strstr ( ipv4v6_str, "+CGCONTRDP: " ) )
    {
        if(strcmp(apn_pdp_type, "IP") == 0 || strcmp(apn_pdp_type, "IPV4V6") == 0)
        {
            ret = at_tok_regular_more ( ipv4v6_str, pattern_v4, regex_buf );
            if(ret == 0)
            {
                snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_dns1", cid_index );
                nv_set(strRamOpt, regex_buf[1]);
                snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_dns2", cid_index );
                nv_set(strRamOpt, regex_buf[2]);
            }

            if(strcmp(apn_pdp_type, "IPV4V6") == 0)
            {
                memset ( regex_buf, 0, sizeof ( regex_buf ) );
                ipv4v6_content = strtok ( NULL, "\r\n" );

                snprintf ( ipv4v6_str, sizeof(ipv4v6_str), "%s", ipv4v6_content );
                CLOGD ( FINE, "Get line:\n%s\n\n", ipv4v6_str );
                if ( strstr ( ipv4v6_str, "+CGCONTRDP: " ) )
                {
                    goto IPV6_FLAG;
                }
            }
        }
        else if(strcmp(apn_pdp_type, "IPV6") == 0)
        {
IPV6_FLAG:
            ret = at_tok_regular_more ( ipv4v6_str, pattern_v6, regex_buf );
            if(ret == 0)
            {
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

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_ip", cid_index );
                    nv_set ( strRamOpt, strcmp ( splice_ipv6, "" ) ? splice_ipv6 : regex_buf[1] );
                    CLOGD ( FINE, "%s->[%s]\n", strRamOpt, strcmp ( splice_ipv6, "" ) ? splice_ipv6 : regex_buf[1] );

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_mask", cid_index );
                    nv_set ( strRamOpt, "FFFF:FFFF:FFFF:FFFF::" );

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_gateway", cid_index );
                    nv_set ( strRamOpt, "fe80::1" );

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_dns1", cid_index );
                    nv_set ( strRamOpt, regex_buf[3] );
                    CLOGD ( FINE, "%s->[%s]\n", strRamOpt, regex_buf[3] );

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_dns2", cid_index );
                    nv_set ( strRamOpt, regex_buf[4] );
                    CLOGD ( FINE, "%s->[%s]\n", strRamOpt, regex_buf[4] );

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_state", cid_index );
                    nv_set ( strRamOpt, "connect" );
                }

            }
        }
    }

    if(strcmp(apn_pdp_type, "IP") == 0 || strcmp(apn_pdp_type, "IPV4V6") == 0)
    {
        memset(strRamOpt, 0, sizeof(strRamOpt));
        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ip", cid_index );
        nv_get(strRamOpt, nvram_ipv4_addr, sizeof(nvram_ipv4_addr));

        if ( strcmp ( ipv4_addr[ cid_index - 1 ], nvram_ipv4_addr ) )
        {
            snprintf ( ipv4_addr[ cid_index - 1 ], sizeof ( ipv4_addr[0] ), "%s", nvram_ipv4_addr );
            if ( 1 == apns_msg_flag[ cid_index ] )
            {
                apns_msg_flag[ cid_index ] = 0;
            }
        }
        ipv4_updated = 1;
    }

    if(strcmp(apn_pdp_type, "IPV6") == 0  || strcmp(apn_pdp_type, "IPV4V6") == 0)
    {
        memset(strRamOpt, 0, sizeof(strRamOpt));
        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_ip", cid_index );
        nv_get(strRamOpt, nvram_ipv6_addr, sizeof(nvram_ipv6_addr));

        if ( strcmp ( ipv6_addr[ cid_index - 1 ], nvram_ipv6_addr ) )
        {
            snprintf ( ipv6_addr[ cid_index - 1 ], sizeof ( ipv6_addr[0] ), "%s", nvram_ipv6_addr );
            if ( 1 == apns_msg_flag[ cid_index ] )
            {
                apns_msg_flag[ cid_index ] = 0;
            }
        }

        ipv6_updated = 1;
    }

    if ( 0 == ipv4_updated )
    {
        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ip", cid_index );
        nv_set ( strRamOpt, "" );
        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_mask", cid_index );
        nv_set ( strRamOpt, "" );
        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_gateway", cid_index );
        nv_set ( strRamOpt, "" );
        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_dns1", cid_index );
        nv_set ( strRamOpt, "" );
        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_dns2", cid_index );
        nv_set ( strRamOpt, "" );
        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_state", cid_index );
        nv_set ( strRamOpt, "disconnect" );
    }

    if ( 0 == ipv6_updated )
    {
        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_ip", cid_index );
        nv_set ( strRamOpt, "" );
        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_mask", cid_index );
        nv_set ( strRamOpt, "" );
        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_gateway", cid_index );
        nv_set ( strRamOpt, "" );
        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_dns1", cid_index );
        nv_set ( strRamOpt, "" );
        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_dns2", cid_index );
        nv_set ( strRamOpt, "" );
        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_state", cid_index );
        nv_set ( strRamOpt, "disconnect" );
    }

    return ( ipv4_updated | ipv6_updated );
}

void parsing_telit_pls_lockpin ( char* data )
{
    if ( strstr ( data, "\r\nOK\r\n" ) )
    {
        nv_set ( "lockpin_set", "0" );
    }
    else
    {
        nv_set ( "lockpin_set", "1" );
    }
}

void parsing_telit_pls_enterpin ( char* data )
{
    if ( strstr ( data, "\r\nOK\r\n" ) )
    {
        nv_set ( "enterpin_set", "0" );
    }
    else
    {
        nv_set ( "enterpin_set", "1" );
    }
}

void parsing_telit_pls_modifypin ( char* data )
{
    if ( strstr ( data, "\r\nOK\r\n" ) )
    {
        nv_set ( "modifypin_set", "0" );
    }
    else
    {
        nv_set ( "modifypin_set", "1" );
    }
}

void parsing_telit_pls_enterpuk ( char* data )
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

/*
 * +CCLK: "20/11/30,12:03:38+32"
 *      year/mon/day,hour:min:sec(+-)zone
 */
void parsing_telit_pls_cclk ( char* data )
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

    if ( 80 == time_s[0] || 0 == strcmp ( zone_type, "" ) )
    {
        nv_set ( "date_time", "" );
        CLOGD ( FINE, "NITZ time is unreasonable ...\n", time_s[0] );
        return;
    }

    gmt = nitz_time_total_second (
                time_s[0] + 2000, time_s[1], time_s[2],
                time_s[3], time_s[4], time_s[5]
            );

    SwitchLocalTime = gmtime ( &gmt );
    snprintf ( sCurrentTime, sizeof ( sCurrentTime ), "%4d-%02d-%02d %02d:%02d:%02d",
                ( SwitchLocalTime->tm_year + 1900 ), ( SwitchLocalTime->tm_mon + 1 ),
                SwitchLocalTime->tm_mday, SwitchLocalTime->tm_hour,
                SwitchLocalTime->tm_min, SwitchLocalTime->tm_sec
        );

    if ( 2021 < ( SwitchLocalTime->tm_year + 1900 ) )
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

}

void parsing_telit_pls_csq ( char* data )
{
    char rssi_str[8] = {0};
    int rssi = 0;
    sscanf(data, "\r\n+CSQ: %[^,],*", rssi_str);

    rssi = atoi(rssi_str);

    if(rssi == 99)
    {
        nv_set ( "rssi", "--" );
    }
    else
    {
        rssi = -113 + rssi*2;
        snprintf(rssi_str,sizeof(rssi_str),"%d",rssi);
        nv_set ( "rssi", rssi_str );
    }
}

void parsing_telit_pls_cpin_get ( char* data )
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

}

void parsing_telit_pls_clck_get ( char* data )
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
    else if(strstr(data, "PIN required") || strstr(data, "PUK required"))
    {
        nv_set ( "lockpin_status", "1" );
    }
    else
    {
        nv_set ( "lockpin_status", "" );
        CLOGD ( WARNING, "AT+CLCK=SC,2 return ERROR !\n" );
    }

    return;
}

void parsing_telit_pls_smonp (char* data)
{
    char strRamOpt[32] = {0};

    char neighbour_arfcn[16] = {0};
    char neighbour_rsrq[16] = {0};
    char neighbour_rsrp[16] = {0};
    char neighbour_rssi[16] = {0};

    char current_pci[16] = {0};
    int current_pci_int = 0;

    char neighbour_pci[16] = {0};
    long neighbour_pci_int = 0;

    char* pattern = "%[^,],%[^,],%[^,],%*[^,],%[^,],%[^,],*";
    char* neighbor_cell_ptr = NULL;

    int i = 0;
    int ret = 0;
    int neigbour_count = 0;
    neighbor_cell_ptr = strtok ( data, "\r\n" );

    nv_get("pci",current_pci,sizeof(current_pci));

    if(strcmp(current_pci,"") == 0 || strcmp(current_pci,"--") == 0)
    {
        return;
    }

    current_pci_int = atoi(current_pci);

    while ( neighbor_cell_ptr )
    {
        if(strncmp(neighbor_cell_ptr,"3G",2) == 0 || strncmp(neighbor_cell_ptr,"2G",2) == 0)
        {
            break;
        }
        else if(strncmp(neighbor_cell_ptr,"4G",2) == 0)
        {
            neighbor_cell_ptr = strtok ( NULL, "\r\n" );
            continue;
        }

        ret = sscanf(neighbor_cell_ptr,pattern,neighbour_arfcn,neighbour_rsrq,neighbour_rsrp,neighbour_pci,neighbour_rssi);

        if ( 5 == ret )
        {
            current_pci_int = atoi(neighbour_pci);
            if((neighbour_pci_int == current_pci_int) )
            {
                neighbor_cell_ptr = strtok ( NULL, "\r\n" );
                continue;
            }

            CLOGD ( FINE, "neighbor_count -> [%d]\n", ++neigbour_count );
            snprintf ( strRamOpt, sizeof ( strRamOpt ), "neighbor_rsrp_%d", neigbour_count );
            nv_set ( strRamOpt, neighbour_rsrp );

            snprintf ( strRamOpt, sizeof ( strRamOpt ), "neighbor_rsrq_%d", neigbour_count );
            nv_set ( strRamOpt, neighbour_rsrq );

            snprintf ( strRamOpt, sizeof ( strRamOpt ), "neighbor_rssi_%d", neigbour_count );
            nv_set ( strRamOpt, neighbour_rssi );

            snprintf ( strRamOpt, sizeof ( strRamOpt ), "neighbor_pci_%d", neigbour_count );
            nv_set ( strRamOpt, neighbour_pci );

            snprintf ( strRamOpt, sizeof ( strRamOpt ), "neighbor_earfcn_%d", neigbour_count );
            nv_set ( strRamOpt, neighbour_arfcn );

            snprintf ( strRamOpt, sizeof ( strRamOpt ), "neighbor_sinr_%d", neigbour_count );
            nv_set ( strRamOpt, "--" );

        }
        neighbor_cell_ptr = strtok ( NULL, "\r\n" );
    }
}

int parsing_telit_pls_cfun_get ( char* data )
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

