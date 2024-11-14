#include "comd_share.h"
#include "comd_sms.h"
#include "atchannel.h"
#include "mtk_status_refresh.h"
#include "mtk_atcmd_parsing.h"
#include "mtk_config_response.h"
#include "config_key.h"
#include "hal_platform.h"

#define USE_QUECTEL_VOICE_API 0
#define USE_QUECTEL_USSD_API 1

static int abnormal_check_flag[5] = { 1, 1, 1, 1, 1 };
static int cpin_error_check = 0;
static int main_status_check = 0;
static int dial_status_check = 0;
static int param_init_done = 0;

static long abnormal_start_time = 0;

extern int manual_conn;
extern char led_mode[4];
extern char dualsim_en[4];
extern int apns_msg_flag[5];
extern char volte_enable[8];
extern char unlock_code[UNLOCK_CODE_NUM][UNLOCK_CODE_LEN + 1];
extern char celllock_enable[4];
extern char pinlock_enable[4];
extern char simlock_enable[4];

#if defined(MTK_T750) && CONFIG_QUECTEL_T750

#if USE_QUECTEL_VOICE_API
#include "ql_slic.h"
#include "ql_voice.h"
#endif

#include "ql_nw.h"
#include "ql_type.h"
#include "ql_data_call.h"
#include "ql_net_common.h"

#if USE_QUECTEL_USSD_API
#include "ql_sms.h"
#endif

#define APN_PLUS    0

#define IS_BACKGROUND FALSE

#ifndef FALSE
#define FALSE (0)
#endif

#ifndef TRUE
#define TRUE (!FALSE)
#endif

extern CELL_EARFCN_FREQ earfcn_freq;
extern int datacall_init_ret;

static int mtkParam_initIAAPN ( char *apn, char *pdp, char *auth, char *user, char *pass )
{
    int ret = -1;
    bool set_ia_default = TRUE;
    ql_set_ia_apn_config_t cfg;

    memset ( &cfg, 0, sizeof ( ql_set_ia_apn_config_t ) );

    snprintf ( cfg.apn_name, QL_NET_MAX_APN_NAME_LEN, "%s", apn );

    if ( 0 == strcmp ( pdp, "IP" ) )
    {
        cfg.ip_type = QL_NET_IP_VER_V4;
    }
    else if ( 0 == strcmp ( pdp, "IPV6" ) )
    {
        cfg.ip_type = QL_NET_IP_VER_V6;
    }
    else
    {
        cfg.ip_type = QL_NET_IP_VER_V4V6;
    }
    cfg.roaming_type = cfg.ip_type;

    if ( 0 == strcmp ( auth, "2" ) )
    {
        cfg.auth_type = QL_NET_AUTH_PREF_CHAP_ONLY_ALLOWED;
    }
    else if ( 0 == strcmp ( auth, "1" ) )
    {
        cfg.auth_type = QL_NET_AUTH_PREF_PAP_ONLY_ALLOWED;
    }
    else
    {
        cfg.auth_type = QL_NET_AUTH_PREF_PAP_CHAP_NOT_ALLOWED;
    }

    snprintf ( cfg.username, QL_NET_MAX_APN_USERNAME_LEN, "%s", user );
    snprintf ( cfg.password, QL_NET_MAX_APN_PASSWORD_LEN, "%s", pass );

    ret = ql_ia_apn_set ( &cfg, set_ia_default );

    CLOGD ( FINE, "ia_apn_set: [%s].[%d].[%d],[%d].[%s].[%s] -> ret: [%d]\n",
            cfg.apn_name, cfg.ip_type, cfg.roaming_type,
            cfg.auth_type, cfg.username, cfg.password,
            ret
        );

    return ret;
}

static void get_cell_id ( int scc_id, int rat, char *globalID, char *plmn )
{
    unsigned long id = 0;
    int id1 = 0;
    int id2 = 0;
    char p_id2[8] = {0};
    char *p_id1 = NULL;
    char tmp_str[64] = {0};
    char nv_enbid[16] = {0};
    char nv_cellid[16] = {0};
    char nv_cgi[16] = {0};

    if ( 0 < scc_id )
    {
        snprintf ( tmp_str, sizeof ( tmp_str ), "S%d_%s", scc_id, ( 4 == rat ) ? "" : "5g_" );
    }
    else if ( 4 != rat )
    {
        snprintf ( tmp_str, sizeof ( tmp_str ), "%s", "5g_" );
    }

    snprintf ( nv_enbid, sizeof ( nv_enbid ), "%seNBid", tmp_str );
    snprintf ( nv_cellid, sizeof ( nv_cellid ), "%scellid", tmp_str );
    snprintf ( nv_cgi, sizeof ( nv_cgi ), "%scgi", tmp_str );

    if ( strstr ( globalID, "fffffff" ) || 0 == strcmp ( globalID, "0" ) )
    {
        nv_set ( nv_enbid, "N/A" );
        nv_set ( nv_cellid, "N/A" );
        nv_set ( nv_cgi, "N/A" );
        return ;
    }

    if ( 4 == rat )
    {
        sscanf ( globalID, "%lx", &id );

        id1 = ( id & 0x00000FF );
        snprintf ( tmp_str, sizeof ( tmp_str ), "%d", id1 );
        nv_set ( nv_cellid, tmp_str );

        id2 = ( id >> 8 );
        snprintf ( tmp_str, sizeof ( tmp_str ), "%d", id2 );
        nv_set ( nv_enbid, tmp_str );

        snprintf ( tmp_str, sizeof ( tmp_str ), "Cell ID:%07d-%03d PLMN:%s", id2, id1, plmn);
        nv_set ( nv_cgi, tmp_str );
    }
    else
    {
        p_id1 = globalID + 6;
        sscanf ( p_id1, "%x", &id1 );
        snprintf ( tmp_str, sizeof ( tmp_str ), "%d", id1 );
        nv_set ( nv_cellid, tmp_str );

        snprintf ( p_id2, 7, "%s", globalID );
        sscanf ( p_id2, "%x", &id2 );
        snprintf ( tmp_str, sizeof ( tmp_str ), "%d", id2 );
        nv_set ( nv_enbid, tmp_str );

        snprintf ( tmp_str, sizeof ( tmp_str ), "Cell ID:%08d-%04d PLMN:%s", id2, id1, plmn);
        nv_set ( nv_cgi, tmp_str );
    }
}

static const char* serving_rat_to_string ( QL_NW_RADIO_TECH_TYPE_E cell_type )
{
    switch ( cell_type )
    {
    case QL_NW_RADIO_TECH_NONE:     return "QL_NW_RADIO_TECH_NONE";
    case QL_NW_RADIO_TECH_GSM:      return "QL_NW_RADIO_TECH_GSM";
    case QL_NW_RADIO_TECH_UMTS:     return "QL_NW_RADIO_TECH_WCDMA";
    case QL_NW_RADIO_TECH_TD_SCDMA: return "QL_NW_RADIO_TECH_TD_SCDMA";
    case QL_NW_RADIO_TECH_LTE:      return "QL_NW_RADIO_TECH_LTE";
    case QL_NW_RADIO_TECH_NR5G:     return "QL_NW_RADIO_TECH_NR5G";
    case QL_NW_RADIO_TECH_NSA5G:    return "QL_NW_RADIO_TECH_NSA5G";
    default:                        return "UNKNOWN_RADIO_MODE";
    }
}

static const char* reg_state_to_string ( QL_NW_SERVICE_TYPE_E type )
{
    switch ( type )
    {
    case QL_NW_SERVICE_NONE:        return "QL_NW_SERVICE_NONE";
    case QL_NW_SERVICE_LIMITED:     return "QL_NW_SERVICE_LIMITED";
    case QL_NW_SERVICE_FULL:        return "QL_NW_SERVICE_FULL";
    case QL_NW_SERVICE_SEARCHING:   return "QL_NW_SERVICE_SEARCHING";
    case QL_NW_SERVICE_DENIED:      return "QL_NW_SERVICE_DENIED";
    case QL_NW_SERVICE_UNKNOWN:     return "QL_NW_SERVICE_UNKNOWN";
    case QL_NW_SERVICE_ROAMING:     return "QL_NW_SERVICE_ROAMING";
    case QL_NW_SERVICE_HOME_SMS_ONLY: return "QL_NW_SERVICE_HOME_SMS_ONLY";
    case QL_NW_SERVICE_ROAMING_SMS_ONLY: return "QL_NW_SERVICE_ROAMING_SMS_ONLY";
    case QL_NW_SERVICE_HOME_CSFB_NOT_PREF: return "QL_NW_SERVICE_HOME_CSFB_NOT_PREF";
    case QL_NW_SERVICE_ROAMING_CSFB_NOT_PREF: return "QL_NW_SERVICE_ROAMING_CSFB_NOT_PREF";
    default:                        return "<unknown SERVICE_TYPE>";
    }
}

static void print_reg_status_info ( ql_nw_reg_status_info_t *p_reg_info )
{
    if ( !p_reg_info )
    {
        CLOGD ( WARNING, "Invalid parameter !\n" );
        return;
    }

    CLOGD ( FINE, "tech_domain = [%s]\n", "3GPP" );
    CLOGD ( FINE, "radio_tech  = [%s]\n", serving_rat_to_string ( p_reg_info->radio_tech ) );
    CLOGD ( FINE, "roaming     = [%d]\n", p_reg_info->roaming );
    CLOGD ( FINE, "deny_reason = [%d]\n", ( int ) p_reg_info->deny_reason );
    CLOGD ( FINE, "reg_state   = [%s]\n", reg_state_to_string ( p_reg_info->reg_state ) );
    CLOGD ( FINE, "forbidden   = [%d]\n", p_reg_info->forbidden );
    CLOGD ( FINE, "mcc = [%s]\n", p_reg_info->mcc );
    CLOGD ( FINE, "mnc = [%s]\n", p_reg_info->mnc );
    CLOGD ( FINE, "cid = [%lx]\n", p_reg_info->cid );
    CLOGD ( FINE, "lac = [%x]\n", p_reg_info->lac );
    CLOGD ( FINE, "psc = [%d]\n", p_reg_info->psc );
    CLOGD ( FINE, "tac = [%x]\n", p_reg_info->tac );
    CLOGD ( FINE, "css = [%d]\n", p_reg_info->css );
    CLOGD ( FINE, "sid = [%d]\n", p_reg_info->sid );
    CLOGD ( FINE, "nid = [%d]\n", p_reg_info->nid );
    CLOGD ( FINE, "bsid  = [%d]\n", p_reg_info->bsid );
    CLOGD ( FINE, "inPRL = [%d]\n", p_reg_info->inPRL );
}

#if USE_QUECTEL_VOICE_API
static void ims_reg_status_info_cb ( ql_ims_status_ind_info *p_info )
{
    static int old_ims_status = -1;

    CLOGD ( FINE, "IMS reg status change: [%d] -> [%d]\n", old_ims_status, p_info->ims_state );

    switch ( p_info->ims_state )
    {
    case QL_IMS_STATE_REGISTERED:
        system ( "echo Registered > /tmp/voice/voice-reg-status.txt" );
        break;
    case QL_IMS_STATE_UNREGISTERED:
    default:
        system ( "echo Unregistered > /tmp/voice/voice-reg-status.txt" );
        break;
    }

    old_ims_status = p_info->ims_state;
}
#endif

static void data_call_status_ind_cb ( int org_call_id, QL_NET_DATA_CALL_STATUS_E pre_call_status, ql_data_call_status_t *p_msg )
{
    char strRamOpt[32] = {0};
    char cmd_str[256]={0};

    CLOGD ( FINE, "data call [%d]: status change from [%d] to [%d]\n",
                            org_call_id, pre_call_status, p_msg->call_status );

    if ( org_call_id != p_msg->call_id || org_call_id <= APN_PLUS )
    {
        CLOGD ( WARNING, "org_call_id: [%d], p_msg->call_id: [%d]\n", org_call_id, p_msg->call_id );
        return;
    }

    int call_id = org_call_id - APN_PLUS;

    switch ( p_msg->call_status )
    {
    case QL_NET_DATA_CALL_STATUS_CONNECTED:
    case QL_NET_DATA_CALL_STATUS_PARTIAL_V4_CONNECTED:
    case QL_NET_DATA_CALL_STATUS_PARTIAL_V6_CONNECTED:
        CLOGD ( FINE, "call_id      : [%d]\n", p_msg->call_id );
        CLOGD ( FINE, "call_name    : [%s]\n", p_msg->call_name );
        CLOGD ( FINE, "device_name  : [%s]\n", p_msg->device );
        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_netdev", call_id );
        nv_set ( strRamOpt, p_msg->device );

        system ( "killall -USR1 monitord" );

        if ( p_msg->has_addr )
        {
            CLOGD ( FINE, "IPV4 addr    : [%s]\n", p_msg->addr.addr );
            CLOGD ( FINE, "IPV4 gateway : [%s]\n", p_msg->addr.gateway );
            CLOGD ( FINE, "IPV4 netmask : [%s]\n", p_msg->addr.netmask );
            CLOGD ( FINE, "IPV4 dns1    : [%s]\n", p_msg->addr.dnsp );
            CLOGD ( FINE, "IPV4 dns2    : [%s]\n", p_msg->addr.dnss );
            CLOGD ( FINE, "IPV4 mtu     : [%d]\n", p_msg->addr.v4_mtu );
            if ( 1 <= call_id && call_id <= 4 )
            {
                snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ip", call_id );
                nv_set ( strRamOpt, p_msg->addr.addr );
                snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_mask", call_id );
                nv_set ( strRamOpt, p_msg->addr.netmask );
                snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_gateway", call_id );
                nv_set ( strRamOpt, p_msg->addr.gateway );
                snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_dns1", call_id );
                nv_set ( strRamOpt, p_msg->addr.dnsp );
                snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_dns2", call_id );
                nv_set ( strRamOpt, p_msg->addr.dnss );
                snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_state", call_id );
                nv_set ( strRamOpt, "connect" );
                snprintf(cmd_str,sizeof(cmd_str),"ifconfig %s netmask %s",p_msg->device,p_msg->addr.netmask);
                system(cmd_str);
            }
        }
        if ( p_msg->has_addr6 )
        {
            CLOGD ( FINE, "IPV6 addr    : [%s]\n", p_msg->addr6.addr );
            CLOGD ( FINE, "IPV6 gateway : [%s]\n", p_msg->addr6.gateway );
            CLOGD ( FINE, "IPV6 prefix  : [%s]\n", p_msg->addr6.prefix );
            CLOGD ( FINE, "IPV6 dns1    : [%s]\n", p_msg->addr6.dnsp );
            CLOGD ( FINE, "IPV6 dns2    : [%s]\n", p_msg->addr6.dnss );
            CLOGD ( FINE, "IPV6 mtu     : [%d]\n", p_msg->addr6.v6_mtu );
            if ( 1 <= call_id && call_id <= 4 )
            {
                snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_ip", call_id );
                nv_set ( strRamOpt, p_msg->addr6.addr );
                snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_mask", call_id );
                nv_set ( strRamOpt, "FFFF:FFFF:FFFF:FFFF::" );
                snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_gateway", call_id );
                nv_set ( strRamOpt, "FE80::1" );
                snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_dns1", call_id );
                nv_set ( strRamOpt, p_msg->addr6.dnsp );
                snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_dns2", call_id );
                nv_set ( strRamOpt, p_msg->addr6.dnss );
                snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_state", call_id );
                nv_set ( strRamOpt, "connect" );
            }
        }
        if ( 1 <= call_id && call_id <= 4 )
        {
            CLOGD ( FINE, "APN[%d] act msg ...\n", call_id );
            apns_msg_flag [ call_id ] = 0;
            apnActOrDeactMsgEvent ( call_id, 1 );
        }
        break;
    case QL_NET_DATA_CALL_STATUS_IDLE:
    case QL_NET_DATA_CALL_STATUS_DISCONNECTED:
        if ( pre_call_status == QL_NET_DATA_CALL_STATUS_CONNECTED ||
             pre_call_status == QL_NET_DATA_CALL_STATUS_PARTIAL_V4_CONNECTED ||
             pre_call_status == QL_NET_DATA_CALL_STATUS_PARTIAL_V6_CONNECTED )
        {
            if ( 1 <= call_id && call_id <= 4 )
            {
                snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ip", call_id );
                nv_set ( strRamOpt, "--" );
                snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_mask", call_id );
                nv_set ( strRamOpt, "--" );
                snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_gateway", call_id );
                nv_set ( strRamOpt, "--" );
                snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_dns1", call_id );
                nv_set ( strRamOpt, "--" );
                snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_dns2", call_id );
                nv_set ( strRamOpt, "--" );
                snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_state", call_id );
                nv_set ( strRamOpt, "disconnect" );

                snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_ip", call_id );
                nv_set ( strRamOpt, "--" );
                snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_mask", call_id );
                nv_set ( strRamOpt, "--" );
                snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_gateway", call_id );
                nv_set ( strRamOpt, "--" );
                snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_dns1", call_id );
                nv_set ( strRamOpt, "--" );
                snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_dns2", call_id );
                nv_set ( strRamOpt, "--" );
                snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_ipv6_state", call_id );
                nv_set ( strRamOpt, "disconnect" );

                CLOGD ( FINE, "APN[%d] deact msg ...\n", call_id );
                apns_msg_flag [ call_id ] = 1;
                apnActOrDeactMsgEvent ( call_id, 0 );
                comd_wait_ms ( 1500 );
                snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_netdev", call_id );
                nv_set ( strRamOpt, "ccmni20" );

                system ( "killall -USR1 monitord" );
            }
        }
        break;
    default:
        break;
    }

    return;
}

#if USE_QUECTEL_USSD_API
static void ussd_response_cb ( ql_ussd_response_ind_t *result )
{
    FILE *fp = NULL;

    fp = fopen ( "/tmp/ussd_code.txt", "w+" );
    if ( NULL != fp )
    {
        fprintf ( fp, "%s",  result->ussd_payload );
        fclose ( fp );
        fp = NULL;
        nv_set ( "ussd_code_set", "0" );
        return ;
    }
    else
    {
        CLOGD ( WARNING, "open /tmp/ussd_code.txt failed !\n" );
    }
    nv_set ( "ussd_code_set", "1" );
}
#endif

static int datacall_init_done[4] = { 0, 0, 0, 0 };
int mtkParam_setupDataCall ( int apn_index, int del_flag )
{
    ql_data_call_param_t *p_param = NULL;
    int ret = 0;
    int org_call_id = ( apn_index + APN_PLUS );
    char call_name[16] = {0};
    char strUciOpt[64] = {0};
    char strUciVal[64] = {0};

    CLOGD ( FINE, "apn_index: [%d], org_call_id: [%d], del_flag: [%d]\n", apn_index, org_call_id, del_flag );

    if ( 0 == datacall_init_done[apn_index - 1] )
    {
        ret = ql_data_call_stop ( org_call_id );
        CLOGD ( FINE, "ql_data_call_stop [%d] ret: [%d]\n", org_call_id, ret );

        ret = ql_data_call_delete ( org_call_id );
        CLOGD ( FINE, "ql_data_call_delete [%d] ret: [%d]\n", org_call_id, ret );

        if ( 1 == del_flag )
        {
            datacall_init_done[apn_index - 1] = 1;
            return 0;
        }

        CLOGD ( FINE, "enter init data call ...\n" );

        snprintf ( call_name, sizeof ( call_name ), "APN%d", apn_index );

        CLOGD ( FINE, "create %s`s data call ...\n", call_name );

        ret = ql_data_call_create ( org_call_id, call_name, IS_BACKGROUND );
        CLOGD ( FINE, "ql_data_call_create, call_id=[%d], ret=[%d]\n", org_call_id, ret );
        if ( QL_ERR_OK != ret )
        {
            return -1;
        }

        p_param = ql_data_call_param_alloc();
        if ( NULL == p_param )
        {
            CLOGD ( ERROR, "ql_data_call_param_alloc fail, no memory ???\n" );
            return -1;
        }

        ret = ql_data_call_param_init ( p_param );
        CLOGD ( FINE, "ql_data_call_param_init ret: [%d]\n", ret );

        ret = ql_data_call_param_set_apn_id ( p_param, 1 );     // apn type : default 1
        CLOGD ( FINE, "ql_data_call_param_set_apn_id ret: [%d]\n", ret );

        snprintf ( strUciOpt, sizeof ( strUciOpt ), "apn%d_name", apn_index );
        memset ( strUciVal, 0, sizeof ( strUciVal ) );
        nv_get ( strUciOpt, strUciVal, sizeof ( strUciVal ) );
        CLOGD ( FINE, "%s -> [%s]\n", strUciOpt, strUciVal );
        if ( 0 == strcmp ( strUciVal, "" ) )
        {
            nv_get ( "auto_select_apn", strUciVal, sizeof ( strUciVal ) );
            CLOGD ( FINE, "auto_select_apn -> [%s]\n", strUciVal );
        }
        ret = ql_data_call_param_set_apn_name ( p_param, strUciVal );
        CLOGD ( FINE, "ql_data_call_param_set_apn_name [%s] ret: [%d]\n", strUciVal, ret );

        snprintf ( strUciOpt, sizeof ( strUciOpt ), "pdn%d_type", apn_index );
        memset ( strUciVal, 0, sizeof ( strUciVal ) );
        nv_get ( strUciOpt, strUciVal, sizeof ( strUciVal ) );
        if ( 0 == strcmp ( strUciVal, "0" ) )
            ret = ql_data_call_param_set_ip_version ( p_param, QL_NET_IP_VER_V4 );
        else if ( 0 == strcmp ( strUciVal, "1" ) )
            ret = ql_data_call_param_set_ip_version ( p_param, QL_NET_IP_VER_V6 );
        else
            ret = ql_data_call_param_set_ip_version ( p_param, QL_NET_IP_VER_V4V6 );
        CLOGD ( FINE, "ql_data_call_param_set_ip_version [%s] ret: [%d]\n", strUciVal, ret );

        snprintf ( strUciOpt, sizeof ( strUciOpt ), "auth_type%d", apn_index );
        memset ( strUciVal, 0, sizeof ( strUciVal ) );
        nv_get ( strUciOpt, strUciVal, sizeof ( strUciVal ) );
        if ( 0 == strcmp ( strUciVal, "2" ) )
            ret = ql_data_call_param_set_auth_pref ( p_param, QL_NET_AUTH_PREF_CHAP_ONLY_ALLOWED );
        else if ( 0 == strcmp ( strUciVal, "1" ) )
            ret = ql_data_call_param_set_auth_pref ( p_param, QL_NET_AUTH_PREF_PAP_ONLY_ALLOWED );
        else
            ret = ql_data_call_param_set_auth_pref ( p_param, QL_NET_AUTH_PREF_PAP_CHAP_NOT_ALLOWED );
        CLOGD ( FINE, "ql_data_call_param_set_auth_pref [%s] ret: [%d]\n", strUciVal, ret );

        snprintf ( strUciOpt, sizeof ( strUciOpt ), "user_name%d", apn_index );
        memset ( strUciVal, 0, sizeof ( strUciVal ) );
        nv_get ( strUciOpt, strUciVal, sizeof ( strUciVal ) );
        ret = ql_data_call_param_set_user_name ( p_param, strUciVal );
        CLOGD ( FINE, "ql_data_call_param_set_user_name [%s] ret: [%d]\n", strUciVal, ret );

        snprintf ( strUciOpt, sizeof ( strUciOpt ), "password%d", apn_index );
        memset ( strUciVal, 0, sizeof ( strUciVal ) );
        nv_get ( strUciOpt, strUciVal, sizeof ( strUciVal ) );
        ret = ql_data_call_param_set_user_password ( p_param, strUciVal );
        CLOGD ( FINE, "ql_data_call_param_set_user_password [%s] ret: [%d]\n", strUciVal, ret );

        ret = ql_data_call_param_set_reconnect_interval ( p_param, 15 );
        CLOGD ( FINE, "ql_data_call_param_set_reconnect_interval ret: [%d]\n", ret );

        ret = ql_data_call_set_config ( org_call_id, p_param );
        CLOGD ( FINE, "ql_data_call_config, call_id=[%d], ret=[%d]\n", org_call_id, ret );
        if ( QL_ERR_OK != ret )
        {
            if ( p_param )
            {
                ret = ql_data_call_param_free ( p_param );
                CLOGD ( FINE, "ql_data_call_param_free ret: [%d]\n", ret );
            }
            return -1;
        }

        ret = ql_data_call_start ( org_call_id );
        CLOGD ( FINE, "ql_data_call_start, call_id=[%d], ret=[%d]\n", org_call_id, ret );
        if ( QL_ERR_OK != ret )
        {
            if ( p_param )
            {
                ret = ql_data_call_param_free ( p_param );
                CLOGD ( FINE, "ql_data_call_param_free ret: [%d]\n", ret );
            }
            return -1;
        }

        if ( p_param )
        {
            ret = ql_data_call_param_free ( p_param );
            CLOGD ( FINE, "ql_data_call_param_free ret: [%d]\n", ret );
        }

        datacall_init_done[apn_index - 1] = 1;
    }

    return 0;
}

int mtkParam_initDataCallBk ()
{
    int ret = 0;

    if ( -1 == datacall_init_ret )
    {
        ret = ql_data_call_init ( DC_IPC_MODE_DEFAULT );
        CLOGD ( FINE, "ql_data_call_init, ret: [%d]\n", ret );
        if ( QL_ERR_OK != ret )
        {
            return -1;
        }
        datacall_init_ret = -2;
    }

    if ( -2 == datacall_init_ret )
    {
        ret = ql_data_call_set_status_ind_cb ( data_call_status_ind_cb );
        CLOGD ( FINE, "ql_data_call_set_status_ind_cb, ret: [%d]\n", ret );
        if ( QL_ERR_OK != ret )
        {
            return -2;
        }
#if USE_QUECTEL_VOICE_API
        datacall_init_ret = -3;
    }

    if ( -3 == datacall_init_ret )
    {
        ret = ql_voice_init ();
        CLOGD ( FINE, "ql_voice_init, ret: [%d]\n", ret );
        if ( QL_VOICE_SUCCESS != ret )
        {
            return -3;
        }
        datacall_init_ret = -4;
    }

    if ( -4 == datacall_init_ret )
    {
        ret = ql_ims_status_cb ( ims_reg_status_info_cb );
        CLOGD ( FINE, "ql_ims_status_cb, ret: [%d]\n", ret );
        if ( QL_VOICE_SUCCESS != ret )
        {
            return -4;
        }
#endif
#if USE_QUECTEL_USSD_API
        datacall_init_ret = -5;
    }

    if ( -5 == datacall_init_ret )
    {
        ret = ql_sms_init ( SMS_IPC_MODE_DEFAULT );
        CLOGD ( FINE, "ql_sms_init, ret: [%d]\n", ret );
        if ( QL_SMS_SUCCESS != ret )
        {
            return -5;
        }
        datacall_init_ret = -6;
    }

    if ( -6 == datacall_init_ret )
    {
        ret = ql_ussd_response_cb ( ussd_response_cb );
        CLOGD ( FINE, "ql_ussd_response_cb, ret: [%d]\n", ret );
        if ( QL_SMS_SUCCESS != ret )
        {
            return -6;
        }
#endif
        datacall_init_ret = -7;
    }

    return 0;
}

void mtkParam_deInitDataCall ( int deInitType )
{
    int ret = 0;
    int org_call_id = ( 1 + APN_PLUS );

    for ( ; org_call_id <= ( 4 + APN_PLUS ); org_call_id++ )
    {
        ret = ql_data_call_stop ( org_call_id );
        CLOGD ( FINE, "ql_data_call_stop [%d] ret: [%d]\n", org_call_id, ret );

        ret = ql_data_call_delete ( org_call_id );
        CLOGD ( FINE, "ql_data_call_delete [%d] ret: [%d]\n", org_call_id, ret );

        datacall_init_done[org_call_id - 1] = 0;
    }

    if ( 1 == deInitType )
    {
        return;
    }

    ret = ql_data_call_deinit();
    CLOGD ( FINE, "ql_data_call_deinit, ret: [%d]\n", ret );

#if USE_QUECTEL_VOICE_API
    ret = ql_voice_deinit ();
    CLOGD ( FINE, "ql_voice_deinit, ret: [%d]\n", ret );
#endif
#if USE_QUECTEL_USSD_API
    ret = ql_sms_release ();
    CLOGD ( FINE, "ql_sms_release, ret: [%d]\n", ret );
#endif
    ret = ql_nw_set_cscon_ind_cb( NULL );
    CLOGD ( FINE, "ql_nw_set_cscon_ind_cb unsubscribe, ret: [%d]\n", ret );

}
#else
void mtkParam_deInitDataCall ( int deInitType )
{
    return;
}
#endif

static void mtkParam_radioRestart ()
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+CFUN=0", 10000, at_rep, sizeof ( at_rep ) );

    comd_wait_ms ( 1500 );

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( "AT+CFUN=1", 10000, at_rep, sizeof ( at_rep ) );
}

int mtkParam_setATEcho ( char* data )
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "ATE0", 3000, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
        return -1;

    return 0;
}

int mtkParam_setIpv6Format ( char* data )
{
    char at_rep[64] = {0};
    COMD_AT_PROCESS ( "AT+CGPIAF=1,1,0,1", 10000, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
        return -1;

    return 0;
}

int  mtkParam_updateUsbmode ( char* data )
{

    return 0;
}

int mtkParam_SetIms ( char* data )
{
    char at_rep[64] = {0};
    char at_cmd[64] = {0};

    if ( 0 == strcmp ( volte_enable, "VOLTE" ) )
    {
        snprintf ( at_cmd, sizeof ( at_cmd ), "%s", "AT+EIMSCFG=1,0,0,0,1,1" );//enable IMS
    }
    else
    {
        snprintf ( at_cmd, sizeof ( at_cmd ), "%s", "AT+EIMSCFG=1,0,0,0,1,0" );//disable IMS
    }

    COMD_AT_PROCESS ( at_cmd, 3000, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
        return -1;

    return 0;
}

int mtkParam_radioLockInit ( char* data )
{
    char uciScanMode[16] = {0};
    char uciLocked5gBand[128] = {0};
    char uciLocked4gBand[128] = {0};
    char uciLockedBand[256] = {0};
    char uciLockedFreq[256] = {0};
    char uciLockedPci[256] = {0};

    sys_get_config ( WAN_LTE_LOCK_MODE, uciScanMode, sizeof ( uciScanMode ) );

    if ( 0 == strcmp ( uciScanMode, "BAND_LOCK" ) )
    {
        sys_get_config ( WAN_LTE_LOCK_BAND, uciLocked4gBand, sizeof ( uciLocked4gBand ) );

        if ( 0 != strcmp ( uciLocked4gBand, "" ) )
        {
            // "42 43" -> "42_43_"
            comd_strrpl ( uciLocked4gBand, " ", "_" );
            strcat ( uciLocked4gBand, "_" );
        }

        sys_get_config ( WAN_LTE_LOCK_BAND5G, uciLocked5gBand, sizeof ( uciLocked5gBand ) );

        if ( 0 != strcmp ( uciLocked5gBand, "" ) )
        {
            // "42 43" -> "42_43_"
            comd_strrpl ( uciLocked5gBand, " ", "_" );
            strcat ( uciLocked5gBand, "_" );
        }

        snprintf ( uciLockedBand, sizeof ( uciLockedBand ), "%s,%s", uciLocked4gBand, uciLocked5gBand );

        mtkParam_lockband ( uciLockedBand, 0 );
    }
    else if ( 0 == strcmp ( uciScanMode, "FREQ_LOCK" ) )
    {
        sys_get_config ( WAN_LTE_LOCK_FREQ, uciLockedFreq, sizeof ( uciLockedFreq ) );

        if ( 0 != strcmp ( uciLockedFreq, "" ) )
        {
            // "42001 42003" -> "42001_42003_"
            //comd_strrpl ( uciLockedFreq, " ", "_" );
            //strcat ( uciLockedFreq, "_" );

            mtkParam_lockearfcn ( uciLockedFreq, 0 );
        }
    }
    else if ( 0 == strcmp ( uciScanMode, "PCI_LOCK" ) )
    {
        sys_get_config ( WAN_LTE_LOCK_PCI, uciLockedPci, sizeof ( uciLockedPci ) );

        if ( 0 != strcmp ( uciLockedPci, "" ) )
        {
            // "42005,3 43012,1" -> "42005,3_43012,1_"
            //comd_strrpl ( uciLockedPci, " ", "_" );
            //strcat ( uciLockedPci, "_" );

            mtkParam_lockpci ( uciLockedPci, 0 );
        }
    }
    else if ( NULL == data )
    {
        mtkParam_lockband ( "", 0 );
    }
    else if ( 0 == strcmp ( data, "restore_fullband" ) )
    {
        mtkParam_lockband ( "", 1 );
    }

    return 0;
}

int mtkParam_updateImei ( char* data )
{
    unsigned int check_times = 0;
    char imei_val[32] = {0};
    char at_rep[STR_AT_RSP_LEN] = {0};

    while ( check_times++ < 5 )
    {
        if ( 0 == COMD_AT_PROCESS ( "AT+EGMR=0,7", 1500, at_rep, sizeof ( at_rep ) ) )
        {
            parsing_mtk_imei ( at_rep );
            nv_get ( "imei", imei_val, sizeof ( imei_val ) );
            if ( 15 <= strlen ( imei_val ) )
                return 0;
            memset ( imei_val, 0, sizeof ( imei_val ) );
        }
        comd_wait_ms ( 1000 );
        memset ( at_rep, 0, sizeof ( at_rep ) );
    }

    return 0; // do not return -1, even can not get imei
}

int mtkParam_updateSN ( char* data )
{
    unsigned int check_times = 0;
    char sn_val[32] = {0};
    char at_rep[STR_AT_RSP_LEN] = {0};

    while ( check_times++ < 5 )
    {
        if (0 == COMD_AT_PROCESS ( "AT+EGMR=0,5", 1500, at_rep, sizeof ( at_rep ) ) )
        {
            parsing_mtk_sn ( at_rep );
            nv_get ( "SN", sn_val, sizeof ( sn_val ) );
            if ( strcmp ( sn_val, "" ) )
                return 0;
            memset ( sn_val, 0, sizeof ( sn_val ) );
        }
        comd_wait_ms ( 1000 );
        memset ( at_rep, 0, sizeof ( at_rep ) );
    }

    return 0; // do not return -1, even can not get sn
}

static int mtkParam_updateUnlockCode ( char* data )
{
    char board_sn[32] = {0};
    char module_imei[32] = {0};
    char strRamOpt[16] = {0};
    int sn_last_val[UNLOCK_CODE_LEN] = {0};
    int imei_last_val[UNLOCK_CODE_LEN] = {0};
    int i = 0, j = 0, k = 0;
    int sn_len = 0, imei_len = 0;

    comd_exec ( "/lib/factory_tool.sh show | grep ^BOARD_SN | cut -d '=' -f 2 | tr -d '\r\n'",
                board_sn, sizeof ( board_sn ) );
    sn_len = strlen ( board_sn );
    CLOGD ( FINE, "board_sn: [%s], sn_len: [%d]\n", board_sn, sn_len );

    nv_get ( "imei", module_imei, sizeof ( module_imei ) );
    imei_len = strlen ( module_imei );
    CLOGD ( FINE, "module_imei: [%s], imei_len: [%d]\n", module_imei, imei_len );

    for ( k = 0; k < UNLOCK_CODE_LEN && k < sn_len && k < imei_len; k++ )
    {
        if ( board_sn[sn_len - 1 - k] < '0' || '9' < board_sn[sn_len - 1 - k] ||
             module_imei[imei_len - 1 - k] < '0' || '9' < module_imei[imei_len - 1 - k] )
        {
            continue;
        }
        sn_last_val[UNLOCK_CODE_LEN - 1 - k] = ( board_sn[sn_len - 1 - k] - '0' );
        imei_last_val[UNLOCK_CODE_LEN - 1 - k] = ( module_imei[imei_len - 1 - k] - '0' );
    }

    for ( i = 0; i < UNLOCK_CODE_NUM; i++ )
    {
        memset ( unlock_code[i], 0, sizeof ( unlock_code[i] ) );
        for ( j = 0; j < UNLOCK_CODE_LEN; j++ )
        {
            unlock_code[i][j] = ( sn_last_val[j] + i + 1 ) * imei_last_val[j] % 10 + '0';
        }
        snprintf ( strRamOpt, sizeof ( strRamOpt ), "unlock_code%d", i + 1 );
        nv_set ( strRamOpt, unlock_code[i] );
        CLOGD ( FINE, "%s: [%s]\n", strRamOpt, unlock_code[i] );
    }

    return 0;
}

static int mtkParam_initZteCellLock ( char *data )
{
#if defined (CONFIG_SW_STC_CELLLOCK)
    char at_rep[64] = {0};

    if ( 0 == strcmp ( celllock_enable, "0" ) )
    {
        //COMD_AT_PROCESS ( "AT+ZCWLC=0", 1500, at_rep, sizeof ( at_rep ) );
    }
    else
    {
        //COMD_AT_PROCESS ( "AT+ZCWLC=1", 1500, at_rep, sizeof ( at_rep ) );
    }
#endif

    return 0;
}

#if defined (CONFIG_SW_STC_CELLLOCK)
static int mtkParam_updateCellLock ( char *data )
{
    char at_rep[64] = {0};
#if 0
    COMD_AT_PROCESS ( "AT+ZCELLAVAILABLE?", 1500, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "+ZCELLAVAILABLE: 0" ) )
    {
        nv_set ( "celllocked", "1" );
        return 1;
    }
#endif
    nv_set ( "celllocked", "0" );
    return 0;
}
#endif

int mtkParam_updateModuleModel ( char* data )
{
    char module_type[32] = {0};
    char at_rep[STR_AT_RSP_LEN] = {0};

    if ( 0 == access ( "/lib/update_modulemodel.sh", F_OK ) )
    {
        system ( "/lib/update_modulemodel.sh" );

        nv_get ( "modulemodel", module_type, sizeof ( module_type ) );
        if ( 0 != strcmp ( module_type, "" ) )
        {
            return 0;
        }
    }

    COMD_AT_PROCESS ( "AT+CGMM", 1500, at_rep, sizeof ( at_rep ) );

    return parsing_mtk_moduleModel ( at_rep );
}

int mtkParam_updateModuleVersion ( char* data )
{
    char at_rep[STR_AT_RSP_LEN] = {0};

    COMD_AT_PROCESS ( "AT+CGMR", 1500, at_rep, sizeof ( at_rep ) );

    parsing_mtk_moduleVersion ( at_rep );

#if defined(MTK_T750) && CONFIG_QUECTEL_T750
#else
    system("ilte_mon getBypassVersion");
#endif

    return 0;
}

int mtkParam_updateSuppBands ( char* data )
{
    char module_type[32] = {0};
    char lte_bands[256] = {0};
    char nr5g_bands[256] = {0};

    char old_suppband[128] = {0};
    char old_suppband5g[128] = {0};

    nv_get ( "modulemodel", module_type, sizeof ( module_type ) );

    mtkParam_getSupportBand();

    nv_get ( "band_4g_list", lte_bands, sizeof ( lte_bands ) );
    nv_get ( "band_5g_list", nr5g_bands, sizeof ( nr5g_bands ) );

    if( 0 == strcmp ( lte_bands, "" ) )
    {
        CLOGD ( ERROR, "Api gets band list failed! Using default band list\n" );
        snprintf ( lte_bands, sizeof ( lte_bands ), "%s",
                                strstr ( module_type, "RG500L-EU" ) ? QUECTEL_RG500L_EU_4G_BANDS : (
                                strstr ( module_type, "RG500L-NA" ) ? QUECTEL_RG500L_NA_4G_BANDS : (
                                strstr ( module_type, "RG600L-EU" ) ? QUECTEL_RG600L_EU_4G_BANDS : (
                                strstr ( module_type, "RG620T-EU" ) ? QUECTEL_RG620T_EU_4G_BANDS : (
                                strstr ( module_type, "RG620T-NA" ) ? QUECTEL_RG620T_NA_4G_BANDS : (
                                strstr ( module_type, "EM160R-GL" ) ? QUECTEL_EM160R_GL_4G_BANDS : ""
                            ) ) ) ) )
                        );
    }

    if ( 0 == strcmp ( nr5g_bands, "" ) )
    {
        snprintf ( nr5g_bands, sizeof ( nr5g_bands ), "%s",
                                strstr ( module_type, "RG500L-EU" ) ? QUECTEL_RG500L_EU_5G_BANDS : (
                                strstr ( module_type, "RG500L-NA" ) ? QUECTEL_RG500L_NA_5G_BANDS : (
                                strstr ( module_type, "RG600L-EU" ) ? QUECTEL_RG600L_EU_5G_BANDS : (
                                strstr ( module_type, "RG620T-EU" ) ? QUECTEL_RG620T_EU_5G_BANDS : (
                                strstr ( module_type, "RG620T-NA" ) ? QUECTEL_RG620T_NA_5G_BANDS : (
                                strstr ( module_type, "EM160R-GL" ) ? QUECTEL_EM160R_GL_5G_BANDS : ""
                            ) ) ) ) )
                        );
    }

    if ( strcmp ( lte_bands, "" ) || strcmp ( nr5g_bands, "" ) )
    {
        CLOGD ( FINE, "4G_BANDS_ORG -> [%s]\n", lte_bands );
        nv_set ( "suppband_org", lte_bands );
        comd_strrpl ( lte_bands, ":", " " );
        CLOGD ( FINE, "4G_SUPPBAND  -> [%s]\n", lte_bands );
        nv_set ( "suppband", lte_bands );

        CLOGD ( FINE, "5G_BANDS_ORG -> [%s]\n", nr5g_bands );
        nv_set ( "suppband5g_org", nr5g_bands );
        comd_strrpl ( nr5g_bands, ":", " " );
        CLOGD ( FINE, "5G_SUPPBAND  -> [%s]\n", nr5g_bands );
        nv_set ( "suppband5g", nr5g_bands );

        sys_get_config ( LTE_PARAMETER_SUPPORT_BAND, old_suppband, sizeof ( old_suppband ) );
        sys_get_config ( LTE_PARAMETER_SUPPORT_BAND5G, old_suppband5g, sizeof ( old_suppband5g ) );
        if ( strcmp ( old_suppband, lte_bands ) )
        {
            CLOGD ( WARNING, "Save supported bands to flash !\n" );
            sys_set_config ( LTE_PARAMETER_SUPPORT_BAND, lte_bands );
            sys_commit_config ( "lte_param" );
        }
        if ( strcmp ( old_suppband5g, nr5g_bands ) )
        {
            CLOGD ( WARNING, "Save supported bands to flash !\n" );
            sys_set_config ( LTE_PARAMETER_SUPPORT_BAND5G, nr5g_bands );
            sys_commit_config ( "lte_param" );
        }

        return 0;
    }

    CLOGD ( ERROR, "No support band list for [%s]\n", module_type );

    return -1;
}

int mtkParam_updateRrcState ()
{
    char at_rep[RECV_BUF_SIZE] = {0};

    COMD_AT_PROCESS ( "AT+KRRCSTATE?", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "+KRRCSTATE:" ) )
        parsing_mtk_rrc_state ( at_rep );

    return 0;
}

int mtkParam_updateCGDCONT ( char* data )
{
    char at_rep[RECV_BUF_SIZE * 2] = {0};

    COMD_AT_PROCESS ( "AT+CGDCONT?", 3000, at_rep, sizeof ( at_rep ) );

    if ( NULL == data && strstr ( at_rep, "\r\n+CGDCONT:" ) )
    {
        return parsing_mtk_apn ( at_rep );
    }

    return -1;
}

int mtkParam_initAPNsetting ( char* data )
{
    char uci_apnenable[4] = {0};
    char uci_pdptype[8] = {0};
    char uci_apnname[64] = {0};
    char uci_profilename[64] = {0};
    char uci_authtype[8] = {0};
    char uci_username[64] = {0};
    char uci_password[64] = {0};
    char strUciOpt[128] = {0};
    char strRamOpt[128] = {0};
#if defined(MTK_T750) && CONFIG_QUECTEL_T750
#else
    char at_rep[128] = {0};
    char at_cmd[128] = {0};
    char eth_mac[18]={0};
#endif
    int i = 0;
    int apnset_start_index = 1;
    int apnset_end_index = 4;

    if ( data )
    {
        apn_profile* apn_settingdata = ( apn_profile* ) data;
        apnset_start_index = apn_settingdata->index + 1;
        apnset_end_index = apn_settingdata->index + 1;

        snprintf ( uci_profilename, sizeof ( uci_profilename ), "%s", apn_settingdata->profile_name );
        CLOGD ( FINE, "apn_msg_profilename -> [%s]\n", uci_profilename );

        if ( 0 == strcmp ( apn_settingdata->pdn_type, "0" ) )
        {
            strcpy ( uci_pdptype, "IP" );
        }
        else if ( 0 == strcmp ( apn_settingdata->pdn_type, "1" ) )
        {
            strcpy ( uci_pdptype, "IPV6" );
        }
        else
        {
            strcpy ( uci_pdptype, "IPV4V6" );
        }
        CLOGD ( FINE, "apn_msg_pdptype -> [%s]\n", uci_pdptype );

        snprintf ( uci_apnname, sizeof ( uci_apnname ), "%s", apn_settingdata->apn_name );
        CLOGD ( FINE, "apn_msg_apnname -> [%s]\n", uci_apnname );

        snprintf ( uci_apnenable, sizeof ( uci_apnenable ), "%s", apn_settingdata->apn_enable );
        CLOGD ( FINE, "apn_msg_enabled -> [%s]\n", uci_apnenable );

        snprintf ( uci_authtype, sizeof ( uci_authtype ), "%s", apn_settingdata->auth_type );
        CLOGD ( FINE, "apn_msg_authtype -> [%s]\n", uci_authtype );

        snprintf ( uci_username, sizeof ( uci_username ), "%s", apn_settingdata->user_name );
        CLOGD ( FINE, "apn_msg_username -> [%s]\n", uci_username );

        snprintf ( uci_password, sizeof ( uci_password ), "%s", apn_settingdata->password );
        CLOGD ( FINE, "apn_msg_password -> [%s]\n", uci_password );

        nv_set ( "apn_set", "0" );
    }

    for ( i = apnset_start_index; i <= apnset_end_index; i++ )
    {
        if ( apnset_start_index < apnset_end_index )
        {
            snprintf ( strUciOpt, sizeof ( strUciOpt ), LTE_APN_PROFILE_NAME, i );
            sys_get_config ( strUciOpt, uci_profilename, sizeof ( uci_profilename ) );
            CLOGD ( FINE, "%s -> [%s]\n", strUciOpt, uci_profilename );
        }

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "profile_name%d", i );
        nv_set ( strRamOpt, uci_profilename );

        if ( apnset_start_index < apnset_end_index )
        {
            snprintf ( strUciOpt, sizeof ( strUciOpt ), LTE_APN_PDPTYPE, i );
            sys_get_config ( strUciOpt, uci_pdptype, sizeof ( uci_pdptype ) );
            CLOGD ( FINE, "%s -> [%s]\n", strUciOpt, uci_pdptype );
        }

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "pdn%d_type", i );
        nv_set ( strRamOpt, strcmp ( uci_pdptype, "IP" ) ? ( strcmp ( uci_pdptype, "IPV6" ) ? "2" : "1" ) : "0" );

        if ( apnset_start_index < apnset_end_index )
        {
            snprintf ( strUciOpt, sizeof ( strUciOpt ), LTE_APN_APNNAME, i );
            sys_get_config ( strUciOpt, uci_apnname, sizeof ( uci_apnname ) );
            CLOGD ( FINE, "%s -> [%s]\n", strUciOpt, uci_apnname );
        }

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_name", i );
        nv_set ( strRamOpt, uci_apnname );

        if ( apnset_start_index < apnset_end_index )
        {
            snprintf ( strUciOpt, sizeof ( strUciOpt ), LTE_APN_ENABLE, i );
            sys_get_config ( strUciOpt, uci_apnenable, sizeof ( uci_apnenable ) );
            CLOGD ( FINE, "%s -> [%s]\n", strUciOpt, uci_apnenable );
        }

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "apn%d_enable", i );
        nv_set ( strRamOpt, uci_apnenable );

        if ( apnset_start_index < apnset_end_index )
        {
            snprintf ( strUciOpt, sizeof ( strUciOpt ), LTE_APN_AUTH_TYPE, i );
            sys_get_config ( strUciOpt, uci_authtype, sizeof ( uci_authtype ) );
            CLOGD ( FINE, "%s -> [%s]\n", strUciOpt, uci_authtype );
            strcpy ( uci_authtype, strcmp ( uci_authtype, "CHAP" ) ? ( strcmp ( uci_authtype, "PAP" ) ? "0" : "1" ) : "2" );
        }

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "auth_type%d", i );
        nv_set ( strRamOpt, uci_authtype );

        if ( apnset_start_index < apnset_end_index )
        {
            snprintf ( strUciOpt, sizeof ( strUciOpt ), LTE_APN_USERNAME, i );
            sys_get_config ( strUciOpt, uci_username, sizeof ( uci_username ) );
            CLOGD ( FINE, "%s -> [%s]\n", strUciOpt, uci_username );
        }

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "user_name%d", i );
        nv_set ( strRamOpt, uci_username );

        if ( apnset_start_index < apnset_end_index )
        {
            snprintf ( strUciOpt, sizeof ( strUciOpt ), LTE_APN_PASSWORD, i );
            sys_get_config ( strUciOpt, uci_password, sizeof ( uci_password ) );
            CLOGD ( FINE, "%s -> [%s]\n", strUciOpt, uci_password );
        }

        snprintf ( strRamOpt, sizeof ( strRamOpt ), "password%d", i );
        nv_set ( strRamOpt, uci_password );

#if defined(MTK_T750) && CONFIG_QUECTEL_T750
        if ( 1 == i )
        {
            mtkParam_initIAAPN ( uci_apnname, uci_pdptype, uci_authtype, uci_username, uci_password );
        }
        datacall_init_done [ i - 1 ] = 0;
#else
        mtkParam_configCgact ( i, "0" );
        memset ( at_rep, 0, sizeof ( at_rep ) );
        snprintf ( strRamOpt, sizeof ( strRamOpt ), "eth1.10%d_mac", i-1 );

        memset ( eth_mac , 0, sizeof ( eth_mac ) );
        nv_get ( strRamOpt, eth_mac, sizeof ( eth_mac ) );
        if ( strlen ( eth_mac ) )
        {
            snprintf ( at_cmd, sizeof ( at_cmd ), "AT+KAPNCFG=\"WANMAC\",%d,1,\"%s\"",i,eth_mac );
        }
        else
        {
            snprintf ( at_cmd, sizeof ( at_cmd ), "AT+KAPNCFG=\"WANMAC\",%d,0",i );
        }
        COMD_AT_PROCESS ( at_cmd, 3000, at_rep, sizeof ( at_rep ) );

        memset ( at_rep, 0, sizeof ( at_rep ) );

        snprintf ( at_cmd, sizeof ( at_cmd ), "AT+KAPNCFG=\"MPDN\",%d,1,%s,%s,%s,%s,%s",
                        i,
                        uci_apnname,
                        strcmp ( uci_pdptype, "IP" ) ? ( strcmp ( uci_pdptype, "IPV6" ) ? "3" : "2" ) : "1",
                        uci_authtype,
                        uci_username,
                        uci_password );

        COMD_AT_PROCESS ( at_cmd, 3000, at_rep, sizeof ( at_rep ) );

        mtkParam_configCgact ( i, uci_apnenable );
#endif
    }

    return 0;
}

int mtkParam_updateDualSim ()
{
    char cur_sim_mode[4] = {0};
    char uci_sim_mode[4] = {0};
    char ram_val[16] = {0};
    char at_req[16] = {0};
    char at_rep[128] = {0};
    int i = 0;
    static int autosim_have_checked = 0;

    COMD_AT_PROCESS ( "AT+QUIMSLOT?", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "\r\n+QUIMSLOT: 1" ) )
    {
        strcpy ( cur_sim_mode, "0" );
        nv_set ( "sim_slot_used", "0" );
    }
    else if ( strstr ( at_rep, "\r\n+QUIMSLOT: 2" ) )
    {
        strcpy ( cur_sim_mode, "1" );
        nv_set ( "sim_slot_used", "1" );
    }
    else
    {
        return 0;
    }

    sys_get_config ( "lte_param.parameter.simslot", uci_sim_mode, sizeof ( uci_sim_mode ) );
    CLOGD ( FINE, "sim_slot uci value -> [%s]\n", uci_sim_mode );

    if ( 0 == strcmp ( uci_sim_mode, "2" ) )
    {
        if ( 1 == autosim_have_checked  )
        {
            return 0;
        }

        autosim_have_checked = 1;

        while ( i++ < 3 )
        {
            nv_get ( "cpin", ram_val, sizeof ( ram_val ) );
            if ( strcmp ( ram_val, "ERROR" ) )
            {
                return 0;
            }
            comd_wait_ms ( 3000 );
            mtkParam_updatePinStatus ();
        }

        snprintf ( at_req, sizeof ( at_req ), "AT+QUIMSLOT=%s",
                                                    strcmp ( cur_sim_mode, "0" ) ? "1" : "2" );
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( at_req, 5000, at_rep, sizeof ( at_rep ) );

        if ( strstr ( at_rep, "\r\nOK\r\n" ) )
            return 1;
    }
    else
    {
        autosim_have_checked = 0;

        if ( strcmp ( uci_sim_mode, cur_sim_mode ) )
        {
            snprintf ( at_req, sizeof ( at_req ), "AT+QUIMSLOT=%s",
                                                        strcmp ( cur_sim_mode, "0" ) ? "1" : "2" );
            memset ( at_rep, 0, sizeof ( at_rep ) );
            COMD_AT_PROCESS ( at_req, 5000, at_rep, sizeof ( at_rep ) );

            if ( strstr ( at_rep, "\r\nOK\r\n" ) )
                return 1;
        }
    }

    return 0;
}

void mtkParam_updatePinStatus ()
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+CPIN?", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "+CPIN:" ) || strstr ( at_rep, "ERROR" ) )
        parsing_mtk_cpin_get ( at_rep );
}

void mtkParam_updatePinLockStatus ()
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+CLCK=\"SC\",2", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "+CLCK:" ) )
        parsing_mtk_clck_get ( at_rep );
}

void mtkParam_updatePinPukCount ()
{
    char at_rep[256] = {0};

    COMD_AT_PROCESS ( "AT+QPINC?", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "+QPINC:" ) )
        parsing_mtk_qpinc ( at_rep );
}

void mtkParam_updateImsi ()
{
    char at_rep[64] = {0};
    char ram_val[16] = {0};

    nv_get ( "imsi", ram_val, sizeof ( ram_val ) );

    if ( 0 == strcmp ( ram_val, "" ) || 0 == strcmp ( ram_val, "--" ) )
    {
        COMD_AT_PROCESS ( "AT+CIMI?", 3000, at_rep, sizeof ( at_rep ) );

        if ( strstr ( at_rep, "+CIMI:" ) )
            parsing_mtk_cimi ( at_rep );

        mtkParam_updateUsimMncLen ();
        mtkParam_setIpv6Format ( NULL );

#if defined(MTK_T750) && CONFIG_QUECTEL_T750
        system ( "/lib/t750_auto_select_apn.sh" );
#endif
    }
}

void mtkParam_updateUsimMncLen ()
{
    char at_rep[128] = {0};

    COMD_AT_PROCESS ( "AT+CRSM=176,28589,0,0,4", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "+CRSM:" ) )
        parsing_mtk_simMncLen ( at_rep );
}

void mtkParam_updateSimSPN ()
{
    char at_rep[256] = {0};
    char ram_val[64] = {0};

    nv_get ( "SIM_SPN", ram_val, sizeof ( ram_val ) );

    if ( 0 == strcmp ( ram_val, "" ) || 0 == strcmp ( ram_val, "--" ) )
    {
        COMD_AT_PROCESS ( "AT+CRSM=176,28486,0,0,17", 3000, at_rep, sizeof ( at_rep ) );

        if ( strstr ( at_rep, "+CRSM:" ) )
            parsing_mtk_sim_spn ( at_rep );
    }
}

void mtkParam_updateIccid ()
{
    char at_rep[64] = {0};
    char ram_val[32] = {0};

    nv_get ( "iccid", ram_val, sizeof ( ram_val ) );

    if ( 0 == strcmp ( ram_val, "" ) || 0 == strcmp ( ram_val, "--" ) )
    {
        COMD_AT_PROCESS ( "AT+ICCID?", 3000, at_rep, sizeof ( at_rep ) );

        if ( strstr ( at_rep, "+ICCID:" ) )
            parsing_mtk_iccid ( at_rep );
    }
}

void mtkParam_updateCereg ()
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+CEREG?", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "\r\n+CEREG:" ) )
        parsing_mtk_cereg ( at_rep );
}

void mtkParam_updateC5greg ()
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+C5GREG?", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "\r\n+C5GREG:" ) )
        parsing_mtk_c5greg (at_rep);
}

void mtkParam_updateCireg ()
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+CIREG?", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "+CIREG:" ) )
        parsing_mtk_cireg ( at_rep );
}

void mtkParam_updateCesq ()
{
    char at_rep[128] = {0};

    COMD_AT_PROCESS ( "AT+CESQ", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "+CESQ:" ) )
        parsing_mtk_cesq ( at_rep );
}

void mtkParam_updateCgatt ()
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+CGATT?", 3000, at_rep, sizeof ( at_rep ) );

    if ( strcmp ( at_rep, "" ) )
        parsing_mtk_cgatt ( at_rep );
}

void mtkParam_updateCgact ()
{
    char at_rep[STR_AT_RSP_LEN] = {0};

    COMD_AT_PROCESS ( "AT+CGACT?", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "\r\nOK\r\n" ) )
        parsing_mtk_cgact ( at_rep );
}

int mtkParam_configCgact ( int cid_index, char* act_val )
{
    char at_req[32] = {0};
    char at_rep[128] = {0};
    char strRamOpt[64] = {0};

    snprintf ( at_req, sizeof ( at_req ), "AT+KAPNCFG=\"connect\",%d,%s", cid_index, act_val );
    COMD_AT_PROCESS ( at_req, 3000, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
        return -1;

    snprintf ( strRamOpt, sizeof ( strRamOpt ), "cid_%d_state", cid_index );
    nv_set ( strRamOpt, act_val );

    return 0;
}

void mtkParam_updateCops ()
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+COPS?", 3000, at_rep, sizeof ( at_rep ) );

    if ( strcmp ( at_rep, "" ) )
        parsing_mtk_operator ( at_rep );
}

void mtkParam_updateKcops ()
{
#if defined(MTK_T750) && CONFIG_QUECTEL_T750
        return;
#endif

    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+KCOPS?", 3000, at_rep, sizeof ( at_rep ) );

    if ( strcmp ( at_rep, "" ) )
        parsing_mtk_ktoperator ( at_rep );
}

int mtkParam_updateIPV4V6 ( int apn_index )
{
    char at_rep[RECV_BUF_SIZE] = {0};
    char at_cmd[32] = {0};

    snprintf ( at_cmd, sizeof ( at_cmd ), "AT+KAPNCFG=\"WWAN\",%d", apn_index );

    COMD_AT_PROCESS ( at_cmd, 3000, at_rep, sizeof ( at_rep ) );

    return strstr ( at_rep, "+KAPNCFG:" ) ? parsing_mtk_ipv4v6 ( apn_index, at_rep ) : 0;
}

void mtkParam_updatePccSccInfo ( char* data )
{
    char at_rep[RECV_BUF_SIZE * 2] = {0};

    COMD_AT_PROCESS ( "AT+EDMFAPP=6,4", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "+EDMFAPP:" ) )
        parsing_mtk_edmfapp_six_four ( at_rep );

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( "AT+EDMFAPP=6,10", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "+EDMFAPP:" ) )
        parsing_mtk_edmfapp_six_ten ( at_rep );
}

void mtkParam_updateServNeighInfo ( char* data )
{
#if defined(MTK_T750) && CONFIG_QUECTEL_T750
    int i = 0;
    char tmp_val[32] = {0};
    char tmp_str[64] = {0};

    ql_nw_init ( NW_IPC_MODE_DEFAULT );

#if 1    /* step [1]: update operator */
    ql_nw_mobile_operator_name_info_t p_name_info;
    memset ( ( void * ) &p_name_info, 0, sizeof ( ql_nw_mobile_operator_name_info_t ) );
    ql_nw_get_mobile_operator_name ( &p_name_info );

    CLOGD ( FINE, "long_eons  = [%s]\n", p_name_info.long_eons );
    nv_set ( "operator", p_name_info.long_eons );

    CLOGD ( FINE, "short_eons = [%s]\n", p_name_info.short_eons );

    CLOGD ( FINE, "mcc = [%s]\n", p_name_info.mcc );
    nv_set ( "mcc", p_name_info.mcc );
    nv_set ( "5g_mcc", p_name_info.mcc );

    CLOGD ( FINE, "mnc = [%s]\n", p_name_info.mnc );
    nv_set ( "mnc", p_name_info.mnc );
    nv_set ( "5g_mnc", p_name_info.mnc );
#endif

#if 2    /* step [2]: update data & voice reg status */
    ql_nw_reg_status_info_t p_reg_info;

    memset ( ( void * ) &p_reg_info, 0, sizeof ( ql_nw_reg_status_info_t ) );
    if ( QL_NW_SUCCESS == ql_nw_get_data_reg_status ( &p_reg_info ) )
    {
        CLOGD ( FINE, "print data reg status:\n" );
        print_reg_status_info ( &p_reg_info );
    }
    else
    {
        CLOGD ( WARNING, "ql_nw_get_data_reg_status FAIL !!!\n" );
    }

    memset ( ( void * ) &p_reg_info, 0, sizeof ( ql_nw_reg_status_info_t ) );
    if ( QL_NW_SUCCESS == ql_nw_get_voice_reg_status ( &p_reg_info ) )
    {
        CLOGD ( FINE, "print voice reg status:\n" );
        print_reg_status_info ( &p_reg_info );
    }
    else
    {
        CLOGD ( WARNING, "ql_nw_get_voice_reg_status FAIL !!!\n" );
    }
#endif

#if 3    /* step [3]: update signal strength */
    ql_nw_signal_strength_info_t sig_info;
    QL_NW_SIGNAL_STRENGTH_LEVEL_E level = QL_NW_SIGNAL_STRENGTH_LEVEL_NONE;
    memset ( &sig_info, 0, sizeof ( sig_info ) );
    ql_nw_get_signal_strength ( &sig_info, &level );

    if ( sig_info.has_lte )
    {
        snprintf ( tmp_str, sizeof ( tmp_str ), "%d", sig_info.lte.rsrp );
        nv_set ( "rsrp", tmp_str );
        nv_set ( "rsrp0", tmp_str );
        CLOGD ( FINE, "lte_sig_info : rsrp=[%s]\n", tmp_str );

        snprintf ( tmp_str, sizeof ( tmp_str ), "%d", sig_info.lte.rsrq );
        nv_set ( "rsrq", tmp_str );
        CLOGD ( FINE, "lte_sig_info : rsrq=[%s]\n", tmp_str );

        snprintf ( tmp_str, sizeof ( tmp_str ), "%d", sig_info.lte.rssi );
        nv_set ( "rssi", tmp_str );
        CLOGD ( FINE, "lte_sig_info : rssi=[%s]\n", tmp_str );

        snprintf ( tmp_str, sizeof ( tmp_str ), "%d", sig_info.lte.snr );
        nv_set ( "sinr", tmp_str );
        CLOGD ( FINE, "lte_sig_info : sinr=[%s]\n", tmp_str );

        if ( sig_info.has_nr5g )
        {
            snprintf ( tmp_str, sizeof ( tmp_str ), "%d", sig_info.nr5g.rsrp );
            nv_set ( "5g_rsrp", tmp_str );
            nv_set ( "5g_rsrp0", tmp_str );
            CLOGD ( FINE, "nr5g_sig_info : rsrp=[%s]\n", tmp_str );

            snprintf ( tmp_str, sizeof ( tmp_str ), "%d", sig_info.nr5g.rsrq );
            nv_set ( "5g_rsrq", tmp_str );
            CLOGD ( FINE, "nr5g_sig_info : rsrq=[%s]\n", tmp_str );

            snprintf ( tmp_str, sizeof ( tmp_str ), "%d", sig_info.nr5g.rssi );
            nv_set ( "5g_rssi", tmp_str );
            CLOGD ( FINE, "nr5g_sig_info : rssi=[%s]\n", tmp_str );

            snprintf ( tmp_str, sizeof ( tmp_str ), "%d", sig_info.nr5g.snr );
            nv_set ( "5g_sinr", tmp_str );
            CLOGD ( FINE, "nr5g_sig_info : sinr=[%s]\n", tmp_str );
        }
    }
    else if ( sig_info.has_nr5g )
    {
        snprintf ( tmp_str, sizeof ( tmp_str ), "%d", sig_info.nr5g.rsrp );
        nv_set ( "5g_rsrp", tmp_str );
        nv_set ( "5g_rsrp0", tmp_str );
        CLOGD ( FINE, "nr5g_sig_info : rsrp=[%s]\n", tmp_str );

        snprintf ( tmp_str, sizeof ( tmp_str ), "%d", sig_info.nr5g.rsrq );
        nv_set ( "5g_rsrq", tmp_str );
        CLOGD ( FINE, "nr5g_sig_info : rsrq=[%s]\n", tmp_str );

        snprintf ( tmp_str, sizeof ( tmp_str ), "%d", sig_info.nr5g.rssi );
        nv_set ( "5g_rssi", tmp_str );
        CLOGD ( FINE, "nr5g_sig_info : rssi=[%s]\n", tmp_str );

        snprintf ( tmp_str, sizeof ( tmp_str ), "%d", sig_info.nr5g.snr );
        nv_set ( "5g_sinr", tmp_str );
        CLOGD ( FINE, "nr5g_sig_info : sinr=[%s]\n", tmp_str );
    }

    switch ( level )
    {
    case QL_NW_SIGNAL_STRENGTH_LEVEL_NONE:
        break;
    case QL_NW_SIGNAL_STRENGTH_LEVEL_POOR:
        break;
    case QL_NW_SIGNAL_STRENGTH_LEVEL_MODERATE:
        break;
    case QL_NW_SIGNAL_STRENGTH_LEVEL_GOOD:
        break;
    case QL_NW_SIGNAL_STRENGTH_LEVEL_GREAT:
        break;
    default:
        break;
    }
#endif

#if 4    /* step [4]: update band info */
    ql_nw_get_band_info_t p_band_info;
    memset ( ( void * ) &p_band_info, 0, sizeof ( ql_nw_get_band_info_t ) );
    ql_nw_get_band_info ( &p_band_info );

    CLOGD ( FINE, "lte_band     = [%d]\n", p_band_info.lte_band );
    CLOGD ( FINE, "dl_bandwidth = [%s]\n", p_band_info.lte_dl_bandwidth );
    CLOGD ( FINE, "ul_bandwidth = [%s]\n", p_band_info.lte_ul_bandwidth );
    CLOGD ( FINE, "5g_band      = [%d]\n", p_band_info.nr_band );
    CLOGD ( FINE, "dl_bandwidth = [%s]\n", p_band_info.nr_dl_bandwidth );
    CLOGD ( FINE, "ul_bandwidth = [%s]\n", p_band_info.nr_ul_bandwidth );

    if ( 0 < p_band_info.lte_band )
    {
        snprintf ( tmp_str, sizeof ( tmp_str ), "B%d", p_band_info.lte_band );
        nv_set ( "band", tmp_str );

        snprintf ( tmp_str, sizeof ( tmp_str ), "%s", p_band_info.lte_dl_bandwidth );
        if ( strstr ( tmp_str, "M" ) )
        {
            tmp_str [ strlen ( tmp_str ) - 1 ] = '\0';
        }
        nv_set ( "bandwidth", tmp_str );
        nv_set ( "dl_bandwidth", tmp_str );

        snprintf ( tmp_str, sizeof ( tmp_str ), "%s", p_band_info.lte_ul_bandwidth );
        if ( strstr ( tmp_str, "M" ) )
        {
            tmp_str [ strlen ( tmp_str ) - 1 ] = '\0';
        }
        nv_set ( "ul_bandwidth", tmp_str );

        if ( 0 < p_band_info.nr_band )
        {
            snprintf ( tmp_str, sizeof ( tmp_str ), "N%d", p_band_info.nr_band );
            nv_set ( "5g_band", tmp_str );

            snprintf ( tmp_str, sizeof ( tmp_str ), "%s", p_band_info.nr_dl_bandwidth );
            if ( strstr ( tmp_str, "M" ) )
            {
                tmp_str [ strlen ( tmp_str ) - 1 ] = '\0';
            }
            nv_set ( "5g_bandwidth", tmp_str );
            nv_set ( "5g_dl_bandwidth", tmp_str );

            snprintf ( tmp_str, sizeof ( tmp_str ), "%s", p_band_info.nr_ul_bandwidth );
            if ( strstr ( tmp_str, "M" ) )
            {
                tmp_str [ strlen ( tmp_str ) - 1 ] = '\0';
            }
            nv_set ( "5g_ul_bandwidth", tmp_str );
        }
    }
    else if ( 0 < p_band_info.nr_band )
    {
        snprintf ( tmp_str, sizeof ( tmp_str ), "N%d", p_band_info.nr_band );
        nv_set ( "5g_band", tmp_str );

        snprintf ( tmp_str, sizeof ( tmp_str ), "%s", p_band_info.nr_dl_bandwidth );
        if ( strstr ( tmp_str, "M" ) )
        {
            tmp_str [ strlen ( tmp_str ) - 1 ] = '\0';
        }
        nv_set ( "5g_bandwidth", tmp_str );
        nv_set ( "5g_dl_bandwidth", tmp_str );

        snprintf ( tmp_str, sizeof ( tmp_str ), "%s", p_band_info.nr_ul_bandwidth );
        if ( strstr ( tmp_str, "M" ) )
        {
            tmp_str [ strlen ( tmp_str ) - 1 ] = '\0';
        }
        nv_set ( "5g_ul_bandwidth", tmp_str );
    }
#endif

#if 5    /* step [5]: update cell info */
    ql_nw_cell_info_t p_cell_info;
    memset ( ( void * ) &p_cell_info, 0, sizeof ( ql_nw_cell_info_t ) );
    ql_nw_get_cell_info ( &p_cell_info );
    if ( 1 /* p_cell_info */ )
    {
        CLOGD ( FINE, "serving_rat = [%d] -> [%s]\n",
                    p_cell_info.serving_rat, serving_rat_to_string ( p_cell_info.serving_rat ) );

        nv_set ( "neighbor_count", "0" );

        switch ( p_cell_info.serving_rat )
        {
        case QL_NW_RADIO_TECH_GSM:
            nv_set ( "mode", "2G" );
            break;
        case QL_NW_RADIO_TECH_UMTS:
            nv_set ( "mode", "3G" );
            break;
        case QL_NW_RADIO_TECH_LTE:
FAKE_NSA_5G:
            nv_set ( "mode", "4G" );
            if ( p_cell_info.lte_info_valid )
            {
                CLOGD ( FINE, "LTE_CELL_INFO -> cells count:[%d]\n", p_cell_info.lte_info_len );
                for ( i = 0; i < p_cell_info.lte_info_len; i++ )
                {
                    CLOGD ( FINE, "[%d] plmn   = [%s]\n", i, p_cell_info.lte_info[i].plmn );

                    CLOGD ( FINE, "[%d] earfcn = [%d]\n", i, p_cell_info.lte_info[i].earfcn );
                    snprintf ( tmp_str, sizeof ( tmp_str ), "neighbor_earfcn_%d", i );
                    snprintf ( tmp_val, sizeof ( tmp_val ), "%d", p_cell_info.lte_info[i].earfcn );
                    nv_set ( tmp_str, tmp_val );

                    CLOGD ( FINE, "[%d] cid    = [%x]\n", i, p_cell_info.lte_info[i].cid );

                    CLOGD ( FINE, "[%d] pci    = [%d]\n", i, p_cell_info.lte_info[i].pci );
                    snprintf ( tmp_str, sizeof ( tmp_str ), "neighbor_pci_%d", i );
                    snprintf ( tmp_val, sizeof ( tmp_val ), "%d", p_cell_info.lte_info[i].pci );
                    nv_set ( tmp_str, tmp_val );

                    CLOGD ( FINE, "[%d] tac    = [%x]\n", i, p_cell_info.lte_info[i].tac );

                    CLOGD ( FINE, "[%d] rsrp   = [%d]\n", i, p_cell_info.lte_info[i].rsrp );
                    snprintf ( tmp_str, sizeof ( tmp_str ), "neighbor_rsrp_%d", i );
                    snprintf ( tmp_val, sizeof ( tmp_val ), "%d", p_cell_info.lte_info[i].rsrp );
                    nv_set ( tmp_str, tmp_val );

                    CLOGD ( FINE, "[%d] rsrq   = [%d]\n", i, p_cell_info.lte_info[i].rsrq );
                    snprintf ( tmp_str, sizeof ( tmp_str ), "neighbor_rsrq_%d", i );
                    snprintf ( tmp_val, sizeof ( tmp_val ), "%d", p_cell_info.lte_info[i].rsrq );
                    nv_set ( tmp_str, tmp_val );

                    CLOGD ( FINE, "[%d] rssi   = [%d]\n", i, p_cell_info.lte_info[i].rssi );
                    snprintf ( tmp_str, sizeof ( tmp_str ), "neighbor_rssi_%d", i );
                    snprintf ( tmp_val, sizeof ( tmp_val ), "%d", p_cell_info.lte_info[i].rssi );
                    nv_set ( tmp_str, tmp_val );

                    CLOGD ( FINE, "[%d] sinr   = [%s]\n", i, "--" );
                    snprintf ( tmp_str, sizeof ( tmp_str ), "neighbor_sinr_%d", i );
                    snprintf ( tmp_val, sizeof ( tmp_val ), "%s", "--" );
                    nv_set ( tmp_str, tmp_val );
                }

                if ( 1 < p_cell_info.lte_info_len )
                {
                    snprintf ( tmp_val, sizeof ( tmp_val ), "%d", p_cell_info.lte_info_len - 1 );
                    nv_set ( "neighbor_count", tmp_val );
                }

                snprintf ( tmp_str, sizeof ( tmp_str ), "%d", p_cell_info.lte_info[0].earfcn );
                nv_set ( "dl_earfcn", tmp_str );

                earfcn_freq.dl_earfcn = p_cell_info.lte_info[0].earfcn;
                earfcn_freq.band = p_band_info.lte_band;
                if ( 0 == calc_freq_from_earfcn ( &earfcn_freq ) )
                {
                    snprintf ( tmp_str, sizeof ( tmp_str ), "%d", earfcn_freq.ul_earfcn );
                    nv_set ( "ul_earfcn", tmp_str );
                    snprintf ( tmp_str, sizeof ( tmp_str ), "%d.%03d", earfcn_freq.dl_frequency / 1000, earfcn_freq.dl_frequency % 1000 );
                    nv_set ( "frequency", tmp_str );
                    nv_set ( "dl_frequency", tmp_str );
                    snprintf ( tmp_str, sizeof ( tmp_str ), "%d.%03d", earfcn_freq.ul_frequency / 1000, earfcn_freq.ul_frequency % 1000 );
                    nv_set ( "ul_frequency", tmp_str );
                }

                snprintf ( tmp_str, sizeof ( tmp_str ), "%x", p_cell_info.lte_info[0].cid );
                nv_set ( "globalid", tmp_str );
                get_cell_id ( 0, 4, tmp_str, p_cell_info.lte_info[0].plmn );

                snprintf ( tmp_str, sizeof ( tmp_str ), "%d", p_cell_info.lte_info[0].pci );
                nv_set ( "pci", tmp_str );

                snprintf ( tmp_str, sizeof ( tmp_str ), "%x", p_cell_info.lte_info[0].tac );
                nv_set ( "tac", tmp_str );
            }
            break;
        case QL_NW_RADIO_TECH_NR5G:
            nv_set ( "mode", "5G" );
            if ( p_cell_info.nr_info_valid )
            {
                CLOGD ( FINE, "NR_CELL_INFO -> cells count:[%d]\n", p_cell_info.nr_info_len );
                for ( i = 0; i < p_cell_info.nr_info_len; i++ )
                {
                    CLOGD ( FINE, "[%d] plmn   = [%s]\n", i, p_cell_info.nr_info[i].plmn );

                    CLOGD ( FINE, "[%d] narfcn = [%d]\n", i, p_cell_info.nr_info[i].nr_arfcn );
                    snprintf ( tmp_str, sizeof ( tmp_str ), "neighbor_earfcn_%d", i );
                    snprintf ( tmp_val, sizeof ( tmp_val ), "%d", p_cell_info.nr_info[i].nr_arfcn );
                    nv_set ( tmp_str, tmp_val );

                    CLOGD ( FINE, "[%d] cid    = [%lx]\n", i, p_cell_info.nr_info[i].cid );

                    CLOGD ( FINE, "[%d] pci    = [%d]\n", i, p_cell_info.nr_info[i].pci );
                    snprintf ( tmp_str, sizeof ( tmp_str ), "neighbor_pci_%d", i );
                    snprintf ( tmp_val, sizeof ( tmp_val ), "%d", p_cell_info.nr_info[i].pci );
                    nv_set ( tmp_str, tmp_val );

                    CLOGD ( FINE, "[%d] tac    = [%x]\n", i, p_cell_info.nr_info[i].tac );

                    CLOGD ( FINE, "[%d] rsrp   = [%d]\n", i, p_cell_info.nr_info[i].rsrp );
                    snprintf ( tmp_str, sizeof ( tmp_str ), "neighbor_rsrp_%d", i );
                    snprintf ( tmp_val, sizeof ( tmp_val ), "%d", p_cell_info.nr_info[i].rsrp );
                    nv_set ( tmp_str, tmp_val );

                    CLOGD ( FINE, "[%d] rsrq   = [%d]\n", i, p_cell_info.nr_info[i].rsrq );
                    snprintf ( tmp_str, sizeof ( tmp_str ), "neighbor_rsrq_%d", i );
                    snprintf ( tmp_val, sizeof ( tmp_val ), "%d", p_cell_info.nr_info[i].rsrq );
                    nv_set ( tmp_str, tmp_val );

                    CLOGD ( FINE, "[%d] rssi   = [%d]\n", i, p_cell_info.nr_info[i].rssi );
                    snprintf ( tmp_str, sizeof ( tmp_str ), "neighbor_rssi_%d", i );
                    snprintf ( tmp_val, sizeof ( tmp_val ), "%d", p_cell_info.nr_info[i].rssi );
                    nv_set ( tmp_str, tmp_val );

                    CLOGD ( FINE, "[%d] sinr   = [%d]\n", i, p_cell_info.nr_info[i].sinr );
                    snprintf ( tmp_str, sizeof ( tmp_str ), "neighbor_sinr_%d", i );
                    snprintf ( tmp_val, sizeof ( tmp_val ), "%d", p_cell_info.nr_info[i].sinr );
                    nv_set ( tmp_str, tmp_val );

                    CLOGD ( FINE, "[%d] ta     = [%x]\n", i, p_cell_info.nr_info[i].ta );
                }

                if ( 1 < p_cell_info.nr_info_len )
                {
                    snprintf ( tmp_val, sizeof ( tmp_val ), "%d", p_cell_info.nr_info_len - 1 );
                    nv_set ( "neighbor_count", tmp_val );
                }

                snprintf ( tmp_str, sizeof ( tmp_str ), "%d", p_cell_info.nr_info[0].nr_arfcn );
                nv_set ( "5g_dl_earfcn", tmp_str );

                earfcn_freq.dl_earfcn = p_cell_info.nr_info[0].nr_arfcn;
                earfcn_freq.band = p_band_info.nr_band;
                if ( 0 == calc_5g_freq_from_narfcn ( &earfcn_freq ) )
                {
                    snprintf ( tmp_str, sizeof ( tmp_str ), "%d", earfcn_freq.ul_earfcn );
                    nv_set ( "5g_ul_earfcn", tmp_str );
                    snprintf ( tmp_str, sizeof ( tmp_str ), "%d.%03d", earfcn_freq.dl_frequency / 1000, earfcn_freq.dl_frequency % 1000 );
                    nv_set ( "5g_frequency", tmp_str );
                    nv_set ( "5g_dl_frequency", tmp_str );
                    snprintf ( tmp_str, sizeof ( tmp_str ), "%d.%03d", earfcn_freq.ul_frequency / 1000, earfcn_freq.ul_frequency % 1000 );
                    nv_set ( "5g_ul_frequency", tmp_str );
                }

                snprintf ( tmp_str, sizeof ( tmp_str ), "%lx", p_cell_info.nr_info[0].cid );
                nv_set ( "5g_globalid", tmp_str );
                get_cell_id ( 0, 5, tmp_str, p_cell_info.nr_info[0].plmn );

                snprintf ( tmp_str, sizeof ( tmp_str ), "%d", p_cell_info.nr_info[0].pci );
                nv_set ( "5g_pci", tmp_str );

                snprintf ( tmp_str, sizeof ( tmp_str ), "%x", p_cell_info.nr_info[0].tac );
                nv_set ( "5g_tac", tmp_str );
            }
            break;
        case QL_NW_RADIO_TECH_NSA5G:
            if ( p_band_info.nr_band <= 0 )
            {
                goto FAKE_NSA_5G;
            }
            nv_set ( "mode", "ENDC" );
            if ( p_cell_info.lte_info_valid && p_cell_info.nr_info_valid )
            {
                CLOGD ( FINE, "[NSA] LTE_CELL_INFO -> cells count:[%d]\n", p_cell_info.lte_info_len );
                for ( i = 0; i < p_cell_info.lte_info_len; i++ )
                {
                    CLOGD ( FINE, "[%d] plmn   = [%s]\n", i, p_cell_info.lte_info[i].plmn );

                    CLOGD ( FINE, "[%d] earfcn = [%d]\n", i, p_cell_info.lte_info[i].earfcn );
                    snprintf ( tmp_str, sizeof ( tmp_str ), "neighbor_earfcn_%d", i );
                    snprintf ( tmp_val, sizeof ( tmp_val ), "%d", p_cell_info.lte_info[i].earfcn );
                    nv_set ( tmp_str, tmp_val );

                    CLOGD ( FINE, "[%d] cid    = [%x]\n", i, p_cell_info.lte_info[i].cid );

                    CLOGD ( FINE, "[%d] pci    = [%d]\n", i, p_cell_info.lte_info[i].pci );
                    snprintf ( tmp_str, sizeof ( tmp_str ), "neighbor_pci_%d", i );
                    snprintf ( tmp_val, sizeof ( tmp_val ), "%d", p_cell_info.lte_info[i].pci );
                    nv_set ( tmp_str, tmp_val );

                    CLOGD ( FINE, "[%d] tac    = [%x]\n", i, p_cell_info.lte_info[i].tac );

                    CLOGD ( FINE, "[%d] rsrp   = [%d]\n", i, p_cell_info.lte_info[i].rsrp );
                    snprintf ( tmp_str, sizeof ( tmp_str ), "neighbor_rsrp_%d", i );
                    snprintf ( tmp_val, sizeof ( tmp_val ), "%d", p_cell_info.lte_info[i].rsrp );
                    nv_set ( tmp_str, tmp_val );

                    CLOGD ( FINE, "[%d] rsrq   = [%d]\n", i, p_cell_info.lte_info[i].rsrq );
                    snprintf ( tmp_str, sizeof ( tmp_str ), "neighbor_rsrq_%d", i );
                    snprintf ( tmp_val, sizeof ( tmp_val ), "%d", p_cell_info.lte_info[i].rsrq );
                    nv_set ( tmp_str, tmp_val );

                    CLOGD ( FINE, "[%d] rssi   = [%d]\n", i, p_cell_info.lte_info[i].rssi );
                    snprintf ( tmp_str, sizeof ( tmp_str ), "neighbor_rssi_%d", i );
                    snprintf ( tmp_val, sizeof ( tmp_val ), "%d", p_cell_info.lte_info[i].rssi );
                    nv_set ( tmp_str, tmp_val );

                    CLOGD ( FINE, "[%d] sinr   = [%s]\n", i, "--" );
                    snprintf ( tmp_str, sizeof ( tmp_str ), "neighbor_sinr_%d", i );
                    snprintf ( tmp_val, sizeof ( tmp_val ), "%s", "--" );
                    nv_set ( tmp_str, tmp_val );
                }

                if ( 1 < p_cell_info.lte_info_len )
                {
                    snprintf ( tmp_val, sizeof ( tmp_val ), "%d", p_cell_info.lte_info_len - 1 );
                    nv_set ( "neighbor_count", tmp_val );
                }

                snprintf ( tmp_str, sizeof ( tmp_str ), "%d", p_cell_info.lte_info[0].earfcn );
                nv_set ( "dl_earfcn", tmp_str );

                earfcn_freq.dl_earfcn = p_cell_info.lte_info[0].earfcn;
                earfcn_freq.band = p_band_info.lte_band;
                if ( 0 == calc_freq_from_earfcn ( &earfcn_freq ) )
                {
                    snprintf ( tmp_str, sizeof ( tmp_str ), "%d", earfcn_freq.ul_earfcn );
                    nv_set ( "ul_earfcn", tmp_str );
                    snprintf ( tmp_str, sizeof ( tmp_str ), "%d.%03d", earfcn_freq.dl_frequency / 1000, earfcn_freq.dl_frequency % 1000 );
                    nv_set ( "frequency", tmp_str );
                    nv_set ( "dl_frequency", tmp_str );
                    snprintf ( tmp_str, sizeof ( tmp_str ), "%d.%03d", earfcn_freq.ul_frequency / 1000, earfcn_freq.ul_frequency % 1000 );
                    nv_set ( "ul_frequency", tmp_str );
                }

                snprintf ( tmp_str, sizeof ( tmp_str ), "%x", p_cell_info.lte_info[0].cid );
                nv_set ( "globalid", tmp_str );
                get_cell_id ( 0, 4, tmp_str, p_cell_info.lte_info[0].plmn );

                snprintf ( tmp_str, sizeof ( tmp_str ), "%d", p_cell_info.lte_info[0].pci );
                nv_set ( "pci", tmp_str );

                snprintf ( tmp_str, sizeof ( tmp_str ), "%x", p_cell_info.lte_info[0].tac );
                nv_set ( "tac", tmp_str );

                CLOGD ( FINE, "[NSA] NR_CELL_INFO -> cells count:[%d]\n", p_cell_info.nr_info_len );
                for ( i = 0; i < p_cell_info.nr_info_len; i++ )
                {
                    CLOGD ( FINE, "[%d] plmn   = [%s]\n", i, p_cell_info.nr_info[i].plmn );

                    CLOGD ( FINE, "[%d] narfcn = [%d]\n", i, p_cell_info.nr_info[i].nr_arfcn );
                    snprintf ( tmp_str, sizeof ( tmp_str ), "neighbor_earfcn_%d", i );
                    snprintf ( tmp_val, sizeof ( tmp_val ), "%d", p_cell_info.nr_info[i].nr_arfcn );
                    nv_set ( tmp_str, tmp_val );

                    CLOGD ( FINE, "[%d] cid    = [%lx]\n", i, p_cell_info.nr_info[i].cid );

                    CLOGD ( FINE, "[%d] pci    = [%d]\n", i, p_cell_info.nr_info[i].pci );
                    snprintf ( tmp_str, sizeof ( tmp_str ), "neighbor_pci_%d", i );
                    snprintf ( tmp_val, sizeof ( tmp_val ), "%d", p_cell_info.nr_info[i].pci );
                    nv_set ( tmp_str, tmp_val );

                    CLOGD ( FINE, "[%d] tac    = [%x]\n", i, p_cell_info.nr_info[i].tac );

                    CLOGD ( FINE, "[%d] rsrp   = [%d]\n", i, p_cell_info.nr_info[i].rsrp );
                    snprintf ( tmp_str, sizeof ( tmp_str ), "neighbor_rsrp_%d", i );
                    snprintf ( tmp_val, sizeof ( tmp_val ), "%d", p_cell_info.nr_info[i].rsrp );
                    nv_set ( tmp_str, tmp_val );

                    CLOGD ( FINE, "[%d] rsrq   = [%d]\n", i, p_cell_info.nr_info[i].rsrq );
                    snprintf ( tmp_str, sizeof ( tmp_str ), "neighbor_rsrq_%d", i );
                    snprintf ( tmp_val, sizeof ( tmp_val ), "%d", p_cell_info.nr_info[i].rsrq );
                    nv_set ( tmp_str, tmp_val );

                    CLOGD ( FINE, "[%d] rssi   = [%d]\n", i, p_cell_info.nr_info[i].rssi );
                    snprintf ( tmp_str, sizeof ( tmp_str ), "neighbor_rssi_%d", i );
                    snprintf ( tmp_val, sizeof ( tmp_val ), "%d", p_cell_info.nr_info[i].rssi );
                    nv_set ( tmp_str, tmp_val );

                    CLOGD ( FINE, "[%d] sinr   = [%d]\n", i, p_cell_info.nr_info[i].sinr );
                    snprintf ( tmp_str, sizeof ( tmp_str ), "neighbor_sinr_%d", i );
                    snprintf ( tmp_val, sizeof ( tmp_val ), "%d", p_cell_info.nr_info[i].sinr );
                    nv_set ( tmp_str, tmp_val );

                    CLOGD ( FINE, "[%d] ta     = [%x]\n", i, p_cell_info.nr_info[i].ta );
                }

                if ( 1 < p_cell_info.nr_info_len )
                {
                    snprintf ( tmp_val, sizeof ( tmp_val ), "%d", p_cell_info.nr_info_len - 1 );
                    nv_set ( "neighbor_count", tmp_val );
                }

                snprintf ( tmp_str, sizeof ( tmp_str ), "%d", p_cell_info.nr_info[0].nr_arfcn );
                nv_set ( "5g_dl_earfcn", tmp_str );

                earfcn_freq.dl_earfcn = p_cell_info.nr_info[0].nr_arfcn;
                earfcn_freq.band = p_band_info.nr_band;
                if ( 0 == calc_5g_freq_from_narfcn ( &earfcn_freq ) )
                {
                    snprintf ( tmp_str, sizeof ( tmp_str ), "%d", earfcn_freq.ul_earfcn );
                    nv_set ( "5g_ul_earfcn", tmp_str );
                    snprintf ( tmp_str, sizeof ( tmp_str ), "%d.%03d", earfcn_freq.dl_frequency / 1000, earfcn_freq.dl_frequency % 1000 );
                    nv_set ( "5g_frequency", tmp_str );
                    nv_set ( "5g_dl_frequency", tmp_str );
                    snprintf ( tmp_str, sizeof ( tmp_str ), "%d.%03d", earfcn_freq.ul_frequency / 1000, earfcn_freq.ul_frequency % 1000 );
                    nv_set ( "5g_ul_frequency", tmp_str );
                }

                snprintf ( tmp_str, sizeof ( tmp_str ), "%lx", p_cell_info.nr_info[0].cid );
                nv_set ( "5g_globalid", tmp_str );
                get_cell_id ( 0, 5, tmp_str, p_cell_info.lte_info[0].plmn );  // use lte plmn info

                snprintf ( tmp_str, sizeof ( tmp_str ), "%d", p_cell_info.nr_info[0].pci );
                nv_set ( "5g_pci", tmp_str );

                snprintf ( tmp_str, sizeof ( tmp_str ), "%x", p_cell_info.nr_info[0].tac );
                nv_set ( "5g_tac", tmp_str );
            }
            break;
        default:
            break;
        }

        nv_set ( "search_neighbor_set", "0" );
    }
#endif

#if 6    /* step [6]: update roaming pref */
    ql_nw_pref_nwmode_roaming_info_t roam_info;
    memset ( ( void * ) &roam_info, 0, sizeof(ql_nw_pref_nwmode_roaming_info_t));
    ql_nw_get_roaming ( &roam_info );
    CLOGD ( FINE, "pref_roaming: [%d]\n", roam_info.preferred_roaming );
#endif

    ql_nw_release ();
#else
    char at_rep[STR_AT_RSP_LEN_2X] = {0};

    COMD_AT_PROCESS ( "AT+KENG=\"servingcell\"", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "+KENG:" ) )
        parsing_mtk_serving_cellinfo ( at_rep );

#endif
}

void mtkParam_updatePhoneNumber(void)
{
    char at_rep[256] = {0};
    char ram_val[32] = {0};

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( "AT+CNUM", 1500, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "+CNUM:" ) )
    {
        parsing_mtk_cnum ( at_rep );
    }

}

int mtkParam_setCfunMode ( char* mode )
{
    char cfunSet_ret[4] = {0};

    mtkParam_radioOnOffSet ( mode );

    nv_get ( "setOnOff_lte", cfunSet_ret, sizeof ( cfunSet_ret ) );

    return strcmp ( cfunSet_ret, "0" ) ? -1 : 0;
}

int mtkParam_updateDefaultGateway ( char* data )
{
    char nv_gateway[8] = {0};
    char config_gw[8] = {0};

    sys_get_config ( LTE_PARAMETER_APN_GW, config_gw, sizeof ( config_gw ) );

    nv_get ( "default_gateway", nv_gateway, sizeof ( nv_gateway ) );
    if ( strcmp ( nv_gateway, "9" ) )
    {
        nv_set ( "default_gateway", strcmp ( config_gw, "" ) ? config_gw : "1" );
    }

    return 0;
}

static int mtkParam_setSmsCmgf ( char* data )
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+CMGF=0", 1500, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
        return -1;

    return 0;
}

static int mtkParam_setSmsCnmi ( char* data )
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+CNMI=2,1,0,1,0", 1500, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
        return -1;

    return 0;
}

static int mtkParam_setSmsCpms ( char* data )
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+CPMS=\"ME\",\"ME\",\"ME\"", 1500, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
        return -1;

    return 0;
}

static int mtkParam_getSmsMaxNum ( char* data )
{
    char at_rep[128] = {0};

    COMD_AT_PROCESS ( "AT+CPMS?", 1500, at_rep, sizeof ( at_rep ) );

    parsing_mtk_cpms_get ( at_rep );

    if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
        return -1;

    return 0;
}

static int mtkParam_setSmsCsmp ( char* data )
{
    char at_rep[64] = {0};

    COMD_AT_PROCESS ( "AT+CSMP=0,255,0,8", 1500, at_rep, sizeof ( at_rep ) );

    if ( NULL == strstr ( at_rep, "\r\nOK\r\n" ) )
        return -1;

    return 0;
}

void mtkParam_initSmsSimCardMessage ()
{
    char at_rep[RECV_SMS_SIZE] = {0};

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( "AT+CMGL=4", 5000, at_rep, sizeof ( at_rep ) );

    if ( strcmp ( at_rep, "" ) )
        parsing_mtk_cmgl_four_sim_card ( at_rep );

}

void mtkParam_getSimBookMaxNumber ( char* data )
{
    char at_rep[32] = {0};

    memset ( at_rep, 0, sizeof ( at_rep ) );
    COMD_AT_PROCESS ( "AT+CSCS=\"GSM\"", 3000, at_rep, sizeof ( at_rep ) );

    if ( strstr ( at_rep, "\r\nOK\r\n" ) )
    {
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( "AT+CPBS?", 3000, at_rep, sizeof ( at_rep ) );

        if( !strstr( at_rep, "CPBS: \"SM\"" ) )
        {
            COMD_AT_PROCESS ( "AT+CPBS=\"SM\"", 3000, at_rep, sizeof ( at_rep ) );
            memset ( at_rep, 0, sizeof ( at_rep ) );
            COMD_AT_PROCESS ( "AT+CPBS?", 3000, at_rep, sizeof ( at_rep ) );
        }

        if ( strcmp ( at_rep, "" ) )
        {
            parsing_mtk_cpbs ( at_rep );
        }
    }
}

void mtkParam_updateCmglInfo ()
{

    char at_rep[RECV_SMS_SIZE] = {0};
    int ret = 0;
    char sms_all_number_buf[8] = {0};

    nv_get ( "all_sms_number", sms_all_number_buf, sizeof ( sms_all_number_buf) );
    if(strcmp(sms_all_number_buf,""))
    {
        if(atoi(sms_all_number_buf)>200)
            return;
    }

    COMD_AT_PROCESS ( "AT+CMGL=4", 5000, at_rep, sizeof ( at_rep ) );

    if ( strcmp ( at_rep, "" ) )
    {
        ret = parsing_mtk_cmgl_zero ( at_rep );
    }

    memset(at_rep,0,sizeof(at_rep));

    if(ret == 0)
    {
        COMD_AT_PROCESS ( "AT+CMGD=1,4", 5000, at_rep, sizeof ( at_rep ) );
    }

    long_message_table_check();
}

void mtkParam_updateSmsCenterNum ()
{
    char at_rep[64] = {0};
    char ram_val[32] = {0};

    nv_get ( "sms_center_num", ram_val, sizeof ( ram_val ) );

    if ( 0 == strcmp ( ram_val, "" ) || 0 == strcmp ( ram_val, "--" ) )
    {
        COMD_AT_PROCESS ( "AT+CSCA?", 3000, at_rep, sizeof ( at_rep ) );

        if ( strstr ( at_rep, "+CSCA:" ) )
            parsing_mtk_csca_get ( at_rep );
    }
}

void mtkParam_initBookOnSim ( char* data )
{
    char simbook_maxnum[4] = {0};
    char simbook_curnum[4] = {0};
    char szCmd[128] = {0};
    char at_rep[2560] = {0};
    char *substr = NULL;
    int maxnum = 0;
    int curnum = 0;
    int i = 0;
    int j = 0;
    int ret = 0;
    char sql[4096]={0};
    char *errmsg = NULL;
    sqlite3 *db;

    ret = sqlite3_open ( SMS_DIR_TMP, &db );
    if ( ret != SQLITE_OK )
    {
        perror ( "sqlite open :" );
        return;
    }

    sprintf ( sql, "delete from sms_book where location = '1';" );

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

    sqlite3_close ( db );
    nv_set ( "initBookOnSim", "1" );

    nv_get ( "simbook_maxnum", simbook_maxnum, sizeof ( simbook_maxnum ) );
    nv_get ( "simbook_curnum", simbook_curnum, sizeof ( simbook_curnum ) );

    CLOGD ( FINE, "simbook_maxnum [%s] simbook_curnum [%s]\n", simbook_maxnum, simbook_curnum );

    maxnum = atoi ( simbook_maxnum );
    curnum = atoi ( simbook_curnum );

    for ( ; i < maxnum && j < curnum; i = i + 20 )
    {
        snprintf ( szCmd, sizeof ( szCmd ), "AT+CPBR=%d,%d", i + 1, i + 20 );
        memset ( at_rep, 0, sizeof ( at_rep ) );
        COMD_AT_PROCESS ( szCmd, 1500, at_rep, sizeof ( at_rep ) );

        CLOGD ( FINE, "AT+CPBR=%d,%d \n return :\n [%s]\n", i + 1, i + 20, at_rep );

        if ( strstr ( at_rep, "\r\nOK\r\n" ) )
        {
            substr = strtok ( at_rep, "\r\n" );

            while ( NULL != substr )
            {
                CLOGD ( FINE, "substr: [%s]\n", substr );

                if ( strstr ( substr, "+CPBR:" ) )
                {
                    parsing_mtk_cpbr ( substr );
                    j++;
                }
                substr = strtok ( NULL, "\r\n" );
            }
        }
    }
    nv_set ( "initBookOnSim", "1" );
}

static void mtkParam_updateSignalBar ()
{
    char signal_s[8] = {0};
    int signal_i = 0;
    char ram_val[8] = {0};

#if 0
    nv_get ( "sinr", signal_s, sizeof ( signal_s ) );
    sscanf ( signal_s, "%d", &signal_i );

    if (signal_i < -3)
    {
        nv_set("signal_bar", (char *)"0");
    }
    else if (signal_i < 1)
    {
        nv_set("signal_bar", (char *)"1");
    }
    else if (signal_i < 7)
    {
        nv_set("signal_bar", (char *)"2");
    }
    else if (signal_i < 13)
    {
        nv_set("signal_bar", (char *)"3");
    }
    else
    {
        nv_set("signal_bar", (char *)"4");
    }
#else
    nv_get ( "mode", ram_val, sizeof ( ram_val ) );

    nv_get ( strcmp ( ram_val, "5G" ) ? "rsrp0" : "5g_rsrp0", signal_s, sizeof ( signal_s ) );

    sscanf ( signal_s, "%d", &signal_i );

    if ( signal_i <= -135 )
    {
        nv_set ( "signal_bar", "0" );
    }
    else if ( signal_i <= -120 )
    {
        nv_set ( "signal_bar", "1" );
    }
    else if ( signal_i <= -100 )
    {
        nv_set ( "signal_bar", "2" );
    }
    else if ( signal_i <= -30 )
    {
        if ( signal_i <= -85 || 0 == strncmp ( led_mode, "1", 1 ) )
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
#endif
}

static int mtkParam_initNetSelectMode ( char* data )
{
    char uci_conn_mode[4] = {0};

    sys_get_config ( WAN_LTE_SELECT_MODE, uci_conn_mode, sizeof ( uci_conn_mode ) );

    if ( 0 == strcmp ( uci_conn_mode, "0" ) )
    {
        manual_conn = 0;
        nv_set ( "net_select_mode", "manual_select" );
    }
    else
    {
        manual_conn = 1;
        nv_set ( "net_select_mode", "auto_select" );
    }

    return 0;
}

#if defined (CONFIG_SW_STC_PINLOCK)
void mtkParam_randomPinCode ()
{
    char pin_cpwd[64] = {0};
    char at_rep[32] = {0};
    char strPinLock[64] = {0};
    char strIccid[128] = {0};
    char iccid[32] = {0};

    //Generate random pin code (1000-99999999)
    //modify the pin code of the SIM card
    //and save the random pin code to flash

    //reset cell lock list
    mtkParam_setCellLock ( "12,00000000" );

    srand ( mix ( clock(), time ( NULL ), getpid() ) );
    int random_num = 1000 + rand() % ( 99999999 - 1000 );
    CLOGD ( FINE, "Random PIN code is [%d]\n", random_num );

    snprintf ( strPinLock, sizeof ( strPinLock ),
        "/lib/factory_tool.sh CONFIG_HW_STC_PIN_CODE=%d", random_num );
    system ( strPinLock );

    //Save STC SIM ICCID to flash
    nv_get ( "iccid", iccid, sizeof ( iccid ) );
    snprintf ( strIccid, sizeof ( strIccid ),
        "/lib/factory_tool.sh CONFIG_HW_STC_ICCID=%s", iccid );
    system ( strIccid );

    snprintf ( pin_cpwd, sizeof ( pin_cpwd ),
        "AT+CPWD=\"SC\",\"%s\",\"%d\"", STC_PINLOCK_CODE, random_num );
    COMD_AT_PROCESS ( pin_cpwd, 1500, at_rep, sizeof ( at_rep ) );
}
#endif

static void init_abnormal_check_config ()
{
    char conf_val[8] = {0};
    int i = 0;
    int conf_mask = 0;

    sys_get_config ( LTE_PARAMETER_ABNORMAL_CHECK, conf_val, sizeof ( conf_val ) );

    if ( strcmp ( conf_val, "" ) )
    {
        conf_mask = atoi ( conf_val );

        for ( i = 0; i < 5; i++ )
        {
            abnormal_check_flag[i] = ( conf_mask & ( 1 << i ) );
            CLOGD ( FINE, "abnormal_check_flag[%d]: [%d]\n", i, abnormal_check_flag[i] );
        }
    }
    else
    {
        for ( i = 0; i < 5; i++ )
        {
            abnormal_check_flag[i] = 1;
            CLOGD ( FINE, "abnormal_check_flag[%d]: [%d]\n", i, abnormal_check_flag[i] );
        }
    }
}

int mtk_module_param_init ()
{
    reset_apns_msg_flag ( 0 );

    init_abnormal_check_config ();

    cpin_error_check = 0;
    main_status_check = 0;
    abnormal_start_time = 0;
    dial_status_check = 0;
    param_init_done = 0;

    if (
        mtkParam_setATEcho ( NULL ) ||
        mtkParam_setCfunMode ( "off" ) ||
        mtkParam_updateImei ( NULL ) ||
        mtkParam_updateSN ( NULL ) ||
        mtkParam_updateUnlockCode ( NULL ) ||
        mtkParam_initZteCellLock ( NULL ) ||
        mtkParam_updateModuleModel ( NULL ) ||
        mtkParam_updateModuleVersion ( NULL ) ||
        mtkParam_updateDefaultGateway ( NULL ) ||
        mtkParam_updateSuppBands ( NULL ) ||
        mtkParam_updateUsbmode ( NULL ) ||
        mtkParam_SetIms ( NULL ) ||
        mtkParam_initAPNsetting ( NULL ) ||
        mtkParam_radioLockInit ( NULL ) ||
        mtkParam_lteRoamingSet ( NULL ) ||
        mtkParam_setCfunMode ( "on" ) ||
        mtkParam_initNetSelectMode ( NULL ) ||
        mtkParam_setRatPriority ( NULL )
      )
        return -1;

    return 0;
}

static int abnormal_more_than_xxx_seconds ( int check_times, int xxx )
{
    struct sysinfo info;

    sysinfo ( &info );

    if ( 0 == check_times || 0 == abnormal_start_time )
    {
        abnormal_start_time = info.uptime;
    }
    else if ( xxx < ( info.uptime - abnormal_start_time ) )
    {
        CLOGD ( ERROR, "abnormal have lasted more than %d seconds\n", xxx );
        return 1;
    }

    return 0;
}

static int mtkParam_checkAbnormal ( char* sim_stat, char* main_stat )
{
    int ret = 1;
    char radio_stat[8] = {0};
    char net_select_mode[32] = {0};
    char sim_lock_status[8] = {0};
    char pinlocked[4] = {0};
    char celllocked[4] = {0};

    nv_get ( "simlocked", sim_lock_status, sizeof ( sim_lock_status ) );
    nv_get ( "lte_on_off", radio_stat, sizeof ( radio_stat ) );
    nv_get ( "net_select_mode", net_select_mode, sizeof ( net_select_mode ) );
    nv_get ( "pinlocked", pinlocked, sizeof ( pinlocked ) );
    nv_get ( "celllocked", celllocked, sizeof ( celllocked ) );

    CLOGD ( FINE, "sim_stat: [%s], main_stat: [%s]\n", sim_stat, main_stat );
    CLOGD ( FINE, "sim_lock_status   -> [%s]\n", sim_lock_status );
    CLOGD ( FINE, "cpin_error_check   -> [%d]\n", cpin_error_check );
    CLOGD ( FINE, "main_status_check  -> [%d]\n", main_status_check );
    CLOGD ( FINE, "dial_status_check  -> [%d]\n", dial_status_check );
    CLOGD ( FINE, "lte_on_off         -> [%s]\n", radio_stat );
    CLOGD ( FINE, "net_select_mode    -> [%s]\n", net_select_mode );

    if ( 0 == strcmp ( radio_stat, "off" ) || 0 == strcmp ( net_select_mode, "manual_select" ) )
    {
        cpin_error_check = 0;
        main_status_check = 0;
        dial_status_check = 0;
        return 0;
    }

    if ( 0 == strcmp ( sim_stat, "ERROR" ) )
    {
        if ( abnormal_more_than_xxx_seconds ( cpin_error_check, 120 ) || cpin_error_check++ > 90 )
        {
            CLOGD ( FINE, "abnormal_check_flag[0]: [%d]\n", abnormal_check_flag[0] );
            if ( abnormal_check_flag[0] )
            {
                mtkParam_radioRestart ();
                CLOGD ( ERROR, "exe AT+CFUN=0/1 for sim_not_ready !!!\n" );
                SYS_INFO ( "SIM card is ERROR, exe AT+CFUN=0/1" );
            }
            cpin_error_check = 0;
            ret = -1;
        }

        main_status_check = 0;
        dial_status_check = 0;
        return ret;
    }
    else
    {
        cpin_error_check = 0;
    }

    if ( 0 == strcmp ( sim_stat, "READY" ) && strcmp ( main_stat, "connected" ) &&
         0 == strcmp ( sim_lock_status, "0" ) && 0 == strcmp ( pinlocked, "0" ) &&
         0 == strcmp ( celllocked, "0" ) )
    {
        if ( abnormal_more_than_xxx_seconds ( main_status_check, 120 ) || main_status_check++ > 90 )
        {
            CLOGD ( FINE, "abnormal_check_flag[1]: [%d]\n", abnormal_check_flag[1] );
            if ( abnormal_check_flag[1] )
            {
                mtkParam_radioRestart ();
                CLOGD ( ERROR, "exe AT+CFUN=0/1 for main_status not connected !!!\n" );
                SYS_INFO ( "Network connection status is abnormal, exe AT+CFUN=0/1" );
            }
            main_status_check = 0;
            ret = -1;
        }

        dial_status_check = 0;
        return ret;
    }
    else
    {
        main_status_check = 0;
    }

    char apn1_state[16] = {0};
    char apn1_ipv6_state[16] = {0};

    nv_get ( "apn1_state", apn1_state, sizeof ( apn1_state ) );
    nv_get ( "apn1_ipv6_state", apn1_ipv6_state, sizeof ( apn1_ipv6_state ) );

    if ( 0 == strcmp ( main_stat, "connected" ) && strcmp ( apn1_state, "connect" ) && strcmp ( apn1_ipv6_state, "connect" ) )
    {
        if ( abnormal_more_than_xxx_seconds ( dial_status_check, 120 ) || dial_status_check++ > 90 )
        {
            CLOGD ( FINE, "abnormal_check_flag[1]: [%d]\n", abnormal_check_flag[2] );
            if ( abnormal_check_flag[2] )
            {
                mtkParam_radioRestart ();
                CLOGD ( ERROR, "exe AT+CFUN=0/1 for main_status not connected !!!\n" );
                SYS_INFO ( "Dail status is abnormal, exe AT+CFUN=0/1" );
            }
            dial_status_check = 0;
            ret = -1;
        }

        return ret;
    }
    else
    {
        dial_status_check = 0;
    }

    return 0;
}

void mtk_normal_status_refresh()
{
    char cpin_value[16] = {0};
    char mStatusVal[32] = {0};
    char cesq_value[64] = {0};
    char cgatt_value[4] = {0};
    char c5greg_stat[4] = {0};
    char time_str[16] = {0};
    char apn_enable[4] = {0};
    char strUciOpt[64] = {0};
    char strRamOpt[64] = {0};
#if defined(MTK_T750) && CONFIG_QUECTEL_T750
#else
    char apn_act_state[4] = {0};
    char ram_val[16] = {0};
    char apn_netdev[16] = {0};
    char socket_cmd[64] = {0};
    char signal_value[8] = {0};
    int ret = 0;
#endif
    int i = 0;
    char radio_stat[8] = {0};
    char simlock_stat[4] = {0};
    char ram_conn_mode[16] = {0};
    int cur_cfun_state = 0;
    char at_rep[32] = {0};
    char lockpin_value[4] = {0};
    char pinlock_code[16] = {0};
    char pin_times[4] = {0};
    char pinlocked[4] = {0};
    char pin_clck[64] = {0};
    char iccid[32] = {0};
    char factory_iccid[32] = {0};

    mtkParam_updatePinStatus ();
    mtkParam_updatePinLockStatus ();
    mtkParam_updatePinPukCount ();

    if ( 0 == strncmp ( dualsim_en, "1", 1 ) && 1 == mtkParam_updateDualSim () )
    {
        comd_wait_ms ( 10 * 1000 );
        mtkParam_initAPNsetting ( NULL );
        mtkParam_radioLockInit ( "restore_fullband" );
        nv_set ( "imsi", "--" );
        return;
    }

    nv_get ( "cpin", cpin_value, sizeof ( cpin_value ) );
    nv_get ( "main_status", mStatusVal, sizeof ( mStatusVal ) );
#if defined(MTK_T750) && CONFIG_QUECTEL_T750
#else
    snprintf ( socket_cmd, sizeof ( socket_cmd ), "ilte_mon \"main_status=%s\"", mStatusVal );
    CLOGD ( FINE, "socket_cmd -> [%s]\n", socket_cmd );
    system ( socket_cmd );
#endif

    mtkParam_checkAbnormal ( cpin_value, mStatusVal );

    if ( 0 == strcmp ( cpin_value, "READY" ) )
    {
        mtkParam_updateImsi ();
        mtkParam_updateSimSPN ();
        mtkParam_updateIccid ();
#if defined (CONFIG_SW_STC_PINLOCK)
        if ( 0 != strcmp ( pinlock_enable, "0" ) )
        {
            nv_get ( "lockpin_status", lockpin_value, sizeof ( lockpin_value ) );
            CLOGD ( FINE, "lockpin_status is [%s]\n", lockpin_value );

            if ( 0 == strcmp ( lockpin_value, "0" ) )
            {
                //Query pin_times, stop querying CLCK when <= 1,
                //Prevent the SIM card from entering the PUK state
                nv_get ( "pin_times", pin_times, sizeof ( pin_times ) );

                if ( 1 >= atoi ( pin_times ) )
                {
                    nv_set ( "pinlocked", "1" );
                    CLOGD ( FINE, "pinlocked is ERROR !\n" );
                    goto END;
                }

                snprintf ( pin_clck, sizeof ( pin_clck ), "AT+CLCK=\"SC\",1,\"%s\"", STC_PINLOCK_CODE );
                COMD_AT_PROCESS ( pin_clck, 1500, at_rep, sizeof ( at_rep ) );
                if ( strstr ( at_rep, "\r\nOK\r\n" ) )
                {
                    nv_set ( "pinlocked", "0" );
                    CLOGD ( FINE, "pinlocked is READY !\n" );
                    mtkParam_randomPinCode ();
                }
                else
                {
                    nv_set ( "pinlocked", "1" );
                    CLOGD ( FINE, "pinlocked is ERROR !\n" );
                    goto END;
                }
            }
            else
            {
                nv_set ( "pinlocked", "0" );
                CLOGD ( FINE, "pinlocked is READY !\n" );
            }
        }
#endif
        if(CONFIG_SW_SMS == 1)
        {
            if(param_init_done == 0)
            {
                comd_wait_ms ( 5000 ); // wait for SIM really ready
                mtkParam_updatePhoneNumber ();
                mtkParam_setSmsCmgf ( NULL );
                mtkParam_setSmsCnmi ( NULL );
                mtkParam_setSmsCpms ( NULL );
                mtkParam_getSmsMaxNum ( NULL );
                mtkParam_setSmsCsmp ( NULL );
                initSmsSqlite3 ();
                mtkParam_initSmsSimCardMessage ( NULL );
                mtkParam_getSimBookMaxNumber ( NULL );
                //mtkParam_initBookOnSim ( NULL );

                param_init_done = 1;
            }

            mtkParam_updateSmsCenterNum();
        }

        mtkParam_getSimlock ( NULL );

        nv_get ( "simlocked", simlock_stat, sizeof ( simlock_stat ) );
#if defined(MTK_T750) && CONFIG_QUECTEL_T750
#else
        nv_get ( "signal_bar", signal_value, sizeof ( signal_value ) );
        snprintf ( socket_cmd, sizeof ( socket_cmd ), "ilte_mon \"simlocked=%s\"", simlock_stat );
        CLOGD ( FINE, "socket_cmd -> [%s]\n", socket_cmd );
        system ( socket_cmd );

        snprintf ( socket_cmd, sizeof ( socket_cmd ), "ilte_mon \"signal_bar=%s\"", signal_value );
        CLOGD ( FINE, "socket_cmd -> [%s]\n", socket_cmd );
        system ( socket_cmd );
#endif
        nv_get ( "lte_on_off", radio_stat, sizeof ( radio_stat ) );
        nv_get ( "net_select_mode", ram_conn_mode, sizeof ( ram_conn_mode ) );
        nv_get ( "setOnOff_lte", strRamOpt, sizeof ( strRamOpt ) );

        COMD_AT_PROCESS ( "AT+CFUN?", 3000, at_rep, sizeof ( at_rep ) );

        cur_cfun_state = parsing_mtk_cfun_get(at_rep);

        if ( strncmp(strRamOpt,"-1",2) != 0 && strcmp ( simlock_stat, "1" ) == 0 && cur_cfun_state == 1)
        {
            COMD_AT_PROCESS ( "AT+CFUN=4", 3000, at_rep, sizeof ( at_rep ) );
        }
        else if ( strncmp(strRamOpt,"-1",2) != 0 && strcmp ( simlock_stat, "0" ) == 0 && cur_cfun_state == 0 && strcmp ( radio_stat, "on" ) == 0 )
        {
            COMD_AT_PROCESS ( "AT+CFUN=1", 3000, at_rep, sizeof ( at_rep ) );
        }


        mtkParam_updateCesq ();
        nv_get ( "cesq", cesq_value, sizeof ( cesq_value ) );

        if ( strcmp ( cesq_value, "" ) && strcmp ( cesq_value, "99,99,255,255,255,255,255,255,255" ) )
        {
            mtkParam_updateCereg ();
C5GREG_LOOP:
            mtkParam_updateCops ();
            mtkParam_updateKcops ();
            mtkParam_updateServNeighInfo ( NULL );
            mtkParam_updatePccSccInfo ( NULL );
            mtkParam_updateSignalBar ();
            mtkParam_updateCgatt ();
            mtkParam_updateCGDCONT ( "no_parse" );
            mtkParam_updateRrcState ();

            nv_get ( "cgatt_val", cgatt_value, sizeof ( cgatt_value ) );
            CLOGD ( FINE, "cgatt_value -> [%s]\n", cgatt_value );
            if ( 0 == strcmp ( cgatt_value, "1" ) )
            {
                if ( 0 == strcmp ( volte_enable, "VOLTE" ) )
                {
                    mtkParam_updateCireg ();
                }

                if ( 0 == strcmp ( simlock_stat, "1" ) )
                {
#if defined (CONFIG_SW_STC_SIMLOCK)
                    if ( 0 != strcmp ( simlock_enable, "0" ) )
                    {
                        mtkParam_setCfunMode ( "off" );//disable scan
                        CLOGD ( WARNING, "MTK module have locked invalid usim.\n" );
                    }
#else
                    mtkParam_disconnectNetwork ( NULL );
#endif
                    return;
                }

                nv_set ( "main_status", "connected" );
                apnActOrDeactMsgEvent ( 0, 1 );

                nv_get ( "connected_time", time_str, sizeof( time_str ) );
                CLOGD ( FINE, "connected_time -> [%s]\n", time_str );
                if ( 0 == strcmp ( time_str, "0" ) )
                {
                    struct sysinfo info;
                    sysinfo ( &info );
                    snprintf ( time_str, sizeof ( time_str ), "%ld", info.uptime );
                    nv_set ( "connected_time", time_str );
                    CLOGD ( FINE, "update connected_time -> [%s]\n", time_str );
                }


                if ( 1 == CONFIG_SW_SMS )
                {
                    mtkParam_updateCmglInfo ();
                }
#if defined(MTK_T750) && CONFIG_QUECTEL_T750
                // mtkParam_updateCgcontrdp ( 0 );
#else
                int multi_apn_act = 0;
#endif
                for ( i = 1; i <= 4; i++ )
                {
                    snprintf ( strUciOpt, sizeof ( strUciOpt ), "apn%d_enable", i );
                    memset ( apn_enable, 0, sizeof ( apn_enable ) );
                    nv_get ( strUciOpt, apn_enable, sizeof ( apn_enable ) );
                    CLOGD ( FINE, "%s -> [%s]\n", strUciOpt, apn_enable );

#if defined(MTK_T750) && CONFIG_QUECTEL_T750
                    mtkParam_setupDataCall ( i, strcmp ( apn_enable, "1" ) ? 1 : 0 );
#else
                    snprintf ( strRamOpt, sizeof ( strRamOpt ), "cid_%d_state", i );
                    memset ( apn_act_state, 0, sizeof ( apn_act_state ) );
                    nv_get ( strRamOpt, apn_act_state, sizeof ( apn_act_state ) );
                    CLOGD ( FINE, "%s -> [%s]\n", strRamOpt, apn_act_state );

                    if ( 0 == strcmp ( apn_act_state, "1" ) )
                    {
                        if ( 0 == strcmp ( apn_enable, "1" ) )
                        {
                            snprintf ( ram_val, sizeof ( ram_val ), "apn%d_netdev", i );
                            snprintf ( apn_netdev, sizeof ( apn_netdev ), "eth1.10%d", i - 1 );
                            nv_set ( ram_val, apn_netdev );

                            ret = mtkParam_updateIPV4V6 ( i );

                            if ( 0 < ret )
                                apnActOrDeactMsgEvent ( i, 1 );
                        }
                        else
                        {
                            mtkParam_configCgact ( i, "0" );
                        }
                    }
                    else
                    {
                        parsing_mtk_ipv4v6 ( i, "" );
                        apnActOrDeactMsgEvent ( i, 0 );
                        if ( 0 == strcmp ( apn_enable, "1" ) )
                        {
                            if ( 0 == multi_apn_act )
                            {
                                comd_wait_ms ( 1500 );
                                multi_apn_act = 1;
                                mtkParam_configCgact ( i, "1" );
                            }
                        }
                    }
#endif
                }
            }
            else
            {
                if ( 0 == strcmp ( simlock_stat, "0" ) &&
                    strcmp ( radio_stat, "off" ) &&
                    strcmp ( ram_conn_mode, "manual_select" ) )
                {
                    mtkParam_connectNetwork ( NULL );
                }

                apnActOrDeactMsgEvent ( 0, 0 );
                if ( strcmp ( mStatusVal, "disconnected" ) )
                {
                    nv_set ( "main_status", "disconnected" );
                    goto END_S;
                }
            }
        }
        else
        {
            mtkParam_updateC5greg ();
            nv_get ( "c5greg_stat", c5greg_stat, sizeof ( c5greg_stat ) );
            if ( 0 == strcmp ( c5greg_stat, "1" ) || 0 == strcmp ( c5greg_stat, "5" ) )
            {
                goto C5GREG_LOOP;
            }

            if ( 0 == strcmp ( simlock_stat, "0" ) &&
                strcmp ( radio_stat, "off" ) &&
                strcmp ( ram_conn_mode, "manual_select" ) )
            {
                mtkParam_connectNetwork ( NULL );
            }

            apnActOrDeactMsgEvent ( 0, 0 );
            if(strcmp ( ram_conn_mode, "manual_select" ) == 0)
            {
                if ( strcmp ( mStatusVal, "disconnected" ) )
                {
                    nv_set ( "main_status", "disconnected" );
                    goto END;
                }
            }
            else if(strcmp ( radio_stat, "off" ) == 0)
            {
                if ( strcmp ( mStatusVal, "no_service" ) )
                {
                    nv_set ( "main_status", "no_service" );
                    goto END;
                }
            }
            else
            {
                if ( strcmp ( mStatusVal, "searching" ) )
                {
                    nv_set ( "main_status", "searching" );
                    goto END;
                }
            }
        }
    }
    else if ( 0 == strcmp ( cpin_value, "SIM PIN" ) )
    {
#if defined (CONFIG_SW_STC_PINLOCK)
        if ( 0 != strcmp ( pinlock_enable, "0" ) )
        {
            //Query the ICCID of the SIM card and compare it with the ICCID saved in the flash
            mtkParam_updateIccid ();
            nv_get ( "iccid", iccid, sizeof ( iccid ) );
            comd_exec ( FACTORY_STC_ICCID, factory_iccid, sizeof ( factory_iccid ) );

            if ( 0 == strcmp ( factory_iccid, "" ) || 0 != strcmp ( iccid, factory_iccid ) )
            {
                nv_set ( "pinlocked", "1" );
                CLOGD ( FINE, "pinlocked is ERROR !\n" );
                goto END;
            }

            memset ( pinlocked, 0, sizeof ( pinlocked ) );
            nv_get ( "pinlocked", pinlocked, sizeof ( pinlocked ) );
            if ( 0 != strcmp ( pinlocked, "1" ) )
            {
                comd_exec ( FACTORY_STC_PIN_CODE, pinlock_code, sizeof ( pinlock_code ) );
                CLOGD ( FINE, "pinlock_code [%s]\n", pinlock_code );

                if ( 0 == strcmp ( pinlock_code, "" ) )
                {
                    nv_set ( "pinlocked", "1" );
                    CLOGD ( FINE, "pinlocked is ERROR !\n" );
                    goto END;
                }
                else
                {
                    mtkParam_enterPin ( pinlock_code );

                    memset ( cpin_value, 0, sizeof ( cpin_value ) );
                    nv_get ( "cpin", cpin_value, sizeof ( cpin_value ) );
                    CLOGD ( FINE, "cpin is [%s]\n", cpin_value );

                    if ( 0 == strcmp ( cpin_value, "READY" ) )
                    {
                        nv_set ( "pinlocked", "0" );
                        CLOGD ( FINE, "pinlocked is READY !\n" );
                    }
                    else
                    {
                        nv_set ( "pinlocked", "1" );
                        CLOGD ( FINE, "pinlocked is ERROR !\n" );
                        goto END;
                    }
                }
            }
        }
        else
        {
            apnActOrDeactMsgEvent ( 0, 0 );
            if ( strcmp ( mStatusVal, "need_pin" ) )
            {
                nv_set ( "main_status", "need_pin" );
                goto END;
            }
        }
#else
        apnActOrDeactMsgEvent ( 0, 0 );
        if ( strcmp ( mStatusVal, "need_pin" ) )
        {
            nv_set ( "main_status", "need_pin" );
            goto END;
        }
#endif
    }
    else if ( 0 == strcmp ( cpin_value, "SIM PUK" ) )
    {
        apnActOrDeactMsgEvent ( 0, 0 );
        if ( strcmp ( mStatusVal, "need_puk" ) )
        {
            nv_set ( "main_status", "need_puk" );
            goto END;
        }
    }
    else
    {
        apnActOrDeactMsgEvent ( 0, 0 );
        if ( strcmp ( mStatusVal, "sim_not_ready" ) )
        {
            nv_set ( "main_status", "sim_not_ready" );
            nv_set ( "imsi", "--" );
            nv_set ( "iccid", "--" );
            nv_set ( "SIM_SPN", "--" );
            nv_set ( "phone_number", "--" );
            goto END;
        }
    }

    return;

END:
    lteinfo_data_restore ();
    mtk_sccinfo_data_restort ( 0, 0 );

END_S:
    nv_set ( "connected_time",  "0" );
    nv_set ( "cereg_stat",      "0" );
    nv_set ( "cgatt_val",       "0" );
    nv_set ( "cid_1_state",     "0" );
    nv_set ( "cid_2_state",     "0" );
    nv_set ( "cid_3_state",     "0" );
    nv_set ( "cid_4_state",     "0" );
    nv_set ( "secondary_cell",  "0" );

    system ( "echo Unregistered > /tmp/voice/voice-reg-status.txt" );

#if defined(MTK_T750) && CONFIG_QUECTEL_T750
    CLOGD ( FINE, "[MTK_T750 && CONFIG_QUECTEL_T750] not reset ipv4v6_data here\n" );
#else
    ipv4v6_data_restore ( 0 );
#endif
}

void mtk_install_mode_refresh ()
{
    return;
}

