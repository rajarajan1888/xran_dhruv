/* 
 * xranc_interface.h
 *
 * Created on: Oct 25, 2016
 *     Author: araja
*/

#ifndef _XRANC_INTERFACE_H

#define _XRANC_INTERFACE_H 

#include <sys/types.h>

#define MAX_MCC_DIGITS 3
#define MAX_MNC_DIGITS 3
#define MAX_CELL_IDENTITY_OCTETS  4

/* Module Ids */
#define XRANC  101
#define ENB_CP_AGENT 102

typedef enum
{
    FALSE = 0,
    TRUE
}bool_et;


typedef unsigned char   uint8;
typedef signed   char   int8;
typedef unsigned short  uint16;
typedef unsigned int    uint32;

typedef struct _cell_plmn_info_t
{
    uint8   mcc[MAX_MCC_DIGITS]; 
    uint8   num_mnc_digit; 
    uint8   mnc[MAX_MNC_DIGITS]; 
}cell_plmn_info_t;

typedef struct _eutran_global_cell_id_t
{
    cell_plmn_info_t   primary_plmn_id; 
    uint8   cell_identity[MAX_CELL_IDENTITY_OCTETS];  
} eutran_global_cell_id_t;

typedef enum
{
   FDD = 0, 
   TDD = 1,
} duplex_mode_et; 

/* API Header structure to be prepended before all the API payload exchanged between xRANC and RRM */
typedef struct
{
   uint8   src_id;     /* module id of producer i.e. XRANC or ENBRRM */
   uint8   dst_id;     /* module id of consumer i.e. XRANC or ENBRRM */ 
   uint16  api_id;     /* xranc_api_id_et */
   uint8   trans_id;   /* xranc transaction id */
   uint8   xranc_api_ver_num; /* xRANc version number */
   uint16  msg_len;    /* Octet length of meassge payload+this API header */ 
} xranc_msg_api_hdr_t;

/* Enum for API Ids exchanged between xRANc and RRM */
typedef enum
{
   XRANC_CELL_CONFIG_REQUEST       = 1,
   XRANC_CELL_CONFIG_REPORT        = 2,
   XRANC_UE_ADMISSION_REQUEST      = 3,
   XRANC_UE_ADMISSION_RESPONSE     = 4,
   XRANC_UE_ADMISSION_STATUS       = 5,
   XRANC_UE_RECONFIG_IND           = 6,
   XRANC_UE_RELEASE_IND            = 7,
   XRANC_HANDOFF_REQUEST           = 8,
   XRANC_HANDOFF_FAILURE           = 9,
   XRANC_HANDOFF_COMPLETE          = 10,
   XRANC_BEARER_ADMISSION_REQUEST  = 11,
   XRANC_BEARER_ADMISSION_RESPONSE = 12,
   XRANC_BEARER_ADMISSION_STATUS   = 13,
   XRANC_BEARER_RELEASE_IND        = 14,
   XRANC_RX_SIGNAL_MEAS_CONFIG     = 15,
   XRANC_RX_SIGNAL_MEAS_REPORT     = 16,
   XRANC_L2_MEAS_CONFIG            = 17,
   XRANC_RADIO_MEAS_REPORT_PER_UE  = 18,
   XRANC_RADIO_MEAS_REPORT_PER_CELL= 19,
   XRANC_SCHED_MEAS_REPORT_PER_UE  = 20,
   XRANC_SCHED_MEAS_REPORT_PER_CELL= 21,
   XRANC_PDCP_MEAS_REPORT_PER_UE   = 22,
   XRANC_xICIC_CONFIG              = 23,
   XRANC_xICIC_RESPONSE            = 24, 
} xranc_api_id_et;


/* API Id: XRANC_CELL_CONFIG_REQUEST
   Dir: xRANc -> Cell 
   Usage: xRANc invokes this API to obtain message request */
typedef struct
{
    eutran_global_cell_id_t ecgi;   /* PLMN id+ Eutran Cell ID */ 
} cell_config_request_m; 

/* API Id: XRANC_CELL_CONFIG_REPORT
   Dir: Cell -> xRANc 
   Usage: Cell invokes this API to report configuration */
typedef struct
{ 
   eutran_global_cell_id_t ecgi;   /* PLMN id+ Eutran Cell ID */ 
   uint16   pci; 
   uint32   earfcn_dl; 
   uint32   earfcn_ul;
 
   uint16   rbs_per_tti_dl;
   uint16   rbs_per_tti_ul; 

   uint8    num_tx_antennas; 
   uint8    duplex_mode; 
   uint8    tdd_config; 
   uint8    tdd_spl_config; 
   uint16   max_num_connected_ues;
   uint16   max_num_bearers;
   uint16   max_num_ues_sched_per_tti_dl;
   uint16   max_num_ues_sched_per_tti_ul;
   uint8    dlfs_sched_enable;

} cell_config_report_m;

/* API Id: XRANC_UE_ADMISSION_REQUEST
   Dir: Cell -> xRANc 
   Usage: Cell invokes this API for UE Admission */

typedef enum
{
   ADM_CAUSE_EMERGENCY = 0,
   ADM_CAUSE_HIGHP_ACCESS = 1, 
   ADM_CAUSE_MT_ACCESS = 2, 
   ADM_CAUSE_MO_SIGNALING = 3, 
   ADM_CAUSE_MO_DATA = 4, 
} adm_est_cause_et;


typedef struct
{
   uint16 crnti;
   eutran_global_cell_id_t ecgi;
   adm_est_cause_et adm_est_cause;
} ue_admission_request_m;

/* API Id: XRANC_UE_ADMISSION_RESPONSE
   Dir: xRANc -> Cell 
   Usage: xranc invokes this API for UE Admission response*/

typedef enum
{
   ADM_RESPONSE_SUCCESS = 0,
   ADM_RESPONSE_FAILURE = 1, 
} adm_est_response_et;

typedef struct
{
   uint16 crnti;
   eutran_global_cell_id_t ecgi;
   adm_est_response_et adm_est_response;
} ue_admission_response_m;

/* API Id: XRANC_UE_ADMISSION_STATUS
   Dir: Cell -> xRANc */

typedef enum
{
   ADM_STATUS_SUCCESS = 0,
   ADM_STATUS_FAILURE = 1, 
} adm_est_status_et;

typedef struct
{
   uint16 crnti;
   eutran_global_cell_id_t ecgi;
   adm_est_status_et adm_est_status;
} ue_admission_status_m;

/* API Id: XRANC_UE_RECONFIG_IND
   Dir: Cell -> xRANc */

typedef enum
{
   RECONFIG_IND_RLF = 0,
   RECONFIG_IND_HO_FAIL = 1,
   RECONFIG_IND_OTHERS = 2, 
} reconfig_ind_reason_et;

typedef struct
{
   eutran_global_cell_id_t ecgi;
   uint16 crnti_old;
   uint16 crnti_new;
   reconfig_ind_reason_et reconfig_ind_reason;
} ue_reconfig_ind_m;

/* API Id: XRANC_UE_RELEASE_IND
   Dir: Cell -> xRANc */

typedef enum
{
   UE_REL_INACTIVITY = 0,
   UE_REL_RLF = 1,
   UE_REL_OTHER = 2,  
} ue_rel_reason_et;

typedef struct
{
   uint16 crnti;
   eutran_global_cell_id_t ecgi;
   ue_rel_reason_et ue_rel_reason;
} ue_release_ind_m;

/* API Id: XRANC_BEARER_ADMISSION_REQUEST
   Dir: Cell -> xRANc */

#define MAX_NUM_ERABS 11

typedef struct
{
   uint16 ue_ambr_dl;
   uint16 ue_ambr_ul;
} ue_ambr_t;

typedef enum
{
   ERAB_DL = 0,
   ERAB_UL = 1,
   ERAB_BOTH = 2,
} erab_direction_et;

typedef enum 
{
   ERAB_DEFAULT = 0,
   ERAB_DEDICATED = 1,
} erab_type_et;

typedef struct 
{
   uint16 erab_id;
   erab_direction_et erab_direction;
   erab_type_et      erab_type;
   uint16 qci;
   uint16 arp;
   uint16 gbr_dl;
   uint16 gbr_ul;
   uint16 mbr_dl;
   uint16 mbr_ul;
} erabs_params_t;

typedef struct
{
   uint16 crnti;
   eutran_global_cell_id_t ecgi;
   ue_ambr_t ue_ambr; 
   uint16 num_erabs;
   erabs_params_t erabs_params[MAX_NUM_ERABS];
} bearer_admission_request_m;

/* API Id: XRANC_BEARER_ADMISSION_RESPONSE
   Dir: Cell <- xRANc */

typedef enum
{
   ERABS_DECISION_SUCCESS = 0,
   ERABS_DECISION_FAIL  = 1,
} erabs_decision_et;

typedef struct 
{
   uint16 erab_id;
   erabs_decision_et erabs_decision;
} erabs_response_t;


typedef struct
{
   uint16 crnti;
   eutran_global_cell_id_t ecgi;
   uint16 num_erabs;
   erabs_response_t erabs_response[MAX_NUM_ERABS];
} bearer_admission_response_m;

/* API Id: XRANC_BEARER_ADMISSION_STATUS
   Dir: Cell -> xRANc */

typedef struct
{
   uint16 crnti;
   eutran_global_cell_id_t ecgi;
   uint16 num_erabs;
   erabs_response_t erabs_response[MAX_NUM_ERABS];
} bearer_admission_status_m;

/* API Id: XRANC_BEARER_RELEASE_IND
   Dir: Cell -> xRANc */


typedef struct
{
   uint16 crnti;
   eutran_global_cell_id_t ecgi;
   uint16 num_erabs;
   uint16 erab_ids[MAX_NUM_ERABS];
} bearer_release_ind_m;


/* API Id: XRANC_HANDOFF_REQUEST
   Dir: xRANc -> Cell */
typedef struct
{
   uint16 crnti;
   eutran_global_cell_id_t ecgi_s; 
   eutran_global_cell_id_t ecgi_t;
} handoff_request_m;

/* API Id: XRANC_HANDOFF_FAILURE
   Dir: xRANc -> Cell */

typedef enum
{
   HO_FAIL_OTHER = 0,  
} ho_failure_cause_et;

typedef struct
{
   uint16 crnti;
   eutran_global_cell_id_t ecgi_s; 
   ho_failure_cause_et ho_failure_cause;
} handoff_failure_m;


/* API Id: XRANC_HANDOFF_COMPLETE
   Dir: Cell -> xRANc */
typedef struct
{
   eutran_global_cell_id_t ecgi_t;
   eutran_global_cell_id_t ecgi_s;
   uint16 crnti_new;
} handoff_complete_m;

/* API Id: XRANC_RX_SIGNAL_MEAS_CONFIG
   Dir: xRANC -> Cell */

typedef enum
{
   meas_qty_RSRP = 0, 
   meas_qty_RSRQ = 0, 
   meas_qty_BOTH = 2,
} rx_signal_meas_qty_et;


typedef struct
{
   eutran_global_cell_id_t ecgi;
   uint16 crnti;
   rx_signal_meas_qty_et trigger_qty;
   rx_signal_meas_qty_et report_qty; 
   uint8 max_reported_cells; 
   uint32 report_interval_ms;
} rx_signal_meas_config_m;

/* API Id: XRANC_RX_SIGNAL_MEAS_REPORT
   Dir: xRANC <- Cell */

#define MAX_MEASURED_NCELLS 5

typedef struct 
{
   eutran_global_cell_id_t ecgi;
   uint8 rsrp;
   uint8 rsrq;
} rx_signal_report_t;

typedef struct 
{
   uint16 crnti;
   rx_signal_report_t scell_meas_report;
   uint8 num_ncells; 
   rx_signal_report_t ncell_meas_report[MAX_MEASURED_NCELLS];
} rx_signal_meas_report_m;

/* API Id: XRANC_L2_MEAS_CONFIG
   Dir: xRANC -> Cell */

typedef struct
{
   eutran_global_cell_id_t ecgi;
   uint32 report_interval_ms;
} l2_meas_config_m;

/* API Id: XRANC_RADIO_MEAS_REPORT_PER_UE
   Dir: xRANC <- Cell */

#define MAX_NUM_CQIs 16
#define MAX_NUM_RIs 2
#define SINR_HIST_SIZE 14
#define INVALID_MEAS_VALUE 0xffff

typedef struct 
{
   eutran_global_cell_id_t ecgi;
   uint16 crnti;
   uint16 cqi_hist[MAX_NUM_CQIs];
   uint16 ri_hist[MAX_NUM_RIs];
   uint16 pusch_sinr_hist[SINR_HIST_SIZE];
   uint16 pucch_sinr_hist[SINR_HIST_SIZE];

} radio_meas_report_per_ue_m;

/* API Id: XRANC_RADIO_MEAS_REPORT_PER_CELL
   Dir: xRANC <- Cell */

#define INTF_PWR_HIST_SIZE 17

typedef struct 
{
   eutran_global_cell_id_t ecgi;
   uint16 pusch_intf_power_hist[INTF_PWR_HIST_SIZE];
   uint16 pucch_intf_power_hist[INTF_PWR_HIST_SIZE];

} radio_meas_report_per_cell_m;

/* API Id: XRANC_SCHED_MEAS_REPORT_PER_UE
   Dir: xRANC <- Cell */

#define MAX_NUM_SUPPORTED_QCI 17

typedef struct 
{
   eutran_global_cell_id_t ecgi;
   uint16 crnti;
   uint16 prb_usage_dl[MAX_NUM_SUPPORTED_QCI];
   uint16 prb_usage_ul[MAX_NUM_SUPPORTED_QCI];
   uint16 mcs_dl[MAX_NUM_SUPPORTED_QCI];
   uint16 num_sched_ttis_dl[MAX_NUM_SUPPORTED_QCI];
   uint16 mcs_ul[MAX_NUM_SUPPORTED_QCI];
   uint16 num_sched_ttis_ul[MAX_NUM_SUPPORTED_QCI];
   uint16 rank_dl[MAX_NUM_SUPPORTED_QCI][2];

} sched_meas_report_per_ue_m;

/* API Id: XRANC_SCHED_MEAS_REPORT_PER_CELL
   Dir: xRANC <- Cell */

typedef struct 
{
   eutran_global_cell_id_t ecgi;
   uint16 prb_usage_dl[MAX_NUM_SUPPORTED_QCI];
   uint16 prb_usage_ul[MAX_NUM_SUPPORTED_QCI];

} sched_meas_report_per_cell_m;

/* API Id: XRANC_PDCP_MEAS_REPORT_PER_UE
   Dir: xRANC <- Cell */

typedef struct 
{
   eutran_global_cell_id_t ecgi;
   uint16 crnti;
   uint16 data_vol_dl[MAX_NUM_SUPPORTED_QCI];
   uint16 data_vol_ul[MAX_NUM_SUPPORTED_QCI];
   uint16 pkt_delay_dl[MAX_NUM_SUPPORTED_QCI];
   uint16 pkt_discard_rate_dl[MAX_NUM_SUPPORTED_QCI];
   uint16 pkt_loss_rate_dl[MAX_NUM_SUPPORTED_QCI];
   uint16 pkt_loss_rate_ul[MAX_NUM_SUPPORTED_QCI];
   uint16 throughput_dl[MAX_NUM_SUPPORTED_QCI];
   uint16 throughput_ul[MAX_NUM_SUPPORTED_QCI];

} pdcp_meas_report_per_ue_m;

/* API Id: XRANC_xICIC_CONFIG
   Dir: xRANC -> Cell */

#define NUM_SF_IN_FRAME 10 

typedef struct 
{
   eutran_global_cell_id_t ecgi;
   uint16 crnti;
   int8 p_a;
   uint8 start_prb_dl; 
   uint8 end_prb_dl; 
   uint8 sub_frame_pattern_dl[NUM_SF_IN_FRAME];
   int8 p0_ue_pusch;
   uint8 start_prb_ul; 
   uint8 end_prb_ul; 
   uint8 sub_frame_pattern_ul[NUM_SF_IN_FRAME];
} xicic_config_m;

/* API Id: XRANC_xICIC_RESPONSE
   Dir: xRANC <- Cell */

typedef enum{
   xicic_response_SUCCESS = 0,
   xicic_response_FAILURE = 1,
} xicic_response_et;

typedef struct 
{
   eutran_global_cell_id_t ecgi;
   uint16 crnti;
   xicic_response_et xicic_response_dl;
   xicic_response_et xicic_response_ul;
} xicic_response_m;

#endif // ifndef _XRANC_INTERFACE_H
