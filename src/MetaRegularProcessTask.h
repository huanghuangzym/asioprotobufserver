#ifndef _METAREGULARPROCESSTASK_HH__
#define _METAREGULARPROCESSTASK_HH__

#include "Bmco/Timestamp.h"
namespace BM35
{



enum RegularProcessTaskControl
{
	CONTROL_TERMINATE = 0,  //����ֹͣ����
	CONTROL_START = 1,  //��������
	CONTROL_KILL =2 //ǿɱ����
	
};


enum RegularProcessPauseControl
{
	CONTROL_RESTORE = 0,  //�ָ�����
	CONTROL_PAUSE= 1 //��ͣ����
};



	//��ʼ�����Ƚ�������
	//���Ժ��Ĳ��������ļ�������BOL������Ӧ���ɽ��̹���������HLA��Ӧ�ó���
	//����ǳ�פ���͵ĳ�����Ҫ�����״̬���쳣ʱ�ٴ�����
	
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
		Bmco::UInt32 ID; //Ψһ�������ĳ�������ID              
		Bmco::UInt32 ProgramID;     //��Ҫִ�еĳ���ID     
		//����BpcbID����ֶΣ�Ϊ�����hla���񷽱㣬���ÿ��Ǹ��ֶΣ�bol���̵�bpcbid�ӳ������ĳ������ͺ���λ��ȡ
		//Bmco::UInt64 BpcbID;//����BOL��פ���̶���Ԥ�����bpcb id������<1000
		//					//����HLA��פ���̣����ڶ�ʵ�������������Ԥ����bpcb id����0
		//					//(1�Ž��̵��ȸ�����ʱ��ͨ��ȫ��sequence����̬��ȡ����ID��Ϊbpcb id)
		Bmco::UInt32 InstanceID; //��һ��ִ���ļ�������������̺���instanceID��ʶ�����ڶ�ʵ���еĵڼ���
								//�����ڸߵ�ˮ���ȵĳ���instanceID ��0
		Bmco::UInt32 FlowID;	//�������κ����̣�����0�����ڣ������Ӧ������ID
		//Bmco::UInt32 FlowStep;	//����: Prep->Format->Price�У�prep��step��1��Format��2��price��3'
		int IsValid;			//0-��Ч����ɱ����  1-��Ч�������� 2-ǿɱ����
		Bmco::Timestamp TimeStamp;//����ʱ��
		bool IsCapValid;		//�Ƿ���Ч����Ϊ���̹���������װʹ��
		Bmco::UInt32 Priority;   ///�������ȼ�����Դ���䣩
		std::string ExCommand; //��������ʱ��չ����������
		int IsNoMonitor;  //0  �������   1 ��״̬������  2  ����״̬������

		Bmco::Int32 pause_status;//��ͣ�ֶ�

		
		Bmco::Int32 is_warning;

		std::string OrgID;
		std::string dynamic_param;

	private:
		MetaRegularProcessTask();
	};

	
	

}


#endif //_METAREGULARPROCESSTASK_HH__

