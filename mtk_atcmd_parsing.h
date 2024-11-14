#ifndef _MTK_ATCMD_PARSING_H
#define _MTK_ATCMD_PARSING_H 1

int parsing_mtk_imei ( char* data );
int parsing_mtk_sn ( char* data );
int parsing_mtk_moduleModel ( char* data );
int parsing_mtk_moduleVersion ( char* data );
int parsing_mtk_apn ( char* data );
void parsing_mtk_apnAndAuth ( int cid_index, char* data );
int parsing_mtk_supp4gband ( char* data );
int parsing_mtk_supp5gband ( char* data );
void parsing_mtk_cimi ( char* data );
void parsing_mtk_sim_spn ( char* data );
void parsing_mtk_iccid ( char* data );
void parsing_mtk_operator ( char* data );
void parsing_mtk_ktoperator ( char* data );
void parsing_mtk_lockpin ( char* data );
void parsing_mtk_enterpin ( char* data );
void parsing_mtk_modifypin ( char* data );
void parsing_mtk_enterpuk ( char* data );
int parsing_mtk_cfun_set ( char* data );
void parsing_mtk_cpin_get ( char* data );
void parsing_mtk_clck_get ( char* data );
void parsing_mtk_qpinc ( char* data );
int parsing_mtk_ipv4v6 ( int cid_index, char* data );
int parsing_mtk_cereg ( char* data );
int parsing_mtk_c5greg ( char* data );
int parsing_mtk_cireg ( char* data );
int parsing_mtk_cesq ( char* data );
int parsing_mtk_cgatt ( char* data );
void parsing_mtk_cgact ( char* data );
int parsing_mtk_neighbor_cellinfo ( char* data );
void parsing_mtk_serving_cellinfo ( char* data );
void parsing_mtk_simMncLen ( char* data );
void parsing_mtk_cclk ( char* data );
void mtk_sccinfo_data_restort ( int lte_scc_num, int nr_scc_num  );
void parsing_mtk_edmfapp_six_four ( char* data );
void parsing_mtk_edmfapp_six_ten ( char* data );
void parsing_mtk_emtsi ( char* data );
int parsing_mtk_cfun_get ( char* data );
void parsing_mtk_supportband ( char* data );
int parsing_mtk_rrc_state ( char* data );
void parsing_mtk_cnum ( char* data );
void parsing_mtk_cpms_get ( char* data );
void parsing_mtk_cmgl_four_sim_card ( char* data );
int parsing_mtk_cmgl_zero ( char* data );
void parsing_mtk_csca_get ( char* data );
void parsing_mtk_cpbs ( char* data );
void parsing_mtk_cpbr ( char* data );
void parsing_mtk_gcmgs ( char* data );

#endif
