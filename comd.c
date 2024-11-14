#include "comd_share.h"
#include "atchannel.h"
#include "comd_ip_helper.h"
#include "comd_messages.h"
#include "gct_status_refresh.h"
#include "mtk_status_refresh.h"
#include "tdtek_status_refresh.h"
#include "quectel_status_refresh.h"
#include "unisoc_status_refresh.h"
#include "quectel_lte_status_refresh.h"
#include "telit_status_refresh.h"
#include "telit_pls_status_refresh.h"

#include "config_key.h"
#include "hal_platform.h"

int module_id = 0;
int manual_conn = 0;

char led_mode[4] = {0};
char dualsim_en[4] = {0};
char esim_supp[4] = {0};
char unlock_code[UNLOCK_CODE_NUM][UNLOCK_CODE_LEN + 1];
char pinlock_enable[4] = {0};
char simlock_enable[4] = {0};
char celllock_enable[4] = {0};
char volte_enable[8] = {0};

int atchannel_ready = -1; // 0: ready, -1: not ready
COMD_DEV_INFO at_dev;
pthread_t s_tid_status;
pthread_t s_tid_config;

extern int atProcess_sem_id;
extern int allSms_sem_id;
extern int speed_arr[10];
extern int baud_arr[10];

#if defined(MTK_T750) && CONFIG_QUECTEL_T750
#if USE_QUECTEL_API_SEND_AT
#include "quectel_atcid_sender.h"
COMM_DEV *pcomm_dev = NULL;
#endif
int datacall_init_ret = -1;
#endif

static int m_status_running = 0;
static void pthread_module_status()
{
    char install_mode[8] = {0};
    char refresh_flag[8] = {0};
    char ap_idle_flag[8] = {0};

    prctl ( PR_SET_NAME, "comd_status_thrd" );
    m_status_running = 1;
    while ( 0 == atchannel_ready )
    {
        nv_get ( "ap_idle_flag", ap_idle_flag, sizeof ( ap_idle_flag ) );
        if ( 0 == strcmp ( ap_idle_flag, "1" ) )
        {
            comd_wait_ms ( 3000 );
            continue;
        }

        nv_get ( "m_status_refresh", refresh_flag, sizeof ( refresh_flag ) );
        if ( 0 == strcmp ( refresh_flag, "stop" ) )
        {
            comd_wait_ms ( 3000 );
            continue;
        }

        CLOGD ( FINE, "comd_status_thrd is running\n" );

        nv_get ( "is_project_page", install_mode, sizeof ( install_mode ) );
        CLOGD ( FINE, "is_project_page -> [%s]\n", install_mode );

        switch ( at_dev.comd_mVendor )
        {
        case M_VENDOR_GCT:
            if ( 0 == strcmp ( install_mode, "yes" ) )
            {
                gct_install_mode_refresh();
            }
            else
            {
                gct_normal_status_refresh();
            }
            break;
        case M_VENDOR_SQNS:
            break;
        case M_VENDOR_FIBOCOM_X55:
            break;
        case M_VENDOR_QUECTEL_X55:
            if ( 0 == strcmp ( install_mode, "yes" ) )
            {
                CLOGD ( WARNING, "not support install mode !\n" );
            }
            else
            {
                quectel_normal_status_refresh();
            }
            break;
        case M_VENDOR_QUECTEL_T750:
            if ( 0 == strcmp ( install_mode, "yes" ) )
            {
                mtk_install_mode_refresh();
            }
            else
            {
                mtk_normal_status_refresh();
            }
            break;
        case M_VENDOR_ZTE_X55:
            break;
        case M_VENDOR_TD_MT5710:
            if ( 0 == strcmp ( install_mode, "yes" ) )
            {
                tdtek_install_mode_refresh();
            }
            else
            {
                tdtek_normal_status_refresh();
            }
            break;
        case M_VENDOR_QUECTEL_UNISOC:
            if ( 0 == strcmp ( install_mode, "yes" ) )
            {
                unisoc_install_mode_refresh();
            }
            else
            {
                unisoc_normal_status_refresh();
            }
            break;
        case M_VENDOR_QUECTEL_LTE:
            if ( 0 == strcmp ( install_mode, "yes" ) )
            {
                quectel_lte_install_mode_refresh();
            }
            else
            {
                quectel_lte_normal_status_refresh();
            }
            break;
        case M_VENDOR_TELIT_4G:
        case M_VENDOR_TELIT_5G:
            if ( 0 == strcmp ( install_mode, "yes" ) )
            {
            }
            else
            {
                telit_normal_status_refresh();
            }
            break;
        case M_VENDOR_TELIT_PLS:
            if ( 0 == strcmp ( install_mode, "yes" ) )
            {
            }
            else
            {
                telit_pls_normal_status_refresh();
            }
            break;
        default:
            break;
        }

        comd_wait_ms ( 1000 );
        at_handshake();
    }
    m_status_running = 0;
    CLOGD ( WARNING, "comd_status_thrd exit !!!\n" );
}

static int m_config_running = 0;
static void pthread_module_config()
{
    prctl ( PR_SET_NAME, "comd_config_thrd" );
    m_config_running = 1;
    while ( 0 == atchannel_ready )
    {
        comd_wait_ms ( 1000 );
    }
    m_config_running = 0;
    CLOGD ( WARNING, "comd_config_thrd exit !!!\n" );
}

static int module_param_init()
{
    CLOGD ( FINE, "module_param_init ...\n" );

    int ret = -2;

    //check if AT return OK
    if ( 0 == at_handshake() )
    {
        switch ( at_dev.comd_mVendor )
        {
        case M_VENDOR_GCT:
            ret = gct_module_param_init();
            break;
        case M_VENDOR_SQNS:
            ret = 0;
            break;
        case M_VENDOR_FIBOCOM_X55:
            break;
        case M_VENDOR_QUECTEL_X55:
            ret = quectel_module_param_init();
            break;
        case M_VENDOR_QUECTEL_T750:
            ret = mtk_module_param_init();
            break;
        case M_VENDOR_ZTE_X55:
            break;
        case M_VENDOR_TD_MT5710:
            ret = tdtek_module_param_init();
            break;
        case M_VENDOR_QUECTEL_UNISOC:
            ret = unisoc_module_param_init();
            break;
        case M_VENDOR_QUECTEL_LTE:
            ret = quectel_lte_module_param_init();
            break;
        case M_VENDOR_TELIT_4G:
        case M_VENDOR_TELIT_5G:
            ret = telit_module_param_init();
            break;
        case M_VENDOR_TELIT_PLS:
            ret = telit_pls_module_param_init();
            break;
        default :
            ret = 0;
            break;
        }
    }

    return ret;
}

static void waitForAtChannelClose()
{
    while ( 0 == atchannel_ready )
    {
        comd_wait_ms ( 1000 );
    }

    // wait pthread_status & pthread_config exit
    while ( m_status_running || m_config_running )
    {
        CLOGD ( WARNING, "pthread_status: [%d], pthread_config: [%d]\n",
                m_status_running, m_config_running );
        comd_wait_ms ( 1000 );
    }

    if ( 0 < at_dev.dev_fd )
    {
        close ( at_dev.dev_fd );
    }

#if defined(MTK_T750) && CONFIG_QUECTEL_T750 && USE_QUECTEL_API_SEND_AT
#if (QUECTEL_API_SEND_AT_MODE == 1)
    if ( pcomm_dev )
    {
        quectel_atcid_sender_deinit ( pcomm_dev );
        pcomm_dev = NULL;
    }
#endif
#endif
}

static int msgInit()
{
    return comdMessageInit();
}

static int enter_factory_test_mode ()
{
    char test_mode[4] = {0};

    sys_get_config ( LTE_PARAMETER_FACTORY_TEST_MODE, test_mode, sizeof ( test_mode ) );

    return strcmp ( test_mode, "1" ) ? 0 : 1;
}

static void create_vlan_for_GCT_module()
{
    int vlan_idx = 0;
    char wan_if[64] = {0};
    char cmd_str[128] = {0};
    char device_if[8] = {0};
    char board_type[16] = {0};

    sys_get_config ( LTE_PARAMETER_WAN_IF, wan_if, sizeof ( wan_if ) );
    strcpy ( device_if, strstr ( wan_if, "eth1" ) ? "eth1" : "eth2" );

    if ( M_VENDOR_QUECTEL_UNISOC == at_dev.comd_mVendor )
    {
        nv_set ( "vlan_unisoc_flag", "1" );
    }
    else
    {
        nv_set ( "vlan_unisoc_flag", "0" );
    }

    if ( M_VENDOR_QUECTEL_UNISOC == at_dev.comd_mVendor )
    {
        system ( "ifconfig eth1 up 2>/dev/null" );
    }

    if ( M_VENDOR_QUECTEL_X55 == at_dev.comd_mVendor )
    {
        comd_exec ( CONFIG_HW_BOARD_TYPE, board_type, sizeof ( board_type ) );
        CLOGD ( FINE, "board_type is %s\n", board_type );
        if ( 0 != strcmp ( board_type, "STN0030" ) && 0 != strcmp ( board_type, "STN0040" ) )
        {
            CLOGD ( FINE, "board_type is %s and return!\n" );
            return;
        }
    }

    if ( M_VENDOR_QUECTEL_LTE == at_dev.comd_mVendor )
    {
        CLOGD ( FINE, "start vlan_setup for CONFIG_SW_QUECTEL_X12 !!!\n" );

        for ( vlan_idx = 3; vlan_idx >= 0; vlan_idx-- )
        {
            snprintf ( cmd_str, sizeof ( cmd_str ),
                        "/usr/sbin/X12_vlan_ifconfig.sh %s 10%d 0.0.0.0 0.0.0.0 0.0.0.0",
                        device_if, vlan_idx );
            system ( cmd_str );
        }
    }
    else if( M_VENDOR_TELIT_PLS == at_dev.comd_mVendor )
    {
        CLOGD ( FINE, "start setup for TELIT_PLS_CAT1 !!!\n" );
        for ( vlan_idx = 0; vlan_idx >= 0; vlan_idx-- )
        {
            snprintf ( cmd_str, sizeof ( cmd_str ),
                        "/usr/sbin/X12_vlan_ifconfig.sh %s 10%d 0.0.0.0 0.0.0.0 0.0.0.0",
                        device_if, vlan_idx );
            system ( cmd_str );
        }
    }
    else
    {
        for ( vlan_idx = 0; vlan_idx <= 3; vlan_idx++ )
        {
            snprintf ( cmd_str, sizeof ( cmd_str ),
                        "/usr/sbin/gct_vlan_ifconfig.sh %s 10%d 0.0.0.0 0.0.0.0 0.0.0.0",
                        device_if, vlan_idx );
            system ( cmd_str );
        }
    }

    if ( strstr ( wan_if, "eth2" ) )
    {
        system ( "/lib/netdev_load_balance.sh" );
        system ( "/etc/init.d/ippt restart" );
    }
}


static void multi_apn_vlan_config()
{
    CLOGD ( FINE, "start vlan_setup ...\n" );
#if defined(_MTK_7621_) || defined(_MTK_7981_) || defined(_MTK_7628_)
    if ( M_VENDOR_GCT == at_dev.comd_mVendor ||
        M_VENDOR_QUECTEL_UNISOC == at_dev.comd_mVendor ||
        M_VENDOR_QUECTEL_T750 == at_dev.comd_mVendor ||
        M_VENDOR_QUECTEL_X55 == at_dev.comd_mVendor ||
        M_VENDOR_QUECTEL_LTE == at_dev.comd_mVendor ||
        M_VENDOR_TELIT_PLS == at_dev.comd_mVendor )
    {
        create_vlan_for_GCT_module ();
    }
#elif defined(_QCA_IPQ_)
    if ( M_VENDOR_QUECTEL_X55 == at_dev.comd_mVendor )
    {
        CLOGD ( FINE, "Quectel x55 module ...\n" );
        system ( "/usr/sbin/x55_mac_vlan_setup.sh" );
        system ( "/lib/netdev_load_balance.sh" );
        system ( "/lib/iperf_buff_expand.sh" );
        system ( "/etc/init.d/ippt restart" );
    }
#endif
}

static void *mainLoop ( void *param )
{
    int init_ret = -1;
    int reset_module_timer = 0;

    while ( enter_factory_test_mode() )
    {
        CLOGD ( FINE, "enter factory test mode, comd keep silent !!!\n" );
        comd_wait_ms ( 15000 );
    }

    while ( comd_sem_init ( ( key_t ) AT_PROCESS_SEM_KEY ) )
    {
        CLOGD ( ERROR, "comd_sem_init AT_PROCESS_SEM_KEY FAIL !!!\n" );
        comd_wait_ms ( 3000 );
    }

    while ( comd_sem_init ( ( key_t ) ALL_SMS_SEM_KEY ) )
    {
        CLOGD ( ERROR, "comd_sem_init ALL_SMS_SEM_KEY FAIL !!!\n" );
        comd_wait_ms ( 3000 );
    }

    while ( msgInit() )
    {
        CLOGD ( ERROR, "msgInit FAIL !!!\n" );
        comd_wait_ms ( 3000 );
    }

#if defined(MTK_T750) && CONFIG_QUECTEL_T750
    while ( 1 )
    {
        datacall_init_ret = mtkParam_initDataCallBk ();
        if ( 0 == datacall_init_ret )
        {
            break;
        }
        CLOGD ( ERROR, "mtkParam_initDataCallBk FAIL !!!\n" );
        comd_wait_ms ( 3000 );
    }
#endif

#if defined(_GDM7243_)
    while ( access ( "/tmp/lteatcm.init", 0 ) )
    {
        CLOGD ( FINE, "wait lteatcm init ...\n" );
        comd_wait_ms ( 1000 );
    }
#endif

    while ( 1 )
    {
        if ( 0 == reset_module_timer++ )
        {
            memory_data_restore();
        }
        else if ( reset_module_timer > 60 )
        {
            comd_reboot_module();
            reset_module_timer = 1;
        }

        at_dev.dev_fd = -1;

        switch ( at_dev.comd_ch_type )
        {
        case 0:
        case 1:
#if defined(MTK_T750) && CONFIG_QUECTEL_T750 && USE_QUECTEL_API_SEND_AT
#if (QUECTEL_API_SEND_AT_MODE == 1)
            CLOGD ( FINE, "start quectel_atcid_sender_init ...\n" );
            pcomm_dev = quectel_atcid_sender_init ( TYPE_ADB_ATCI_SOCKET );
            if ( pcomm_dev )
            {
                atchannel_ready = 0;
                CLOGD ( FINE, "quectel_atcid_sender_init ok !!!\n" );
            }
            else
            {
                atchannel_ready = -1;
                CLOGD ( ERROR, "quectel_atcid_sender_init fail !!!\n" );
            }
#else
            atchannel_ready = 0;
            CLOGD ( FINE, "QL_API_SEND_AT_MODE: [%d]\n", QUECTEL_API_SEND_AT_MODE );
#endif
#else
            atchannel_ready = create_at_socket_channel();
#endif
            break;
        case 2:
            atchannel_ready = create_at_serial_channel();
            break;
        default:
            break;
        }

        if ( 0 != atchannel_ready )
        {
            goto NEXT;
        }

        multi_apn_vlan_config();

        memset ( led_mode, 0, sizeof ( led_mode ) );
        comd_exec ( "/lib/hw_def.sh get CONFIG_HW_LED_CNT_THREE | tr -d '\r\n'", led_mode, sizeof ( led_mode ) );
        CLOGD ( FINE, "CONFIG_HW_LED_CNT_THREE -> [%s]\n", led_mode );

        memset ( dualsim_en, 0, sizeof ( dualsim_en ) );
        comd_exec ( "/lib/hw_def.sh get CONFIG_HW_DUALSIM | tr -d '\r\n'", dualsim_en, sizeof ( dualsim_en ) );
        CLOGD ( FINE, "CONFIG_HW_DUALSIM -> [%s]\n", dualsim_en );

        memset ( esim_supp, 0, sizeof ( esim_supp ) );
        comd_exec ( "/lib/hw_def.sh get CONFIG_HW_ESIMSUPP | tr -d '\r\n'", esim_supp, sizeof ( esim_supp ) );
        CLOGD ( FINE, "CONFIG_HW_ESIMSUPP -> [%s]\n", esim_supp );

        memset ( pinlock_enable, 0, sizeof ( pinlock_enable ) );
        comd_exec ( FACTORY_STC_PINLOCK_EN, pinlock_enable, sizeof ( pinlock_enable ) );
        CLOGD ( FINE, "CONFIG_HW_STC_PINLOCK -> %s\n", pinlock_enable );

        memset ( simlock_enable, 0, sizeof ( simlock_enable ) );
        comd_exec ( FACTORY_STC_SIMLOCK_EN, simlock_enable, sizeof ( simlock_enable ) );
        CLOGD ( FINE, "CONFIG_HW_STC_SIMLOCK -> %s\n", simlock_enable );

        memset ( celllock_enable, 0, sizeof ( celllock_enable ) );
        comd_exec ( FACTORY_STC_CELLLOCK_EN, celllock_enable, sizeof ( celllock_enable ) );
        CLOGD ( FINE, "CONFIG_HW_STC_CELLLOCK -> %s\n", celllock_enable );

        memset ( volte_enable, 0, sizeof ( volte_enable ) );
        comd_exec ( CONFIG_HW_VOICE_EN, volte_enable, sizeof ( volte_enable ) );
        CLOGD ( FINE, "CONFIG_HW_VOICE_EN -> %s\n", volte_enable );

        if(access("/etc/config/auto_apn_init",F_OK) == 0)
            system ( "cp /etc/config/auto_apn_init /tmp/auto_apn 2>/dev/null" );

        nv_set ( "main_status", "comd_module_initing" );

        while ( 1 )
        {
            init_ret = module_param_init();
            if ( -2 == init_ret )       // at_handshake fail
                goto NEXT;
            else if ( -1 == init_ret )  // init fail
                comd_wait_ms ( 3000 );
            else                        // init success
            {
                nv_set ( "module_reboot_status", "0" );
                break;
            }

        }

        reset_module_timer = 0;

        pthread_attr_t attr;
        pthread_attr_init ( &attr );
        pthread_attr_setdetachstate ( &attr, PTHREAD_CREATE_DETACHED );

        // start pthread_lte_status
        pthread_create ( &s_tid_status, &attr, ( void * ) pthread_module_status, NULL );

        // This thread is no longer needed in the new design framework
        // start pthread_lte_config
        // pthread_create ( &s_tid_config, &attr, ( void * ) pthread_module_config, NULL );

        pthread_attr_destroy ( &attr );

NEXT:
        comd_wait_ms ( 3000 );
        waitForAtChannelClose();
    }

    return NULL;
}

static int baud_rate_valid ( int rate )
{
    int ind = 0;
    int num = sizeof ( baud_arr ) / sizeof ( int );
    for ( ind = 0; ind < num; ind++ )
    {
        if ( rate == baud_arr[ind] )
        {
            at_dev.comd_ser_rate = speed_arr[ind];
            return 1;
        }
    }

    printf ( "Invalid baud rate: [%d]\n", rate );
    return 0;
}

static void sig_handler ( int signum )
{
    CLOGD ( WARNING, "recv signal: [%d]\n", signum );
    switch ( signum )
    {
    case SIGUSR1 :
        comd_log_init();
        break;
    case SIGUSR2 :
        break;
    case SIGPIPE :
        CLOGD ( WARNING, "Get SIGPIPE, set atchannel_ready = -1 !!!\n" );
        atchannel_ready = -1;
        break;
    default :
        memory_data_restore();
        if ( 0 < at_dev.dev_fd )
        {
            close ( at_dev.dev_fd );
        }

#if defined(MTK_T750) && CONFIG_QUECTEL_T750
#if USE_QUECTEL_API_SEND_AT && (QUECTEL_API_SEND_AT_MODE == 1)
        if ( pcomm_dev )
        {
            quectel_atcid_sender_deinit ( pcomm_dev );
            pcomm_dev = NULL;
        }
#endif
        if ( -1 != datacall_init_ret )
        {
            mtkParam_deInitDataCall ( 0 );
        }
#endif
        MessageEvtSend ( MESSAGE_ID_SODD, MSG_SODD_EVT_NETWORK_DOWN, NULL, 0 );
        CLOGD ( WARNING, "comd exit !!!\n" );
        SYS_INFO ( "comd exit" );
        exit ( 0 );
    }
}

static void signal_init()
{
    struct sigaction sigact;

    sigact.sa_handler = sig_handler;
    sigact.sa_flags = 0;
    sigemptyset ( &sigact.sa_mask );
    sigaction ( SIGUSR1, &sigact, NULL );
    sigaction ( SIGUSR2, &sigact, NULL );
    sigaction ( SIGTERM, &sigact, NULL );
    sigaction ( SIGKILL, &sigact, NULL );
    sigaction ( SIGQUIT, &sigact, NULL );
    sigaction ( SIGINT, &sigact, NULL );
    sigaction ( SIGPIPE, &sigact, NULL );

    /* ignore SIGHUP */
    sigact.sa_handler = SIG_IGN;
    sigaction ( SIGHUP, &sigact, NULL );
}

static void usage ( char *param )
{
    printf ( "Usage: %s [options] ...\n\n", param );
    printf ( "Options:\n" );
    printf ( "\t-v <GCT | SQNS | fibocom_x55 | quectel_x55 | zte_x55 | td_mt5710 | quectel_unisoc | quectel_t750>\n" );
    printf ( "\t-i <ipaddr> -p <port> -u(UDP)\n" );
    printf ( "\t-d </dev/tty> -b <baudrate>\n\n" );
    printf ( "Example:\n" );
    printf ( "\t%s -v fibocom_x55 -i 127.0.0.1 -p 5555 -u\n", param );
    printf ( "\t%s -v fibocom_x55 -d /dev/ttyUSB2 -b 115200\n", param );
    printf ( "\t%s -v quectel_unisoc -d /dev/smd1 -b 115200\n", param );
    exit ( -1 );
}

int main ( int argc, char *argv[] )
{
    int opt = 0;
    int udp_used = 0;
    char *param_str = "v:i:p:d:b:u";

    while ( -1 != ( opt = getopt ( argc, argv, param_str ) ) )
    {
        printf ( "opt = %-*c", 8, opt );
        printf ( "optarg = %-*s", 16, optarg );
        printf ( "optind = %-*d", 8, optind );
        printf ( "argv[optind] = %s\n", argv[optind] );
        switch ( opt )
        {
        case 'v':
            if ( 0 == strcasecmp ( optarg, "GCT" ) )
            {
                at_dev.comd_mVendor = M_VENDOR_GCT;
            }
            else if ( 0 == strcasecmp ( optarg, "SQNS" ) )
            {
                at_dev.comd_mVendor = M_VENDOR_SQNS;
            }
            else if ( 0 == strcasecmp ( optarg, "fibocom_x55" ) )
            {
                at_dev.comd_mVendor = M_VENDOR_FIBOCOM_X55;
            }
            else if ( 0 == strcasecmp ( optarg, "fibocom_x55_pcie" ) )
            {
                at_dev.comd_mVendor = M_VENDOR_FIBOCOM_X55;
                nv_set ( "comd_dial_mode", "pcie" );
            }
            else if ( 0 == strcasecmp ( optarg, "quectel_x55" ) )
            {
                at_dev.comd_mVendor = M_VENDOR_QUECTEL_X55;
            }
            else if ( 0 == strcasecmp ( optarg, "quectel_t750" ) )
            {
                at_dev.comd_mVendor = M_VENDOR_QUECTEL_T750;
            }
            else if ( 0 == strcasecmp ( optarg, "zte_x55" ) )
            {
                at_dev.comd_mVendor = M_VENDOR_ZTE_X55;
            }
            else if ( 0 == strcasecmp ( optarg, "zte_9011" ) )
            {
                at_dev.comd_mVendor = M_VENDOR_ZTE_X55;
            }
            else if ( 0 == strcasecmp ( optarg, "td_mt5710" ) )
            {
                at_dev.comd_mVendor = M_VENDOR_TD_MT5710;
            }
            else if ( 0 == strcasecmp ( optarg, "quectel_unisoc" ) )
            {
                at_dev.comd_mVendor = M_VENDOR_QUECTEL_UNISOC;
            }
            else if ( 0 == strcasecmp ( optarg, "quectel_lte" ) )
            {
                at_dev.comd_mVendor = M_VENDOR_QUECTEL_LTE;
            }
            else if ( 0 == strcasecmp ( optarg, "telit_4g" ) )
            {
                at_dev.comd_mVendor = M_VENDOR_TELIT_4G;
                nv_set("whose_modem", "telit");
            }
            else if ( 0 == strcasecmp ( optarg, "telit_5g" ) )
            {
                at_dev.comd_mVendor = M_VENDOR_TELIT_5G;
                nv_set("whose_modem", "telit");
            }
            else if ( 0 == strcasecmp ( optarg, "telit_pls" ) )
            {
                at_dev.comd_mVendor = M_VENDOR_TELIT_PLS;
                nv_set("whose_modem", "telit_pls");
                nv_set("limit_apn_num", "1");
            }

            else
                usage ( argv[0] );

            break;
        case 'i':
            strncpy ( at_dev.comd_ip_addr, optarg, sizeof ( at_dev.comd_ip_addr ) - 1 );
            break;
        case 'p':
            at_dev.comd_ip_port = atoi ( optarg );
            break;
        case 'd':
            strncpy ( at_dev.comd_ser_name, optarg, sizeof ( at_dev.comd_ser_name ) - 1 );
            break;
        case 'b':
            at_dev.comd_ser_rate = atoi ( optarg );
            break;
        case 'u':
            udp_used = 1;
            break;
        default:
            usage ( argv[0] );
            break;
        }
    }

    char mVendor_tmp[8] = {0};

    snprintf ( mVendor_tmp, sizeof ( mVendor_tmp ), "%d", at_dev.comd_mVendor );
    nv_set ( "comd_moduleVendor", mVendor_tmp );

    if ( IS_IPv4_ADDR ( at_dev.comd_ip_addr ) &&
            ( 0 < at_dev.comd_ip_port && at_dev.comd_ip_port < 65535 ) )
    {
        at_dev.comd_ch_type = ( udp_used ? 0 : 1 );
    }
    else if ( strcmp ( at_dev.comd_ser_name, "" ) && baud_rate_valid ( at_dev.comd_ser_rate ) )
    {
        at_dev.comd_ch_type = 2;
    }
    else
        usage ( argv[0] );

    signal_init();

    comd_log_init();

    SYS_INFO ( "comd start" );

    mainLoop ( NULL );

    return 0;
}

