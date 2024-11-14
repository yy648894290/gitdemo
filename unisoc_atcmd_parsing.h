#ifndef _UNISOC_ATCMD_PARSING_H
#define _UNISOC_ATCMD_PARSING_H 1

int parsing_unisoc_lockpin ( char* data );
int parsing_unisoc_enterpin ( char* data );
int parsing_unisoc_modifypin ( char* data );
int parsing_unisoc_enterpuk ( char* data );
int parsing_unisoc_moduleModel ( char* data );
int parsing_unisoc_moduleVersion ( char* data );
int parsing_unisoc_cfun_set ( char* data );
int parsing_unisoc_imei ( char* data );
int parsing_unisoc_sn ( char* data );
void parsing_unisoc_cpin_get ( char* data );
void parsing_unisoc_clck_get ( char* data );
void parsing_unisoc_qpinc ( char* data );
void parsing_unisoc_cimi ( char* data );
void parsing_unisoc_sim_spn ( char* data );
void parsing_unisoc_iccid ( char* data );
void parsing_unisoc_cnum ( char* data );
int parsing_unisoc_cgdcont ( char* data );
int parsing_unisoc_clcc ( char* data );
void parsing_unisoc_cesq ( char* data );
void parsing_unisoc_operator ( char* data );
void parsing_unisoc_serving_cellinfo ( char* data );
void parsing_unisoc_lte_ulmcs ( char* data );
void parsing_unisoc_lte_dlmcs ( char* data );
void parsing_unisoc_nr5g_ulmcs ( char* data );
void parsing_unisoc_nr5g_dlmcs ( char* data );
void parsing_unisoc_qcainfo ( char* data );
void parsing_unisoc_endc_qcainfo ( char* data );
int parsing_unisoc_cgatt ( char* data );
void parsing_unisoc_cgact ( char* data );
int parsing_unisoc_qnetdevstatus ( int apn_index, char* data );
int parsing_unisoc_supp4gband ( char* data );
int parsing_unisoc_supp5gband ( char* data );
int parsing_unisoc_neighbor_cellinfo ( char* data );
void parsing_unisoc_cclk ( char* data );
void parsing_unisoc_simMncLen ( char* data );
int parsing_unisoc_cfun_get ( char* data );
void unisoc_quectel_csiinfo( char* data );
void parsing_unisoc_cpms_get ( char* data );
void parsing_unisoc_cmgl_four_sim_card ( char* data );
void parsing_unisoc_cpbs ( char* data );
int parsing_unisoc_cmgl_zero ( char* data );
void parsing_unisoc_csca_get ( char* data );
void parsing_unisoc_gcmgs ( char* data );
void parsing_unisoc_cusd_set( char* data, int ret_mode);


#endif
