#ifndef __META_BPCB_INFO_HH__
#define __META_BPCB_INFO_HH__


#include "Bmco/Timestamp.h"
#include "Bmco/DateTimeFormatter.h"
#include "Bmco/DateTimeFormat.h"
#include "Bmco/SharedPtr.h"

namespace BM35 {

enum BolStatus{
    BOL_NORMAL = 0,
    BOL_MAINTAIN = 1,
    BOL_OFFLINE = 2
};

//������Դ
enum BpcbTaskSource
{
	DEFALUT_PROCESS_TASK = 0,	//0#��1#��������
	AUTOGENNERATE_PROCESS_TASK=1, //1#�ڲ����ɵ�����2�ţ�5�Ž��̵�����
	REGULAR_PROCESS_TASK = 2,	//��ʼ�����Ƚ�������
	BOL_CRONTAB_PROCESS_TASK	//�����Զ�ʱ��������
};

//����״̬
enum BpcbProcessState{
	ST_INIT = 0,  //��ʼ̬
	ST_READY = 1,  //����̬
	ST_RUNNING =2, //����̬
	ST_HOLDING = 3,//����̬
	ST_MAITENANCE = 4,//ά��̬
	ST_ABORT = 5, //�쳣
	ST_STOP = 6,
	ST_CORE = 7,//���̲���
	ST_DEAD = 8,//���̽���
	ST_STOPING=9,//����ֹͣ��
	ST_PAUSE=10, //��ͣ����
	ST_RESTORE=11//�ָ�����
};

//SnapShot��ʶλ
enum BpcbSnapShot
{
	BPCB_SNAPSHOT_INT = 0,//��ʼ״̬
	BPCB_SNAPSHOT_COMMAND = 1,//�������״̬
	BPCB_SNAPSHOT_OTHER = 2	//����������Ӧ״̬
};

//bol����enum��Ԥ�����bpcbID��
enum BolProcess
{
	BOL_PROCESS_KERNEL = 0, //0#
	BOL_PROCESS_PROCESSMGR = 1, //1#
	BOL_PROCESS_MEMMANAGER = 2,	//2#
	BOL_PROCESS_CONNMANAGER = 3, //3#
	BOL_PROCESS_STORAGEMANAGER = 4, //4#
	BOL_PROCESS_CLOUDAGENT = 5 ,//5#
	BOL_PROCESS_BRMC = 6, //#6

	BOL_PROCESS_SECUREMANAGER = 91, //91#
	BOL_PROCESS_LOGSERVER = 90,

	BOL_PROCESS_BONMGR = 11, //11#
	BOL_PROCESS_DCCAGENT = 12, //12#
	BOL_PROCESS_HLAMASTER = 97, //97#
	BOL_PROCESS_OMCAGENT = 98 , //98#
	BOL_PROCESS_MDRGUARD = 99,  //99#
	BOL_PROCESS_OMCSERVER = 996, //omc server
	BOL_PROCESS_FILESERVER = 997, //file server
	BOL_PROCESS_KPIPROCESS = 998  //998#
};

//�ⲿ��֪ĳ���̵Ķ���
enum ProcessActionOption
{
	PROCESS_ACTION_NULL = 0,   //��״̬
	PROCESS_ACTION_PAUSE = 1, //��ͣ����
	PROCESS_ACTION_ACTIVATE = 2, //�������
	PROCESS_ACTION_PARAM_REFRESH //���Ĳ���ˢ��
};

//invalid system pid
static const Bmco::UInt32 INVALID_SYS_PID = 0;

///�������ͣ��������ȱ�(1#���ȱ�)�������ǳ�������ʱ��״̬��Ǩ��ֱ�Ӹ���bpcbinfo��
//the info of bpcb  stored in control region
struct MetaBpcbInfo 
{
	/// smart ptr definition
	typedef Bmco::SharedPtr<MetaBpcbInfo> Ptr;

	

    MetaBpcbInfo(Bmco::UInt64 ID){
        m_iBpcbID=ID;
        m_iSysPID=m_iProgramID=m_iStatus=m_iFDLimitation=m_iFDInUsing=m_iCPU=0;
        m_iRAM=m_iFlowID=m_iInstanceID=m_iTaskSource=m_iSourceID=0;
        m_iSnapShot = BPCB_SNAPSHOT_INT;
        m_iAction = PROCESS_ACTION_NULL;
		m_tThisTimeStartTime = m_tLastTimeStartTime = 0;
		m_result_code="";
		m_result_desc="";
		m_result_time="";
		m_Next_Time="";
    }
	
	MetaBpcbInfo(Bmco::Int64 iBpcbID,
		Bmco::Int32 SysPID,
		Bmco::Int32 ProgramID,
		Bmco::Timestamp CreateTime,
		Bmco::Timestamp HeartBeat,
		Bmco::Int32 Status,
		Bmco::Int32 FDLimitation,
		Bmco::Int32 FDInUsing,
		Bmco::Int32 CPU,
		Bmco::Int64 RAM,
		Bmco::Int32 FlowID,
		Bmco::Int32 InstanceID,
		Bmco::Int32 TaskSource,
		Bmco::Int32 SourceID,
		Bmco::Int32 bpcbtype=1, //1--��������2--��ʱ����,Ĭ��γ�������
		Bmco::Int32 SnapShot = BPCB_SNAPSHOT_INT,
		Bmco::Int32 Action = PROCESS_ACTION_NULL)
		:m_iBpcbID(iBpcbID),
	     m_iSysPID(SysPID),
		 m_iProgramID(ProgramID),
		 m_tCreateTime(CreateTime),
		 m_tHeartBeat(HeartBeat),
		 m_iStatus(Status),
		 m_iFDLimitation(FDLimitation),
		 m_iFDInUsing(FDInUsing),
		 m_iCPU(CPU),
		 m_iRAM(RAM),
		 m_iFlowID(FlowID),
		 m_iInstanceID(InstanceID),
		 m_iTaskSource(TaskSource),
		 m_iSourceID(SourceID),
		 /*m_iFlowStep(FlowStep),*/
		 m_iSnapShot(SnapShot),
		 m_iAction(Action),
		 m_bpcb_type(bpcbtype)
	{
			m_tThisTimeStartTime = m_tLastTimeStartTime = 0;
			m_result_code="";
			m_result_desc="";
			m_result_time="";
			m_Next_Time="";
			
	}

	MetaBpcbInfo(const MetaBpcbInfo& r)
	{
		assign(r);
	}

	MetaBpcbInfo &operator=(const MetaBpcbInfo& r)
	{
		assign(r);
		return *this;
	}

    void assign(const MetaBpcbInfo& r)
    {
		if ((void*)this != (void*)&r)
		{
			m_iBpcbID = r.m_iBpcbID;
			m_iSysPID = r.m_iSysPID;
			m_iProgramID = r.m_iProgramID;
			m_tCreateTime = r.m_tCreateTime;
			m_tHeartBeat = r.m_tHeartBeat;
			m_iStatus = r.m_iStatus;
			m_iFDLimitation = r.m_iFDLimitation;
			m_iFDInUsing = r.m_iFDInUsing;
			m_iCPU = r.m_iCPU;
			m_iRAM = r.m_iRAM;
			m_iFlowID = r.m_iFlowID;
			m_iInstanceID = r.m_iInstanceID;
			m_iTaskSource = r.m_iTaskSource;
			m_iSourceID = r.m_iSourceID;
			m_result_code=r.m_result_code;
			m_result_desc=r.m_result_desc;
			m_result_time=r.m_result_time;
			m_Next_Time=r.m_Next_Time;
			m_bpcb_type=r.m_bpcb_type;
			
			m_iSnapShot = r.m_iSnapShot;
			m_iAction = r.m_iAction;
			m_tThisTimeStartTime = r.m_tThisTimeStartTime;
			m_tLastTimeStartTime = r.m_tLastTimeStartTime;
		}
	}

	Bmco::Int64 m_iBpcbID; //bpcbID
	Bmco::Int32 m_iSysPID;   //ϵͳ����id
	Bmco::Int32 m_iProgramID;  //����ID
	Bmco::Timestamp m_tCreateTime; //��������ʱ��
	Bmco::Timestamp m_tHeartBeat;  //����ʱ��
	Bmco::Int32 m_iStatus;   //����״̬
	Bmco::Int32 m_iFDLimitation;   //���ļ�������
	Bmco::Int32 m_iFDInUsing;	//�Ѵ��ļ���
	Bmco::Int32 m_iCPU;   //cpu ��ʹ����
	Bmco::Int64 m_iRAM;   //RAMʹ����(�ֽ�)
	Bmco::Int32 m_iFlowID;    //��������ID
	Bmco::Int32 m_iInstanceID; //ʵ��ID
	Bmco::Int32 m_iTaskSource; //������Դ ������enum BpcbTaskSource
	Bmco::Int32 m_iSourceID;  //��ԴID
	Bmco::Int32 m_iSnapShot; //SnapShot��־λ
	Bmco::Int32 m_iAction;   //������ʶλ����ͣ������ȣ�
	Bmco::Timestamp m_tThisTimeStartTime; //��������ʱ��
	Bmco::Timestamp m_tLastTimeStartTime; //�ϴ�����ʱ��

	std::string m_result_code;
	std::string m_result_desc;
	std::string m_result_time;
	std::string m_Next_Time;
	Bmco::Int32 m_bpcb_type;
	

	
	
};



}

#endif
