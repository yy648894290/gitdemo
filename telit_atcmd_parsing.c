#include "comd_share.h"
#include "atchannel.h"
#include "at_tok.h"
#include "comd_ip_helper.h"
#include "telit_atcmd_parsing.h"
#include "config_key.h"
#include "hal_platform.h"

extern CELL_EARFCN_FREQ earfcn_freq;
extern int apns_msg_flag[5];
extern COMD_DEV_INFO at_dev;

static void clear_telit_4g_cellinfo (void)
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

static void clear_telit_5g_cellinfo (void)
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
    nv_set( "5g_cqi", "--" );
    nv_set( "5g_dlmcs", "--" );
    nv_set( "5g_ulmcs", "--" );

}
int parsing_telit_cfun_set ( char* data )
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

int parsing_telit_moduleModel ( char* data )
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

int parsing_telit_moduleVersion ( char* data )
{
    char regex_buf[4][REGEX_BUF_ONE] = {0};
    int ret = 0;
    char *pattern = NULL;

    pattern = "[\r\n]([A-z0-9]+\.[A-z0-9]+)";
    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "\r\n" ), pattern, regex_buf );

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

int parsing_telit_imei ( char* data )
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

int parsing_telit_sn ( char* data )
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

int parsing_telit_bandlist( char* data, char* lte_band_list, int size_band_4g, char* nr_band_list, int size_band_5g )
{
    char regex_buf[16][512] = {0};
    int ret = 0;

    char band_4g_list_hex[32] = {0};
    char band_5g_list_hex[32] = {0};
    char band_3g_recover_str[32] = {0};
    char band_4g_recover_str[32] = {0};
    char band_5g_nsa_recover_str[32] = {0};
    char band_5g_sa_recover_str[32] = {0};
    int i = 0;
    char band_pad[16] = {0};
    int band_standard_num = 0;

    if( at_dev.comd_mVendor == M_VENDOR_TELIT_5G )
    {
        band_standard_num = 7;
    }
    else
    {
        band_standard_num = 3;
    }

    //char* pattern = "\\([A-z0-9]+\\),\\([A-z0-9]+\\),\\([A-z0-9]+\\),\\([A-z0-9]+\\),\\([A-z0-9]+\\),\\([A-z0-9]+\\)";

    //"#BND: (0),(0-11,22,23),(A7E2BB0F38DF),(42),(1A0290800D7),(7042),(81A0290800D7),(7442)";
    ret = sscanf(strstr(data, "#BND"), "#BND: (%[^)]),(%*[^)]),(%[^)]),(%[^)]),(%[^)]),(%[^)]),(%[^)]),(%[^)])",
            regex_buf[0],regex_buf[1],regex_buf[2],regex_buf[3],regex_buf[4],regex_buf[5],regex_buf[6]);

    if ( band_standard_num == ret )
    {
        CLOGD(FINE, "0:[%s],1:[%s],2:[%s],3:[%s],4:[%s],5:[%s],6:[%s]",
            regex_buf[0],regex_buf[1],regex_buf[2],regex_buf[3],regex_buf[4],regex_buf[5],regex_buf[6]);

        snprintf(band_3g_recover_str, sizeof(band_3g_recover_str), "0,0");
        snprintf(band_4g_recover_str, sizeof(band_4g_recover_str), "%s,%s", regex_buf[1],regex_buf[2]);
        if( at_dev.comd_mVendor == M_VENDOR_TELIT_5G )
        {

            snprintf(band_5g_nsa_recover_str, sizeof(band_5g_nsa_recover_str), "%s,%s", regex_buf[3],regex_buf[4]);
            snprintf(band_5g_sa_recover_str, sizeof(band_5g_sa_recover_str), "%s,%s", regex_buf[5],regex_buf[6]);
        }

        nv_set("band_3g_recover_str", band_3g_recover_str);
        nv_set("band_4g_recover_str", band_4g_recover_str);
        nv_set("band_5g_nsa_recover_str", band_5g_nsa_recover_str);
        nv_set("band_5g_sa_recover_str", band_5g_sa_recover_str);

        for(i = 0;i < (16 - strlen(regex_buf[1])); i++)
        {
            band_pad[i] = '0';
        }

        sprintf(band_4g_list_hex,"%s%s%s",regex_buf[2],band_pad,regex_buf[1]);
        for(i = 0;i < (16 - strlen(regex_buf[5])); i++)
        {
            band_pad[i] = '0';
        }


        if( at_dev.comd_mVendor == M_VENDOR_TELIT_5G )
        {
            sprintf(band_5g_list_hex,"%s%s%s",regex_buf[6],band_pad,regex_buf[5]);
        }

        ret = telit_bandlist_hexstr_to_strlist(band_4g_list_hex,band_5g_list_hex,lte_band_list,size_band_4g,nr_band_list,size_band_5g);

    }
    else
    {
        nv_set ( "suppband", "" );
        nv_set ( "suppband_org", "" );
        nv_set ( "suppband5g", "" );
        nv_set ( "suppband5g_org", "" );
        CLOGD ( ERROR, "GET SUPPBAND return wrong value!\n" );
    }

    return ret;
}

void parsing_telit_cpin_get ( char* data )
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

void parsing_telit_clck_get ( char* data )
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

void parsing_telit_cpinr ( char* data )
{
    int ret = 0;
    char *pattern = NULL;
    char regex_buf[8][REGEX_BUF_ONE] = {0};

    pattern = "\\+CPINR: SIM PIN,([0-9]+),([0-9]+).*+CPINR: SIM PUK,([0-9]+),([0-9]+).*OK.*$";

    ret = at_tok_regular_more ( strstr ( data, "+CPINR:" ), pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( "pin_times", regex_buf[1] );
        nv_set ( "puk_times", regex_buf[3] );
    }
    else
    {
        nv_set ( "pin_times", "3" );
        nv_set ( "puk_times", "10" );
        CLOGD ( WARNING, "getPinPukTimes fail !\n" );
    }

}

void parsing_telit_cimi ( char* data )
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

void parsing_telit_sim_spn ( char* data )
{
    char regex_buf[2][REGEX_BUF_ONE];
    int ret = 0;
    char *pattern = NULL;
    char SIM_SPN[64] = {0};

    pattern = "\\+CRSM: [0-9]+,[0-9]+,([A-z0-9]+).*$";

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

void parsing_telit_simMncLen ( char* data )
{
    int ret = 0;
    int buf_len = 0;
    char *pattern = NULL;
    char regex_buf[2][REGEX_BUF_ONE];
    char imsi_val[32] = {0};
    char usim_mcc[4] = {0};
    char usim_mnc[4] = {0};

    pattern = "\\+CRSM: [0-9]+,[0-9]+,([0-9]+).*OK.*$";

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

void parsing_telit_iccid ( char* data )
{
    char regex_buf[4][REGEX_BUF_ONE] = {0};
    int i = 0;
    int ret = 0;
    char *pattern = NULL;

    pattern = "\\+CRSM: [0-9]+,[0-9]+,([0-9]+).*OK.*$";

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

void parsing_telit_cnum ( char* data )
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

void parsing_telit_cesq( char* data )
{
    int cesq_param[9] = { 99, 99, 255, 255, 255, 255, 255, 255, 255 };
    char cesq_val[64] = {0};

    if ( NULL == strstr ( data, "+CESQ:" ) )
        return;

    sscanf ( strstr ( data, "+CESQ:" ), "+CESQ: %d,%d,%d,%d,%d,%d,%d,%d,%d\r\n",
                    &cesq_param[0], &cesq_param[1], &cesq_param[2],
                    &cesq_param[3], &cesq_param[4], &cesq_param[5],
                    &cesq_param[6], &cesq_param[7], &cesq_param[8] );

    snprintf ( cesq_val, sizeof ( cesq_val ), "%d,%d,%d,%d,%d,%d,%d,%d,%d",
                    cesq_param[0], cesq_param[1], cesq_param[2],
                    cesq_param[3], cesq_param[4], cesq_param[5],
                    cesq_param[6], cesq_param[7], cesq_param[8] );

    nv_set ( "cesq", cesq_val );
    snprintf(cesq_val, sizeof(cesq_val), "%d", cesq_param[8]);
    nv_set ( "5g_orig_sinr", cesq_val );

}

void parsing_telit_operator ( char* data )
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

int parsing_telit_cgatt ( char* data )
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

void parsing_telit_serving_4gcellinfo ( char* data )
{
    CLOGD ( FINE, "start 4g servingcell parsing ...\n" );

    int ret = 0;

    float lte_sinr_calc = 0;
    char lte_earfcn[16] = {0};
    char lte_band[16] = {0};
    char lte_bandwidth[16] = {0};
    char lte_plmn[16] = {0};
    char lte_tac[16] = {0};
    char lte_cellid_pci[16] = {0};
    char lte_rsrp[16] = {0};
    char lte_rsrq[16] = {0};
    char lte_rssi[16] = {0};
    char lte_cqi[16] = {0};
    char lte_sinr[16] = {0};
    char lte_rrc[16] = {0};
    char lte_txpwr[16] = {0};
    char lte_earfcn_dl[16] = {0};
    char lte_earfcn_ul[16] = {0};
    char lte_phy_cell_id[8] = {0};
    char lte_cell_id[8] = {0};
    char lte_mcc[8] = {0};
    char lte_mnc[8] = {0};
    char strRamVal[16] = {0};
    long e_utran_cellid = 0;
    int real_cell_Id = 0;
    //                              earfcn band   bw  plmn   tac  cellid esmc  drx    rsrp  rsrq   rssi   l2w   ri    cqi
    ret = sscanf(data, "\r\n#LTEDS: %[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%*[^,],%*[^,],%[^,],%[^,],%[^,],%*[^,],%*[^,],%[^,], \
                        %*[^,],%*[^,],%[^,],%*[^,],%[^,],%[^,],*",
    //                  status sub-s  rrc   svc    sinr  txpowr
                        lte_earfcn,lte_band,lte_bandwidth,lte_plmn,lte_tac,lte_cellid_pci,lte_rsrp,lte_rsrq,lte_rssi,lte_cqi,
                        lte_rrc,lte_sinr,lte_txpwr);
    CLOGD(FINE, "ret = %d",ret);
    CLOGD(FINE, "lte_earfcn -> [%s]\n \
                 lte_band -> [%s]\n \
                 lte_bandwidth -> [%s]\n \
                 lte_plmn -> [%s]\n \
                 lte_tac -> [%s]\n \
                 lte_cellid -> [%s] \n \
                 lte_rsrp -> [%s]\n \
                 lte_rsrq -> [%s]\n \
                 lte_rssi -> [%s]\n \
                 lte_cqi -> [%s]\n \
                 lte_rrc -> [%s]\n \
                 lte_sinr -> [%s]\n \
                 lte_txpwr -> [%s]\n",
                 lte_earfcn,lte_band,lte_bandwidth,lte_plmn,lte_tac,lte_cellid_pci,lte_rsrp,lte_rsrq,lte_rssi,lte_cqi,
                 lte_rrc,lte_sinr,lte_txpwr);

    if(ret == 12)
    {
        sscanf(lte_earfcn, "%[^/]/%s",lte_earfcn_dl ,lte_earfcn_ul);
        CLOGD(FINE, "lte_earfcn_dl -> [%s]\nlte_earfcn_ul -> [%s]", lte_earfcn_dl ,lte_earfcn_ul);

        sscanf(lte_cellid_pci, "%[^(](%[^)])",lte_cell_id, lte_phy_cell_id);
        CLOGD(FINE, "lte_cell_id -> [%s]\nlte_phy_cell_id -> [%s]\n", lte_cell_id, lte_phy_cell_id);

        sscanf(lte_plmn, "%s %s",lte_mcc, lte_mnc);
        nv_set ( "mcc", lte_mcc );
        nv_set ( "mnc", lte_mnc );
        nv_set ( "dl_earfcn", lte_earfcn_dl );
        nv_set ( "ul_earfcn", lte_earfcn_ul );
        nv_set ( "pci", lte_phy_cell_id );
        e_utran_cellid = strtol(lte_cell_id, NULL, 16);
        real_cell_Id = e_utran_cellid & 0xff;
        snprintf(lte_cell_id,sizeof(lte_cell_id),"%x",real_cell_Id);

        nv_set ( "cellid", lte_cell_id );
        nv_set ( "band", lte_band );

        earfcn_freq.dl_earfcn = atoi ( lte_earfcn_dl );
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

        switch ( atoi ( lte_bandwidth ) )
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
                snprintf ( strRamVal, sizeof ( strRamVal ), "%d", ( atoi ( lte_bandwidth ) - 1 ) * 5 );
            break;
            default:
                CLOGD ( WARNING, "Invalid DL bandwidth value -> [%d]\n", atoi ( lte_bandwidth ) );
                snprintf ( strRamVal, sizeof ( strRamVal ), "%s", "--" );
            break;
        }

        switch ( atoi ( lte_rrc ) )
        {
            case 2:
                nv_set ( "rrc_state", "CONNECTED" );
                break;
            default:
                nv_set ( "rrc_state", "IDLE" );
                break;
        }

        nv_set ( "bandwidth", strRamVal );
        nv_set ( "dl_bandwidth", strRamVal );

        nv_set ( "ul_bandwidth", strRamVal );

        nv_set ( "tac", lte_tac );

        nv_set ( "rsrp0", lte_rsrp );
        nv_set ( "rsrp1", lte_rsrp );
        nv_set ( "rsrq", lte_rsrq );
        nv_set ( "rssi", lte_rssi );
        nv_set ( "cqi", lte_cqi );

        lte_sinr_calc = (float)(atoi(lte_sinr)*0.2) - 20;

        snprintf(lte_sinr,sizeof(lte_sinr),"%0.1f",lte_sinr_calc);
        nv_set ( "sinr", lte_sinr );
        nv_set ( "txpower", "--" );

        nv_set ( "network_mode", "4G" );
    }
    else
    {
        clear_telit_4g_cellinfo();
        nv_set ( "rrc_state", "--" );
    }

}

int parsing_telit_serving_5gcellinfo_basic ( char* data )
{
    CLOGD ( FINE, "start 5g servingcell basic parsing ...\n" );
    int ret = 0;
    int i = 0;
    char nr_bandwidth[16] = {0};
    char nr_dl_bandwidth[16] = {0};
    char nr_band[16] = {0};
    char nr_mcc[16] = {0};
    char nr_mnc[16] = {0};
    char nr_ul_earfcn[16] = {0};
    char nr_dl_earfcn[16] = {0};
    char nr_rsrp[16] = {0};
    char nr_rsrq[16] = {0};
    char nr_rssi[16] = {0};
    char nr_txpower[16] = {0};
    char strRamVal[32] = {0};

    char network_state[8] = {0};

    nv_get("network_mode",network_state,sizeof(network_state));

    if(strcmp(network_state,"ENDC") == 0)
    {
        ret = sscanf(data, "\r\n#RFSTS: \"%s %[^\"]\",%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],\
%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^\r]",
                                nr_mcc,nr_mnc,nr_dl_earfcn,nr_ul_earfcn,nr_rsrp,nr_rssi,nr_rsrq,nr_band,nr_bandwidth,
                                nr_dl_bandwidth,nr_txpower);
    }
    else
    {
        ret = sscanf(data, "\r\n#RFSTS: \"%s %[^\"]\",%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^\r]*",
                    nr_mcc,nr_mnc,nr_dl_earfcn,nr_ul_earfcn,nr_rsrp,nr_rssi,nr_rsrq,nr_band,nr_bandwidth,
                    nr_dl_bandwidth,nr_txpower);
    }
    CLOGD(FINE, "ret = %d",ret);
    CLOGD(FINE, "nr_mcc -> [%s]\n \
                 nr_mnc-> [%s]\n \
                 nr_dl_earfcn -> [%s]\n \
                 nr_ul_earfcn -> [%s]\n \
                 nr_rsrp -> [%s]\n \
                 nr_rssi -> [%s]\n \
                 nr_rsrq -> [%s]\n \
                 nr_band -> [%s]\n \
                 nr_bandwidth -> [%s]\n \
                 nr_dl_bandwidth -> [%s]\n \
                 nr_txpower -> [%s]",
                 nr_mcc,nr_mnc,nr_dl_earfcn,nr_ul_earfcn,nr_rsrp,nr_rssi,nr_rsrq,nr_band,nr_bandwidth,nr_dl_bandwidth,
                 nr_txpower);

        if(ret == 11)
        {
            nv_set ( "5g_mcc", nr_mcc );
            nv_set ( "5g_mnc", nr_mnc );
            nv_set ( "5g_band", nr_band );
            nv_set ( "5g_dl_earfcn", nr_dl_earfcn );
            nv_set ( "5g_ul_earfcn", nr_ul_earfcn );
            earfcn_freq.band = atoi ( nr_band );
            earfcn_freq.dl_earfcn = atoi ( nr_dl_earfcn );

            if ( 0 == calc_5g_freq_from_narfcn ( &earfcn_freq ) )
            {
                snprintf ( strRamVal, sizeof ( strRamVal ), "%d.%03d", earfcn_freq.dl_frequency / 1000, earfcn_freq.dl_frequency % 1000 );
                nv_set ( "5g_dl_frequency", strRamVal );
                snprintf ( strRamVal, sizeof ( strRamVal ), "%d.%03d", earfcn_freq.ul_frequency / 1000, earfcn_freq.ul_frequency % 1000 );
                nv_set ( "5g_ul_frequency", strRamVal );
            }

            nv_set ( "5g_bandwidth", nr_bandwidth );
            nv_set ( "5g_dl_bandwidth", nr_dl_bandwidth );
            nv_set ( "5g_ul_bandwidth", nr_bandwidth );
            nv_set ( "5g_txpower", nr_txpower );

            nv_set ( "5g_rsrp0", nr_rsrp );
            nv_set ( "5g_rsrp1", nr_rsrp );
            nv_set ( "5g_rsrq", nr_rsrq );
            nv_set ( "5g_rssi", nr_rssi );
            ret = 0;
        }
        else
        {
            clear_telit_5g_cellinfo();
            ret = 1;
        }

   return ret;
}
int parsing_telit_serving_5gcellinfo_adv ( char* data )
{
    CLOGD ( FINE, "start 5g servingcell advance parsing ...\n" );

    int ret = 0;
    int i = 0;
    float nr_sinr_calc = 0;
    char nr_pci[16] = {0};
    char nr_cqi[16] = {0};
    char nr_sinr[16] = {0};
    char endc_state[8] = {0};
    ret = sscanf(data, "\r\n#NRDS: %*[^,],%*[^,],%*[^,],%[^,],%*[^,],%*[^,],%*[^,],%[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],\
                        %*[^,],%*[^,],%[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],\
                        %*[^,],%*[^,],%*[^,],%*[^,],%*[^,],*",
                        nr_pci,nr_cqi,endc_state);
    CLOGD(FINE, "ret = %d",ret);
    CLOGD(FINE, "nr_pci -> [%s]\n \
                 nr_cqi-> [%s]\n \
                 nr_endc_state-> [%s]\n",
                 nr_pci,nr_cqi,endc_state);

    if(ret == 3)
    {
        nv_set ( "5g_pci", nr_pci );
        nv_set ( "5g_cqi", nr_cqi );

        nv_get("5g_orig_sinr",nr_sinr,sizeof(nr_sinr));

        nr_sinr_calc = (float)(atoi(nr_sinr)*0.5) - 23;
        snprintf(nr_sinr,sizeof(nr_sinr),"%0.1f",nr_sinr_calc);

        nv_set ( "5g_sinr", nr_sinr );

        if(atoi(endc_state) == 1)
        {
            nv_set ( "network_mode", "ENDC" );
        }
        else
        {
            nv_set ( "network_mode", "5G" );
        }
        ret = 0;
    }
    else
    {
        clear_telit_5g_cellinfo();
        ret = 1;
    }

    return ret;
}

void parsing_telit_neighbour_cell( char* data, const char* ca_mode, int* neigbour_count )
{
    char regex_buf[8][REGEX_BUF_ONE] = {0};
    char strRamOpt[32] = {0};
    char current_pci[16] = {0};
    int current_pci_int = 0;

    char neighbour_pci[16] = {0};
    long neighbour_pci_int = 0;

    char* pattern = "\\#MONI: RSRP:([-[0-9]+) RSRQ:([-[0-9]+) Id:([[A-z0-9]+) EARFCN:([[A-z0-9]+).*";
    char* neighbor_cell_ptr = NULL;
    int i = 0;
    int ret = 0;
    neighbor_cell_ptr = strtok ( data, "\r\n" );

    nv_get("pci",current_pci,sizeof(current_pci));

    if(strcmp(current_pci,"") == 0 || strcmp(current_pci,"--") == 0)
    {
        return;
    }

    current_pci_int = atoi(current_pci);

    while ( neighbor_cell_ptr )
    {
        if(strncmp(neighbor_cell_ptr,"OK",2) == 0)
        {
            break;
        }
        for ( i = 0; i < 8; i++ )
        {
            memset ( regex_buf[i], 0, REGEX_BUF_ONE );
        }

        ret = at_tok_regular_more ( neighbor_cell_ptr, pattern, regex_buf );

        if ( 0 == ret )
        {
            neighbour_pci_int = strtol(regex_buf[3], NULL, 16);

            /* Now I just know that AT#MONI=1 return back intra-band cell info could include main cell
            *  We need to kick main cell out from the neighbour list
            */
            if( (strcmp(ca_mode,"intra")) == 0 && (neighbour_pci_int == current_pci_int) )
            {
                neighbor_cell_ptr = strtok ( NULL, "\r\n" );
                continue;
            }

            CLOGD ( FINE, "neighbor_count -> [%d]\n", ++*neigbour_count );
            snprintf ( strRamOpt, sizeof ( strRamOpt ), "neighbor_rsrp_%d", *neigbour_count );
            nv_set ( strRamOpt, regex_buf[1] );

            snprintf ( strRamOpt, sizeof ( strRamOpt ), "neighbor_rsrq_%d", *neigbour_count );
            nv_set ( strRamOpt, regex_buf[2] );

            snprintf ( strRamOpt, sizeof ( strRamOpt ), "neighbor_pci_%d", *neigbour_count );
            nv_set ( strRamOpt, regex_buf[3] );

            snprintf ( strRamOpt, sizeof ( strRamOpt ), "neighbor_earfcn_%d", *neigbour_count );
            nv_set ( strRamOpt, regex_buf[4] );

        }
        neighbor_cell_ptr = strtok ( NULL, "\r\n" );
    }
}

int parsing_telti_cereg ( char* data )
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

int parsing_telti_c5greg ( char* data )
{
    char c5greg_mode[16] = {0};
    char c5greg_state[16] = {0};
    char c5greg_tac[16] = {0};
    char c5greg_cell_id[16] = {0};

    int ret = 0;


    ret = sscanf(data, "\r\n+C5GREG: %[^,],%[^,],\"%[^\"]\",\"%[^\"],*",
                            c5greg_mode,c5greg_state,c5greg_tac,c5greg_cell_id);

    if ( ret == 4 )
    {
        CLOGD ( FINE, "c5greg_stat -> [%s]\n \
                       5g_tac -> [%s]\n \
                       5g_cellid -> [%s]\n",
                       c5greg_state, c5greg_tac, c5greg_cell_id);
        nv_set ( "c5greg_stat", c5greg_state );
        nv_set ( "5g_tac", c5greg_tac );
        nv_set ( "5g_cellid", c5greg_cell_id );
        ret = 0;
    }
    else if(ret == 2)
    {
        ret = sscanf(data, "\r\n+C5GREG: %[^,],%[^\r]*",c5greg_mode,c5greg_state);
        nv_set ( "c5greg_stat", c5greg_state );
        ret = 0;
    }
    else
    {
        CLOGD ( WARNING, "AT+C5GREG? regex error !\n" );
        ret = 1;
    }

    return ret;
}

void parsing_telit_cgact ( char* data )
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

int parsing_telit_cgcontrdp ( int cid_index )
{
    static char ipv4_addr[8][REGEX_BUF_ONE];
    static char ipv6_addr[8][REGEX_BUF_ONE];

    char nvram_ipv4_addr[32] = {0};
    char nvram_ipv6_addr[128] = {0};
    char apn_pdp_type[8] = {0};
    char strRamOpt[128] = {0};
    char splice_ipv6[64] = {0};
    char ipv6_prefix[64] = {0};
    char ipv6_suffix[64] = {0};

    int ipv4_updated = 0;
    int ipv6_updated = 0;
    snprintf ( strRamOpt, sizeof ( strRamOpt ), LTE_APN_PDPTYPE, cid_index );
    sys_get_config(strRamOpt, apn_pdp_type, sizeof(apn_pdp_type));


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

//I didn't find anyone required to get nvram of apn%d_ipv6_suffix, why other part of the code write it down?
#if 0
        if ( 0 == calc_ipv6_addr_prefix ( nvram_ipv6_addr, 64, ipv6_prefix ) )
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
#endif
        ipv6_updated = 1;
    }

    //Now I dont know what happened for apn state whether is updated successful or not by using orig QMI system to dial up Modem.
    //Need fixed in the future maybe.

    return ( ipv4_updated | ipv6_updated );
}

void parsing_telit_lockpin ( char* data )
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

void parsing_telit_enterpin ( char* data )
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

void parsing_telit_modifypin ( char* data )
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

void parsing_telit_enterpuk ( char* data )
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
void parsing_telit_cclk ( char* data )
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

void parsing_telit_serving_5g_linkstat ( char* data )
{
    char nr_rrc[8] = {0};

    sscanf(data, "\r\n#5GLINKSTAT: %*[^,],%*[^,],%[^\r]", nr_rrc);

    nv_set ( "nr5g_rrc_state", nr_rrc );
}

int parsing_telit_cfun_get ( char* data )
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

