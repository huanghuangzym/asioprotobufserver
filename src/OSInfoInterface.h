#ifndef __OSINFO_INTERFACE__HH__
#define __OSINFO_INTERFACE__HH__


#include "Bmco/DateTime.h"
#include "Bmco/Process.h"
#include "Bmco/Platform.h"
namespace BM35{

//-----------------------begin no dynamci fun-------------------------------
	//get os boot time
	Bmco::DateTime GetOsBootTimeUtc();
	
	//get total physical memory
	Bmco::Int64 GetOSTotalPhyMemory();
	
//----------------------end no dynamic fun----------------------------------

//----------------------------begin dynamic functions----------------------
	//get free memory at call moment
	Bmco::Int64 GetOSFreePhyMemory();

	//get cpu use percetn at call moment
	double GetOsCpuUsePercent();


	Bmco::Int32 GetProcessCpuUse(Bmco::Process::PID pid);
	//  GetProcessTimes
	
	Bmco::Int64 GetProcessMemUse(Bmco::Process::PID pid);
	//  GetProcessMemoryInfo
	
//----------------------------end dynamic functions----------------------
}
#endif


