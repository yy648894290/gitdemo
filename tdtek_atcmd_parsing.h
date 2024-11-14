#ifndef _TDTEK_ATCMD_PARSING_H
#define _TDTEK_ATCMD_PARSING_H 1

int parsing_tdtek_imei ( char* data );
int parsing_tdtek_sn ( char* data );
int parsing_tdtek_moduleModel ( char* data );
int parsing_tdtek_moduleVersion ( char* data );
int parsing_tdtek_apn ( char* data );
void parsing_tdtek_cimi ( char* data );
void parsing_tdtek_sim_spn ( char* data );
void parsing_tdtek_iccid ( char* data );
void parsing_tdtek_operator ( char* data );
void parsing_tdtek_lockpin ( char* data );
void parsing_tdtek_enterpin ( char* data );
void parsing_tdtek_modifypin ( char* data );
void parsing_tdtek_enterpuk ( char* data );
int parsing_tdtek_cfun_set ( char* data );
void parsing_tdtek_cpin_get ( char* data );
void parsing_tdtek_clck_get ( char* data );
void parsing_tdtek_cpinr ( char* data );
int parsing_tdtek_cereg ( char* data );
int parsing_tdtek_c5greg ( char* data );
int parsing_tdtek_cireg ( char* data );
int parsing_tdtek_csq ( char* data );
int parsing_tdtek_cgatt ( char* data );
void parsing_tdtek_cgact ( char* data );
int parsing_tdtek_neighbor_cellinfo ( char* data );
void parsing_tdtek_serving_cellinfo ( char* data );
void parsing_tdtek_simMncLen ( char* data );
void parsing_tdtek_cclk ( char* data );
void parsing_tdtek_nwtime ( char* data );

void parsing_tdtek_dhcpv4_info ( int cid_index, char* data );
void parsing_tdtek_dhcpv6_info ( int cid_index, char* data );

void parsing_tdtek_rrc_state ( char* data );
void parsing_tdtek_hfreqinfo ( char* data );
void parsing_tdtek_hcsq ( char* data );
void parsing_tdtek_ulmcs ( char* data );
void parsing_tdtek_dlmcs ( char* data );
void parsing_tdtek_4g_txpower ( char* data );
void parsing_tdtek_5g_txpower ( char* data );

int parsing_tdtek_cfun_get ( char* data );

#endif
