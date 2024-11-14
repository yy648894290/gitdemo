#ifndef _QUECTEL_ATCMD_PARSING_H
#define _QUECTEL_ATCMD_PARSING_H 1

int parsing_quectel_imei ( char* data );
int parsing_quectel_sn ( char* data );
int parsing_quectel_moduleModel ( char* data );
int parsing_quectel_moduleVersion ( char* data );
void parsing_quectel_iccid ( char* data );
void parsing_quectel_cnum ( char* data );
int parsing_quectel_apn ( char* data );
void parsing_quectel_apnAndAuth ( int cid_index, char* data );
int parsing_quectel_supp4gband ( char* data );
int parsing_quectel_supp5gband ( char* data );
void parsing_quectel_cimi ( char* data );
void parsing_quectel_operator ( char* data );
void parsing_quectel_lockpin ( char* data );
void parsing_quectel_enterpin ( char* data );
void parsing_quectel_modifypin ( char* data );
void parsing_quectel_enterpuk ( char* data );
int parsing_quectel_cfun_set ( char* data );
void parsing_quectel_cpin_get ( char* data );
void parsing_quectel_clck_get ( char* data );
void parsing_quectel_qpinc ( char* data );
int parsing_quectel_cgcontrdp ( int cid_index, char* data );
int parsing_quectel_creg ( char* data );
int parsing_quectel_c5greg ( char* data );
int parsing_quectel_csq ( char* data );
int parsing_quectel_cgatt ( char* data );
void parsing_quectel_cgact ( char* data );
void parsing_quectel_rrc_state ( char* data );
int parsing_quectel_neighbor_cellinfo ( char* data );
void parsing_quectel_serving_cellinfo ( char* data );
void parsing_quectel_simMncLen ( char* data );
void parsing_quectel_cclk ( char* data );
void parsing_quectel_qimscfg ( char* data );
void parsing_quectel_ethrgmii ( char* data );
int parsing_quectel_qlogmask ( char* data );
void parsing_quectel_csiinfo ( char* data );
void parsing_quectel_qcainfo ( char* data );
void parsing_quectel_sim_spn ( char* data );
int parsing_quectel_NR5Gtxpower ( char* data );
int parsing_quectel_cfun_get ( char* data );

#endif
