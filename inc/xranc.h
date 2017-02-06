/*
 * xranc_state.h
 *
 *  Created on: Oct 25, 2016
 *      Author: rsiva
 */

#ifndef INC_XRANC_H_
#define INC_XRANC_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <map>
#include <iostream>
#include <vector>

#include "xranc_cell.h"
#include "xranc_interface.h"
//#include "xranc_ue.h"

using namespace std;

// AR: Keeping the max message size to be 1024 bytes for now. This should be big enough. 
#define MAX_MSG_SIZE 1024
// Defining a default cause for an event
#define DEF_CAUSE 0

class xranc_comm_api
{
   private:
      uint32_t udp_Socket;

      void *rcvd_msg_buf;
      void *send_msg_buf;

      struct sockaddr_in *recv_sockAddr;
      struct sockaddr_in **send_sockAddr;

      //uint8_t  trans_id;
      // std::map < uint8_t , uint16_t , eutran_global_cell_id_t ,  uint8_t , uint8_t > transactionMap;

   public:
      // Function declarations
      xranc_comm_api (uint32_t xranc_port , uint16_t num_active_cells , std::string rrm_ip_address_strings[] , std::string rrm_port_strings[]);
      std::pair<xranc_api_id_et, void*> xranc_rcv_message ();
      bool send_ho_request (xranc_cell *cell , uint32_t cellId , uint32_t crnti , eutran_global_cell_id_t srcCell , eutran_global_cell_id_t targetCell );
      //void xranc_send_handoff_message (xranc_cell *src_cell , xranc_cell *dest_cell , xranc_ue *ue , struct sockaddr_in *eNb_sockAddr);
      uint32_t getUdpSocket ();
      bool xranc_cell_config_request (xranc_cell *cell , uint16_t cellCounter);
      bool xranc_ue_admission_response (xranc_cell *cell , uint16_t cellCounter , uint16_t crnti , adm_est_response_et adm_est_response);
      bool xranc_bearer_admission_response (xranc_cell *cell, uint16_t cellCounter , uint16_t crnti, uint16_t num_erab_list , erabs_response_t *listOfErabsResponses );
      bool xranc_rx_signal_meas_config (xranc_cell *cell , uint16_t cellCounter, uint16_t crnti);
      bool xranc_l2_meas_config (xranc_cell *cell , uint16_t cellCounter);
};

class xranc_state
{
private:
	uint16_t num_active_cells;
	xranc_cell **cells;
	uint32_t total_num_users;
	std::string banner;
public:
	xranc_state (uint16_t num_active_cells);
	void update_cell_info (uint16_t cellId , 
         std::string eciString, 
         uint32_t rx_signal_meas_report_interval_ms, 
         uint32_t l2_meas_report_interval_ms);
	uint16_t getNumActiveCells ();
	xranc_cell *getCell (uint16_t cellId);
	void updateUsers (bool addDelChoice);
	uint32_t getTotalNumUsers ();
};

class event_processor_t
{
public:
	virtual ~event_processor_t ();
	virtual bool processEvent () = 0;
};

class cell_config_processor_t : public event_processor_t
{
public:
	cell_config_processor_t ();
	~cell_config_processor_t ();
	bool processEvent ();
};

class ue_admission_request_processor_t : public event_processor_t
{
	xranc_state *xranc_state_ptr;
	ue_admission_request_m *ue_adm_req_ptr;
public:
	ue_admission_request_processor_t (xranc_state *xranc_state_ptr , ue_admission_request_m *ue_adm_req_ptr);
	~ue_admission_request_processor_t ();
	bool processEvent ();
};

class handover_processor_t : public event_processor_t
{
	xranc_state *xranc_state_ptr;
   handoff_request_m *ho_request;  

public:
	handover_processor_t (xranc_state *xranc_state_ptr , handoff_request_m *ho_request);
	~handover_processor_t ();
	bool processEvent ();
};

typedef struct
{
   bearer_admission_request_m bearer_req;
   erabs_response_t *erabs_responses;
   xranc_ue *ueptr;
} bearerTrans_t;

class xranc
{
   private:
      xranc_state *xranc_state_ptr;
      xranc_comm_api *xranc_comm_api_ptr;
      uint32_t cell_config_interval_seconds;
      uint32_t rx_signal_meas_report_interval_ms; 
      uint32_t l2_meas_report_interval_ms; 
      //	event_processor_t *eventProcessor;
      std::map<uint32_t , handoff_request_m> ho_transactionMap;
      std::map<uint32_t , ue_admission_request_m> adm_control_transactionMap;
      std::map<uint32_t , bearerTrans_t > bearer_transactionMap;
  	  std::ofstream logFile;

   public:
      xranc (std::string configFile , std::string logFile);
      ~xranc ();
      void *eventTriggerThread (void *ptr);
      void sendCellConfig ();
      void send ();
      void receive ();
      void bearerAdmissionControl (uint16_t api_id, void* msg_p);
      void updateRadioMeasurements (uint16_t api_id, void* msg_p);
      void ueAdmissionControl (uint16_t api_id, void *msg_p);
      void hoControl (uint16_t api_id, void *msg_p);
      void bearerRelease (uint16_t api_id, void* msg_p);
      void updateSchedulerMeasurements ( uint16_t apiId , void *msg_p );
      void updatePdcpMeasurements (uint16_t apiId , void *msg_p );
      //friend ofstream & operator << (ofstream & fileName , xranc_state *xranc_state_ptr);
      void print_log(); 
};

#endif /* INC_XRANC_H_ */
