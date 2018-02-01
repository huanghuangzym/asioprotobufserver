#ifndef __METAPROGRAMDEF_HH__
#define __METAPROGRAMDEF_HH__

#include "Bmco/Timestamp.h"
#include "Bmco/DateTimeFormatter.h"
#include "Bmco/DateTimeFormat.h"

namespace BM35
{
//程序类0xA的枚举
enum ProgramTypeA
{
	BOL_PROGRAM_TYPE_A = 1, //bol
	HLA_PROGRAM_TYPE_A = 2 //hla
};
//enum ProgramType	//启动顺序：0-->1,1--->5 ,1--->2,1--->BOL, 1--->HLA
//{
//	KERNEL_PROGRAM_TYPE = 0, //0号进程的BPCB_ID
//	MOINTOR_PROGRAM_TYPE = 1, //1号进程的BPCB_ID
//	MEMMGR_PROGRAM_TYPE = 2, //2号进程的BPCB_ID
//	CLOUDAGENT_PROGRAM_TYPE = 5 //5号进程的BPCB_ID
//};

	//定义BM3.5的所有需要调度的程序，包括BOL中的核心程序，HLA中的业务程序或者数据服务程序；
	//本表中的数据除ID为自增生成以外，其他字段全部来自核心配置文件
	
	struct MetaProgramDef
	{
		///// smart ptr definition
		typedef Bmco::SharedPtr<MetaProgramDef> Ptr;



		MetaProgramDef(Bmco::UInt32 id,Bmco::UInt32 maxInstance,Bmco::UInt32 minInstance,Bmco::UInt32 processType,
						Bmco::UInt32 processLifecycleType,Bmco::UInt32 processBusinessType,Bmco::UInt32  bolBpcbID,
						const char* exeName,const char * staticParams,
						const char * dynamicParams,const char * displayName,const char * exepath,const char * libpth,
						const char * rootpth,Bmco::Timestamp timeStamp,Bmco::UInt32  priority,
						Bmco::Int64 cpulimit,	Bmco::Int32  ramlimit):
						ID(id),MaxInstance(maxInstance),MinInstance(minInstance),ProcessType(processType),
						ProcessLifecycleType(processLifecycleType),ProcessBusinessType(processBusinessType),BolBpcbID(bolBpcbID),
						ExeName(exeName),StaticParams(staticParams),
						DynamicParams(dynamicParams),AbsolutePath(exepath),libpath(libpth),rootdir(rootpth),
						DisplayName(displayName),TimeStamp(timeStamp),Priority(priority),
						Ramlimit(ramlimit),Cpulimit(cpulimit)
						{
						}

		
		MetaProgramDef(const MetaProgramDef& r)
		{
			assign(r);
		}

		
		MetaProgramDef& operator = (const MetaProgramDef& r)
		{
			assign(r);
			return *this;
		}

		
		void assign(const MetaProgramDef& r)
		{
			if((void*)this!=(void*)&r){
				ID = r.ID;
				MaxInstance = r.MaxInstance;
				MinInstance = r.MinInstance;
				ProcessType = r.ProcessType;
				ProcessLifecycleType = r.ProcessLifecycleType;
				ProcessBusinessType = r.ProcessBusinessType;
				BolBpcbID = r.BolBpcbID;
				AbsolutePath = r.AbsolutePath.c_str();
				ExeName = r.ExeName.c_str();
				StaticParams = r.StaticParams.c_str();
				DynamicParams = r.DynamicParams.c_str();
				DisplayName = r.DisplayName.c_str();
				TimeStamp = r.TimeStamp;
				Priority = r.Priority;
			}
		}

		//field definition
		Bmco::UInt32 ID; //进程组ID，自增，自动生成，从1开始编号
		Bmco::UInt32 MaxInstance; //表示最大启动实例进程的个数，如果本字段大于最小值字段，且最小值字段不为0，则支持高低水调度
		Bmco::UInt32 MinInstance; //表示最小启动实例进程的个数，如果为0，则表示在本bol中，不启动该进程
		//Bmco::UInt32 Type; //'程序类型，如：0xABCCDDDD
		//					//4bit A表示： BOL，HLA等
		//					//4bit B表示： 常驻进程(置后台一直运行)， 一次性进程(运行一次就退出)等
		//					//8bit CC表示：HLA中的数据服务、业务进程
		//					//16bit DDDD表示： 程序编号，如0#进程则为0，SR中的1000进程则为1000';
		Bmco::UInt32 ProcessType;  //程序大类别：BOL，HLA等
		Bmco::UInt32 ProcessLifecycleType; //程序生命周期类别：常驻或单次执行
		Bmco::UInt32 ProcessBusinessType;  //程序业务类别
		Bmco::UInt32 BolBpcbID;       ///BOL程序需要制定启动后的BPCB ID，HLA不需要
		std::string AbsolutePath; //存放可执行文件的文件系统绝对路径
		std::string ExeName; //可执行文件名字，如prep.exe'
		std::string StaticParams; //固定需要在命令行中传入的参数，比如 -DSomeKey=SomeValue
		std::string DynamicParams; //-FlowID=%d -InstanceID=%d -BpcbID=%d
		std::string DisplayName;	//用于展示，比如"规整" “批价”
		std::string libpath;//库路径
		std::string rootdir;
		Bmco::Int64 Ramlimit;
		Bmco::Int32  Cpulimit;
		
		
		Bmco::Timestamp TimeStamp; //一条定义记录的创建时间戳
		Bmco::UInt32 Priority;   ///进程优先级（资源分配）
		
	private:
		MetaProgramDef();/// default constructor is not allowed
	};


}

#endif //end __METAPROGRAMDEF_HH__

