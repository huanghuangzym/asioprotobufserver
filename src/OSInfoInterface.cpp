#include "OSInfoInterface.h"
#include "Bmco/Environment.h"
#include "Bmco/Event.h"

#if BMCO_OS == BMCO_OS_WINDOWS_NT
#include <Windows.h>
#include <psapi.h>

#elif BMCO_OS == BMCO_OS_LINUX
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#ifndef __CYGWIN__
#include <net/if_arp.h>
#else // workaround for Cygwin, which does not have if_arp.h 
#define ARPHRD_ETHER 1 /* Ethernet 10Mbps */
#endif 
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdio>
#include <sys/sysinfo.h>


#endif

#include "Bmco/Format.h"
#include "Bmco/Thread.h"


namespace BM35{


	const static Bmco::Int64 SYSERROR=-1;

	Bmco::DateTime GetOsBootTimeUtc()
	{
		Bmco::UInt64 tosrun;
		Bmco::Timespan tspan;
		Bmco::DateTime tnow;
		Bmco::DateTime tosboot;
		#if BMCO_OS == BMCO_OS_WINDOWS_NT

		tosrun=GetTickCount64();
		tspan=tosrun/1000000;

		#elif BMCO_OS == BMCO_OS_LINUX

		struct sysinfo s_info;
		long uptime = 0;
		if(0==sysinfo(&s_info))
		{
			tspan = s_info.uptime;
		}   

		#endif
		
		tosboot=tnow-tspan;
		return tosboot;
	}

	//const static Bmco::UInt64 MEMDIV=1024*1024;
	Bmco::Int64 GetOSTotalPhyMemory()
	{
		#if BMCO_OS == BMCO_OS_WINDOWS_NT

		MEMORYSTATUSEX statex;
		statex.dwLength = sizeof (statex);
		GlobalMemoryStatusEx (&statex);
		return statex.ullTotalPhys;

		#elif BMCO_OS == BMCO_OS_LINUX

		struct sysinfo s_info;
		if(0==sysinfo(&s_info))
		{
			return s_info.totalram;
		}

		#endif

		return SYSERROR;
	}

	Bmco::Int64 GetOSFreePhyMemory()
	{
		#if BMCO_OS == BMCO_OS_WINDOWS_NT

		MEMORYSTATUSEX statex;
		statex.dwLength = sizeof (statex);
		GlobalMemoryStatusEx (&statex);
		return statex.ullAvailPhys;

		#elif BMCO_OS == BMCO_OS_LINUX

		struct sysinfo s_info;
		if(0==sysinfo(&s_info))
		{
			return s_info.freeram;
		}   
		
		#endif

		return SYSERROR;
	}

	#if BMCO_OS == BMCO_OS_WINDOWS_NT
	static Bmco::Int64 CompareFileTime(FILETIME time1, FILETIME time2)
	{
		Bmco::UInt64 a = time1.dwHighDateTime;
		a <<= 32;
		a |= time1.dwLowDateTime;

		Bmco::UInt64 b = time2.dwHighDateTime;
		b <<= 32;
		b |= time2.dwLowDateTime;

		return   (b - a);
	}
	#endif

	#if BMCO_OS == BMCO_OS_LINUX
	
	typedef struct PACKED         //定义一个cpu occupy的结构体
	{
		char name[20];      //
		Bmco::UInt64 user; //
		Bmco::UInt64 nice; //
		Bmco::UInt64 system;//
		Bmco::UInt64 idle; //
	}CPU_OCCUPY;

	void get_cpuoccupy (CPU_OCCUPY *cpust) //对无类型get函数含有一个形参结构体类弄的指针O
	{   
		FILE *fd;               
		char buff[256]; 
		CPU_OCCUPY *cpu_occupy;
		cpu_occupy=cpust;

		fd = fopen ("/proc/stat", "r"); 
		fgets (buff, sizeof(buff), fd);

		sscanf (buff, "%s %u %u %u %u", cpu_occupy->name, &cpu_occupy->user, &cpu_occupy->nice,&cpu_occupy->system, &cpu_occupy->idle);

		fclose(fd);     
	}
	
	double cal_cpuoccupy (CPU_OCCUPY *o, CPU_OCCUPY *n) 
	{   
		Bmco::UInt64 od, nd;    
		Bmco::UInt64 id, sd;
		double cpu_use = 0;   

		od = (Bmco::UInt64) (o->user + o->nice + o->system +o->idle);//第一次(用户+优先级+系统+空闲)的时间再赋给od
		nd = (Bmco::UInt64) (n->user + n->nice + n->system +n->idle);//第二次(用户+优先级+系统+空闲)的时间再赋给od

		id = (Bmco::UInt64) (n->user - o->user);    //用户第一次和第二次的时间之差再赋给id
		sd = (Bmco::UInt64) (n->system - o->system);//系统第一次和第二次的时间之差再赋给sd
		if((nd-od) != 0)
			cpu_use = (double)((sd+id))*1.00/(nd-od); //((用户+系统))除(第一次和第二次的时间差)再赋给g_cpu_used
		else cpu_use = 0;
		

		return cpu_use;
	}
	
	#endif


	double GetOsCpuUsePercent()
	{
		#if BMCO_OS == BMCO_OS_WINDOWS_NT
		FILETIME preidleTime;
		FILETIME prekernelTime;
		FILETIME preuserTime;

		FILETIME idleTime;
		FILETIME kernelTime;
		FILETIME userTime;

		GetSystemTimes( &idleTime, &kernelTime, &userTime );
		preidleTime = idleTime;
		prekernelTime = kernelTime;
		preuserTime = userTime ;

		Bmco::Event hEvent(false);
		hEvent.wait(500);
		
		GetSystemTimes( &idleTime, &kernelTime, &userTime );

		Bmco::Int64 idle = CompareFileTime( preidleTime,idleTime);
		Bmco::Int64 kernel = CompareFileTime( prekernelTime, kernelTime);
		Bmco::Int64 user = CompareFileTime(preuserTime, userTime);

		double cpu = (kernel +user - idle) *100.0/(kernel+user);


		return cpu;
		#elif BMCO_OS == BMCO_OS_LINUX
		
		CPU_OCCUPY cpu_stat1;
		CPU_OCCUPY cpu_stat2;

		get_cpuoccupy((CPU_OCCUPY *)&cpu_stat1);
		
		Bmco::Thread::sleep(1000);
		get_cpuoccupy((CPU_OCCUPY *)&cpu_stat2);

		double cpu = cal_cpuoccupy ((CPU_OCCUPY *)&cpu_stat1, (CPU_OCCUPY *)&cpu_stat2);

		return cpu;
		
		
		#endif
		
		//default
		return SYSERROR;

	}

#ifdef DEF_HP
#define _guard_def_unix95__
#endif
#ifdef DEF_SOLARIS
#define _guard_def_unix95__
#endif

#ifdef DEF_UNIX95
#define _guard_def_unix95__
#endif

	//const static Bmco::UInt64 PROMEMDIV=1024;//convert pro mem to KB
	Bmco::Int64 GetProcessMemUse(Bmco::Process::PID pid)
	{

		#if BMCO_OS == BMCO_OS_WINDOWS_NT
		HANDLE hProcess;
		PROCESS_MEMORY_COUNTERS pmc;
		hProcess = OpenProcess(  PROCESS_QUERY_INFORMATION |PROCESS_VM_READ,FALSE, pid );

		if (NULL == hProcess)
			return SYSERROR;
		if ( GetProcessMemoryInfo( hProcess, &pmc, sizeof(pmc)) )
		{
			/*
			printf( "\pid: %u\n", pid);
			printf( "\tPageFaultCount: %d\n", pmc.PageFaultCount/PROMEMDIV );
			printf( "\tPeakWorkingSetSize: %d\n",pmc.PeakWorkingSetSize/PROMEMDIV );
			printf( "\tWorkingSetSize: %d\n", pmc.WorkingSetSize/PROMEMDIV );
			printf( "\tQuotaPeakPagedPoolUsage: %d\n",pmc.QuotaPeakPagedPoolUsage/PROMEMDIV );
			printf( "\tQuotaPagedPoolUsage: %d\n",pmc.QuotaPagedPoolUsage/PROMEMDIV );
			printf( "\tQuotaPeakNonPagedPoolUsage: %d\n",pmc.QuotaPeakNonPagedPoolUsage/PROMEMDIV  );
			printf( "\tQuotaNonPagedPoolUsage: %d\n",pmc.QuotaNonPagedPoolUsage/PROMEMDIV  );
			printf( "\tPagefileUsage: %d\n", pmc.PagefileUsage/PROMEMDIV  ); 
			printf( "\tPeakPagefileUsage: %d\n",pmc.PeakPagefileUsage/PROMEMDIV  );
			*/
			CloseHandle( hProcess );
			return pmc.PagefileUsage;
		}

		CloseHandle( hProcess );
		return SYSERROR;

		#elif BMCO_OS == BMCO_OS_LINUX || BMCO_OS_FAMILY_UNIX == 1

		std::string sCommand;
		Bmco::Int32 mem=0;
#ifdef _guard_def_unix95__
		sCommand=Bmco::format("UNIX95= ps -p %d -o vsz |grep -v ""VSZ"" |awk '{print $1}'",pid);
		fp = popen (sCommand.c_str(), "r");
		if (fp == NULL)
			return SYSERROR;

		while(1){
			if (feof(fp)) break;
			if (fscanf (fp, "%d \n", &mem) == EOF){
				pclose(fp);
				return SYSERROR;
			}
		}
#else
		sCommand=Bmco::format("ps -p %d -o '%%z ' |grep -v ""VSZ"" |awk '{print $1}'",pid);

		FILE *fp = popen (sCommand.c_str(), "r");
		if (fp == NULL)
			return SYSERROR;

		if (fscanf (fp, "%d \n", &mem) == EOF)
		{
			pclose(fp);
			return SYSERROR;
		}
#endif
		pclose(fp);
		return mem;
		#endif

		//default
		return SYSERROR;
	}


	Bmco::Int32 GetProcessCpuUse(Bmco::Process::PID pid)
	{
		
		//double cpu=0;
		Bmco::Int32 cpu=0;
		#if BMCO_OS == BMCO_OS_WINDOWS_NT
		try
		{
			HANDLE hProcess;
			hProcess = OpenProcess(  PROCESS_QUERY_INFORMATION |PROCESS_VM_READ,FALSE, pid );
			if (NULL == hProcess)
				return SYSERROR;
			
			FILETIME prenow;
			FILETIME precreateTime;
			FILETIME preexitTime;
			FILETIME prekernelTime;
			FILETIME preuserTime;

			FILETIME now;
			FILETIME createTime;
			FILETIME exitTime;
			FILETIME kernelTime;
			FILETIME userTime;



			GetProcessTimes( hProcess,&createTime, &exitTime, &kernelTime,&userTime);
			GetSystemTimeAsFileTime(&now);  

			prenow=now;
			precreateTime = createTime;
			preexitTime=exitTime;
			prekernelTime = kernelTime;
			preuserTime = userTime ;

			Bmco::Event hEvent(false);
			hEvent.wait(500);
			GetProcessTimes( hProcess,&createTime, &exitTime, &kernelTime,&userTime);
			GetSystemTimeAsFileTime(&now);

			Bmco::Int64 idle = CompareFileTime( prenow, now);
			Bmco::Int64 kernel = CompareFileTime( prekernelTime, kernelTime);
			Bmco::Int64 user = CompareFileTime(preuserTime, userTime);
			
			
			if (kernel+user != 0)
			{
				cpu= ((kernel +user)*100.0/Bmco::Environment::processorCount()+idle/2)/(idle);
			}
			
			
			CloseHandle(hProcess);
			return cpu;
		}
		catch (...)
		{
			return SYSERROR;
		}
		
		#elif BMCO_OS == BMCO_OS_LINUX || BMCO_OS_FAMILY_UNIX == 1

		
		std::string sCommand;
#ifdef _guard_def_unix95__
		sCommand=Bmco::format("UNIX95= ps -p %d -o pcpu |grep -v ""CPU"" |awk '{print $1}'",pid);
		fp = popen (sCommand.c_str(), "r");
		if (fp == NULL)
			return SYSERROR;

		while(1){
			if (feof(fp)) break;
			if (fscanf (fp, "%d \n", &cpu) == EOF){
				pclose(fp);
				return SYSERROR;
			}
		}
#else
		sCommand=Bmco::format("ps -p %d -o '%%C ' |grep -v ""CPU"" |awk '{print $1}'",pid);

		FILE *fp = popen (sCommand.c_str(), "r");
		if (fp == NULL)
			return SYSERROR;

		if (fscanf (fp, "%d \n", &cpu) == EOF)
		{
			pclose(fp);
			return SYSERROR;
		}
#endif
		pclose(fp);
		return cpu;

		#endif

		//default
		return SYSERROR;
	}


}


