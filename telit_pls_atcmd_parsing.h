#ifndef _TELIT_PLS_ATCMD_PARSING_H
#define _TELIT_PLS_ATCMD_PARSING_H 1

int parsing_telit_pls_cfun_set ( char* data );
int parsing_telit_pls_moduleModel ( char* data );
int parsing_telit_pls_moduleVersion ( char* data );
int parsing_telit_pls_imei ( char* data );
int parsing_telit_pls_sn ( char* data );
void parsing_telit_pls_cpin_get ( char* data );
void parsing_telit_pls_clck_get ( char* data );
void parsing_telit_pls_spic ( char* data );
void parsing_telit_pls_cimi ( char* data );
void parsing_telit_pls_sim_spn ( char* data );
void parsing_telit_pls_simMncLen ( char* data );
void parsing_telit_pls_iccid ( char* data );
void parsing_telit_pls_cnum ( char* data );
void parsing_telit_pls_cesq( char* data );
void parsing_telit_pls_operator ( char* data );
int parsing_telit_pls_cgatt ( char* data );
void parsing_telit_pls_serving_cellinfo ( char* data );
void parsing_telit_pls_neighbour_cell( char* data, const char* ca_mode, int* neigbour_count );
int parsing_telti_pls_cereg ( char* data );
void parsing_telit_pls_serving_4gcellinfo ( char* data );
void parsing_telit_pls_cgact ( char* data );
int parsing_telit_pls_cgcontrdp ( int cid_index, char* at_rep );
void parsing_telit_pls_lockpin ( char* data );
void parsing_telit_pls_enterpin ( char* data );
void parsing_telit_pls_modifypin ( char* data );
void parsing_telit_pls_enterpuk ( char* data );
void parsing_telit_pls_cclk ( char* data );
void parsing_telit_pls_csq ( char* data );
void parsing_telit_pls_smonp (char* data);
int parsing_telit_pls_cfun_get ( char* data );

#endif
