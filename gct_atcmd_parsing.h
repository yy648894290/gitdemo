#ifndef _GCT_ATCMD_PARSING_H
#define _GCT_ATCMD_PARSING_H 1

void memsetGctParsingStr();

void parsing_gct_ktlteinfo ( char* data );
void parsing_gct_glteconnstatus ( char* data );
void parsing_gct_gdmitem ( char* data );
int parsing_gct_imei ( char* data );
int parsing_gct_suppband ( char* data );
int parsing_gct_moduleModel ( char* data );
int parsing_gct_moduleType ( char* data );
int parsing_gct_moduleVersion ( char* data );
int parsing_gct_FwVersion ( char* data );
void parsing_gct_lockpin ( char* data );
void parsing_gct_enterpin ( char* data );
void parsing_gct_modifypin ( char* data );
void parsing_gct_enterpuk ( char* data );
void parsing_gct_lteinfo ( char* data );
void parsing_gct_cclk ( char* data );
void parsing_gct_gsimauth ( char* data );
void parsing_gct_gcmgs ( char* data );
void parsing_gct_cmgl_four ( char* data );
void parsing_gct_cmgd ( char* data );
void parsing_gct_cmgr ( char* data );
void parsing_gct_csca_set ( char* data );
void parsing_gct_csca_get ( char* data );
void parsing_gct_cmss ( char* data );
void parsing_gct_cops_plmnSearch ( char* data );
void parsing_gct_cops_select ( char* data );
int parsing_gct_cfun_set ( char* data );
void parsing_gct_txpowlmt ( char* data );
void parsing_gct_cpms_get ( char* data );

void parsing_gct_cpin_get ( char* data );
void parsing_gct_clck_get ( char* data );
void parsing_gct_cimi ( char* data );
void parsing_gct_simMncLen ( char* data );
void parsing_gct_sim_spn ( char* data );
void parsing_gct_iccid ( char* data );
void parsing_gct_cpinr ( char* data );
void parsing_gct_cnum ( char* data );

void parsing_gct_cesq ( char* data );
void parsing_gct_cereg ( char* data );
void parsing_gct_cgatt_get ( char* data );
void parsing_gct_cgact_get ( char* data );

void parsing_gct_gdmmodtc ( char* data );
void parsing_gct_gbler ( char* data );
void parsing_gct_gharq ( char* data );
void parsing_gct_defbrdp ( int cid_index, char* data );

void parsing_gct_gdmcqi ( char* data );
void parsing_gct_grankindex ( char* data );
int parsing_gct_cgcontrdp ( int cid_index, char* data );
void parsing_gct_wanTraffic ( char* data );

#endif
