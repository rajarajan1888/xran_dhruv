/*
 * dummy_eNB_rrm.h
 *
 *  Created on: Oct 31, 2016
 *      Author: rsiva
 */
#include <iostream>
#include <cstdio>
#include <vector>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "xranc_interface.h"
#include <map>

#ifndef INC_DUMMY_ENB_RRM_H_
#define INC_DUMMY_ENB_RRM_H_

// AR: Keeping the max message size to be 1024 bytes for now. This should be big enough. 
#define MAX_MSG_SIZE 1024

class dummyeNB
{
private:
	uint32_t recv_Socket;
	uint32_t send_Socket;
	std::string eciString;
	cell_config_report_m cellInfo;
	void *rcvd_msg_buf;
	void *send_msg_buf;
	struct sockaddr_in *recv_sockAddr;
	struct sockaddr_in *send_sockAddr;

	std::string xranc_ip_address;
	uint32_t xranc_port;

	uint32_t rrm_port;
	uint32_t cell_config_interval_seconds;
	std::map<uint32_t , uint32_t> crnti_ueid_map;
	std::vector<uint32_t> alreadyremovedUEs;


public:
	dummyeNB (std::string configFile);
	void setCellInfo (std::string eciString);
	void send_xranc_cell_config_report ();
    std::pair <uint16_t, void*> receive_xranc_messages ();
	void send_ue_admission_request (uint16_t crnti , adm_est_cause_et ue_admission_cause);
	void send_xranc_ue_admission_status_report ( uint16_t crnti);

	sockaddr_in * getSendSockAddr ();
	uint32_t getSendSocket ();

	void send_handover_failure_decision ();
	void make_xranc_handoff_decision (handoff_request_m *ho_request);
	cell_config_report_m getCellInfo ();
	void checkforrelease (uint32_t userTrackerId);
	void checkforbearerrelease ();
	void checkforbearer ();
	void send_xranc_bearer_admission_request ( uint32_t crnti , ue_ambr_t ambrVal ,  uint16_t num_erabs , erabs_params_t erabs_params[] );
	void send_xranc_bearer_admission_release ( uint32_t crnti , uint16_t num_erabs , uint32_t erabs_id[] );
	void make_xranc_bearer_admission_status_decision (bearer_admission_response_m *bearer_response);
	void send_xranc_ue_release (uint16_t crnti);
	void send_xranc_bearer_admission_status (uint32_t crnti , uint16_t num_erabs , erabs_response_t *bearerResponseDecision);
	void send ();
	void receive ();
};



#endif /* INC_DUMMY_ENB_RRM_H_ */
