#include "MemTableOp.h"
#include "MetaBpcbInfo.h"

namespace BM35 {


const std::string strMetaBolConfigInfo="MetaBolConfigInfo";
                  
const std::string strMetaBolCrontab="MetaBolCrontab";
const std::string strNextExecuteTime="NextExecuteTime";
const std::string strTasktype="Task_type";
const std::string strdynamic_para="dynamic_para";
const std::string strMinute="Minute";
const std::string strHour="Hour";
const std::string strDay="Day";
const std::string strMonth="Month";
const std::string strYear="Year";
const std::string strWeek="Week";
const std::string strTimeStamp="TimeStamp";
                  
const std::string strMetaBolInfo="MetaBolInfo";
const std::string strstatus="status";
                  
const std::string strMetaBpcbInfo="MetaBpcbInfo";
const std::string strBpcbID="BpcbID";
const std::string strHeartBeat="HeartBeat";
const std::string strCPU="CPU";
const std::string strRAM="RAM";
const std::string strSysPID="SysPID";
const std::string strStatus="Status";
const std::string strFDLimitation="FDLimitation";
const std::string strSnapShot="SnapShot";
const std::string strAction="Action";
const std::string strFDInUsing="FDInUsing";
const std::string strInstanceID="InstanceID";
const std::string strFlowID="FlowID";
const std::string strTaskSource="TaskSource";
const std::string strSourceID="SourceID";
const std::string strresult_code="result_code";
const std::string strresult_desc="result_desc";
const std::string strresult_time="result_time";
const std::string strbpcb_type="bpcb_type";
const std::string strNext_Time="Next_Time";
const std::string strcreateTime="createTime";
const std::string strThisTimeStartTime="ThisTimeStartTime";
const std::string strLastTimeStartTime="LastTimeStartTime";


const std::string strMetaCfgDict="MetaCfgDict";
const std::string strDict_Type="Dict_Type";
const std::string strDict_Key="Dict_Key";
const std::string strValue="Value";
const std::string strValue2="Value2";



                  
const std::string strMetaChunkInfo="MetaChunkInfo";
const std::string strregionId="regionId";
const std::string strbytes="bytes";
const std::string strname="name";
const std::string strisGrow="isGrow";
const std::string strusedBytes="usedBytes";
const std::string strisMappedFile="isMappedFile";
const std::string strisShrink="isShrink";
const std::string strisSnapShot="isSnapShot";
const std::string strlockName="lockName";
const std::string strowner="owner";
const std::string strstate="state";
const std::string strtime="time";

const std::string strMetaRegionInfo="MetaRegionInfo";
const std::string strid="id";
const std::string strmaxBytes="maxBytes";


	
	
const std::string strMetaMemTaskList="MetaMemTaskList";
const std::string strMetaMQDef="MetaMQDef";
const std::string strMetaMQRela="MetaMQRela";
                 
const std::string strMetaProgramDef="MetaProgramDef";
const std::string strBolBpcbID="BolBpcbID";
const std::string strExeName="ExeName";
const std::string strStaticParams="StaticParams";
const std::string strDynamicParams="DynamicParams";
const std::string strDisplayName="DisplayName";
const std::string strHeartBeatType="Heatbeat_type";
const std::string strMaxInstance="MaxInstance";
const std::string strMinInstance="MinInstance";
const std::string strProcessType="ProcessType";
const std::string strProcessLifecycleType="ProcessLifecycleType";
const std::string strProcessBusinessType="ProcessBusinessType";
const std::string strPriority="Priority";
const std::string strexe_path="Exe_Path";
const std::string strlib_path="lib_path";
const std::string strroot_dir="root_dir";
const std::string strRam_limit="Ram_limit";
const std::string strCpu_limit="Cpu_limit";

                  
const std::string strMetaUniqueSequenceID="MetaUniqueSequenceID";
const std::string strsequenceValue="sequenceValue";
const std::string strsequenceName="sequenceName";
const std::string strminValue="minValue";
const std::string strmaxValue="maxValue";
const std::string strupStep="upStep";
const std::string strsequenceType="sequenceType";
const std::string strupdateTime="updateTime";
	
	
const std::string strMetaRegularProcessTask="MetaRegularProcessTask";
const std::string strID="ID";
const std::string strProgramID="ProgramID";
const std::string strIsValid="IsValid";
const std::string strIsCapValid="IsCapValid";
const std::string strExCommand="ExCommand";
const std::string strIsNoMonitor="IsNoMonitor";
const std::string strpausestatus="pause_status";
const std::string striswarning="Is_warning";
const std::string strOrgID="OrgID";

MemTableOper::MemTableOper() 
{
}

MemTableOper::~MemTableOper() 
{
}

MemTableOper& MemTableOper::instance() 
{
    static MemTableOper *pMemTableOper = NULL;
    if (NULL == pMemTableOper) 
    {
        static MemTableOper memop;
        pMemTableOper = &memop;
    }

    return *pMemTableOper;
}


bool MemTableOper::QueryTableAllData(const std::string &tablename,Matrix &matx) 
{  
	
 	int ret=DataAPI::query(tablename,NULL,matx);
	if(ret != 0)
	{
		return false;
	}
	
	return true;
}



bool MemTableOper::heartbeat(int bpcbid) 
{  
	Pair selectedCondition;
	Pair updateValue;
		
	selectedCondition.addFieldPair(strBpcbID, bpcbid);

	Bmco::LocalDateTime now;
	updateValue.addFieldPair(strHeartBeat, now.timestamp().epochMicroseconds());

 	int ret=DataAPI::update(strMetaBpcbInfo,&selectedCondition,updateValue);
	return ret > 0;
}


bool MemTableOper::UpdateBpcbAction(int bpcbid,int action) 
{  
	Pair selectedCondition;
	Pair updateValue;
		
	selectedCondition.addFieldPair(strBpcbID, bpcbid);

	
	updateValue.addFieldPair(strAction, action);

 	int ret=DataAPI::update(strMetaBpcbInfo,&selectedCondition,updateValue);
	return ret > 0;
}




bool MemTableOper::UpdateBpcbCreateTime(int bpcbid) 
{  
	Pair selectedCondition;
	Pair updateValue;
		
	selectedCondition.addFieldPair(strBpcbID, bpcbid);

	Bmco::LocalDateTime now;
	updateValue.addFieldPair(strcreateTime, now.timestamp().epochMicroseconds());

 	int ret=DataAPI::update(strMetaBpcbInfo,&selectedCondition,updateValue);
	return ret > 0;
}



bool MemTableOper::UpdateBpcbItem(BpcbProcessState _s,  Bmco::Int32 _pid) 
{  
	Pair selectedCondition;
	Pair updateValue;
		
	selectedCondition.addFieldPair(strBpcbID, BOL_PROCESS_KERNEL);
	updateValue.addFieldPair(strStatus, _s);
	updateValue.addFieldPair(strSysPID, _pid);
	
 	int ret=DataAPI::update(strMetaBpcbInfo,&selectedCondition,updateValue);
	return ret > 0;
}

bool MemTableOper::SetBolStatus(int bolstatus) 
{  
	Pair updateValue;
		
	updateValue.addFieldPair(strstatus, bolstatus);

 	int ret=DataAPI::update(strMetaBolInfo,NULL,updateValue);

	return ret > 0;
}


bool MemTableOper::GetBolStatus(int &bolstatus) 
{  
	
	Matrix matx;
 	int ret=DataAPI::query(strMetaBolInfo,NULL,matx);

	if(ret != 0)
	{
		return false;
	}
	const FieldValue& pfiledv=matx.getFieldValue(strstatus,0);
	


	return pfiledv.getInt32(bolstatus);
}


bool MemTableOper::IsExistSequence(const std::string &seqname) 
{  
	
	Pair selectedCondition;
	selectedCondition.addFieldPair(strsequenceName, seqname);
	Matrix matx;
 	int ret=DataAPI::query(strMetaUniqueSequenceID,&selectedCondition,matx);

	if(ret != 0)
	{
		return false;
	}

	return matx.rowValueCount() > 0;
}

bool MemTableOper::GetNextSequenceId(const std::string &seqname,Bmco::Int64 & id) 
{  
	
	Pair selectedCondition;
	selectedCondition.addFieldPair(strsequenceName, seqname);
	Matrix matx;
 	int ret=DataAPI::query(strMetaUniqueSequenceID,&selectedCondition,matx);

	if(ret != 0)
	{
		return false;
	}
	const FieldValue& pfiledv=matx.getFieldValue(strsequenceValue,0);



	Pair updateValue;
	updateValue.addFieldPair(strsequenceValue, pfiledv.getInt64()+1);
	
 	DataAPI::update(strMetaUniqueSequenceID,&selectedCondition,updateValue);


	return pfiledv.getInt64(id);
}



bool MemTableOper::GetSnapShotFlag(BolProcess bpcbid,Bmco::Int32 & snapflag) 
{  
	Pair selectedCondition;
	selectedCondition.addFieldPair(strBpcbID, bpcbid);
	Matrix matx;
 	int ret=DataAPI::query(strMetaBpcbInfo,&selectedCondition,matx);

	if(ret != 0)
	{
		return false;
	}
	const FieldValue& pfiledv=matx.getFieldValue(strSnapShot,0);


	return pfiledv.getInt32(snapflag);
}



Bmco::Int32 MemTableOper::GetOmcFileTime(const std::string &strauditid) 
{  
	Pair selectedCondition;
	selectedCondition.addFieldPair(strDict_Type, "OMC_TASK");
	selectedCondition.addFieldPair(strDict_Key, strauditid);
	
	
	Matrix matx;
 	int ret=DataAPI::query(strMetaCfgDict,&selectedCondition,matx);

	if(ret != 0)
	{
		return -1;
	}

	if(matx.rowValueCount() ==0 )
	{
		return -1;
	}

	const FieldValue& pfiledv=matx.getFieldValue(strValue,0);
	return pfiledv.getInt32();
	
}



std::string MemTableOper::GetDictDynamicParam(const std::string &strdynamic_param) 
{  
	Pair selectedCondition;
	selectedCondition.addFieldPair(strDict_Type, "DICT_MACRO");

	//"DICT_MACRO_" Ö®ºóµÄ
	selectedCondition.addFieldPair(strDict_Key, strdynamic_param);
	
	
	Matrix matx;
 	int ret=DataAPI::query(strMetaCfgDict,&selectedCondition,matx);

	if(ret != 0)
	{
		return "";
	}

	if(matx.rowValueCount() ==0 )
	{
		return "";
	}

	const FieldValue& pfiledv=matx.getFieldValue(strValue,0);
	return pfiledv.getString();
	
}




bool MemTableOper::GetisOmcFileValid(const std::string &strauditid) 
{  
	Pair selectedCondition;
	selectedCondition.addFieldPair(strDict_Type, "OMC_TASK");
	selectedCondition.addFieldPair(strDict_Key, strauditid);
	
	
	Matrix matx;
 	int ret=DataAPI::query(strMetaCfgDict,&selectedCondition,matx);

	if(ret != 0)
	{
		return true;
	}

	if(matx.rowValueCount() ==0 )
	{
		return true;
	}

	const FieldValue& pfiledv=matx.getFieldValue(strValue2,0);
	return pfiledv.getString() == "on";
	
}



bool MemTableOper::GetProgramDefbyId(Bmco::Int32 proid,Matrix& matx) 
{  
	Pair selectedCondition;
	selectedCondition.addFieldPair(strID, proid);
	
 	int ret=DataAPI::query(strMetaProgramDef,&selectedCondition,matx);

	return ret == 0;
}

bool MemTableOper::GetBpcbInfobySource(Bmco::Int32 taskSource,Bmco::Int32 sourceID, 
	bool & isHaving,Matrix& matx) 
{  
	Pair selectedCondition;
	selectedCondition.addFieldPair(strTaskSource, taskSource);
	selectedCondition.addFieldPair(strSourceID, sourceID);
	
 	int ret=DataAPI::query(strMetaBpcbInfo,&selectedCondition,matx);

	isHaving=(matx.rowValueCount() != 0);

	return ret == 0;
}


bool MemTableOper::GetBpcbInfobyBpcbID(Bmco::Int64 bpcbid,Matrix& matx) 
{  
	Pair selectedCondition;
	selectedCondition.addFieldPair(strBpcbID, bpcbid);
	
 	int ret=DataAPI::query(strMetaBpcbInfo,&selectedCondition,matx);

	return matx.rowValueCount() > 0;
}



bool MemTableOper::UpdateSnapShotFlag(BolProcess bpcbid,Bmco::Int32 & snapflag) 
{  
	Pair selectedCondition;
	Pair updateValue;
		
	selectedCondition.addFieldPair(strBpcbID, bpcbid);
	updateValue.addFieldPair(strSnapShot, snapflag);

	
 	int ret=DataAPI::update(strMetaBpcbInfo,&selectedCondition,updateValue);
	return ret > 0;
}

bool MemTableOper::UpdateBpcbThisStartTime(Bmco::Int64 bpcbid) 
{  
	Pair selectedCondition;
	Pair updateValue;
		
	selectedCondition.addFieldPair(strBpcbID, bpcbid);

	Bmco::LocalDateTime now;
	updateValue.addFieldPair(strThisTimeStartTime, now.timestamp().epochMicroseconds());

	
 	int ret=DataAPI::update(strMetaBpcbInfo,&selectedCondition,updateValue);
	return ret > 0;
}

bool MemTableOper::UpdateBpcbLastStartTime(Bmco::Int64 bpcbid,Bmco::Int64 lasttime) 
{  
	Pair selectedCondition;
	Pair updateValue;
		
	selectedCondition.addFieldPair(strBpcbID, bpcbid);
	updateValue.addFieldPair(strLastTimeStartTime, lasttime);

	
 	int ret=DataAPI::update(strMetaBpcbInfo,&selectedCondition,updateValue);
	return ret > 0;
}

bool MemTableOper::UpdateBpcbCpuUse(Bmco::Int64 bpcbid,Bmco::Int32 cpuuse) 
{  
	Pair selectedCondition;
	Pair updateValue;
		
	selectedCondition.addFieldPair(strBpcbID, bpcbid);
	updateValue.addFieldPair(strCPU, cpuuse);

	
 	int ret=DataAPI::update(strMetaBpcbInfo,&selectedCondition,updateValue);
	return ret > 0;
}

bool MemTableOper::UpdateBpcbProgramID(Bmco::Int64 bpcbid,Bmco::Int32 programid) 
{  
	Pair selectedCondition;
	Pair updateValue;
		
	selectedCondition.addFieldPair(strBpcbID, bpcbid);
	updateValue.addFieldPair(strProgramID, programid);

	
 	int ret=DataAPI::update(strMetaBpcbInfo,&selectedCondition,updateValue);
	return ret > 0;
}




bool MemTableOper::UpdateBpcbStatusByID(Bmco::Int64 bpcbid,Bmco::Int32 status) 
{  
	Pair selectedCondition;
	Pair updateValue;
		
	selectedCondition.addFieldPair(strBpcbID, bpcbid);
	updateValue.addFieldPair(strStatus, status);

	
 	int ret=DataAPI::update(strMetaBpcbInfo,&selectedCondition,updateValue);
	return ret > 0;
}


bool MemTableOper::UpdateBpcbMemUse(Bmco::Int64 bpcbid,Bmco::Int64 memuse) 
{  
	Pair selectedCondition;
	Pair updateValue;
		
	selectedCondition.addFieldPair(strBpcbID, bpcbid);
	updateValue.addFieldPair(strRAM, memuse);

	
 	int ret=DataAPI::update(strMetaBpcbInfo,&selectedCondition,updateValue);
	return ret > 0;
}



bool MemTableOper::UpdateBpcbSysPid(Bmco::Int64 bpcbid,Bmco::Int32 syspid) 
{  
	Pair selectedCondition;
	Pair updateValue;
		
	selectedCondition.addFieldPair(strBpcbID, bpcbid);
	updateValue.addFieldPair(strSysPID, syspid);

	
 	int ret=DataAPI::update(strMetaBpcbInfo,&selectedCondition,updateValue);
	return ret > 0;
}


bool MemTableOper::DeleteBpcbInfoBybpcbd(Bmco::Int64 bpcbid) 
{  
	Pair selectedCondition;
		
	selectedCondition.addFieldPair(strBpcbID, bpcbid);

 	int ret=DataAPI::erase(strMetaBpcbInfo,&selectedCondition);
	return ret > 0;
}


bool MemTableOper::UpdateCronNextExeTime(Bmco::Int64 cronid,const std::string& nextime) 
{  
	Pair selectedCondition;
	Pair updateValue;
		
	selectedCondition.addFieldPair(strID, cronid);
	updateValue.addFieldPair(strNextExecuteTime, nextime);

	
 	int ret=DataAPI::update(strMetaBolCrontab,&selectedCondition,updateValue);
	return ret > 0;
}



bool MemTableOper::UpdateBpcbNextExeTime(Bmco::Int64 bpcbid,std::string& nextime) 
{  
	Pair selectedCondition;
	Pair updateValue;
		
	selectedCondition.addFieldPair(strBpcbID, bpcbid);
	updateValue.addFieldPair(strNext_Time, nextime);

	
 	int ret=DataAPI::update(strMetaBpcbInfo,&selectedCondition,updateValue);
	return ret > 0;
}


bool MemTableOper::UpdateBpcbThisExeTime(Bmco::Int64 bpcbid,std::string& thistime) 
{  
	Pair selectedCondition;
	Pair updateValue;
		
	selectedCondition.addFieldPair(strBpcbID, bpcbid);
	updateValue.addFieldPair(strThisTimeStartTime, thistime);

	
 	int ret=DataAPI::update(strMetaBpcbInfo,&selectedCondition,updateValue);
	return ret > 0;
}



bool MemTableOper::UpdateCronTimeStamp(Bmco::Int64 cronid) 
{  
	Pair selectedCondition;
	Pair updateValue;
		
	selectedCondition.addFieldPair(strID, cronid);

	Bmco::LocalDateTime now;
	updateValue.addFieldPair(strTimeStamp, now.timestamp().epochMicroseconds());

	
 	int ret=DataAPI::update(strMetaBolCrontab,&selectedCondition,updateValue);
	return ret > 0;
}


bool MemTableOper::GetReginonInfobyId(Bmco::Int32 regionid,Matrix& matx) 
{  
	Pair selectedCondition;
	selectedCondition.addFieldPair(strid, regionid);
	
 	int ret=DataAPI::query(strMetaRegionInfo,&selectedCondition,matx);

	return ret == 0;
}


bool MemTableOper::GetReginonUsedBytesbyId(Bmco::Int32 regionid,Bmco::Int64 &usebytes) 
{  

	Matrix matx;
	Pair selectedCondition;
	selectedCondition.addFieldPair(strid, regionid);
	
 	int ret=DataAPI::query(strMetaRegionInfo,&selectedCondition,matx);

	matx.getFieldValue(strusedBytes, 0).getInt64(usebytes);

	return ret == 0;
}



bool MemTableOper::UpdateReginUseBytesById(Bmco::Int32 regionid,Bmco::Int64 usebytes) 
{  
	Pair selectedCondition;
	Pair updateValue;
		
	selectedCondition.addFieldPair(strid, regionid);

	updateValue.addFieldPair(strusedBytes, usebytes);

	
 	int ret=DataAPI::update(strMetaRegionInfo,&selectedCondition,updateValue);
	return ret >0;
}

bool MemTableOper::UpdateChunkUseBytesByName(const std::string& chunkname,Bmco::Int64 usebytes) 
{  
	Pair selectedCondition;
	Pair updateValue;
		
	selectedCondition.addFieldPair(strname, chunkname);

	updateValue.addFieldPair(strusedBytes, usebytes);

	
 	int ret=DataAPI::update(strMetaChunkInfo,&selectedCondition,updateValue);
	return ret >0;
}



bool MemTableOper::GetProgramDefBytypeAndBpcbID(Bmco::Int32 protype,Bmco::Int32 bpcbid,Matrix& matx) 
{  

	Pair selectedCondition;
	selectedCondition.addFieldPair(strProcessType, protype);
	selectedCondition.addFieldPair(strBolBpcbID, bpcbid);
	
	
 	int ret=DataAPI::query(strMetaProgramDef,&selectedCondition,matx);


	return ret == 0;
}



} // namespace BM35
