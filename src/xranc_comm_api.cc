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

#include "../inc/xranc.h"
//#include "../inc/xranc_cell.h"
//#include "../inc/xranc_ue.h"
//#include "../inc/xranc_interface.h"

#include <netdb.h>           // For gethostbyname()
#include <arpa/inet.h>       // For inet_addr()
#include <unistd.h>          // For close()

using namespace std;

//Refer to class xranc_cell in xranc_cell.h
xranc_comm_api :: xranc_comm_api (uint32_t xranc_port , uint16_t num_active_cells , std::string rrm_ip_address_strings[] , std::string rrm_port_strings[])
{

   // Create/Bind xranc receive socket
	udp_Socket = socket(PF_INET , SOCK_DGRAM , 0);
	recv_sockAddr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
	recv_sockAddr->sin_family = AF_INET;
	recv_sockAddr->sin_port = htons(xranc_port);
	recv_sockAddr->sin_addr.s_addr = INADDR_ANY;
	std::memset(recv_sockAddr->sin_zero , '\0', sizeof(recv_sockAddr->sin_zero));
	int rcvSockStatus = bind(udp_Socket , (struct sockaddr *)recv_sockAddr , sizeof(*recv_sockAddr));
	//cout << "Receive Socket status binding = " << rcvSockStatus << endl;

	rcvd_msg_buf = malloc(MAX_MSG_SIZE);

//    trans_id = 0;

   // Create Destination Socket Addresses for the cells based on the config file

	send_sockAddr = (struct sockaddr_in **)malloc(sizeof(struct sockaddr_in *));
	for ( uint16_t cellId = 0 ; cellId < num_active_cells ; cellId++ ) {
		struct sockaddr_in *sendSock = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
		sendSock->sin_family = AF_INET;
		sendSock->sin_port = htons((uint32_t)atoi(rrm_port_strings[cellId].c_str()));
		sendSock->sin_addr.s_addr = inet_addr(rrm_ip_address_strings[cellId].c_str());
		send_sockAddr[cellId] = sendSock;
		std::memset(send_sockAddr[cellId]->sin_zero , '\0' , sizeof(send_sockAddr[cellId]->sin_zero ));
	}
	send_msg_buf = NULL;
}

uint32_t xranc_comm_api :: getUdpSocket ()
{
	return udp_Socket;
}

std::pair<xranc_api_id_et , void * > xranc_comm_api :: xranc_rcv_message ()
{
	xranc_msg_api_hdr_t *msg_hdr_p;
	void *msg_body_p;

	socklen_t sockAddr_size;
	sockAddr_size = sizeof(*recv_sockAddr);

//	cout << "Waiting to read from socket" << endl;
	recvfrom (udp_Socket , rcvd_msg_buf , MAX_MSG_SIZE , 0 , (struct sockaddr *)recv_sockAddr , &sockAddr_size);
//	cout << "Received " << endl;
	msg_hdr_p = (xranc_msg_api_hdr_t *)rcvd_msg_buf;
//	cout << "Source ID = " << msg_hdr_p->src_id << " Destination ID = " << msg_hdr_p->dst_id << endl;

	if ( msg_hdr_p->src_id == ENB_CP_AGENT && msg_hdr_p->dst_id == XRANC ) {
	//	cout << " Validity of received message - passed (Stage 1) " << endl;
		msg_body_p = (char*)(rcvd_msg_buf) + sizeof(xranc_msg_api_hdr_t);
      return std::make_pair ( (xranc_api_id_et) msg_hdr_p->api_id, msg_body_p);
	} else {
		cout << "Invalid Message Received: src_id - " << msg_hdr_p->src_id << "dst_id - " << msg_hdr_p->dst_id << endl << endl;
		return std::make_pair( (xranc_api_id_et)0, (void*)NULL);
	}
}

bool xranc_comm_api :: xranc_bearer_admission_response (
      xranc_cell *cell,
      uint16_t cellCounter,
      uint16_t crnti,
      uint16_t num_erab_list,
      erabs_response_t *listOfErabsResponses )
{
   send_msg_buf = malloc(sizeof(xranc_msg_api_hdr_t) + sizeof(bearer_admission_response_m));

   xranc_msg_api_hdr_t *msg_hdr;
   bearer_admission_response_m *beareradmresponse_msg;

   msg_hdr = (xranc_msg_api_hdr_t*)send_msg_buf;
   beareradmresponse_msg = (bearer_admission_response_m *) ((char*)(send_msg_buf) + sizeof(xranc_msg_api_hdr_t));

   //Initialize message;
   msg_hdr->api_id = XRANC_BEARER_ADMISSION_RESPONSE;
   msg_hdr->src_id = XRANC;
   msg_hdr->dst_id = ENB_CP_AGENT;
   msg_hdr->msg_len = sizeof(xranc_msg_api_hdr_t) + sizeof(bearer_admission_response_m);
   //msg_hdr->trans_id = trans_id++;

   beareradmresponse_msg->ecgi = cell->getCellInfo()->eci;
   beareradmresponse_msg->crnti = crnti;
   cout << "XRANC_BEARER_ADMISSION_RESPONSE: CRNTI  " << crnti << endl;
   for ( uint16_t erabId = 0 ; erabId < num_erab_list ; erabId++ ) {
      cout << "ERAB ID: " << listOfErabsResponses[erabId].erab_id << "; RESP: " << listOfErabsResponses[erabId].erabs_decision << endl;
      beareradmresponse_msg->erabs_response[erabId] = listOfErabsResponses[erabId];
   }
   beareradmresponse_msg->num_erabs = num_erab_list;

   socklen_t cellSocketSize = sizeof(*send_sockAddr[cellCounter]);
   //cout << "Socket length: xRAN-C Cell Config Request = " << cellSocketSize << "Cell ID = " << cellCounter << endl;

   int cellcfgreq = sendto(udp_Socket, send_msg_buf, msg_hdr->msg_len, 0, (const struct sockaddr *) send_sockAddr[cellCounter], cellSocketSize);

   if ( cellcfgreq < 0 ) {
      cout << "WARNING: Failed to send XRANC_UE_ADMISSION_RESPONSE " << endl;
      return FALSE;
   }

   free(send_msg_buf);
   return TRUE;
}

bool xranc_comm_api :: xranc_ue_admission_response (
      xranc_cell *cell,
      uint16_t cellCounter,
      uint16_t crnti,
      adm_est_response_et adm_est_response)
{
	// Initiate buffer
		   send_msg_buf = malloc(sizeof(xranc_msg_api_hdr_t) + sizeof(ue_admission_response_m));

		   xranc_msg_api_hdr_t *msg_hdr;
		   ue_admission_response_m *ueadmissionresponse_msg;

		   msg_hdr = (xranc_msg_api_hdr_t*)send_msg_buf;
		   ueadmissionresponse_msg = (ue_admission_response_m *) ((char*)(send_msg_buf) + sizeof(xranc_msg_api_hdr_t));

		   //Initialize message;
		   msg_hdr->api_id = XRANC_UE_ADMISSION_RESPONSE;
		   msg_hdr->src_id = XRANC;
		   msg_hdr->dst_id = ENB_CP_AGENT;
		   msg_hdr->msg_len = sizeof(xranc_msg_api_hdr_t) + sizeof(ue_admission_response_m);
//	       msg_hdr->trans_id = trans_id++;

		   ueadmissionresponse_msg->ecgi = cell->getCellInfo()->eci;
		   ueadmissionresponse_msg->crnti = crnti;
           ueadmissionresponse_msg->adm_est_response = adm_est_response;

		   socklen_t cellSocketSize = sizeof(*send_sockAddr[cellCounter]);
		   //cout << "Socket length: xRAN-C Cell Config Request = " << cellSocketSize << "Cell ID = " << cellCounter << endl;

		   int cellcfgreq = sendto(udp_Socket, send_msg_buf, msg_hdr->msg_len, 0, (const struct sockaddr *) send_sockAddr[cellCounter], cellSocketSize);

		   if ( cellcfgreq < 0 ) {
			   cout << "WARNING: Failed to send XRANC_UE_ADMISSION_RESPONSE " << endl;
			   return FALSE;
		   }

		   free(send_msg_buf);
		   return TRUE;
}

bool xranc_comm_api :: send_ho_request (xranc_cell *cell , uint32_t cellCounter , uint32_t crnti , eutran_global_cell_id_t srcCell , eutran_global_cell_id_t targetCell )
{
		   send_msg_buf = malloc(sizeof(xranc_msg_api_hdr_t) + sizeof(handoff_request_m));

		   xranc_msg_api_hdr_t *msg_hdr;
		   handoff_request_m *ho_request;

		   msg_hdr = (xranc_msg_api_hdr_t*)send_msg_buf;
		   ho_request = (handoff_request_m*) ((char*)(send_msg_buf) + sizeof(xranc_msg_api_hdr_t));

		   //Initialize message;
		   msg_hdr->api_id = XRANC_HANDOFF_REQUEST;
		   msg_hdr->src_id = XRANC;
		   msg_hdr->dst_id = ENB_CP_AGENT;
		   msg_hdr->msg_len = sizeof(xranc_msg_api_hdr_t) + sizeof(handoff_request_m);
	 //      msg_hdr->trans_id = trans_id++;

		   ho_request->crnti = crnti;
		   ho_request->ecgi_s = srcCell;
		   ho_request->ecgi_t = targetCell;

		   socklen_t cellSocketSize = sizeof(*send_sockAddr[cellCounter]);
		   //cout << "Socket length: xRAN-C Cell Config Request = " << cellSocketSize << "Cell ID = " << cellCounter << endl;

		   int cellcfgreq = sendto(udp_Socket, send_msg_buf, msg_hdr->msg_len, 0, (const struct sockaddr *) send_sockAddr[cellCounter], cellSocketSize);

		   if ( cellcfgreq < 0 ) {
			   //cout << "HO request not successfully sent from xRAN-C " << endl;
            return FALSE;
		   }

		   //cout << "Sent HO request status " << cellcfgreq << endl;

		   free(send_msg_buf);
		   return TRUE;
}

bool xranc_comm_api :: xranc_cell_config_request (xranc_cell *cell , uint16_t cellCounter)
{
	   // Initiate buffer
	   send_msg_buf = malloc(sizeof(xranc_msg_api_hdr_t) + sizeof(cell_config_request_m));

	   xranc_msg_api_hdr_t *msg_hdr;
	   cell_config_request_m *cellconfigrequest_msg;

	   msg_hdr = (xranc_msg_api_hdr_t*)send_msg_buf;
	   cellconfigrequest_msg = (cell_config_request_m*) ((char*)(send_msg_buf) + sizeof(xranc_msg_api_hdr_t));

	   //Initialize message;
	   msg_hdr->api_id = XRANC_CELL_CONFIG_REQUEST;
	   msg_hdr->src_id = XRANC;
	   msg_hdr->dst_id = ENB_CP_AGENT;
	   msg_hdr->msg_len = sizeof(xranc_msg_api_hdr_t) + sizeof(cell_config_request_m);
 //      msg_hdr->trans_id = trans_id++;

	   cellconfigrequest_msg->ecgi = cell->getCellInfo()->eci;
	   socklen_t cellSocketSize = sizeof(*send_sockAddr[cellCounter]);
	   //cout << "Socket length: xRAN-C Cell Config Request = " << cellSocketSize << "Cell ID = " << cellCounter << endl;

	   int cellcfgreq = sendto(udp_Socket, send_msg_buf, msg_hdr->msg_len, 0, (const struct sockaddr *) send_sockAddr[cellCounter], cellSocketSize);

	   if ( cellcfgreq < 0 ) {
	   	   cout << "WARNING: Failed to send XRANC_CELL_CONFIG_REQUEST" << endl;
	   	   return FALSE;
	   }

	   cout << "XRANC_CELL_CONFIG_REQUEST sent for Cell-Id: " << cellCounter + 1 << endl << endl;

	   free(send_msg_buf);
	   return TRUE;
}

bool xranc_comm_api :: xranc_rx_signal_meas_config (xranc_cell *cell , uint16_t cellCounter, uint16_t crnti)
{
	   // Initiate buffer
	   send_msg_buf = malloc(sizeof(xranc_msg_api_hdr_t) + sizeof(rx_signal_meas_config_m));

	   xranc_msg_api_hdr_t *msg_hdr;
	   rx_signal_meas_config_m *meas_config_msg_p;

	   msg_hdr = (xranc_msg_api_hdr_t*)send_msg_buf;
	   meas_config_msg_p = (rx_signal_meas_config_m*) ((char*)(send_msg_buf) + sizeof(xranc_msg_api_hdr_t));

	   //Initialize message;
	   msg_hdr->api_id = XRANC_RX_SIGNAL_MEAS_CONFIG;
	   msg_hdr->src_id = XRANC;
	   msg_hdr->dst_id = ENB_CP_AGENT;
	   msg_hdr->msg_len = sizeof(xranc_msg_api_hdr_t) + sizeof(rx_signal_meas_config_m);
 //      msg_hdr->trans_id = trans_id++;

	   meas_config_msg_p->ecgi = cell->getCellInfo()->eci;
	   meas_config_msg_p->crnti = crnti;
      meas_config_msg_p->trigger_qty = meas_qty_BOTH;
      meas_config_msg_p->report_qty = meas_qty_BOTH;
      meas_config_msg_p->max_reported_cells = 4;

      meas_config_msg_p->report_interval_ms = cell->getCellInfo()->rx_signal_meas_report_interval_ms;

      socklen_t cellSocketSize = sizeof(*send_sockAddr[cellCounter]);

	   //cout << "Socket length: xRAN-C Cell Config Request = " << cellSocketSize << "Cell ID = " << cellCounter << endl;

	   int cellcfgreq = sendto(udp_Socket, send_msg_buf, msg_hdr->msg_len, 0, (const struct sockaddr *) send_sockAddr[cellCounter], cellSocketSize);

	   if ( cellcfgreq < 0 ) {
	   	   cout << "WARNING: Failed to send XRANC_RX_SIGNAL_MEAS_CONFIG" << endl;
	   	   return FALSE;
	   }

	   cout << "XRANC_RX_SIGNAL_MEAS_CONFIG sent for Cell-Id: " << cellCounter + 1 << " and UE: " << crnti << endl << endl;

	   free(send_msg_buf);
	   return TRUE;
}

bool xranc_comm_api :: xranc_l2_meas_config (xranc_cell *cell , uint16_t cellCounter)
{
	   // Initiate buffer
	   send_msg_buf = malloc(sizeof(xranc_msg_api_hdr_t) + sizeof(l2_meas_config_m));

	   xranc_msg_api_hdr_t *msg_hdr;
	   l2_meas_config_m *meas_config_msg_p;

	   msg_hdr = (xranc_msg_api_hdr_t*)send_msg_buf;
	   meas_config_msg_p = (l2_meas_config_m*) ((char*)(send_msg_buf) + sizeof(xranc_msg_api_hdr_t));

	   //Initialize message;
	   msg_hdr->api_id = XRANC_L2_MEAS_CONFIG;
	   msg_hdr->src_id = XRANC;
	   msg_hdr->dst_id = ENB_CP_AGENT;
	   msg_hdr->msg_len = sizeof(xranc_msg_api_hdr_t) + sizeof(l2_meas_config_m);
 //      msg_hdr->trans_id = trans_id++;

	   meas_config_msg_p->ecgi = cell->getCellInfo()->eci;
      meas_config_msg_p->report_interval_ms = cell->getCellInfo()->l2_meas_report_interval_ms;

      socklen_t cellSocketSize = sizeof(*send_sockAddr[cellCounter]);

	   //cout << "Socket length: xRAN-C Cell Config Request = " << cellSocketSize << "Cell ID = " << cellCounter << endl;

	   int cellcfgreq = sendto(udp_Socket, send_msg_buf, msg_hdr->msg_len, 0, (const struct sockaddr *) send_sockAddr[cellCounter], cellSocketSize);

	   if ( cellcfgreq < 0 ) {
	   	   cout << "WARNING: Failed to send XRANC_L2_MEAS_CONFIG" << endl;
	   	   return FALSE;
	   }

	   cout << "XRANC_L2_MEAS_CONFIG sent for Cell-Id: " << cellCounter + 1 << endl << endl;

	   free(send_msg_buf);
	   return TRUE;
}

// Refer to class xranc in xranc.h

