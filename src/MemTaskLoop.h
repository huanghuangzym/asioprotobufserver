#ifndef __MEMTASKLOOP__HH__
#define __MEMTASKLOOP__HH__

#include "Bmco/Task.h"
#include "Bmco/Util/Application.h"
#include "Bmco/Process.h"
#include "Bmco/String.h"
#include "MemTableOp.h"



using Bmco::Util::Application;
using Bmco::Process;
namespace BM35 {
/// BOL->MemManager run time task name, it will be used as thread name by taskmanager
static const char BOL_MEM_TASK_NAME[] = "Bol_Mem_Task_Loop_Thread";

class MemTaskLoop : public Bmco::Task
{
public:
	MemTaskLoop(): theLogger(Bmco::Util::Application::instance().logger())
		, Task(BOL_MEM_TASK_NAME) 
	  {
		 
		  m_isNeedWaiting = true;
	  }
	virtual ~MemTaskLoop(void){
		
	}


	void runTask();
	
	/// do check and initlization before task loop
	virtual bool beforeTask(){
		sChunkPath.assign(Bmco::Path::forDirectory(Bmco::Util::Application::instance().config().getString("common.datapath","")).toString());
		if(false == init())  //初始化失败，杀死自己
		{
			Process::requestTermination(Process::id());
			return false;
		}

		return  true;
	}
					
	
	/// do uninitlization after task loop
	virtual bool afterTask(){
		
		return true;
	}
	
	/// perform business logic
	virtual bool handleOneUnit()
	{
		///第一次需要等待块信息
		if(m_isNeedWaiting)
		{
			if(! waitForChunkInfo())
			{
				
				if(!MemTableOper::instance().QueryTableAllData(strMetaBolCrontab,m_chunkmtx)) 
				{
					bmco_error_f2(theLogger,"%s|%s|query ChunkInfo is error",std::string("0"),std::string(__FUNCTION__));
				}
				return false;
			}
			m_isNeedWaiting = false;
		}

		
		statMemUsage();

		respondlParamterRefresh();

		this->sleep(5000);

		return true;
	}
	
protected:
	/// stat managed shm usage data and update related info in CTL Region
	bool statMemUsage();

	
	bool init();

	bool waitForChunkInfo();//不停等待cloudagent下发块信息，收到后处理

	
	///create segment
	bool doCreateSegment(Bmco::Int32 regionid,const std::string &chunkname,Bmco::Int64 shrinksize,
	Bmco::Int32 ismappedfile);

	///destory segment
	bool doDeleteSegment(Bmco::Int32 regionid,const std::string &chunkname,Bmco::Int64 shrinksize,
	Bmco::Int32 ismappedfile);

	///grow segment
	bool doGrowSegment(Bmco::Int32 regionid,const std::string &chunkname,Bmco::Int64 shrinksize,
	Bmco::Int32 ismappedfile);

	///shrink segment
	bool doShrinkSegment(Bmco::Int32 regionid,const std::string &chunkname,Bmco::Int64 shrinksize,
	Bmco::Int32 ismappedfile);
	


	bool respondlParamterRefresh();///响应参数刷新
	bool chunkInfoRefresh();  ///chunk表刷新


private:

	Bmco::Logger&					theLogger;
	
	Bmco::LocalDateTime  m_OmcNextExeTime;
	
	std::string sChunkPath;
	
	Matrix  m_chunkmtx;
	bool m_isNeedWaiting;   //进程启动之初需要判断该字段，true表示需要进入等待任务，false表示不需要再进入
};


} //namespace BM35 
#endif
