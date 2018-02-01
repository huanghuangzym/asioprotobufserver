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
		if(false == init())  //��ʼ��ʧ�ܣ�ɱ���Լ�
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
		///��һ����Ҫ�ȴ�����Ϣ
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

	bool waitForChunkInfo();//��ͣ�ȴ�cloudagent�·�����Ϣ���յ�����

	
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
	


	bool respondlParamterRefresh();///��Ӧ����ˢ��
	bool chunkInfoRefresh();  ///chunk��ˢ��


private:

	Bmco::Logger&					theLogger;
	
	Bmco::LocalDateTime  m_OmcNextExeTime;
	
	std::string sChunkPath;
	
	Matrix  m_chunkmtx;
	bool m_isNeedWaiting;   //��������֮����Ҫ�жϸ��ֶΣ�true��ʾ��Ҫ����ȴ�����false��ʾ����Ҫ�ٽ���
};


} //namespace BM35 
#endif
