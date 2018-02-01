#ifndef __REGULARPROCESSTASKLOOP__H__
#define __REGULARPROCESSTASKLOOP__H__
#include "Bmco/Task.h"
#include "Bmco/Util/Application.h"
#include "Bmco/Environment.h"
#include "Bmco/Process.h"
#include "Bmco/Any.h"
#include "Bmco/Format.h"
#include "Bmco/Foundation.h"
#include "Bmco/String.h"
#include "MemTableOp.h"
#include "MetaBpcbInfo.h"
#include "MetaProgramDef.h"
#include "MetaRegularProcessTask.h"
#include "BolServiceLog.h"

using Bmco::Util::Application;
using Bmco::Process;
using Bmco::ProcessHandle;
using Bmco::format;
using Bmco::SystemException;

namespace BM35 {

static const char  REGUALAR_PROCESS_TASK_NAME[] =  "Regular_Process_Task_Loop_Thread";
static const char  UNIQUE_SEQUENCE_BPCB_INFO[] = "unique_sequence_bpcb_info";

extern const std::string LDENVNAME;

struct ManagedChildInfo
{
	bool                        isBolProcess;   //true:bol , false:hla or other 
	
	Bmco::Timestamp				launchTime;		//time of launch child

	Bmco::Int64				bpcbId;			//record the bpcb id that should be managed
	Bmco::Timestamp::TimeDiff	expSeconds;		//expire seconds from .ini file
	
	int taskvalid;

	int is_warning;
	
	MetaProgramDef::Ptr					 programDef;	// static info about child program
	MetaBpcbInfo::Ptr					 bpcbItem;		// query from BPCB table
	int                                 isNoMonitor;   //flag for monitor process heartbeating,true:just warn false:relaunch it
	std::string                          exCommand;     //extend command paramter for process starting 
	std::string                          strorgid;
	std::string                          dynamicargs;

	int         _exeminutes;//��ʱ�����ִ��ʱ�̵ķ���
	bool      _isExecuted;//�Ƿ�ִ�й�

	char cl_Dow[7];                 /* 0-6, beginning sunday */
	char cl_Mons[12];               /* 0-11 */
	char cl_Hrs[24];                /* 0-23 */
	char cl_Days[32];               /* 1-31 */
	char cl_Mins[60];               /* 0-59 */
	
	
	ManagedChildInfo():
		isBolProcess(false)
		, expSeconds(30)
		, programDef(NULL)
		, bpcbItem(NULL)
		,isNoMonitor(0)
		,taskvalid(CONTROL_START)
		,exCommand("")
		,is_warning(1)
		,strorgid("-1")
		,_exeminutes(-1)
		,_isExecuted(false)
		{

		memset(cl_Dow, 0, 7);
		memset(cl_Mons, 0, 12);
		memset(cl_Hrs, 0, 24);
		memset(cl_Days, 0, 32);
		memset(cl_Mins, 0, 60);
		
	}
};



struct CrontabExecuteInfo
{
	bool                        isExecuted;   //��ǰ�����Ƿ�ִ�й�
	int                  exeMinute;//��ǰִ�еķ���
	
	
	CrontabExecuteInfo():
		isExecuted(false)
		, exeMinute(-1)
		{}
};



////���̵�Ψһ�����ṹ
struct ProcessPrimaryKey
{
	Bmco::Int32 m_iProgramID;  //����ID
	Bmco::Int32 m_iFlowID;    //��������ID
	Bmco::Int32 m_iInstanceID; //ʵ��ID
	//Bmco::UInt32 m_iFlowStep; //���������еĽ׶�

	ProcessPrimaryKey(const MetaRegularProcessTask & r)
	{
		m_iProgramID = r.ProgramID;
		m_iFlowID = r.FlowID;
		m_iInstanceID = r.InstanceID;
	}


	ProcessPrimaryKey(Bmco::Int32 iProgramID,Bmco::Int32 iFolwID,Bmco::Int32 iInstanceID)
	{
		m_iProgramID = iProgramID;
		m_iFlowID = iFolwID;
		m_iInstanceID = iInstanceID;
	}
	

	ProcessPrimaryKey(const ProcessPrimaryKey &r)
	{
		m_iProgramID = r.m_iProgramID;
		m_iFlowID = r.m_iFlowID;
		m_iInstanceID = r.m_iInstanceID;
		/*m_iFlowStep = r.m_iFlowStep;*/
	}

	bool operator <(const ProcessPrimaryKey & r ) const
	{
		std::string str1 = Bmco::format("%?d_%?d_%?d",m_iProgramID,m_iFlowID,m_iInstanceID/*,m_iFlowStep*/);
        std::string str2 = Bmco::format("%?d_%?d_%?d",r.m_iProgramID,r.m_iFlowID,r.m_iInstanceID/*,r.m_iFlowStep*/);
        return str1<str2;
	}

	std::string Print()
	{
		return Bmco::format("ProgramID:%?d,FlowID:%?d,InstanceID:%?d",m_iProgramID,m_iFlowID,m_iInstanceID/*,m_iFlowStep*/);
	}
};


class ScopedEnvSetter
{
public:
	ScopedEnvSetter(const std::string &oldenv,const std::string &prolibpath): m_oldenv(oldenv)
	{

		std::string realpath=Bmco::replace(prolibpath,
			std::string("${common.vnodepath}"),Bmco::Util::Application::instance().config().getString("common.vnodepath",""));
		
	
		std::string newenv=realpath+":"+oldenv;
		Bmco::Environment::set(LDENVNAME,newenv);
		INFORMATION_LOG(" %s is %s",LDENVNAME.c_str(),Bmco::Environment::get(LDENVNAME).c_str());
		
	}
	
	
	~ScopedEnvSetter()
	{
		Bmco::Environment::set(LDENVNAME,m_oldenv);
		INFORMATION_LOG(" %s is %s",LDENVNAME.c_str(),Bmco::Environment::get(LDENVNAME).c_str());
	}

private:
	std::string m_oldenv;
	
	ScopedEnvSetter();
	ScopedEnvSetter(const ScopedEnvSetter&);
	ScopedEnvSetter& operator = (const ScopedEnvSetter&);
};



class RegularProcessTaskLoop: public Bmco::Task
{
   public:
	   RegularProcessTaskLoop(): theLogger(Bmco::Util::Application::instance().logger())
		, Task(REGUALAR_PROCESS_TASK_NAME)
	   {
		   mapProcessHandle.clear();
		   m_vecRegularTaskLast.clear();
		   m_isNeedWaiting = true;
	   }

	   void runTask();
		
	   virtual ~RegularProcessTaskLoop() {
		  
		   mapProcessHandle.clear();
		   m_vecRegularTaskLast.clear();
	   }


		/// do check and initlization before task loop
		virtual bool beforeTask()
		{
			
			
			m_ldenv=Bmco::Environment::get(LDENVNAME);
			bmco_information_f3(theLogger,"%s|%s| LDENV {%s}.",
					std::string("0"),std::string(__FUNCTION__),m_ldenv);

			m_lexpSeconds=Bmco::Util::Application::instance().config().getInt("kernel.expiresec",30);
			m_iscontrolcpuram=Bmco::Util::Application::instance().config().getBool("kernel.isControlCpuRam",false);
			m_vnodepath=Bmco::Util::Application::instance().config().getString("common.vnodepath","/opt/opthb/");
			

			if(!init()) 
			{ //��ʼ��ʧ�ܣ�ɱ���Լ�
				this->sleep(3000);          //����һ�������ĳ�����ʱ�����BPCB
			    	terminateAllChild();	//�˳������̣߳�����Ҫ�����е��������Ľ��̶�ͣ��
				Process::requestTermination(Process::id());
				return false;
			}

			return true;
		}
	
		/// do uninitlization after task loop
		virtual bool afterTask()
		{
			//�˳������̣߳�����Ҫ�����е��������Ľ��̶�ͣ��
			terminateAllChild();
			return true;
		}
	
		/// perform business logic
		virtual bool handleOneUnit()
		{
			///��һ����Ҫ�ȴ���������
			if(m_isNeedWaiting)
			{
				
				if(! waitForRegularTask())
				{
					checkProcess();  ///�ȴ������·���ʱ����2�ź�5�Ž���
					processTimeTask();
					if(!getAllRegularTask(m_vecRegularTaskLast))
					{
						bmco_error_f2(theLogger,"%s|%s|Failed to execute QueryAll on MetaShmRegularProcessTaskTable",std::string("0"),std::string(__FUNCTION__));
					}
					return false;
				}
				m_isNeedWaiting = false;
			}

			
			respondlParamterRefresh();
			checkProcess();


			processTimeTask();
			updateBpcbUsingInfo(BOL_PROCESS_KERNEL);
			
			this->sleep(3000);
			return true;
		}
		
	   	
	    bool RegistBpcbInfo(MetaBpcbInfo::Ptr & bpcbptr);

		bool RestetBpcb(MetaRegularProcessTask::Ptr& ptr);

		bool getOneCronChildInfo(int cronid,int programid);

		 bool getBpcbItemByID(Bmco::Int32 bpcbid,MetaBpcbInfo::Ptr & bpcbptr);

		 bool getAllRegularTask(std::vector<MetaRegularProcessTask>& vecMetaRegularProcessTask);

		void DeleteOldBpcbInfo(BM35::Matrix&);

		  bool getAllBpcbItem(std::map<Bmco::Int32,MetaBpcbInfo::Ptr>& bpcbmap);

		  bool PauseOneChild(Bmco::Int64 iBpcbID);

		bool isBolStatueNormal();

		bool IsProcessHasStoped(MetaRegularProcessTask::Ptr ptr);

		/// update bol status in control region to _s
		bool setBolStatus(BolStatus _s);

		bool CheckUpdateFile();

		bool processTimeTask();
		  

	   bool init();
	   bool waitForRegularTask(); //��ͣ�ȴ�cloudagent�·������յ�����
	   bool checkProcess();		//����BPCB�Ľ��̼��
	   MetaProgramDef::Ptr getProcessTypeByID(Bmco::Int32 programID); //ͨ������ID��ȡ��������
	   
	   bool terminateOneChild(MetaRegularProcessTask::Ptr ptr); ///isInit��ʾ��ʼ��ʧ�ܵ�ʱ����Ҫɱ��֮ǰ��Ľ���
	   bool launchOneChild(MetaRegularProcessTask::Ptr& ptr);
	   bool getOneChildInfo(MetaRegularProcessTask::Ptr& ptr);
	  
	   bool terminateAllChild();///isInit��ʾ��ʼ��ʧ�ܵ�ʱ����Ҫɱ��֮ǰ��Ľ���
	   bool updateBpcbUsingInfo(Bmco::Int64 iBpcbID); //����1#�ų��������Ľ��̵�cpu��mem
	   
	  
	   bool respondlParamterRefresh(); ///��Ӧ���·��Ĳ���ˢ�£��Ƚ����ϳ�������Ͷ�ʱ����

	   bool programeDefRefresh()  ///����mapProcessHandle��ManagedChildInfo���˳������ָ�룬����Ҫ��Ӧ������ı仯
	   {
		   for(std::map<ProcessPrimaryKey,ManagedChildInfo >::iterator it = mapProcessHandle.begin();
			   it != mapProcessHandle.end();it++)
		   {
		   		
				it->second.programDef = getProcessTypeByID(it->first.m_iProgramID);
		   }
		   return true;
	   }


	   bool regularProcessRefresh()      ///RegularProcess��ˢ��
	   {
		 
			std::vector<MetaRegularProcessTask> vecMetaRegularProcessTask;
			std::vector<MetaRegularProcessTask> vecRegularTask = m_vecRegularTaskLast;

			if( !getAllRegularTask(vecMetaRegularProcessTask))
			{
				bmco_error_f2(theLogger,"%s|%s|Failed to execute QueryAll on MetaShmRegularProcessTaskTable",std::string("0"),std::string(__FUNCTION__));
				return false;
			}

			m_vecRegularTaskLast = vecMetaRegularProcessTask; ///�������������б�

			bmco_information_f4(theLogger,"%s|%s| ,before task compare,old task num is %?d, new task num is %?d.",
					std::string("0"),std::string(__FUNCTION__),vecRegularTask.size(),vecMetaRegularProcessTask.size());

			///��������ȶԣ��������ģ����µĽ��д���ɾ���ļ������״̬ΪֹͣҲ���Խ���ˢ��
			
			std::list<MetaRegularProcessTask> listNew;
			  listNew.assign(vecMetaRegularProcessTask.begin(),vecMetaRegularProcessTask.end());
			  std::list<MetaRegularProcessTask> listOld;
			  listOld.assign(vecRegularTask.begin(),vecRegularTask.end());
				std::list<MetaRegularProcessTask>::iterator itNew;
				std::list<MetaRegularProcessTask>::iterator itOld;;
				typedef std::list<MetaRegularProcessTask>::iterator itTask;
				std::list<itTask> vecNew;vecNew.clear();
				std::list<itTask> vecOld;vecOld.clear();
				for(itNew = listNew.begin();itNew != listNew.end();itNew++)
				{
					for(itOld = listOld.begin();itOld != listOld.end();itOld++)
					{
					  if((itNew->ProgramID == itOld->ProgramID )&&
					  	(itNew->InstanceID == itOld->InstanceID)&&
					  	(itNew->FlowID == itOld->FlowID))
					  {
					  	if(mapProcessHandle.find(ProcessPrimaryKey(*itNew)) != mapProcessHandle.end())
						  {
						  	if(0 != Bmco::icompare(mapProcessHandle.find(ProcessPrimaryKey(*itNew))->second.exCommand,itNew->ExCommand.c_str()))
  						  {
  							  bmco_information_f4(theLogger,"%s|%s|BPCB-ID[%?d]:ExCommand is changed %s.",
  										std::string("0"),std::string(__FUNCTION__),mapProcessHandle.find(ProcessPrimaryKey(*itNew))->second.bpcbId,
  										Bmco::format("[%s] to [%s]",mapProcessHandle.find(ProcessPrimaryKey(*itNew))->second.exCommand,std::string(itNew->ExCommand.c_str())));
  							  mapProcessHandle.find(ProcessPrimaryKey(*itNew))->second.exCommand = itNew->ExCommand.c_str();

  						  }

						
						
						  if(itOld->IsValid != itNew->IsValid)
  						  {
  							  bmco_information_f4(theLogger,"%s|%s|BPCB-ID[%?d]:IsValid is changed %s.",
  										std::string("0"),std::string(__FUNCTION__),mapProcessHandle.find(ProcessPrimaryKey(*itNew))->second.bpcbId,
  										Bmco::format("[%?d] to [%?d]",itOld->IsValid,itNew->IsValid));
  
  							  std::map<ProcessPrimaryKey,ManagedChildInfo >::iterator itMap = mapProcessHandle.find(ProcessPrimaryKey(*itNew));
							  
  							  MetaRegularProcessTask::Ptr ptr = new  MetaRegularProcessTask(itNew->ID,itMap->first.m_iProgramID,
  								 itMap->first.m_iInstanceID,itMap->first.m_iFlowID,itNew->IsValid,0,false,0,
  								 itMap->second.exCommand.c_str(),itMap->second.isNoMonitor,itNew->pause_status,
  								 itNew->is_warning,itNew->OrgID,itNew->dynamic_param);
  							  if(itNew->IsValid == CONTROL_START)
  							  {
								  
							  	  RestetBpcb(ptr);
  								  launchOneChild(ptr);
  							  }
							  else
  							  {
  								  terminateOneChild(ptr);
  							  }
  						  }

						  //��Ӧ��ͣ���ָ�Ӧ��
						  if(itOld->pause_status != itNew->pause_status)
  						  {
  							  bmco_information_f4(theLogger,"%s|%s|BPCB-ID[%?d]:pause_status is changed %s.",
  										std::string("0"),std::string(__FUNCTION__),mapProcessHandle.find(ProcessPrimaryKey(*itNew))->second.bpcbId,
  										Bmco::format("[%?d] to [%?d]",itOld->pause_status,itNew->pause_status));
  
  							  std::map<ProcessPrimaryKey,ManagedChildInfo >::iterator itMap = 
							  			mapProcessHandle.find(ProcessPrimaryKey(*itNew));

							  if(itMap != mapProcessHandle.end())
							  {
							  	 
								 if(MemTableOper::instance().UpdateBpcbAction( itMap->second.bpcbId, itNew->pause_status))
  							  	 {
  							  	 	PauseOneChild(itMap->second.bpcbId);
  							  	 }
								 else
								 {
								 	ERROR_LOG(0, "update bpcb [%?d] to [%?d] failed",itMap->second.bpcbId,itNew->pause_status);
								 }
	  							  
							  }
							  
  						  }


						  //���Ӧ������̬ͣ����ֱ�Ӹ���Ϊ��̬ͣ
						  /*
						  if((1 == itNew->pause_status)  && (itNew->IsValid==1))
						  {
						  	std::map<ProcessPrimaryKey,ManagedChildInfo >::iterator itMap = 
							  			mapProcessHandle.find(ProcessPrimaryKey(*itNew));

							  if(itMap != mapProcessHandle.end())
							  {
							  	if(!MemTableOper::instance().UpdateBpcbStatusByID( itMap->second.bpcbId, ST_PAUSE))
							  	 {
							  	 	ERROR_LOG(0, "update bpcb [%d] to ST_PAUSE failed",itMap->second.bpcbId);
							  	 }
							  }
						  	
						  }
						  */

						 
  						   mapProcessHandle.find(ProcessPrimaryKey(*itNew))->second.isNoMonitor = itNew->IsNoMonitor;
						   mapProcessHandle.find(ProcessPrimaryKey(*itNew))->second.taskvalid = itNew->IsValid;
						   mapProcessHandle.find(ProcessPrimaryKey(*itNew))->second.is_warning = itNew->is_warning;
						   mapProcessHandle.find(ProcessPrimaryKey(*itNew))->second.strorgid = itNew->OrgID;
						    mapProcessHandle.find(ProcessPrimaryKey(*itNew))->second.dynamicargs = itNew->dynamic_param;
  
  						  vecNew.push_back(itNew);
						  bmco_information_f3(theLogger,"put old in  vecOld taskid %?d proid %?d  flowid %?d.",
						  		(*itOld).ID,(*itOld).ProgramID,(*itOld).FlowID);
  						  vecOld.push_back(itOld);
  						  break;
						  }
					  }
					}
				}	

				if(!vecNew.empty())
				{
					for(std::list<itTask>::iterator itN  = vecNew.begin();itN != vecNew.end(); itN++)
					{
						listNew.erase(*itN);
					}
				}

				if(!vecOld.empty())
				{
					for(std::list<itTask>::iterator itO  = vecOld.begin();itO != vecOld.end(); itO++)
					{
						 bmco_information_f3(theLogger," erase old in  vecOld taskid %?d proid %?d  flowid %?d.",
						  		(*itO)->ID,(*itO)->ProgramID,(*itO)->FlowID);
						listOld.erase(*itO);
					}
				}
				
				bmco_information_f3(theLogger,"%s|%s| the same task num is %?d.",
					std::string("0"),std::string(__FUNCTION__),vecNew.size());
				
				bmco_information_f4(theLogger,"%s|%s| ,after task compare,leaving old task num is %?d, adding new task num is %?d.",
					std::string("0"),std::string(__FUNCTION__),vecOld.size() - listOld.size(),listNew.size());

			///�����������õ�����
			if(listNew.size() != 0)
			{
				for(std::list<MetaRegularProcessTask>::iterator it = listNew.begin(); 
					it != listNew.end(); it++)
				{
					bmco_information_f3(theLogger,"%s|%s|new added task is %s.",
						std::string("0"),std::string(__FUNCTION__),ProcessPrimaryKey(*it).Print());

					it->Priority = getProcessTypeByID(it->ProgramID)->Priority;
				}
				////�������ȼ�����
				listNew.sort();
				 for(std::list<MetaRegularProcessTask>::iterator it1 = listNew.begin(); 
					it1 != listNew.end(); it1++)
				 {
					MetaRegularProcessTask::Ptr ptr = new MetaRegularProcessTask(*it1);
					if(false == getOneChildInfo(ptr))
						continue;
					if(false == launchOneChild(ptr))
					{
						bmco_error_f4(theLogger,"%s|%s|����ID[%?d],����ID[%?d]:��������ʧ��",
							std::string("0"),std::string(__FUNCTION__),ptr->ID,ptr->ProgramID);
					}
				 }
			}
			///����ɾ�����õ������ж���Щ�����Ƿ���ڣ����粻���ڿ��Դ�bpcb�����
			if(listOld.size() != 0)
			{
				for(std::list<MetaRegularProcessTask>::iterator it = listOld.begin(); 
					it != listOld.end(); it++)
				{

					 
					 MetaRegularProcessTask::Ptr ptr = new MetaRegularProcessTask(*it);
					 ptr->IsValid = CONTROL_TERMINATE;
					 terminateOneChild(ptr);
				
					bmco_information_f3(theLogger,"%s|%s|old canceled task is %s.",
						std::string("0"),std::string(__FUNCTION__),ProcessPrimaryKey(*it).Print());
					//ע��bpcb
					if(MemTableOper::instance().DeleteBpcbInfoBybpcbd(mapProcessHandle.find(ProcessPrimaryKey(*it))->second.bpcbId))
					{
						bmco_information_f3(theLogger,"%s|%s|the process BPCB-ID[%?d] related to this task is not alive,do unregistBpcbInfo sucessfully.",
							std::string("0"),std::string(__FUNCTION__),mapProcessHandle.find(ProcessPrimaryKey(*it))->second.bpcbId);
					}
					else
					{
						bmco_error_f3(theLogger,"%s|%s|the process BPCB-ID[%?d] related to this task is not alive,fail to do unregistBpcbInfo.",
							std::string("0"),std::string(__FUNCTION__),mapProcessHandle.find(ProcessPrimaryKey(*it))->second.bpcbId);
					}
				   
					mapProcessHandle.erase(ProcessPrimaryKey(*it));
				}
			}
         		return true;
	   }

  private:
	 Bmco::Logger&					theLogger;
	  

	  std::map<ProcessPrimaryKey,ManagedChildInfo > mapProcessHandle; ///���̵�״̬

	 std::map<int,ManagedChildInfo> m_CronmapProcessHandle;//��ʱ����״̬

	  
	  std::set<int> m_bolstartBpcbset;
	  
	  std::string m_vnodepath;
	
	  Bmco::UInt64 m_lexpSeconds;
	  bool m_iscontrolcpuram;

	  std::vector<MetaRegularProcessTask> m_vecRegularTaskLast;  ///��¼��һ�ε�����
	  /////��Ӧ����ˢ�£����·�������Ҫ��vecRegularTaskLast���жԱ�
	  /////mapProcessHandle��BPCB���жԱȼ�飬�Ӷ���ؽ���״̬


	  std::string m_ldenv;

         Bmco::LocalDateTime  m_OmcNextExeTime;

	  Bmco::Int32  m_lastminute;//��ʱ������ϴ�ִ�еķ���ֵ

		 
	  bool m_isNeedWaiting;   //��������֮����Ҫ�жϸ��ֶΣ�true��ʾ��Ҫ����ȴ�����false��ʾ����Ҫ�ٽ���
};

}
#endif//__REGULARPROCESSTASKLOOP__H__
