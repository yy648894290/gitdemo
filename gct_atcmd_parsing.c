#include "comd_share.h"
#include "atchannel.h"
#include "at_tok.h"
#include "comd_ip_helper.h"
#include "gct_atcmd_parsing.h"
#include "pdn_client_oper.h"
#include "config_key.h"
#include "hal_platform.h"

extern char led_mode[4];
extern int allSms_sem_id;
extern LTEINFO_AT_MSG pccInfo;

extern int apns_msg_flag[5];

static char old_mcc[8] = {0};
static char old_mnc[8] = {0};

static char g_strPinSt[STR_AT_RSP_LEN];
static char g_strPinPuk[STR_AT_RSP_LEN];
static char g_strCesq[STR_AT_RSP_LEN];
static char g_strCereg[STR_AT_RSP_LEN];
static char g_strCgatt[STR_AT_RSP_LEN];
static char g_strCgact[STR_AT_RSP_LEN];
static char g_strRankIndex[STR_AT_RSP_LEN];
static char g_strCqi[STR_AT_RSP_LEN_2X];
static char g_strQam[STR_AT_RSP_LEN];
static char g_strBler[STR_AT_RSP_LEN];
static char g_strHarq[STR_AT_RSP_LEN];
static char g_strDefbrdp[4][STR_AT_RSP_LEN];
static char g_strIp4and6[4][STR_AT_RSP_LEN_4X];

void memsetGctParsingStr()
{
    memset ( old_mcc, 0, sizeof ( old_mcc ) );
    memset ( old_mnc, 0, sizeof ( old_mnc ) );

    memset ( g_strPinSt, 0, sizeof ( g_strPinSt ) );
    memset ( g_strPinPuk, 0, sizeof ( g_strPinPuk ) );
    memset ( g_strCesq, 0, sizeof ( g_strCesq ) );
    memset ( g_strCereg, 0, sizeof ( g_strCereg ) );
    memset ( g_strCgatt, 0, sizeof ( g_strCgatt ) );
    memset ( g_strCgact, 0, sizeof ( g_strCgact ) );
    memset ( g_strRankIndex, 0, sizeof ( g_strRankIndex ) );
    memset ( g_strCqi, 0, sizeof ( g_strCqi ) );
    memset ( g_strQam, 0, sizeof ( g_strQam ) );
    memset ( g_strBler, 0, sizeof ( g_strBler ) );
    memset ( g_strHarq, 0, sizeof ( g_strHarq ) );

    int i = 0;

    for ( i = 0; i < 4; i++ )
    {
        memset ( g_strDefbrdp[i], 0, STR_AT_RSP_LEN );
        memset ( g_strIp4and6[i], 0, STR_AT_RSP_LEN_4X );
    }
}

static void get_secondary_cell_bandwidth ( char* S_bandwidth, char* data )
{
    if ( NULL == data )
        return;

    switch ( atoi ( data ) )
    {
    case 5:
        nv_set ( S_bandwidth, "20" );
        break;
    case 4:
        nv_set ( S_bandwidth, "15" );
        break;
    case 3:
        nv_set ( S_bandwidth, "10" );
        break;
    case 2:
        nv_set ( S_bandwidth, "5" );
        break;
    case 1:
        nv_set ( S_bandwidth, "3" );
        break;
    case 0:
        nv_set ( S_bandwidth, "1.4" );
        break;

    default:
        nv_set ( S_bandwidth, "0" );
        break;
    }
}

static void get_secondary_cell_mcs ( int scc_num, char* S_dlmcs, char* data )
{
    int p_cw0_mcs[32] = {0};
    int i = 0, maxMcs = 0, maxMcsVal = 0;
    char mcs_max[4] = {0};
    static char old_data[3][128] = {{0}, {0}, {0}};

    if ( 0 == strcmp ( old_data[scc_num-1], data ) )
        return;
    strcpy ( old_data[scc_num-1], data );

    sscanf ( data, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
             &p_cw0_mcs[0], &p_cw0_mcs[1], &p_cw0_mcs[2], &p_cw0_mcs[3],
             &p_cw0_mcs[4], &p_cw0_mcs[5], &p_cw0_mcs[6], &p_cw0_mcs[7],
             &p_cw0_mcs[8], &p_cw0_mcs[9], &p_cw0_mcs[10], &p_cw0_mcs[11],
             &p_cw0_mcs[12], &p_cw0_mcs[13], &p_cw0_mcs[14], &p_cw0_mcs[15],
             &p_cw0_mcs[16], &p_cw0_mcs[17], &p_cw0_mcs[18], &p_cw0_mcs[19],
             &p_cw0_mcs[20], &p_cw0_mcs[21], &p_cw0_mcs[22], &p_cw0_mcs[23],
             &p_cw0_mcs[24], &p_cw0_mcs[25], &p_cw0_mcs[26], &p_cw0_mcs[27],
             &p_cw0_mcs[28], &p_cw0_mcs[29], &p_cw0_mcs[30], &p_cw0_mcs[31] );

    for ( i = 0; i < 32; i++ )
    {
        if ( p_cw0_mcs[i] > maxMcsVal )
        {
            maxMcsVal = p_cw0_mcs[i];
            maxMcs = i;
        }
    }

    sprintf ( mcs_max, "%d", maxMcs );

    nv_set ( S_dlmcs, mcs_max );
}

static void findOperatorName ( const char* plmn, char* buffer, int buf_len )
{
    if ( plmn == NULL || 0 == strcmp ( plmn, "" ) )
    {
        snprintf ( buffer, buf_len, "%s", "--" );
        return;
    }

    int i = 0;
    char plmn_val[8] = {0};

    for ( ; i < MAX_NUM_OP_NAME; i++ )
    {
        snprintf ( plmn_val, sizeof ( plmn_val ), "%s%s",
                        operatorName[i].pMCC, operatorName[i].pMNC );
        if ( 0 == strcmp ( plmn, plmn_val ) )
        {
            snprintf ( buffer, buf_len, "%s", operatorName[i].pLongName );
            break;
        }
    }

    if ( 0 == strcmp ( buffer, "" ) )
    {
        snprintf ( buffer, buf_len, "%s", plmn );
    }

    return;
}

static void gctParam_getOperator ( char* mcc, char* mnc )
{
    char plmn[8] = {0};
    char buffer[64] = {0};
    char usim_mcc[4] = {0};
    char usim_mnc[4] = {0};
    char usim_plmn[8] = {0};
    char buffer_usim[64] = {0};
    char operator_val[64] = {0};
    char SIM_SPN[64] = {0};

    snprintf ( plmn, sizeof ( plmn ), "%s%s", mcc, mnc );

    nv_get ( "SIM_SPN", SIM_SPN, sizeof ( SIM_SPN ) );

    if ( 0 == strcmp ( plmn, "" ) )
    {
        nv_set ( "operator", SIM_SPN );
        return;
    }

    findOperatorName ( plmn, buffer, sizeof ( buffer ) );
    nv_set ( "operator_eNB", buffer );

    nv_get ( "usim_mcc", usim_mcc, sizeof ( usim_mcc ) );
    nv_get ( "usim_mnc", usim_mnc, sizeof ( usim_mnc ) );
    snprintf ( usim_plmn, sizeof ( usim_plmn ), "%s%s", usim_mcc, usim_mnc );

    if ( strcmp ( usim_plmn, plmn ) )
    {
        findOperatorName ( usim_plmn, buffer_usim, sizeof ( buffer_usim ) );
        if ( strcmp ( buffer, buffer_usim ) )
        {
            if ( 0 == strcmp(SIM_SPN, "--") || 0 == strcmp ( SIM_SPN, "" ) )
            {
                snprintf ( operator_val, sizeof ( operator_val ),
                                    "%s(%s)", buffer_usim, buffer );
            }
            else
            {
                snprintf ( operator_val, sizeof ( operator_val ),
                                        "%s(%s)", SIM_SPN, buffer );
            }

            nv_set ( "operator", operator_val );
            return;
        }
    }

    if ( 0 == strcmp ( SIM_SPN, "--" ) || 0 == strcmp ( SIM_SPN, "" ) )
    {
        nv_set ( "operator", buffer );
    }
    else
    {
        nv_set ( "operator", SIM_SPN );
    }

    return;
}

static void tac_lock_function ( char* tac_val )
{
    return;
}

static void get_cell_id ( const char* globalID, char* cellid, char* eNBid )
{
    int id = 0;
    int id1 = 0;
    int id2 = 0;

    id = atoi ( globalID );

    id1 = id & 0x000000FF;

    sprintf ( cellid, "%d", id1 );

    id2 = id >> 8;

    sprintf ( eNBid, "%d", id2 );
}

static void calc_signal_bar ( const char* sinr )
{
    char _rsrp0[8] = {0};
    char _rsrp1[8] = {0};
    int rsrp_val = 0;
    int sinr_val = 0;

    switch ( atoi ( led_mode ) )
    {
    case 0:
        sinr_val = atoi ( sinr );

        if ( 13 <= sinr_val )
            nv_set ( "signal_bar", "4" );
        else if ( 7 <= sinr_val )
            nv_set ( "signal_bar", "3" );
        else if ( 1 <= sinr_val )
            nv_set ( "signal_bar", "2" );
        else if ( -3 <= sinr_val )
            nv_set ( "signal_bar", "1" );
        else
            nv_set ( "signal_bar", "0" );
        break;
    default:
        nv_get ( "rsrp0", _rsrp0, sizeof ( _rsrp0 ) );
        nv_get ( "rsrp1", _rsrp1, sizeof ( _rsrp1 ) );
        rsrp_val = ( atoi ( _rsrp0 ) > atoi ( _rsrp1 ) ?
                        atoi ( _rsrp0 ) : atoi ( _rsrp1 ) );

        if ( rsrp_val <= -135 )
        {
            nv_set ( "signal_bar", "0" );
        }
        else if ( rsrp_val <= -120 )
        {
            nv_set ( "signal_bar", "1" );
        }
        else if ( rsrp_val <= -100 )
        {
            nv_set ( "signal_bar", "2" );
        }
        else if ( rsrp_val <= -30 )
        {
            if ( rsrp_val <= -85 || 0 == strncmp ( led_mode, "1", 1 ) )
            {
                nv_set ( "signal_bar", "3" );
            }
            else
            {
                nv_set ( "signal_bar", "4" );
            }
        }
        else
        {
            nv_set ( "signal_bar", "-1" );
        }
        break;
    }
}

static int parsing_pcc_rrc_state ( char* line_str )
{
    if ( 0 == strcmp ( pccInfo.rrc_state, line_str ) )
    {
        return 0;
    }
    snprintf ( pccInfo.dl_earfcn, sizeof ( pccInfo.dl_earfcn ), "%s", line_str );

    CLOGD ( FINE, "#[%s]#\n", line_str );

    int ret = 0;
    int i = 0;
    char *pattern = NULL;
    char regex_buf[2][REGEX_BUF_ONE];

    pattern = "\\rrcState : ([A-Z ]*).*$";

    for ( i = 0; i < 2; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( line_str, pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( "rrc_state", regex_buf[1] );
    }

    return ret;
}

static int parsing_pcc_dl_earfcn ( char* line_str )
{
    if ( 0 == strcmp ( pccInfo.dl_earfcn, line_str ) )
    {
        return 0;
    }
    snprintf ( pccInfo.dl_earfcn, sizeof ( pccInfo.dl_earfcn ), "%s", line_str );

    CLOGD ( FINE, "#[%s]#\n", line_str );

    int ret = 0;
    int i = 0;
    char *pattern = NULL;
    char regex_buf[2][REGEX_BUF_ONE];

    pattern = "\\EARFCN : ([0-9]*).*$";

    for ( i = 0; i < 2; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( line_str, pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( "dl_earfcn", regex_buf[1] );
    }

    return ret;
}

static int parsing_pcc_ul_earfcn ( char* line_str )
{
    if ( 0 == strcmp ( pccInfo.ul_earfcn, line_str ) )
    {
        return 0;
    }
    snprintf ( pccInfo.ul_earfcn, sizeof ( pccInfo.ul_earfcn ), "%s", line_str );

    CLOGD ( FINE, "#[%s]#\n", line_str );

    int ret = 0;
    int i = 0;
    char *pattern = NULL;
    char regex_buf[2][REGEX_BUF_ONE];

    pattern = "\\ULEARFCN : ([0-9]*).*$";

    for ( i = 0; i < 2; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( line_str, pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( "ul_earfcn", regex_buf[1] );
    }

    return ret;
}

static int parsing_pcc_mcc ( char* line_str )
{
    if ( 0 == strcmp ( pccInfo.mcc, line_str ) )
    {
        return 0;
    }
    snprintf ( pccInfo.mcc, sizeof ( pccInfo.mcc ), "%s", line_str );

    CLOGD ( FINE, "#[%s]#\n", line_str );

    int ret = 0;
    int i = 0;
    char *pattern = NULL;
    char regex_buf[2][REGEX_BUF_ONE];

    pattern = "\\MCC : ([0-9]*).*$";

    for ( i = 0; i < 2; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( line_str, pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( "mcc", regex_buf[1] );
    }

    return ret;
}

static int parsing_pcc_mnc ( char* line_str )
{
    if ( 0 == strcmp ( pccInfo.mnc, line_str ) )
    {
        return 0;
    }
    snprintf ( pccInfo.mnc, sizeof ( pccInfo.mnc ), "%s", line_str );

    CLOGD ( FINE, "#[%s]#\n", line_str );

    int ret = 0;
    int i = 0;
    char *pattern = NULL;
    char regex_buf[2][REGEX_BUF_ONE];

    pattern = "\\MNC : ([0-9]*).*$";

    for ( i = 0; i < 2; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( line_str, pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( "mnc", regex_buf[1] );
    }

    return ret;
}

static int parsing_pcc_tac ( char* line_str )
{
    if ( 0 == strcmp ( pccInfo.tac, line_str ) )
    {
        return 0;
    }
    snprintf ( pccInfo.tac, sizeof ( pccInfo.tac ), "%s", line_str );

    CLOGD ( FINE, "#[%s]#\n", line_str );

    char tac_one[4] = {0};
    char tac_two[4] = {0};
    char tac_val[8] = {0};
    int ret = 0;
    int i = 0;
    char *pattern = NULL;
    char regex_buf[2][REGEX_BUF_ONE];

    pattern = "\\TAC : ([0-9()]*).*$";

    for ( i = 0; i < 2; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( line_str, pattern, regex_buf );

    if ( 0 == ret )
    {
        sscanf ( regex_buf[1], "%[0-9](%[0-9])", tac_one, tac_two );
        snprintf ( tac_val, 8, "%d", atoi ( tac_one ) * 256 + atoi ( tac_two ) );
        nv_set ( "tac_org", regex_buf[1] );
        nv_set ( "tac", tac_val );
        tac_lock_function ( tac_val );
    }

    return ret;
}

static int parsing_pcc_cellid ( char* line_str )
{
    if ( 0 == strcmp ( pccInfo.cellid, line_str ) )
    {
        return 0;
    }
    snprintf ( pccInfo.cellid, sizeof ( pccInfo.cellid ), "%s", line_str );

    CLOGD ( FINE, "#[%s]#\n", line_str );

    char cellId[16] = {0};
    char eNBId[16] = {0};
    int ret = 0;
    int i = 0;
    char *pattern = NULL;
    char regex_buf[2][REGEX_BUF_ONE];

    pattern = "\\CID : ([0-9]*).*$";

    for ( i = 0; i < 2; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( line_str, pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( "globalid", regex_buf[1] );
        get_cell_id ( regex_buf[1], cellId, eNBId );
        nv_set ( "cellid", cellId );
        nv_set ( "eNBid", eNBId );
    }

    return ret;
}

static int parsing_pcc_band ( char* line_str )
{
    if ( 0 == strcmp ( pccInfo.band, line_str ) )
    {
        return 0;
    }
    snprintf ( pccInfo.band, sizeof ( pccInfo.band ), "%s", line_str );

    CLOGD ( FINE, "#[%s]#\n", line_str );

    int ret = 0;
    int i = 0;
    char *pattern = NULL;
    char regex_buf[2][REGEX_BUF_ONE];

    pattern = "\\Bd : B([0-9]*).*$";

    for ( i = 0; i < 2; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( line_str, pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( "band", regex_buf[1] );
        if ( 32 < atoi ( regex_buf[1] ) && atoi ( regex_buf[1] ) < 54 )
        {
            nv_set ( "mode",  "TDD" );
        }
        else
        {
            nv_set ( "mode",  "FDD" );
        }
    }

    return ret;
}

static int parsing_pcc_dl_bandwidth ( char* line_str )
{
    if ( 0 == strcmp ( pccInfo.dl_bandwidth, line_str ) )
    {
        return 0;
    }
    snprintf ( pccInfo.dl_bandwidth, sizeof ( pccInfo.dl_bandwidth ), "%s", line_str );

    CLOGD ( FINE, "#[%s]#\n", line_str );

    int ret = 0;
    int i = 0;
    char *pattern = NULL;
    char regex_buf[2][REGEX_BUF_ONE];

    pattern = "\\D : ([-.0-9]*)MHz.*$";

    for ( i = 0; i < 2; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( line_str, pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( "bandwidth", regex_buf[1] );
        nv_set ( "dl_bandwidth", regex_buf[1] );
    }

    return ret;
}

static int parsing_pcc_ul_bandwidth ( char* line_str )
{
    if ( 0 == strcmp ( pccInfo.ul_bandwidth, line_str ) )
    {
        return 0;
    }
    snprintf ( pccInfo.ul_bandwidth, sizeof ( pccInfo.ul_bandwidth ), "%s", line_str );

    CLOGD ( FINE, "#[%s]#\n", line_str );

    int ret = 0;
    int i = 0;
    char *pattern = NULL;
    char regex_buf[2][REGEX_BUF_ONE];

    pattern = "\\U : ([-.0-9]*)MHz.*$";

    for ( i = 0; i < 2; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( line_str, pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( "ul_bandwidth", regex_buf[1] );
    }

    return ret;
}

static int parsing_pcc_cinr ( char* line_str )
{
    if ( 0 == strcmp ( pccInfo.cinr, line_str ) )
    {
        return 0;
    }
    snprintf ( pccInfo.cinr, sizeof ( pccInfo.cinr ), "%s", line_str );

    CLOGD ( FINE, "#[%s]#\n", line_str );

    int ret = 0;
    int i = 0;
    char *pattern = NULL;
    char regex_buf[3][REGEX_BUF_ONE];

    pattern = "\\SNR : ([-.0-9]*),([-.0-9]*),.*$";

    for ( i = 0; i < 3; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( line_str, pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( "cinr",  regex_buf[1] );
        nv_set ( "cinr0", regex_buf[1] );
        nv_set ( "cinr1", regex_buf[2] );
    }

    return ret;
}

static int parsing_pcc_pci ( char* line_str )
{
    if ( 0 == strcmp ( pccInfo.pci, line_str ) )
    {
        return 0;
    }
    snprintf ( pccInfo.pci, sizeof ( pccInfo.pci ), "%s", line_str );

    CLOGD ( FINE, "#[%s]#\n", line_str );

    int ret = 0;
    int i = 0;
    char *pattern = NULL;
    char regex_buf[2][REGEX_BUF_ONE];

    pattern = "\\PCI : ([0-9]*).*$";

    for ( i = 0; i < 2; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( line_str, pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( "pci", regex_buf[1] );
    }

    return ret;
}

static int parsing_pcc_rsrq ( char* line_str )
{
    if ( 0 == strcmp ( pccInfo.rsrq, line_str ) )
    {
        return 0;
    }
    snprintf ( pccInfo.rsrq, sizeof ( pccInfo.rsrq ), "%s", line_str );

    CLOGD ( FINE, "#[%s]#\n", line_str );

    int ret = 0;
    int i = 0;
    char *pattern = NULL;
    char regex_buf[3][REGEX_BUF_ONE];

    pattern = "\\RSRQ : ([-.0-9]*),([-.0-9]*),.*$";

    for ( i = 0; i < 3; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( line_str, pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( "rsrq",  regex_buf[1] );
        nv_set ( "rsrq0", regex_buf[1] );
        nv_set ( "rsrq1", regex_buf[2] );
    }

    return ret;
}

static int parsing_pcc_rsrp ( char* line_str )
{
    if ( 0 == strcmp ( pccInfo.rsrp, line_str ) )
    {
        return 0;
    }
    snprintf ( pccInfo.rsrp, sizeof ( pccInfo.rsrp ), "%s", line_str );

    CLOGD ( FINE, "#[%s]#\n", line_str );

    int ret = 0;
    int i = 0;
    char *pattern = NULL;
    char regex_buf[3][REGEX_BUF_ONE];

    pattern = "\\RSRP : ([-.0-9]*),([-.0-9]*),.*$";

    for ( i = 0; i < 3; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( line_str, pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( "rsrp",  regex_buf[1] );
        nv_set ( "rsrp0", regex_buf[1] );
        nv_set ( "rsrp1", regex_buf[2] );
    }

    return ret;
}

static int parsing_pcc_rssi ( char* line_str )
{
    if ( 0 == strcmp ( pccInfo.rssi, line_str ) )
    {
        return 0;
    }
    snprintf ( pccInfo.rssi, sizeof ( pccInfo.rssi ), "%s", line_str );

    CLOGD ( FINE, "#[%s]#\n", line_str );

    int ret = 0;
    int i = 0;
    char *pattern = NULL;
    char regex_buf[3][REGEX_BUF_ONE];

    pattern = "\\RSSI : ([-.0-9]*),([-.0-9]*),.*$";

    for ( i = 0; i < 3; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( line_str, pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( "rssi",  regex_buf[1] );
        nv_set ( "rssi0", regex_buf[1] );
        nv_set ( "rssi1", regex_buf[2] );
    }

    return ret;
}

static int parsing_pcc_dl_frequency ( char* line_str )
{
    if ( 0 == strcmp ( pccInfo.dl_freq, line_str ) )
    {
        return 0;
    }
    snprintf ( pccInfo.dl_freq, sizeof ( pccInfo.dl_freq ), "%s", line_str );

    CLOGD ( FINE, "#[%s]#\n", line_str );

    int ret = 0;
    int i = 0;
    char *pattern = NULL;
    char regex_buf[2][REGEX_BUF_ONE];

    pattern = "\\DLFREQ : ([0-9]*.[0-9]).*$";

    for ( i = 0; i < 2; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( line_str, pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( "dl_frequency", regex_buf[1] );
    }

    return ret;
}

static int parsing_pcc_ul_frequency ( char* line_str )
{
    if ( 0 == strcmp ( pccInfo.ul_freq, line_str ) )
    {
        return 0;
    }
    snprintf ( pccInfo.ul_freq, sizeof ( pccInfo.ul_freq ), "%s", line_str );

    CLOGD ( FINE, "#[%s]#\n", line_str );

    int ret = 0;
    int i = 0;
    char *pattern = NULL;
    char regex_buf[2][REGEX_BUF_ONE];

    pattern = "\\ULFREQ : ([0-9]*.[0-9]).*$";

    for ( i = 0; i < 2; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( line_str, pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( "ul_frequency", regex_buf[1] );
    }

    return ret;
}

static int parsing_pcc_dl_mcs ( char* line_str )
{
    if ( 0 == strcmp ( pccInfo.dl_mcs, line_str ) )
    {
        return 0;
    }
    snprintf ( pccInfo.dl_mcs, sizeof ( pccInfo.dl_mcs ), "%s", line_str );

    CLOGD ( FINE, "#[%s]#\n", line_str );

    int ret = 0;
    int i = 0;
    char *pattern = NULL;
    char regex_buf[2][REGEX_BUF_ONE];

    pattern = "\\DLMCS : ([0-9]*).*$";

    for ( i = 0; i < 2; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( line_str, pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( "dlmcs", regex_buf[1] );
    }

    return ret;
}

static int parsing_pcc_ul_mcs ( char* line_str )
{
    if ( 0 == strcmp ( pccInfo.ul_mcs, line_str ) )
    {
        return 0;
    }
    snprintf ( pccInfo.ul_mcs, sizeof ( pccInfo.ul_mcs ), "%s", line_str );

    CLOGD ( FINE, "#[%s]#\n", line_str );

    int ret = 0;
    int i = 0;
    char *pattern = NULL;
    char regex_buf[2][REGEX_BUF_ONE];

    pattern = "\\ULMCS : ([0-9]*).*$";

    for ( i = 0; i < 2; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( line_str, pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( "ulmcs", regex_buf[1] );
    }

    return ret;
}

static int parsing_pcc_txpower ( char* line_str )
{
    if ( 0 == strcmp ( pccInfo.tx_power, line_str ) )
    {
        return 0;
    }
    snprintf ( pccInfo.tx_power, sizeof ( pccInfo.tx_power ), "%s", line_str );

    CLOGD ( FINE, "#[%s]#\n", line_str );

    int ret = 0;
    int i = 0;
    int tx_power = 0;
    char *pattern = NULL;
    char regex_buf[2][REGEX_BUF_ONE];

    pattern = "\\TXPOWER : ([-.0-9]*).*$";

    for ( i = 0; i < 2; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( line_str, pattern, regex_buf );

    if ( 0 == ret )
    {
        tx_power = atoi ( regex_buf[1] );
        if ( tx_power > 230 )
        {
            nv_set ( "txpower", "23.0" );
        }
        else if ( tx_power < -550 )
        {
            nv_set ( "txpower", "-55.0" );
        }
        else
        {
            snprintf ( regex_buf[1], REGEX_BUF_ONE, "%d.%d", tx_power / 10,
                                ( tx_power < 0 ? -tx_power : tx_power ) % 10 );
            nv_set ( "txpower", regex_buf[1] );
        }
    }

    return ret;
}

static int parsing_pcc_sinr ( char* line_str )
{
    if ( 0 == strcmp ( pccInfo.sinr, line_str ) )
    {
        return 0;
    }
    snprintf ( pccInfo.sinr, sizeof ( pccInfo.sinr ), "%s", line_str );

    CLOGD ( FINE, "#[%s]#\n", line_str );

    int ret = 0;
    int i = 0;
    char *pattern = NULL;
    char regex_buf[2][REGEX_BUF_ONE];

    pattern = "\\SINR : ([-.0-9]*).*$";

    for ( i = 0; i < 2; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( line_str, pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( "sinr", regex_buf[1] );
        calc_signal_bar ( regex_buf[1] );
    }

    return ret;
}

static int parsing_pcc_rrcpower ( char* line_str )
{
    if ( 0 == strcmp ( pccInfo.rrc_power, line_str ) )
    {
        return 0;
    }
    snprintf ( pccInfo.rrc_power, sizeof ( pccInfo.rrc_power ), "%s", line_str );

    CLOGD ( FINE, "#[%s]#\n", line_str );

    int ret = 0;
    int i = 0;
    char *pattern = NULL;
    char regex_buf[2][REGEX_BUF_ONE];

    pattern = "\\RRCPOWER : ([-.0-9]*).*$";

    for ( i = 0; i < 2; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( line_str, pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( "rccpower", regex_buf[1] );
    }

    return ret;
}

static int parsing_pcc_rxlv ( char* line_str )
{
    if ( 0 == strcmp ( pccInfo.rxlv, line_str ) )
    {
        return 0;
    }
    snprintf ( pccInfo.rxlv, sizeof ( pccInfo.rxlv ), "%s", line_str );

    CLOGD ( FINE, "#[%s]#\n", line_str );

    int ret = 0;
    int i = 0;
    char *pattern = NULL;
    char regex_buf[2][REGEX_BUF_ONE];

    pattern = "\\RXLV : ([-.0-9]*).*$";

    for ( i = 0; i < 2; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( line_str, pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( "pcc_rxlv", regex_buf[1] );
    }

    return ret;
}

static int parsing_pcc_txmode ( char* line_str )
{
    if ( 0 == strcmp ( pccInfo.tx_mode, line_str ) )
    {
        return 0;
    }
    snprintf ( pccInfo.tx_mode, sizeof ( pccInfo.tx_mode ), "%s", line_str );

    CLOGD ( FINE, "#[%s]#\n", line_str );

    int ret = 0;
    int i = 0;
    char *pattern = NULL;
    char regex_buf[2][REGEX_BUF_ONE];

    pattern = "\\TXMODE : ([0-9]*).*$";

    for ( i = 0; i < 2; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( line_str, pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( "tx_mode", regex_buf[1] );
    }

    return ret;
}

static int parsing_pcc_handover ( char* line_str )
{
    if ( 0 == strcmp ( pccInfo.handover, line_str ) )
    {
        return 0;
    }
    snprintf ( pccInfo.handover, sizeof ( pccInfo.handover ), "%s", line_str );

    CLOGD ( FINE, "#[%s]#\n", line_str );

    int ret = 0;
    int i = 0;
    char *pattern = NULL;
    char regex_buf[3][REGEX_BUF_ONE];

    pattern = "\\HANDOVER : ([0-9]*),([0-9]*).*$";

    for ( i = 0; i < 3; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( line_str, pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( "handover_failed", regex_buf[1] );
        nv_set ( "handover_total",  regex_buf[2] );
    }

    return ret;
}

static int parsing_pcc_bearscount ( char* line_str )
{
    if ( 0 == strcmp ( pccInfo.bears_count, line_str ) )
    {
        return 0;
    }
    snprintf ( pccInfo.bears_count, sizeof ( pccInfo.bears_count ), "%s", line_str );

    CLOGD ( FINE, "#[%s]#\n", line_str );

    int ret = 0;
    int i = 0;
    char *pattern = NULL;
    char regex_buf[2][REGEX_BUF_ONE];

    pattern = "\\BEARSCOUNT : ([0-9]*).*$";

    for ( i = 0; i < 2; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( line_str, pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( "bears_count", regex_buf[1] );
    }

    return ret;
}

void parsing_gct_ktlteinfo ( char* data )
{
    char *line_buf = NULL;
    int line_num = 0;
    char ram_mcc[8] = {0};
    char ram_mnc[8] = {0};
    char SIM_SPN[64] = {0};

    if ( strstr ( data, "\r\nERROR\r\n" ) )
    {
        CLOGD ( FINE, "ktlteinfo return ERROR!\n" );
        goto ERROR;
    }

    CLOGD ( FINE, "start ktlteinfo parsing ...\n" );

    line_buf = strtok ( data, "\r\n" );
    while ( line_buf )
    {
        if ( 0 == line_num && strstr ( line_buf, "rrcState :" ) )
        {
            if ( parsing_pcc_rrc_state ( line_buf ) )
            {
                break;
            }
            line_num ++;
        }
        else if ( 0 == line_num && strstr ( line_buf, "EARFCN :" ) )
        {
            if ( parsing_pcc_dl_earfcn ( line_buf ) )
            {
                break;
            }
            line_num ++;
        }
        else if ( 1 == line_num && strstr ( line_buf, "ULEARFCN :" ) )
        {
            if ( parsing_pcc_ul_earfcn ( line_buf ) )
            {
                break;
            }
            line_num ++;
        }
        else if ( 2 == line_num && strstr ( line_buf, "MCC :" ) )
        {
            if ( parsing_pcc_mcc ( line_buf ) )
            {
                break;
            }
            line_num ++;
        }
        else if ( 3 == line_num && strstr ( line_buf, "MNC :" ) )
        {
            if ( parsing_pcc_mnc ( line_buf ) )
            {
                break;
            }
            line_num ++;
        }
        else if ( 4 == line_num && strstr ( line_buf, "TAC :" ) )
        {
            if ( parsing_pcc_tac ( line_buf ) )
            {
                break;
            }
            line_num ++;
        }
        else if ( 5 == line_num && strstr ( line_buf, "CID :" ) )
        {
            if ( parsing_pcc_cellid ( line_buf ) )
            {
                break;
            }
            line_num ++;
        }
        else if ( 6 == line_num && strstr ( line_buf, "Bd :" ) )
        {
            if ( parsing_pcc_band ( line_buf ) )
            {
                break;
            }
            line_num ++;
        }
        else if ( 7 == line_num && strstr ( line_buf, "D :" ) )
        {
            if ( parsing_pcc_dl_bandwidth ( line_buf ) )
            {
                break;
            }
            line_num ++;
        }
        else if ( 8 == line_num && strstr ( line_buf, "U :" ) )
        {
            if ( parsing_pcc_ul_bandwidth ( line_buf ) )
            {
                break;
            }
            line_num ++;
        }
        else if ( 9 == line_num && strstr ( line_buf, "SNR :" ) )
        {
            if ( parsing_pcc_cinr ( line_buf ) )
            {
                break;
            }
            line_num ++;
        }
        else if ( 10 == line_num && strstr ( line_buf, "PCI :" ) )
        {
            if ( parsing_pcc_pci ( line_buf ) )
            {
                break;
            }
            line_num ++;
        }
        else if ( 11 == line_num && strstr ( line_buf, "RSRQ :" ) )
        {
            if ( parsing_pcc_rsrq ( line_buf ) )
            {
                break;
            }
            line_num ++;
        }
        else if ( 12 == line_num && strstr ( line_buf, "RSRP :" ) )
        {
            if ( parsing_pcc_rsrp ( line_buf ) )
            {
                break;
            }
            line_num ++;
        }
        else if ( 13 == line_num && strstr ( line_buf, "RSSI :" ) )
        {
            if ( parsing_pcc_rssi ( line_buf ) )
            {
                break;
            }
            line_num ++;
        }
        else if ( 14 == line_num && strstr ( line_buf, "DLFREQ :" ) )
        {
            if ( parsing_pcc_dl_frequency ( line_buf ) )
            {
                break;
            }
            line_num ++;
        }
        else if ( 15 == line_num && strstr ( line_buf, "ULFREQ :" ) )
        {
            if ( parsing_pcc_ul_frequency ( line_buf ) )
            {
                break;
            }
            line_num ++;
        }
        else if ( 16 == line_num && strstr ( line_buf, "DLMCS :" ) )
        {
            if ( parsing_pcc_dl_mcs ( line_buf ) )
            {
                break;
            }
            line_num ++;
        }
        else if ( 17 == line_num && strstr ( line_buf, "ULMCS :" ) )
        {
            if ( parsing_pcc_ul_mcs ( line_buf ) )
            {
                break;
            }
            line_num ++;
        }
        else if ( 18 == line_num && strstr ( line_buf, "TXPOWER :" ) )
        {
            if ( parsing_pcc_txpower ( line_buf ) )
            {
                break;
            }
            line_num ++;
        }
        else if ( 19 == line_num && strstr ( line_buf, "SINR :" ) )
        {
            if ( parsing_pcc_sinr ( line_buf ) )
            {
                break;
            }
            line_num ++;
        }
        else if ( 20 == line_num && strstr ( line_buf, "RRCPOWER :" ) )
        {
            if ( parsing_pcc_rrcpower ( line_buf ) )
            {
                break;
            }
            line_num ++;
        }
        else if ( 21 == line_num && strstr ( line_buf, "RXLV :" ) )
        {
            if ( parsing_pcc_rxlv ( line_buf ) )
            {
                break;
            }
            line_num ++;
        }
        else if ( 22 == line_num && strstr ( line_buf, "TXMODE :" ) )
        {
            if ( parsing_pcc_txmode ( line_buf ) )
            {
                break;
            }
            line_num ++;
        }
        else if ( 23 == line_num && strstr ( line_buf, "HANDOVER :" ) )
        {
            if ( parsing_pcc_handover ( line_buf ) )
            {
                break;
            }
            line_num ++;
        }
        else if ( 24 == line_num && strstr ( line_buf, "BEARSCOUNT :" ) )
        {
            if ( parsing_pcc_bearscount ( line_buf ) )
            {
                break;
            }
            line_num ++;
        }

        line_buf = strtok ( NULL, "\r\n" );
    }

    CLOGD ( FINE, "line_num: [%d]\n", line_num );

    if ( 3 < line_num )
    {
        nv_get ( "mcc", ram_mcc, sizeof ( ram_mcc ) );
        nv_get ( "mnc", ram_mnc, sizeof ( ram_mnc ) );
        if ( strcmp ( ram_mcc, old_mcc ) || strcmp ( ram_mnc, old_mnc ) )
        {
            snprintf ( old_mcc, sizeof ( old_mcc ), "%s", ram_mcc );
            snprintf ( old_mnc, sizeof ( old_mnc ), "%s", ram_mnc );
            gctParam_getOperator ( old_mcc, old_mnc );
        }

        if ( 24 < line_num )
            return;
    }
    else
    {
        nv_get ( "SIM_SPN", SIM_SPN, sizeof ( SIM_SPN ) );
        nv_set ( "operator", SIM_SPN );
        snprintf ( old_mcc, sizeof ( old_mcc ), "%s", "FFF" );
    }

    return;

ERROR:
    lteinfo_data_restore();
}

void parsing_gct_glteconnstatus ( char* data )
{
    CLOGD ( FINE, "start parsing_gct_glteconnstatus ...\n" );

    char *pattern = NULL;
    int ret = 0;
    int i = 0;
    char cellId[16] = {0};
    char eNBId[16] = {0};
    char regex_buf[8][REGEX_BUF_ONE];

    if ( strstr ( data, "\r\nERROR\r\n" ) )
    {
        CLOGD ( FINE, "glteconnstatus return ERROR!\n" );
        goto ERROR;
    }

    pattern = "Band ([0-9]*), lteBW ([0-9]*)MHz,.*OK.*$";

    for ( i = 0; i < 8; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "Band " ), pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( "band",      regex_buf[1] );
        nv_set ( "bandwidth", regex_buf[2] );
    }
    else
    {
        CLOGD ( WARNING, "parsing_gct_glteconnstatus PART1 regex error\n" );
        goto ERROR;
    }

    pattern = "dlEarfcn ([0-9]*), ulEarfcn ([0-9]*),.*MCC ([0-9]*), MNC ([0-9]*),.*OK.*$";

    for ( i = 0; i < 8; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "dlEarfcn " ), pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( "dl_earfcn", regex_buf[1] );
        nv_set ( "ul_earfcn", regex_buf[2] );
        nv_set ( "mcc",       regex_buf[3] );
        nv_set ( "mnc",       regex_buf[4] );
    }
    else
    {
        CLOGD ( WARNING, "parsing_gct_glteconnstatus PART2 regex error\n" );
        goto ERROR;
    }

    pattern = "Tac ([0-9()]*), phyCID ([0-9]*), nasCID ([0-9]*),.*OK.*$";

    for ( i = 0; i < 8; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "Tac " ), pattern, regex_buf );

    if ( 0 == ret )
    {
        char tac_one[4] = {0};
        char tac_two[4] = {0};
        char tac_val[8] = {0};
        sscanf ( regex_buf[1], "%[0-9](%[0-9])", tac_one, tac_two );
        snprintf ( tac_val, 8, "%d", atoi ( tac_one ) * 256 + atoi ( tac_two ) );
        nv_set ( "tac_org",    regex_buf[1] );
        nv_set ( "tac",             tac_val );
        nv_set ( "pci",        regex_buf[2] );
        nv_set ( "globalid",   regex_buf[3] );
        get_cell_id ( regex_buf[3], cellId, eNBId );
        nv_set ( "cellid",           cellId );
        nv_set ( "eNBid",             eNBId );
    }
    else
    {
        CLOGD ( WARNING, "parsing_gct_glteconnstatus PART3 regex error\n" );
        goto ERROR;
    }

    pattern = "pccRSRP ([-.0-9]*),([-.0-9]*),([-.0-9]*),([-.0-9]*),A:([-.0-9]*),.*OK.*$";

    for ( i = 0; i < 8; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "pccRSRP " ), pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( "rsrp0",   regex_buf[1] );
        nv_set ( "rsrp1",   regex_buf[2] );
        nv_set ( "rsrp2",   regex_buf[3] );
        nv_set ( "rsrp3",   regex_buf[4] );
        nv_set ( "rsrp",    regex_buf[5] );
    }
    else
    {
        CLOGD ( WARNING, "parsing_gct_glteconnstatus PART4 regex error\n" );
        goto ERROR;
    }

    pattern = "pccCINR ([-.0-9]*),([-.0-9]*),([-.0-9]*),([-.0-9]*),.*pccSINR ([-.0-9]*),.*OK.*$";

    for ( i = 0; i < 8; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "pccCINR " ), pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( "cinr0",   regex_buf[1] );
        nv_set ( "cinr1",   regex_buf[2] );
        nv_set ( "cinr2",   regex_buf[3] );
        nv_set ( "cinr3",   regex_buf[4] );
        nv_set ( "sinr",    regex_buf[5] );
    }
    else
    {
        CLOGD ( WARNING, "parsing_gct_glteconnstatus PART5 regex error\n" );
        goto ERROR;
    }

    pattern = "pccRSSI ([-.0-9]*),([-.0-9]*),([-.0-9]*),([-.0-9]*),A:([-.0-9]*),.*OK.*$";

    for ( i = 0; i < 8; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "pccRSSI " ), pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( "rssi0",   regex_buf[1] );
        nv_set ( "rssi1",   regex_buf[2] );
        nv_set ( "rssi2",   regex_buf[3] );
        nv_set ( "rssi3",   regex_buf[4] );
        nv_set ( "rssi",    regex_buf[5] );
    }
    else
    {
        CLOGD ( WARNING, "parsing_gct_glteconnstatus PART6 regex error\n" );
        goto ERROR;
    }

    pattern = "pccRSRQ ([-.0-9]*),([-.0-9]*),([-.0-9]*),([-.0-9]*),A:([-.0-9]*),.*OK.*$";

    for ( i = 0; i < 8; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "pccRSRQ " ), pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( "rsrq0",   regex_buf[1] );
        nv_set ( "rsrq1",   regex_buf[2] );
        nv_set ( "rsrq2",   regex_buf[3] );
        nv_set ( "rsrq3",   regex_buf[4] );
        nv_set ( "rsrq",    regex_buf[5] );
    }
    else
    {
        CLOGD ( WARNING, "parsing_gct_glteconnstatus PART7 regex error\n" );
        goto ERROR;
    }

    CLOGD ( FINE, "end parsing_gct_glteconnstatus ...\n" );
    return;

ERROR:
    lteinfo_data_restore();
}

static void parsing_gct_gdmitem_value ( int in_times, char* data )
{
    int i = 0, ret = 0;
    char *pattern = NULL;
    char regex_buf[5][REGEX_BUF_ONE];

    char S_pci[8] = {0};
    char S_bandwidth[16] = {0};
    char S_dl_frequency[16] = {0};
    char S_rssi[8] = {0};
    char S_rssi0[16] = {0};
    char S_rssi1[16] = {0};
    char S_rsrq[8] = {0};
    char S_rsrq0[16] = {0};
    char S_rsrq1[16] = {0};
    char S_rsrp0[16] = {0};
    char S_rsrp1[16] = {0};
    char S_cinr0[16] = {0};
    char S_cinr1[16] = {0};
    char S_dlmcs[16] = {0};

    if ( in_times == 0 )
    {

        pattern = "%GDMITEM: \"L1SCC\", ([0-3]).*%GDMITEM:.*$";
        for ( i = 0; i < 5; i++ )
            memset ( regex_buf[i], 0, REGEX_BUF_ONE );

        ret = at_tok_regular_more ( data, pattern, regex_buf );
        if ( ret == 0 )
        {
            nv_set ( "secondary_cell",    regex_buf[1] );
        }
        else
        {
            CLOGD ( FINE, "GDMITEM not match! PART 0!\n" );
            goto ERROR;
        }

        return;
    }

    pattern = "PCI, ([0-9]*), BW, ([0-9]*), FREQ, ([0-9.]*), SCC_IDX.*$";

    for ( i = 0; i < 5; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( data, pattern, regex_buf );
    if ( ret == 0 )
    {
        sprintf ( S_pci, "S%d_pci", in_times );
        nv_set ( S_pci, regex_buf[1] );

        sprintf ( S_bandwidth, "S%d_bandwidth", in_times );
        get_secondary_cell_bandwidth ( S_bandwidth, regex_buf[2] );

        sprintf ( S_dl_frequency, "S%d_dl_frequency", in_times );
        if ( NULL == strstr ( regex_buf[3], "." ) )
        {
            int s_freq_len = strlen ( regex_buf[3] );
            regex_buf[3][s_freq_len] = regex_buf[3][s_freq_len - 1];
            regex_buf[3][s_freq_len - 1] = '.';
            regex_buf[3][s_freq_len + 1] = '\0';
        }
        nv_set ( S_dl_frequency, regex_buf[3] );
    }
    else
    {
        CLOGD ( FINE, "GDMITEM not match! PART 1!\n" );
        goto ERROR;
    }

    pattern = "SCC_IDX, [0-9]*.*M-RSSI, \\(([-.0-9]*), [-.0-9]*\\), D-RSSI, \\(([-.0-9]*), [-.0-9]*\\).*M-RSRQ, \\(([-.0-9]*), [-.0-9]*\\).*D-RSRQ.*$";

    for ( i = 0; i < 5; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "SCC_IDX, " ), pattern, regex_buf );
    if ( ret == 0 )
    {
        sprintf ( S_rssi, "S%d_rssi", in_times );
        nv_set ( S_rssi, regex_buf[1] );

        sprintf ( S_rssi0, "S%d_rssi0", in_times );
        nv_set ( S_rssi0, regex_buf[1] );

        sprintf ( S_rssi1, "S%d_rssi1", in_times );
        nv_set ( S_rssi1, regex_buf[2] );

        sprintf ( S_rsrq, "S%d_rsrq", in_times );
        nv_set ( S_rsrq, regex_buf[3] );

        sprintf ( S_rsrq0, "S%d_rsrq0", in_times );
        nv_set ( S_rsrq0, regex_buf[3] );
    }
    else
    {
        CLOGD ( FINE, "GDMITEM not match! PART 2!\n" );
        goto ERROR;
    }

    pattern = "D-RSRQ, \\(([-.0-9]*), [-.0-9]*\\).*M-RSRP, \\(([-.0-9]*), [-.0-9]*\\), D-RSRP, \\(([-.0-9]*), [-.0-9]*\\).*M-CINR, \\(([-.0-9]*), [-.0-9]*\\).*D-CINR.*$";

    for ( i = 0; i < 5; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "D-RSRQ, " ), pattern, regex_buf );
    if ( ret == 0 )
    {
        sprintf ( S_rsrq1, "S%d_rsrq1", in_times );
        nv_set ( S_rsrq1, regex_buf[1] );

        sprintf ( S_rsrp0, "S%d_rsrp0", in_times );
        nv_set ( S_rsrp0, regex_buf[2] );

        sprintf ( S_rsrp1, "S%d_rsrp1", in_times );
        nv_set ( S_rsrp1, regex_buf[3] );

        sprintf ( S_cinr0, "S%d_cinr0", in_times );
        nv_set ( S_cinr0, regex_buf[4] );
    }
    else
    {
        CLOGD ( FINE, "GDMITEM not match! PART 3!\n" );
        goto ERROR;
    }

    pattern = "D-CINR, \\(([-.0-9]*), [-.0-9]*\\).*DL_CW1_MCS, ([-,0-9]*).*DL_CW2_MCS.*$";

    for ( i = 0; i < 5; i++ )
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "D-CINR, " ), pattern, regex_buf );
    if ( ret == 0 )
    {
        sprintf ( S_cinr1, "S%d_cinr1", in_times );
        nv_set ( S_cinr1, regex_buf[1] );

        sprintf ( S_dlmcs, "S%d_dlmcs", in_times );
        get_secondary_cell_mcs ( in_times, S_dlmcs, regex_buf[2] );
    }
    else
    {
        CLOGD ( FINE, "GDMITEM not match! PART 4!\n" );
        goto ERROR;
    }

    return;

ERROR:
    {
        nv_set ( "secondary_cell",    "0" );
    }

}
void parsing_gct_gdmitem ( char* data )
{
    int i = 0;
    char gdmitem_data[1024] = {0};
    char *ptr = NULL;
    char *start = data;

    if ( strstr ( data, "\r\nERROR\r\n" ) )
    {
        CLOGD ( FINE, "GDMITEM return ERROR!\n" );
        nv_set ( "secondary_cell", "0" );
        return;
    }

    ptr = strstr ( data, "PCI" );
    if ( ptr == NULL )
    {
        CLOGD ( FINE, "GDMITEM no SCC!\n" );
        nv_set ( "secondary_cell", "0" );
        return;
    }

    while ( ptr != NULL )
    {
        if ( i > 0 )
        {
            strncpy ( gdmitem_data, start - 1, ptr - start );
        }
        else
        {
            strncpy ( gdmitem_data, start, ptr - start );
        }

        parsing_gct_gdmitem_value ( i, gdmitem_data );

        start = ptr + 1;
        ptr = strstr ( start, "PCI" );
        i++;
        memset ( gdmitem_data, 0, sizeof ( gdmitem_data ) );
    }
    strcpy ( gdmitem_data, start - 1 );
    parsing_gct_gdmitem_value ( i, gdmitem_data );
}

int parsing_gct_imei ( char* data )
{
    char regex_buf[2][REGEX_BUF_ONE];
    int ret = 0;
    char *pattern = NULL;
    pattern = "\\IMEI:[ ]*([0-9]*).*OK.*$";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "IMEI:" ), pattern, regex_buf );
    if ( ret == 0 )
    {
        nv_set ( "imei", regex_buf[1] );
    }
    else
    {
        nv_set ( "imei", "" );
        CLOGD ( WARNING, "AT IMEI return error!\n" );
    }

    return ret;
}

int parsing_gct_suppband ( char* data )
{
    char regex_buf[2][REGEX_BUF_ONE];
    int ret = 0;
    char *pattern = NULL;
    int band_num = 1;
    char band_num_val[8] = {0};
    int i = 0;
    char list[REGEX_BUF_ONE] = {0};

    char old_suppbands[128] = {0};

    pattern = ".*%GGETBAND: [-0-9]*,([0-9,]*).*";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( data, pattern, regex_buf );
    if ( ret == 0 )
    {
        strcpy ( list, regex_buf[1] );
        nv_set ( "suppband_org", list );
        for ( i = 0; i < strlen ( list ); i++ )
        {
            if ( list[i] == ',' )
            {
                band_num ++;
                list[i] = ' ';
            }
        }
        snprintf ( band_num_val, sizeof ( band_num_val ), "%d", band_num );
        nv_set ( "suppband_num", band_num_val );
        nv_set ( "suppband", list );

        sys_get_config ( LTE_PARAMETER_SUPPORT_BAND, old_suppbands, sizeof ( old_suppbands ) );
        if ( strcmp ( old_suppbands, list ) )
        {
            CLOGD ( WARNING, "Save supported bands to flash !\n" );
            sys_set_config ( LTE_PARAMETER_SUPPORT_BAND, list );
            sys_commit_config ( "lte_param" );
        }
    }
    else
    {
        nv_set ( "suppband", "" );
        nv_set ( "suppband_org", "" );
        nv_set ( "suppband_num", "0" );
        CLOGD ( WARNING, "GET SUPPBAND return wrong value!\n" );
    }

    return ret;
}

int parsing_gct_moduleModel ( char* data )
{
    char regex_buf[2][REGEX_BUF_ONE];
    int ret = 0;
    char *pattern = NULL;
    char moduletype[8] = {0};

    pattern = ".*+CGMM: ([^\r\n]*).*OK.*$";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( data, pattern, regex_buf );
    if ( ret == 0 )
    {
        nv_get ( "moduletype", moduletype, sizeof ( moduletype ) );
        if ( 0 == strcmp ( moduletype, "CAT4" ) )
        {
            nv_set ( "modulemodel", "CAT4");
            nv_set ( "modulemodel_orig", regex_buf[1] );
        }
        else
        {
            nv_set ( "modulemodel", regex_buf[1] );
        }
    }
    else
    {
        nv_set ( "modulemodel", "" );
        CLOGD ( WARNING, "AT+CGMM return error!\n" );
    }

    return ret;
}

int parsing_gct_moduleType ( char* data )
{
    char regex_buf[2][REGEX_BUF_ONE];
    int ret = 0;
    char *pattern = NULL;
    pattern = ".*+KTCGMM: ([^\r\n]*).*OK.*$";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( data, pattern, regex_buf );
    if ( ret == 0 )
    {
        nv_set ( "moduletype", regex_buf[1] );
    }
    else
    {
        nv_set ( "moduletype", "" );
        CLOGD ( WARNING, "AT+KTCGMM return error!\n" );
    }

    return ret;
}

int parsing_gct_moduleVersion ( char* data )
{
    char regex_buf[2][REGEX_BUF_ONE];
    int ret = 0;
    char *pattern = NULL;
    pattern = ".*+KTCGMR: ([^\r\n]*).*OK.*$";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( data, pattern, regex_buf );
    if ( ret == 0 )
    {
        nv_set ( "moduleversion", regex_buf[1] );
    }
    else
    {
        nv_set ( "moduleversion", "" );
        CLOGD ( WARNING, "AT+KTCGMR return error!\n" );
    }

    return ret;
}

int parsing_gct_FwVersion ( char* data )
{
    char fw_version[64] = {0};
    char regex_buf[3][REGEX_BUF_ONE];
    int ret = 0;
    char *pattern = NULL;
    pattern = ".*ARM0:([0-9.]*) ARM1:([0-9.]*).*$";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );
    memset ( regex_buf[2], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( data, pattern, regex_buf );
    if ( ret == 0 )
    {
        snprintf(fw_version, sizeof(fw_version), "%s-%s", regex_buf[1], regex_buf[2]);
        nv_set ( "FW_VER", fw_version );
    }
    else
    {
        nv_set ( "FW_VER", "" );
        CLOGD ( WARNING, "AT%%SWV1 return error!\n" );
    }

    return ret;
}

void parsing_gct_lockpin ( char* data )
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

void parsing_gct_enterpin ( char* data )
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

void parsing_gct_modifypin ( char* data )
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

void parsing_gct_enterpuk ( char* data )
{
    if ( strstr ( data, "\r\nOK\r\n" ) )
    {
        nv_set ( "enterpuk_set", "0" );
    }
    else
    {
        nv_set ( "enterpuk_set", "1" );
    }
}

static int get_neighbor_cell_intra ( char* data )
{
    char* head = data;
    char* p = strstr ( head, "\r" );
    int neighbor_count = 0;

    char earfcn_val[8] = {0};
    char band[8] = {0};
    char D_W[8] = {0};
    char U_W[8] = {0};
    char pci[8] = {0};
    char rsrq[8] = {0};
    char rsrp[8] = {0};
    char rssi[8] = {0};
    char rxlv[8] = {0};
    char sinr[8] = {0};

    char earfcn_set_str[32] = {0};
    char pci_set_str[32] = {0};
    char rsrq_set_str[32] = {0};
    char rsrp_set_str[32] = {0};
    char rssi_set_str[32] = {0};
    char sinr_set_str[32] = {0};

    while ( p != NULL )
    {
        neighbor_count++;

        /* get the one line for each neighbor */
        *p = '\0';

        memset ( pci, 0, sizeof ( pci ) );
        memset ( rsrq, 0, sizeof ( rsrq ) );
        memset ( rsrp, 0, sizeof ( rsrp ) );
        memset ( rssi, 0, sizeof ( rssi ) );
        memset ( sinr, 0, sizeof ( sinr ) );

        sscanf ( head, "%s %s %s %s %s %s %s %s %s ",
                        band, D_W, U_W, pci, rsrq, rsrp, rssi, rxlv, sinr
                );

        sprintf ( earfcn_set_str, "neighbor_earfcn_%d", neighbor_count );
        sprintf ( pci_set_str, "neighbor_pci_%d", neighbor_count );
        sprintf ( rsrq_set_str, "neighbor_rsrq_%d", neighbor_count );
        sprintf ( rsrp_set_str, "neighbor_rsrp_%d", neighbor_count );
        sprintf ( rssi_set_str, "neighbor_rssi_%d", neighbor_count );
        sprintf ( sinr_set_str, "neighbor_sinr_%d", neighbor_count );

        nv_get ( "dl_earfcn", earfcn_val, sizeof ( earfcn_val ) );
        nv_set ( earfcn_set_str, earfcn_val );

        if ( strcmp ( pci, "NV" ) )
        {
            nv_set ( pci_set_str, pci );
        }
        else
        {
            nv_set ( pci_set_str, "N/A" );
        }
        if ( strcmp ( rsrq, "NV" ) )
        {
            nv_set ( rsrq_set_str, rsrq );
        }
        else
        {
            nv_set ( rsrq_set_str, "N/A" );
        }
        if ( strcmp ( rsrp, "NV" ) )
        {
            nv_set ( rsrp_set_str, rsrp );
        }
        else
        {
            nv_set ( rsrp_set_str, "N/A" );
        }
        if ( strcmp ( rssi, "NV" ) )
        {
            nv_set ( rssi_set_str, rssi );
        }
        else
        {
            nv_set ( rssi_set_str, "N/A" );
        }
        if (strcmp ( sinr, "NV" ) )
        {
            nv_set ( sinr_set_str, sinr );
        }
        else
        {
            nv_set ( sinr_set_str, "N/A" );
        }

        /* get the next line */
        head = p + 1;
        p = strstr ( head, "\r" );
    }

    return neighbor_count;
}

static int get_neighbor_cell_inter ( char* data, int neighbor_count )
{
    char* head = data;
    char* p = strstr ( head, "\r" );

    char band[8] = {0};
    char D_W[8] = {0};
    char U_W[8] = {0};
    char earfcn[8] = {0};
    char ThresholdLow[8] = {0};
    char ThresholdHi[8] = {0};
    char Priority[8] = {0};
    char pci[8] = {0};
    char rsrq[8] = {0};
    char rsrp[8] = {0};
    char rssi[8] = {0};
    char rxlv[8] = {0};
    char sinr[8] = {0};

    char earfcn_set_str[32] = {0};
    char pci_set_str[32] = {0};
    char rsrq_set_str[32] = {0};
    char rsrp_set_str[32] = {0};
    char rssi_set_str[32] = {0};
    char sinr_set_str[32] = {0};

    while ( p != NULL )
    {
        neighbor_count++;

        /* get the one line for each neighbor */
        *p = '\0';

        memset ( earfcn, 0, sizeof ( earfcn ) );
        memset ( pci, 0, sizeof ( pci ) );
        memset ( rsrq, 0, sizeof ( rsrq ) );
        memset ( rsrp, 0, sizeof ( rsrp ) );
        memset ( rssi, 0, sizeof ( rssi ) );
        memset ( sinr, 0, sizeof ( sinr ) );

        sscanf ( head, "%s %s %s %s %s %s %s %s %s %s %s %s %s ",
                        earfcn, ThresholdLow, ThresholdHi, Priority, pci,
                        rsrq, rsrp, rssi, rxlv, band, sinr, D_W, U_W
                );

        sprintf ( earfcn_set_str, "neighbor_earfcn_%d", neighbor_count );
        sprintf ( pci_set_str, "neighbor_pci_%d", neighbor_count );
        sprintf ( rsrq_set_str, "neighbor_rsrq_%d", neighbor_count );
        sprintf ( rsrp_set_str, "neighbor_rsrp_%d", neighbor_count );
        sprintf ( rssi_set_str, "neighbor_rssi_%d", neighbor_count );
        sprintf ( sinr_set_str, "neighbor_sinr_%d", neighbor_count );

        if ( strcmp ( earfcn, "NV" ) )
        {
            nv_set ( earfcn_set_str, earfcn );
        }
        else
        {
            nv_set ( pci_set_str, "N/A" );
        }
        if ( strcmp ( pci, "NV" ) )
        {
            nv_set(pci_set_str, pci);
        }
        else
        {
            nv_set(pci_set_str, "N/A");
        }
        if ( strcmp ( rsrq, "NV" ) )
        {
            nv_set ( rsrq_set_str, rsrq );
        }
        else
        {
            nv_set ( rsrq_set_str, "N/A" );
        }
        if ( strcmp ( rsrp, "NV" ) )
        {
            nv_set ( rsrp_set_str, rsrp );
        }
        else
        {
            nv_set ( rsrp_set_str, "N/A" );
        }
        if ( strcmp ( rssi, "NV" ) )
        {
            nv_set ( rssi_set_str, rssi );
        }
        else
        {
            nv_set ( rssi_set_str, "N/A" );
        }
        if ( strcmp ( sinr, "NV" ) )
        {
            nv_set ( sinr_set_str, sinr );
        }
        else
        {
            nv_set ( sinr_set_str, "N/A" );
        }

        /* get the next line */
        head = p + 1;
        p = strstr ( head, "\r" );
    }

    return neighbor_count;
}

void parsing_gct_lteinfo ( char* data )
{
    char* find1 = "IntraFreq: Bd D U PCI RSRQ RSRP RSSI RXLV SNR\r\n";
    char* find2 = "InterFreq: EARFCN ThresholdLow ThresholdHi Priority PCI RSRQ RSRP RSSI RXLV Bd SNR D U\r\n";
    char* find3 = "\r\nOK\r\n";
    char* ptr1 = NULL;
    char* ptr2 = NULL;
    char* ptr3 = NULL;
    int flag1 = 0, flag2 = 0;
    int neighbor_intra_count = 0, neighbor_inter_count = 0;
    char neighbor_count_str[4] = {0};

    ptr1 = strstr ( data, find1 );
    ptr2 = strstr ( data, find2 );
    ptr3 = strstr ( data, find3 );

    if ( ptr1 == NULL || ptr2 == NULL || ptr3 == NULL )
    {
        nv_set ( "neighbor_count", "0" );
        nv_set ( "search_neighbor_set", "0" );
        return;
    }

    ptr1 += strlen ( find1 );
    if ( ptr1 == ptr2 )
    {
        flag1 = 1;
    }
    *ptr2 = '\0';

    ptr2 += strlen ( find2 );
    if ( ptr2 == ptr3 )
    {
        flag2 = 1;
    }
    *ptr3 = '\0';

    if ( 1 == flag1 && 1 == flag2 )
    {
        nv_set("neighbor_count", "0");
        nv_set("search_neighbor_set", "0");
        return;
    }

    if ( 0 == flag1 )
    {
        neighbor_intra_count = get_neighbor_cell_intra ( ptr1 );
    }

    if ( 0 == flag2 )
    {
        neighbor_inter_count = get_neighbor_cell_inter ( ptr2, neighbor_intra_count );
    }

    sprintf ( neighbor_count_str, "%d",
            ( neighbor_intra_count > neighbor_inter_count ) ? neighbor_intra_count : neighbor_inter_count );
    nv_set ( "neighbor_count", neighbor_count_str );
    nv_set ( "search_neighbor_set", "0" );
}

/*
 * +CCLK: "20/11/30,12:03:38+32"
 *      year/mon/day,hour:min:sec(+-)zone
 */
void parsing_gct_cclk ( char* data )
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

    gmt = nitz_time_total_second (
                time_s[0] + 2000, time_s[1], time_s[2],
                time_s[3], time_s[4], time_s[5]
            );

#if 0
    /*
     * GCT module AT+CCLK? return ( UTC_time & time_zone )
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

void parsing_gct_gsimauth ( char* data )
{
    int ret = 0;
    char *pattern = NULL;
    char regex_buf[4][REGEX_BUF_ONE];

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );
    memset ( regex_buf[2], 0, REGEX_BUF_ONE );
    memset ( regex_buf[3], 0, REGEX_BUF_ONE );

    pattern = "%GSIMAUTH: ([0-3]*), .*OK";
    ret = at_tok_regular_more ( strstr ( data, "%GSIMAUTH" ), pattern, regex_buf );
    if ( 0 == ret )
    {
        CLOGD ( FINE, "volte_aka_rslt : [%s]\n", regex_buf[1] );
        nv_set ( "volte_aka_rslt", regex_buf[1] );
        if ( 0 == strcmp ( "0", regex_buf[1] ) )
        {
            memset ( regex_buf[0], 0, REGEX_BUF_ONE );
            memset ( regex_buf[1], 0, REGEX_BUF_ONE );
            pattern = "%GSIMAUTH: 0, \"([A-Za-z0-9+/=]*)\", \"([A-Za-z0-9+/=]*)\", \"([A-Za-z0-9+/=]*)\".*OK";
            ret = at_tok_regular_more ( strstr ( data, "%GSIMAUTH" ), pattern, regex_buf );
            if ( 0 == ret )
            {
                CLOGD ( FINE, "volte_aka_xres : [%s]\n", regex_buf[1] );
                nv_set ( "volte_aka_xres", regex_buf[1] );
                CLOGD ( FINE, "volte_aka_ck : [%s]\n", regex_buf[2] );
                nv_set ( "volte_aka_ck", regex_buf[2] );
                CLOGD ( FINE, "volte_aka_ik : [%s]\n", regex_buf[3] );
                nv_set ( "volte_aka_ik", regex_buf[3] );
            }
        }
        else if ( 0 == strcmp( "1", regex_buf[1] ) )
        {
            memset ( regex_buf[0], 0, REGEX_BUF_ONE );
            memset ( regex_buf[1], 0, REGEX_BUF_ONE );
            pattern = "%GSIMAUTH: 1, \"([A-Za-z0-9+/=]*)\".*OK";
            ret = at_tok_regular_more ( strstr ( data, "%GSIMAUTH" ), pattern, regex_buf );
            if ( 0 == ret )
            {
                CLOGD ( FINE, "volte_aka_auts : [%s]\n", regex_buf[1] );
                nv_set ( "volte_aka_auts", regex_buf[1] );
            }
        }

        return;
    }

    nv_set ( "volte_aka_rslt", "--" );
    return;
}

void parsing_gct_gcmgs ( char* data )
{
    if ( strstr ( data, "+CMGS:" ) )
    {
        nv_set ( "send_sms", "0" );
    }
    else
    {
        nv_set ( "send_sms", "1" );
    }

    return;
}

void parsing_gct_cmgl_four ( char* data )
{
    static char old_allSms[RECV_SMS_SIZE] = {0};

    if ( 0 == strcmp ( old_allSms, data ) )
    {
        return;
    }

    comd_semaphore_p ( allSms_sem_id );

    FILE *fp = NULL;

    fp = fopen ( "/tmp/allSms.txt", "w+" );

    if ( fp )
    {
        fprintf ( fp, "%s", data );
        strcpy ( old_allSms, data );
        fclose ( fp );

        if ( strstr ( data, "REC UNREAD" ) )
        {
            nv_set ( "unread_sms_exist", "1" );
        }
        else
        {
            nv_set ( "unread_sms_exist", "0" );
        }
        nv_set ( "sms_need_update", "1" );
    }

    comd_semaphore_v ( allSms_sem_id );
    return;
}

void parsing_gct_cmgd ( char* data )
{
    if ( strstr ( data, "\r\nOK\r\n" ) )
    {
        nv_set ( "del_sms", "0" );
    }
    else
    {
        nv_set ( "del_sms", "1" );
    }

    return ;
}

void parsing_gct_cmgr ( char* data )
{
    if ( strstr ( data, "\r\nOK\r\n" ) )
    {
        nv_set ( "read_sms", "0" );
    }
    else
    {
        nv_set ( "read_sms", "1" );
    }

    return ;
}

void parsing_gct_csca_get ( char* data )
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

    return;
}

void parsing_gct_csca_set ( char* data )
{
    if ( strstr ( data, "\r\nOK\r\n" ) )
    {
        nv_set ( "set_smsc", "0" );
    }
    else
    {
        nv_set ( "set_smsc", "1" );
    }

    return;
}

void parsing_gct_cmss ( char* data )
{
    if ( strstr ( data, "\r\nOK\r\n" ) )
    {
        nv_set ( "resend_sms", "0" );
    }
    else
    {
        nv_set ( "resend_sms", "1" );
    }

    return;
}

static void parsing_plmnSearch_content ( char* plmnContent )
{
    char *temp = NULL;
    int i = 0, j = 0, k = 0;
    char plmnContent_value[RECV_BUF_SIZE] = {0};

    while ( i < strlen ( plmnContent ) - 1)
    {
        if ( ')' == plmnContent[i++] )
        {
            plmnContent[i] = ';';
        }

        if ( '(' != plmnContent[i] && ')' != plmnContent[i] && '"' != plmnContent[i] )
        {
            plmnContent[j++] = plmnContent[i];
        }
    }

    plmnContent[j] = '\0';

    temp = strtok ( plmnContent, "," );
    while ( temp )
    {
        if ( ( k++ % 4 ) != 1 )
        {
            if ( 0 == strcmp ( temp, "Play" ) )
            {
                snprintf ( plmnContent_value + strlen ( plmnContent_value ),
                            sizeof ( plmnContent_value ) - strlen ( plmnContent_value ),
                            "%s,", "PLAY"
                    );
            }
            else if ( 0 == strcmp ( temp, "PLUS" ) )
            {
                snprintf ( plmnContent_value + strlen ( plmnContent_value ),
                            sizeof ( plmnContent_value ) - strlen ( plmnContent_value ),
                            "%s,", "PLAY(Plus)"
                    );
            }
            else if (0 == strcmp ( temp, "Orange" ) )
            {
                snprintf ( plmnContent_value + strlen ( plmnContent_value ),
                            sizeof ( plmnContent_value ) - strlen ( plmnContent_value ),
                            "%s,", "PLAY(Orange)"
                    );
            }
            else if ( 0 == strcmp ( temp, "TM PL" ) )
            {
                snprintf ( plmnContent_value + strlen ( plmnContent_value ),
                            sizeof ( plmnContent_value ) - strlen ( plmnContent_value ),
                            "%s,", "PLAY(T-Mobile)"
                    );
            }
            else
            {
                snprintf ( plmnContent_value + strlen ( plmnContent_value ),
                            sizeof ( plmnContent_value ) - strlen ( plmnContent_value ),
                            "%s,", temp
                    );
            }
        }
        temp = strtok ( NULL, "," );
    }

    plmnContent_value[strlen ( plmnContent_value ) - 1] = '\0';

    nv_set ( "m_netselect_contents", plmnContent_value );

    return;
}

void parsing_gct_cops_plmnSearch ( char* data )
{
    int ret = 0;
    char *pattern = NULL;
    char regex_buf[2][REGEX_BUF_ONE];

    pattern = "\\+COPS: ([^\r\n]*).*OK.*$";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "+COPS:" ), pattern, regex_buf );

    if ( 0 == ret )
    {
        if ( strstr ( regex_buf[1], ")" ) )
        {
            parsing_plmnSearch_content ( regex_buf[1] );
            nv_set ( "m_netselect_status", "manual_selected" );
        }
        else
        {
            nv_set ( "m_netselect_status", "manual_search_fail" );
            nv_set ( "m_netselect_contents", "" );
        }
    }
    else
    {
        nv_set ( "m_netselect_status", "manual_search_fail" );
        nv_set ( "m_netselect_contents", "" );
        CLOGD ( WARNING, "AT+COPS=? return nothing !\n");
    }
}

void parsing_gct_cops_select ( char* data )
{
    if ( strstr ( data, "\r\nOK\r\n" ) )
    {
        nv_set ( "plmn_search_net_set", "0" );
    }
    else
    {
        nv_set ( "plmn_search_net_set", "1" );
    }

    return;
}

int parsing_gct_cfun_set ( char* data )
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

void parsing_gct_txpowlmt ( char* data )
{
    if ( strstr ( data, "\r\nOK\r\n" ) )
    {
        nv_set ( "txpower_limit_set", "0" );
    }
    else
    {
        nv_set ( "txpower_limit_set", "1" );
        CLOGD ( WARNING, "AT TXPOWLMT set error !\n" );
    }
}

void parsing_gct_cpms_get ( char* data )
{
    int ret = 0;
    char *pattern = NULL;
    char sms_used_max[8] = {0};
    char regex_buf[2][REGEX_BUF_ONE];

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    pattern = "\\+CPMS: \"[A-Z]*\",[0-9]*,([0-9]*),.*OK.*$";

    ret = at_tok_regular_more ( strstr ( data, "+CPMS:" ), pattern, regex_buf );
    if ( 0 == ret )
    {
        snprintf ( sms_used_max, sizeof ( sms_used_max ),
                            "%d", atoi ( regex_buf[1] ) - 30 );
        nv_set ( "sms_max_num", sms_used_max );
    }
    else
    {
        nv_set ( "sms_max_num", "120" );
        CLOGD ( WARNING, "AT+CPMS? return error !\n" );
    }
}

void parsing_gct_cpin_get ( char* data )
{
    int ret = 0;
    char *pattern = NULL;
    char regex_buf[2][REGEX_BUF_ONE];

    if ( 0 == strcmp ( data, g_strPinSt ) )
    {
        return;
    }

    snprintf ( g_strPinSt, sizeof ( g_strPinSt ), "%s", data );

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

void parsing_gct_clck_get ( char* data )
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
}

void parsing_gct_cimi ( char* data )
{
    int ret = 0;
    char *pattern = NULL;
    char regex_buf[2][REGEX_BUF_ONE];

    pattern = "\r\n([0-9]*)[\r\n]{1,}OK.*$";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( data, pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( "imsi", regex_buf[1] );
    }
    else
    {
        nv_set ( "imsi", "" );
        CLOGD ( WARNING, "AT+CIMI return error !\n" );
    }
}

void parsing_gct_sim_spn ( char* data )
{
    int ret = 0;
    char *pattern = NULL;
    char SIM_SPN[64] = {0};
    char regex_buf[2][REGEX_BUF_ONE];

    pattern = "\\+CRSM: [0-9]*,[0-9]*,\"(.*)\".*OK.*$";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "+CRSM:" ), pattern, regex_buf );

    if ( 0 == ret && 34 <= strlen ( regex_buf[1] ) )
    {
        HexToStr ( SIM_SPN, regex_buf[1] + 2, strlen ( regex_buf[1] ) / 2 - 1 );
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

void parsing_gct_iccid ( char* data )
{
    int ret = 0;
    char *pattern = NULL;
    char regex_buf[2][REGEX_BUF_ONE];

    pattern = "\\GICCID: ([^\r\n]*).*OK.*$";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "GICCID:" ), pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( "iccid", regex_buf[1] );
    }
    else
    {
        nv_set ( "iccid", "" );
        CLOGD ( WARNING, "AT GICCID return error !\n" );
    }
}

void parsing_gct_cpinr ( char* data )
{
    int ret = 0;
    char *pattern = NULL;
    char regex_buf[3][REGEX_BUF_ONE];

    if ( 0 == strcmp ( data, g_strPinPuk ) )
    {
        return;
    }

    snprintf ( g_strPinPuk, sizeof ( g_strPinPuk ), "%s", data );

    pattern = "\\+CPINR: SIM PIN, ([0-9]+),.*+CPINR: SIM PUK, ([0-9]+),.*OK.*$";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );
    memset ( regex_buf[2], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "+CPINR:" ), pattern, regex_buf );

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
}

void parsing_gct_cnum ( char* data )
{
    int ret = 0;
    char *pattern = NULL;
    char regex_buf[2][REGEX_BUF_ONE];

    pattern = "\\+CNUM: \".*\",\"(.*)\",.*OK.*$";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "+CNUM:" ), pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( "phone_number", regex_buf[1] );
    }
    else
    {
        nv_set ( "phone_number", "" );
        CLOGD ( WARNING, "AT+CNUM return error !\n" );
    }
}

void parsing_gct_simMncLen ( char* data )
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

void parsing_gct_cesq ( char* data )
{
    int ret = 0;
    char *pattern = NULL;
    char regex_buf[3][REGEX_BUF_ONE];

    if ( 0 == strcmp ( data, g_strCesq ) )
    {
        return;
    }

    snprintf ( g_strCesq, sizeof ( g_strCesq ), "%s", data );

    pattern = "\\+CESQ: [ ,0-9]+,[ ]*([0-9]+),[ ]*([0-9]+)[\r\n]*OK.*$";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );
    memset ( regex_buf[2], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "+CESQ" ), pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( "cesq_rsrq", regex_buf[1] );
        nv_set ( "cesq_rsrp", regex_buf[2] );
    }
    else
    {
        nv_set ( "cesq_rsrq", "255" );
        nv_set ( "cesq_rsrp", "255" );
        CLOGD ( WARNING, "AT+CESQ not match !\n" );
    }
}

void parsing_gct_cereg ( char* data )
{
    int ret = 0;
    char *pattern = NULL;
    char regex_buf[2][REGEX_BUF_ONE];

    if ( 0 == strcmp ( data, g_strCereg ) )
    {
        return;
    }
    snprintf ( g_strCereg, sizeof ( g_strCereg ), "%s", data );

    pattern = "\\+CEREG: [0-9]+,([0-9]+).*OK.*$";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "+CEREG:" ), pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( "cereg_stat", regex_buf[1] );
    }
    else
    {
        nv_set ( "cereg_stat", "4" );
        CLOGD ( WARNING, "AT+CEREG? return ERROR !\n" );
    }
}

void parsing_gct_cgatt_get ( char* data )
{
    int ret = 0;
    char *pattern = NULL;
    char regex_buf[2][REGEX_BUF_ONE];

    if ( 0 == strcmp ( data, g_strCgatt ) )
    {
        return;
    }
    snprintf ( g_strCgatt, sizeof ( g_strCgatt ), "%s", data );

    pattern = "\\+CGATT:([0-9]+).*OK.*$";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "+CGATT:" ), pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( "cgatt_val", regex_buf[1] );
    }
    else
    {
        nv_set ( "cgatt_val", "0" );
        CLOGD ( WARNING, "AT+CGATT? return ERROR !\n" );
    }
}

void parsing_gct_cgact_get ( char* data )
{
    int ret = 0;
    char *pattern = NULL;
    char regex_buf[2][REGEX_BUF_ONE];

    if ( 0 == strcmp ( data, g_strCgact ) )
    {
        return;
    }
    snprintf ( g_strCgact, sizeof ( g_strCgact ), "%s", data );

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );
    pattern = "\\+CGACT: 1,([0-9]{1}).*";
    ret = at_tok_regular_more ( strstr ( data, "+CGACT: 1," ), pattern, regex_buf );
    if ( 0 == ret )
        nv_set ( "cid_1_state", regex_buf[1] );
    else
        nv_set ( "cid_1_state", "0" );

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );
    pattern = "\\+CGACT: 2,([0-9]{1}).*";
    ret = at_tok_regular_more ( strstr ( data, "+CGACT: 2," ), pattern, regex_buf );
    if ( 0 == ret )
        nv_set ( "cid_2_state", regex_buf[1] );
    else
        nv_set ( "cid_2_state", "0" );

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );
    pattern = "\\+CGACT: 3,([0-9]{1}).*";
    ret = at_tok_regular_more ( strstr ( data, "+CGACT: 3," ), pattern, regex_buf );
    if (0 == ret)
        nv_set ( "cid_3_state", regex_buf[1] );
    else
        nv_set ( "cid_3_state", "0" );

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );
    pattern = "\\+CGACT: 4,([0-9]{1}).*";
    ret = at_tok_regular_more ( strstr ( data, "+CGACT: 4," ), pattern, regex_buf );
    if (0 == ret)
        nv_set ( "cid_4_state", regex_buf[1] );
    else
        nv_set ( "cid_4_state", "0" );
}

static char* get_max_qam ( int ul_or_dl, char* qam_data )
{
    int p_qam[8] = {0};
    int max_count = 0;
    int index = 0;
    int i_tmp = 0;
#if 0
    int max_val = 0;
    int p_qam_calc[4] = {0};
#endif

    if ( 0 == ul_or_dl )
    {
        max_count = 4;
        sscanf ( qam_data, "%d, %d, %d, %d, %d, %d, %d, %d",
                    &p_qam[0], &p_qam[1], &p_qam[2], &p_qam[3],
                    &p_qam[4], &p_qam[5], &p_qam[6], &p_qam[7] );
    }
    else
    {
        max_count = 3;
        sscanf ( qam_data, "%d, %d, %d, %d, %d, %d",
                    &p_qam[0], &p_qam[1], &p_qam[2],
                    &p_qam[3], &p_qam[4], &p_qam[5] );
    }

    for ( i_tmp = 0; i_tmp < max_count; i_tmp++ )
    {
#if 0
        p_qam_calc[i_tmp] = p_qam[i_tmp] + p_qam[i_tmp + max_count];
    }

    for ( max_val = p_qam_calc[0], i_tmp = 1; i_tmp < max_count; i_tmp++ )
    {
        if ( p_qam_calc[i_tmp] > max_val )
        {
            index = i_tmp;
            max_val = p_qam_calc[i_tmp];
        }
    }
#else
        if ( p_qam[i_tmp] || p_qam[i_tmp + max_count] )
            index = i_tmp;
    }
#endif

    /* mcs-Table    : QPSK, 16QAM, 64QAM, 256QAM    */
    /* 64QAMLowSE   : 0-14, 15-20, 21-28            */
    /* 64QAM        : 0-9 , 10-16, 17-28            */
    /* 256QAM       : 0-4 ,  5-10, 11-19, 20-27     */
    switch ( index )
    {
    case 1:
        return "16QAM";
    case 2:
        return "64QAM";
    case 3:
        return "256QAM";
    default:
        return "QPSK";
    }
}

void parsing_gct_gdmmodtc ( char* data )
{
    int ret = 0;
    char *pattern = NULL;
    char regex_buf[3][REGEX_BUF_ONE];

    if ( 0 == strcmp ( data, g_strQam ) )
    {
        return;
    }
    snprintf ( g_strQam, sizeof ( g_strQam ), "%s", data );

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );
    memset ( regex_buf[2], 0, REGEX_BUF_ONE );

    pattern = "\\%GDMMODTC: \"DL\", ([ ,0-9]*).*%GDMMODTC: \"UL\", ([ ,0-9]*).*OK.*$";

    ret = at_tok_regular_more ( strstr ( data, "%GDMMODTC:" ), pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( "dl_qam", get_max_qam ( 0, regex_buf[1] ) );
        nv_set ( "ul_qam", get_max_qam ( 1, regex_buf[2] ) );
    }
    else
    {
        nv_set ( "dl_qam", "QPSK" );
        nv_set ( "ul_qam", "QPSK" );
        CLOGD ( WARNING, "AT GDMMODTC return ERROR !\n" );
    }
}

void parsing_gct_gbler ( char* data )
{
    int ret = 0;
    int i = 0;
    char *pattern = NULL;
    char regex_buf[5][REGEX_BUF_ONE];

    if ( 0 == strcmp ( data, g_strBler ) )
    {
        return;
    }
    snprintf ( g_strBler, sizeof ( g_strBler ), "%s", data );

    pattern = "\\%GBLER: ([0-9]*),([0-9]*),([0-9]*),([0-9]*).*OK.*$";

    for ( i = 0; i < 5; i++ )
    {
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );
    }

    ret = at_tok_regular_more ( strstr ( data, "%GBLER:") , pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( "bler_err1", regex_buf[1] );
        nv_set ( "bler_err2", regex_buf[2] );
        nv_set ( "bler_total1", regex_buf[3] );
        nv_set ( "bler_total2", regex_buf[4] );
    }
    else
    {
        nv_set ( "bler_err1", "" );
        nv_set ( "bler_err2", "" );
        nv_set ( "bler_total1", "" );
        nv_set ( "bler_total2", "" );
    }
}

void parsing_gct_gharq ( char* data )
{
    int ret = 0;
    int i = 0;
    char *pattern = NULL;
    char regex_buf[7][REGEX_BUF_ONE];

    if ( 0 == strcmp ( data, g_strHarq ) )
    {
        return;
    }
    snprintf ( g_strHarq, sizeof ( g_strHarq ), "%s", data );

    pattern = "\\%GHARQ: ([0-9]*),([0-9]*),([0-9]*),([0-9]*),([0-9]*),([0-9]*).*OK.*$";

    for ( i = 0; i < 7; i++ )
    {
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );
    }

    ret = at_tok_regular_more ( data, pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( "initial_harq1", regex_buf[1] );
        nv_set ( "initial_harq2", regex_buf[2] );
        nv_set ( "initial_bleq1", regex_buf[3] );
        nv_set ( "initial_bleq2", regex_buf[4] );
        nv_set ( "retx_harq1", regex_buf[5] );
        nv_set ( "retx_harq2", regex_buf[6] );
    }
    else
    {
        nv_set ( "initial_harq1", "" );
        nv_set ( "initial_harq2", "" );
        nv_set ( "initial_bleq1", "" );
        nv_set ( "initial_bleq2", "" );
        nv_set ( "retx_harq1", "" );
        nv_set ( "retx_harq2", "" );
    }
}

void parsing_gct_defbrdp ( int cid_index, char* data )
{
    char apn_qci[32] = {0};
    int apn_index = get_apn_from_cid_index ( cid_index );

    if ( 0 == strcmp ( data, g_strDefbrdp[apn_index - 1] ) )
    {
        return;
    }
    snprintf ( g_strDefbrdp[apn_index - 1], STR_AT_RSP_LEN, "%s", data );

    snprintf ( apn_qci, sizeof ( apn_qci ), "apn%d_qci", apn_index );

    if ( 0 == strcmp ( data, "" ) )
    {
        nv_set ( apn_qci, "" );
        return;
    }

    int ret = 0;
    int i = 0;
    char *pattern = NULL;
    char regex_buf[6][REGEX_BUF_ONE];

    pattern = "\\%DEFBRDP: ([0-9]*),\"(.*)\",([0-9]*),([0-9]*),([0-9]*).*OK.*$";

    for ( i = 0; i < 6; i++ )
    {
        memset ( regex_buf[i], 0, REGEX_BUF_ONE );
    }

    ret = at_tok_regular_more ( data, pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( apn_qci, regex_buf[3] );
    }
    else
    {
        nv_set ( apn_qci, "" );
    }
}

static void get_max_cqi ( char* cw0_cqi, char* cw1_cqi )
{
    int p_cw0_cqi[16] = {0};
    int p_cw1_cqi[16] = {0};
    int p_cqi[16] = {0};
    int i = 0;
    int cqi = 0;
    char S_cqi[4] = {0};

    sscanf ( cw0_cqi, "%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d",
                    &p_cw0_cqi[0], &p_cw0_cqi[1], &p_cw0_cqi[2], &p_cw0_cqi[3],
                    &p_cw0_cqi[4], &p_cw0_cqi[5], &p_cw0_cqi[6], &p_cw0_cqi[7],
                    &p_cw0_cqi[8], &p_cw0_cqi[9], &p_cw0_cqi[10], &p_cw0_cqi[11],
                    &p_cw0_cqi[12], &p_cw0_cqi[13], &p_cw0_cqi[14], &p_cw0_cqi[15] );

    sscanf ( cw1_cqi, "%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d",
                    &p_cw1_cqi[0], &p_cw1_cqi[1], &p_cw1_cqi[2], &p_cw1_cqi[3],
                    &p_cw1_cqi[4], &p_cw1_cqi[5], &p_cw1_cqi[6], &p_cw1_cqi[7],
                    &p_cw1_cqi[8], &p_cw1_cqi[9], &p_cw1_cqi[10], &p_cw1_cqi[11],
                    &p_cw1_cqi[12], &p_cw1_cqi[13], &p_cw1_cqi[14], &p_cw1_cqi[15] );

    for ( i = 0; i < 16; i++ )
    {
        p_cqi[i] = p_cw0_cqi[i] + p_cw1_cqi[i];
    }

    for ( i = 1; i < 16; i++ )
    {
        if ( p_cqi[i] > p_cqi[i - 1] )
        {
            cqi = i;
        }
    }

    snprintf ( S_cqi, sizeof ( S_cqi ), "%d", cqi );

    nv_set ( "cqi", S_cqi );
}

void parsing_gct_gdmcqi ( char* data )
{
    int ret = 0;
    char *pattern = NULL;
    char regex_buf[3][REGEX_BUF_ONE];

    if ( 0 == strcmp ( data, g_strCqi ) )
    {
        return;
    }
    snprintf ( g_strCqi, sizeof ( g_strCqi ), "%s", data );

    pattern = "\\%GDMCQI: \"CW0\", ([ ,0-9]*).*%GDMCQI: \"CW1\", ([ ,0-9]*).*OK.*$";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );
    memset ( regex_buf[2], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "%GDMCQI: " ), pattern, regex_buf );

    if ( 0 == ret )
    {
        get_max_cqi ( regex_buf[1], regex_buf[2] );
    }
    else
    {
        nv_set ( "cqi", "0" );
        CLOGD ( WARNING, "AT GDMCQI return ERROR !\n" );
    }
}

void parsing_gct_grankindex ( char* data )
{
    int ret = 0;
    char *pattern = NULL;
    char regex_buf[2][REGEX_BUF_ONE];

    if ( 0 == strcmp ( data, g_strRankIndex ) )
    {
        return;
    }
    snprintf ( g_strRankIndex, sizeof ( g_strRankIndex ), "%s", data );

    pattern = "\\%GRANKINDEX: ([1-4]).*OK.*$";

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    ret = at_tok_regular_more ( strstr ( data, "%GRANKINDEX:" ), pattern, regex_buf );

    if ( 0 == ret )
    {
        nv_set ( "rankIndex", regex_buf[1] );
    }
    else
    {
        nv_set ( "rankIndex", "0" );
        CLOGD ( WARNING, "AT GRANKINDEX return ERROR !\n" );
    }
}

int parsing_gct_cgcontrdp ( int cid_index, char* data )
{
    int apn_index = get_apn_from_cid_index ( cid_index );

    if ( 0 == strcmp ( data, g_strIp4and6[apn_index - 1] ) )
        return -1;

    strncpy ( g_strIp4and6[apn_index - 1], data, STR_AT_RSP_LEN_4X - 1 );

    char regex_buf[7][REGEX_BUF_ONE];
    char addr_mask[3][REGEX_BUF_ONE];
    int ret = 0;
    int i = 0;
    int j = 0;
    char strRamOpt[32] = {0};
    char *pattern = NULL;
    char *ipv4v6_content = NULL;
    char ipv4v6_str[RECV_BUF_SIZE] = {0};
    int ipv4_updated = 0;
    int ipv6_updated = 0;
    int ipv6AddrLen = 0;
    char ipv6NewAddr[REGEX_BUF_ONE] = {0};
    LTE_DEV_MAC_ADDR_T macAddr;

    ipv4v6_content = strtok ( data, "\r\n" );
    while ( ipv4v6_content && i < 2 )
    {
        snprintf ( ipv4v6_str, RECV_BUF_SIZE, "%s", ipv4v6_content );
        CLOGD ( FINE, "Get line:\n%s\n\n", ipv4v6_str );
        if ( strstr ( ipv4v6_str, "+CGCONTRDP: " ) )
        {
            pattern = "\\+CGCONTRDP: [0-9]*,[0-9]*,\".*\","
                      "\"([0-9A-Fa-f.: ]*)\",\"([0-9A-Fa-f.:]*)\","
                      "\"([0-9A-Fa-f.:]*)\",\"([0-9A-Fa-f.:]*)\","
                      "\"([0-9A-Fa-f.:]*)\",\"([0-9A-Fa-f.:]*)\".*";

            for ( j = 0; j < 7; j++ )
            {
                memset ( regex_buf[j], 0, REGEX_BUF_ONE );
            }

            ret = at_tok_regular_more ( ipv4v6_str, pattern, regex_buf );
            if ( 0 == ret )
            {
                CLOGD ( FINE, "regex_buf[1]:\n[%s]\n\n", regex_buf[1] );
                if ( strstr ( regex_buf[1], ":" ) ) // ipv6
                {
                    pattern = "([0-9A-Fa-f:]*) (.*)";
                    for ( j = 0; j < 3; j++ )
                    {
                        memset ( addr_mask[j], 0, REGEX_BUF_ONE );
                    }

                    ret = at_tok_regular_more ( regex_buf[1], pattern, addr_mask );
                    if ( 0 == ret )
                    {
                        CLOGD( FINE, "v6_addr_mask[1]:\n[%s]\n\n", addr_mask[1] );
                        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_ip", apn_index );
                        ipv6AddrLen = strlen ( addr_mask[1] );
                        if ( ':' == addr_mask[1][ipv6AddrLen - 1] )
                        {
                            memset ( &macAddr, 0, sizeof ( macAddr ) );
                            snprintf ( macAddr.devName, sizeof ( macAddr.devName ),
#if defined(_GDM7243_)
                                                "lte0pdn%d"
#else
                                                "eth1.10%d"
#endif
                                                            , apn_index - 1 );

                            getMacAddr ( &macAddr );
                            snprintf ( ipv6NewAddr, sizeof ( ipv6NewAddr ),
                                                        "%s%02x%02x:%02x%02x:%02x%02x",
                                                        addr_mask[1],
                                                        macAddr.devMac[0], macAddr.devMac[1],
                                                        macAddr.devMac[2], macAddr.devMac[3],
                                                        macAddr.devMac[4], macAddr.devMac[5] );

                            CLOGD( FINE, "ipv6NewAddr:\n[%s]\n\n", ipv6NewAddr );
                            nv_set ( strRamOpt, ipv6NewAddr );
                        }
                        else
                        {
                            nv_set ( strRamOpt, addr_mask[1] );
                        }

                        CLOGD( FINE, "v6_addr_mask[2]:\n[%s]\n\n", addr_mask[2] );
                        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_mask", apn_index );
                        nv_set ( strRamOpt, addr_mask[2] );
                    }

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_gateway", apn_index );
                    nv_set ( strRamOpt, regex_buf[2] );

                    if ( 0 == strcmp ( regex_buf[3], "" ) && 0 == strcmp ( regex_buf[4], "" ) )
                    {
                        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_dns1", apn_index );
                        nv_set ( strRamOpt, "2001:4860:4860::8888" );
                        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_dns2", apn_index );
                        nv_set ( strRamOpt, "2001:4860:4860::8844" );
                    }
                    else
                    {
                        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_dns1", apn_index );
                        nv_set ( strRamOpt, regex_buf[3] );
                        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_dns2", apn_index );
                        nv_set ( strRamOpt, regex_buf[4] );
                    }

                    if ( apn_index == GCT_VOLTE_APN_INDEX )
                    {
                        nv_set ( "ipv6_pcscf", regex_buf[5] );
                        nv_set ( "ipv6_pcscf2", regex_buf[6] );
                    }

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_state", apn_index );
                    nv_set ( strRamOpt, "connect" );

                    ipv6_updated = 1;
                }
                else if ( strcmp ( regex_buf[1], "" ) ) // ipv4
                {
                    pattern = "([0-9]*.[0-9]*.[0-9]*.[0-9]*).(.*)";
                    for ( j = 0; j < 3; j++ )
                    {
                        memset ( addr_mask[j], 0, REGEX_BUF_ONE );
                    }

                    ret = at_tok_regular_more ( regex_buf[1], pattern, addr_mask );
                    if ( 0 == ret )
                    {
                        CLOGD( FINE, "addr_mask[1]:\n[%s]\n\n", addr_mask[1] );
                        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ip", apn_index );
                        nv_set ( strRamOpt, addr_mask[1] );
                        CLOGD( FINE, "addr_mask[2]:\n[%s]\n\n", addr_mask[2] );
                        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_mask", apn_index );
                        nv_set ( strRamOpt, addr_mask[2] );
                    }

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_gateway", apn_index );
                    nv_set ( strRamOpt, regex_buf[2] );

                    if ( 0 == strcmp ( regex_buf[3], "" ) && 0 == strcmp ( regex_buf[4], "" ) )
                    {
                        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_dns1", apn_index );
                        nv_set ( strRamOpt, "8.8.8.8" );
                        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_dns2", apn_index );
                        nv_set ( strRamOpt, "8.8.4.4" );
                    }
                    else
                    {
                        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_dns1", apn_index );
                        nv_set ( strRamOpt, regex_buf[3] );
                        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_dns2", apn_index );
                        nv_set ( strRamOpt, regex_buf[4] );
                    }

                    if ( apn_index == GCT_VOLTE_APN_INDEX )
                    {
                        nv_set ( "ip_pcscf", regex_buf[5] );
                        nv_set ( "ip_pcscf2", regex_buf[6] );
                    }

                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_state", apn_index );
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
        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ip", apn_index );
        nv_set ( strRamOpt, "" );
        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_mask", apn_index );
        nv_set ( strRamOpt, "" );
        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_gateway", apn_index );
        nv_set ( strRamOpt, "" );
        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_dns1", apn_index );
        nv_set ( strRamOpt, "" );
        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_dns2", apn_index );
        nv_set ( strRamOpt, "" );
        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_state", apn_index );
        nv_set ( strRamOpt, "disconnect" );
        if ( apn_index == GCT_VOLTE_APN_INDEX )
        {
            nv_set ( "ip_pcscf", "" );
            nv_set ( "ip_pcscf2", "" );
        }
    }

    if ( 0 == ipv6_updated )
    {
        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_ip", apn_index );
        nv_set ( strRamOpt, "" );
        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_mask", apn_index );
        nv_set ( strRamOpt, "" );
        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_gateway", apn_index );
        nv_set ( strRamOpt, "" );
        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_dns1", apn_index );
        nv_set ( strRamOpt, "" );
        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_dns2", apn_index );
        nv_set ( strRamOpt, "" );
        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_state", apn_index );
        nv_set ( strRamOpt, "disconnect" );
        if ( apn_index == GCT_VOLTE_APN_INDEX )
        {
            nv_set ( "ipv6_pcscf", "" );
            nv_set ( "ipv6_pcscf2", "" );
        }
    }

    if ( ipv4_updated || ipv6_updated )
    {
        if ( 1 == apns_msg_flag[apn_index] )
        {
            apns_msg_flag[apn_index] = 0;
        }
    }

    return ( ipv4_updated | ipv6_updated );
}

void parsing_gct_wanTraffic ( char* data )
{
    char *p1 = NULL;
    char *p2 = NULL;
    char retData[RECV_BUF_SIZE * 4] = {0};
    char resData[RECV_BUF_SIZE * 4] = {0};

    if ( NULL == data )
    {
        return;
    }

    memset ( retData, 0, sizeof ( retData ) );
    memset ( resData, 0, sizeof ( resData ) );

    strcpy ( retData, data );
    p1 = strstr ( retData, "\%SYSCMD: " );

    while ( p1 )
    {
        p2 = p1 + strlen ( "\%SYSCMD: " );
        strcpy ( retData, p2 );
        p2 = strstr ( retData, "\%SYSCMD: " );
        if ( p2 )
        {
            strncat ( resData, retData, strlen ( retData ) - strlen ( p2 ) );
            p1 = p2;
        }
        else
        {
            p2 = strstr ( retData, "OK" );
            if ( p2 )
            {
                strncat ( resData, retData, strlen ( retData ) - strlen ( p2 ) );
            }
            else
            {
                strcat ( resData, retData );
            }
            p1 = NULL;
        }
    }

    if ( 0 != strcmp ( resData, "" ) )
    {
        FILE *fp = NULL;
        fp = fopen ( "/tmp/wanTraffic", "wb+" );
        if ( NULL == fp )
        {
            CLOGD ( WARNING, "open wanTraffic failed !\n" );
            return;
        }
        fwrite ( resData, strlen ( resData ), 1, fp );
        fclose ( fp );
    }
}

