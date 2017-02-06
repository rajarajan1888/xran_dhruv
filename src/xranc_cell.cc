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

//#include "../inc/xranc_interface.h"
#include "../inc/xranc_cell.h"
//#include "../inc/xranc_ue.h"

#include <netdb.h>           // For gethostbyname()
#include <arpa/inet.h>       // For inet_addr()
#include <unistd.h>          // For close()

using namespace std;

//Refer to class xranc_cell in xranc_cell.h

xranc_cell :: xranc_cell (std::string eciString,
      uint32_t rx_signal_meas_report_interval_ms, 
      uint32_t l2_meas_report_interval_ms)
{
	char temp_str[3];
	char *str_p = (char *)malloc(eciString.size());
	strcpy (str_p , eciString.c_str());

	cell_info = (xranc_cell_info_t *)malloc(sizeof(xranc_cell_info_t));

   cell_info->rx_signal_meas_report_interval_ms = rx_signal_meas_report_interval_ms;
   cell_info->l2_meas_report_interval_ms = l2_meas_report_interval_ms;

	strncpy(temp_str, str_p ,2);
	cell_info->eci.primary_plmn_id.mcc[0] = strtol(temp_str, NULL, 16);
	str_p +=2;

	strncpy(temp_str, str_p ,2);
	cell_info->eci.primary_plmn_id.mcc[1] = strtol(temp_str, NULL, 16);
	str_p +=2;

	strncpy(temp_str, str_p ,2);
	cell_info->eci.primary_plmn_id.mcc[2] = strtol(temp_str, NULL, 16);
	str_p +=2;

   cout << "MCC: " << hex << (uint16)cell_info->eci.primary_plmn_id.mcc[0] <<
      ", " << hex << (uint16)cell_info->eci.primary_plmn_id.mcc[1] <<
      ", " << hex << (uint16)cell_info->eci.primary_plmn_id.mcc[2] << endl;

	   // Read num_mnc_digit
	strncpy(temp_str, str_p ,2);
	cell_info->eci.primary_plmn_id.num_mnc_digit = strtol(temp_str, NULL, 16);
	str_p +=2;

	   // Read mnc
	strncpy(temp_str, str_p ,2);
	cell_info->eci.primary_plmn_id.mnc[0] = strtol(temp_str, NULL, 16);
	str_p +=2;

	strncpy(temp_str, str_p ,2);
	cell_info->eci.primary_plmn_id.mnc[1] = strtol(temp_str, NULL, 16);
	str_p +=2;

	strncpy(temp_str, str_p ,2);
	cell_info->eci.primary_plmn_id.mnc[2] = strtol(temp_str, NULL, 16);
	str_p +=2;

   cout << "MNC: " << hex << (uint16)cell_info->eci.primary_plmn_id.mnc[0] <<
      ", " << hex << (uint16)cell_info->eci.primary_plmn_id.mnc[1] <<
      ", " << hex << (uint16)cell_info->eci.primary_plmn_id.mnc[2] << endl;


	   // Read cell_identity
	strncpy(temp_str, str_p ,2);
	cell_info->eci.cell_identity[0] = strtol(temp_str, NULL, 16);
	str_p +=2;
   

	strncpy(temp_str, str_p ,2);
	cell_info->eci.cell_identity[1] = strtol(temp_str, NULL, 16);
	str_p +=2;

	strncpy(temp_str, str_p ,2);
	cell_info->eci.cell_identity[2] = strtol(temp_str, NULL, 16);
	str_p +=2;

	strncpy(temp_str, str_p ,2);
	cell_info->eci.cell_identity[3] = strtol(temp_str, NULL, 16);
	str_p +=2;

   cout << "ECI: " << hex << (uint16)cell_info->eci.cell_identity[0] <<
      ", " << hex << (uint16)cell_info->eci.cell_identity[1] <<
      ", " << hex << (uint16)cell_info->eci.cell_identity[2] <<
      ", " << hex << (uint16)cell_info->eci.cell_identity[3] << endl;

	cell_info->attached_UEs = new xranc_ue *[cell_info->cellConfig.max_num_connected_ues];
	cell_info->num_active_UEs = 0;
	//cell_info->ded_bearer_tpt = 0;
	cell_info->cellConfig_rcvd = FALSE;
}

void xranc_cell :: addUE (uint32_t global_ue_id , uint32_t crnti)
{
	cell_info->num_active_UEs++;
	cout << "Adding new UE with CRNTI " << crnti << " , global UE ID " << global_ue_id << 
      " to cell with ECI: " << hex << *((uint32*)(cell_info->eci.cell_identity)) << endl; 
    cout << "Num of active UEs in the cell = " << cell_info->num_active_UEs << endl;
	xranc_ue *ue_ptr = new xranc_ue (crnti , global_ue_id);

	cell_info->attached_UEs[cell_info->num_active_UEs-1] = ue_ptr;

}

void xranc_cell :: addUE (xranc_ue * ue_ptr)
{
	cell_info->num_active_UEs++;
	cout << "Adding UE with CRNTI " << ue_ptr->getUEInfo()->crnti << " , global UE ID " << ue_ptr->getUEInfo()->global_ue_id << 
      " to cell with ECI: " << hex << *((uint32*)(cell_info->eci.cell_identity)) << endl; 
    cout << "Num of active UEs in the cell = " << cell_info->num_active_UEs << endl;

	cell_info->attached_UEs[cell_info->num_active_UEs-1] = ue_ptr;

}

xranc_ue *xranc_cell :: getUE (uint32_t crnti)
{
	xranc_ue *ueptr = NULL;
	for (uint16_t ueId = 0 ; ueId < cell_info->num_active_UEs ; ueId++ ) {
		if ( cell_info->attached_UEs[ueId]->getUEInfo()->crnti == crnti ) {
			ueptr = cell_info->attached_UEs[ueId];
			break;
		}
	}
	return ueptr;
}

xranc_ue* xranc_cell :: removeUE (uint32_t crnti)
{
	uint16_t userId;
   xranc_ue * removedUE; 

	for ( userId = 0 ; userId < cell_info->num_active_UEs ; userId++ ) {
		if ( cell_info->attached_UEs[userId]->getUEInfo()->crnti == crnti ) {
			removedUE = cell_info->attached_UEs[userId];
         break;
		}
	}

	if (userId < cell_info->num_active_UEs) {
		uint16_t shiftUserId;
		for ( shiftUserId = userId+1 ; shiftUserId < cell_info->num_active_UEs ; shiftUserId++ )
			cell_info->attached_UEs[shiftUserId - 1] = cell_info->attached_UEs[shiftUserId];

	//	delete cell_info->attached_UEs[shiftUserId];
		cell_info->num_active_UEs--;
		cout << "Removing UE with CRNTI " << crnti 
         << " from cell with ECI:" << hex << *((uint32*)(cell_info->eci.cell_identity)) << endl;
      cout << "Num of active UEs in the cell = " << cell_info->num_active_UEs << endl;
		return removedUE;
	} else {
		cout << "ALERT: UE with CRNTI " << crnti << " not found." << endl << endl;
		return NULL;
	}
}

xranc_cell_info_t *xranc_cell :: getCellInfo ()
{
	return cell_info;
}

void xranc_cell :: setCellConfig (cell_config_report_m *cellConfig_ptr)
{
	cell_info->cellConfig = *cellConfig_ptr;
	cell_info->cellConfig_rcvd = TRUE;

   cout << "***************************************" << endl; 
   cout << "Cell Configuration: ECI - " << hex << *((uint32*)(cell_info->eci.cell_identity)) << endl;
   cout << "PCI: " << dec << cell_info->cellConfig.pci << endl <<
      "EARFCN-DL: " << cell_info->cellConfig.earfcn_dl << endl <<
      "EARFCN-UL: " << cell_info->cellConfig.earfcn_ul << endl <<
      "RBs Per TTI-DL: " << cell_info->cellConfig.rbs_per_tti_dl << endl <<
      "RBs Per TTI-UL: " << cell_info->cellConfig.rbs_per_tti_ul << endl <<
      "Num Tx Antennas: " << (uint32_t) cell_info->cellConfig.num_tx_antennas << endl <<
      "Duplex Mode: " << (uint32_t) cell_info->cellConfig.duplex_mode << endl <<
      "Max Connected UEs: " << cell_info->cellConfig.max_num_connected_ues << endl <<
      "Max Num Bearers: " << cell_info->cellConfig.max_num_bearers << endl <<
      "Max Sched Bearers per TTI in DL: " << cell_info->cellConfig.max_num_ues_sched_per_tti_dl << endl <<
      "Max Sched Bearers per TTI in UL: " << cell_info->cellConfig.max_num_ues_sched_per_tti_ul << endl <<
      "DLFS Scheduling enabled: " << (uint32_t) cell_info->cellConfig.dlfs_sched_enable << endl;
   cout << "***************************************" << endl; 
}

cell_config_report_m xranc_cell :: getCellConfig ()
{
	return cell_info->cellConfig;
}
//Refer to class xranc_state in xranc.h

uint32_t xranc_cell :: getMaxNumOfUEsPerCell ()
{
	return MAX_NUM_UES;
}

uint16_t xranc_cell :: getNumActiveUEs ()
{
	return cell_info->num_active_UEs;
}

xranc_ue **xranc_cell :: getAttachedUEList ()
{
	return cell_info->attached_UEs;
}

void xranc_cell :: setRadioMeasurementReport (uint16_t pucch_intf_power_hist[] , uint16_t pusch_intf_power_hist[])
{
	for ( uint32_t id = 0 ; id < INTF_PWR_HIST_SIZE ; id++ ) {
		cell_info->pucch_intf_power_hist[id] = pucch_intf_power_hist[id];
		cell_info->pusch_intf_power_hist[id] = pusch_intf_power_hist[id];
	}
}

void xranc_cell :: setSchedulingMeasurementReport ( uint16_t prb_usage_dl[] , uint16_t prb_usage_ul[] )
{
	for ( uint16_t id = 0 ; id < MAX_NUM_SUPPORTED_QCI ; id++ ) {
		cell_info->cellScheduler.prb_usage_dl[id] = prb_usage_dl[id];
		cell_info->cellScheduler.prb_usage_ul[id] = prb_usage_ul[id];
	}
}
