#ifndef _QUECTEL_LTE_ATCMD_PARSING_H
#define _QUECTEL_LTE_ATCMD_PARSING_H 1

int parsing_quectel_lte_cfun_set ( char* data );
int parsing_quectel_lte_cfun_get ( char* data );
int parsing_quectel_lte_moduleModel ( char* data );
int parsing_quectel_lte_moduleVersion ( char* data );
int parsing_quectel_lte_imei ( char* data );
int parsing_quectel_lte_sn ( char* data );
void parsing_quectel_lte_cpin_get ( char* data );
void parsing_quectel_lte_clck_get ( char* data );
void parsing_quectel_lte_qpinc ( char* data );
void parsing_quectel_lte_cimi ( char* data );
void parsing_quectel_lte_iccid ( char* data );
void parsing_quectel_lte_cnum ( char* data );
int parsing_quectel_lte_cgdcont ( char* data );
int parsing_quectel_lte_clcc ( char* data );
void parsing_quectel_lte_simMncLen ( char* data );
void parsing_quectel_lte_sim_spn ( char* data );
int parsing_quectel_lte_csq ( char* data );
int parsing_quectel_lte_creg ( char* data );
int parsing_quectel_lte_cgatt ( char* data );
void parsing_quectel_lte_Qnetdevctl ( char* data );
void parsing_quectel_lte_cgact ( char* data );
void parsing_quectel_lte_operator ( char* data );
void parsing_quectel_lte_lockpin ( char* data );
void parsing_quectel_lte_enterpin ( char* data );
void parsing_quectel_lte_modifypin ( char* data );
void parsing_quectel_lte_enterpuk ( char* data );
void parsing_quectel_lte_rrc_state ( char* data );
void parsing_quectel_lte_serving_cellinfo ( char* data );
int parsing_quectel_lte_neighbor_cellinfo ( char* data );
void parsing_quectel_lte_cclk ( char* data );
void parsing_quectel_lte_qcainfo ( char* data );
int parsing_quectel_lte_cgcontrdp ( int cid_index, char* data );
int parsing_quectel_lte_supp4gband ( char* data );

#endif
