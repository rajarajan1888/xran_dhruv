/*
 * xranc_ue_info.h
 *
 *  Created on: Oct 25, 2016
 *      Author: rsiva
 */

#ifndef INC_XRANC_UE_H_
#define INC_XRANC_UE_H_

#include "xranc_interface.h"

typedef struct
{
	erabs_params_t erab_params;
} xranc_bearer_info_t;

typedef struct
{
	eutran_global_cell_id_t eci;
	uint32_t rsrp;
	uint32_t rsrq;
} xranc_meas_rep_info_t;

typedef struct
{
	uint16_t cqi_hist[MAX_NUM_CQIs];
	uint16_t ri_hist[MAX_NUM_RIs];
	uint16_t pusch_sinr_hist[SINR_HIST_SIZE];
	uint16_t pucch_sinr_hist[SINR_HIST_SIZE];
} radio_meas_reports_t ;

typedef struct
{
	uint16 prb_usage_dl[MAX_NUM_SUPPORTED_QCI];
	uint16 prb_usage_ul[MAX_NUM_SUPPORTED_QCI];
	uint16 mcs_dl[MAX_NUM_SUPPORTED_QCI];
	uint16 num_sched_ttis_dl[MAX_NUM_SUPPORTED_QCI];
	uint16 mcs_ul[MAX_NUM_SUPPORTED_QCI];
	uint16 num_sched_ttis_ul[MAX_NUM_SUPPORTED_QCI];

	uint16 rank_dl[MAX_NUM_SUPPORTED_QCI][2];
} sched_meas_reports_t ;

typedef struct
{
   uint16 data_vol_dl[MAX_NUM_SUPPORTED_QCI];
   uint16 data_vol_ul[MAX_NUM_SUPPORTED_QCI];
   uint16 pkt_delay_dl[MAX_NUM_SUPPORTED_QCI];
   uint16 pkt_discard_rate_dl[MAX_NUM_SUPPORTED_QCI];
   uint16 pkt_loss_rate_dl[MAX_NUM_SUPPORTED_QCI];
   uint16 pkt_loss_rate_ul[MAX_NUM_SUPPORTED_QCI];
   uint16 throughput_dl[MAX_NUM_SUPPORTED_QCI];
   uint16 throughput_ul[MAX_NUM_SUPPORTED_QCI];

} pdcp_meas_report_t;

typedef struct
{
	uint32_t global_ue_id;
	uint32_t crnti;
	uint16_t no_of_bearers;
	uint16_t no_of_dedbearers;

	xranc_bearer_info_t **bearer_info;
	xranc_meas_rep_info_t meas_info_sc;
	xranc_meas_rep_info_t meas_info_nc[MAX_MEASURED_NCELLS];

	radio_meas_reports_t radio_meas_reports;
	sched_meas_reports_t sched_meas_reports;
	pdcp_meas_report_t pdcp_meas_reports;

	ue_ambr_t ue_ambr;

	int prb_usage;
	int qoe;

} xranc_ue_info_t;

class xranc_ue
{
private:
	xranc_ue_info_t *ue_info;
public:
	xranc_ue (uint32_t crnti , uint32_t global_ue_id);
	~xranc_ue ();
	xranc_ue_info_t *getUEInfo ();
	void updateBearers (uint16_t num_erabs);
	void removeBearer ();
	void removeDedBearer ();
	void updateDedBearers (uint16_t num_dedErabs);
	void initializeBearerInformation ();
	uint16_t getNumBearers ();
	uint16_t getNumDedBearers ();
	void addBearerInformation (erabs_params_t parameter);
	void setNeighboringCells (uint8_t numNeighboringCells , rx_signal_report_t neighboringCells[]);
	void setRadioMeasurementReport (uint16_t cqi_hist [] , uint16_t pucch_sinr_hist [] , uint16_t pusch_sinr_hist [] , uint16_t ri_hist [] );
	void setServingCellSignalValues (rx_signal_report_t servingCellSignal);
	void setPdcpMeasurementReport (uint16_t data_vol_dl [], uint16_t data_vol_ul [], uint16_t pkt_delay_dl [], uint16_t pkt_discard_rate_dl [], uint16_t pkt_loss_rate_dl [], uint16_t pkt_loss_rate_ul [], uint16_t throughput_dl [], uint16_t throughput_ul[] );
	void setSchedulingMeasurementReport ( uint16_t mcs_dl[] , uint16_t mcs_ul[] , uint16_t num_sched_ttis_dl[] , uint16_t num_sched_ttis_ul [], uint16_t prb_usage_dl[] , uint16_t prb_usage_ul[] , uint16_t rank_dl[][2]);
};

#endif /* INC_XRANC_UE_H_ */
