#ifndef _TELIT_ATCMD_PARSING_H
#define _TELIT_ATCMD_PARSING_H 1

int parsing_telit_cfun_set ( char* data );
int parsing_telit_moduleModel ( char* data );
int parsing_telit_moduleVersion ( char* data );
int parsing_telit_imei ( char* data );
int parsing_telit_sn ( char* data );
int parsing_telit_bandlist( char* data, char* lte_band_list, int size_band_4g, char* nr_band_list, int size_band_5g );
void parsing_telit_cpin_get ( char* data );
void parsing_telit_clck_get ( char* data );
void parsing_telit_cpinr ( char* data );
void parsing_telit_cimi ( char* data );
void parsing_telit_sim_spn ( char* data );
void parsing_telit_simMncLen ( char* data );
void parsing_telit_iccid ( char* data );
void parsing_telit_cnum ( char* data );
void parsing_telit_cesq( char* data );
void parsing_telit_operator ( char* data );
int parsing_telit_cgatt ( char* data );
void parsing_telit_serving_cellinfo ( char* data );
void parsing_telit_neighbour_cell( char* data, const char* ca_mode, int* neigbour_count );
int parsing_telti_cereg ( char* data );
int parsing_telti_c5greg ( char* data );
void parsing_telit_serving_4gcellinfo ( char* data );
int parsing_telit_serving_5gcellinfo_basic ( char* data );
int parsing_telit_serving_5gcellinfo_adv ( char* data );
void parsing_telit_cgact ( char* data );
int parsing_telit_cgcontrdp ( int cid_index );
void parsing_telit_lockpin ( char* data );
void parsing_telit_enterpin ( char* data );
void parsing_telit_modifypin ( char* data );
void parsing_telit_enterpuk ( char* data );
void parsing_telit_cclk ( char* data );
void parsing_telit_serving_5g_linkstat ( char* data );
int parsing_telit_cfun_get ( char* data );

#endif
