#include "TimeTaskLoop.h"
#include "Bmco/DateTimeFormatter.h"
#include "Bmco/DateTimeFormat.h"
#include "RegularProcessTaskLoop.h"
#include "Bmco/Timespan.h"
#include "Bmco/DateTimeParser.h"
#include "Bmco/StringTokenizer.h"
#include "Bmco/String.h"
#include "MemTableOp.h"
#include "Bmco/NumberParser.h"
#include "Bmco/Environment.h"

using Bmco::DateTimeFormatter;
using Bmco::DateTimeFormat;


namespace BM35 {


void TimeTaskLoop::runTask() 
{

	try
	{
		if (!beforeTask()) {
			Process::requestTermination(Process::id());
			return;
		}

		while (!isCancelled()) 
		{
			try
			{
				handleOneUnit();
			}
			catch(...)
			{
				bmco_error_f2(theLogger, "%s|%s| one unit unknown exception."
					, std::string("0"),std::string(__FUNCTION__));
			}
		}

		if (!afterTask()) return;
	}
	catch(...)
	{
		bmco_error_f2(theLogger, "%s|%s| done time task loop unknown exception."
			, std::string("0"),std::string(__FUNCTION__));
		return;
	}
	bmco_information_f2(theLogger, "%s|%s| done time task loop gracefully."
		, std::string("0"),std::string(__FUNCTION__));
}


	
bool TimeTaskLoop::processTimeTask()
{

	Bmco::LocalDateTime comparenow;
	Matrix matx;
	if(!MemTableOper::instance().QueryTableAllData(strMetaBolCrontab,matx)) //查询所有的定时任务
	{
		bmco_error_f2(theLogger,"%s|%s|Failed to execute QueryAll on MetaShmBolCrontabTable",
			std::string("0"),std::string(__FUNCTION__));
		return false;
	}

	for(int i=0;i<matx.rowValueCount();i++)	//遍历处理定时任务
	{

		int isvalid=0;
		matx.getFieldValue(strIsValid,i).getInt32(isvalid);
				
		if(isvalid == 0)  //任务失效不需要处理
			continue;

		std::string strNextexeTime=matx.getFieldValue(strNextExecuteTime,i).getString();
		Bmco::Int64 iNextExecuteTime=0;
	
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

		Bmco::Int32 iprogramid=0;
		matx.getFieldValue(strProgramID,i).getInt32(iprogramid);


		Bmco::Int32 icronid=0;
		matx.getFieldValue(strID,i).getInt32(icronid);

		
		Matrix prodefmatx;
		if(!MemTableOper::instance().GetProgramDefbyId(iprogramid,prodefmatx))
		{
			bmco_error_f3(theLogger,"%s|%s|TaskID[%?d]:Failed to execute queryProgramDefByID on  MetaShmProgramDefTable",
			std::string("0"),std::string(__FUNCTION__),icronid);
			continue;
		}

		bool isHaving = false; //BPCB里面是否有任务调度里面的任务
		

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
			else if((*it1).find("${BolID}") != std::string::npos)
			{
				realarg=Bmco::replace((*it1),std::string("${BolID}"),
					Bmco::Util::Application::instance().config().getString("info.bol_id",""));
			}
			else
			{
				realarg=*it1;
			}
			
			args.push_back(realarg);

	
		}


		int cyclecount=0;

		//更新下次执行的时间
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

			std::string sDate = Bmco::DateTimeFormatter::format(nextExecDate, "%Y%m%d%H%M%S");
			for(int i=0;i<200;i++)
			{
				if(!MemTableOper::instance().UpdateCronNextExeTime(icronid,nextExecDate.timestamp().epochMicroseconds()))
				{
					bmco_error_f3(theLogger,"%s|%s|TaskID[%?d]:update next exe time failed",
						std::string("0"),std::string(__FUNCTION__),icronid);
				}
				else
				{
					bmco_information_f4(theLogger,"%s|%s|TaskID[%?d]:update next exetime[%s]!",
						std::string("0"),std::string(__FUNCTION__),icronid,sDate);
					break;
				}
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

		}
		catch(Bmco::Exception &e)
		{
			bmco_error_f3(theLogger,"%s|%s|%s",std::string("0"),std::string(__FUNCTION__),e.displayText());
			continue;
		}

		
		
	}

	return true;
}

bool TimeTaskLoop::updateBpcbUsingInfo(Bmco::UInt64 iBpcbID)
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

}

