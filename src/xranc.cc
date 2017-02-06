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

xranc::xranc (std::string configFile , std::string logFileName)
{
	uint16_t num_active_cells;
	std::string *cellID_strings;
	std::string *rrm_ip_address_strings;
	std::string *rrm_port_strings;
	std::ifstream ifsInputFile (configFile.c_str() ,  std::ifstream::in);
	logFile.open(logFileName.c_str(), std::ofstream::out);
	std::string xranc_ip_address;
	uint32_t xranc_port;

	//eventProcessor = NULL;
	if ( ifsInputFile.is_open() ) {
		std::string strLine;
		while (std::getline(ifsInputFile , strLine)) {
			std::stringstream ssWordsBuf (strLine);
			std::string strWord;
			ssWordsBuf >> strWord;
			if (strWord.compare("NUM_ACTIVE_CELLS") == 0 ) {
				ssWordsBuf >> strWord;
				num_active_cells = (uint16_t)(atoi (strWord.c_str()));
            cout << "*******************************************************" << endl;
				cout << "Number of active cells = " << num_active_cells << endl;
				cellID_strings = new std::string[num_active_cells];
				rrm_ip_address_strings = new std::string[num_active_cells];
				rrm_port_strings = new std::string[num_active_cells];
				xranc_state_ptr = new xranc_state (num_active_cells);
			} else if (strWord.compare("CELL_IDENTITIES") == 0 ) {
				for ( uint16_t cellId = 0 ; cellId < num_active_cells ; cellId++ ) {
					ssWordsBuf >> strWord;
					cellID_strings[cellId] = strWord;
				}
			} else if (strWord.compare("RRM_IP_ADDRESSES") == 0) {
				for ( uint16_t cellId = 0 ; cellId < num_active_cells ; cellId++ ) {
					ssWordsBuf >> strWord;
					rrm_ip_address_strings[cellId] = strWord;
				}
			}  else if (strWord.compare("RRM_PORTS") == 0) {
				for ( uint16_t cellId = 0 ; cellId < num_active_cells ; cellId++ ) {
					ssWordsBuf >> strWord;
					rrm_port_strings[cellId] = strWord;
				}
			} else if (strWord.compare("RX_SIGNAL_MEAS_REPORT_INTERVAL_MS") == 0) {
					ssWordsBuf >> strWord;
					rx_signal_meas_report_interval_ms = (uint32_t) atoi(strWord.c_str());
			} else if (strWord.compare("L2_MEAS_REPORT_INTERVAL_MS") == 0) {
					ssWordsBuf >> strWord;
					l2_meas_report_interval_ms = (uint32_t) atoi(strWord.c_str());
			}/*else if (strWord.compare("XRANC_IP_ADDRESS") == 0) {
				ssWordsBuf >> strWord;
				xranc_ip_address = strWord;
			} */ else if ( strWord.compare("XRANC_PORT") == 0 ) {
				ssWordsBuf >> strWord;
				xranc_port = (uint32_t)(atoi(strWord.c_str()));
			} else if ( strWord.compare("XRANC_CELLCONFIGREQUEST_INTERVAL_SECONDS") == 0 ) {
				ssWordsBuf >> strWord;
				cell_config_interval_seconds = (uint32_t)(atoi(strWord.c_str()));
			}
		}
		for ( uint16_t cellId = 0 ; cellId < num_active_cells ; cellId++ ) {
         cout << "Cell-Id: " << cellId+1 << endl;
			xranc_state_ptr->update_cell_info(cellId ,
               cellID_strings[cellId],
               rx_signal_meas_report_interval_ms,
               l2_meas_report_interval_ms);
		}
		xranc_comm_api_ptr = new xranc_comm_api (xranc_port , num_active_cells , rrm_ip_address_strings , rrm_port_strings);
      cout << "*******************************************************" << endl;
		ifsInputFile.close();
	}
   else
   {
      cout << "Problem reading config file " << configFile << endl;
      assert(0);
   }
}

void xranc :: sendCellConfig ()
{
	while (1) {
		for ( uint32_t cellId = 0 ; cellId < xranc_state_ptr->getNumActiveCells () ; cellId++ ) {
			xranc_cell *cellptr = xranc_state_ptr->getCell(cellId);
			if(cellptr->getCellInfo()->cellConfig_rcvd == FALSE) {
			    if (!xranc_comm_api_ptr->xranc_cell_config_request(cellptr , cellId) ) {
			    	cout << "WARNING: Failed to send XRANC_CELL_CONFIG_REQUEST " << endl;
			    	exit(-1);
			    }
			}
		}
		sleep (cell_config_interval_seconds);
	}
}

void *cellConfigThread (void *xranc_ptr)
{
	xranc *xranc_func_ptr = (xranc *)xranc_ptr;
	xranc_func_ptr->sendCellConfig ();
	return (void *)NULL;
}

//void *xranc :: eventTriggerThread (void *ptr)
void xranc :: send ()
{
	// cout << "xRAN-C send() function " << endl;

	event_processor_t *eventProcessor;
	handoff_request_m *ho_request = (handoff_request_m *)malloc(sizeof(handoff_request_m));
	eventProcessor = new handover_processor_t (xranc_state_ptr , ho_request );
	while (1) {
		bool handover_status = eventProcessor->processEvent();
		if ( handover_status == 1 ) {

         // Add Ho Transactions
         hoControl(XRANC_HANDOFF_REQUEST, (void*)(ho_request));

		}
	}
}

void xranc :: ueAdmissionControl (uint16_t api_id , void *msg_p)
{

   uint32_t transactionId;
   uint16_t cellId;

   if (api_id == XRANC_UE_ADMISSION_REQUEST)
   {

      ue_admission_request_m *ueadmreq = (ue_admission_request_m *) msg_p;

      cout << "XRANC_UE_ADMISSION_REQUEST" << endl <<
         "ECI: " << hex << *((uint32*)(ueadmreq->ecgi.cell_identity)) <<
         "; CRNTI: " << ueadmreq->crnti <<
         "; ADM CAUSE: " << ueadmreq->adm_est_cause <<endl << endl;


      for ( cellId = 0 ; cellId < xranc_state_ptr->getNumActiveCells () ; cellId++ ) {
         if ( *((uint32*)(xranc_state_ptr->getCell(cellId)->getCellInfo()->eci.cell_identity)) ==
               *((uint32*)(ueadmreq->ecgi.cell_identity)) ) {

            // Admit user successfully always
            adm_est_response_et ue_admission_response;

            if ( xranc_state_ptr->getCell(cellId)->getCellInfo()->num_active_UEs <
                  xranc_state_ptr->getCell(cellId)->getMaxNumOfUEsPerCell() ) {

               ue_admission_response = ADM_RESPONSE_SUCCESS;

               // Add to transactions, only IF ADMITTED SUCCESSFULLY
               if (adm_control_transactionMap.size() == 0 ) {
                  transactionId = 1;
               } else {
                  transactionId = adm_control_transactionMap.rbegin()->first + 1;
               }

               adm_control_transactionMap.insert (std::pair<uint32_t , ue_admission_request_m>(transactionId , *ueadmreq));

            } else {
               ue_admission_response = ADM_RESPONSE_FAILURE;
            }

            bool response_status =
               xranc_comm_api_ptr->xranc_ue_admission_response(
                     xranc_state_ptr->getCell(cellId),
                     cellId,
                     ueadmreq->crnti,
                     ue_admission_response);

            if (response_status == TRUE)
            {
               cout << "XRANC_UE_ADMISSION_RESPONSE sent" << endl <<
                  "ADM RESPONSE: "<< ue_admission_response << endl << endl;
            }

            break;
         }
      }

      if( cellId == xranc_state_ptr->getNumActiveCells() )
      {
         cout << "XRANC_UE_ADMISSION_REQUEST received for cell-identity not configured at xRANc" << endl << endl;
      }
   }
   else if (api_id == XRANC_UE_ADMISSION_STATUS)
   {

      ue_admission_status_m *ueadmstatus = (ue_admission_status_m *)msg_p;

      cout << "XRANC_UE_ADMISSION_STATUS" << endl <<
         "ECI: " << hex << *((uint32*)(ueadmstatus->ecgi.cell_identity)) <<
         "; CRNTI: " << ueadmstatus->crnti <<
         "; ADM STATUS: " << ueadmstatus->adm_est_status <<endl << endl;


      std::map<uint32_t , ue_admission_request_m >::iterator it = adm_control_transactionMap.begin ();
      for (  ; it != adm_control_transactionMap.end() ; it++ ) {

         // Add a user if a valid transaction is found
         if ( ( *((uint32*)(it->second.ecgi.cell_identity)) ==
                  *((uint32*)(ueadmstatus->ecgi.cell_identity)) ) &&
               ( it->second.crnti == ueadmstatus->crnti )) {


            if ( ueadmstatus->adm_est_status == ADM_STATUS_SUCCESS ) {

               xranc_cell *xranc_cell_ptr = NULL;

               // Look up cellId
               for ( cellId = 0 ; cellId < xranc_state_ptr->getNumActiveCells () ; cellId++ ) {
                  if ( *((uint32*)(xranc_state_ptr->getCell(cellId)->getCellInfo()->eci.cell_identity)) ==
                        *((uint32*)(ueadmstatus->ecgi.cell_identity)) ) {
                     break;
                  }
               }
               xranc_cell_ptr = xranc_state_ptr->getCell(cellId);


               xranc_cell_ptr->addUE (xranc_state_ptr->getTotalNumUsers() , ueadmstatus->crnti );
               xranc_state_ptr->updateUsers (TRUE);

               xranc_comm_api_ptr->xranc_rx_signal_meas_config(
                        xranc_state_ptr->getCell(cellId),
                        cellId,
                        ueadmstatus->crnti);


            }

            //Remove transaction
            adm_control_transactionMap.erase(it);
            break;

         }
      }

      if( it == adm_control_transactionMap.end() )
      {
         cout << "WARNING: XRNAC_ADMISSION_CONTROL_STATUS received invalidly." << endl << endl;
      }


   }
   else
   {
      cout << "admission control transaction state machine not programmed to handle api_id: " << api_id << endl << endl;
   }

   if( adm_control_transactionMap.size() > 0 ) {
      cout << "**********************************************" << endl;
      cout << "List of ongoing admission control transactions" << endl;
      for (std::map<uint32_t , ue_admission_request_m>::iterator it = adm_control_transactionMap.begin (); it != adm_control_transactionMap.end() ; it++ ) {
         cout << "Transaction ID = " << it->first <<
            " CRNTI = " << it->second.crnti <<
            " ECI = " << hex << *((uint32*)(it->second.ecgi.cell_identity)) << endl;
      }
      cout << "**********************************************" << endl;
      cout << endl;

   }

}

void xranc :: hoControl(uint16_t api_id, void* msg_p)
{

   uint32_t transactionId;
   uint32_t cellId;

   if(api_id == XRANC_HANDOFF_REQUEST)
   {

      handoff_request_m *ho_request = (handoff_request_m*)msg_p;

      // Send HO request to both Source and Target Cell
      for ( uint32_t cellId = 0 ; cellId < xranc_state_ptr->getNumActiveCells() ; cellId++ ) {

         if (*((uint32_t *)(xranc_state_ptr->getCell(cellId)->getCellInfo()->eci.cell_identity)) ==
               *((uint32_t *)(ho_request->ecgi_s.cell_identity))) {


            if (!xranc_comm_api_ptr->send_ho_request (xranc_state_ptr->getCell(cellId) , cellId , ho_request->crnti , ho_request->ecgi_s , ho_request->ecgi_t) ) {
               cout << "Unable to send HO request to the dummy eNBs " << endl;
               exit(-1);
            }
         }

         if (*((uint32_t *)(xranc_state_ptr->getCell(cellId)->getCellInfo()->eci.cell_identity)) ==
               *((uint32_t *)(ho_request->ecgi_t.cell_identity))) {

            if (!xranc_comm_api_ptr->send_ho_request (xranc_state_ptr->getCell(cellId) , cellId , ho_request->crnti , ho_request->ecgi_s , ho_request->ecgi_t) ) {
               cout << "Unable to send HO request to the dummy eNBs " << endl;
               exit(-1);
            }
         }
      }

      cout << "XRANC_HANDOFF_REQUEST sent" << endl <<
         "src ECI: " << hex << *((uint32*)(ho_request->ecgi_s.cell_identity)) <<
         "; CRNTI: " << ho_request->crnti <<
         "; tgt ECI: " << hex << *((uint32*)(ho_request->ecgi_t.cell_identity)) << endl << endl;

      // Add to transactions
      if (ho_transactionMap.size() == 0 ) {
         transactionId = 1;
      } else {
         transactionId = ho_transactionMap.rbegin()->first + 1;
      }
      ho_transactionMap.insert (std::pair<uint32_t , handoff_request_m>(transactionId , *ho_request));



   }
   else if(api_id == XRANC_HANDOFF_COMPLETE)
   {

      handoff_complete_m *ho_complete = (handoff_complete_m *)msg_p;

      cout << "XRANC_HANDOFF_COMPLETE" << endl <<
         "src ECI: " << hex << *((uint32*)(ho_complete->ecgi_s.cell_identity)) <<
         "; tgt ECI: " << hex << *((uint32*)(ho_complete->ecgi_t.cell_identity)) <<
         "; CRNTI: " << ho_complete->crnti_new << endl << endl;


      std::map<uint32_t , handoff_request_m >::iterator it = ho_transactionMap.begin ();
      for (  ; it != ho_transactionMap.end() ; it++ ) {

         // Handoff UE if a valid transaction is found
         if ( ( *((uint32*)(it->second.ecgi_s.cell_identity)) ==
                  *((uint32*)(ho_complete->ecgi_s.cell_identity)) ) &&
               *((uint32*)(it->second.ecgi_t.cell_identity)) ==
               *((uint32*)(ho_complete->ecgi_t.cell_identity)) )
         {


            // Remove user from source cell
            for ( cellId = 0 ; cellId < xranc_state_ptr->getNumActiveCells() ; cellId++ ) {
               if ( *((uint32_t *)(xranc_state_ptr->getCell(cellId)->getCellInfo()->eci.cell_identity)) ==
                     *((uint32_t *)(ho_complete->ecgi_s.cell_identity)) ) {

                  xranc_cell *xranc_cell_ptr = xranc_state_ptr->getCell(cellId);

                  // Find UE ptr in source cell, and remove from it
                  xranc_ue *xranc_ue_ptr = xranc_cell_ptr->removeUE ( it->second.crnti );

                  if( xranc_ue_ptr != NULL ){

                     // Find tgt Cell-Id
                     for ( cellId = 0 ; cellId < xranc_state_ptr->getNumActiveCells () ; cellId++ ) {
                        if ( *((uint32*)(xranc_state_ptr->getCell(cellId)->getCellInfo()->eci.cell_identity)) ==
                              *((uint32*)(ho_complete->ecgi_t.cell_identity)) ) {
                           break;
                        }
                     }
                     xranc_cell_ptr = xranc_state_ptr->getCell(cellId);

                     // Modify the parameters for the new UE
                     xranc_ue_ptr->getUEInfo()->crnti = ho_complete->crnti_new;
                     xranc_ue_ptr->getUEInfo()->prb_usage = 0;
                     xranc_ue_ptr->getUEInfo()->qoe = 0;

                     // Add to Tgt cell
                     xranc_cell_ptr->addUE (xranc_ue_ptr);

                  }

               }
            }

            //Remove transaction
            ho_transactionMap.erase(it);
            break;

         }
      }
   }
   else if(api_id == XRANC_HANDOFF_FAILURE)
   {

      handoff_failure_m *ho_failure = (handoff_failure_m *)msg_p;

      cout << "XRANC_HANDOFF_FAILURE" << endl <<
         "src ECI: " << hex << *((uint32*)(ho_failure->ecgi_s.cell_identity)) <<
         "; CRNTI: " << ho_failure->crnti <<
         "; Cause: " << ho_failure->ho_failure_cause << endl << endl;

      std::map<uint32_t , handoff_request_m >::iterator it = ho_transactionMap.begin ();
      for (  ; it != ho_transactionMap.end() ; it++ ) {

         // Handoff UE if a valid transaction is found
         if ( ( *((uint32*)(it->second.ecgi_s.cell_identity)) ==
                  *((uint32*)(ho_failure->ecgi_s.cell_identity)) ) &&
               (it->second.crnti == ho_failure->crnti) )
         {

            //Remove transaction
            ho_transactionMap.erase(it);
            break;

         }
      }
      if( it == ho_transactionMap.end() )
      {
         cout << "WARNING: XRNAC_HANDOFF_FAILURE received invalidly." << endl << endl;
      }

   }
   else
   {
      cout << "Handoff control transaction state machine not programmed to handle api_id: " << api_id << endl << endl;
   }

   if ( ho_transactionMap.size() > 0 ){

      cout << "************************************" << endl;
      cout << "List of ongoing handoff transactions" << endl;
      for (std::map<uint32_t , handoff_request_m >::iterator it = ho_transactionMap.begin (); it != ho_transactionMap.end() ; it++ ) {
         cout << "Transaction ID = " << it->first <<
            " CRNTI = " << it->second.crnti <<
            " src ECI = " << hex << *((uint32*)(it->second.ecgi_s.cell_identity)) <<
            " tgt ECI = " << hex << *((uint32*)(it->second.ecgi_t.cell_identity)) << endl;
      }
      cout << "************************************" << endl;
      cout << endl;

   }


}

void xranc :: bearerRelease (uint16_t api_id, void* msg_p)
{

   uint32_t cellId;

	if ( api_id == XRANC_BEARER_RELEASE_IND ) {
		bearer_release_ind_m *bearerrel = (bearer_release_ind_m *)msg_p;

      cout << "XRANC_BEARER_RELEASE_IND" << endl <<
         "ECI: " << hex << *((uint32*)(bearerrel->ecgi.cell_identity)) <<
         "; CRNTI: " << bearerrel->crnti << endl;


		for ( cellId = 0 ; cellId < xranc_state_ptr->getNumActiveCells() ; cellId++ ) {
			if ( *((uint32*)(xranc_state_ptr->getCell(cellId)->getCellInfo()->eci.cell_identity)) ==
			                 *((uint32*)(bearerrel->ecgi.cell_identity)) ) {
				xranc_ue *ueptr = xranc_state_ptr->getCell(cellId)->getUE(bearerrel->crnti);

            if( ueptr == NULL ){
               cout << "WARNING: crnti not found for the Cell Id " << cellId << endl << endl;
               return;
            }

            uint16_t bearerRelId;
            for ( bearerRelId = 0 ; bearerRelId < bearerrel->num_erabs ; bearerRelId++ ) {

               uint16_t userBearerId;
               bool found_erab = FALSE;
               for ( userBearerId = 0 ; userBearerId < ueptr->getNumBearers() ; userBearerId++ ) {
                  if ( bearerrel->erab_ids[bearerRelId] ==
                        ueptr->getUEInfo()->bearer_info[userBearerId]->erab_params.erab_id ) {

                     // Free Bearer info memory for the released bearer
                     free(ueptr->getUEInfo()->bearer_info[userBearerId]);
                     for ( uint16_t bearerId = userBearerId + 1 ; bearerId < ueptr->getNumBearers() ; bearerId++ )
                     {
                        ueptr->getUEInfo()->bearer_info[bearerId-1] = ueptr->getUEInfo()->bearer_info[bearerId];
                     }

                     cout << "ERAB Id: " << bearerrel->erab_ids[bearerRelId] << " released." << endl;
                     ueptr->removeBearer();
                     if ( ueptr->getUEInfo()->bearer_info[userBearerId]->erab_params.erab_type == ERAB_DEDICATED )
                        ueptr->removeDedBearer();

                     found_erab = TRUE;

                     break;
                  }
               }

               if ( !found_erab)
               {
                  cout << "ERAB Id: " << bearerrel->erab_ids[bearerRelId] << " not found." << endl;
               }

               if ( bearerRelId == bearerrel->num_erabs ) {
                  break;
               }

            }

            break;
			}
		}

      if (cellId == xranc_state_ptr->getNumActiveCells())
      {
         cout << "WARNING: Cell not found " << endl;
      }

	}

   cout << endl;
}

void xranc :: bearerAdmissionControl (uint16_t api_id, void* msg_p)
{
   uint32_t transactionId;
   uint32_t cellId;
   uint16_t bearerId;

   if(api_id == XRANC_BEARER_ADMISSION_REQUEST ) {
      bearer_admission_request_m *beareradmreq = (bearer_admission_request_m *) msg_p;

      cout << "XRANC_BEARER_ADMISSION_REQUEST" << endl <<
         "ECI: " << hex << *((uint32*)(beareradmreq->ecgi.cell_identity)) <<
         "; CRNTI: " << beareradmreq->crnti << endl << endl;

      for ( cellId = 0 ; cellId < xranc_state_ptr->getNumActiveCells () ; cellId++ ) {
         if ( *((uint32*)(xranc_state_ptr->getCell(cellId)->getCellInfo()->eci.cell_identity)) ==
               *((uint32*)(beareradmreq->ecgi.cell_identity)) ) {

            xranc_ue *ueptr = xranc_state_ptr->getCell(cellId)->getUE(beareradmreq->crnti);

            if(ueptr == NULL)
            {
               cout << "WARNING: crnti does not exist for this cell. " << endl << endl;
               return;
            }

            // Update UE_AMBR
            ueptr->getUEInfo()->ue_ambr = beareradmreq->ue_ambr;

            erabs_response_t erabs_responses[MAX_NUM_ERABS];
            erabs_decision_et erab_decision;

            // Limiting number of bearers attached to a UE to MAX_NUM_ERABS
            // No partial-success supported
            if( ueptr->getUEInfo()->no_of_bearers + beareradmreq->num_erabs < MAX_NUM_ERABS)
            {
               erab_decision = ERABS_DECISION_SUCCESS;
            }
            else
            {
               erab_decision = ERABS_DECISION_FAIL;
            }

            for (bearerId = 0; bearerId < beareradmreq->num_erabs; bearerId++){
               erabs_responses[bearerId].erab_id = beareradmreq->erabs_params[bearerId].erab_id;
               erabs_responses[bearerId].erabs_decision = erab_decision;
            }

            // Add to transactions, only IF ADMITTED SUCCESSFULLY
            std::map<uint32_t , bearerTrans_t>::iterator it = bearer_transactionMap.begin() ;
            for ( ; it != bearer_transactionMap.end() ; it++ ) {
               if ( it->second.bearer_req.crnti == beareradmreq->crnti &&
                     ( *( (uint32*)(it->second.bearer_req.ecgi.cell_identity)) ==
                       *((uint32*)(beareradmreq->ecgi.cell_identity))) ) {
                  break;
               }
            }

            // If the crnti, ecgi pair does not exist in ongoing transactions
            if (it == bearer_transactionMap.end() ) {
               transactionId = bearer_transactionMap.size() + 1;
               bearerTrans_t bearerTrans;
               bearerTrans.bearer_req = *beareradmreq;

               bearerTrans.erabs_responses = new erabs_response_t[MAX_NUM_ERABS];
               bearerTrans.ueptr = ueptr;
               for ( bearerId = 0 ; bearerId <  beareradmreq->num_erabs; bearerId++ ) {

                  bearerTrans.erabs_responses[bearerId] = erabs_responses[bearerId];

               }

               //bearerTrans.erabs_responses = erabs_responses;
               bearer_transactionMap.insert (std::pair<uint32_t , bearerTrans_t>(transactionId , bearerTrans));

               // Send XRANC_BEARER_ADMISSION_RESPONSE
               bool response_status = xranc_comm_api_ptr->xranc_bearer_admission_response(
                     xranc_state_ptr->getCell(cellId),
                     cellId,
                     beareradmreq->crnti,
                     beareradmreq->num_erabs,
                     erabs_responses);

               if (response_status == TRUE) {
                  cout << "XRANC_BEARER_ADMISSION_RESPONSE sent" << endl;
               }

            } else {
               cout << "WARNING: Concurrent Transactions not supported. " <<
                  "Already undergoing transaction for this ECGI, crnti." << endl << endl;
            }

            break;

         }
      }

      if( cellId == xranc_state_ptr->getNumActiveCells() )
      {
         cout << "XRANC_BEARER_ADMISSION_REQUEST received for cell-identity not configured at xRANc" << endl << endl;
      }

   } else if (api_id == XRANC_BEARER_ADMISSION_STATUS) {

      bearer_admission_status_m *beareradmstatus = (bearer_admission_status_m *)msg_p;

      cout << "XRANC_BEARER_ADMISSION_STATUS" << endl <<
         "ECI: " << hex << *((uint32*)(beareradmstatus->ecgi.cell_identity)) <<
         "; CRNTI: " << beareradmstatus->crnti << endl;

      std::map<uint32_t , bearerTrans_t >::iterator it = bearer_transactionMap.begin ();
      for (  ; it != bearer_transactionMap.end() ; it++ ) {

         // Add bearers if a valid transaction is found
         if ( ( *((uint32*)(it->second.bearer_req.ecgi.cell_identity)) ==
                  *((uint32*)(beareradmstatus->ecgi.cell_identity)) ) &&
               ( it->second.bearer_req.crnti == beareradmstatus->crnti )) {

            xranc_ue *ueptr = it->second.ueptr;

            // AR: Is this needed?
            if ( ueptr->getNumBearers() == 0 ) {
               ueptr->initializeBearerInformation();
            }

            for ( uint16_t erabId = 0 , bearerId = 0 ; erabId < beareradmstatus->num_erabs && bearerId < it->second.bearer_req.num_erabs; erabId++ , bearerId++ ) {

               if (beareradmstatus->erabs_response[erabId].erab_id ==
                     it->second.bearer_req.erabs_params[bearerId].erab_id){

                  if ( (beareradmstatus->erabs_response[erabId].erabs_decision == ERABS_DECISION_SUCCESS) &&
                        (it->second.erabs_responses[bearerId].erabs_decision == ERABS_DECISION_SUCCESS) ) {

                     cout << "Admitted ERAB Id: " << beareradmstatus->erabs_response[erabId].erab_id << endl;
                     ueptr->addBearerInformation (it->second.bearer_req.erabs_params[bearerId]);
                  }
               }
               else
               {
                  cout << "WARNING: ERAB ID mismatch between request(" <<
                     it->second.bearer_req.erabs_params[bearerId].erab_id << ") and status(" <<
                     beareradmstatus->erabs_response[erabId].erab_id << ")" << endl;
               }
            }

            // Delete dynamic memory allocated
            delete it->second.erabs_responses;

            //Remove transaction
            bearer_transactionMap.erase(it);

            break;

         }
      }

      if( it == bearer_transactionMap.end() )
      {
         cout << "WARNING: XRANC_BEARER_ADMISSION_STATUS received invalidly." << endl << endl;
      }
   }

   if ( bearer_transactionMap.size() > 0 ){

      cout << "**********************************************" << endl;
      cout << "List of ongoing bearer management transactions" << endl;
      for (std::map<uint32_t , bearerTrans_t >::iterator it = bearer_transactionMap.begin (); it != bearer_transactionMap.end() ; it++ ) {
         cout << "Transaction ID = " << it->first <<
            " CRNTI = " << it->second.ueptr->getUEInfo()->crnti << " #ERABS = " << it->second.bearer_req.num_erabs << endl;
      }
      cout << "**********************************************" << endl;

   }

   cout << endl;
}


ofstream & operator << (ofstream & fileName , xranc_state *xranc_state_ptr )
{

	time_t rawtime;
	struct tm * timeinfo;

	time (&rawtime);
	timeinfo = localtime (&rawtime);

	fileName << "Time Stamp: " << asctime(timeinfo) << endl;
	fileName << "Number of active cells = " << xranc_state_ptr->getNumActiveCells() << " , Number of UEs = " << xranc_state_ptr->getTotalNumUsers() << endl;

	fileName << "-------------------------------------------------------------------------------" << endl;

	for ( uint16_t cellId = 0 ; cellId < xranc_state_ptr->getNumActiveCells() ; cellId++ ) {
		fileName << "Cell ID = " << hex << *((uint32_t *)(xranc_state_ptr->getCell(cellId)->getCellInfo()->eci.cell_identity)) << endl;
		fileName << "Number of UEs attached to the cell = " << xranc_state_ptr->getCell(cellId)->getNumActiveUEs() << endl;

		fileName << "-------------------------------------------------------------------------------" << endl;

		for ( uint16_t userId = 0 ; userId < xranc_state_ptr->getCell(cellId)->getNumActiveUEs () ; userId++ ) {
			xranc_ue **attached_ue_ptr = xranc_state_ptr->getCell(cellId)->getAttachedUEList();
			xranc_ue *ue_ptr = attached_ue_ptr[userId];
			fileName << "User ID CRNTI = " << ue_ptr->getUEInfo()->crnti << endl;
			fileName << "Global UE ID = " << ue_ptr->getUEInfo()->global_ue_id << endl ;
			//fileName << "Number of bearers = " << ue_ptr->getNumBearers() << " and Number of dedicated bearers = " << ue_ptr->getNumDedBearers() << endl;
			fileName << "Serving cell ID = " << *((uint32_t *)(ue_ptr->getUEInfo()->meas_info_sc.eci.cell_identity)) << "RSRP = " << ue_ptr->getUEInfo()->meas_info_sc.rsrp << "RSRQ = " << ue_ptr->getUEInfo()->meas_info_sc.rsrq << endl;

			fileName << "------------------------------RADIO MEASUREMENTS-------------------------------" << endl;

			for ( uint16_t neighborCellId = 0 ; (ue_ptr->getUEInfo()->meas_info_nc + neighborCellId) != NULL ; neighborCellId++ ) {
				xranc_meas_rep_info_t *neighborMeas = ue_ptr->getUEInfo()->meas_info_nc + neighborCellId;
				fileName << "Neighboring cell ID = " << *((uint32_t *)(neighborMeas->eci.cell_identity)) << "RSRP = " << neighborMeas->rsrp << "RSRQ = " << neighborMeas->rsrq << endl;
			}

			for ( uint16_t cqiId = 0 ; cqiId < MAX_NUM_CQIs ; cqiId++) {
				fileName << "CQI (History) " << cqiId + 1 << " is " << (uint32_t)ue_ptr->getUEInfo()->radio_meas_reports.cqi_hist[cqiId] << endl;
			}

			for ( uint16_t riId = 0 ; riId < MAX_NUM_RIs ; riId++ ) {
				fileName << "RI (History) " << riId + 1 << " is " << (uint32_t)ue_ptr->getUEInfo()->radio_meas_reports.ri_hist[riId] << endl;
			}

			for ( uint16_t sinrId = 0 ; sinrId < SINR_HIST_SIZE ; sinrId++ ) {
				fileName << "SINR ID " << sinrId+1 << " of PUSCH" << (uint32_t)ue_ptr->getUEInfo()->radio_meas_reports.pusch_sinr_hist[sinrId] << " and PUCCH " << (uint32_t)ue_ptr->getUEInfo()->radio_meas_reports.pucch_sinr_hist[sinrId] << endl;
			}

			fileName << "----------------------------SCHEDULER MEASUREMENTS----------------------------" << endl;

			for ( uint16_t qciId = 0 ; qciId < MAX_NUM_SUPPORTED_QCI ; qciId++ ) {
					fileName << "Downlink PRB Usage = " << (uint32_t)(ue_ptr->getUEInfo()->sched_meas_reports.prb_usage_dl[qciId]) << endl;
					fileName << "Uplink PRB Usage = " << (uint32_t)(ue_ptr->getUEInfo()->sched_meas_reports.prb_usage_ul[qciId]) << endl;
					fileName << "Downlink MCS Usage = " << (uint32_t)(ue_ptr->getUEInfo()->sched_meas_reports.mcs_dl[qciId]) << endl;
					fileName << "Uplink MCS Usage = " << (uint32_t)(ue_ptr->getUEInfo()->sched_meas_reports.mcs_ul[qciId]) << endl;
					fileName << "Downlink TTIs = " << (uint32_t)(ue_ptr->getUEInfo()->sched_meas_reports.num_sched_ttis_dl[qciId]) << endl;
					fileName << "Uplink TTIs = " << (uint32_t)(ue_ptr->getUEInfo()->sched_meas_reports.num_sched_ttis_ul[qciId]) << endl;

					fileName << "Rank Indicator : " << "\t";
					for ( uint16_t rankIndicatorMatrix = 0; rankIndicatorMatrix < 2 ; rankIndicatorMatrix++ )
					fileName << "" << ue_ptr->getUEInfo()->sched_meas_reports.rank_dl[qciId][rankIndicatorMatrix] << "\t";
			}

			fileName << "--------------------------PDCP MEASUREMENTS----------------------------------" << endl;

			for ( uint16_t qciId = 0 ; qciId < MAX_NUM_SUPPORTED_QCI ; qciId++ ) {
					fileName << "Data volume downlink = " << (uint32_t)(ue_ptr->getUEInfo()->pdcp_meas_reports.data_vol_dl[qciId]) << endl;
					fileName << "Data volume uplink = " << (uint32_t)(ue_ptr->getUEInfo()->pdcp_meas_reports.data_vol_ul[qciId]) << endl;
					fileName << "Packet delay downlink = " << (uint32_t)(ue_ptr->getUEInfo()->pdcp_meas_reports.pkt_delay_dl[qciId]) << endl;
					fileName << "Packet discard rate downlink = " << (uint32_t)(ue_ptr->getUEInfo()->pdcp_meas_reports.pkt_discard_rate_dl[qciId]) << endl;
					fileName << "Packet loss rate downlink = " << (uint32_t)(ue_ptr->getUEInfo()->pdcp_meas_reports.pkt_loss_rate_dl[qciId]) << endl;
					fileName << "Packet loss rate uplink = " << (uint32_t)(ue_ptr->getUEInfo()->pdcp_meas_reports.pkt_loss_rate_ul[qciId]) << endl;
					fileName << "Downlink throughput = " << (uint32_t)(ue_ptr->getUEInfo()->pdcp_meas_reports.throughput_dl[qciId]) << endl;
					fileName << "Uplink throughput = " << (uint32_t)(ue_ptr->getUEInfo()->pdcp_meas_reports.throughput_ul[qciId]) << endl;
			}

		}
	}
	return fileName;
}

xranc :: ~xranc ()
{
	logFile.close();
}

void xranc :: updateRadioMeasurements (uint16_t api_id , void *msg_p)
{
	if (api_id == XRANC_RX_SIGNAL_MEAS_REPORT ) {

		rx_signal_meas_report_m *rxSignalReport = (rx_signal_meas_report_m *)msg_p;
		for ( uint32_t cellId = 0 ; cellId < xranc_state_ptr->getNumActiveCells () ; cellId++ ) {
			if ( *((uint32*)(xranc_state_ptr->getCell(cellId)->getCellInfo()->eci.cell_identity)) ==
			               *((uint32*)(rxSignalReport->scell_meas_report.ecgi.cell_identity )) ) {

				uint16_t crnti = rxSignalReport->crnti;
				xranc_ue *ueptr = xranc_state_ptr->getCell(cellId)->getUE(crnti);

				ueptr->setServingCellSignalValues (rxSignalReport->scell_meas_report);

				ueptr->setNeighboringCells (rxSignalReport->num_ncells , rxSignalReport->ncell_meas_report );
				break;
			}
		}
	} else if ( api_id == XRANC_RADIO_MEAS_REPORT_PER_CELL ) {

		radio_meas_report_per_cell_m *radioMeasReportMsgPerCell = (radio_meas_report_per_cell_m *)msg_p;
		for ( uint32_t cellId = 0 ; cellId < xranc_state_ptr->getNumActiveCells () ; cellId++ ) {
			if ( *((uint32*)(xranc_state_ptr->getCell(cellId)->getCellInfo()->eci.cell_identity)) ==
					               *((uint32*)(radioMeasReportMsgPerCell->ecgi.cell_identity )) ) {

				xranc_cell *cellPtr = xranc_state_ptr->getCell(cellId);
				cellPtr->setRadioMeasurementReport (radioMeasReportMsgPerCell->pucch_intf_power_hist , radioMeasReportMsgPerCell->pusch_intf_power_hist);

				break;
			}
		}
	} else if ( api_id == XRANC_RADIO_MEAS_REPORT_PER_UE ) {

		radio_meas_report_per_ue_m *radioMeasReportMsg = (radio_meas_report_per_ue_m *)msg_p;
		for ( uint32_t cellId = 0 ; cellId < xranc_state_ptr->getNumActiveCells () ; cellId++ ) {
			if ( *((uint32*)(xranc_state_ptr->getCell(cellId)->getCellInfo()->eci.cell_identity)) ==
					               *((uint32*)(radioMeasReportMsg->ecgi.cell_identity )) ) {

				uint16_t crnti = radioMeasReportMsg->crnti;
				xranc_ue *ueptr = xranc_state_ptr->getCell(cellId)->getUE(crnti);

				ueptr->setRadioMeasurementReport ( radioMeasReportMsg->cqi_hist , radioMeasReportMsg->ri_hist , radioMeasReportMsg->pucch_sinr_hist , radioMeasReportMsg->pusch_sinr_hist );
				break;
			}
		}
	}
	logFile << xranc_state_ptr;
}

void xranc :: updateSchedulerMeasurements ( uint16_t apiId , void *msg_p )
{
	if ( apiId == XRANC_SCHED_MEAS_REPORT_PER_UE ) {

		sched_meas_report_per_ue_m *schedMeasReport = (sched_meas_report_per_ue_m *)msg_p;
		for ( uint32_t cellId = 0 ; cellId < xranc_state_ptr->getNumActiveCells() ; cellId++ ) {
			if ( *((uint32*)(xranc_state_ptr->getCell(cellId)->getCellInfo()->eci.cell_identity)) ==
								               *((uint32*)(schedMeasReport->ecgi.cell_identity )) ) {

				uint16_t crnti = schedMeasReport->crnti;
				xranc_ue *ueptr = xranc_state_ptr->getCell(cellId)->getUE(crnti);

				ueptr->setSchedulingMeasurementReport (schedMeasReport->mcs_dl , schedMeasReport->mcs_ul , schedMeasReport->num_sched_ttis_dl , schedMeasReport->num_sched_ttis_ul , schedMeasReport->prb_usage_dl , schedMeasReport->prb_usage_ul , schedMeasReport->rank_dl);

				break;
			}
		}
	} else if ( apiId == XRANC_SCHED_MEAS_REPORT_PER_CELL ) {
		sched_meas_report_per_cell_m *schedMeasReport = (sched_meas_report_per_cell_m *)msg_p;
		for ( uint32_t cellId = 0 ; cellId < xranc_state_ptr->getNumActiveCells() ; cellId++ ) {
			if ( *((uint32*)(xranc_state_ptr->getCell(cellId)->getCellInfo()->eci.cell_identity)) ==
								               *((uint32*)(schedMeasReport->ecgi.cell_identity )) ) {

				xranc_cell *cellPtr = xranc_state_ptr->getCell(cellId);
				cellPtr->setSchedulingMeasurementReport (schedMeasReport->prb_usage_dl , schedMeasReport->prb_usage_ul );

				break;
			}
		}
	}
	logFile << xranc_state_ptr;
}

void xranc :: updatePdcpMeasurements (uint16_t apiId , void *msg_p )
{
	if ( apiId == XRANC_PDCP_MEAS_REPORT_PER_UE ) {
		pdcp_meas_report_per_ue_m *pdcpMeasReport = (pdcp_meas_report_per_ue_m *)msg_p;
		for ( uint32_t cellId = 0 ; cellId < xranc_state_ptr->getNumActiveCells() ; cellId++ ) {
			if ( *((uint32*)(xranc_state_ptr->getCell(cellId)->getCellInfo()->eci.cell_identity)) ==
								               *((uint32*)(pdcpMeasReport->ecgi.cell_identity)) ) {
				uint16_t crnti = pdcpMeasReport->crnti;
				xranc_ue *ueptr = xranc_state_ptr->getCell(cellId)->getUE(crnti);

				ueptr->setPdcpMeasurementReport (pdcpMeasReport->data_vol_dl , pdcpMeasReport->data_vol_ul , pdcpMeasReport->pkt_delay_dl , pdcpMeasReport->pkt_discard_rate_dl , pdcpMeasReport->pkt_loss_rate_dl , pdcpMeasReport->pkt_loss_rate_ul , pdcpMeasReport->throughput_dl , pdcpMeasReport->throughput_ul );

				//ueptr->setSchedulingMeasurementReport (pdcpMeasReport->mcs_dl , schedMeasReport->mcs_ul , schedMeasReport->num_sched_ttis_dl , schedMeasReport->num_sched_ttis_ul , schedMeasReport->prb_usage_dl , schedMeasReport->prb_usage_ul , schedMeasReport->rank_dl);
				logFile << xranc_state_ptr;
				break;
			}
		}
	}
}

void xranc :: receive ()
{
   /*pthread_t tid;
     pthread_create (&tid , NULL , &eventTriggerThread , (void *)NULL);*/
   event_processor_t *eventProcessor;
   while ( 1 ) {

      std::pair < xranc_api_id_et, void * > tuple_rcvd = xranc_comm_api_ptr->xranc_rcv_message();


      if ( tuple_rcvd.first == XRANC_CELL_CONFIG_REPORT ) {
         // cout << "Validity of the received message - PASSED " << endl;
         cell_config_report_m *cellConfigReport = (cell_config_report_m *)tuple_rcvd.second;

         //xranc_state_ptr->xranc_cell_config_update (cellConfigReport);

         uint32_t cellId = 0;
         for ( ; cellId < xranc_state_ptr->getNumActiveCells() ; cellId++ ) {
            if ( *((uint32_t *)(xranc_state_ptr->getCell(cellId)->getCellInfo()->eci.cell_identity)) ==
                  *((uint32_t *)(cellConfigReport->ecgi.cell_identity)) ) {

               xranc_cell *xranc_cell_ptr = xranc_state_ptr->getCell(cellId);
               xranc_cell_ptr->setCellConfig(cellConfigReport);
               cout << "XRANC_CELL_CONFIG_REPORT received for Cell-Id: " << (uint16_t) (cellId+1) << endl << endl;

               // Send L2 Measurement config for cell
               xranc_comm_api_ptr->xranc_l2_meas_config(
                     xranc_state_ptr->getCell(cellId),
                     cellId);

               break;
            }
         }

         if( cellId == xranc_state_ptr->getNumActiveCells() ) {
            cout << "XRANC_CELL_CONFIG_REPORT received for a cell not in configuration; ECI:" <<
               hex << 	*((uint32*)(cellConfigReport->ecgi.cell_identity)) << endl << endl;
         }


      } else if ( tuple_rcvd.first == XRANC_UE_ADMISSION_REQUEST || tuple_rcvd.first == XRANC_UE_ADMISSION_STATUS) {

         ueAdmissionControl(tuple_rcvd.first, tuple_rcvd.second);

      }  else if ( tuple_rcvd.first == XRANC_UE_RELEASE_IND ) {
         //cout << "Validity of the received message - PASSED " << endl;
         ue_release_ind_m *uerelind = (ue_release_ind_m *)tuple_rcvd.second;
         xranc_cell *xranc_cell_ptr = NULL;

         for ( uint32_t cellId = 0 ; cellId < xranc_state_ptr->getNumActiveCells() ; cellId++ ) {
            if ( *((uint32*)(xranc_state_ptr->getCell(cellId)->getCellInfo()->eci.cell_identity)) ==
                  *((uint32*)(uerelind->ecgi.cell_identity)) ) {
               //cout << "Cell ID match " << endl;
               cout << "XRANC_UE_RELEASE_IND" << endl <<
                  "ECI: " << hex << *((uint32*)(uerelind->ecgi.cell_identity)) <<
                  "; CRNTI: " << uerelind->crnti <<
                  "; REL REASON: " << uerelind->ue_rel_reason <<endl << endl;

               xranc_cell_ptr = xranc_state_ptr->getCell(cellId);
               xranc_ue *xranc_ue_ptr = xranc_cell_ptr->removeUE ( uerelind->crnti );

               // Free memory and delete user
               if( xranc_ue_ptr != NULL ){

                  // Free bearer info memory
                  uint16_t bearerId;
                  for (bearerId = 0; bearerId < xranc_ue_ptr->getNumBearers(); bearerId++)
                  {
                     free(xranc_ue_ptr->getUEInfo()->bearer_info[bearerId]);
                  }

                  delete xranc_ue_ptr;
                  xranc_state_ptr->updateUsers (FALSE);
               }
               break;
            }
         }
      }  else if ( tuple_rcvd.first == XRANC_HANDOFF_COMPLETE || tuple_rcvd.first == XRANC_HANDOFF_FAILURE ) {
         hoControl(tuple_rcvd.first, tuple_rcvd.second);
      }  else if ( tuple_rcvd.first == XRANC_BEARER_ADMISSION_REQUEST || tuple_rcvd.first == XRANC_BEARER_ADMISSION_STATUS ) {
         bearerAdmissionControl (tuple_rcvd.first , tuple_rcvd.second);
      }  else if (tuple_rcvd.first == XRANC_BEARER_RELEASE_IND ) {
         bearerRelease (tuple_rcvd.first , tuple_rcvd.second);
      }  else if (tuple_rcvd.first == XRANC_RX_SIGNAL_MEAS_REPORT ) {
    	  updateRadioMeasurements (tuple_rcvd.first , tuple_rcvd.second );
    	 cout << "XRANC_MEAS_REPORT received" << endl;
      }  else if (tuple_rcvd.first == XRANC_RADIO_MEAS_REPORT_PER_UE ) {
    	  updateRadioMeasurements (tuple_rcvd.first , tuple_rcvd.second );
    	 cout << "XRANC_RADIO_MEAS_REPORT_PER_UE received " << endl;
      }  else if ( tuple_rcvd.first == XRANC_RADIO_MEAS_REPORT_PER_CELL ) {
    	  updateRadioMeasurements (tuple_rcvd.first , tuple_rcvd.second );
    	 cout << "XRANC_RADIO_MEAS_REPORT_PER_CELL received " << endl;
      }  else if ( tuple_rcvd.first == XRANC_SCHED_MEAS_REPORT_PER_UE ) {
    	  updateSchedulerMeasurements (tuple_rcvd.first , tuple_rcvd.second );
      }  else if ( tuple_rcvd.first == XRANC_SCHED_MEAS_REPORT_PER_CELL ) {
    	  updateSchedulerMeasurements (tuple_rcvd.first , tuple_rcvd.second );
      }  else if ( tuple_rcvd.first == XRANC_PDCP_MEAS_REPORT_PER_UE ) {
    	  updatePdcpMeasurements (tuple_rcvd.first , tuple_rcvd.second );
      }  else{

         cout << "WARNING: xranc does not support API ID: " << tuple_rcvd.first << endl << endl;
         //exit(-1);
      }

   }
}

void *eventTriggerThread (void *xranc_ptr)
{
	xranc *xranc_func_ptr = (xranc *)xranc_ptr;
	xranc_func_ptr->send();
	return (void *)NULL;
}

void mainThread (xranc *xranc_ptr)
{
	pthread_t tid , tid1;
	pthread_create (&tid1 , NULL , &cellConfigThread , (void *)xranc_ptr);
	pthread_create (&tid , NULL , &eventTriggerThread , (void *)xranc_ptr);
	xranc_ptr->receive();
}

int main (int argc, char* argv[])
{
	std::string configFile;
	std::string logFile;
   if(argv[1] == NULL && argv[2] == NULL)
   {
      cout << "No config and log files specified in the argument. Using xranc_config_file.cfg and xranc_log_file.cfg as the default files." << endl;
      configFile = "xranc_config_file.cfg";
      logFile = "xranc_log_file.cfg";
   }
   else if ( argv[2] == NULL )
   {
      configFile = argv[1];
      logFile = "xranc_log_file.cfg";
   } else {
	 configFile = argv[1];
	 logFile = argv[2];
   }


   xranc *xranc_ptr = new xranc (configFile , logFile);
	mainThread (xranc_ptr);
	return 0;
}
