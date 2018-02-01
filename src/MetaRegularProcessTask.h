#ifndef _METAREGULARPROCESSTASK_HH__
#define _METAREGULARPROCESSTASK_HH__

#include "Bmco/Timestamp.h"
namespace BM35
{



enum RegularProcessTaskControl
{
	CONTROL_TERMINATE = 0,  //正常停止任务
	CONTROL_START = 1,  //启动任务
	CONTROL_KILL =2 //强杀任务
	
};


enum RegularProcessPauseControl
{
	CONTROL_RESTORE = 0,  //恢复任务
	CONTROL_PAUSE= 1 //暂停任务
};



	//初始待调度进程任务
	//来自核心参数配置文件，定义BOL启动后应该由进程管理启动的HLA层应用程序
	//如果是常驻类型的程序，需要监控其状态，异常时再次拉起
	
	struct MetaRegularProcessTask
	{
		typedef Bmco::SharedPtr<MetaRegularProcessTask> Ptr;

		//constructor
		MetaRegularProcessTask(Bmco::UInt32 id,Bmco::UInt32 programID,
							Bmco::UInt32 instanceID,Bmco::UInt32 flowID,
							int isValid,Bmco::Timestamp timeStamp,bool isCapValid,Bmco::UInt32 Priority,
							const char * extendCommand,int isNoMonitor = 0,
							Bmco::Int32 pausestatus=0,Bmco::Int32 iswarning=1,std::string orgid="-1",
							std::string dynamicparam=""):
							ID(id),ProgramID(programID),/*BpcbID(bpcbID),*/
							InstanceID(instanceID),FlowID(flowID),/*FlowStep(flowStep),*/
							IsValid(isValid),TimeStamp(timeStamp),IsCapValid(isCapValid),Priority(Priority),
							ExCommand(extendCommand),IsNoMonitor(isNoMonitor),pause_status(pausestatus),
							is_warning(iswarning),OrgID(orgid),dynamic_param(dynamicparam)
							{
							}

		
		MetaRegularProcessTask(const MetaRegularProcessTask& r)
		{
			assign(r);
		}

		
		MetaRegularProcessTask& operator = (const MetaRegularProcessTask & r)
		{
			assign(r);
			return *this;
		}

		
		bool operator == (const MetaRegularProcessTask & r) const
		{
			return ((ProgramID == r.ProgramID)&&(InstanceID == r.InstanceID)
				&&(FlowID == r.FlowID)/*&&(FlowStep == r.FlowStep)*/);
		}

		
		void assign(const MetaRegularProcessTask& r)
		{
			if((void*)this!=(void*)&r){
				ID = r.ID;
				ProgramID = r.ProgramID;
				/*BpcbID = r.BpcbID;*/
				InstanceID = r.InstanceID;
				FlowID = r.FlowID;
				/*FlowStep = r.FlowStep;*/
				IsValid = r.IsValid;
				TimeStamp = r.TimeStamp;
				IsCapValid = r.IsCapValid;
				Priority = r.Priority;
				ExCommand = r.ExCommand.c_str();
				IsNoMonitor = r.IsNoMonitor;
				pause_status = r.pause_status;
				is_warning = r.is_warning;
				OrgID=r.OrgID;
				dynamic_param=r.dynamic_param;
			}
		}

		
		bool operator < (const MetaRegularProcessTask& r) const
		{
			std::string str1 = Bmco::format("%?d_%?d_%?d_%?d",this->Priority,this->FlowID,this->ProgramID,this->TimeStamp);
			std::string str2 = Bmco::format("%?d_%?d_%?d_%?d",r.Priority,r.FlowID,r.ProgramID,r.TimeStamp);

			return str1 > str2;
			//if (this->Priority > r.Priority)
			//{
			//	return true;

			//}else if(this->Priority == r.Priority)
			//{
			//	return (this->TimeStamp < r.TimeStamp);
			//}else
			//{
			//	return false;
			//}
		}

		//field definition
		Bmco::UInt32 ID; //唯一且自增的常规任务ID              
		Bmco::UInt32 ProgramID;     //需要执行的程序ID     
		//屏蔽BpcbID这个字段，为了添加hla任务方便，不用考虑该字段，bol进程的bpcbid从程序定义表的程序类型后四位读取
		//Bmco::UInt64 BpcbID;//所有BOL常驻进程都有预分配的bpcb id，编码<1000
		//					//所有HLA常驻进程，存在多实例的情况，不能预分配bpcb id，填0
		//					//(1号进程调度该任务时，通过全局sequence来动态获取自增ID作为bpcb id)
		Bmco::UInt32 InstanceID; //单一可执行文件，启动多个进程后，由instanceID标识其属于多实例中的第几个
								//不存在高低水调度的程序，instanceID 填0
		Bmco::UInt32 FlowID;	//不属于任何流程，则填0，属于，则填对应的流程ID
		//Bmco::UInt32 FlowStep;	//流程: Prep->Format->Price中，prep的step填1，Format填2，price填3'
		int IsValid;			//0-无效正常杀进程  1-有效启动进程 2-强杀进程
		Bmco::Timestamp TimeStamp;//创建时间
		bool IsCapValid;		//是否生效，仅为进程管理能力封装使用
		Bmco::UInt32 Priority;   ///进程优先级（资源分配）
		std::string ExCommand; //任务启动时扩展的启动参数
		int IsNoMonitor;  //0  监控拉起   1 改状态不拉起  2  不改状态不拉起

		Bmco::Int32 pause_status;//暂停字段

		
		Bmco::Int32 is_warning;

		std::string OrgID;
		std::string dynamic_param;

	private:
		MetaRegularProcessTask();
	};

	
	

}


#endif //_METAREGULARPROCESSTASK_HH__

