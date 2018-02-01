#ifndef __METAPROGRAMDEF_HH__
#define __METAPROGRAMDEF_HH__

#include "Bmco/Timestamp.h"
#include "Bmco/DateTimeFormatter.h"
#include "Bmco/DateTimeFormat.h"

namespace BM35
{
//������0xA��ö��
enum ProgramTypeA
{
	BOL_PROGRAM_TYPE_A = 1, //bol
	HLA_PROGRAM_TYPE_A = 2 //hla
};
//enum ProgramType	//����˳��0-->1,1--->5 ,1--->2,1--->BOL, 1--->HLA
//{
//	KERNEL_PROGRAM_TYPE = 0, //0�Ž��̵�BPCB_ID
//	MOINTOR_PROGRAM_TYPE = 1, //1�Ž��̵�BPCB_ID
//	MEMMGR_PROGRAM_TYPE = 2, //2�Ž��̵�BPCB_ID
//	CLOUDAGENT_PROGRAM_TYPE = 5 //5�Ž��̵�BPCB_ID
//};

	//����BM3.5��������Ҫ���ȵĳ��򣬰���BOL�еĺ��ĳ���HLA�е�ҵ�����������ݷ������
	//�����е����ݳ�IDΪ�����������⣬�����ֶ�ȫ�����Ժ��������ļ�
	
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
		Bmco::UInt32 ID; //������ID���������Զ����ɣ���1��ʼ���
		Bmco::UInt32 MaxInstance; //��ʾ�������ʵ�����̵ĸ�����������ֶδ�����Сֵ�ֶΣ�����Сֵ�ֶβ�Ϊ0����֧�ָߵ�ˮ����
		Bmco::UInt32 MinInstance; //��ʾ��С����ʵ�����̵ĸ��������Ϊ0�����ʾ�ڱ�bol�У��������ý���
		//Bmco::UInt32 Type; //'�������ͣ��磺0xABCCDDDD
		//					//4bit A��ʾ�� BOL��HLA��
		//					//4bit B��ʾ�� ��פ����(�ú�̨һֱ����)�� һ���Խ���(����һ�ξ��˳�)��
		//					//8bit CC��ʾ��HLA�е����ݷ���ҵ�����
		//					//16bit DDDD��ʾ�� �����ţ���0#������Ϊ0��SR�е�1000������Ϊ1000';
		Bmco::UInt32 ProcessType;  //��������BOL��HLA��
		Bmco::UInt32 ProcessLifecycleType; //��������������𣺳�פ�򵥴�ִ��
		Bmco::UInt32 ProcessBusinessType;  //����ҵ�����
		Bmco::UInt32 BolBpcbID;       ///BOL������Ҫ�ƶ��������BPCB ID��HLA����Ҫ
		std::string AbsolutePath; //��ſ�ִ���ļ����ļ�ϵͳ����·��
		std::string ExeName; //��ִ���ļ����֣���prep.exe'
		std::string StaticParams; //�̶���Ҫ���������д���Ĳ��������� -DSomeKey=SomeValue
		std::string DynamicParams; //-FlowID=%d -InstanceID=%d -BpcbID=%d
		std::string DisplayName;	//����չʾ������"����" �����ۡ�
		std::string libpath;//��·��
		std::string rootdir;
		Bmco::Int64 Ramlimit;
		Bmco::Int32  Cpulimit;
		
		
		Bmco::Timestamp TimeStamp; //һ�������¼�Ĵ���ʱ���
		Bmco::UInt32 Priority;   ///�������ȼ�����Դ���䣩
		
	private:
		MetaProgramDef();/// default constructor is not allowed
	};


}

#endif //end __METAPROGRAMDEF_HH__

