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

xranc_state :: xranc_state (uint16_t num_active_cells)
{
	this->num_active_cells = num_active_cells;
	cells = new xranc_cell*[this->num_active_cells];
	total_num_users = 0;
	banner = "";
}

xranc_cell *xranc_state :: getCell (uint16_t cellId)
{
	return cells[cellId];
}

void xranc_state :: update_cell_info (uint16_t cellId ,
      std::string eciString,
      uint32_t rx_signal_meas_report_interval_ms,
      uint32_t l2_meas_report_interval_ms)
{
	xranc_cell *xranc_cell_ptr = new xranc_cell (eciString,
         rx_signal_meas_report_interval_ms,
         l2_meas_report_interval_ms);

	cells[cellId] = xranc_cell_ptr;
}

void xranc_state :: updateUsers (bool addDelChoice)
{
	if ( addDelChoice == TRUE ) {
		total_num_users++;
	} else {
		total_num_users--;
	}
	cout << "Total number of users = " << total_num_users << endl << endl;
}

uint32_t xranc_state :: getTotalNumUsers ()
{
	return total_num_users;
}

uint16_t xranc_state :: getNumActiveCells ()
{
	return num_active_cells;
}
//Refer to class xranc_comm_api in xranc.h

event_processor_t :: ~event_processor_t ()
{

}

ue_admission_request_processor_t :: ue_admission_request_processor_t (xranc_state *xranc_state_ptr , ue_admission_request_m *ue_adm_req_ptr )
{
	this->xranc_state_ptr = xranc_state_ptr;
	this->ue_adm_req_ptr = ue_adm_req_ptr;
}

ue_admission_request_processor_t :: ~ue_admission_request_processor_t ()
{

}

handover_processor_t :: handover_processor_t (xranc_state *xranc_state_ptr , handoff_request_m *ho_request)
{
	this->xranc_state_ptr = xranc_state_ptr;
	this->ho_request = ho_request;
}

handover_processor_t :: ~handover_processor_t ()
{

}

bool handover_processor_t :: processEvent ()
{
	static uint32_t trackerId = 1;
	char trackerStr[32];
	snprintf(trackerStr , sizeof(trackerStr) , "%u", trackerId);
	trackerStr[sizeof(trackerStr)] = '\0';
	std::string trackerString(trackerStr);
	std::string fileName = "handover_" + trackerString + ".txt";
	std::ifstream ifsInputFile (fileName.c_str() , std::ifstream::in);

	if ( ifsInputFile.is_open() ) {

      cout << "Handover File: " << fileName << " found " << endl;
		std::string strLine;
		while (std::getline(ifsInputFile , strLine)) {
			std::stringstream ssWordsBuf (strLine);
			std::string strWord;
			ssWordsBuf >> strWord;

         cout << strWord << endl;

			if ( strWord.compare("CRNTI") == 0 ) {
				ssWordsBuf >> strWord;
				ho_request->crnti = (uint32_t)atoi(strWord.c_str());
            cout << "CRNTI: " << ho_request->crnti << endl;
			} else if ( strWord.compare("SOURCE_CELL") == 0 ) {
				ssWordsBuf >> strWord;
				uint32_t srcId = (uint32_t)atoi(strWord.c_str());
				eutran_global_cell_id_t srcCellId = xranc_state_ptr->getCell(srcId-1)->getCellInfo()->eci;
				ho_request->ecgi_s = srcCellId;
            cout << "src Cell Id " << srcId << endl;
			} else if ( strWord.compare("TARGET_CELL") == 0 ) {
				ssWordsBuf >> strWord;
				uint32_t destId = (uint32_t)atoi(strWord.c_str());
				eutran_global_cell_id_t destCellId = xranc_state_ptr->getCell(destId-1)->getCellInfo()->eci;
				ho_request->ecgi_t = destCellId;
				cout << "tgt Cell Id " << destId << endl;
			}
		}

		trackerId++;
		ifsInputFile.close();
		return TRUE;
	} else {
		return FALSE;
	}
}

bool ue_admission_request_processor_t :: processEvent ()
{
   return TRUE;
}

