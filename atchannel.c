#include "comd_share.h"
#include "atchannel.h"

#if defined(MTK_T750) && CONFIG_QUECTEL_T750 && USE_QUECTEL_API_SEND_AT
#include "quectel_atcid_sender.h"
extern COMM_DEV *pcomm_dev;
#endif

#if defined(_MTK_7621_) || defined(_MTK_7628_)
#define COMD_SOCKET_PORT 7788
#else
#define COMD_SOCKET_PORT 8877
#endif

#define NUM_ELEMS(x) (sizeof(x)/sizeof(x[0]))

extern COMD_DEV_INFO at_dev;
extern int atProcess_sem_id;
extern int atchannel_ready;

struct termios oldtio;
struct sockaddr_in their_addr, client_addr;

int speed_arr[10] = { B230400, B115200, B57600, B38400, B19200, B9600, B4800, B2400, B1200, B300 };
int baud_arr[10] = {  230400,  115200,  57600,  38400,  19200,  9600,  4800,  2400,  1200,  300  };

/**
 * returns 1 if line is a final response indicating error
 * See 27.007 annex B
 * WARNING: NO CARRIER and others are sometimes unsolicited
 */
static const char * s_finalResponsesError[] =
{
    "ERROR\n",
    "+CME ERROR:",
    "\r\nERROR\r\n",
    "\r\n+CMS ERROR:",
    "\r\n+CME ERROR:",
    "\r\nNO CARRIER\r\n", /* sometimes! */
    "\r\nNO ANSWER\r\n",
    "\r\nNO DIALTONE\r\n"
};

static int isFinalResponseError ( const char *line )
{
    int i = 0;

    for ( i = 0; i < NUM_ELEMS ( s_finalResponsesError ); i++ )
    {
        if ( strstr ( line, s_finalResponsesError[i] ) )
        {
            return 1;
        }
    }

    return 0;
}

/**
 * returns 1 if line is a final response indicating success
 * See 27.007 annex B
 * WARNING: NO CARRIER and others are sometimes unsolicited
 */
static const char * s_finalResponsesSuccess[] =
{
    "OK\n",
    "\r\nOK\r\n",
    "\r\nCONNECT\r\n"   /* some stacks start up data on another channel */
};

static int isFinalResponseSuccess ( const char *line )
{
    int i = 0;;

    for ( i = 0; i < NUM_ELEMS ( s_finalResponsesSuccess ); i++ )
    {
        if ( strstr ( line, s_finalResponsesSuccess[i] ) )
        {
            return 1;
        }
    }

    return 0;
}

/**
 * returns 1 if line is a final response, either  error or success
 * See 27.007 annex B
 * WARNING: NO CARRIER and others are sometimes unsolicited
 */
static int isFinalResponse ( const char *line )
{
    return isFinalResponseSuccess ( line ) || isFinalResponseError ( line );
}

/**
 * returns 1 if line is the first line in (what will be) a two-line
 * SMS unsolicited response
 */
static const char * s_smsUnsoliciteds[] =
{
    "\r\n+CMT:",
    "\r\n+CDS:",
    "\r\n+CBM:"
};

static int isSMSUnsolicited ( const char *line )
{
    int i = 0;

    for ( i = 0; i < NUM_ELEMS ( s_smsUnsoliciteds ); i++ )
    {
        if ( strstr ( line, s_smsUnsoliciteds[i] ) )
        {
            return 1;
        }
    }

    return 0;
}

int create_at_socket_channel()
{
    int ret = -1;

    switch ( at_dev.comd_ch_type )
    {
    case 0:     // UDP
        ret = ( at_dev.dev_fd = socket ( AF_INET, SOCK_DGRAM, IPPROTO_UDP ) );
        break;
    case 1:     // TCP
        ret = ( at_dev.dev_fd = socket ( AF_INET, SOCK_STREAM, IPPROTO_IP ) );
        break;
    default:
        CLOGD ( ERROR, "unknown socket type [%d] !!!\n", at_dev.comd_ch_type );
        return -1;
    }

    if ( -1 == ret )
    {
        perror ( "socket" );
        CLOGD ( ERROR, "socket failed !!!\n" );
        return -1;
    }

    bzero ( &their_addr, sizeof ( struct sockaddr_in ) );
    their_addr.sin_family = AF_INET;
    their_addr.sin_addr.s_addr = inet_addr ( at_dev.comd_ip_addr );
    their_addr.sin_port = htons ( at_dev.comd_ip_port );

    bzero ( &client_addr, sizeof ( struct sockaddr_in ) );
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = htonl ( INADDR_ANY );
    client_addr.sin_port = htons ( COMD_SOCKET_PORT );

    unsigned char service_type = 0xe0 | IPTOS_LOWDELAY | IPTOS_RELIABILITY;

    if ( 0 > setsockopt ( at_dev.dev_fd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof ( int ) ) )
    {
        perror ( "setsockopt SO_REUSEADDR" );
        CLOGD ( ERROR, "setsockopt SO_REUSEADDR failed !!!\n" );
    }
    else if ( -1 == bind ( at_dev.dev_fd, ( struct sockaddr * ) &client_addr, sizeof ( client_addr ) ) )
    {
        perror ( "bind" );
        CLOGD ( ERROR, "socket bind failed !!!\n" );
    }
    else
    {
        switch ( at_dev.comd_ch_type )
        {
        case 0:     // UDP
            return 0;
        case 1:     // TCP
            if ( 0 > setsockopt ( at_dev.dev_fd, IPPROTO_IP, IP_TOS, ( void * ) &service_type, sizeof ( service_type ) ) )
            {
                perror ( "setsockopt IP_TOS" );
                CLOGD ( ERROR, "setsockopt IP_TOS failed !!!\n" );
            }
            else if ( -1 == connect ( at_dev.dev_fd, ( struct sockaddr * ) &their_addr, sizeof ( their_addr ) ) )
            {
                perror ( "connect" );
                CLOGD ( ERROR, "socket connect failed !!!\n" );
            }
            else
            {
                return 0;
            }
            break;
        }
    }

    close ( at_dev.dev_fd );
    return -1;
}

int create_at_serial_channel()
{
    if ( access ( at_dev.comd_ser_name, F_OK ) )
    {
        CLOGD ( ERROR, "can not access [%s] ...\n", at_dev.comd_ser_name );
        return -1;
    }

    at_dev.dev_fd = open ( at_dev.comd_ser_name, O_RDWR | O_NOCTTY );
    if ( at_dev.dev_fd < 0 )
    {
        CLOGD ( ERROR, "open %s failed ...\n", at_dev.comd_ser_name );
        return -1;
    }

    tcgetattr ( at_dev.dev_fd, &oldtio );

    struct termios newtio;
    memset ( &newtio, 0, sizeof ( newtio ) );

    /*
     * Control Flag
     */
    newtio.c_cflag = at_dev.comd_ser_rate | CS8 | CLOCAL | CREAD;

    /*
     * Input Flag
     *      IGNPAR : Ignore parity
     *      ICRNL : replace CR to NL - need this only to test manual typing from a pc terminal
     */
    newtio.c_iflag = IGNPAR;

    /*
     * Output Flag
     */
    newtio.c_oflag = 0;

    /*
     * Local Flag
     *      ECHO : echo all
     *      ECHONL : echo NL (useful for half-duplex)
     *      ICANON: Canonical mode
     */
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0;    /* inter-character timer unused */
    newtio.c_cc[VMIN] = 1;     /* blocking read until 1 character arrives */

    tcflush ( at_dev.dev_fd, TCIOFLUSH );

#if defined(TD_MT5710)
    CLOGD ( WARNING, "TD_MT5710 [%s] -> no need tcsetattr !\n", at_dev.comd_ser_name );
    return 0;
#endif

#if defined(_QCA_X55_)
    if ( 0 == strcmp ( CONFIG_HW_PLATFORM_TYPE, "RG500" ) )
    {
        CLOGD ( WARNING, "RG500 [%s] -> no need tcsetattr !\n", at_dev.comd_ser_name );
        return 0;
    }
#endif

    if ( tcsetattr ( at_dev.dev_fd, TCSANOW, &newtio ) )
    {
        CLOGD ( ERROR, "tcsetattr failed !!!\n" );
        close ( at_dev.dev_fd );
        return -1;
    }

    return 0;
}

static void saveUssdReportContent ( char* data )
{
    char *ussd_str = NULL;
    int ret = 0;
    char ussd_nval[8] = {0};

    ret = sscanf ( data, "\r\n+CUSD: %[0-9]", ussd_nval );

    if ( ret <= 0 )
    {
        goto END;
    }

    CLOGD ( FINE, "USSD (n): [%s]\n", ussd_nval );

    nv_set ( "ussd_reply_type", ussd_nval );

    /* USSD (n) value:
     * 0: messages that do not require user to continue to reply
     * 1: messages that require user to continue to reply
     * 2: network side actively ends the USSD call
     * 3: other local customers have been responded
     * 4: operation not supported
     * 5: network side timeout
     */

    if ( 0 == strcmp ( ussd_nval, "2" ) || 0 == strcmp ( ussd_nval, "3" ) ||
         0 == strcmp ( ussd_nval, "4" ) || 0 == strcmp ( ussd_nval, "5" ) )
    {
        /*
         * In fact, we can set "ussd_code_set" to "0" here.
         * but if "0" value is setted,
         * web should display different Pop-up information
         * based on "ussd_reply_type"(2/3/4/5).
         * currently we have no plan to do this, so here is set to "1" by default
         */
        nv_set ( "ussd_code_set", "1" );
        return;
    }
    else if ( strcmp ( ussd_nval, "0" ) && strcmp ( ussd_nval, "1" ) )
    {
        goto END;
    }

    ussd_str = strtok ( data, "\"" );

    if ( NULL != ussd_str )
    {
        ussd_str = strtok ( NULL, "\"" );

        if ( NULL != ussd_str )
        {
            FILE *fp = NULL;
            char ussdContent[RECV_BUF_SIZE * 2] = {0};

            HexToStr ( ussdContent, ussd_str, sizeof ( ussdContent ) );

            fp = fopen ( "/tmp/ussd_code.txt", "w+" );
            if ( NULL != fp )
            {
                fprintf ( fp, "%s", ussdContent );
                fclose ( fp );
                fp = NULL;
                nv_set ( "ussd_code_set", "0" );
                return ;
            }
            else
            {
                CLOGD ( WARNING, "open /tmp/ussd_code.txt failed !\n" );
            }
        }
        else
        {
            CLOGD ( WARNING, "Step 2: invalid USSD content\n" );
        }
    }
    else
    {
        CLOGD ( WARNING, "Step 1: invalid USSD content\n" );
    }

END:
    nv_set ( "ussd_code_set", "1" );
}

static void clean_at_msg_cache( int comd_type, char *send_at  )
{
    char buf_recv[RECV_BUF_SIZE] = {0};
    int sel_max_times = 30;
    struct timeval tv;
    fd_set rfd;
    unsigned int their_len = sizeof ( their_addr );

    if ( 0 == comd_type )
        their_addr.sin_port = htons ( at_dev.comd_ip_port );

    while ( sel_max_times-- > 0 )
    {
        FD_ZERO ( &rfd );
        FD_SET ( at_dev.dev_fd, &rfd );
        tv.tv_sec = 0;
        tv.tv_usec = 50000;
        if ( select ( at_dev.dev_fd + 1, &rfd, NULL, NULL, &tv ) <= 0 )
            break;

        memset ( buf_recv, 0, RECV_BUF_SIZE );
        if ( comd_type == 0 )
            recvfrom ( at_dev.dev_fd, buf_recv, RECV_BUF_SIZE - 1, 0,
                        ( struct sockaddr * ) &their_addr, &their_len );
        else
            read ( at_dev.dev_fd, buf_recv, RECV_BUF_SIZE - 1 );

        CLOGD ( DETAILS, "[%s] OLD_CACHE: %s\n", send_at, buf_recv );

        if ( strstr ( buf_recv, "\r\n+CUSD:" ) )
        {
            saveUssdReportContent ( strstr ( buf_recv, "\r\n+CUSD:" ) );
        }

#if defined(_MTK_7621_) || defined(_MTK_7628_)
        if ( strstr ( buf_recv, "+CGEV: ME DETACH" ) )
            apnActOrDeactMsgEvent ( 0, 0 );
#endif
    }
}

int COMD_AT_PROCESS ( char *send_buf, int outime, char* recv_buf, int recv_len )
{
    comd_semaphore_p ( atProcess_sem_id );

    int result = -1;
    int CMGS_SEND_SMS_CMD = 0;
    char buffer[SEND_MAX_SIZE] = {0};
    char *sms_len = NULL;
    char *sms_data = NULL;

#if defined(CONFIG_SW_QUECTEL_X62)
    if(at_dev.comd_mVendor == M_VENDOR_QUECTEL_X55)
    {
        memcpy ( buffer + 3, send_buf, strlen ( send_buf ) );
        memcpy ( buffer + 3 + strlen ( send_buf ), "\r\n", 2 );
        CLOGD ( FINE, "SEND_BUF: %s\n", send_buf );

        buffer[0] = 0xa4;
        buffer[1] = (uint8_t)((strlen(buffer+3) & (0xff00))>>8);
        buffer[2] = (uint8_t)(strlen(buffer+3) & (0x00ff));
    }
    else
    {
        /* when send sms ,if add "/r",GCT bypass device  will not send message.*/
        if( 0 == strncmp(send_buf, "AT+GCMGS=", 9) )
        {
            snprintf ( buffer, sizeof ( buffer ), "%s", send_buf );
        }
        else if( 0 == strncmp(send_buf, "AT+CMGS=", 8) && strstr ( send_buf, ";" ))
        {
            sms_len = strtok ( send_buf, ";" );
            if ( NULL == sms_len )
            {
                CLOGD ( WARNING, "NULL == sms_len ...\n" );
                goto END;
            }

            sms_data = strtok ( NULL, ";" );
            if ( NULL == sms_data )
            {
                CLOGD ( WARNING, "NULL == sms_data ...\n" );
                goto END;
            }
SEND_SMS_PDU_STEP:
            if ( 0 == CMGS_SEND_SMS_CMD )
            {
                CMGS_SEND_SMS_CMD = 1;
                // STEP[0]: AT+CMGS=PDU_len
                snprintf ( buffer, sizeof ( buffer ), "%s\r", sms_len );
            }
            else
            {
                CMGS_SEND_SMS_CMD = 0;
                // STEP[1]: PDU & ctrl-z
                snprintf ( buffer, sizeof ( buffer ), "%s%c", sms_data, 0x1A );
            }
        }
        else
        {
            snprintf ( buffer, sizeof ( buffer ), "%s\r", send_buf );
            CLOGD ( FINE, "SEND_BUF: %s\n", buffer );
        }
    }

#else
    if( 0 == strncmp(send_buf, "AT+CMGS=", 8) && strstr ( send_buf, ";" ))
    {
        sms_len = strtok ( send_buf, ";" );
        if ( NULL == sms_len )
        {
            CLOGD ( WARNING, "NULL == sms_len ...\n" );
            goto END;
        }

        sms_data = strtok ( NULL, ";" );
        if ( NULL == sms_data )
        {
            CLOGD ( WARNING, "NULL == sms_data ...\n" );
            goto END;
        }
SEND_SMS_PDU_STEP:
        if ( 0 == CMGS_SEND_SMS_CMD )
        {
            CMGS_SEND_SMS_CMD = 1;
            // STEP[0]: AT+CMGS=PDU_len
            snprintf ( buffer, sizeof ( buffer ), "%s\r", sms_len );
        }
        else
        {
            CMGS_SEND_SMS_CMD = 0;
            // STEP[1]: PDU & ctrl-z
            snprintf ( buffer, sizeof ( buffer ), "%s%c", sms_data, 0x1A );
        }
    }
    else
    {
        snprintf ( buffer, sizeof ( buffer ), "%s\r", send_buf );
        CLOGD ( FINE, "SEND_BUF: %s\n", buffer );
    }

#endif

    if ( -1 == atchannel_ready )
    {
        CLOGD ( WARNING, "[%s] atchannel_ready is [-1] !!!\n", send_buf );
        goto END;
    }

#if defined(MTK_T750) && CONFIG_QUECTEL_T750 && USE_QUECTEL_API_SEND_AT
    atcmd_sender_msg_t at_msg;

#if (QUECTEL_API_SEND_AT_MODE == 0)
    pcomm_dev = quectel_atcid_sender_init ( TYPE_ADB_ATCI_SOCKET );
#endif

    if ( NULL != pcomm_dev )
    {
        memset ( &at_msg, 0, sizeof ( atcmd_sender_msg_t ) );
        strncpy ( at_msg.at_cmd, buffer, ATCMD_MAX_LEN - 2 );
        strncat ( at_msg.at_cmd, "\n", strlen ( "\n" ) );
        at_msg.timeout = ( outime / 1000 ) + ( ( outime % 1000 ) ? 1 : 0 );

        result = quectel_atcid_sender_sending ( pcomm_dev, &at_msg );
        CLOGD ( FINE, "timeout: [%d]s, result: [%d]\n", at_msg.timeout, result );

        if ( 0 < result )
        {
            snprintf ( recv_buf, recv_len, "%s", at_msg.at_recv );
            if ( strlen ( recv_buf ) < 1920 )   // LOG_MSG_MAX_BUFF is 2048 in log.h
                CLOGD ( FINE, "[%s] RECV_BUF: %s\n", send_buf, recv_buf );
            else
                CLOGD ( WARNING, "[%s] RECV_BUF: %d >= 1920, not print !!!\n", send_buf, strlen ( recv_buf ) );
        }

#if (QUECTEL_API_SEND_AT_MODE == 0)
        quectel_atcid_sender_deinit ( pcomm_dev );
        pcomm_dev = NULL;
#endif
    }
    else
    {
        CLOGD ( ERROR, "pcomm_dev is NULL !!!\n" );
    }
#else
    int send_ret = -1;

    switch ( at_dev.comd_ch_type )
    {
    case 0:     // socket UDP
        clean_at_msg_cache( at_dev.comd_ch_type, send_buf );
        their_addr.sin_port = htons ( at_dev.comd_ip_port );
        send_ret = sendto ( at_dev.dev_fd, buffer, strlen ( buffer ), 0,
                            ( struct sockaddr * ) &their_addr, sizeof ( struct sockaddr_in ) );
        break;
    default :
        clean_at_msg_cache( at_dev.comd_ch_type, send_buf );
#if defined(CONFIG_SW_QUECTEL_X62)
        if(at_dev.comd_mVendor == M_VENDOR_QUECTEL_X55)
        {
            send_ret = write ( at_dev.dev_fd, buffer, ( 3 + ( int ) strlen ( buffer + 3 ) ) );
        }
        else
        {
            send_ret = write ( at_dev.dev_fd, buffer, strlen ( buffer ) );
        }
#else
        send_ret = write ( at_dev.dev_fd, buffer, strlen ( buffer ) );
#endif
        break;
    }

    if ( 0 > send_ret )
    {
        CLOGD ( ERROR, "Send [%s] failed ...\n", send_buf );
        goto END;
    }

    if ( 1 == CMGS_SEND_SMS_CMD )
    {
        comd_wait_ms ( 100 );
        goto SEND_SMS_PDU_STEP;
    }

    if ( 0 >= outime )
    {
        result = 1;
        CLOGD ( WARNING, "Send [%s] without recv return msg!\n", send_buf );
        goto END;
    }

    fd_set readfds;
    int ret = 0;
    struct timeval timeout;
    char buf_recv[RECV_BUF_SIZE] = {0};
    unsigned int their_len = sizeof ( their_addr );

    *recv_buf = 0;

    while ( 1 )
    {
        FD_ZERO ( &readfds );
        FD_SET ( at_dev.dev_fd, &readfds );
        timeout.tv_sec = outime / 1000;
        timeout.tv_usec = ( outime % 1000 ) * 1000;

        if ( 0 < select ( at_dev.dev_fd + 1, &readfds, NULL, NULL, &timeout ) )
        {
            memset ( buf_recv, 0, sizeof ( buf_recv ) );

            switch ( at_dev.comd_ch_type )
            {
            case 0:     // socket UDP
                ret = recvfrom ( at_dev.dev_fd, buf_recv, sizeof ( buf_recv ) - 1, 0,
                                ( struct sockaddr * ) &their_addr, &their_len );
                break;
            default :
                ret = read ( at_dev.dev_fd, buf_recv, sizeof ( buf_recv ) - 1 );
                break;
            }

            if ( ret <= 0 )
            {
                CLOGD ( ERROR, "[%s] read failed ...\n", send_buf );
                goto END;
            }

#if defined(CONFIG_SW_QUECTEL_X62)
            if(at_dev.comd_mVendor == M_VENDOR_QUECTEL_X55)
            {
                CLOGD ( DETAILS, "[%s] recv [%d]: %s\n", send_buf, strlen ( buf_recv + 3 ), buf_recv + 3 );
                snprintf ( recv_buf + strlen ( recv_buf ), recv_len - strlen ( recv_buf ), "%s", buf_recv + 3 );
            }
            else
            {
                CLOGD ( DETAILS, "[%s] recv [%d]: %s\n", send_buf, strlen ( buf_recv ), buf_recv );
                snprintf ( recv_buf + strlen ( recv_buf ), recv_len - strlen ( recv_buf ), "%s", buf_recv );
            }
#else
            CLOGD ( DETAILS, "[%s] recv [%d]: %s\n", send_buf, strlen ( buf_recv ), buf_recv );
            snprintf ( recv_buf + strlen ( recv_buf ), recv_len - strlen ( recv_buf ), "%s", buf_recv );
#endif

            if ( isFinalResponse ( recv_buf ) )
            {
                break;
            }

            if ( strlen ( recv_buf ) >= ( recv_len - 1 ) )
            {
                CLOGD ( ERROR, "[%s] recv_len:[%d] is not enough !\n", send_buf, recv_len );
                goto END;
            }
        }
        else
        {
            CLOGD ( ERROR, "[%s] select failed ...\n", send_buf );
            goto END;
        }
    }

    result = 1;

    if ( strlen ( recv_buf ) < 1920 )   // LOG_MSG_MAX_BUFF is 2048 in log.h
    {
        CLOGD ( FINE, "[%s] RECV_BUF: %s", send_buf, recv_buf );
    }
    else
    {
        CLOGD ( WARNING, "[%s] RECV_BUF: %d >= 1920, not print !!!\n", send_buf, strlen ( recv_buf ) );
    }

#if defined(_MTK_7621_) || defined(_MTK_7628_)
    if ( strstr ( recv_buf, "+CGEV: ME DETACH" ) )
        apnActOrDeactMsgEvent ( 0, 0 );
#endif
#endif

END:
    comd_semaphore_v ( atProcess_sem_id );

    return ( 0 < result ) ? 0 : -1;
}

int at_handshake()
{
    char at_rep[RECV_BUF_SIZE] = {0};
    int check_atchannel = 0;

    while ( COMD_AT_PROCESS ( "AT", 3000, at_rep, sizeof ( at_rep ) ) || NULL == strstr ( at_rep, "OK" ) )
    {
        if ( 0 == atchannel_ready && check_atchannel++ < AT_TIMEOUT_TIMER )
        {
            comd_wait_ms ( 1000 );
            memset ( at_rep, 0, sizeof ( at_rep ) );
        }
        else
        {
            atchannel_ready = -1; // SIGPIPE may set -1
            return -1;
        }
    }

    return 0;
}

