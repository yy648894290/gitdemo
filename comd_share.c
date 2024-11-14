#include "comd_share.h"
#include "atchannel.h"
#include "at_tok.h"
#include "band_freq_info.h"
#include "config_key.h"
#include "hal_platform.h"

int atProcess_sem_id;
int allSms_sem_id;
LTEINFO_AT_MSG pccInfo;

extern char volte_enable[8];
extern int module_id;
extern COMD_DEV_INFO at_dev;

#if defined(CONFIG_SW_APN_MAX_NUM)
#define MAX_WAN_COUNT "1"
#else
#define MAX_WAN_COUNT "4"
#endif

#define MAX_LOG_F_SIZE 2097152  // 2M

#define _COMD_OWN_LOG_ 1
#define SYS_LOG_ENABLE 0

#define LOG_LEVEL_QTY       4
#define LOG_MSG_MAX_BUFF    2048

static FILE *log_file = NULL;
static int log_fd = -1;
static int comd_log_level = 0;
static int comd_log_enable = 0;

const char* logLevelStr[LOG_LEVEL_QTY] =
{
    "ERROR",
    "WARNING",
    "FINE",
    "DETAILS"
};

void comd_log_init ()
{
    int log_mask = 0;
    char *tmp_str = NULL;
    char log_conf[16] = {0};
    char log_path[64] = {0};

    sys_get_config ( LTE_PARAMETER_COMD_LOG_LEVEL, log_conf, sizeof ( log_conf ) );

    comd_log_level = 0;
    comd_log_enable = 0;

    tmp_str = strtok ( log_conf, "," );
    if ( NULL == tmp_str )
    {
        return;
    }

    comd_log_level = atoi ( tmp_str );
    if ( comd_log_level < 0 || LOG_LEVEL_QTY < comd_log_level )
    {
        comd_log_level = 0;
    }

    tmp_str = strtok ( NULL, "," );
    if ( NULL == tmp_str )
    {
        return;
    }

    log_mask = atoi ( tmp_str );

    comd_log_enable = ( log_mask & ( 1 << module_id ) );

    if ( 0 < comd_log_enable && NULL == log_file )
    {
        snprintf ( log_path, sizeof ( log_path ), "/var/log/comd_m%d.log", module_id + 1 );
        log_file = fopen ( log_path, "a+" );
    }

    // DANGEROUS !!! DANGEROUS !!! DANGEROUS !!!
    // if third param is 1, means not check log file size, it may be very large.
    tmp_str = strtok ( NULL, "," );
    if ( tmp_str && 0 == strcmp ( tmp_str, "1" ) )
        log_fd = -1;
    else
        log_fd = log_file ? fileno ( log_file ) : -1;
}

void logEntry ( const char* func, int line, const char* level, const char *fmt, ... )
{
#if _COMD_OWN_LOG_
    int ind = 0;

    if ( comd_log_enable <= 0 )
    {
        return;
    }

    for ( ; ind < comd_log_level; ind++ )
    {
        if ( 0 == strcmp ( level, logLevelStr [ ind ] ) )
            break;
    }

    if ( ind == comd_log_level )
    {
        return;
    }

    char msg[LOG_MSG_MAX_BUFF] = {0};
    char time_stamp[32] = {0};

#if SYS_LOG_ENABLE
    memset ( time_stamp, 0, sizeof ( time_stamp ) );
#else
    struct timeval tv;
    struct tm tr;

    gettimeofday ( &tv, NULL );
    localtime_r ( &tv.tv_sec, &tr );

    snprintf ( time_stamp, sizeof ( time_stamp ),
                    "%02d:%02d:%02d.%03d >| ",
                    tr.tm_hour, tr.tm_min,
                    tr.tm_sec, ( int ) tv.tv_usec / 1000 );
#endif

    int offset = snprintf ( msg, LOG_MSG_MAX_BUFF - 1,
                                "%sCOMD >| %7s >| [M%d][%s:%d] >| ",
                                time_stamp, level, module_id + 1, func, line );

    va_list ap;
    va_start ( ap, fmt );
    vsnprintf ( &msg[offset], LOG_MSG_MAX_BUFF - 1 - offset, fmt, ap );
    va_end ( ap );

#if SYS_LOG_ENABLE
    openlog ( "", LOG_PID | LOG_CONS, LOG_USER );
    syslog ( LOG_INFO, "%s", msg );
    closelog();
#else
    struct stat st;
    char b_r_comdlog[32] = {0};

    if ( log_file )
    {
        if ( -1 != log_fd )
        {
            fstat ( log_fd, &st );
            if ( st.st_size > MAX_LOG_F_SIZE )
            {
                snprintf ( b_r_comdlog, sizeof ( b_r_comdlog ),
                           "/lib/backup_reset_comdlog.sh %d", module_id + 1 );
                system ( b_r_comdlog );
            }
        }
        fprintf ( log_file, "%s", msg );
    }
#endif
#else
    char msg[LOG_MSG_MAX_BUFF] = {0};

    int offset = snprintf ( msg, sizeof ( msg ), "[M%d][%s:%d] >| ", module_id + 1, func, line );

    va_list ap;
    va_start ( ap, fmt );
    vsnprintf ( &msg[offset], sizeof ( msg ) - 1 - offset, fmt, ap );
    va_end ( ap );

    if ( 0 == strcmp ( level, "ERROR" ) )
    {
        ERR ( "%s", msg );
    }
    else if ( 0 == strcmp ( level, "WARNING" ) )
    {
        ERR ( "%s", msg );
    }
    else if ( 0 == strcmp ( level, "FINE" ) )
    {
        INFO ( "%s", msg );
    }
    else if ( 0 == strcmp ( level, "DETAILS" ) )
    {
        DBG ( "%s", msg );
    }
    else
    {
        TRACE ( "%s", msg );
    }
#endif
}

void ucfg_info_get ( char* nodeStr, char* node, char* get_val, int get_len )
{
    char at_req[128] = {0};
    char at_rep[RECV_BUF_SIZE] = {0};
    char regex_buf[2][REGEX_BUF_ONE];
    char patternStr[64] = {0};
    char regex_node[64] = {0};

    snprintf(at_req, sizeof(at_req), "AT%%SYSCMD=\"ucfg get %s\"", nodeStr);

    COMD_AT_PROCESS ( at_req, 3000, at_rep, sizeof ( at_rep ) );

    memset ( regex_buf[0], 0, REGEX_BUF_ONE );
    memset ( regex_buf[1], 0, REGEX_BUF_ONE );

    snprintf(regex_node, sizeof(regex_node), "SYSCMD: %s=", node);
    snprintf(patternStr, sizeof(patternStr), "%s([^\r\n]*)[\r\n]*OK[\r\n]*$", regex_node);
    at_tok_regular_more ( strstr(at_rep, regex_node), patternStr, regex_buf );
    snprintf ( get_val, get_len, "%s", regex_buf[1] );
}

void ucfg_info_set ( char* nodeStr, char* set_val )
{
    char at_req[128] = {0};
    char at_rep[RECV_BUF_SIZE] = {0};

    snprintf(at_req, sizeof(at_req), "AT%%SYSCMD=\"ucfg set %s '%s'\"", nodeStr, set_val);

    COMD_AT_PROCESS ( at_req, 3000, at_rep, sizeof ( at_rep ) );
}

int comd_wait_ms ( int time_ms )
{
    struct timeval tv;
    tv.tv_sec = time_ms / 1000;
    tv.tv_usec = ( time_ms % 1000 ) * 1000;

    struct timeval* tmp = time_ms ? ( &tv ) : NULL;

    return select ( 0, NULL, NULL, NULL, tmp );
}

int comd_exec ( char *cmd, char *buf, int buf_len )
{
    FILE * fd = NULL;
    if ( ( fd = popen ( cmd, "r" ) ) == NULL )
        return -1;

    char get_buf[RECV_BUF_SIZE] = {0};
    int get_lines = 0;

    while ( fgets ( get_buf, sizeof ( get_buf ), fd ) )
    {
        snprintf ( buf + strlen ( buf ), buf_len - strlen ( buf ), "%s", get_buf );
        get_lines++;
        memset ( get_buf, 0, sizeof ( get_buf ) );
    }

    pclose ( fd );

    return get_lines;
}

int getMacAddr ( LTE_DEV_MAC_ADDR_T *pMacInfo )
{
    int ret = -1;
    int sock = -1;
    struct ifreq req;

    if ( NULL == pMacInfo )
        return -1;

    sock = socket ( AF_INET, SOCK_DGRAM, 0 );
    if ( sock < 0 )
        return -1;

    strcpy ( req.ifr_name, pMacInfo->devName );

    ret = ioctl ( sock, SIOCGIFHWADDR, &req );
    if ( ret < 0 )
        return -1;

    close ( sock );

    memcpy ( pMacInfo->devMac, req.ifr_hwaddr.sa_data, ETH_ALEN );

    return 0;
}

/*
*   sem define in comd
*/
static int comd_set_semvalue ( int sem_id )
{
    union semun sem_union;

    sem_union.val = 1;
    if ( semctl ( sem_id, 0, SETVAL, sem_union ) == -1 )
        return 1;

    return 0;
}

int comd_semaphore_p ( int sem_id )
{
    struct sembuf sem_b;
    sem_b.sem_num = 0;
    sem_b.sem_op = -1;
    sem_b.sem_flg = SEM_UNDO;
    if ( semop ( sem_id, &sem_b, 1 ) == -1 )
    {
        CLOGD ( ERROR, "comd_semaphore_p failed\n" );
        return 1;
    }

    return 0;
}

int comd_semaphore_v ( int sem_id )
{
    struct sembuf sem_b;
    sem_b.sem_num = 0;
    sem_b.sem_op = 1;
    sem_b.sem_flg = SEM_UNDO;
    if ( semop ( sem_id, &sem_b, 1 ) == -1 )
    {
        CLOGD ( ERROR, "comd_semaphore_v failed\n" );
        return 1;
    }

    return 0;
}

int comd_sem_init ( key_t sem_key )
{
    switch ( sem_key )
    {
    case AT_PROCESS_SEM_KEY     :
        CLOGD ( FINE, "start init AT_PROCESS_SEM_KEY ...\n" );
        atProcess_sem_id = semget ( sem_key, 1, 0666 );
        if ( atProcess_sem_id == -1 )
        {
            atProcess_sem_id = semget ( sem_key, 1, 0666 | IPC_CREAT );
            if ( atProcess_sem_id == -1 )
            {
                CLOGD ( ERROR, "atProcess_sem_id semget failed\n" );
                return 1;
            }
            if ( comd_set_semvalue ( atProcess_sem_id ) )
            {
                CLOGD ( ERROR, "Failed to initialize atProcess_sem_id\n" );
                return 1;
            }
        }
        break;
    case ALL_SMS_SEM_KEY        :
        CLOGD ( FINE, "start init ALL_SMS_SEM_KEY ...\n" );
        allSms_sem_id = semget ( sem_key, 1, 0666 );
        if ( allSms_sem_id == -1 )
        {
            allSms_sem_id = semget ( sem_key, 1, 0666 | IPC_CREAT );
            if ( allSms_sem_id == -1 )
            {
                CLOGD ( ERROR, "allSms_sem_id semget failed\n" );
                return 1;
            }
            if ( comd_set_semvalue ( allSms_sem_id ) )
            {
                CLOGD ( ERROR, "Failed to initialize allSms_sem_id\n" );
                return 1;
            }
        }
        break;
    default                     :
        CLOGD ( ERROR, "UNKNOW_SEM_KEY ...\n" );
        return 1;
    }

    return 0;
}

int comd_sem_exit ( int sem_id )
{
    union semun sem_union;

    if ( semctl ( sem_id, 0, IPC_RMID, sem_union ) == -1 )
    {
        CLOGD ( ERROR, "semaphore delete failed\n" );
        return 1;
    }

    return 0;
}

int apns_msg_flag[5] = {0};

void reset_apns_msg_flag ( int flag )
{
    int i = 0;

    for ( ; i < sizeof ( apns_msg_flag ) / sizeof ( int ); i++ )
    {
        if ( flag < 0 && 1 == apns_msg_flag[i] )
        {
            apns_msg_flag[i] = -1;
        }
        else if ( 0 == flag || 1 == flag )
        {
            apns_msg_flag[i] = flag;
        }
    }
}

void HexToStr ( char *pbDest, const char *pbSrc, int nLen )
{
    char h1, h2;
    unsigned char s1, s2;
    int i = 0;
    int j = 0;

    for ( i = 0; i < nLen; i++ )
    {
        h1 = pbSrc[2 * i];
        h2 = pbSrc[2 * i + 1];

        if ( h1 < '0' || h1 > '7' )
            continue;

        s1 = toupper ( h1 ) - 0x30;
        if ( s1 > 9 ) s1 -= 7;

        s2 = toupper ( h2 ) - 0x30;
        if ( s2 > 9 ) s2 -= 7;

        pbDest[j++] = s1 * 16 + s2;
    }

    pbDest[j] = '\0';
}

void comd_reboot_module()
{
    CLOGD ( ERROR, "comd abnormal, reboot system or module !!!\n" );
    system ( "/lib/comd_abnormal_reboot.sh" );
}

void lteinfo_data_restore()
{
    nv_set ( "rrc_state",        "--" );
    nv_set ( "dl_earfcn",        "--" );
    nv_set ( "ul_earfcn",        "--" );
    nv_set ( "mcc",              "--" );
    nv_set ( "mnc",              "--" );
    nv_set ( "tac_org",          "--" );
    nv_set ( "tac",              "--" );
    nv_set ( "globalid",         "--" );
    nv_set ( "cellid",           "--" );
    nv_set ( "eNBid",            "--" );
    nv_set ( "band",             "--" );
    nv_set ( "mode",             "--" );
    nv_set ( "bandwidth",        "--" );
    nv_set ( "dl_bandwidth",     "--" );
    nv_set ( "ul_bandwidth",     "--" );
    nv_set ( "cinr",             "--" );
    nv_set ( "cinr0",            "--" );
    nv_set ( "cinr1",            "--" );
    nv_set ( "cinr2",            "--" );
    nv_set ( "cinr3",            "--" );
    nv_set ( "pci",              "--" );
    nv_set ( "rsrq",             "--" );
    nv_set ( "rsrq0",            "--" );
    nv_set ( "rsrq1",            "--" );
    nv_set ( "rsrq2",            "--" );
    nv_set ( "rsrq3",            "--" );
    nv_set ( "rsrp",             "--" );
    nv_set ( "rsrp0",            "--" );
    nv_set ( "rsrp1",            "--" );
    nv_set ( "rsrp2",            "--" );
    nv_set ( "rsrp3",            "--" );
    nv_set ( "rssi",             "--" );
    nv_set ( "rssi0",            "--" );
    nv_set ( "rssi1",            "--" );
    nv_set ( "rssi2",            "--" );
    nv_set ( "rssi3",            "--" );
    nv_set ( "signal_bar",       "-1" );
    nv_set ( "dl_frequency",     "--" );
    nv_set ( "ul_frequency",     "--" );
    nv_set ( "dlmcs",            "--" );
    nv_set ( "ulmcs",            "--" );
    nv_set ( "txpower",          "--" );
    nv_set ( "sinr",             "--" );
    nv_set ( "tx_mode",          "--" );
    nv_set ( "handover_failed",   "0" );
    nv_set ( "handover_total",    "0" );
    nv_set ( "bears_count",       "0" );
    nv_set ( "ta_value",         "--" );
    nv_set ( "lte_rankIndex",    "--" );

    memset ( &pccInfo, 0, sizeof ( pccInfo ) );
}

void ipv4v6_data_restore ( int apn_index )
{
    int idx = apn_index;
    int max_count = apn_index;
    char ram_node[32] = {0};

    if ( 0 == apn_index )
    {
        idx = 1;
        max_count = 4;
    }

    for ( ; idx <= max_count; idx++ )
    {
        if ( GCT_VOLTE_APN_INDEX == idx )
        {
            nv_set ( "last_ip_volte", "" );
            nv_set ( "ip_pcscf", "" );
            nv_set ( "ip_pcscf2", "" );
            nv_set ( "last_ip_pcscf", "" );

            nv_set ( "last_ipv6_volte", "" );
            nv_set ( "ipv6_pcscf", "" );
            nv_set ( "ipv6_pcscf2", "" );
            nv_set ( "last_ipv6_pcscf", "" );
        }

        snprintf ( ram_node, sizeof ( ram_node ), "apn%d_ip", idx );
        nv_set ( ram_node, "" );

        snprintf ( ram_node, sizeof ( ram_node ), "apn%d_mask", idx );
        nv_set ( ram_node, "" );

        snprintf ( ram_node, sizeof ( ram_node ), "apn%d_gateway", idx );
        nv_set ( ram_node, "" );

        snprintf ( ram_node, sizeof ( ram_node ), "apn%d_dns1", idx );
        nv_set ( ram_node, "" );

        snprintf ( ram_node, sizeof ( ram_node ), "apn%d_dns2", idx );
        nv_set ( ram_node, "" );

        snprintf ( ram_node, sizeof ( ram_node ), "apn%d_state", idx );
        nv_set ( ram_node, "disconnect" );

        snprintf ( ram_node, sizeof ( ram_node ), "apn%d_ipv6_ip", idx );
        nv_set ( ram_node, "" );

        snprintf ( ram_node, sizeof ( ram_node ), "apn%d_ipv6_mask", idx );
        nv_set ( ram_node, "" );

        snprintf ( ram_node, sizeof ( ram_node ), "apn%d_ipv6_dns1", idx );
        nv_set ( ram_node, "" );

        snprintf ( ram_node, sizeof ( ram_node ), "apn%d_ipv6_dns2", idx );
        nv_set ( ram_node, "" );

        snprintf ( ram_node, sizeof ( ram_node ), "apn%d_ipv6_state", idx );
        nv_set ( ram_node, "disconnect" );
    }
}

void memory_data_restore()
{
    nv_set ( "main_status", "module_communication_abnormal" );
    nv_set ( "operator",            "" );
    nv_set ( "cpin",           "ERROR" );
    nv_set ( "SIM_SPN",             "" );
    nv_set ( "iccid",               "" );
    nv_set ( "imsi",                "" );
    nv_set ( "simlocked",          "0" );
    nv_set ( "lte_on_off",        "on" );
    nv_set ( "sms_center_num",      "" );
    nv_set ( "sms_max_num",      "120" );
    nv_set ( "connected_time",     "0" );
    nv_set ( "wancount", MAX_WAN_COUNT );
    nv_set ( "m_netselect_status", "manual_search_fail" );

    lteinfo_data_restore();

    nv_set ( "cqi",            "--" );
    nv_set ( "rankIndex",      "--" );
    nv_set ( "ul_rankIndex",   "--" );
    nv_set ( "dl_qam",       "QPSK" );
    nv_set ( "ul_qam",       "QPSK" );
    nv_set ( "cereg_stat",      "0" );
    nv_set ( "cgatt_val",       "0" );
    nv_set ( "cid_1_state",     "0" );
    nv_set ( "cid_2_state",     "0" );
    nv_set ( "cid_3_state",     "0" );
    nv_set ( "cid_4_state",     "0" );
    nv_set ( "apn1_qci",         "" );
    nv_set ( "apn2_qci",         "" );
    nv_set ( "apn3_qci",         "" );
    nv_set ( "apn4_qci",         "" );
    nv_set ( "bler_err1",       "0" );
    nv_set ( "bler_err2",       "0" );
    nv_set ( "bler_total1",     "0" );
    nv_set ( "bler_total2",     "0" );
    nv_set ( "initial_harq1",   "0" );
    nv_set ( "initial_harq2",   "0" );
    nv_set ( "initial_bleq1",   "0" );
    nv_set ( "initial_bleq2",   "0" );
    nv_set ( "retx_harq1",      "0" );
    nv_set ( "retx_harq2",      "0" );

    nv_set ( "secondary_cell",  "0" );

    nv_set ( "5g_cqi",         "--" );
    nv_set ( "5g_dlmcs",       "--" );
    nv_set ( "5g_ulmcs",       "--" );
    nv_set ( "5g_rankIndex",   "--" );

    ipv4v6_data_restore ( 0 );
}

/* used for GCT module */
int get_cid_from_apn_index ( int index )
{
    return ( 1 == index ) ? 3 : ( ( 2 == index ) ? 1 : ( ( 3 == index ) ? 2 : 4 ) );
}

int get_apn_from_cid_index ( int index )
{
    return ( 3 == index ) ? 1 : ( ( 1 == index ) ? 2 : ( ( 2 == index ) ? 3 : 4 ) );
}

void clean_connected_time ()
{
    char gateway[4] = {0};

    set_connected_total_time();
    nv_get ( "default_gateway", gateway, sizeof ( gateway ) );

    if ( strcmp ( gateway, "9" ) )
    {
        nv_set ( "connected_time", "0" );
    }
}

void set_connected_total_time()
{
    char time_str[16] = {0};
    long conn_time = 0;
    long total_time = 0;

    nv_get ( "connected_time", time_str, sizeof( time_str ) );
    if ( 0 != strcmp ( time_str, "0" ) )
    {
        struct sysinfo info;
        sysinfo ( &info );
        if(info.uptime > atol(time_str))
        {
            conn_time = info.uptime - atol(time_str);
            nv_get ( "total_time", time_str, sizeof( time_str ) );
            if(0 != strcmp ( time_str, "" ))
            {
                total_time = conn_time + atol(time_str);
            }
            else
            {
                total_time = conn_time;
            }
            snprintf ( time_str, sizeof ( time_str ), "%ld", total_time );
            nv_set ( "total_time",  time_str );
        }
    }
}

static void send_pcscf_update_msg2sod ( int wait_time )
{
    char v4_pcscf_val[32] = {0};
    char v6_pcscf_val[64] = {0};

    if ( 0 == strcmp ( volte_enable, "VOLTE" ) )
    {
        nv_get ( "ip_pcscf", v4_pcscf_val, sizeof ( v4_pcscf_val ) );
        nv_get ( "ipv6_pcscf", v6_pcscf_val, sizeof ( v6_pcscf_val ) );
        if ( strcmp ( v4_pcscf_val, "" ) || strcmp ( v6_pcscf_val, "" ) )
        {
            comd_wait_ms ( wait_time );
            MessageEvtSend ( MESSAGE_ID_SODD, MSG_EVT_P_CSCF_IP_CHANGE, NULL, 0 );
        }
    }
}

void apnActOrDeactMsgEvent ( int apn_index, int apn_event )
{
    char apnStr[8] = {0};
    int i = 1;
    char gateway[4] = {0};

    if ( apn_index <= 0 )
    {
        if ( apn_event != apns_msg_flag[0] )
        {
            apns_msg_flag[0] = apn_event;
            if ( 0 == apn_event )
            {
                CLOGD ( FINE, "MSG_SODD_EVT_NETWORK_DOWN\n" );
                SYS_INFO ( "The network is disconnected" );
                clean_connected_time ();
                MessageEvtSend ( MESSAGE_ID_SODD, MSG_SODD_EVT_NETWORK_DOWN, NULL, 0 );
                ipv4v6_data_restore ( 0 );
                for ( i = 1; i <= 4; i++ )
                {
                    if ( 0 != apns_msg_flag[i] )
                    {
                        snprintf ( apnStr, sizeof ( apnStr ), "APN%d", i );
                        apns_msg_flag[i] = 0;
                        MessageEvtSend ( MESSAGE_ID_NETD, MSG_EVT_APN_DEACT,
                                                        apnStr, strlen ( apnStr ) + 1 );
                    }
                }
            }
            else
            {
                CLOGD ( FINE, "MSG_SODD_EVT_NETWORK_UP\n" );
                SYS_INFO ( "The network is connected" );
                MessageEvtSend ( MESSAGE_ID_SODD, MSG_SODD_EVT_NETWORK_UP, NULL, 0 );
            }
        }
    }
    else if ( apn_index <= 4 )
    {
        if ( apn_event != apns_msg_flag[apn_index] )
        {
            apns_msg_flag[apn_index] = apn_event;
            snprintf ( apnStr, sizeof ( apnStr ), "APN%d", apn_index );
            if ( 0 == apn_event )
            {
                CLOGD ( FINE, "MSG_EVT_APN_DEACT: [%s]\n", apnStr );
                SYS_INFO ( "%s is deactive", apnStr );
                MessageEvtSend ( MESSAGE_ID_NETD, MSG_EVT_APN_DEACT,
                                                apnStr, strlen ( apnStr ) + 1 );
            }
            else
            {
#if defined(_QCA_IPQ_)
                if ( M_VENDOR_QUECTEL_X55 == at_dev.comd_mVendor )
                {
                    comd_wait_ms ( 3000 ); // wait for quectel-CM to up usb0
                }
                system("/etc/init.d/re_init_lan start &");
#else
                if ( M_VENDOR_QUECTEL_T750 == at_dev.comd_mVendor )
                {
                    comd_wait_ms ( 3000 ); // wait for quectel-CM to up usb0
                }
#endif
                CLOGD ( FINE, "MSG_EVT_APN_ACT: [%s]\n", apnStr );
                SYS_INFO ( "%s is atctive", apnStr );
                MessageEvtSend ( MESSAGE_ID_NETD, MSG_EVT_APN_ACT,
                                                apnStr, strlen ( apnStr ) + 1 );

                nv_get ( "default_gateway", gateway, sizeof ( gateway ) );
                if ( 1 == apn_index || apn_index == atoi ( gateway ) )
                {
                    CLOGD ( FINE, "MSG_EVT_APN_CHANGE: [%s]\n", apnStr );
                    MessageEvtSend ( MESSAGE_ID_SODD, MSG_EVT_APN_CHANGE, NULL, 0 );
                }

                CLOGD ( FINE, "MSG_EVT_APN_IP_CHANGE: [%s]\n", apnStr );
                SYS_INFO ( "%s ip has changed", apnStr );
                snprintf ( apnStr, sizeof ( apnStr ), "%d", apn_index );
                MessageEvtSend ( MESSAGE_ID_SODD, MSG_EVT_APN_IP_CHANGE,
                                                apnStr, strlen ( apnStr ) + 1 );

#if defined(_MTK_7621_) || defined(_MTK_7628_)
                if ( GCT_VOLTE_APN_INDEX == apn_index )
                {
                    send_pcscf_update_msg2sod ( 1000 );
                }
#endif
            }
        }
    }
}

CELL_EARFCN_FREQ earfcn_freq;

int calc_band_from_earfcn ( int earfcn_dl, int bands[], int bands_count )
{
    int i = 0;
    int j = 0;

    if ( bands_count > 0 )
    {
        for ( i = 0; i < bands_count; i++ )
        {
            if ( 1 <= bands[i] && bands[i] <= 255 )
            {
                j = bands[i];
            }
            else
            {
                continue;
            }

            if ( earfcn_dl >= bandFreqRanges[j].dl_start_earfcn &&
                    earfcn_dl < bandFreqRanges[j].dl_start_earfcn + ( bandFreqRanges[j].band_width / 100 ) )
            {
                return bandFreqRanges[j].band;
            }
        }
    }
    else
    {
        for ( i = 1; i < ALL_BAND_NUMS; i++ )
        {
            if ( earfcn_dl >= bandFreqRanges[i].dl_start_earfcn &&
                    earfcn_dl < bandFreqRanges[i].dl_start_earfcn + ( bandFreqRanges[i].band_width / 100 ) )
            {
                return bandFreqRanges[i].band;
            }
        }
    }

    return 0;
}

int calc_freq_from_earfcn ( CELL_EARFCN_FREQ* info )
{
    int i = 1;
    int earfcn_count = 0;
    int earfcn_plus_val = 0;

    for ( ; i < ALL_BAND_NUMS; i++ )
    {
        if ( info->band != bandFreqRanges[i].band )
        {
            continue;
        }

        earfcn_count = ( bandFreqRanges[i].band_width / 100 );
        if ( info->dl_earfcn >= bandFreqRanges[i].dl_start_earfcn &&
                info->dl_earfcn < bandFreqRanges[i].dl_start_earfcn + earfcn_count )
        {
            earfcn_plus_val = info->dl_earfcn - bandFreqRanges[i].dl_start_earfcn;
            info->ul_earfcn = bandFreqRanges[i].ul_start_earfcn + earfcn_plus_val;
            info->dl_frequency = bandFreqRanges[i].dl_start_freq + ( earfcn_plus_val * 100 );
            info->ul_frequency = bandFreqRanges[i].ul_start_freq + ( earfcn_plus_val * 100 );
            return 0;
        }

        return -1;
    }

    return 1;
}

int calc_5g_freq_from_narfcn ( CELL_EARFCN_FREQ* info_5g )
{
    int i = 0;
    int narfcn_count = 0;
    int narfcn_plus_val = 0;

    for ( ; i < ALL_5G_BAND_NUMS; i++ )
    {
        if ( info_5g->band != bandFreqRanges_5g[i].band || bandFreqRanges_5g[i].dl_start_narfcn <= 0 )
        {
            continue;
        }

        narfcn_count = bandFreqRanges_5g[i].band_width / bandFreqRanges_5g[i].raster_ref;
        if ( info_5g->dl_earfcn >= bandFreqRanges_5g[i].dl_start_narfcn &&
                info_5g->dl_earfcn < bandFreqRanges_5g[i].dl_start_narfcn + narfcn_count )
        {
            narfcn_plus_val = info_5g->dl_earfcn - bandFreqRanges_5g[i].dl_start_narfcn;
            info_5g->ul_earfcn = bandFreqRanges_5g[i].ul_start_narfcn + narfcn_plus_val;
            info_5g->dl_frequency = bandFreqRanges_5g[i].dl_start_freq + narfcn_plus_val * bandFreqRanges_5g[i].raster_ref;
            info_5g->ul_frequency = bandFreqRanges_5g[i].ul_start_freq + narfcn_plus_val * bandFreqRanges_5g[i].raster_ref;
            return 0;
        }

        return -1;
    }

    return 1;
}

int calc_5g_duplex_mode_from_narfcn ( int narfcn_dl )
{
    int i = 0;
    int narfcn_count = 0;

    for ( ; i < ALL_5G_BAND_NUMS; i++ )
    {
        if ( bandFreqRanges_5g[i].dl_start_narfcn <= 0 )
        {
            continue;
        }
        narfcn_count = bandFreqRanges_5g[i].band_width / bandFreqRanges_5g[i].raster_ref;
        if ( narfcn_dl >= bandFreqRanges_5g[i].dl_start_narfcn &&
                narfcn_dl < bandFreqRanges_5g[i].dl_start_narfcn + narfcn_count )
        {
            return bandFreqRanges_5g[i].band_mode;
        }
    }

    return 0;
}

int calc_4g_band_from_earfcn( int earfcn_dl, char *band_list, int length )
{
    int i = 0;

    char band[8] = {0};
    char suppband[256] = {0};
    char suppband_org[256] = {0};

    nv_get ( "suppband", suppband_org, sizeof ( suppband_org ) );

    snprintf( suppband,sizeof(suppband)," %s ",suppband_org );

    for ( ; i < ALL_BAND_NUMS; i++ )
    {
        if ( bandFreqRanges[i].band_width <= 0 )
        {
            continue;
        }
        if ( earfcn_dl >= bandFreqRanges[i].dl_start_earfcn &&
                    earfcn_dl < bandFreqRanges[i].dl_start_earfcn + ( bandFreqRanges[i].band_width / 100 ) )
        {
            snprintf ( band ,sizeof( band ), " %d ",bandFreqRanges[i].band );
            if( NULL != strstr ( suppband,band ) )
            {
                snprintf ( band_list + strlen ( band_list ), length - strlen ( band_list ), "%d:", bandFreqRanges[i].band);
            }
        }
    }

    return 0;
}


int calc_5g_band_from_narfcn( int narfcn_dl, char *band_list, int length )
{
    int i = 0;
    int narfcn_count = 0;

    char band[8] = {0};
    char suppband5g[256] = {0};
    char suppband5g_org[256] = {0};

    nv_get ( "suppband5g", suppband5g_org, sizeof ( suppband5g_org ) );

    snprintf( suppband5g,sizeof(suppband5g)," %s ",suppband5g_org );

    for ( ; i < ALL_5G_BAND_NUMS; i++ )
    {
        if ( bandFreqRanges_5g[i].dl_start_narfcn <= 0 )
        {
            continue;
        }
        narfcn_count = bandFreqRanges_5g[i].band_width / bandFreqRanges_5g[i].raster_ref;
        if ( narfcn_dl >= bandFreqRanges_5g[i].dl_start_narfcn &&
                narfcn_dl < bandFreqRanges_5g[i].dl_start_narfcn + narfcn_count )
        {
            snprintf ( band ,sizeof( band ), " %d ",bandFreqRanges_5g[i].band );
            if( NULL != strstr ( suppband5g,band ) )
            {
                snprintf ( band_list + strlen ( band_list ), length - strlen ( band_list ), "%d:", bandFreqRanges_5g[i].band);
            }
        }
    }

    return 0;
}

void split_and_remove_duplicates(char *str_in, char *str_out, int length)
{
    char *tokens[100];  // 最多支持100个token
    int num_tokens = 0;
    char *token;
    int i;

    token = strtok(str_in, ":");  // 分割第一个token

    while (token != NULL) {
        // 检查当前token是否已经在之前的tokens中出现过
        for (i = 0; i < num_tokens; i++) {
            if (strcmp(token, tokens[i]) == 0) {
                break;
            }
        }
        // 如果当前token没有出现过，则加入tokens数组
        if (i == num_tokens) {
            tokens[num_tokens] = token;
            num_tokens++;
        }
        token = strtok(NULL, ":");  // 继续分割下一个token
    }

    // 输出去重后的字符串数组
    for (i = 0; i < num_tokens; i++) {
        snprintf ( str_out + strlen ( str_out ), length - strlen ( str_out ), "%s:", tokens[i]);
    }
}

unsigned long nitz_time_total_second (
    const unsigned int year0, const unsigned int mon0,
    const unsigned int day, const unsigned int hour,
    const unsigned int min, const unsigned int sec
    )
{
    unsigned int mon = mon0, year = year0;

    /* 1..12 -> 11,12,1..10 */
    if ( 0 >= ( int ) ( mon -= 2 ) )
    {
        mon += 12;  /* Puts Feb last since it has leap day */
        year -= 1;
    }

    return ( ( ( ( unsigned long )
                 ( year / 4 - year / 100 + year / 400 + 367 * mon / 12 + day )
                 + year * 365 - 719499
               ) * 24 + hour /* now have hours */
             ) * 60 + min /* now have minutes */
           ) * 60 + sec; /* finally seconds */
}

int calc_ipv6_addr_prefix ( char *full_v6addr, unsigned int mask, char* v6addr_prefix )
{
    if ( mask < 0 || 128 < mask || ( ! full_v6addr ) )
    {
        CLOGD ( ERROR, "Illegal parameter !!!\n" );
        return -1;
    }

    struct in6_addr v6addr = IN6ADDR_ANY_INIT;
    int ret = 0;
    int i = ( mask >> 3 );
    int j = ( mask & 0x7 );

    ret = inet_pton ( AF_INET6, full_v6addr, &v6addr );
    if ( 1 != ret )
    {
        CLOGD ( WARNING, "Invalid ipv6_addr -> [%s]\n", full_v6addr );
        return -1;
    }

    if ( 0 != j ) i++;

    memset ( v6addr.s6_addr + i, 0, sizeof ( v6addr.s6_addr ) - i );
    memcpy ( v6addr.s6_addr, &v6addr, i );

    if ( 0 != j ) v6addr.s6_addr[i - 1] &= ( 0xff00 >> j );

    return inet_ntop ( AF_INET6, &v6addr, v6addr_prefix, INET6_ADDRSTRLEN ) ? 0 : -1;
}

int calc_spec_char_counts ( char *str_org, char spec_char )
{
    char *local_s = str_org;
    int i = 0;
    char *tmp = NULL;

    while ( local_s )
    {
        tmp = strchr ( local_s, spec_char );
        if ( NULL == tmp )
            break;

        i = i + 1;
        local_s += ( tmp - local_s + 1 );
    }

    return i;
}

int comd_strrpl ( char *str_org, char *x, char *y )
{
    if ( NULL == str_org || NULL == x || NULL == y ||
         strlen ( x ) != strlen ( y ) || 0 == strcmp ( x, y ) )
    {
        return 0;
    }

    int rpl_num = 0;
    char *substr = NULL;

    while ( 1 )
    {
        substr = strstr ( str_org, x );
        if ( NULL == substr )
        {
            break;
        }
        rpl_num ++;
        memcpy ( substr, y, strlen ( y ) );
    }

    return rpl_num;
}

void calc_ipv4_gw_from_addr ( char* ipaddr, char* gateway, int gw_len )
{
    int ip_deli[4] = { 0, 0, 0, 0 };

    sscanf ( ipaddr, "%d.%d.%d.%d", &ip_deli[0], &ip_deli[1], &ip_deli[2], &ip_deli[3] );

    snprintf ( gateway, gw_len, "%d.%d.%d.%d", ip_deli[0], ip_deli[1], ip_deli[2],
                                ( 0 == ip_deli[3] || 255 == ip_deli[3] ) ? 1 : ( 255 - ip_deli[3] ) );
}

void calc_x55_mask_gw_from_ip ( char* ip_addr, char* net_mask, char* gate_way )
{
    struct in_addr addr;

    inet_aton ( ip_addr, &addr );
    uint32_t input = ntohl ( addr.s_addr );
    uint32_t pub_ip = ntohl ( addr.s_addr );

    in_addr_t netmask = 0xffffffff;
    int cnt = 1;
    int flag = ( input & 1 );

    if ( input <= 0 )
    {
        goto NEXT;
    }

    while ( flag == ( input & 1 ) )
    {
        input = ( input >> 1 );
        cnt++;
    }

NEXT:
    netmask = ( netmask << cnt );
    addr.s_addr = htonl ( netmask );
    snprintf ( net_mask, 32, "%s", inet_ntoa ( addr ) );

    if ( 0xffffffff == ( ( pub_ip + 1 ) | netmask ) )
    {
        addr.s_addr = htonl ( pub_ip - 1 );
    }
    else
    {
        addr.s_addr = htonl ( pub_ip + 1 );
    }
    snprintf ( gate_way, 32, "%s", inet_ntoa ( addr ) );
}

void delchar ( char *str, char c )
{
    int i = 0, j = 0;
    for ( ; str[i] != '\0'; i++ )
    {
        if ( str[i] != c )
        {
           str[j++] = str[i];
        }
    }
    str[j] = '\0';
}

unsigned long mix ( unsigned long a, unsigned long b, unsigned long c )
{
    a = a - b;
    a = a - c;
    a = a ^ ( c >> 13 );
    b = b - c;
    b = b - a;
    b = b ^ ( a << 8 );
    c = c - a;
    c = c - b;
    c = c ^ ( b >> 13 );
    a = a - b;
    a = a - c;
    a = a ^ ( c >> 12 );
    b = b - c;
    b = b - a;
    b = b ^ ( a << 16 );
    c = c - a;
    c = c - b;
    c = c ^ ( b >> 5 );
    a = a - b;
    a = a - c;
    a = a ^ ( c >> 3 );
    b = b - c;
    b = b - a;
    b = b ^ ( a << 10 );
    c = c - a;
    c = c - b;
    c = c ^ ( b >> 15 );
    return c;
}

int autoApnSetting( int apn_index,apn_profile* apn_settingdata)
{
    char mcc_str[4] = {0};
    char mnc_str[4] = {0};
    char apn_mode[4] = {0};
    char auto_file[64] = {0};
    char szCmd[128] = {0};
    char szVal[128] = {0};
    char auto_id[4] = {0};

    nv_get ( "usim_mcc", mcc_str, sizeof ( mcc_str ) );
    nv_get ( "usim_mnc", mnc_str, sizeof ( mnc_str ) );

    snprintf ( auto_file, sizeof ( auto_file ),
               "/etc/apns-conf/mcc_mnc/%s%s", mcc_str, mnc_str );

    if(!strlen ( mcc_str ) || !strlen( mnc_str))
    {
        CLOGD ( ERROR, "Can not get mcc or mnc [%s-%s]\n", mcc_str, mnc_str );
        return -1;
    }

    if ( 0 == access ( auto_file, F_OK ) )
    {
        snprintf ( auto_file, sizeof ( auto_file ),
                   "cp /etc/apns-conf/mcc_mnc/%s%s /tmp/auto_apn", mcc_str, mnc_str );
        system ( auto_file );
    }
    else
    {
        CLOGD ( WARNING, "Can not find auto apn file for [%s-%s]\n", mcc_str, mnc_str );
    }

    sys_get_config ( "lte_param.apn.mode", apn_mode, sizeof ( apn_mode ) );

    if ( strcmp ( apn_mode, "1" ) )
    {
        return -1;
    }

    if ( 0 ==  apn_index )
    {
        sys_get_config ( "lte_param.apn.curId", auto_id, sizeof ( auto_id ) );
    }
    else
    {
        snprintf ( auto_id, sizeof ( auto_id ), "%d", apn_index );
    }

    snprintf ( szCmd, sizeof ( szCmd ), "auto_apn.apn%s.profile_name", auto_id );
    memset ( szVal, 0, sizeof ( szVal ) );
    sys_get_config ( szCmd, szVal, sizeof ( szVal ) );
    CLOGD ( FINE, "%s -> [%s]\n", szCmd, szVal );
    strncpy ( apn_settingdata->profile_name, szVal, APN_LIST_PARAMETER_LEN );

    snprintf ( szCmd, sizeof ( szCmd ), "auto_apn.apn%s.apnname", auto_id );
    memset ( szVal, 0, sizeof ( szVal ) );
    sys_get_config ( szCmd, szVal, sizeof ( szVal ) );
    CLOGD ( FINE, "%s -> [%s]\n", szCmd, szVal );
    strncpy ( apn_settingdata->apn_name, szVal, APN_LIST_PARAMETER_LEN );

    snprintf ( szCmd, sizeof ( szCmd ), "auto_apn.apn%s.pdptype", auto_id );
    memset ( szVal, 0, sizeof ( szVal ) );
    sys_get_config ( szCmd, szVal, sizeof ( szVal ) );
    CLOGD ( FINE, "%s -> [%s]\n", szCmd, szVal );
    strncpy ( apn_settingdata->pdn_type,
              strcmp(szVal, "IP") ? (strcmp(szVal, "IPV6") ? "2" : "1") : "0",
              APN_LIST_PARAMETER_LEN );

    snprintf ( szCmd, sizeof ( szCmd ), "auto_apn.apn%s.auth_type", auto_id );
    memset ( szVal, 0, sizeof ( szVal ) );
    sys_get_config ( szCmd, szVal, sizeof ( szVal ) );
    CLOGD ( FINE, "%s -> [%s]\n", szCmd, szVal );
    strncpy ( apn_settingdata->auth_type,
              strcmp(szVal, "NULL") ? (strcmp(szVal, "PAP") ? "2" : "1") : "0",
              APN_LIST_PARAMETER_LEN );

    snprintf ( szCmd, sizeof ( szCmd ), "auto_apn.apn%s.username", auto_id );
    memset ( szVal, 0, sizeof ( szVal ) );
    sys_get_config ( szCmd, szVal, sizeof ( szVal ) );
    CLOGD ( FINE, "%s -> [%s]\n", szCmd, szVal );
    strncpy ( apn_settingdata->user_name, szVal, APN_LIST_PARAMETER_LEN );

    snprintf ( szCmd, sizeof ( szCmd ), "auto_apn.apn%s.password", auto_id );
    memset ( szVal, 0, sizeof ( szVal ) );
    sys_get_config ( szCmd, szVal, sizeof ( szVal ) );
    CLOGD ( FINE, "%s -> [%s]\n", szCmd, szVal );
    strncpy ( apn_settingdata->password, szVal, APN_LIST_PARAMETER_LEN );

    apn_settingdata->index = 0;
    apn_settingdata->mtu = 1500;
    strncpy ( apn_settingdata->apn_enable, "1", APN_LIST_PARAMETER_LEN );
    strncpy ( apn_settingdata->band_mac, "", APN_LIST_PARAMETER_LEN );
    strncpy ( apn_settingdata->vid, "0", APN_LIST_PARAMETER_LEN );

    nv_set("auto_apn_set","1");

    return 0;
}

int verify_5g_narfcn_by_band(char* band, char* narfcn)
{
    int ret = 0;
    int i = 0;
    int band_int = atoi(band);
    int narfcn_int = atoi(narfcn);
    int narfcn_count = 0;
    for ( ; i < ALL_5G_BAND_NUMS; i++ )
    {
        if(bandFreqRanges_5g[i].band == band_int)
        {
            narfcn_count = bandFreqRanges_5g[i].band_width / bandFreqRanges_5g[i].raster_ref;
            if ( narfcn_int >= bandFreqRanges_5g[i].dl_start_narfcn &&
                narfcn_int < bandFreqRanges_5g[i].dl_start_narfcn + narfcn_count )
            {
                ret = 1;
            }
        }
    }

    return ret;
}

void decodeGsm7bitPdu ( char *szDes, const char *pbSrc, int nLen )
{
    unsigned char szSrc[512] = {0};
    int iSrcLen = ( strlen ( pbSrc ) / 2 );
    int i = 0;
    int tmp[2] = {0};
    int pos = 0;
    int left = 7;
    int high_last = 0;
    unsigned char tmp_c = 0;

    for ( i = 0; i < iSrcLen && pos < nLen; i++ )
    {
        tmp[0] = lib_HexChar2Dec ( pbSrc[2*i] );
        if ( tmp[0] < 0 )
            continue;

        tmp[1] = lib_HexChar2Dec ( pbSrc[2*i+1] );
        if ( tmp[1] < 0 )
            continue;

        szSrc[i] = ( tmp[0] * 16 +  tmp[1] );

        left = ( 7 - ( i % 7 ) );

        if ( i > 0 )
            high_last = ( szSrc[i - 1] >> ( left + 1 ) );

        tmp_c = ( szSrc[i] << ( 8 - left ) );
        tmp_c = ( tmp_c >> 1 );

        szDes[pos++] = tmp_c + high_last;
        if ( 6 == ( i % 7 ) )
        {
            szDes[pos++] = ( szSrc[i] >> left );
        }
    }
}

static int enc_unicode_to_utf8_one ( unsigned long unic, char *pOutput )
{
    if ( unic <= 0x0000007F )
    {
        // * U-00000000 - U-0000007F:  0xxxxxxx
        *pOutput     = (unic & 0x7F);
        return 1;
    }
    else if ( unic >= 0x00000080 && unic <= 0x000007FF )
    {
        // * U-00000080 - U-000007FF:  110xxxxx 10xxxxxx
        *(pOutput+1) = (unic & 0x3F) | 0x80;
        *pOutput     = ((unic >> 6) & 0x1F) | 0xC0;
        return 2;
    }
    else if ( unic >= 0x00000800 && unic <= 0x0000FFFF )
    {
        // * U-00000800 - U-0000FFFF:  1110xxxx 10xxxxxx 10xxxxxx
        *(pOutput+2) = (unic & 0x3F) | 0x80;
        *(pOutput+1) = ((unic >>  6) & 0x3F) | 0x80;
        *pOutput     = ((unic >> 12) & 0x0F) | 0xE0;
        return 3;
    }
#if 0
    else if ( unic >= 0x00010000 && unic <= 0x001FFFFF )
    {
        // * U-00010000 - U-001FFFFF:  11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
        *(pOutput+3) = (unic & 0x3F) | 0x80;
        *(pOutput+2) = ((unic >>  6) & 0x3F) | 0x80;
        *(pOutput+1) = ((unic >> 12) & 0x3F) | 0x80;
        *pOutput     = ((unic >> 18) & 0x07) | 0xF0;
        return 4;
    }
    else if ( unic >= 0x00200000 && unic <= 0x03FFFFFF )
    {
        // * U-00200000 - U-03FFFFFF:  111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
        *(pOutput+4) = (unic & 0x3F) | 0x80;
        *(pOutput+3) = ((unic >>  6) & 0x3F) | 0x80;
        *(pOutput+2) = ((unic >> 12) & 0x3F) | 0x80;
        *(pOutput+1) = ((unic >> 18) & 0x3F) | 0x80;
        *pOutput     = ((unic >> 24) & 0x03) | 0xF8;
        return 5;
    }
    else if ( unic >= 0x04000000 && unic <= 0x7FFFFFFF )
    {
        // * U-04000000 - U-7FFFFFFF:  1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
        *(pOutput+5) = (unic & 0x3F) | 0x80;
        *(pOutput+4) = ((unic >>  6) & 0x3F) | 0x80;
        *(pOutput+3) = ((unic >> 12) & 0x3F) | 0x80;
        *(pOutput+2) = ((unic >> 18) & 0x3F) | 0x80;
        *(pOutput+1) = ((unic >> 24) & 0x3F) | 0x80;
        *pOutput     = ((unic >> 30) & 0x01) | 0xFC;
        return 6;
    }
#endif

    return 0;
}

int unicode2chars ( char* my_uni, char* my_str, int str_len )
{
    char out_char[4] = {0};
    int i = 0;
    int uni_hex = 0;
    unsigned long uni_hex_sum = 0;

    CLOGD ( FINE, "[%s]\n", my_uni );

    for ( ; i < strlen ( my_uni ); i++ )
    {
        if ( my_uni[i] >= '0' && my_uni[i] <= '9' )
        {
            uni_hex = my_uni[i] - '0';
        }
        else if ( my_uni[i] >= 'a' && my_uni[i] <= 'f' )
        {
            uni_hex = my_uni[i] - 'a' + 10;
        }
        else if ( my_uni[i] >= 'A' && my_uni[i] <= 'F' )
        {
            uni_hex = my_uni[i] - 'A' + 10;
        }
        else
        {
            return 1;
        }

        uni_hex_sum = uni_hex_sum * 16 + uni_hex;

        if ( 0 == ( ( i + 1 ) % 4 ) )
        {
            memset ( out_char, 0, sizeof ( out_char ) );
            enc_unicode_to_utf8_one ( uni_hex_sum, out_char );
            uni_hex_sum = 0;
            snprintf ( my_str + strlen ( my_str ), str_len - strlen ( my_str ), "%s", out_char);
        }
    }

    CLOGD ( FINE, "[%s]\n", my_str );

    return 0;
}

/* Pad a char between symbol.
*  Input str: aa,bb,cc,,dd,,ee,,ff,
*  Output str: aa,bb,cc,0,dd,0,ee,0,ff,
*/

void padding_char_between_two_same_symbol(char* pad_char, char* symbol, char* input_str, char* output_str)
{
    int i = 0;
    int j = 0;

    for(i = 0;i < strlen(input_str); i++)
    {
        if(input_str[i] == symbol && (i+1 < strlen(input_str)) && input_str[i+1] == symbol)
        {
            output_str[j] = input_str[i];
            output_str[++j] = pad_char;
            j++;
        }
        else
        {
            output_str[j] = input_str[i];
            j++;
        }
    }
}
