#ifndef __MEM_TABLE_OP_H
#define __MEM_TABLE_OP_H

#include "DataAPI.h"
#include "Bmco/Mutex.h"
#include "MetaBpcbInfo.h"

namespace BM35 {

extern const std::string strMetaBolConfigInfo;
                         
extern const std::string strMetaBolCrontab;
extern const std::string strdynamic_para;
extern const std::string strID;
extern const std::string strIsValid;
extern const std::string strNextExecuteTime;
extern const std::string strTasktype;
extern const std::string strProgramID;
extern const std::string strMinute;
extern const std::string strHour;
extern const std::string strDay;
extern const std::string strMonth;
extern const std::string strYear;
extern const std::string strWeek;
extern const std::string strTimeStamp;
                         
extern const std::string strMetaBolInfo;
extern const std::string strstatus;
                         
extern const std::string strMetaBpcbInfo;
extern const std::string strBpcbID;
extern const std::string strHeartBeat;
extern const std::string strCPU;
extern const std::string strRAM;
extern const std::string strStatus;
extern const std::string strSysPID;
extern const std::string strSnapShot;
extern const std::string strAction;
extern const std::string strTaskSource;
extern const std::string strFlowID;
extern const std::string strFDLimitation;
extern const std::string strFDInUsing;
extern const std::string strSourceID;
extern const std::string strresult_code;
extern const std::string strresult_desc;
extern const std::string strresult_time;
extern const std::string strbpcb_type;
extern const std::string strNext_Time;
extern const std::string strcreateTime;
extern const std::string strThisTimeStartTime;
extern const std::string strLastTimeStartTime;



extern const std::string strMetaCfgDict;
extern const std::string strDict_Type;
extern const std::string strDict_Key;
extern const std::string strValue;
extern const std::string strValue2;


                         
extern const std::string strMetaChunkInfo;
extern const std::string strid;
extern const std::string strregionId;
extern const std::string strbytes;
extern const std::string strname;
extern const std::string strisGrow;
extern const std::string strusedBytes;
extern const std::string strisMappedFile;
extern const std::string strisShrink;
extern const std::string strisSnapShot;
extern const std::string strlockName;
extern const std::string strowner;
extern const std::string strstate;
extern const std::string strtime;
	
extern const std::string strMetaRegionInfo;
extern const std::string strid;
extern const std::string strmaxBytes;
extern const std::string strusedBytes;

	
extern const std::string strMetaMemTaskList;
extern const std::string strMetaMQDef;
extern const std::string strMetaMQRela;
                         
extern const std::string strMetaProgramDef;
extern const std::string strID;
extern const std::string strBolBpcbID;
extern const std::string strExeName;
extern const std::string strStaticParams;
extern const std::string strDynamicParams;
extern const std::string strDisplayName;
extern const std::string strHeartBeatType;
extern const std::string strMaxInstance;
extern const std::string strMinInstance;
extern const std::string strProcessType;
extern const std::string strProcessLifecycleType;
extern const std::string strProcessBusinessType;
extern const std::string strPriority;
extern const std::string strexe_path;
extern const std::string strlib_path;
extern const std::string strroot_dir;
extern const std::string strRam_limit;
extern const std::string strCpu_limit;
                         
extern const std::string strMetaUniqueSequenceID;
extern const std::string strsequenceValue;
extern const std::string strsequenceName;
extern const std::string strlockName;
extern const std::string strminValue;
extern const std::string strmaxValue;
extern const std::string strupStep;
extern const std::string strsequenceType;
extern const std::string strupdateTime;

extern const std::string strMetaRegularProcessTask;
extern const std::string strID;
extern const std::string strProgramID;
extern const std::string strInstanceID;
extern const std::string strIsValid;
extern const std::string strIsCapValid;
extern const std::string strExCommand;
extern const std::string strIsNoMonitor;
extern const std::string strpausestatus;
extern const std::string striswarning;
extern const std::string strOrgID;


class MemTableOper {
public:
    ~MemTableOper();

    static MemTableOper& instance();

    bool heartbeat(int bpcbid);

    bool UpdateBpcbCreateTime(int bpcbid);

    bool UpdateBpcbItem(BpcbProcessState _s,  Bmco::Int32 _pid);

    bool SetBolStatus(int bolstatus);

	bool GetBolStatus(int &bolstatus);

	bool GetSnapShotFlag(BolProcess bpcbid,Bmco::Int32 & snapflag);

	bool UpdateSnapShotFlag(BolProcess bpcbid,Bmco::Int32 & snapflag);

	bool GetProgramDefbyId(Bmco::Int32 proid,Matrix& matx);

	bool QueryTableAllData(const std::string &tablename,Matrix &matx);

	bool UpdateBpcbNextExeTime(Bmco::Int64 cronid,std::string& nextime);

	bool UpdateBpcbCpuUse(Bmco::Int64 bpcbid,Bmco::Int32 cpuuse);

	bool UpdateCronTimeStamp(Bmco::Int64 cronid);

	bool UpdateCronNextExeTime(Bmco::Int64 cronid,const std::string& nextime);

	bool UpdateBpcbThisExeTime(Bmco::Int64 bpcbid,std::string& thistime);


	std::string GetDictDynamicParam(const std::string &strdynamic_param);

	bool UpdateBpcbMemUse(Bmco::Int64 bpcbid,Bmco::Int64 memuse);

	bool UpdateBpcbLastStartTime(Bmco::Int64 bpcbid,Bmco::Int64 lasttime);


	Bmco::Int32 GetOmcFileTime(const std::string &strauditid);

	bool GetisOmcFileValid(const std::string &strauditid);

	bool UpdateBpcbAction(int bpcbid,int action);
	

	bool GetBpcbInfobySource(Bmco::Int32 taskSource,Bmco::Int32 sourceID, 
	bool & isHaving,Matrix& matx);

	bool IsExistSequence(const std::string &seqname);

	bool GetNextSequenceId(const std::string &seqname,Bmco::Int64 & id);
	
	bool GetBpcbInfobyBpcbID(Bmco::Int64 bpcbid,Matrix& matx);

	bool UpdateBpcbThisStartTime(Bmco::Int64 bpcbid);

	bool UpdateChunkUseBytesByName(const std::string& chunkname,Bmco::Int64 usebytes);

	bool UpdateReginUseBytesById(Bmco::Int32 regionid,Bmco::Int64 usebytes);

	bool GetReginonUsedBytesbyId(Bmco::Int32 regionid,Bmco::Int64 &usebytes);

	bool GetReginonInfobyId(Bmco::Int32 regionid,Matrix& matx);

	bool DeleteBpcbInfoBybpcbd(Bmco::Int64 bpcbid);

	bool GetProgramDefBytypeAndBpcbID(Bmco::Int32 protype,Bmco::Int32 bpcbid,Matrix& matx);

	bool UpdateBpcbProgramID(Bmco::Int64 bpcbid,Bmco::Int32 programid);

	bool UpdateBpcbStatusByID(Bmco::Int64 bpcbid,Bmco::Int32 status);

	bool UpdateBpcbSysPid(Bmco::Int64 bpcbid,Bmco::Int32 syspid);

private:
    MemTableOper();

    MemTableOper(const MemTableOper&);

    MemTableOper& operator = (const MemTableOper&);

private:
    

    Bmco::FastMutex _mtx;
}; 

} // namespace BM35

#endif 
