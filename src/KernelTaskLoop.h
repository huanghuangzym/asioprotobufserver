#ifndef __KERNELTASKLOOP__HH__
#define __KERNELTASKLOOP__HH__
#include "Bmco/Task.h"
#include "Bmco/Util/Application.h"
#include "MetaBpcbInfo.h"




namespace BM35 {
/// BOLKernel run time task name, it will be used as thread name by taskmanager
#define BOL_KERNEL_TASK_NAME  "Bol_Kernel_Task_Loop_Thread"




/// This class do runtime jobs for BOL->Kernel
/// which will be run in a dedicated thread and managed by Bmco::TaskManager
class KernelTaskLoop: public Bmco::Task
{
public:
    KernelTaskLoop(BolStatus status)
		: theLogger(Bmco::Util::Application::instance().logger())
		, bolStatus(status)
		, Task(BOL_KERNEL_TASK_NAME) 
	{
		
	}
	virtual ~KernelTaskLoop(void) {
	}
    
	/// this function execute kernel runtime jobs periodically 
	/// in a dedicated task thead managed by taskmanager
    virtual void runTask();

protected:
	/// set self status and launch #1
	bool beforeTaskLoop();

	/// main loop to monitor #1 and response external request (snapshot/shrink/grow etc)
	bool onTaskLoop();

	/// terminate child process and wait their exiting
	bool afterTaskLoop();

	/// one unit of run time job, include (update bpcb, check log level and manage child process)
	bool handlOneRuntimeJob();

private:
	

	/// update bpcb status and syspid in bpcb
	bool updateSelfBpcbItems(BpcbProcessState _s, Bmco::UInt32 _pid = 0);
	/// update heartbeat in bpcb
	bool keepBpcbItemAlive(BolProcess bpcbid);
	
	
	

	/// query snapshot flag from bpcb
	Bmco::UInt32 getSnapshotFlag();
	/// update snapshot flag in bpcb to f
	bool updateSnapshotFlag(Bmco::Int32 f);


	/// refresh #0 log level and petri result file path
	void refreshLogLevelAndPetriFile();



private:
	/// the logger
	Bmco::Logger&					theLogger;
	/// bol status
	BolStatus						bolStatus;
	
	
};
} //namespace BM35 
#endif

