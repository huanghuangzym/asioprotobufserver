#ifndef _TIMETASKLOOP_H__
#define _TIMETASKLOOP_H__

#include "Bmco/Task.h"
#include "Bmco/Util/Application.h"
#include "OSInfoInterface.h"
#include "MetaBpcbInfo.h"

using Bmco::Util::Application;
namespace BM35
{
static const char BOL_CRONTAB_TASK_NAME[] =  "BOL_Crontab_Task_Loop_Thread";
class TimeTaskLoop : public Bmco::Task
{
public:
	TimeTaskLoop(): theLogger(Bmco::Util::Application::instance().logger())
		, Task(BOL_CRONTAB_TASK_NAME) 
	{}
	 
	virtual ~TimeTaskLoop(void){
		
	}

	void runTask();

	/// do check and initlization before task loop
	virtual bool beforeTask(){
		return true;
	}
	
	/// do uninitlization after task loop
	virtual bool afterTask(){
		return true;
	}

	
	
	/// perform business logic
	virtual bool handleOneUnit()
	{
		processTimeTask(); //ִ�г�������
		updateBpcbUsingInfo(BOL_PROCESS_KERNEL); //����0#
		this->sleep(3000);

		return true;
	}

protected:
	
	
	//����ʱ����
	bool processTimeTask();

	//��ʱ����BPCB��0#,1#��cpu,mem etc��
	bool updateBpcbUsingInfo(Bmco::UInt64 iBpcbID);

private:
	Bmco::Logger&					theLogger;
};

}

#endif //_TIMETASKLOOP_H__

