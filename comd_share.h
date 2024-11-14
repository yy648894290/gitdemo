#ifndef _COMD_SHARE_H
#define _COMD_SHARE_H 1

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <dirent.h>
#include <getopt.h>
#include <errno.h>
#include <fcntl.h>
#include <memory.h>
#include <termios.h>
#include <pthread.h>
#include <stdarg.h>
#include <syslog.h>
#include <regex.h>
#include <ctype.h>
#include <signal.h>
#include <time.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/sysinfo.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <net/if.h>
#include <net/route.h>
#include <linux/sockios.h>
#include <linux/ip.h>
#include <arpa/inet.h>
#include <sqlite3.h>

#include "log.h"
#include "board_config.h"

#include "message.h"
#include "libsod.h"
#include "utils_api.h"
#include "conf.h"


#define SMS_DIR "/tmp/sms/sms.db"
#define SMS_DIR_TMP "/tmp/sms_tmp.db"

#define DNS_CONF_FILE "/tmp/resolv.conf.lte"

#include "uci_cfg.h"

#if defined(_GDM7243_)
#define AT_TIMEOUT_TIMER 15
#else
#define AT_TIMEOUT_TIMER 5
#endif

#define FIBOCOM_GTRNDIS_UPDATE_EN 1
#define FIBOCOM_GOBINET_DRIVER_EN 1

#define UNLOCK_CODE_NUM 4
#define UNLOCK_CODE_LEN 8

#define STC_PINLOCK_CODE "0000"

#define FACTORY_STC_PINLOCK_EN "/lib/factory_tool.sh show | grep ^CONFIG_HW_STC_PINLOCK | cut -d '=' -f 2 | tr -d '\r\n'"
#define FACTORY_STC_SIMLOCK_EN "/lib/factory_tool.sh show | grep ^CONFIG_HW_STC_SIMLOCK | cut -d '=' -f 2 | tr -d '\r\n'"
#define FACTORY_STC_CELLLOCK_EN "/lib/factory_tool.sh show | grep ^CONFIG_HW_STC_CELLLOCK | cut -d '=' -f 2 | tr -d '\r\n'"
#define CONFIG_HW_VOICE_EN "/lib/factory_tool.sh show | grep CONFIG_HW_VOICE | cut -d '=' -f 2 | tr -d '\r\n'"
#define CONFIG_HW_BOARD_TYPE "/lib/factory_tool.sh show | grep CONFIG_HW_BOARD_TYPE | cut -d '=' -f 2 | tr -d '\r\n'"

#define FACTORY_STC_PIN_CODE "/lib/factory_tool.sh show | grep ^CONFIG_HW_STC_PIN_CODE | cut -d '=' -f 2 | tr -d '\r\n'"
#define FACTORY_STC_ICCID "/lib/factory_tool.sh show | grep ^CONFIG_HW_STC_ICCID | cut -d '=' -f 2 | tr -d '\r\n'"

//#define QUECTEL_X55_QCMAP_EN 1

#define QUECTEL_RM500Q_GL_4G_BANDS  "1:2:3:4:5:7:8:12:13:14:17:18:19:20:25:26:28:29:30:32:34:38:39:40:41:42:43:46:48:66:71"
#define QUECTEL_RM500Q_GL_5G_BANDS  "1:2:3:5:7:8:12:20:25:28:38:40:41:48:66:71:77:78:79"
#define QUECTEL_RM505Q_AE_4G_BANDS  "1:2:3:4:5:7:8:12:13:14:17:18:19:20:25:26:28:29:30:32:34:38:39:40:41:42:43:46:48:66:71"
#define QUECTEL_RM505Q_AE_5G_BANDS  "1:2:3:5:7:8:12:20:25:28:38:40:41:48:66:71:77:78:79"
#define QUECTEL_RM520N_GL_4G_BANDS  "1:2:3:4:5:7:8:12:13:14:17:18:19:20:25:26:28:29:30:32:34:38:39:40:41:42:43:46:48:66:71"
#define QUECTEL_RM520N_GL_5G_BANDS  "1:2:3:5:7:8:12:13:14:18:20:25:26:28:29:30:38:40:41:48:66:70:71:75:76:77:78:79"
#define QUECTEL_RG500Q_EA_4G_BANDS  "1:3:5:7:8:18:19:20:26:28:32:34:38:39:40:41:42:43"
#define QUECTEL_RG500Q_EA_5G_BANDS  "1:3:5:7:8:20:28:38:40:41:77:78:79"
#define QUECTEL_RG500Q_NA_4G_BANDS  "2:4:5:7:12:13:14:17:25:26:29:30:41:46:48:66:71"
#define QUECTEL_RG500Q_NA_5G_BANDS  "2:5:7:12:25:41:48:66:71:77:78"
#define QUECTEL_RG500Q_NA_4G_BANDS  "2:4:5:7:12:13:14:17:25:26:29:30:41:46:48:66:71"
#define QUECTEL_RG500Q_NA_5G_BANDS  "2:5:7:12:25:41:48:66:71:77:78"
#define QUECTEL_RG500L_EU_4G_BANDS  "1:3:5:7:8:20:28:32:38:40:41:42:43"
#define QUECTEL_RG500L_EU_5G_BANDS  "1:3:5:7:8:20:28:38:40:41:77:78"
#define QUECTEL_RG500L_NA_4G_BANDS  "2:4:5:7:12:13:14:17:25:26:29:30:38:41:42:43:46:48:66:71"
#define QUECTEL_RG500L_NA_5G_BANDS  "2:5:7:12:25:38:41:48:66:71:77:78"
#define QUECTEL_RG600L_EU_4G_BANDS  "1:3:5:7:8:20:28:32:38:40:41:42:43:71"
#define QUECTEL_RG600L_EU_5G_BANDS  "1:3:5:7:8:20:28:38:40:41:71:75:76:77:78"
#define QUECTEL_EM160R_GL_4G_BANDS  "1:2:3:4:5:7:8:12:13:14:17:18:19:20:25:26:28:29:30:32:38:39:40:41:42:43:46:48:66"
#define QUECTEL_EM160R_GL_5G_BANDS  ""
#define QUECTEL_RG520F_EU_4G_BANDS  "1:3:5:7:8:20:28:32:38:40:41:42:43"
#define QUECTEL_RG520F_EU_5G_BANDS  "1:3:5:7:8:20:28:38:40:41:75:76:77:78"
#define QUECTEL_RG520F_NA_4G_BANDS  "2:4:5:7:12:13:14:17:25:26:29:30:38:41:42:43:46:48:66:71"
#define QUECTEL_RG520F_NA_5G_BANDS  "2:5:7:12:13:14:25:26:29:30:38:41:48:66:70:71:77:78"
#define QUECTEL_RG620T_EU_4G_BANDS  "1:3:5:7:8:20:28:38:40:41:42:43:46"
#define QUECTEL_RG620T_EU_5G_BANDS  "1:3:5:7:8:20:28:38:40:41:75:76:77:78"
#define QUECTEL_RG620T_NA_4G_BANDS  "2:4:5:7:12:13:14:25:26:30:38:41:42:43:48:66:70:71"
#define QUECTEL_RG620T_NA_5G_BANDS  "2:5:7:12:13:14:25:26:29:30:38:41:48:66:70:71:77:78"
#define QUECTEL_RG500U_EA_4G_BANDS  "1:2:3:4:5:7:8:20:28:38:40:41:66"
#define QUECTEL_RG500U_EA_5G_BANDS  "1:3:7:8:20:28:38:40:41:77:78:79"
#define QUECTEL_RG500U_EB_4G_BANDS  "1:2:3:4:5:7:8:20:28:38:40:41:66"
#define QUECTEL_RG500U_EB_5G_BANDS  "1:3:5:7:8:20:28:38:40:41:66:77:78"
#define QUECTEL_EG120K_EA_4G_BANDS  "1:3:5:7:8:20:28:32:38:40:41:42:43"
#define QUECTEL_EG120K_NA_4G_BANDS  "2:4:5:7:12:13:14:25:26:29:30:41:48:66:71"
#define QUECTEL_EC200A_CN_4G_BANDS  "1:3:5:8:34:38:39:40:41"
#define TELIT_PLS_4G_BANDS          "1:2:3:4:5:7:8:12:13:18:19:20:27:28:38:40:41:66"

#define TDTEK_MT5710_CN_4G_BANDS    "1:3:5:8:34:38:39:40:41"
#define TDTEK_MT5710_CN_5G_BANDS    "1:3:5:8:28:41:78:79"

#define M_VENDOR_GCT            1
#define M_VENDOR_SQNS           2
#define M_VENDOR_FIBOCOM_X55    3
#define M_VENDOR_QUECTEL_X55    4
#define M_VENDOR_ZTE_X55        5
#define M_VENDOR_QUECTEL_T750   6
#define M_VENDOR_FIBOCOM_T750   7
#define M_VENDOR_TD_MT5710      8
#define M_VENDOR_QUECTEL_UNISOC 9
#define M_VENDOR_QUECTEL_LTE    10
#define M_VENDOR_TELIT_4G       11
#define M_VENDOR_TELIT_5G       12
#define M_VENDOR_TELIT_PLS      13

#define GCT_VOLTE_APN_INDEX 2

#if defined ( CONFIG_SW_QUECTEL_X62 )
#define APN_OFFSET 0
#else
#define APN_OFFSET 5
#endif

/*
 * 0: use mtk 17171 socket port
 * 1: use quectel api
 */
#define USE_QUECTEL_API_SEND_AT     0

/*
 * valid when USE_QUECTEL_API_SEND_AT is 1
 *
 * 0: init & deinit every time when sending AT
 * 1: init & deinit only when comd start & exit
 */
#define QUECTEL_API_SEND_AT_MODE    0

#ifndef _IPV6_H
struct in6_ifreq
{
    struct in6_addr ifr6_addr;
    unsigned int ifr6_prefixlen;
    int ifr6_ifindex;
};
#endif

void comd_log_init ();

#define CLOGD(lEVEL, fmt, args...)   logEntry(__func__, __LINE__, #lEVEL, fmt, ##args)
void logEntry ( const char* func, int line, const char* level, const char *fmt, ... );

void ucfg_info_get ( char* nodeStr, char* node, char* get_val, int get_len );
void ucfg_info_set ( char* nodeStr, char* set_val );

void calc_ipv4_gw_from_addr ( char* ipaddr, char* gateway, int gw_len );
void calc_x55_mask_gw_from_ip ( char* ip_addr, char* net_mask, char* gate_way );

#define SEND_MAX_SIZE 512
#define RECV_BUF_SIZE 512
#define RECV_SMS_SIZE 51200
#define REGEX_BUF_ONE 128
#define STR_AT_RSP_LEN 128
#define STR_AT_RSP_LEN_2X (STR_AT_RSP_LEN * 2)
#define STR_AT_RSP_LEN_4X (STR_AT_RSP_LEN * 4)

#define MAX_REGEX_MATCH_NUM 16

#define MSG_DATA_BUF_MAX_LEN 640
#define MSG_TYPE_DEFAULT     1

typedef struct
{
    int comd_mVendor;
    int comd_ch_type;
    char comd_ser_name[16];
    int comd_ser_rate;
    char comd_ip_addr[16];
    int comd_ip_port;
    int dev_fd;
} COMD_DEV_INFO;

struct KT_char_akaRequest
{
    int KT_simType;
    int KT_pRandLen;
    char KT_pRand[64];
    int KT_pAutnLen;
    char KT_pAutn[64];
};

struct KT_char_akaResult
{
    int KT_akaResult;
    int KT_respLen;
    char KT_respStr[64];
    int KT_ikLen;
    char KT_ikStr[64];
    int KT_ckLen;
    char KT_ckStr[64];
    int KT_autsLen;
    char KT_autsStr[64];
};

#define APN_LIST_PARAMETER_LEN 32

typedef struct
{
    int index;
    char apn_enable[APN_LIST_PARAMETER_LEN];
    char profile_name[APN_LIST_PARAMETER_LEN];
    char apn_name[APN_LIST_PARAMETER_LEN];
    char auth_type[APN_LIST_PARAMETER_LEN];
    char user_name[APN_LIST_PARAMETER_LEN];
    char password[APN_LIST_PARAMETER_LEN];
    char pdn_type[APN_LIST_PARAMETER_LEN];
    char band_mac[APN_LIST_PARAMETER_LEN];
    char vid[APN_LIST_PARAMETER_LEN];
    int mtu;
} apn_profile;

int comd_exec ( char *cmd, char *buf, int buf_len );
int comd_wait_ms ( int time_ms );

void comd_reboot_module();

typedef struct
{
    char devName[16];
    unsigned char devMac[6];
} LTE_DEV_MAC_ADDR_T;
int getMacAddr ( LTE_DEV_MAC_ADDR_T *pMacInfo );

#define AT_PROCESS_SEM_KEY      0x524F5346
#define ALL_SMS_SEM_KEY         0x524F5347
union semun
{
    int val;
    struct semid_ds *buf;
    unsigned short *arry;
};
int comd_semaphore_p ( int sem_id );
int comd_semaphore_v ( int sem_id );
int comd_sem_init ( key_t sem_key );
int comd_sem_exit ( int sem_id );
int shell_cmd_return_str ( char *cmd, char *ret );

#define SMS_READ_TXT     ",1,,"     // "REC READ"
#define SMS_UNREAD_TXT   ",0,,"     // "REC UNREAD"
#define SMS_SENT_TXT     ",3,,"     // "STO SENT"
#define SMS_UNSENT_TXT   ",2,,"     // "STO UNSENT"

#define SMS_READ_TXT_QUECTEL_T750     ", 1,,"     // "REC READ"
#define SMS_UNREAD_TXT_QUECTEL_T750   ", 0,,"     // "REC UNREAD"
#define SMS_SENT_TXT_QUECTEL_T750     ", 3,,"     // "STO SENT"
#define SMS_UNSENT_TXT_QUECTEL_T750   ", 2,,"     // "STO UNSENT"

#define SMS_READ_TAG     0
#define SMS_UNREAD_TAG   1
#define SMS_SENT_TAG     2
#define SMS_UNSENT_TAG   3

#define MAX_SMS_MSG_SIZE            512
#define MAX_SMS_PDU_SIZE            384
#define MAX_SMS_NUM                 192
typedef struct KT_sms_msg
{
    int cmd_type;
    int req_type;
    int num_len;
    unsigned char sms_msg[MAX_SMS_MSG_SIZE];
} KT_sms_msg;

typedef struct _ATC_PDN_OPERATOR_NAME_
{
    char *pCountryInital;
    char *pMCC;
    char *pMNC;
    char *pLongName;
    char *pShortName;
} ATC_PDN_OPERATOR_NAME;

void reset_apns_msg_flag ( int flag );

void HexToStr ( char *pbDest, const char *pbSrc, int nLen );

void lteinfo_data_restore();
void ipv4v6_data_restore ( int apn_index );
void memory_data_restore();

typedef struct _LTEINFO_AT_MSG_
{
    char rrc_state[16];         // RRC State : RRC CONNECTED
    char dl_earfcn[9 + 7];      // EARFCN : 40936
    char ul_earfcn[11 + 7];     // ULEARFCN : 40936
    char mcc[6 + 4];            // MCC : 460
    char mnc[6 + 4];            // MNC : 00
    char tac[6 + 9];            // TAC : 24(119)
    char cellid[6 + 11];        // CID : 134753729
    char band[6 + 4];           // Bd : B41
    char dl_bandwidth[7 + 4];   // D : 20MHz
    char ul_bandwidth[7 + 4];   // U : 20MHz
    char cinr[6 + 24];          // SNR : 2.3,2.7,-17.0,-17.0
    char pci[6 + 4];            // PCI : 20
    char rsrq[7 + 16];          // RSRQ : -12,-12,-17,-18
    char rsrp[7 + 20];          // RSRP : -92,-95,-129,-132
    char rssi[7 + 20];          // RSSI : -59,-61,-88,-93
    char dl_freq[9 + 8];        // DLFREQ : 2624.6
    char ul_freq[9 + 8];        // ULFREQ : 2624.6
    char dl_mcs[8 + 4];         // DLMCS : 23
    char ul_mcs[8 + 4];         // ULMCS : 8
    char tx_power[10 + 5];      // TXPOWER : -110
    char sinr[7 + 4];           // SINR : 6
    char rrc_power[11 + 4];     // RRCPOWER : 0
    char rxlv[7 + 4];           // RXLV : 33
    char tx_mode[9 + 3];        // TXMODE : 2
    char handover[11 + 8];      // HANDOVER : 0,0
    char bears_count[13 + 3];   // BEARSCOUNT : 2
} __attribute__ ( ( aligned(8) ) ) LTEINFO_AT_MSG;

int get_cid_from_apn_index ( int index );
int get_apn_from_cid_index ( int index );

void clean_connected_time ();
void set_connected_total_time();
void apnActOrDeactMsgEvent ( int apn_index, int apn_event );

int calc_band_from_earfcn ( int earfcn_dl, int bands[], int bands_count );

typedef struct _CELL_EARFCN_FREQ_
{
    unsigned int band;
    unsigned int dl_earfcn;
    unsigned int ul_earfcn;
    unsigned int dl_frequency;       /* KHz */
    unsigned int ul_frequency;       /* KHz */
} CELL_EARFCN_FREQ;

typedef struct _ATC_4G_BAND_FREQ_INFO_
{
    unsigned int band;
    unsigned int band_mode;          /* 0: undefined, 1: FDD, 2: TDD, 3: SDL, 4: SUL */
    unsigned int channel_type;       /* 20MHz 15MHz 10MHz 5MHz 3MHz 1.4MHz */
    unsigned int band_width;         /* KHz */
    unsigned int dl_start_earfcn;
    unsigned int dl_start_freq;      /* KHz */
    unsigned int ul_start_earfcn;
    unsigned int ul_start_freq;      /* KHz */
} ATC_4G_BAND_FREQ_INFO;

typedef struct _ATC_5G_BAND_FREQ_INFO_
{
    unsigned int band;
    unsigned int band_mode;          /* 0: undefined, 1: FDD, 2: TDD, 3: SDL, 4: SUL */
    unsigned int channel_type;       /* 100MHz 90MHz 80MHz 70MHz 60MHz 50MHz 40MHz 30MHz 25MHz 20MHz 15MHz 10MHz 5MHz */
    unsigned int scs_type;           /* 120KHz 60KHz 30KHz 15KHz */
    unsigned int band_width;         /* KHz */
    unsigned int raster_ref;         /* KHz */
    unsigned int dl_start_narfcn;
    unsigned int dl_start_freq;      /* KHz */
    unsigned int ul_start_narfcn;
    unsigned int ul_start_freq;      /* KHz */
} ATC_5G_BAND_FREQ_INFO;

int calc_freq_from_earfcn ( CELL_EARFCN_FREQ* info );
int calc_5g_freq_from_narfcn ( CELL_EARFCN_FREQ* info_5g );
int calc_5g_duplex_mode_from_narfcn ( int narfcn_dl );
int calc_4g_band_from_earfcn( int earfcn_dl, char *band_list, int length );
int calc_5g_band_from_narfcn( int narfcn_dl, char *band_list, int length );
void split_and_remove_duplicates(char *str_in, char *str_out, int length);

unsigned long nitz_time_total_second (
    const unsigned int year0, const unsigned int mon0,
    const unsigned int day, const unsigned int hour,
    const unsigned int min, const unsigned int sec
);

int calc_ipv6_addr_prefix ( char *full_v6addr, unsigned int mask, char* v6addr_prefix );
int calc_spec_char_counts ( char *str_org, char spec_char );

int comd_strrpl ( char *str_org, char *x, char *y );
void delchar ( char *str, char c );
unsigned long mix ( unsigned long a, unsigned long b, unsigned long c );
int autoApnSetting( int apn_index,apn_profile* apn_settingdata);
int verify_5g_narfcn_by_band(char* band, char* narfcn);

void decodeGsm7bitPdu ( char *szDes, const char *pbSrc, int nLen );
int unicode2chars ( char* my_uni, char* my_str, int str_len );

void padding_char_between_two_same_symbol(char* pad_char, char* symbol, char* input_str, char* output_str);

#endif /* _COMD_SHARE_H */

