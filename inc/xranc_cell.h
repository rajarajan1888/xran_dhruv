/*
 * xranc_cell_info.h
 *
 *  Created on: Oct 25, 2016
 *      Author: rsiva
 */

#ifndef INC_XRANC_CELL_H_
#define INC_XRANC_CELL_H_

#include "xranc_interface.h"
#include "xranc_ue.h"
#include <vector>

#define MAX_NUM_UES 320

typedef struct
{
   uint16 prb_usage_dl[MAX_NUM_SUPPORTED_QCI];
   uint16 prb_usage_ul[MAX_NUM_SUPPORTED_QCI];

} cell_scheduler_m;

typedef struct
{
	eutran_global_cell_id_t eci;
	uint16_t num_active_UEs;
	//std::vector<xranc_ue *> attached_UEs;
	xranc_ue **attached_UEs;

	uint16_t def_bearer_tpt;
	uint16_t ded_bearer_tpt;

	uint16_t prb_usage;
	uint16_t qoe;
	uint16_t tot_bearers;
	uint16_t tot_dedbearers;

	cell_scheduler_m cellScheduler;

	cell_config_report_m cellConfig;
	bool cellConfig_rcvd;

    uint32_t rx_signal_meas_report_interval_ms;
    uint32_t l2_meas_report_interval_ms;

    uint16_t pucch_intf_power_hist[INTF_PWR_HIST_SIZE];
    uint16_t pusch_intf_power_hist[INTF_PWR_HIST_SIZE];

} xranc_cell_info_t;


class xranc_cell
{
private:
	xranc_cell_info_t *cell_info;
	//struct sockaddr_storage udp_sockAddr_storage;
	struct sockaddr_in udp_sockAddr;
//	static uint32_t max_num_ues;
public:
	void setRadioMeasurementReport (uint16_t pucch_intf_power_hist[] , uint16_t pusch_intf_power_hist[]);
	xranc_cell (std::string eciString, uint32_t rx_signal_meas_report_interval_ms, uint32_t l2_meas_report_interval_ms);
	void update_info (std::string rrm_address_string , uint32_t rrm_port);
	void setCellConfig (cell_config_report_m *cellConfig_ptr);
	void setCellScheduler (cell_scheduler_m cellScheduler);
	struct sockaddr_in getUdpSockAddr ();
	cell_config_report_m getCellConfig ();
	xranc_cell_info_t *getCellInfo ();
	static uint32_t getMaxNumOfUEsPerCell ();
	void addUE (uint32_t global_ue_id , uint32_t crnti);
	void addUE (xranc_ue *ue_ptr);
	xranc_ue* getUE (uint32_t crnti);
	xranc_ue* removeUE (uint32_t crnti);
	xranc_ue **getAttachedUEList ();
	void setSchedulingMeasurementReport ( uint16_t prb_usage_dl[] , uint16_t prb_usage_ul[] );
	uint16_t getNumActiveUEs ();
};
//uint32_t xranc_cell :: max_num_ues = 320;
#endif /* INC_XRANC_CELL_H_ */
