#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <malloc.h>
#include <pthread.h>
#include <assert.h>
#include <ctime>

#include "../inc/xranc_ue.h"
//#include "../inc/xranc_interface.h"

#include <netdb.h>           // For gethostbyname()
#include <arpa/inet.h>       // For inet_addr()
#include <unistd.h>          // For close()

using namespace std;

//Refer to class xranc_cell in xranc_cell.h

xranc_ue :: xranc_ue (uint32_t crnti , uint32_t global_ue_id)
{
	ue_info = (xranc_ue_info_t *)malloc(sizeof(xranc_ue_info_t));
	ue_info->global_ue_id = global_ue_id;
	ue_info->crnti = crnti;
	ue_info->no_of_bearers = 0;
	ue_info->no_of_dedbearers = 0;
	ue_info->bearer_info =  (xranc_bearer_info_t **)malloc(sizeof(xranc_bearer_info_t *));
//	ue_info->bearer_info = NULL;
//	ue_info->meas_info_nc = new xranc_meas_rep_info_t[NUM_NEIGHBORING_CELLS];
	ue_info->prb_usage = 0;
	ue_info->qoe = 0;
	//cout << "QoE = " << ue_info->qoe << endl;
}

xranc_ue :: ~xranc_ue ()
{
	cout << "Removing UE with CRNTI " << ue_info->crnti << endl;
}

void xranc_ue :: updateBearers (uint16_t num_erabs)
{
	ue_info->no_of_bearers += num_erabs;
	cout << "Total number of bearers = " << ue_info->no_of_bearers << endl;
}

void xranc_ue :: updateDedBearers ( uint16_t num_dedErabs)
{
	ue_info->no_of_dedbearers += num_dedErabs;
	cout << "Total number of dedicated bearers = " << ue_info->no_of_dedbearers << endl;
}

xranc_ue_info_t *xranc_ue :: getUEInfo ()
{
	return ue_info;
}

void xranc_ue :: removeDedBearer ()
{
	ue_info->no_of_dedbearers--;
	cout << "Removing bearer. Total number of remaining dedicated bearers = " << ue_info->no_of_dedbearers << endl;
}

void xranc_ue :: removeBearer ()
{
	ue_info->no_of_bearers--;
	cout << "Removing bearer. Total number of remaining bearers = " << ue_info->no_of_bearers << endl;
}
void xranc_ue :: addBearerInformation (erabs_params_t parameter)
{
	ue_info->bearer_info[ue_info->no_of_bearers] = (xranc_bearer_info_t *)malloc(sizeof(xranc_bearer_info_t));

    ue_info->bearer_info[ue_info->no_of_bearers]->erab_params = parameter;

	if ( parameter.erab_type == ERAB_DEDICATED ) {
		ue_info->no_of_dedbearers++;
   }

	ue_info->no_of_bearers++;
}

uint16_t xranc_ue :: getNumBearers ()
{
	return ue_info->no_of_bearers;
}

uint16_t xranc_ue :: getNumDedBearers ()
{
	return ue_info->no_of_dedbearers;
}

void xranc_ue :: initializeBearerInformation ()
{
	ue_info->bearer_info = (xranc_bearer_info_t **)malloc(sizeof(xranc_bearer_info_t *));
}

void xranc_ue :: setNeighboringCells (uint8_t numNeighboringCells , rx_signal_report_t neighboringCells[])
{
	for ( uint8_t neighboringCellId = 0 ; neighboringCellId < numNeighboringCells ; neighboringCellId++ ) {
		ue_info->meas_info_nc[neighboringCellId].rsrp = neighboringCells[neighboringCellId].rsrp;
		ue_info->meas_info_nc[neighboringCellId].rsrq = neighboringCells[neighboringCellId].rsrq;
	}
}

void xranc_ue :: setRadioMeasurementReport (uint16_t cqi_hist [] , uint16_t ri_hist [] , uint16_t pucch_sinr_hist [] , uint16_t pusch_sinr_hist [] )
{
	for ( uint32_t id = 0 ; id < MAX_NUM_CQIs ; id++ ) {
		ue_info->radio_meas_reports.cqi_hist[id] = cqi_hist[id];
	}

	for ( uint32_t id = 0 ; id < SINR_HIST_SIZE ; id++ ) {
		ue_info->radio_meas_reports.pucch_sinr_hist[id] = pucch_sinr_hist[id];
		ue_info->radio_meas_reports.pusch_sinr_hist[id] = pusch_sinr_hist[id];
	}

	for ( uint32_t id = 0 ; id < MAX_NUM_RIs ; id++ ) {
		ue_info->radio_meas_reports.ri_hist[id] = ri_hist[id];
	}
}

void xranc_ue :: setServingCellSignalValues (rx_signal_report_t servingCellSignal)
{
	ue_info->meas_info_sc.rsrp = servingCellSignal.rsrp;
	ue_info->meas_info_sc.rsrq = servingCellSignal.rsrq;
}

void xranc_ue :: setSchedulingMeasurementReport ( uint16_t mcs_dl[] , uint16_t mcs_ul[] , uint16_t num_sched_ttis_dl[] , uint16_t num_sched_ttis_ul [], uint16_t prb_usage_dl[] , uint16_t prb_usage_ul[] , uint16_t rank_dl[][2])
{
	for ( uint32_t id = 0 ; id < MAX_NUM_SUPPORTED_QCI ; id++ ) {
		ue_info->sched_meas_reports.mcs_dl[id] = mcs_dl[id];
		ue_info->sched_meas_reports.mcs_ul[id] = mcs_ul[id];
		ue_info->sched_meas_reports.num_sched_ttis_dl[id] = num_sched_ttis_dl[id];
		ue_info->sched_meas_reports.num_sched_ttis_ul[id] = num_sched_ttis_ul[id];
		ue_info->sched_meas_reports.prb_usage_dl[id] = prb_usage_dl[id];
		ue_info->sched_meas_reports.prb_usage_ul[id] = prb_usage_ul[id];
		for ( uint32_t j = 0 ; j < 2 ; j++ )
			ue_info->sched_meas_reports.rank_dl[id][j] = rank_dl[id][j];
	}
}

void xranc_ue :: setPdcpMeasurementReport ( uint16_t data_vol_dl[] , uint16_t data_vol_ul[] , uint16_t pkt_delay_dl[], uint16_t pkt_discard_rate_dl[] , uint16_t pkt_loss_rate_dl[] , uint16_t pkt_loss_rate_ul[] , uint16_t throughput_dl[] , uint16_t throughput_ul[] )
{
	for ( uint32_t id = 0 ; id < MAX_NUM_SUPPORTED_QCI ; id++ ) {
		ue_info->pdcp_meas_reports.data_vol_dl[id] = data_vol_dl[id];
		ue_info->pdcp_meas_reports.data_vol_ul[id] = data_vol_ul[id];
		ue_info->pdcp_meas_reports.pkt_delay_dl[id] = pkt_delay_dl[id];
		ue_info->pdcp_meas_reports.pkt_discard_rate_dl[id] = pkt_discard_rate_dl[id];
		ue_info->pdcp_meas_reports.pkt_loss_rate_dl[id] = pkt_loss_rate_dl[id];
		ue_info->pdcp_meas_reports.pkt_loss_rate_ul[id] = pkt_loss_rate_ul[id];
		ue_info->pdcp_meas_reports.throughput_dl[id] = throughput_dl[id];
		ue_info->pdcp_meas_reports.throughput_ul[id] = throughput_ul[id];
	}
}

