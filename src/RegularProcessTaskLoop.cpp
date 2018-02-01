#include "RegularProcessTaskLoop.h"
#include "Bmco/DateTimeFormatter.h"
#include "Bmco/DateTimeFormat.h"
#include "Bmco/Timespan.h"
#include "Bmco/StringTokenizer.h"
#include "Bmco/String.h"
#include "Bmco/File.h"
#include "Bmco/NumberFormatter.h"
#include "Bmco/DateTimeParser.h"
#include "BolServiceRequestHandler.h"
#include <algorithm>
#include <boost/lexical_cast.hpp>
#include "boost/tokenizer.hpp"
#include "boost/algorithm/string.hpp"
#include <boost/typeof/typeof.hpp>
#include "OSInfoInterface.h"

#ifdef BOL_OMC_WRITER 
#include "OmcLog.h"
#endif

#if BMCO_OS == BMCO_OS_LINUX || BMCO_OS_FAMILY_UNIX == 1
 #include <sys/types.h>
 #include <signal.h>
#endif



using Bmco::DateTimeFormatter;
using Bmco::DateTimeFormat;

namespace BM35 {

const std::string LDENVNAME="LD_LIBRARY_PATH";

void RegularProcessTaskLoop::runTask() 
{

	try
	{
		if (!beforeTask()) 
		{
			Process::requestTermination(Process::id());
			return;
		}

		bool isneedterself=false;
		while (!isCancelled()) 
		{
			try
			{
				handleOneUnit();

				if (!isBolStatueNormal()) 
				{
					isneedterself=true;
					break;
				}
				
			}
			catch(...)
			{
				bmco_error_f2(theLogger, "%s|%s| one unit unknown exception."
					, std::string("0"),std::string(__FUNCTION__));
			}
			
		}

		afterTask();

		if(isneedterself)
		{
			Process::requestTermination(Process::id());
		}
		
	}
	catch(...)
	{
		bmco_error_f2(theLogger, "%s|%s| done regular task loop unknown exception."
			, std::string("0"),std::string(__FUNCTION__));
		Process::requestTermination(Process::id());
		return;
	}
	bmco_information_f2(theLogger, "%s|%s| done time task loop gracefully."
		, std::string("0"),std::string(__FUNCTION__));
}



void RegularProcessTaskLoop::DeleteOldBpcbInfo(Matrix &cronmtx)
{

	std::map<Bmco::Int32,MetaBpcbInfo::Ptr> bpcbmap;
	if(!getAllBpcbItem(bpcbmap))
	{
		bmco_warning_f2(theLogger,"%s|%s|failed to query all  bpcb item",
							std::string("0"), std::string(__FUNCTION__));
		return;
	}


	for(std::map<Bmco::Int32,MetaBpcbInfo::Ptr>::iterator it1=bpcbmap.begin();it1!=bpcbmap.end();it1++)
	{
		//对于定时任务类型的bpcb，如果任务已经不存在，则删除
		if((it1->second)->m_bpcb_type == 2)
		{
			//假设不存在
			bool is_exist=false;
			for(int i=0;i<cronmtx.rowValueCount();i++)
			{
				int cronid=0;
				cronmtx.getFieldValue(strID,i).getInt32(cronid);
				if((it1->second)->m_iSourceID == cronid)
				{
					is_exist=true;
				}
			}

			if(is_exist)
				continue;

			//从map中删除
			m_CronmapProcessHandle.erase((it1->second)->m_iSourceID);
			

			//删除该条记录
			if(MemTableOper::instance().DeleteBpcbInfoBybpcbd(it1->first))
			{
				bmco_information_f3(theLogger,"%s|%s|the cron task process BPCB-ID[%?d] related to this task is not alive,do unregistBpcbInfo sucessfully.",
					std::string("0"),std::string(__FUNCTION__),it1->first);
			}
			else
			{
				bmco_error_f3(theLogger,"%s|%s|the cron task process BPCB-ID[%?d] related to this task is not alive,fail to do unregistBpcbInfo.",
					std::string("0"),std::string(__FUNCTION__),it1->first);
			}
			
			
		}
	}


	
	
}



#define isdigit(a) ((unsigned char)((a) - '0') <= 9)


bool ParseField( char *ary, int modvalue, int off, const char *ptr)
{
	
	int n1 = -1;
	int n2 = -1;

	while (1) {
		int skip = 0;

		if (*ptr == '*') {
			n1 = 0;  
			n2 = modvalue - 1;
			skip = 1;
			++ptr;
		} else if (isdigit(*ptr)) {
			char *endp;
			if (n1 < 0) {
				n1 = strtol(ptr, &endp, 10) + off;
			} else {
				n2 = strtol(ptr, &endp, 10) + off;
			}
			ptr = endp; 
			skip = 1;
		}
		

		if (skip == 0) {
			return false;
		}
		if (*ptr == '-' && n2 < 0) {
			++ptr;
			continue;
		}

		if (n2 < 0) {
			n2 = n1;
		}
		if (*ptr == '/') {
			char *endp;
			skip = strtol(ptr + 1, &endp, 10);
			ptr = endp; 
		}

		
		{
			int s0 = 1;
			int failsafe = 1024;

			--n1;
			do {
				n1 = (n1 + 1) % modvalue;

				if (--s0 == 0) {
					ary[n1 % modvalue] = 1;
					s0 = skip;
				}
				if (--failsafe == 0) {
					return false;
				}
			} while (n1 != n2);
		}
		if (*ptr != ',') {
			break;
		}
		++ptr;
		n1 = -1;
		n2 = -1;
	}


	for (int i = 0; i < modvalue; ++i)
		DEBUG_LOG("need to update corn  is %d  %d ",i,(int)ary[i]);
			
	
}




bool RegularProcessTaskLoop::processTimeTask()
{



	Bmco::LocalDateTime comparenow;
	Matrix matx;
	if(!MemTableOper::instance().QueryTableAllData(strMetaBolCrontab,matx)) //查询所有的定时任务
	{
		bmco_error_f2(theLogger,"%s|%s|Failed to execute QueryAll on MetaShmBolCrontabTable",
			std::string("0"),std::string(__FUNCTION__));
		return false;
	}

	//删除不存在任务的对应bpcbid记录
	DeleteOldBpcbInfo(matx);

	for(int i=0;i<matx.rowValueCount();i++)	//遍历处理定时任务
	{

		Bmco::Int32 icronid=0;
		matx.getFieldValue(strID,i).getInt32(icronid);

		Bmco::Int32 iprogramid=0;
		matx.getFieldValue(strProgramID,i).getInt32(iprogramid);


		std::string strorgid="-1";
		strorgid=matx.getFieldValue(strOrgID,i).getString();

		Bmco::Int32 iflowid=-1;
		matx.getFieldValue(strFlowID,i).getInt32(iflowid);


		Bmco::Int32 iinstanceid=-1;
		matx.getFieldValue(strInstanceID,i).getInt32(iinstanceid);


		std::string strInstanceDynamic_parm=matx.getFieldValue(strdynamic_para,i).getString();


		//给存在的定时任务分配bpcbid,无论有效无效
		getOneCronChildInfo(icronid,iprogramid);

		

		int isvalid=0;
		matx.getFieldValue(strIsValid,i).getInt32(isvalid);

		if(isvalid == 0)  //任务失效不需要处理
			continue;

		
		int tasktype=0;
		matx.getFieldValue(strTasktype,i).getInt32(tasktype);
		

		Bmco::Int64 iNextExecuteTime=0;

		if(tasktype == 0)
		{
			//默认的处理方式

			std::string strNextexeTime=matx.getFieldValue(strNextExecuteTime,i).getString();
			
		
			std::string::size_type pos = strNextexeTime.find("-");
	   		 if (pos != std::string::npos) 
			{
				//字符串时间
				int tzd;
				Bmco::DateTime dt = Bmco::DateTimeParser::parse(strNextexeTime, tzd);
				Bmco::LocalDateTime lnextexetime(dt.year(),dt.month(),dt.day(),dt.hour(),dt.minute(),dt.second());
				iNextExecuteTime=lnextexetime.timestamp().epochMicroseconds();
			}
			 else
			 {
			 	matx.getFieldValue(strNextExecuteTime,i).getInt64(iNextExecuteTime);
			 }
			
			if((comparenow.timestamp().epochMicroseconds() - iNextExecuteTime) < 0) //下次执行时间未到不处理
				continue;

			
			
		}
		else
		{

			std::string itday,itmonth,itweek,ithour,itminute;
			itday=matx.getFieldValue(strDay,i).getString();
			itmonth=matx.getFieldValue(strMonth,i).getString();
			itweek=matx.getFieldValue(strWeek,i).getString();
			ithour=matx.getFieldValue(strHour,i).getString();
			itminute=matx.getFieldValue(strMinute,i).getString();

			char cl_Dow[7]={0};                 /* 0-6, beginning sunday */
			char cl_Mons[12]={0};           /* 0-11 */
			char cl_Hrs[24]={0};           /* 0-23 */
			char cl_Days[32]={0};               /* 1-31 */
			char cl_Mins[60]={0};               /* 0-59 */

			

			ParseField( cl_Mins, 60, 0, itminute.c_str());
			ParseField(cl_Hrs, 24, 0, ithour.c_str());
			ParseField( cl_Days, 32, 0, itday.c_str());
			ParseField( cl_Mons, 13, 0, itmonth.c_str());
			ParseField( cl_Dow, 7, 0, itweek.c_str());


			std::ostringstream oss;
			oss <<"need to excute minute is( ";
			for(int i=0;i<60;i++)
			{
				if(cl_Mins[i] == 1)
					oss << i<<",";
			}
			oss <<")";

			oss <<"hour is (";
			for(int i=0;i<24;i++)
			{
				if(cl_Hrs[i] == 1)
					oss << i<<",";
			}
			oss <<")";
				

			oss <<"day is (";
			for(int i=0;i<32;i++)
			{
				if(cl_Days[i] == 1)
					oss << i<<",";
			}
			oss <<")";

			oss <<"month is (";
			for(int i=1;i<13;i++)
			{
				if(cl_Mons[i] == 1)
					oss << i<<",";
			}
			oss <<")";

			oss <<"week is (";
			for(int i=0;i<7;i++)
			{
				if(cl_Dow[i] == 1)
					oss << i<<",";
			}
			oss <<")";
			
			INFORMATION_LOG("need to update corn  is %s ",oss.str().c_str());
	
			if(cl_Mons[comparenow.month()] != 1)
				continue;

			if(cl_Dow[comparenow.dayOfWeek()] != 1)
			{
				continue;
			}

			
			if(cl_Days[comparenow.day()] != 1)
			{
				continue;
			}


			if(cl_Hrs[comparenow.hour()] != 1)
			{
				continue;
			}

			if(cl_Mins[comparenow.minute()] != 1)
			{
				//在下一分钟重置
				m_CronmapProcessHandle[icronid]._isExecuted=false;
				continue;
			}
								

			//如果执行过，则不再执行
			if(m_CronmapProcessHandle[icronid]._isExecuted)
			{
				continue;
			}

			
			
		}
	

		Matrix prodefmatx;
		if(!MemTableOper::instance().GetProgramDefbyId(iprogramid,prodefmatx))
		{
			bmco_error_f3(theLogger,"%s|%s|TaskID[%?d]:Failed to execute queryProgramDefByID on  MetaShmProgramDefTable",
			std::string("0"),std::string(__FUNCTION__),icronid);
			continue;
		}

		std::string exepath=Bmco::replace(prodefmatx.getFieldValue(strexe_path, 0).getString(),
			std::string("${common.vnodepath}"),Bmco::Util::Application::instance().config().getString("common.vnodepath",""));

		
		Bmco::Path AbsolutePath(true);
		if (!AbsolutePath.tryParse(exepath)) 
		{
			bmco_error_f3(theLogger,"%s|%s|TaskID[%?d]:mProgramDef.AbsolutePath is wrong!",
					std::string("0"),std::string(__FUNCTION__),icronid);
			continue;
		}

		
		AbsolutePath.makeDirectory();
		std::string cmd;
		cmd.clear();
		cmd = AbsolutePath.toString() + prodefmatx.getFieldValue(strExeName, 0).getString();
		std::vector<std::string> args;
		args.clear();
		Bmco::StringTokenizer st(prodefmatx.getFieldValue(strStaticParams, 0).getString()," ",Bmco::StringTokenizer::TOK_TRIM);
		for (Bmco::StringTokenizer::Iterator it1 = st.begin();it1!=st.end();it1++)
		{

			std::string realarg="";
			if((*it1).find("common.vnodepath") != std::string::npos)
			{
				realarg=Bmco::replace((*it1),std::string("${common.vnodepath}"),Bmco::Util::Application::instance().config().getString("common.vnodepath",""));
			}
			else if((*it1).find("common.logpath") != std::string::npos)
			{
				realarg=Bmco::replace((*it1),std::string("${common.logpath}"),Bmco::Util::Application::instance().config().getString("common.logpath",""));
			}
			else if((*it1).find("common.binpath") != std::string::npos)
			{
				realarg=Bmco::replace((*it1),std::string("${common.binpath}"),
					Bmco::Util::Application::instance().config().getString("common.binpath",""));
			}
			else if((*it1).find("common.datapath") != std::string::npos)
			{
				realarg=Bmco::replace((*it1),std::string("${common.datapath}"),
					Bmco::Util::Application::instance().config().getString("common.datapath",""));
			}
			else if((*it1).find("${BolName}") != std::string::npos)
			{
				realarg=Bmco::replace((*it1),std::string("${BolName}"),
					Bmco::Util::Application::instance().config().getString("info.bol_name",""));
			}
			else if((*it1).find("${BpcbID}") != std::string::npos)
			{
				realarg=Bmco::replace((*it1),std::string("${BpcbID}"),
					Bmco::format("%?d",(m_CronmapProcessHandle[icronid].bpcbId)));
			}
			else if((*it1).find("${ProgramID}") != std::string::npos)
			{
				realarg=Bmco::replace((*it1),std::string("${ProgramID}"),
					Bmco::format("%?d",iprogramid));
			}
			else if((*it1).find("${FlowID}") != std::string::npos)
			{
				realarg=Bmco::replace((*it1),std::string("${FlowID}"),
					Bmco::format("%?d",iflowid));
			}
			else if((*it1).find("${InstanceID}") != std::string::npos)
			{
				realarg=Bmco::replace((*it1),std::string("${InstanceID}"),
					Bmco::format("%?d",iinstanceid));
			}
			else if((*it1).find("${BolID}") != std::string::npos)
			{
				realarg=Bmco::replace((*it1),std::string("${BolID}"),
					Bmco::Util::Application::instance().config().getString("info.bol_id",""));
			}
			else if((*it1).find("${OrgID}") != std::string::npos)
			{
				realarg=Bmco::replace((*it1),std::string("${OrgID}"),
					strorgid);
			}
			else if((*it1).find("${DICT_MACRO_") != std::string::npos)
			{
				
				std::string::size_type posbegin = (*it1).find("${DICT_MACRO_");
				
				if(posbegin  != std::string::npos)
				{
					std::string::size_type pos=posbegin+strlen("${DICT_MACRO_");
					std::string::size_type posend = (*it1).find('}');
					std::string keystr=(*it1).substr(pos,posend-pos);
					INFORMATION_LOG("the dynamic arg is %s ",keystr.c_str());
				
					realarg=MemTableOper::instance().GetDictDynamicParam(keystr);
				}
			}
			else
			{
				realarg=*it1;
			}
			
			args.push_back(realarg);

	
		}


		//对实例动态参数替换
		Bmco::StringTokenizer st2(strInstanceDynamic_parm," ",Bmco::StringTokenizer::TOK_TRIM);
		for (Bmco::StringTokenizer::Iterator it1 = st2.begin();it1!=st2.end();it1++)
		{

			std::string realarg="";
			std::string::size_type posbegin = (*it1).find("${DICT_MACRO_");
			
			if(posbegin  != std::string::npos)
			{
				std::string::size_type pos=posbegin+strlen("${DICT_MACRO_");
				std::string::size_type posend = (*it1).find('}');
				std::string keystr=(*it1).substr(pos,posend-pos);
				INFORMATION_LOG("the dynamic arg is %s ",keystr.c_str());
			
				realarg=MemTableOper::instance().GetDictDynamicParam(keystr);
			}
			else
			{
				realarg=*it1;
			}
			
			args.push_back(realarg);

	
		}
		
			

		if(tasktype == 0)
		{

			int cyclecount=0;
			//对于间隔性任务更新下次执行的时间
			try
			{
				Bmco::LocalDateTime DateTime;
				if(iNextExecuteTime !=0)
				{
					bmco_information_f1(theLogger,"iNextExecuteTime value is %?d",iNextExecuteTime/1000000);
					Bmco::Timestamp tms(iNextExecuteTime);
					Bmco::DateTime  dt(tms);
					DateTime.assign(dt.year(),dt.month(),dt.day(),dt.hour(),dt.minute(),dt.second());
				}
				
				int year = DateTime.year();
				int month = DateTime.month();
				int day = DateTime.day();
				int hour = DateTime.hour();
				int minute = DateTime.minute();
				int second = DateTime.second();

				bmco_information_f4(theLogger,"local DateTime year is %?d month is %?d day is %?d hour is %?d",
						year,month,day,hour);

				int itday,itmonth,itweek,ithour,itminute;
				matx.getFieldValue(strDay,i).getInt32(itday);
				matx.getFieldValue(strMonth,i).getInt32(itmonth);
				matx.getFieldValue(strWeek,i).getInt32(itweek);
				matx.getFieldValue(strHour,i).getInt32(ithour);
				matx.getFieldValue(strMinute,i).getInt32(itminute);

				if((itday ==0)&&
					(itmonth ==0)&&
					(itweek ==0)&&
					(ithour ==0)&&
					(itminute ==0))
				{
					bmco_information_f2(theLogger,"%s|%s|every period is 0,not deal,please check web",
						std::string("0"),std::string(__FUNCTION__));
					continue;
				}
				

				bmco_information_f4(theLogger,"execute between itmonth is %?d itday is %?d itweek is %?d ithour is %?d",
						itmonth,itday,itweek,ithour);

				Bmco::LocalDateTime nextExecDate(year, month, day,hour,minute,second);
				for(;;)
				{
					cyclecount++;
				
					if(itmonth + month > 12)
					{
						year=year + (itmonth + month)/12;
						month=(itmonth + month)%12;
					}
					else
					{
						month=itmonth + month;
					}
					
					nextExecDate.assign(year, month, nextExecDate.day(), 
								nextExecDate.hour(),nextExecDate.minute(),nextExecDate.second());
					nextExecDate += Bmco::Timespan(itday + (itweek) *7,ithour,itminute,0,0);

					year=nextExecDate.year();
					month=nextExecDate.month();
					

					//对于已经执行过的任务，如果下次执行时间比当前小,则更新周期至比当前时间大
					if((comparenow.timestamp().epochMicroseconds() - nextExecDate.timestamp().epochMicroseconds()) < 0) 
					{
						break;
					}

					std::string sDate = Bmco::DateTimeFormatter::format(nextExecDate, "%Y%m%d%H%M%S");
					bmco_information_f4(theLogger,"%s|%s|TaskID[%?d]:maybe restart  next exetime[%s],ignore it!",
							std::string("0"),std::string(__FUNCTION__),icronid,sDate);
					
				}

				std::string sDate = Bmco::DateTimeFormatter::format(nextExecDate, "%Y-%m-%d %H:%M:%S");

				
				
				if(!MemTableOper::instance().UpdateCronNextExeTime(icronid,sDate))
				{
					bmco_error_f3(theLogger,"%s|%s|TaskID[%?d]:update next exe time failed",
						std::string("0"),std::string(__FUNCTION__),icronid);
				}
				else
				{
					bmco_information_f4(theLogger,"%s|%s|TaskID[%?d]:update next exetime[%s]!",
						std::string("0"),std::string(__FUNCTION__),icronid,sDate);
				}


				if(!MemTableOper::instance().UpdateBpcbNextExeTime(m_CronmapProcessHandle[icronid].bpcbId,sDate))
				{
					bmco_error_f3(theLogger,"%s|%s|bpcbid[%?d]:update bpcb next exe time failed",
						std::string("0"),std::string(__FUNCTION__),m_CronmapProcessHandle[icronid].bpcbId);
				}
				else
				{
					bmco_information_f4(theLogger,"%s|%s|bpcbid[%?d]:update bpcb next exetime[%s]!",
						std::string("0"),std::string(__FUNCTION__),m_CronmapProcessHandle[icronid].bpcbId,sDate);
				}
				
				
				
			}
			catch(Bmco::Exception &e)
			{
				bmco_error_f3(theLogger,"%s|%s|catch bmco %s",std::string("0"),std::string(__FUNCTION__),e.displayText());
			}
			catch (std::exception& e) 
			 {
			 	bmco_error_f3(theLogger, "%s|%s| update time task   catch  exception %s."
					, std::string("0"),std::string(__FUNCTION__),std::string(e.what()));
	   		}
			catch(...)
			{
				bmco_error_f2(theLogger, "%s|%s| gupdate time task   catch    sqlite exception."
					, std::string("0"),std::string(__FUNCTION__));
			}


			//大于一个周期，属于历史，不补不执行
			if(cyclecount>1)
			{
				continue;
			}


			//超过1分钟，属于历史不执行
			if((comparenow.timestamp().epochMicroseconds() - iNextExecuteTime) > 60*1000*1000) 
				continue;
			

		}


		try
		{
			ScopedEnvSetter envsetter(Bmco::Environment::get(LDENVNAME),
				prodefmatx.getFieldValue(strlib_path, 0).getString());
			ProcessHandle ph =  ProcessHandle(Process::launch(cmd, args)); //启动程序，有可能有异常，通过下面catch进行捕获
			{
				/*int rc = ph.wait();*/
				//bmco_information_f4(theLogger,"%s|%s|launch process: cmd is [%s],pid is [%?d]",
					//std::string("0"),std::string(__FUNCTION__),std::string(cmd),ph.id());
			}

			std::string strargs="";
			for (Bmco::Process::Args::iterator it = args.begin(); it!=args.end(); it++) 
			{	
				strargs+=*it+" ";
			}

			WARNING_LOG(0,"launch bin:%s,args is <%s>",cmd.c_str(),strargs.c_str());
			m_CronmapProcessHandle[icronid]._isExecuted=true;


			//更新本次执行时间
			std::string thisDate = Bmco::DateTimeFormatter::format(comparenow, "%Y%m%d%H%M%S");
			if(!MemTableOper::instance().UpdateBpcbThisExeTime(m_CronmapProcessHandle[icronid].bpcbId,thisDate))
			{
				bmco_error_f3(theLogger,"%s|%s|bpcbid[%?d]:update bpcb this exe time failed",
					std::string("0"),std::string(__FUNCTION__),m_CronmapProcessHandle[icronid].bpcbId);
			}
			else
			{
				bmco_information_f4(theLogger,"%s|%s|bpcbid[%?d]:update bpcb this exetime[%s]!",
					std::string("0"),std::string(__FUNCTION__),m_CronmapProcessHandle[icronid].bpcbId,thisDate);
			}

		}
		catch(Bmco::Exception &e)
		{
			bmco_error_f3(theLogger,"%s|%s|%s",std::string("0"),std::string(__FUNCTION__),e.displayText());
			continue;
		}


		
	}

	return true;
}






bool RegularProcessTaskLoop::isBolStatueNormal() 
{
	
	int _s = BOL_NORMAL;
	MemTableOper::instance().GetBolStatus(_s);	

	if (_s != BOL_OFFLINE)
	{
		return true;
	}
	bmco_warning_f2(theLogger,"%s|%s|checked that bol status is set to abnormal and #0 will shutdown",
					std::string("0"),std::string(__FUNCTION__));
	return false;
}


	
//更新自己BPCB的状态（1#）
//BOL初始化的时候，bpcb表里面只有0和1号进程两条记录，把RegularProcessTask表里面需要启动的进程启起来
bool RegularProcessTaskLoop::init()
{
	
	std::string starttask=Bmco::Util::Application::instance().config().getString("process.BolTask.Start.BpcbID","");

	if(starttask == "")
	{
		bmco_information_f2(theLogger,"%s|%s|no need run bol task",
				std::string("0"),std::string(__FUNCTION__));
		return true;
	}
	
	
	std::vector<std::string> staridvec;
	boost::split(staridvec,starttask,boost::is_any_of("+"));
	
	m_bolstartBpcbset.clear();
	for(BOOST_AUTO(it,staridvec.begin());it!=staridvec.end();it++)
	{
		m_bolstartBpcbset.insert(boost::lexical_cast<int>(*it));
	}
	
	for(BOOST_AUTO(it,m_bolstartBpcbset.begin());it!=m_bolstartBpcbset.end();it++)
	{
		int bpcbid=*it;
		Matrix prodefmtx;
		
		if(!MemTableOper::instance().GetProgramDefBytypeAndBpcbID( BOL_PROGRAM_TYPE_A, bpcbid, prodefmtx))
		{
			bmco_error_f3(theLogger,"%s|%s|%d号进程查询程序定义表失败.",
			std::string("0"),std::string(__FUNCTION__),bpcbid);
			return false;
		}

		int programid=0;
		prodefmtx.getFieldValue(strID,0).getInt32(programid);
		
		MetaRegularProcessTask::Ptr taskPtr= new MetaRegularProcessTask(0,programid,0,0,CONTROL_START,0,false,0,"");
		if(false == getOneChildInfo(taskPtr))
		{
			bmco_error_f3(theLogger,"%s|%s|%d 号进程注册失败",
			std::string("0"),std::string(__FUNCTION__),bpcbid);
			return false;
		}

		if(false == launchOneChild(taskPtr))
		{
			bmco_error_f3(theLogger,"%s|%s|%d 号进程启动失败",
			std::string("0"),std::string(__FUNCTION__),bpcbid);

			return false;
		}else
		{
			bmco_information_f3(theLogger,"%s|%s|%d 号进程启动成功",
				std::string("0"),std::string(__FUNCTION__),bpcbid);
		}
	}

	
	

	if(!getAllRegularTask(m_vecRegularTaskLast))
	{
		bmco_error_f2(theLogger,"%s|%s|Failed to execute QueryAll on MetaShmRegularProcessTaskTable",std::string("0"),std::string(__FUNCTION__));
		return false;
	}

	return true;
}



//根据id获取程序定义
 bool RegularProcessTaskLoop::getAllRegularTask(std::vector<MetaRegularProcessTask>& vecMetaRegularProcessTask)
 {

		try
		{
 
		Matrix regularmtx;
		if(!MemTableOper::instance().QueryTableAllData( strMetaRegularProcessTask, regularmtx))
		{
			bmco_error_f2(theLogger,"%s|%s|:Failed to query regular task",
			std::string("0"),std::string(__FUNCTION__));
			return false;
		}

		for(int i=0;i<regularmtx.rowValueCount();i++)
		{
			Bmco::Int32 id;
			Bmco::Int32 programID;
			Bmco::Int32 instanceID;
			Bmco::Int32 flowID;
			Bmco::Int32 isValid;
			Bmco::Int64 timeStamp;
			Bmco::Int32 isCapValid;
			Bmco::Int32 Priority;
			std::string extendCommand;
			Bmco::Int32 isNoMonitor;
			Bmco::Int32 pausestatus;
			Bmco::Int32 iswarning;
			std::string strorgid;
			std::string strdynamicparam;

			regularmtx.getFieldValue(strID, i).getInt32(id);
			regularmtx.getFieldValue(strProgramID, i).getInt32(programID);
			regularmtx.getFieldValue(strInstanceID, i).getInt32(instanceID);
			regularmtx.getFieldValue(strFlowID, i).getInt32(flowID);
			regularmtx.getFieldValue(strIsValid, i).getInt32(isValid);
			regularmtx.getFieldValue(strTimeStamp, i).getInt64(timeStamp);
			regularmtx.getFieldValue(strIsValid, i).getInt32(isValid);
			regularmtx.getFieldValue(strPriority, i).getInt32(Priority);
			extendCommand=regularmtx.getFieldValue(strExCommand, i).getString();
			regularmtx.getFieldValue(strIsNoMonitor, i).getInt32(isNoMonitor);
			regularmtx.getFieldValue(strpausestatus, i).getInt32(pausestatus);
			regularmtx.getFieldValue(striswarning, i).getInt32(iswarning);
			strorgid=regularmtx.getFieldValue(strOrgID, i).getString();
			strdynamicparam=regularmtx.getFieldValue(strdynamic_para, i).getString();
			
			MetaRegularProcessTask::Ptr pdPtr = new MetaRegularProcessTask(id,
				programID,instanceID,flowID,isValid,Bmco::Timestamp(timeStamp),
				isCapValid!=0,Priority,extendCommand.c_str(),isNoMonitor,pausestatus,iswarning,strorgid,strdynamicparam);

			vecMetaRegularProcessTask.push_back(*pdPtr);
			
		}
		}
		catch(...)
		{
			bmco_error_f2(theLogger, "%s|%s| getAllRegularTask   sqlite exception."
				, std::string("0"),std::string(__FUNCTION__));
			return false;
		}
		

		

		return true;
 }



bool RegularProcessTaskLoop::CheckUpdateFile()
{

	std::string updatefilename="/dev/shm/"+Bmco::Util::Application::instance().config().getString("info.bol_name","")+
		  "update";

	 Bmco::File updatefile(updatefilename);
    if (updatefile.exists()) 
    {
    	 bmco_information_f3(theLogger,"%s|%s|update file exist  %s",
				std::string("0"),std::string(__FUNCTION__),updatefilename);
        return true;
    }
    else
    {
    	bmco_information_f3(theLogger,"%s|%s|update file  not  exist  %s",
				std::string("0"),std::string(__FUNCTION__),updatefilename);
        return false;
    }

	return false;
}




bool RegularProcessTaskLoop::waitForRegularTask()
{
	if(m_vecRegularTaskLast.size() == 0)
	{
		bmco_warning_f2(theLogger,"%s|%s|waiting for RegularTask sync from CloudAgent.",std::string("0"),std::string(__FUNCTION__));
		this->sleep(5000);
		return false;
	}

	///由于ini配置文件的程序定义的id和云端下发下来的程序定义ID有可能不一致，需要按照最新的进行刷新
	////刷新包括两部分：1.私有内存的刷新  2.BPCB表里面2号和5号程序程序ID的刷新
	///更新之前启动#2,#5程序定义
	for(std::map<ProcessPrimaryKey,ManagedChildInfo > ::iterator itMap = mapProcessHandle.begin();
		itMap != mapProcessHandle.end();itMap++)
	{

		Matrix prodefmtx;
	
		if(!MemTableOper::instance().GetProgramDefBytypeAndBpcbID( itMap->second.programDef->ProcessType,
			itMap->second.programDef->BolBpcbID, prodefmtx))
		{
			bmco_error_f3(theLogger,"%s|%s|BPCB_ID[%?d]:刷新内部使用程序定义表失败.",
			std::string("0"),std::string(__FUNCTION__),itMap->second.programDef->BolBpcbID);
			continue;
		}

		Bmco::Int32 programid=0;
		prodefmtx.getFieldValue(strID,0).getInt32(programid);
		itMap->second.programDef = getProcessTypeByID(programid);

		if (!MemTableOper::instance().UpdateBpcbProgramID( (itMap->second).bpcbId, programid)) 
		{
			bmco_error_f3(theLogger,"%s|%s|BPCB_ID[%?d]:failed to update  bpcb item programid .",
				std::string("0"), std::string(__FUNCTION__),(itMap->second).bpcbId);
		  	continue;
		}
		itMap->second.bpcbItem->m_iProgramID = itMap->second.programDef->ID;

	}

	///给任务赋值优先级
	for(std::vector<MetaRegularProcessTask>::iterator it = m_vecRegularTaskLast.begin();
		it != m_vecRegularTaskLast.end();it++)
	{
		it->Priority = getProcessTypeByID(it->ProgramID)->Priority;
	}

	////任务按优先级排序
	std::sort(m_vecRegularTaskLast.begin(),m_vecRegularTaskLast.end());

	{
		//再起其他BOL进程
		std::vector<MetaRegularProcessTask>::iterator it = m_vecRegularTaskLast.begin();
		for(;it != m_vecRegularTaskLast.end();it++)
		{
			if((getProcessTypeByID(it->ProgramID))->ProcessType != BOL_PROGRAM_TYPE_A)
				continue;

			MetaRegularProcessTask::Ptr ptr = new MetaRegularProcessTask(*it);
			if(false == getOneChildInfo(ptr))
				continue;
			if(false == launchOneChild(ptr))
			{
				bmco_error_f4(theLogger,"%s|%s|任务ID[%?d],程序ID[%?d]:进程启动失败",
					std::string("0"),std::string(__FUNCTION__),ptr->ID,ptr->ProgramID);
				continue;
			}
		}
		bmco_information_f2(theLogger,"%s|%s|所有BOL任务处理完毕.",
			std::string("0"),std::string(__FUNCTION__));
	}

	{
		
		std::vector<MetaRegularProcessTask>::iterator it = m_vecRegularTaskLast.begin();
		for(;it != m_vecRegularTaskLast.end();it++)
		{
		    if((getProcessTypeByID(it->ProgramID))->ProcessType == BOL_PROGRAM_TYPE_A)
				continue;

			MetaRegularProcessTask::Ptr ptr = new MetaRegularProcessTask(*it);
			if(false == getOneChildInfo(ptr))
				continue;


			//如果升级文件存在，则不启程序!
			if(CheckUpdateFile())
			{
				continue;
			}

			if(ptr->IsValid != CONTROL_START)
			{
				INFORMATION_LOG("not valid ,not start the program proid [%?d]",ptr->ProgramID);
				continue;
			}

			
			if(false == launchOneChild(ptr))
			{
				bmco_error_f4(theLogger,"%s|%s|ID[%?d],程序ID[%?d]:进程启动失败",
					std::string("0"),std::string(__FUNCTION__),ptr->ID,ptr->ProgramID);
				continue;
			}
		}
		bmco_information_f2(theLogger,"%s|%s|所有HLA任务处理完毕",
						std::string("0"),std::string(__FUNCTION__));
		
	}
	return true;
}

 bool RegularProcessTaskLoop::checkProcess()
 {
	
	 


	std::vector<std::vector<std::string> > omcvecs;
	omcvecs.clear();

	Bmco::LocalDateTime statustime;
	
	
	//0#不在监控范围，单独写信息点
	#ifdef BOL_OMC_WRITER 
	if(Bmco::LocalDateTime().timestamp().epochMicroseconds()-
						m_OmcNextExeTime.timestamp().epochMicroseconds()> 0)
	{
		MetaBpcbInfo::Ptr bpcbkernel= new  MetaBpcbInfo(BOL_PROCESS_KERNEL);
		
		bmco_information(theLogger,"begin to write omc json file");
		if (getBpcbItemByID(0,bpcbkernel)) 
		{
			
			std::vector<std::string> omcvec;
			omcvec.push_back(std::string("40201002"));
			omcvec.push_back(std::string("0"));
			omcvec.push_back(boost::lexical_cast<std::string>(bpcbkernel->m_iSysPID));
			omcvec.push_back(boost::lexical_cast<std::string>(bpcbkernel->m_iProgramID));
			omcvec.push_back(DateTimeFormatter::format(bpcbkernel->m_tCreateTime, "%Y%m%d%H%M%S"));
			omcvec.push_back(DateTimeFormatter::format(bpcbkernel->m_tHeartBeat, "%Y%m%d%H%M%S"));
			omcvec.push_back(boost::lexical_cast<std::string>(bpcbkernel->m_iStatus));
			omcvec.push_back(boost::lexical_cast<std::string>(bpcbkernel->m_iCPU));
			omcvec.push_back(boost::lexical_cast<std::string>(bpcbkernel->m_iRAM));
			omcvec.push_back(boost::lexical_cast<std::string>(bpcbkernel->m_iFlowID));
			omcvec.push_back(boost::lexical_cast<std::string>(bpcbkernel->m_iInstanceID));
			omcvec.push_back(Bmco::Util::Application::instance().config().getString("info.bol_name","bol123"));
			omcvec.push_back(boost::lexical_cast<std::string>(HandlerManager::_ConnNum));
			omcvec.push_back(std::string("0"));
			omcvec.push_back(DateTimeFormatter::format(statustime.timestamp(), "%Y%m%d%H%M%S"));

			omcvecs.push_back(omcvec);

			bmco_information(theLogger,"push json vector success");
			
		}
		else
		{
			bmco_warning_f2(theLogger,"%s|%s|BPCB_ID[0]:failed to query  bpcb item within expired seconds",
							std::string("0"), std::string(__FUNCTION__));
		}
		
	}
	#endif

	
	//批量获取所有bpcb信息
	Bmco::LocalDateTime heartnow;
	std::map<Bmco::Int32,MetaBpcbInfo::Ptr> bpcbmap;
	if(!getAllBpcbItem(bpcbmap))
	{
		bmco_warning_f2(theLogger,"%s|%s|failed to query all  bpcb item",
							std::string("0"), std::string(__FUNCTION__));
		return false;
	}

	if(bpcbmap.size()<=0)
	{
		bmco_warning_f2(theLogger,"%s|%s|bpcb table is null",
							std::string("0"), std::string(__FUNCTION__));
		return false;
	}
	

		
	 for( std::map<ProcessPrimaryKey,ManagedChildInfo >::iterator itMap = mapProcessHandle.begin();
		 itMap != mapProcessHandle.end();itMap++)
	 {
		 try
		 {

				

		 
		 		//更新handl中最新的bcbp信息
				if (bpcbmap.find((itMap->second).bpcbId) == bpcbmap.end()) 
				{
						bmco_warning_f3(theLogger,"%s|%s|BPCB_ID[%?d]:failed to get  bpcb item in allmap",
							std::string("0"), std::string(__FUNCTION__),(itMap->second).bpcbId);
						continue;
				}
				(itMap->second).bpcbItem=bpcbmap[(itMap->second).bpcbId];
				


				bmco_information_f4(theLogger,"%s|%s|BPCB_ID[%?d]:is no monitor value is %d.",
					std::string("0"),std::string(__FUNCTION__),(itMap->second).bpcbId,(itMap->second).isNoMonitor);
		 
				//不监控，不改状态，不拉起
				if((itMap->second).isNoMonitor == 2)
		   		{
		   			bmco_information_f3(theLogger,"%s|%s|BPCB_ID[%?d]:child no need to deal.",
					std::string("0"),std::string(__FUNCTION__),(itMap->second).bpcbId);
					continue;
		   		}
		


				if(itMap->second.taskvalid == CONTROL_START)
				{
					
					//到期输出信息点文件
					#ifdef BOL_OMC_WRITER 
					
					if(Bmco::LocalDateTime().timestamp().epochMicroseconds()-
						m_OmcNextExeTime.timestamp().epochMicroseconds()> 0)
					{
						if (1 == itMap->second.is_warning) {
                            INFORMATION_LOG("due to bpcbid[%ld], programid[%d] is_warning: %d, need to generate omc", 
                                (itMap->second).bpcbId, (itMap->second).bpcbItem->m_iProgramID, itMap->second.is_warning);
                            
                            std::vector<std::string> omcvec;
						    omcvec.push_back(std::string("40201002"));
						    omcvec.push_back(boost::lexical_cast<std::string>((itMap->second).bpcbId));
						    omcvec.push_back(boost::lexical_cast<std::string>((itMap->second).bpcbItem->m_iSysPID));
						    omcvec.push_back(boost::lexical_cast<std::string>((itMap->second).bpcbItem->m_iProgramID));
						    omcvec.push_back(DateTimeFormatter::format((itMap->second).bpcbItem->m_tCreateTime, "%Y%m%d%H%M%S"));
						    omcvec.push_back(DateTimeFormatter::format((itMap->second).bpcbItem->m_tHeartBeat, "%Y%m%d%H%M%S"));
						    omcvec.push_back(boost::lexical_cast<std::string>((itMap->second).bpcbItem->m_iStatus));
						    omcvec.push_back(boost::lexical_cast<std::string>((itMap->second).bpcbItem->m_iCPU));
						    omcvec.push_back(boost::lexical_cast<std::string>((itMap->second).bpcbItem->m_iRAM));
						    omcvec.push_back(boost::lexical_cast<std::string>((itMap->second).bpcbItem->m_iFlowID));
						    omcvec.push_back(boost::lexical_cast<std::string>((itMap->second).bpcbItem->m_iInstanceID));
						    omcvec.push_back(Bmco::Util::Application::instance().config().getString("info.bol_name","bol123"));
						    omcvec.push_back(std::string("0"));
						    omcvec.push_back(std::string("0"));
						    omcvec.push_back(DateTimeFormatter::format(statustime.timestamp(), "%Y%m%d%H%M%S"));

						    omcvecs.push_back(omcvec);
                        }
						else {
                            INFORMATION_LOG("due to bpcbid[%ld], programid[%d] is_warning: %d, not need to generate omc", 
                                (itMap->second).bpcbId, (itMap->second).bpcbItem->m_iProgramID, itMap->second.is_warning);
                        }						
					}

					#endif
					
					
					
			
					// check bpcb item, if child is alive, do nothing
					switch ((itMap->second).bpcbItem->m_iStatus)
					{
						case ST_READY:
						case ST_RUNNING:
						case ST_HOLDING:
						case ST_MAITENANCE:
							bmco_information_f3(theLogger,"%s|%s|BPCB_ID[%?d]:checked child process status is normal.",
								std::string("0"),std::string(__FUNCTION__),(itMap->second).bpcbId);
							//continue;
							break;
						case ST_INIT :
						case ST_ABORT:
						case ST_STOP:
						case ST_CORE:
						case ST_DEAD:
						case ST_STOPING:
						default:
							bmco_warning_f3(theLogger,"%s|%s|BPCB_ID[%?d]:checked child process status is abnormal.",
								std::string("0"),std::string(__FUNCTION__),(itMap->second).bpcbId);

							OMC_LOG("20101002|%d|0|%d|%d|%s|%d|%d|%d|%s|%s|%s",
							(itMap->second).bpcbId,
							(itMap->second).bpcbItem->m_iSysPID,
							(itMap->second).bpcbItem->m_iProgramID,
							DateTimeFormatter::format((itMap->second).bpcbItem->m_tHeartBeat, "%Y%m%d%H%M%S").c_str(),
							(itMap->second).bpcbItem->m_iStatus,
							(itMap->second).bpcbItem->m_iFlowID,
							(itMap->second).bpcbItem->m_iInstanceID,
						 	Bmco::Util::Application::instance().config().getString("info.bol_name","bol123").c_str(),
						 	DateTimeFormatter::format(Bmco::LocalDateTime().timestamp(), "%Y%m%d%H%M%S").c_str(),
						 	Bmco::Util::Application::instance().config().getString("info.ne_host","shanghai").c_str()
						 	);


							
							break;
					}

					bool needrestart=false;
					
					
					// child is not alive and if expired, launch it again
					
					
					//任务有效的情况下
					
					if (heartnow.timestamp().epochMicroseconds() -(itMap->second).bpcbItem->m_tHeartBeat.epochMicroseconds()  
						>= (itMap->second).expSeconds * 1000 * 1000)
					{

						std::string strtimeheartbeat=DateTimeFormatter::format((itMap->second).bpcbItem->m_tHeartBeat, "%Y%m%d%H%M%S");
						std::string strtimenow=DateTimeFormatter::format(heartnow, "%Y%m%d%H%M%S");
				
						bmco_information_f3(theLogger,"BPCB_ID[%?d]:heartbeat is %s and now is %s",	(itMap->second).bpcbId,
									strtimeheartbeat,strtimenow);


						OMC_LOG("20101002|%d|1|%d|%d|%s|%d|%d|%d|%s|%s|%s",
							(itMap->second).bpcbId,
							(itMap->second).bpcbItem->m_iSysPID,
							(itMap->second).bpcbItem->m_iProgramID,
							DateTimeFormatter::format((itMap->second).bpcbItem->m_tHeartBeat, "%Y%m%d%H%M%S").c_str(),
							(itMap->second).bpcbItem->m_iStatus,
							(itMap->second).bpcbItem->m_iFlowID,
							(itMap->second).bpcbItem->m_iInstanceID,
						 	Bmco::Util::Application::instance().config().getString("info.bol_name","bol123").c_str(),
						 	DateTimeFormatter::format(Bmco::LocalDateTime().timestamp(), "%Y%m%d%H%M%S").c_str(),
						 	Bmco::Util::Application::instance().config().getString("info.ne_host","shanghai").c_str()
						 	);

						

						bmco_information_f2(theLogger,"%s|%s|before enter into  kill syspid.",
								std::string("0"),std::string(__FUNCTION__));
						//进程不存在，将其置为core，存在置为僵死
						#if BMCO_OS == BMCO_OS_LINUX || BMCO_OS_FAMILY_UNIX == 1
						if(((itMap->second).bpcbItem->m_iSysPID==0) || 
							(-1 == kill((itMap->second).bpcbItem->m_iSysPID,0)))
						{

							
							if(MemTableOper::instance().UpdateBpcbStatusByID((itMap->second).bpcbId,ST_CORE))
							{
								bmco_warning_f4(theLogger,"%s|%s|BPCB_ID[%?d] syspid [%?d]:child is not alive  in  expire seconds and pid not exist we set it core in BPCP.",
									std::string("0"),std::string(__FUNCTION__),(itMap->second).bpcbId,(itMap->second).bpcbItem->m_iSysPID);
								
							}

                            if (ST_CORE != (itMap->second).bpcbItem->m_iStatus && 1 == itMap->second.is_warning) {
                                INFORMATION_LOG("due to bpcbid[%ld], programid[%d] heartbeat time out and change status from status[%d] to ST_CORE, generate omc file", 
                                    (itMap->second).bpcbId, (itMap->second).bpcbItem->m_iProgramID, (itMap->second).bpcbItem->m_iStatus);

                                std::vector<std::vector<std::string> > statusChangedVecVec;
                                std::vector<std::string> omcvec;
						        omcvec.push_back(std::string("40201010"));
						        omcvec.push_back(boost::lexical_cast<std::string>((itMap->second).bpcbId));
						        omcvec.push_back(boost::lexical_cast<std::string>((itMap->second).bpcbItem->m_iSysPID));
						        omcvec.push_back(boost::lexical_cast<std::string>((itMap->second).bpcbItem->m_iProgramID));
						        omcvec.push_back(DateTimeFormatter::format((itMap->second).bpcbItem->m_tCreateTime, "%Y%m%d%H%M%S"));
						        omcvec.push_back(DateTimeFormatter::format((itMap->second).bpcbItem->m_tHeartBeat, "%Y%m%d%H%M%S"));
						        omcvec.push_back(boost::lexical_cast<std::string>(ST_CORE));
						        omcvec.push_back(boost::lexical_cast<std::string>((itMap->second).bpcbItem->m_iCPU));
						        omcvec.push_back(boost::lexical_cast<std::string>((itMap->second).bpcbItem->m_iRAM));
						        omcvec.push_back(boost::lexical_cast<std::string>((itMap->second).bpcbItem->m_iFlowID));
						        omcvec.push_back(boost::lexical_cast<std::string>((itMap->second).bpcbItem->m_iInstanceID));
						        omcvec.push_back(Bmco::Util::Application::instance().config().getString("info.bol_name","bol123"));
						        omcvec.push_back(std::string("0"));
						        omcvec.push_back(std::string("0"));
						        omcvec.push_back(DateTimeFormatter::format(statustime.timestamp(), "%Y%m%d%H%M%S"));
                                statusChangedVecVec.push_back(omcvec);

                                OMCLOGVEC(statusChangedVecVec); 
                            }                          


							if((MemTableOper::instance().UpdateBpcbCpuUse(( itMap->second).bpcbId,0)) 
								&&(MemTableOper::instance().UpdateBpcbMemUse(( itMap->second).bpcbId,0)))
							{
								bmco_information_f3(theLogger,"%s|%s|%s.",
										std::string("0"),std::string(__FUNCTION__),
										Bmco::format("BPCB_ID[%?d]:update using info on MetaShmBpcbInfoTable success",(itMap->second).bpcbId));
							}
							
							
						}
						else
						{
							bmco_information_f2(theLogger,"%s|%s| before enter into UpdateBpcbStatusByID.",
								std::string("0"),std::string(__FUNCTION__));
							
							if(MemTableOper::instance().UpdateBpcbStatusByID((itMap->second).bpcbId,ST_DEAD))
							{
								bmco_warning_f4(theLogger,"%s|%s|BPCB_ID[%?d] syspid [%?d]:child is not alive with in  expire seconds and pid exist we set it dead in BPCP.",
									std::string("0"),std::string(__FUNCTION__),(itMap->second).bpcbId,(itMap->second).bpcbItem->m_iSysPID);
								
							}

                            if (ST_DEAD != (itMap->second).bpcbItem->m_iStatus && 1 == itMap->second.is_warning) {
                                INFORMATION_LOG("due to bpcbid[%ld], programid[%d] heartbeat time out and change status from status[%d] to ST_DEAD, generate omc file", 
                                    (itMap->second).bpcbId, (itMap->second).bpcbItem->m_iProgramID, (itMap->second).bpcbItem->m_iStatus);

                                std::vector<std::vector<std::string> > statusChangedVecVec;
                                std::vector<std::string> omcvec;
						        omcvec.push_back(std::string("40201010"));
						        omcvec.push_back(boost::lexical_cast<std::string>((itMap->second).bpcbId));
						        omcvec.push_back(boost::lexical_cast<std::string>((itMap->second).bpcbItem->m_iSysPID));
						        omcvec.push_back(boost::lexical_cast<std::string>((itMap->second).bpcbItem->m_iProgramID));
						        omcvec.push_back(DateTimeFormatter::format((itMap->second).bpcbItem->m_tCreateTime, "%Y%m%d%H%M%S"));
						        omcvec.push_back(DateTimeFormatter::format((itMap->second).bpcbItem->m_tHeartBeat, "%Y%m%d%H%M%S"));
						        omcvec.push_back(boost::lexical_cast<std::string>(ST_DEAD));
						        omcvec.push_back(boost::lexical_cast<std::string>((itMap->second).bpcbItem->m_iCPU));
						        omcvec.push_back(boost::lexical_cast<std::string>((itMap->second).bpcbItem->m_iRAM));
						        omcvec.push_back(boost::lexical_cast<std::string>((itMap->second).bpcbItem->m_iFlowID));
						        omcvec.push_back(boost::lexical_cast<std::string>((itMap->second).bpcbItem->m_iInstanceID));
						        omcvec.push_back(Bmco::Util::Application::instance().config().getString("info.bol_name","bol123"));
						        omcvec.push_back(std::string("0"));
						        omcvec.push_back(std::string("0"));
						        omcvec.push_back(DateTimeFormatter::format(statustime.timestamp(), "%Y%m%d%H%M%S"));
                                statusChangedVecVec.push_back(omcvec);

                                OMCLOGVEC(statusChangedVecVec);           
                            }

							bmco_information_f2(theLogger,"%s|%s| after enter into UpdateBpcbStatusByID.",
								std::string("0"),std::string(__FUNCTION__));
							
						}
						#endif


						if ((itMap->second).bpcbItem->m_iAction != 0)
				   		{
				   			bmco_warning_f3(theLogger,"%s|%s|BPCB_ID[%?d]:child is not alive with in expire seconds and action is not 0,we will relaunch it.",
							std::string("0"),std::string(__FUNCTION__),(itMap->second).bpcbId);
							needrestart=true;
				   		}
						
						
						
				   		if(itMap->second.isNoMonitor  == 0)
				   		{
				   			bmco_warning_f3(theLogger,"%s|%s|BPCB_ID[%?d]:child is not alive with in expire seconds need monitor,we will relaunch it.",
							std::string("0"),std::string(__FUNCTION__),(itMap->second).bpcbId);
							needrestart=true;
				   		}
						
						
					}


					//配置需监控cpuram且对于有效的任务，监控其cpu或者ram
					if(m_iscontrolcpuram)
					{
						//超过阀值
						if((itMap->second).programDef->Cpulimit > 0)
						{
							if((itMap->second).bpcbItem->m_iCPU > (itMap->second).programDef->Cpulimit)
							{
								bmco_information_f3(theLogger,"%s|%s|BPCB_ID[%?d]:cpu over the limit.we will relaunch it",
									std::string("0"),std::string(__FUNCTION__),(itMap->second).bpcbId);
								needrestart=true;
							}
						}
						
						if((itMap->second).programDef->Ramlimit > 0)
						{
							if((itMap->second).bpcbItem->m_iRAM > (itMap->second).programDef->Ramlimit)
							{
								bmco_information_f3(theLogger,"%s|%s|BPCB_ID[%?d]:ram over the limit.we will relaunch it.",
									std::string("0"),std::string(__FUNCTION__),(itMap->second).bpcbId);
								needrestart=true;
							}
						}
					}


					if(needrestart)
					{
					
						MetaRegularProcessTask::Ptr ptr = new  MetaRegularProcessTask(0,itMap->first.m_iProgramID,
						 				itMap->first.m_iInstanceID,itMap->first.m_iFlowID,CONTROL_TERMINATE,0,false,0,
										itMap->second.exCommand.c_str(),itMap->second.isNoMonitor,0,
										itMap->second.is_warning,itMap->second.strorgid,itMap->second.dynamicargs);
						
						if(terminateOneChild(ptr))
						{
							ptr->IsValid=CONTROL_START;
							//重置为true，因为terminateOneChild会置为false
							launchOneChild(ptr);
						}	
					}
					else
					{
						updateBpcbUsingInfo((itMap->second).bpcbId);
					}

					
					
					
				}
				else
				{
					// check bpcb item, if child is alive, do nothing
					switch ((itMap->second).bpcbItem->m_iStatus)
					{
						case ST_STOP:
							break;
						default:
							bmco_information_f3(theLogger,"%s|%s|BPCB_ID[%?d]:task invalid checked child process status is abnormal.",
								std::string("0"),std::string(__FUNCTION__),(itMap->second).bpcbId);
							break;
					}


					
					//如果是需要告警的任务，则写信息点
					if(itMap->second.is_warning == 1)
					{

						#ifdef BOL_OMC_WRITER 
						
						if(Bmco::LocalDateTime().timestamp().epochMicroseconds()-
							m_OmcNextExeTime.timestamp().epochMicroseconds()> 0)
						{
							
							std::vector<std::string> omcvec;
							omcvec.push_back(std::string("40201002"));
							omcvec.push_back(boost::lexical_cast<std::string>((itMap->second).bpcbId));
							omcvec.push_back(boost::lexical_cast<std::string>((itMap->second).bpcbItem->m_iSysPID));
							omcvec.push_back(boost::lexical_cast<std::string>((itMap->second).bpcbItem->m_iProgramID));
							omcvec.push_back(DateTimeFormatter::format((itMap->second).bpcbItem->m_tCreateTime, "%Y%m%d%H%M%S"));
							omcvec.push_back(DateTimeFormatter::format((itMap->second).bpcbItem->m_tHeartBeat, "%Y%m%d%H%M%S"));
							omcvec.push_back(boost::lexical_cast<std::string>((itMap->second).bpcbItem->m_iStatus));
							omcvec.push_back(boost::lexical_cast<std::string>((itMap->second).bpcbItem->m_iCPU));
							omcvec.push_back(boost::lexical_cast<std::string>((itMap->second).bpcbItem->m_iRAM));
							omcvec.push_back(boost::lexical_cast<std::string>((itMap->second).bpcbItem->m_iFlowID));
							omcvec.push_back(boost::lexical_cast<std::string>((itMap->second).bpcbItem->m_iInstanceID));
							omcvec.push_back(Bmco::Util::Application::instance().config().getString("info.bol_name","bol123"));
							omcvec.push_back(std::string("0"));
							omcvec.push_back(std::string("0"));
							omcvec.push_back(DateTimeFormatter::format(statustime.timestamp(), "%Y%m%d%H%M%S"));

							omcvecs.push_back(omcvec);
							
							
						}

						#endif
						
					}
					
					
					
				}
				

			 }
			 catch(Bmco::Exception &e)
			 {
				 bmco_error_f3(theLogger,"%s|%s|%s",std::string("0"),std::string(__FUNCTION__),e.displayText());
			 }
	 }


	//更新下次写信息点的时间
	#ifdef BOL_OMC_WRITER 
	if((Bmco::LocalDateTime().timestamp().epochMicroseconds()-
						m_OmcNextExeTime.timestamp().epochMicroseconds()> 0) 
		&& (omcvecs.size()>0))
	{

		if(MemTableOper::instance().GetisOmcFileValid("40201002"))
		{
			OMCLOGVEC(omcvecs);
		}
		
	
		omcvecs.clear();
		Bmco::Int32 omcinterval=MemTableOper::instance().GetOmcFileTime("40201002");
		if(omcinterval == -1)
		{
			omcinterval=MemTableOper::instance().GetOmcFileTime("common");
			if(omcinterval == -1)
			{
				omcinterval=300;
			}
		}
		
		Bmco::Timespan tspan(omcinterval,0);
		m_OmcNextExeTime+=tspan;
		
	}
	#endif
	
	 return true;
 }

//根据id获取程序定义
 MetaProgramDef::Ptr RegularProcessTaskLoop::getProcessTypeByID(Bmco::Int32 programID)
 {

 		try
 		{
		Matrix prodefmtx;
		if(!MemTableOper::instance().GetProgramDefbyId( programID, prodefmtx))
		{
			bmco_error_f3(theLogger,"%s|%s|ProgramID[%?d]:Failed to execute queryProgramDefByID on  MetaShmProgramDefTable",
			std::string("0"),std::string(__FUNCTION__),programID);
		}

		Bmco::Int32 maxInstance;
		Bmco::Int32 minInstance;
		Bmco::Int32 processType;
		Bmco::Int32 processLifecycleType;
		Bmco::Int32 processBusinessType;
		Bmco::Int32  bolBpcbID;
		std::string  exeName;
		std::string  exepath;
		std::string  libpath;
		std::string  rootpath;
		std::string  staticParams;
		std::string  dynamicParams;
		std::string  displayName;
		Bmco::Int64 tlocal;
		Bmco::Int32  priority;
		Bmco::Int64 ramlimit;
		Bmco::Int32  cpulimit;
		

		prodefmtx.getFieldValue(strMaxInstance, 0).getInt32(maxInstance);
		prodefmtx.getFieldValue(strMinInstance, 0).getInt32(minInstance);
		prodefmtx.getFieldValue(strProcessType, 0).getInt32(processType);
		prodefmtx.getFieldValue(strProcessLifecycleType, 0).getInt32(processLifecycleType);
		prodefmtx.getFieldValue(strProcessBusinessType, 0).getInt32(processBusinessType);
		prodefmtx.getFieldValue(strBolBpcbID, 0).getInt32(bolBpcbID);
		exeName=prodefmtx.getFieldValue(strExeName, 0).getString();
		staticParams=prodefmtx.getFieldValue(strStaticParams, 0).getString();
		dynamicParams=prodefmtx.getFieldValue(strDynamicParams, 0).getString();
		displayName=prodefmtx.getFieldValue(strDisplayName, 0).getString();
		exepath=prodefmtx.getFieldValue(strexe_path, 0).getString();
		libpath=prodefmtx.getFieldValue(strlib_path, 0).getString();
		rootpath=prodefmtx.getFieldValue(strroot_dir, 0).getString();
		prodefmtx.getFieldValue(strTimeStamp, 0).getInt64(tlocal);
		prodefmtx.getFieldValue(strPriority, 0).getInt32(priority);
		prodefmtx.getFieldValue(strRam_limit, 0).getInt64(ramlimit);
		prodefmtx.getFieldValue(strCpu_limit, 0).getInt32(cpulimit);
		
		


		std::string AbsolutePath=Bmco::replace(exepath,
			std::string("${common.vnodepath}"),Bmco::Util::Application::instance().config().getString("common.vnodepath",""));

		std::string reallibpath=Bmco::replace(libpath,
			std::string("${common.vnodepath}"),Bmco::Util::Application::instance().config().getString("common.vnodepath",""));
		
	
		
		MetaProgramDef::Ptr pdPtr = new MetaProgramDef(programID,
			maxInstance,minInstance,processType,processLifecycleType,processBusinessType,
			bolBpcbID,exeName.c_str(),staticParams.c_str(),dynamicParams.c_str(),displayName.c_str(),AbsolutePath.c_str(),
			reallibpath.c_str(),rootpath.c_str(),
			Bmco::Timestamp(tlocal),priority,cpulimit,ramlimit);

		return pdPtr;

		}
		 catch (std::exception& e) 
		 {
		 	bmco_error_f4(theLogger, "%s|%s| getProcessTypeByID proid %?d  catch  exception %s."
				, std::string("0"),std::string(__FUNCTION__),programID,std::string(e.what()));
        		return NULL;
   		}
		catch(...)
		{
			bmco_error_f3(theLogger, "%s|%s| getProcessTypeByID  proid %?d   sqlite exception."
				, std::string("0"),std::string(__FUNCTION__),programID);
			return NULL;
		}
 }






 bool RegularProcessTaskLoop::RegistBpcbInfo(MetaBpcbInfo::Ptr & bpcbptr)
 {
 		try
 		{

		Pair seqpair;
		seqpair.addFieldPair(strBpcbID, bpcbptr->m_iBpcbID);
		seqpair.addFieldPair(strSysPID, bpcbptr->m_iSysPID);
		seqpair.addFieldPair(strProgramID, bpcbptr->m_iProgramID);
		seqpair.addFieldPair(strcreateTime, bpcbptr->m_tCreateTime.epochMicroseconds());
		seqpair.addFieldPair(strHeartBeat, bpcbptr->m_tHeartBeat.epochMicroseconds());
		seqpair.addFieldPair(strStatus, bpcbptr->m_iStatus);
		seqpair.addFieldPair(strFDLimitation, bpcbptr->m_iFDLimitation);
		seqpair.addFieldPair(strFDInUsing, bpcbptr->m_iFDInUsing);
		seqpair.addFieldPair(strCPU, bpcbptr->m_iCPU);
		seqpair.addFieldPair(strRAM, bpcbptr->m_iRAM);
		seqpair.addFieldPair(strFlowID, bpcbptr->m_iFlowID);
		seqpair.addFieldPair(strInstanceID, bpcbptr->m_iInstanceID);
		seqpair.addFieldPair(strTaskSource, bpcbptr->m_iTaskSource);
		seqpair.addFieldPair(strSourceID, bpcbptr->m_iSourceID);
		seqpair.addFieldPair(strSnapShot, bpcbptr->m_iSnapShot);
		seqpair.addFieldPair(strThisTimeStartTime, bpcbptr->m_tThisTimeStartTime.epochMicroseconds());
		seqpair.addFieldPair(strLastTimeStartTime, bpcbptr->m_tLastTimeStartTime.epochMicroseconds());
		seqpair.addFieldPair(strbpcb_type, bpcbptr->m_bpcb_type);
		
		
		FieldNameSet fset;
		fset.insert(strBpcbID);             
		fset.insert(strSysPID);             
		fset.insert(strProgramID);          
		fset.insert(strcreateTime);         
		fset.insert(strHeartBeat);          
		fset.insert(strStatus);             
		fset.insert(strFDLimitation);       
		fset.insert(strFDInUsing);          
		fset.insert(strCPU);                
		fset.insert(strRAM);                
		fset.insert(strFlowID);             
		fset.insert(strInstanceID);         
		fset.insert(strTaskSource);         
		fset.insert(strSourceID);           
		fset.insert(strSnapShot);           
		fset.insert(strAction);             
		fset.insert(strThisTimeStartTime);  
		fset.insert(strLastTimeStartTime);
		fset.insert(strbpcb_type);
		
		
		Matrix matx;
		matx.setFieldNameSet(fset);
		
		matx.addRowValue(seqpair);
		
		if( DataAPI::add(strMetaBpcbInfo, matx)!=0)
		{
			bmco_error_f3(theLogger,"%s|%s|:Failed to execute Insert  bpcbid [%?d]on  meta bpcb",
			std::string("0"),std::string(__FUNCTION__),bpcbptr->m_iBpcbID);
			return false;
		}

		}
		catch(...)
		{
			bmco_error_f2(theLogger, "%s|%s| regist bpcb   sqlite exception."
				, std::string("0"),std::string(__FUNCTION__));
			return NULL;
		}
		

		bmco_information_f1(theLogger,"regist in table  BPCB_ID[%?d]",bpcbptr->m_iBpcbID);
		return true;
 }


 bool RegularProcessTaskLoop::getAllBpcbItem(std::map<Bmco::Int32,MetaBpcbInfo::Ptr>& bpcbmap)
 {

	try
	{

		Matrix bpcbmtx;
		if(!MemTableOper::instance().QueryTableAllData( strMetaBpcbInfo, bpcbmtx))
		{
			bmco_error_f2(theLogger,"%s|%s|Failed to execute get all bpcb item on  metabpcbtable",
			std::string("0"),std::string(__FUNCTION__));
			return false;
		}


		for(int i=0;i<bpcbmtx.rowValueCount();i++)	
		{
			Bmco::Int32 bpcbID;
			Bmco::Int32 SysPID;
			Bmco::Int32 ProgramID;
			Bmco::Int64 CreateTime;
			Bmco::Int64 HeartBeat;
			Bmco::Int32 Status;
			Bmco::Int32 FDLimitation;
			Bmco::Int32 FDInUsing;
			Bmco::Int32 CPU;
			Bmco::Int64 RAM;
			Bmco::Int32 FlowID;
			Bmco::Int32 InstanceID;
			Bmco::Int32 TaskSource;
			Bmco::Int32 SourceID;
			Bmco::Int32 bpcbtype;

			bpcbmtx.getFieldValue(strBpcbID, i).getInt32(bpcbID);
			bpcbmtx.getFieldValue(strSysPID, i).getInt32(SysPID);
			bpcbmtx.getFieldValue(strProgramID, i).getInt32(ProgramID);
			bpcbmtx.getFieldValue(strcreateTime, i).getInt64(CreateTime);
			bpcbmtx.getFieldValue(strHeartBeat, i).getInt64(HeartBeat);
			bpcbmtx.getFieldValue(strStatus, i).getInt32(Status);
			bpcbmtx.getFieldValue(strFDLimitation, i).getInt32(FDLimitation);
			bpcbmtx.getFieldValue(strFDInUsing, i).getInt32(FDInUsing);
			bpcbmtx.getFieldValue(strCPU, i).getInt32(CPU);
			bpcbmtx.getFieldValue(strRAM, i).getInt64(RAM);
			bpcbmtx.getFieldValue(strFlowID, i).getInt32(FlowID);
			bpcbmtx.getFieldValue(strInstanceID, i).getInt32(InstanceID);
			bpcbmtx.getFieldValue(strTaskSource, i).getInt32(TaskSource);
			bpcbmtx.getFieldValue(strSourceID, i).getInt32(SourceID);
			bpcbmtx.getFieldValue(strbpcb_type, i).getInt32(bpcbtype);

			
			MetaBpcbInfo::Ptr pdPtr = new MetaBpcbInfo(bpcbID,
				SysPID,ProgramID,Bmco::Timestamp(CreateTime),Bmco::Timestamp(HeartBeat),
				Status,FDLimitation,FDInUsing,CPU,RAM,FlowID,InstanceID,TaskSource,SourceID,bpcbtype);

			bpcbmap.insert(std::make_pair(bpcbID,pdPtr));
			
		}
		
		return true;

	}
	catch(...)
	{
		bmco_error_f2(theLogger, "%s|%s| getAllBpcbItem   sqlite exception."
			, std::string("0"),std::string(__FUNCTION__));
		return false;
	}
	
	
 }


 bool RegularProcessTaskLoop::getBpcbItemByID(Bmco::Int32 bpcbid,MetaBpcbInfo::Ptr & bpcbptr)
 {
		try
		{
 
		Matrix bpcbmtx;
		if(!MemTableOper::instance().GetBpcbInfobyBpcbID( bpcbid, bpcbmtx))
		{
			bmco_error_f3(theLogger,"%s|%s|bpcbid[%?d]:Failed to execute get bpcb  on  metabpcbtable",
			std::string("0"),std::string(__FUNCTION__),bpcbid);
			return false;
		}

		Bmco::Int32 SysPID;
		Bmco::Int32 ProgramID;
		Bmco::Int64 CreateTime;
		Bmco::Int64 HeartBeat;
		Bmco::Int32 Status;
		Bmco::Int32 FDLimitation;
		Bmco::Int32 FDInUsing;
		Bmco::Int32 CPU;
		Bmco::Int64 RAM;
		Bmco::Int32 FlowID;
		Bmco::Int32 InstanceID;
		Bmco::Int32 TaskSource;
		Bmco::Int32 SourceID;

		bpcbmtx.getFieldValue(strSysPID, 0).getInt32(SysPID);
		bpcbmtx.getFieldValue(strProgramID, 0).getInt32(ProgramID);
		bpcbmtx.getFieldValue(strcreateTime, 0).getInt64(CreateTime);
		bpcbmtx.getFieldValue(strHeartBeat, 0).getInt64(HeartBeat);
		bpcbmtx.getFieldValue(strStatus, 0).getInt32(Status);
		bpcbmtx.getFieldValue(strFDLimitation, 0).getInt32(FDLimitation);
		bpcbmtx.getFieldValue(strFDInUsing, 0).getInt32(FDInUsing);
		bpcbmtx.getFieldValue(strCPU, 0).getInt32(CPU);
		bpcbmtx.getFieldValue(strRAM, 0).getInt64(RAM);
		bpcbmtx.getFieldValue(strFlowID, 0).getInt32(FlowID);
		bpcbmtx.getFieldValue(strInstanceID, 0).getInt32(InstanceID);
		bpcbmtx.getFieldValue(strTaskSource, 0).getInt32(TaskSource);
		bpcbmtx.getFieldValue(strSourceID, 0).getInt32(SourceID);
	
		
		MetaBpcbInfo::Ptr pdPtr = new MetaBpcbInfo(bpcbid,
			SysPID,ProgramID,Bmco::Timestamp(CreateTime),Bmco::Timestamp(HeartBeat),
			Status,FDLimitation,FDInUsing,CPU,RAM,FlowID,InstanceID,TaskSource,SourceID);
		
		
		bpcbptr=pdPtr;
		return true;

		}
		catch(...)
		{
			bmco_error_f2(theLogger, "%s|%s| getBpcbItemByID   sqlite exception."
				, std::string("0"),std::string(__FUNCTION__));
			return false;
		}
 }


bool RegularProcessTaskLoop::updateBpcbUsingInfo(Bmco::Int64 iBpcbID)
{

	try{

		Matrix _bpcbmtx;
		if(!MemTableOper::instance().GetBpcbInfobyBpcbID(iBpcbID,_bpcbmtx))
		{
			bmco_error_f3(theLogger,"Failed to execute getBpcbInfoByBpcbID %?d  on MetaShmBpcbInfoTable",
				std::string("0"),std::string(__FUNCTION__),iBpcbID);
			return false;
		}
		Bmco::Int64 syspid;
		_bpcbmtx.getFieldValue(strSysPID,0).getInt64(syspid);
		
		

		Bmco::Int32 cpu = GetProcessCpuUse(syspid);
		if(cpu < 0){
			cpu = 0;
		}

		Bmco::Int64 mem = GetProcessMemUse(syspid);
		if(mem < 0){
			mem = 0;
		}

		if((!MemTableOper::instance().UpdateBpcbCpuUse(iBpcbID,cpu)) 
			||(!MemTableOper::instance().UpdateBpcbMemUse(iBpcbID,mem)))
		{
			throw(Bmco::Exception(Bmco::format("BPCB_ID[%?d]:Failed to update using info on MetaShmBpcbInfoTable",iBpcbID)));
		}

	}catch(Bmco::Exception &e)
	{
		bmco_error_f3(theLogger,"%s|%s|%s",std::string("0"),std::string(__FUNCTION__),e.displayText());
		return false;
	}

	return true;
}




bool RegularProcessTaskLoop::getOneCronChildInfo(int cronid,int programid) 
{

	std::map<int,ManagedChildInfo>::iterator itHandle = m_CronmapProcessHandle.find(cronid);
	if(itHandle != m_CronmapProcessHandle.end())
	{
		bmco_warning_f3(theLogger,"%s|%s| cron TaskInfo[%d]:this crontab child process is exsiting now",
			std::string("0"),std::string(__FUNCTION__),cronid);
		return false;
	}

	ManagedChildInfo childInfo;

	if(childInfo.bpcbItem.isNull())  //before init(),the value is null
	{
		try
		{
			//判断进程是否是BOL进程，BOL进程的BpcbID是预分配的（参数配置的）
			Bmco::Int64 bpcbID = 0;
			if(!childInfo.isBolProcess)
			{
				bpcbID=cronid;
			 }
			
			
			childInfo.bpcbId = bpcbID;
			Bmco::LocalDateTime now;

			childInfo.bpcbItem = new MetaBpcbInfo(bpcbID,0,programid,now.timestamp().epochMicroseconds(),now.timestamp().epochMicroseconds(),
				ST_STOP,0,0,0,0,0,0,BOL_CRONTAB_PROCESS_TASK,cronid,2);
		
		
			if(!RegistBpcbInfo(childInfo.bpcbItem )) //注册到bpcb表(被拉起的程序正在起来后需要更新自己的状态)
			{
				throw(Bmco::Exception("Failed to execute registBpcbInfo on MetaShmBpcbInfoTable"));
			}
			
		}
		catch(Bmco::Exception &err) 
		{
			bmco_error_f3(theLogger,"%s|%s|%s",
				std::string("0"),std::string(__FUNCTION__),err.message());
				childInfo.bpcbItem = 0;
			return false;
		}
		
		m_CronmapProcessHandle.insert(std::pair<int,ManagedChildInfo>(cronid,childInfo));
	}

	return true;
}




bool RegularProcessTaskLoop::getOneChildInfo(MetaRegularProcessTask::Ptr & ptr) 
{

	ProcessPrimaryKey key(*ptr);
	std::map<ProcessPrimaryKey,ManagedChildInfo >::iterator itHandle = mapProcessHandle.find(key);
	if(itHandle != mapProcessHandle.end())
	{
		bmco_error_f3(theLogger,"%s|%s|TaskInfo[%s]:this child process is exsiting now",
			std::string("0"),std::string(__FUNCTION__),key.Print());
		return false;
	}

	ManagedChildInfo childInfo;

	childInfo.expSeconds = m_lexpSeconds;
	childInfo.programDef = getProcessTypeByID(ptr->ProgramID);
	
	

	if(childInfo.programDef->ProcessType == BOL_PROGRAM_TYPE_A) //如果是BOL程序
	{
		childInfo.isBolProcess = true;
		childInfo.bpcbId = childInfo.programDef->BolBpcbID;
	}

	if(childInfo.bpcbItem.isNull())  //before init(),the value is null
	{
		try
		{
			//判断进程是否是BOL进程，BOL进程的BpcbID是预分配的（参数配置的）
			Bmco::Int64 bpcbID = 0;
			
			
			if(!childInfo.isBolProcess)
			{

				bpcbID=ptr->ID;
				/*
			
				//其它进程的BpcbID是通过序列号动态生成的
				if(!MemTableOper::instance().IsExistSequence(UNIQUE_SEQUENCE_BPCB_INFO)) //假如该序列号不存在
				{
					Pair seqpair;
					seqpair.addFieldPair(strsequenceName, UNIQUE_SEQUENCE_BPCB_INFO);
					seqpair.addFieldPair(strsequenceValue, 1001);
					seqpair.addFieldPair(strupStep, 1);
					seqpair.addFieldPair(strsequenceType, 1);
					seqpair.addFieldPair(strminValue, 1001);
					seqpair.addFieldPair(strmaxValue, 9999999999999);

					FieldNameSet fset;
					fset.insert(strsequenceValue);
					fset.insert(strsequenceName);
					fset.insert(strlockName);
					fset.insert(strminValue);
					fset.insert(strmaxValue);
					fset.insert(strupStep);
					fset.insert(strsequenceType);
					fset.insert(strupdateTime);
				
					
					Matrix seqmatx;
					seqmatx.setFieldNameSet(fset);
					seqmatx.addRowValue(seqpair);
					
					if( DataAPI::add(strMetaUniqueSequenceID, seqmatx) !=0)
					{
						bmco_error_f2(theLogger,"%s|%s|:Failed to execute Insert on  MetaShmUniqueSequenceIDTable",
						std::string("0"),std::string(__FUNCTION__));
						throw(Bmco::Exception("Failed to execute Insert on  MetaShmUniqueSequenceIDTable"));
					}
					
				}

				if(!MemTableOper::instance().GetNextSequenceId(UNIQUE_SEQUENCE_BPCB_INFO,bpcbID))
					throw(Bmco::Exception("Failed to execute getNextID on  MetaShmUniqueSequenceIDTable"));

					*/
			 }
			else
			 {
				bpcbID = childInfo.bpcbId;
			 }
			
			childInfo.bpcbId = bpcbID;
			Bmco::LocalDateTime now;

			
			if(m_bolstartBpcbset.count(bpcbID) > 0)
			{
				childInfo.bpcbItem = new MetaBpcbInfo(bpcbID,0,ptr->ProgramID,now.timestamp().epochMicroseconds(),now.timestamp().epochMicroseconds(),
					ST_STOP,0,0,0,0,ptr->FlowID,ptr->InstanceID,AUTOGENNERATE_PROCESS_TASK,bpcbID);
			}
			else
			{
				childInfo.bpcbItem = new MetaBpcbInfo(bpcbID,0,ptr->ProgramID,now.timestamp().epochMicroseconds(),now.timestamp().epochMicroseconds(),
					ST_STOP,0,0,0,0,ptr->FlowID,ptr->InstanceID,REGULAR_PROCESS_TASK,ptr->ID);
			}
			
			if(!RegistBpcbInfo(childInfo.bpcbItem )) //注册到bpcb表(被拉起的程序正在起来后需要更新自己的状态)
			{
				throw(Bmco::Exception("Failed to execute registBpcbInfo on MetaShmBpcbInfoTable"));
			}
			
		}
		catch(Bmco::Exception &err) 
		{
			bmco_error_f3(theLogger,"%s|%s|%s",
				std::string("0"),std::string(__FUNCTION__),err.message());
				childInfo.bpcbItem = 0;
			return false;
		}
		

		childInfo.isNoMonitor = ptr->IsNoMonitor;
		childInfo.exCommand = ptr->ExCommand.c_str();
		childInfo.taskvalid = ptr->IsValid;
		childInfo.is_warning = ptr->is_warning;
		
		mapProcessHandle.insert(std::pair<ProcessPrimaryKey,ManagedChildInfo>(key,childInfo));
	}

	return true;
}



bool ExecuteShellResult(const std::string& cmd,std::string &result)
{

	#if BMCO_OS == BMCO_OS_WINDOWS_NT
	
	
	#elif BMCO_OS == BMCO_OS_LINUX || BMCO_OS_FAMILY_UNIX == 1

	FILE *fp = popen (cmd.c_str(), "r");
	if (fp == NULL)
		return false;

	char results[64];
	if (fscanf (fp, "%s\n", results) == EOF)
	{
		pclose(fp);
		return false;
	}

	pclose(fp);
	result=results;
	return true;

	#endif

	//default
	return false;
	
}


bool RegularProcessTaskLoop::RestetBpcb(MetaRegularProcessTask::Ptr& ptr) 
{

	ProcessPrimaryKey key(*ptr);
	std::map<ProcessPrimaryKey,ManagedChildInfo >::iterator itHandle = mapProcessHandle.find(key);
	if(itHandle == mapProcessHandle.end())
		return false;

	return MemTableOper::instance().UpdateBpcbAction((itHandle->second).bpcbId,0);
	
}



bool RegularProcessTaskLoop::launchOneChild(MetaRegularProcessTask::Ptr& ptr) 
{

	ProcessPrimaryKey key(*ptr);
	std::map<ProcessPrimaryKey,ManagedChildInfo >::iterator itHandle = mapProcessHandle.find(key);
	if(itHandle == mapProcessHandle.end())
		return false;

	if(ptr->IsValid != CONTROL_START)
	{
		ERROR_LOG(0, "no need to start the program proid [%d]",ptr->ProgramID);
		return false;
	}


	//启动前加防止异常的保护
	 
	if (!getBpcbItemByID((itHandle->second).bpcbId, (itHandle->second).bpcbItem)) 
	{
		bmco_error_f3(theLogger,"%s|%s|BPCB_ID[%?d]:failed to query  bpcb item",
					std::string("0"), std::string(__FUNCTION__),(itHandle->second).bpcbId);
		return false;
	}
	
	
	
	try
	{	//第一次启动，必然都是0
		if(((itHandle->second).bpcbItem->m_iSysPID != INVALID_SYS_PID))
		{
			if((-1 == kill((itHandle->second).bpcbItem->m_iSysPID,0)))
			{
				
				bmco_warning_f3(theLogger,"%s|%s|BPCB_ID[%?d]:process pid not exist,we can launch it.",
					std::string("0"),std::string(__FUNCTION__), (itHandle->second).bpcbId);
			}
			else
			{
				bmco_warning_f3(theLogger,"%s|%s|BPCB_ID[%?d]:process pid exist  ,process not stop ,we can not launch it.",
					std::string("0"),std::string(__FUNCTION__), (itHandle->second).bpcbId);
				return false;
			}
			
		}
	}
	catch(...)
	{
		bmco_warning_f3(theLogger,"%s|%s|BPCB_ID[%?d]:kill 0  catch exception ,not launch.",
					std::string("0"),std::string(__FUNCTION__), (itHandle->second).bpcbId);
		return false;
	}




	

	Process::Args args; //store cmd line args
	Bmco::StringTokenizer st((itHandle->second).programDef->StaticParams.c_str(), " ", Bmco::StringTokenizer::TOK_TRIM);
	for (Bmco::StringTokenizer::Iterator it = st.begin(); it!=st.end(); it++) 
	{
		std::string realarg="";
#ifndef BMCO_OS_FAMILY_WINDOWS // on windows, do not support --daemon option
		if((*it).find("common.vnodepath") != std::string::npos)
		{
			realarg=Bmco::replace((*it),std::string("${common.vnodepath}"),m_vnodepath);
		}
		else if((*it).find("common.logpath") != std::string::npos)
		{
			realarg=Bmco::replace((*it),std::string("${common.logpath}"),Bmco::Util::Application::instance().config().getString("common.logpath",""));
		}
		else if((*it).find("common.binpath") != std::string::npos)
		{
			realarg=Bmco::replace((*it),std::string("${common.binpath}"),
				Bmco::Util::Application::instance().config().getString("common.binpath",""));
		}
		else if((*it).find("common.datapath") != std::string::npos)
		{
			realarg=Bmco::replace((*it),std::string("${common.datapath}"),
				Bmco::Util::Application::instance().config().getString("common.datapath",""));
		}
		else if((*it).find("${FlowID}") != std::string::npos)
		{
			realarg=Bmco::replace((*it),std::string("${FlowID}"),
				Bmco::format("%?d",ptr->FlowID));
		}
		else if((*it).find("${InstanceID}") != std::string::npos)
		{
			realarg=Bmco::replace((*it),std::string("${InstanceID}"),
				Bmco::format("%?d",ptr->InstanceID));
		}
		else if((*it).find("${BpcbID}") != std::string::npos)
		{
			realarg=Bmco::replace((*it),std::string("${BpcbID}"),
				Bmco::format("%?d",(itHandle->second).bpcbId));
		}
		else if((*it).find("${ProgramID}") != std::string::npos)
		{
			realarg=Bmco::replace((*it),std::string("${ProgramID}"),
				Bmco::format("%?d",ptr->ProgramID));
		}
		else if((*it).find("${BolName}") != std::string::npos)
		{
			realarg=Bmco::replace((*it),std::string("${BolName}"),
				Bmco::Util::Application::instance().config().getString("info.bol_name",""));
		}
		else if((*it).find("${BolID}") != std::string::npos)
		{
			realarg=Bmco::replace((*it),std::string("${BolID}"),
				Bmco::Util::Application::instance().config().getString("info.bol_id",""));
		}
		else if((*it).find("${OrgID}") != std::string::npos)
		{
			realarg=Bmco::replace((*it),std::string("${OrgID}"),
				ptr->OrgID);
		}
		else if((*it).find("${DICT_MACRO_") != std::string::npos)
		{
			
			std::string::size_type posbegin = (*it).find("${DICT_MACRO_");
			
			if(posbegin  != std::string::npos)
			{
				std::string::size_type pos=posbegin+strlen("${DICT_MACRO_");
				std::string::size_type posend = (*it).find('}');
				std::string keystr=(*it).substr(pos,posend-pos);
				INFORMATION_LOG("the dynamic arg is %s ",keystr.c_str());
			
				realarg=MemTableOper::instance().GetDictDynamicParam(keystr);
			}
		}
		else
		{
			realarg=*it;
		}
		
			
#endif
		args.push_back(realarg);
	}




	//对实例动态参数替换
	Bmco::StringTokenizer st2(ptr->dynamic_param," ",Bmco::StringTokenizer::TOK_TRIM);
	for (Bmco::StringTokenizer::Iterator it1 = st2.begin();it1!=st2.end();it1++)
	{

		std::string realarg="";
		std::string::size_type posbegin = (*it1).find("${DICT_MACRO_");
		
		if(posbegin  != std::string::npos)
		{
			std::string::size_type pos=posbegin+strlen("${DICT_MACRO_");
			std::string::size_type posend = (*it1).find('}');
			std::string keystr=(*it1).substr(pos,posend-pos);
			INFORMATION_LOG("the dynamic arg is %s ",keystr.c_str());
		
			realarg=MemTableOper::instance().GetDictDynamicParam(keystr);
		}
		else
		{
			realarg=*it1;
		}
		
		args.push_back(realarg);


	}



	//启动进程之前，判断是否存在bpcbid一样的进程

	if(Bmco::Util::Application::instance().config().getBool("kernel.isJudgeSingle",false))
	{
		std::string sCommand=
			Bmco::format("ps -ef |grep ""DBpcbID=%?d""  |grep -w \"\\-DBolName=%s\" |grep ""DInstanceID=%?d"" |grep ""DFlowID=%?d"" |grep ""DProgramID=%?d"" |grep -v ""grep"" |wc -l",
				(itHandle->second).bpcbId,Bmco::Util::Application::instance().config().getString("info.bol_name",""),
				ptr->InstanceID,ptr->FlowID,ptr->ProgramID);
		std::string result;
		if(ExecuteShellResult(sCommand,result))
		{
			WARNING_LOG(0,"exe %s result %s",sCommand.c_str(),result.c_str());
			if(result != "0")
			{
				WARNING_LOG(0,"bpcbid %d already exist",(itHandle->second).bpcbId);
				return true;
			}
		}
	}
	
	
	
	
	
	Bmco::Path path = Bmco::Path::forDirectory((itHandle->second).programDef->AbsolutePath);

	path.resolve((itHandle->second).programDef->ExeName.c_str());
	
	Bmco::File binfile(path.toString());
	if(!binfile.exists())
	{
		ERROR_LOG(0, "no such file [%s]",path.toString().c_str());
		return false;
	}

	// launch the process
	(itHandle->second).launchTime.update();

	
	if(((itHandle->second).isBolProcess == false) && ((itHandle->second).programDef->DynamicParams.size() >0 ))
	{
		Bmco::StringTokenizer st1((itHandle->second).programDef->DynamicParams.c_str()
			," ",Bmco::StringTokenizer::TOK_TRIM);
		
		for(Bmco::StringTokenizer::Iterator it1 = st1.begin(); it1!=st1.end(); it1++) {
			args.push_back(*it1);
		}
	}

	

	//exCommand for process
	if(((itHandle->second).isBolProcess == false) && (ptr->ExCommand.size() >0 ))
	{
		Bmco::StringTokenizer st2(std::string(ptr->ExCommand.c_str())
									," ",Bmco::StringTokenizer::TOK_TRIM);
		
		for(Bmco::StringTokenizer::Iterator it2 = st2.begin(); it2!=st2.end(); it2++) 
		{
			args.push_back(*it2);
		}
	}


	ScopedEnvSetter envsetter(m_ldenv,(itHandle->second).programDef->libpath);

	
	try {

		std::string strargs="";
		for (Bmco::Process::Args::iterator it = args.begin(); it!=args.end(); it++) 
		{	
			strargs+=*it+" ";
		}

		WARNING_LOG(0,"launch bin:%s,args is <%s>",path.toString().c_str(),strargs.c_str());

		MemTableOper::instance().heartbeat((itHandle->second).bpcbId);

		//更新启动时间
		MemTableOper::instance().UpdateBpcbCreateTime((itHandle->second).bpcbId);

		RestetBpcb(ptr);

		//增加环境变量
		std::string realpath=Bmco::replace((itHandle->second).programDef->libpath,
		std::string("${common.vnodepath}"),Bmco::Util::Application::instance().config().getString("common.vnodepath",""));
		Process::Env env;
		env[LDENVNAME] = realpath+":"+m_ldenv;
		
		Process::launch(path.toString(), args,0,0,0,env);
	
		bmco_information_f4(theLogger,"%s|%s|BPCB_ID[%?d]:launched child process [%s] successfully.",
			std::string("0"),std::string(__FUNCTION__), (itHandle->second).bpcbId,path.toString());

		
	}
	catch (Bmco::Exception &err) 
	{
		bmco_error_f3(theLogger,"%s|%s|%s",
			std::string("0"),std::string(__FUNCTION__),err.message());
		return false;
	}
	catch(...) 
	{
		bmco_error_f2(theLogger,"%s|%s|unknown exception occured when launch child process.",
			std::string("0"),std::string(__FUNCTION__));
		return false;
	}

	return true;
}


bool RegularProcessTaskLoop::IsProcessHasStoped(MetaRegularProcessTask::Ptr ptr)
{

	ProcessPrimaryKey key(*ptr);
	 std::map<ProcessPrimaryKey,ManagedChildInfo >::iterator itHandle = mapProcessHandle.find(key);
	 if(itHandle == mapProcessHandle.end())
	 {
	 	bmco_error_f3(theLogger,"%s|%s|find handle error %s",
					std::string("0"), std::string(__FUNCTION__),key.Print());
	 	return false;
	 }
		
	 
	if (!getBpcbItemByID((itHandle->second).bpcbId, (itHandle->second).bpcbItem)) 
	{
		bmco_error_f3(theLogger,"%s|%s|BPCB_ID[%?d]:failed to query  bpcb item",
					std::string("0"), std::string(__FUNCTION__),(itHandle->second).bpcbId);
		return false;
	}
	
	if(((itHandle->second).bpcbItem->m_iSysPID == INVALID_SYS_PID))
	{

		bmco_warning_f3(theLogger,"%s|%s|BPCB_ID[%?d]:its pid is  INVALID_SYS_PID, we think it has stoped!",
			std::string("0"),std::string(__FUNCTION__),(itHandle->second).bpcbId);
		return true;
	}


	#if BMCO_OS == BMCO_OS_LINUX || BMCO_OS_FAMILY_UNIX == 1
	if((-1 == kill((itHandle->second).bpcbItem->m_iSysPID,0)))
	{
		
		bmco_warning_f3(theLogger,"%s|%s|BPCB_ID[%?d]:process pid not exist in system ,we think it has stoped.",
			std::string("0"),std::string(__FUNCTION__), (itHandle->second).bpcbId);
		return true;
	}
	#endif
	
	
	
}


bool RegularProcessTaskLoop::terminateOneChild(MetaRegularProcessTask::Ptr ptr)
{
	try
	{

		bmco_warning_f3(theLogger,"%s|%s|proid[%?d]:begin to cancel the process.",
				std::string("0"),std::string(__FUNCTION__), ptr->ProgramID);
	
		 ProcessPrimaryKey key(*ptr);
		 std::map<ProcessPrimaryKey,ManagedChildInfo >::iterator itHandle = mapProcessHandle.find(key);
		 if(itHandle == mapProcessHandle.end())
		 {
		 	bmco_error_f3(theLogger,"%s|%s|find handle error %s",
						std::string("0"), std::string(__FUNCTION__),key.Print());
		 	return false;
		 }
			
		 
		if (!getBpcbItemByID((itHandle->second).bpcbId, (itHandle->second).bpcbItem)) 
		{
			bmco_error_f3(theLogger,"%s|%s|BPCB_ID[%?d]:failed to query  bpcb item",
						std::string("0"), std::string(__FUNCTION__),(itHandle->second).bpcbId);
			return false;
		}
		
		if(((itHandle->second).bpcbItem->m_iSysPID == INVALID_SYS_PID))
		{

			bmco_warning_f3(theLogger,"%s|%s|BPCB_ID[%?d]:its pid is  INVALID_SYS_PID, can not terminate proces ,do not start process",
				std::string("0"),std::string(__FUNCTION__),(itHandle->second).bpcbId);
			return false;
		}


		#if BMCO_OS == BMCO_OS_LINUX || BMCO_OS_FAMILY_UNIX == 1
		if((-1 == kill((itHandle->second).bpcbItem->m_iSysPID,0)))
		{
			
			bmco_warning_f3(theLogger,"%s|%s|BPCB_ID[%?d]:process pid not exist,no need to kill.",
				std::string("0"),std::string(__FUNCTION__), (itHandle->second).bpcbId);

			if((MemTableOper::instance().UpdateBpcbCpuUse(( itHandle->second).bpcbId,0)) 
				&&(MemTableOper::instance().UpdateBpcbMemUse(( itHandle->second).bpcbId,0))
				&&(MemTableOper::instance().UpdateBpcbStatusByID((itHandle->second).bpcbId,ST_STOP)))
			{
				bmco_information_f3(theLogger,"%s|%s|%s.",
						std::string("0"),std::string(__FUNCTION__),
						Bmco::format("BPCB_ID[%?d]:update using info on MetaShmBpcbInfoTable success",(itHandle->second).bpcbId));
			}
			
			return true;
		}
		#endif

		
		switch(ptr->IsValid)
		{
			case CONTROL_START:
				break;
			case CONTROL_TERMINATE:
				{
					bmco_warning_f4(theLogger,"%s|%s|BPCB_ID[%?d]:try to terminate child process with pid [%s].",
							std::string("0"),std::string(__FUNCTION__),(itHandle->second).bpcbId, Bmco::NumberFormatter::format((itHandle->second).bpcbItem->m_iSysPID));
					Process::requestTermination((itHandle->second).bpcbItem->m_iSysPID);
		
					//正常停止进程将状态更为停止中
					if(MemTableOper::instance().UpdateBpcbStatusByID((itHandle->second).bpcbId,ST_STOPING))
					{
						bmco_warning_f3(theLogger,"%s|%s|BPCB_ID[%?d]:terminate child , we set it ST_STOPING in BPCP.",
							std::string("0"),std::string(__FUNCTION__),(itHandle->second).bpcbId);
						
					}
				}
				break;
			case CONTROL_KILL:
				{
					bmco_warning_f4(theLogger,"%s|%s|BPCB_ID[%?d]:try to kill child process with pid [%s].",
							std::string("0"),std::string(__FUNCTION__),(itHandle->second).bpcbId, Bmco::NumberFormatter::format((itHandle->second).bpcbItem->m_iSysPID));
					Process::kill((itHandle->second).bpcbItem->m_iSysPID);
					//强杀更新为stop
					if(MemTableOper::instance().UpdateBpcbStatusByID((itHandle->second).bpcbId,ST_STOP))
					{
						bmco_warning_f3(theLogger,"%s|%s|BPCB_ID[%?d]:terminate child , we set it ST_STOP in BPCP.",
							std::string("0"),std::string(__FUNCTION__),(itHandle->second).bpcbId);
						
					}
		
				}
				break;
			default:
				
				break;
		}
		
		

		//等2秒如果能kill,更新字段值，不能则不做处理
		this->sleep(2000);
		#if BMCO_OS == BMCO_OS_LINUX || BMCO_OS_FAMILY_UNIX == 1
		if((-1 == kill((itHandle->second).bpcbItem->m_iSysPID,0)))
		{
			
			bmco_warning_f3(theLogger,"%s|%s|BPCB_ID[%?d]:process cancel successfully.",
				std::string("0"),std::string(__FUNCTION__), (itHandle->second).bpcbId);
		
			if((MemTableOper::instance().UpdateBpcbCpuUse(( itHandle->second).bpcbId,0)) 
				&&(MemTableOper::instance().UpdateBpcbMemUse(( itHandle->second).bpcbId,0))
				&&(MemTableOper::instance().UpdateBpcbStatusByID((itHandle->second).bpcbId,ST_STOP)))
			{
				bmco_information_f3(theLogger,"%s|%s|%s.",
						std::string("0"),std::string(__FUNCTION__),
						Bmco::format("BPCB_ID[%?d]:update using info on MetaShmBpcbInfoTable success",(itHandle->second).bpcbId));
			}

			return true;
		}
		else
		{
			return false;
		}
		
		#endif

	}
	catch (Bmco::Exception &err) 
	{
		bmco_warning_f3(theLogger,"%s|%s|exception occured when terminated child with message [%s]",
			std::string("0"),std::string(__FUNCTION__),err.message());
		//may be process not exist
		return true;
	}
	catch(...) 
	{
		bmco_error_f2(theLogger,"%s|%s|unknown exception occured when terminated child",
			std::string("0"),std::string(__FUNCTION__));
		return false;
	}
	return true;
}






bool RegularProcessTaskLoop::PauseOneChild(Bmco::Int64 iBpcbID)
{
	try
	{

		bmco_warning_f3(theLogger,"%s|%s|bpcbid[%?d]:begin to pause the process.",
				std::string("0"),std::string(__FUNCTION__), iBpcbID);
	

		MetaBpcbInfo::Ptr bpcbprocess= new  MetaBpcbInfo(iBpcbID);
		 
		if (!getBpcbItemByID(iBpcbID, bpcbprocess)) 
		{
			bmco_error_f3(theLogger,"%s|%s|BPCB_ID[%?d]:failed to query  bpcb item",
						std::string("0"), std::string(__FUNCTION__),iBpcbID);
			return false;
		}
		
		if(bpcbprocess->m_iSysPID == INVALID_SYS_PID)
		{

			bmco_warning_f3(theLogger,"%s|%s|BPCB_ID[%?d]:its pid is  INVALID_SYS_PID, can not pause proces ,do nothing",
				std::string("0"),std::string(__FUNCTION__),iBpcbID);
			return false;
		}


		#if BMCO_OS == BMCO_OS_LINUX || BMCO_OS_FAMILY_UNIX == 1
		if(0 == kill(bpcbprocess->m_iSysPID,SIGUSR2))
		{
			
			bmco_warning_f3(theLogger,"%s|%s|BPCB_ID[%?d]:process pid signal success",
				std::string("0"),std::string(__FUNCTION__), iBpcbID);
			
			return true;
		}
		else
		{
			bmco_warning_f3(theLogger,"%s|%s|BPCB_ID[%?d]:process pid signal failed ",
				std::string("0"),std::string(__FUNCTION__), iBpcbID);
			return false;
		}
		#endif


	}
	catch (Bmco::Exception &err) 
	{
		bmco_warning_f3(theLogger,"%s|%s|exception occured when pause child with message [%s]",
			std::string("0"),std::string(__FUNCTION__),err.message());
		//may be process not existpause
		return true;
	}
	catch(...) 
	{
		bmco_error_f2(theLogger,"%s|%s|unknown exception occured when pause child",
			std::string("0"),std::string(__FUNCTION__));
		return false;
	}
	return true;
}






bool RegularProcessTaskLoop::setBolStatus(BolStatus _s) 
{
	
	if (!MemTableOper::instance().SetBolStatus(_s)) {
		bmco_error_f2(theLogger,"%s|%s|failed to update bol status.",
			std::string("0"),std::string(__FUNCTION__));
		return false;
	}
	bmco_information_f2(theLogger,"%s|%s|update bol status successfully.",
			std::string("0"),std::string(__FUNCTION__));
	return true;
}



bool RegularProcessTaskLoop::terminateAllChild()
{
	//先停止hla进程
	 for( std::map<ProcessPrimaryKey,ManagedChildInfo >::iterator it = mapProcessHandle.begin();
	 		it != mapProcessHandle.end();it++)
	 {
		 try
		 {
			//优先停止hla进程
			if((it->second).programDef->ProcessType == BOL_PROGRAM_TYPE_A)
				continue;
			
			 MetaRegularProcessTask::Ptr ptr = new 
				 MetaRegularProcessTask(0,it->first.m_iProgramID,it->first.m_iInstanceID,it->first.m_iFlowID,
				CONTROL_TERMINATE,0,false,0,"",false);
			 terminateOneChild(ptr);

		 }
		 catch(Bmco::Exception &e)
		 {
			 bmco_error_f3(theLogger,"%s|%s|%s",std::string("0"),std::string(__FUNCTION__),e.displayText());
	        }
	 }

	//等待hla进程都停止下来
	for( std::map<ProcessPrimaryKey,ManagedChildInfo >::iterator it = mapProcessHandle.begin();
 		it != mapProcessHandle.end();it++)
	 {
		 try
		 {
			if((it->second).programDef->ProcessType == BOL_PROGRAM_TYPE_A)
				continue;
			
			 MetaRegularProcessTask::Ptr ptr = new 
				 MetaRegularProcessTask(0,it->first.m_iProgramID,it->first.m_iInstanceID,it->first.m_iFlowID,
				CONTROL_TERMINATE,0,false,0,"",false);

			for(int count=0;count<60;count++)
			{	
				if(IsProcessHasStoped(ptr))
				{
					bmco_information_f4(theLogger,"%s|%s|ProgramID[%?d] exe[%s] has stoped!",
					 		std::string("0"),std::string(__FUNCTION__),
					 		(it->second).programDef->ID,(it->second).programDef->ExeName);
					break;
				}
				else
				{
					 bmco_error_f4(theLogger,"%s|%s|ProgramID[%?d] exe[%s] can not stop!",
					 		std::string("0"),std::string(__FUNCTION__),
					 		(it->second).programDef->ID,(it->second).programDef->ExeName);
					Bmco::Thread::sleep(2000);
				}
			}
			 
		 }
		 catch(Bmco::Exception &e)
		 {
			 bmco_error_f3(theLogger,"%s|%s|%s",std::string("0"),std::string(__FUNCTION__),e.displayText());
	        }
	
	}
	

	
	
	

	//停止除了cloudagent的bol进程
	for( std::map<ProcessPrimaryKey,ManagedChildInfo >::iterator it = mapProcessHandle.begin();
	 		it != mapProcessHandle.end();it++)
	 {
		 try
		 {
			if((it->second).programDef->ProcessType != BOL_PROGRAM_TYPE_A)
				continue;

			if((it->second).bpcbId == BOL_PROCESS_CLOUDAGENT)
				continue;
			
			 MetaRegularProcessTask::Ptr ptr = new 
				 MetaRegularProcessTask(0,it->first.m_iProgramID,it->first.m_iInstanceID,it->first.m_iFlowID,
				CONTROL_TERMINATE,0,false,0,"",false);
			 terminateOneChild(ptr);
		 }
		 catch(Bmco::Exception &e)
		 {
			 bmco_error_f3(theLogger,"%s|%s|%s",std::string("0"),std::string(__FUNCTION__),e.displayText());
	        }
	 }

	
	//等待除了cloudagent的bol进程都停止下来
	for( std::map<ProcessPrimaryKey,ManagedChildInfo >::iterator it = mapProcessHandle.begin();
	 		it != mapProcessHandle.end();it++)
	 {
		 try
		 {
			if((it->second).programDef->ProcessType != BOL_PROGRAM_TYPE_A)
				continue;

			if((it->second).bpcbId == BOL_PROCESS_CLOUDAGENT)
				continue;
			
			 MetaRegularProcessTask::Ptr ptr = new 
				 MetaRegularProcessTask(0,it->first.m_iProgramID,it->first.m_iInstanceID,it->first.m_iFlowID,
				CONTROL_TERMINATE,0,false,0,"",false);
			 
			while(true)
			{	
				if(IsProcessHasStoped(ptr))
				{
					bmco_information_f4(theLogger,"%s|%s|ProgramID[%?d] exe[%s] has stoped!",
					 		std::string("0"),std::string(__FUNCTION__),
					 		(it->second).programDef->ID,(it->second).programDef->ExeName);
					break;
				}
				else
				{
					 bmco_error_f4(theLogger,"%s|%s|ProgramID[%?d] exe[%s] can not stop!",
					 		std::string("0"),std::string(__FUNCTION__),
					 		(it->second).programDef->ID,(it->second).programDef->ExeName);
					  Bmco::Thread::sleep(2000);
				}
			}
			
		
		 }
		 catch(Bmco::Exception &e)
		 {
			 bmco_error_f3(theLogger,"%s|%s|%s",std::string("0"),std::string(__FUNCTION__),e.displayText());
	        }
	 }


	setBolStatus(BOL_OFFLINE);

	//停止cloudagent
	for( std::map<ProcessPrimaryKey,ManagedChildInfo >::iterator it = mapProcessHandle.begin();
	 		it != mapProcessHandle.end();it++)
	 {
		 try
		 {
			if((it->second).programDef->ProcessType != BOL_PROGRAM_TYPE_A)
				continue;

			if((it->second).bpcbId == BOL_PROCESS_CLOUDAGENT)
			{
				MetaRegularProcessTask::Ptr ptr = new 
				 MetaRegularProcessTask(0,it->first.m_iProgramID,it->first.m_iInstanceID,it->first.m_iFlowID,
				CONTROL_TERMINATE,0,false,0,"",false);
			 	terminateOneChild(ptr);
			}
				 
		 }
		 catch(Bmco::Exception &e)
		 {
			 bmco_error_f3(theLogger,"%s|%s|%s",std::string("0"),std::string(__FUNCTION__),e.displayText());
	        }
	 }
	

	//等待除了cloudagent的bol进程都停止下来
	for( std::map<ProcessPrimaryKey,ManagedChildInfo >::iterator it = mapProcessHandle.begin();
	 		it != mapProcessHandle.end();it++)
	 {
		 try
		 {
			if((it->second).programDef->ProcessType != BOL_PROGRAM_TYPE_A)
				continue;

			if((it->second).bpcbId == BOL_PROCESS_CLOUDAGENT)
			{
				
				 MetaRegularProcessTask::Ptr ptr = new 
					 MetaRegularProcessTask(0,it->first.m_iProgramID,it->first.m_iInstanceID,it->first.m_iFlowID,
					CONTROL_TERMINATE,0,false,0,"",false);
				 
				while(true)
				{	
					if(IsProcessHasStoped(ptr))
					{
						bmco_information_f4(theLogger,"%s|%s|ProgramID[%?d] exe[%s] has stoped!",
						 		std::string("0"),std::string(__FUNCTION__),
						 		(it->second).programDef->ID,(it->second).programDef->ExeName);
						break;
					}
					else
					{
						 bmco_error_f4(theLogger,"%s|%s|ProgramID[%?d] exe[%s] can not stop!",
						 		std::string("0"),std::string(__FUNCTION__),
						 		(it->second).programDef->ID,(it->second).programDef->ExeName);
						  Bmco::Thread::sleep(2000);
					}
				}
				

			}
			
		
		 }
		 catch(Bmco::Exception &e)
		 {
			 bmco_error_f3(theLogger,"%s|%s|%s",std::string("0"),std::string(__FUNCTION__),e.displayText());
	        }
	 }

	 

	 mapProcessHandle.clear();
	 return true;
}



bool RegularProcessTaskLoop::respondlParamterRefresh()
{

	if(! programeDefRefresh())
	{
		bmco_error_f2(theLogger,"%s|%s|Failed to execute programeDefRefresh.",
			std::string("0"),std::string(__FUNCTION__));
	}

	if(! regularProcessRefresh())
	{
		bmco_error_f2(theLogger,"%s|%s|Failed to execute regularProcessRefresh.",
			std::string("0"),std::string(__FUNCTION__));
		return false;
	}
	
	return true;
}




}

