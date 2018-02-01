#include "KernelTaskLoop.h"
#include "Bmco/Format.h"
#include "Bmco/StringTokenizer.h"
#include "Bmco/String.h"
#include "Bmco/File.h"
#include "Bmco/Process.h"
#include "Bmco/NumberFormatter.h"
#include "Bmco/FileChannel.h"
#include "Bmco/Util/IniFileConfiguration.h"
#include "Bmco/Util/SystemConfiguration.h"
#include "Bmco/Util/MapConfiguration.h"
#include "Bmco/Util/LoggingConfigurator.h"
#include "boost/tokenizer.hpp"
#include "boost/algorithm/string.hpp"
#include "MemTableOp.h"
#include "VirtualShm.h"

using namespace Bmco;
using namespace BM35;


namespace BM35 {

static const std::string TIMEFMTSTR="%Y%m%d%H%M%S";

void KernelTaskLoop::runTask() {

	try
	{
		if (!beforeTaskLoop()) {
			Process::requestTermination(Process::id());
			return;
		}

		if (!onTaskLoop()) {
			Process::requestTermination(Process::id());
		}

		if (!afterTaskLoop()) return;
	}
	catch(...)
	{
		bmco_error_f2(theLogger, "%s|%s| done krenel task loop unknown exception."
			, std::string("0"),std::string(__FUNCTION__));
		return;
	}
	bmco_information_f2(theLogger, "%s|%s| done kernel task loop gracefully."
		, std::string("0"),std::string(__FUNCTION__));
}

bool KernelTaskLoop::beforeTaskLoop() {
	// set bol status
	//if (!setBolStatus(bolStatus)) return false;

	// update self items in BPCB table
	if (!updateSelfBpcbItems(ST_READY, Process::id())) return false;


	bmco_information_f2(theLogger,"%s|%s|update self bpcb info  successfully.",
			std::string("0"),std::string(__FUNCTION__));
	return true;
}



bool KernelTaskLoop::updateSelfBpcbItems(BpcbProcessState _s,  Bmco::UInt32 _pid) {
	
	if(!MemTableOper::instance().UpdateBpcbItem(_s,_pid)) {
		bmco_error_f2(theLogger,"%s|%s|failed to update sys pid item in bpcb.",
			std::string("0"),std::string(__FUNCTION__));
		return false;
	}

	return true;
}


bool KernelTaskLoop::onTaskLoop() 
{
	while (!isCancelled()) 
	{		
			// Do all the job that BOLkernel (@runtime) should do here
			
			if (!handlOneRuntimeJob())
			{
				bmco_information_f2(theLogger,"%s|%s|BOLKernel failed to do run time job and will retry later.",
					std::string("0"),std::string(__FUNCTION__));
			}
			
			this->sleep(3000);
			
			
			continue; //continue next runtime loop
	}//end while

	bmco_information_f2(theLogger,"%s|%s|exit kernel task loop gracefully",
				std::string("0"),std::string(__FUNCTION__));
	return true;
}

void KernelTaskLoop::refreshLogLevelAndPetriFile() {
	
}

Bmco::UInt32 KernelTaskLoop::getSnapshotFlag() 
{
	Bmco::Int32 flag=BPCB_SNAPSHOT_OTHER;
	MemTableOper::instance().GetSnapShotFlag(BOL_PROCESS_KERNEL,flag);	
	return flag;
}

bool KernelTaskLoop::updateSnapshotFlag(Bmco::Int32 snapflag) 
{
	return MemTableOper::instance().UpdateSnapShotFlag(BOL_PROCESS_KERNEL,snapflag);	
}


bool KernelTaskLoop::handlOneRuntimeJob() 
{
	
	// update status
	updateSelfBpcbItems(ST_RUNNING, Process::id());
	// refresh log level
	refreshLogLevelAndPetriFile();
	// bpcb heart beat
	keepBpcbItemAlive(BOL_PROCESS_KERNEL);


	
	return true;
}
	
bool KernelTaskLoop::keepBpcbItemAlive(BolProcess bpcbid) 
{
	return MemTableOper::instance().heartbeat(bpcbid);	
}




bool KernelTaskLoop::afterTaskLoop() 
{
	
	
	updateSelfBpcbItems(ST_STOP, INVALID_SYS_PID);
	

	bmco_information_f2(theLogger,"%s|%s|#0 has joined all other processes and set bol status to be offline.",
				std::string("0"),std::string(__FUNCTION__));
	return true;
}
} //namespace BM35 


