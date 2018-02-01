#include "MemTaskLoop.h"
#include "Bmco/DateTimeFormatter.h"
#include "Bmco/DateTimeFormat.h"
#include <stdio.h>
#include "Bmco/Environment.h"
#include <boost/lexical_cast.hpp>
#include <strings.h>
#include "Bmco/Channel.h"
#include "Bmco/Process.h"
#include "ShmMappedFile.h"
#include "ShmSegment.h"
#include "VirtualShm.h"
#include "BolServiceLog.h"

#ifdef BOL_OMC_WRITER 
#include "OmcLog.h"
#endif



using Bmco::DateTimeFormatter;
using Bmco::DateTimeFormat;

namespace BM35 {

enum RegionType {
    REGN_ALL		= 0, // reserved
    REGN_CONTROL	= 1, // control region of BOL
    REGN_TEMP		= 2, // temp data region of BOL
    REGN_DATA		= 3  // data region of BOL
};



void MemTaskLoop::runTask() 
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
		bmco_error_f2(theLogger, "%s|%s| done mem task loop unknown exception."
			, std::string("0"),std::string(__FUNCTION__));
		return;
	}
	bmco_information_f2(theLogger, "%s|%s| done mem task loop gracefully."
		, std::string("0"),std::string(__FUNCTION__));
}


bool MemTaskLoop::init()
{
	
	if(!MemTableOper::instance().QueryTableAllData(strMetaBolCrontab,m_chunkmtx)) 
	{
		bmco_error_f2(theLogger,"%s|%s|query ChunkInfo is error",std::string("0"),std::string(__FUNCTION__));
		return false;
	}

	return true;
}

bool MemTaskLoop::waitForChunkInfo()
{
	if(m_chunkmtx.rowValueCount() == 1)
	{
		bmco_warning_f2(theLogger,"%s|%s|waiting for chunkinfo sync from CloudAgent",std::string("0"),std::string(__FUNCTION__));
		this->sleep(5000);
		return false;
	}

	Bmco::UInt64 iControlUsedBytes = 0;
	Bmco::UInt64 iTempUsedBytes = 0;
	Bmco::UInt64 iDataUsedBytes = 0;
	
	
	for(int i=0;i<m_chunkmtx.rowValueCount();i++)
	{
		Bmco::Int64 itbytes=0;
		m_chunkmtx.getFieldValue(strbytes,i).getInt64(itbytes);
	
		if(itbytes<= 0)
		{
			bmco_error_f4(theLogger,"%s|%s|Error config for segment which name is [%s] and size is[%?d]",
					std::string("0"),std::string(__FUNCTION__),
					m_chunkmtx.getFieldValue(strname,i).getString(),itbytes);
			continue;
		}

		Bmco::Int32 itregionid=0;
		m_chunkmtx.getFieldValue(strregionId,i).getInt32(itregionid);

		switch(itregionid)
		{
			case REGN_CONTROL:
				iControlUsedBytes +=itbytes;break;
			case REGN_TEMP:
				iTempUsedBytes += itbytes;break;
			case REGN_DATA:
				iDataUsedBytes +=itbytes;break;
		}
		if(itregionid == REGN_CONTROL)
		{
			continue;
		}

		Bmco::Int32 itsMappedFile=0;
		m_chunkmtx.getFieldValue(strisMappedFile,i).getInt32(itsMappedFile);

		
		
		if(itsMappedFile)
		{
		  //when it needs to do checkpoint the mappedfile must create the SnapShot firstly
		  ShmMappedFile::Ptr shmPtr;
		  shmPtr = new ShmMappedFile((sChunkPath + m_chunkmtx.getFieldValue(strname,i).getString()).c_str(), 
		  	SHM_CREATE_OR_OPEN, itbytes);
		  if (!shmPtr->createOrOpen()) 
		  {
			  bmco_error_f3(theLogger,"%s|%s|Failed to create mappedfile which name is [%s]",std::string("0"),std::string(__FUNCTION__),
			  	m_chunkmtx.getFieldValue(strname,i).getString());
			  continue;
		  }else
		  {
			  bmco_information_f3(theLogger,"%s|%s| create mappedfile which name is [%s] successly",std::string("0"),std::string(__FUNCTION__),
			  	m_chunkmtx.getFieldValue(strname,i).getString());
		  }
		}
		else
		{
			ShmSegment::Ptr shmPtr;
			shmPtr =  new ShmSegment((m_chunkmtx.getFieldValue(strname,i).getString()).c_str(), SHM_CREATE_OR_OPEN, itbytes);
		    if (!shmPtr->createOrOpen()) 
		   {
			  bmco_error_f3(theLogger,"%s|%s|Failed to create shared memory which name is [%s]",std::string("0"),std::string(__FUNCTION__),
			  	m_chunkmtx.getFieldValue(strname,i).getString());
			  continue;
		   }else
		   {
			   bmco_information_f3(theLogger,"%s|%s| create shared memory which name is [%s] successly",std::string("0"),std::string(__FUNCTION__),
			   	m_chunkmtx.getFieldValue(strname,i).getString());
		   }
		}
	}

	{
		///更新区的使用大小
		Matrix regionmtx;
		if(!MemTableOper::instance().UpdateReginUseBytesById(REGN_CONTROL,iControlUsedBytes))
		{
			bmco_error_f2(theLogger,"%s|%s|Fail to update on MetaShmRegionInfoTable",std::string("0"),std::string(__FUNCTION__));
		}
		
		if(!MemTableOper::instance().UpdateReginUseBytesById(REGN_TEMP,iTempUsedBytes))
		{
			bmco_error_f2(theLogger,"%s|%s|Fail to update on MetaShmRegionInfoTable",std::string("0"),std::string(__FUNCTION__));
		}
		

		if(!MemTableOper::instance().UpdateReginUseBytesById(REGN_DATA,iDataUsedBytes))
		{
			bmco_error_f2(theLogger,"%s|%s|Fail to Update on MetaShmRegionInfoTable",std::string("0"),std::string(__FUNCTION__));
		}

	}

	return true;
}

bool MemTaskLoop::doCreateSegment(Bmco::Int32 regionid,const std::string &chunkname,Bmco::Int64 size,
	Bmco::Int32 ismappedfile)
{

	Bmco::LocalDateTime starttime;

	
	try{

		
		if(ismappedfile)
		{
			ShmMappedFile::Ptr _shmPtr = 
				new ShmMappedFile((sChunkPath+chunkname).c_str(),SHM_CREATE_OR_OPEN,size);
			if (!_shmPtr->createOrOpen())
				throw(Bmco::Exception("Failed to create ShmMappedFile"));

		}
		else
		{
			ShmSegment::Ptr _shmPtr = new ShmSegment((chunkname).c_str(),SHM_CREATE_OR_OPEN,size);
			if (!_shmPtr->createOrOpen())
				throw(Bmco::Exception("Failed to create ShmSegment"));
		}

		Bmco::Int64 usedbytes=0;
		MemTableOper::instance().GetReginonUsedBytesbyId(regionid,usedbytes);
		usedbytes +=size;
		
		MemTableOper::instance().UpdateReginUseBytesById(regionid,usedbytes);


		
					
		


	}
	catch(Bmco::Exception &e){
		bmco_error_f3(theLogger,"%s|%s|:%s",std::string("0"),std::string(__FUNCTION__),e.displayText());

		OMC_LOG("40201001|%s|1|creatchunk|%s|%s|BolService|%d|1|%s",
	 	Bmco::Util::Application::instance().config().getString("info.bol_name","bol123").c_str(),
	 	Bmco::DateTimeFormatter::format(starttime.timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	Bmco::DateTimeFormatter::format(Bmco::LocalDateTime().timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	Bmco::Process::id(),
	 	e.displayText().c_str()
	 	);
		
		return false;
	}
	catch(...)
	{
		bmco_error_f2(theLogger,"%s|%s|unknown exception occured when doCreateSegment!",
								std::string("0"),std::string(__FUNCTION__));

		OMC_LOG("40201001|%s|1|creatchunk|%s|%s|BolService|%d|1|unknown exception",
	 	Bmco::Util::Application::instance().config().getString("info.bol_name","bol123").c_str(),
	 	Bmco::DateTimeFormatter::format(starttime.timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	Bmco::DateTimeFormatter::format(Bmco::LocalDateTime().timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	Bmco::Process::id()
	 	);

		return false;
	}



	OMC_LOG("40201001|%s|1|creatchunk|%s|%s|BolService|%d|0|success",
 	Bmco::Util::Application::instance().config().getString("info.bol_name","bol123").c_str(),
 	Bmco::DateTimeFormatter::format(starttime.timestamp(), "%Y%m%d%H%M%S").c_str(),
 	Bmco::DateTimeFormatter::format(Bmco::LocalDateTime().timestamp(), "%Y%m%d%H%M%S").c_str(),
 	Bmco::Process::id()
 	);
	
	
	bmco_information_f3(theLogger,"create mem name %s regnid %?d size :%?d",chunkname,regionid,size);
    return true;
}



bool MemTaskLoop::doDeleteSegment(Bmco::Int32 regionid,const std::string &chunkname,Bmco::Int64 size,
	Bmco::Int32 ismappedfile)
{


	INFORMATION_LOG("begin to delete regionid %d,name %s",regionid,chunkname.c_str());
	Bmco::LocalDateTime starttime;
	
	try{

		 Bmco::UInt64 shmSize = 0;

		if(ismappedfile)
		{
		  ShmMappedFile::Ptr shmPtr;
		  shmPtr = new ShmMappedFile((sChunkPath+chunkname).c_str(), SHM_OPEN_READ_WRIT);
		  if (!shmPtr->attach()) 
			  throw(Bmco::Exception("Failed to attach ShmMappedFile"));

		  shmSize = shmPtr->getSize();
		  if(!ShmMappedFile::destroy((sChunkPath+chunkname).c_str()))
				throw(Bmco::Exception("Failed to destroy ShmMappedFile"));

		}
		else
		{
			ShmSegment::Ptr shmPtr;
			shmPtr = new ShmSegment((chunkname).c_str(), SHM_OPEN_READ_WRIT);
			if (!shmPtr->attach()) 
				throw(Bmco::Exception("Failed to attach ShmSegment"));

			shmSize = shmPtr->getSize();
			if (! ShmSegment::destroy((chunkname).c_str()))
				throw(Bmco::Exception("Failed to destroy ShmSegment"));
		}


		Bmco::Int64 usedbytes=0;
		MemTableOper::instance().GetReginonUsedBytesbyId(regionid,usedbytes);
		usedbytes -=size;
		
		MemTableOper::instance().UpdateReginUseBytesById(regionid,usedbytes);
		
		
		
	}
	catch(Bmco::Exception &e){
		bmco_error_f3(theLogger,"%s|%s|:%s",std::string("0"),std::string(__FUNCTION__),e.displayText());

		OMC_LOG("40201001|%s|1|deletechunk|%s|%s|BolService|%d|1|%s",
	 	Bmco::Util::Application::instance().config().getString("info.bol_name","bol123").c_str(),
	 	Bmco::DateTimeFormatter::format(starttime.timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	Bmco::DateTimeFormatter::format(Bmco::LocalDateTime().timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	Bmco::Process::id(),
	 	e.displayText().c_str()
	 	);
		
		return false;
	}
	catch(...)
	{
		bmco_error_f2(theLogger,"%s|%s|unknown exception occured when doDeleteSegment!",
								std::string("0"),std::string(__FUNCTION__));

		OMC_LOG("40201001|%s|1|deletechunk|%s|%s|BolService|%d|1|unknown exception",
	 	Bmco::Util::Application::instance().config().getString("info.bol_name","bol123").c_str(),
	 	Bmco::DateTimeFormatter::format(starttime.timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	Bmco::DateTimeFormatter::format(Bmco::LocalDateTime().timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	Bmco::Process::id()
	 	);

		return false;
	}



	OMC_LOG("40201001|%s|1|deletechunk|%s|%s|BolService|%d|0|success",
 	Bmco::Util::Application::instance().config().getString("info.bol_name","bol123").c_str(),
 	Bmco::DateTimeFormatter::format(starttime.timestamp(), "%Y%m%d%H%M%S").c_str(),
 	Bmco::DateTimeFormatter::format(Bmco::LocalDateTime().timestamp(), "%Y%m%d%H%M%S").c_str(),
 	Bmco::Process::id()
 	);

	
	
	bmco_information_f3(theLogger,"delete mem name %s regnid %?d size :%?d",chunkname,regionid,size);
    return true;
}

bool MemTaskLoop::doGrowSegment(Bmco::Int32 regionid,const std::string &chunkname,Bmco::Int64 growsize,
	Bmco::Int32 ismappedfile)
{
	Bmco::LocalDateTime starttime;

	try{

		//grow the ShmMappedFile or ShmSegment
		if(ismappedfile)
		{
			if(!ShmMappedFile::grow((sChunkPath+chunkname).c_str(),growsize))
				throw(Bmco::Exception("Failed to grow ShmMappedFile"));

		}
		else
		{
			if(!ShmSegment::grow((chunkname).c_str(),growsize))
				throw(Bmco::Exception("Failed to grow ShmSegment"));
		}


		Bmco::Int64 usedbytes=0;
		MemTableOper::instance().GetReginonUsedBytesbyId(regionid,usedbytes);
		usedbytes +=growsize;
		
		MemTableOper::instance().UpdateReginUseBytesById(regionid,usedbytes);

		
	}
	catch(Bmco::Exception &e){
		bmco_error_f3(theLogger,"%s|%s|:%s",std::string("0"),std::string(__FUNCTION__),e.displayText());

		OMC_LOG("40201001|%s|1|growchunk|%s|%s|BolService|%d|1|%s",
	 	Bmco::Util::Application::instance().config().getString("info.bol_name","bol123").c_str(),
	 	Bmco::DateTimeFormatter::format(starttime.timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	Bmco::DateTimeFormatter::format(Bmco::LocalDateTime().timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	Bmco::Process::id(),
	 	e.displayText().c_str()
	 	);
		
		return false;
	}
	catch(...)
	{
		bmco_error_f2(theLogger,"%s|%s|unknown exception occured when doGrowSegment!",
								std::string("0"),std::string(__FUNCTION__));

		OMC_LOG("40201001|%s|1|growchunk|%s|%s|BolService|%d|1|unknown exception",
	 	Bmco::Util::Application::instance().config().getString("info.bol_name","bol123").c_str(),
	 	Bmco::DateTimeFormatter::format(starttime.timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	Bmco::DateTimeFormatter::format(Bmco::LocalDateTime().timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	Bmco::Process::id()
	 	);

		return false;
	}



	OMC_LOG("40201001|%s|1|growchunk|%s|%s|BolService|%d|0|success",
 	Bmco::Util::Application::instance().config().getString("info.bol_name","bol123").c_str(),
 	Bmco::DateTimeFormatter::format(starttime.timestamp(), "%Y%m%d%H%M%S").c_str(),
 	Bmco::DateTimeFormatter::format(Bmco::LocalDateTime().timestamp(), "%Y%m%d%H%M%S").c_str(),
 	Bmco::Process::id()
 	);

	
	
	
	bmco_information_f3(theLogger,"grow mem name %s regnid %?d size :%?d",chunkname,regionid,growsize);


    	return true;
}

bool MemTaskLoop::doShrinkSegment(Bmco::Int32 regionid,const std::string &chunkname,Bmco::Int64 shrinksize,
	Bmco::Int32 ismappedfile)
{
	Bmco::LocalDateTime starttime;
	Bmco::UInt64 newSize = 0;
	Bmco::UInt64 oldSize = 0;
	Bmco::UInt64 size = 0;
	try{
		
		if(ismappedfile)
		{
		  ShmMappedFile::Ptr shmPtr;
		  {
			  shmPtr = new ShmMappedFile((sChunkPath+chunkname).c_str(), SHM_OPEN_READ_WRIT);
			  if (!shmPtr->attach()) 
				  throw(Bmco::Exception("Failed to attach ShmMappedFile"));

			  oldSize = shmPtr->getSize();
		  }

		  if(!ShmMappedFile::shrink((sChunkPath+chunkname).c_str()))
				throw(Bmco::Exception("Failed to grow ShmMappedFile"));

		  {
			  shmPtr = new ShmMappedFile((sChunkPath+chunkname).c_str(), SHM_OPEN_READ_WRIT);
			  if (!shmPtr->attach()) 
					  throw(Bmco::Exception("Failed to attach ShmMappedFile"));
			   newSize = shmPtr->getSize();
		  }

		   if(oldSize-newSize > shrinksize)
		   {
			   if(!ShmMappedFile::grow((sChunkPath+chunkname).c_str(),oldSize - newSize -shrinksize))
					throw(Bmco::Exception("Failed to grow ShmMappedFile"));
		   }
		   {
			  shmPtr = new ShmMappedFile((sChunkPath+chunkname).c_str(), SHM_OPEN_READ_WRIT);
			  if (!shmPtr->attach()) 
					  throw(Bmco::Exception("Failed to attach ShmMappedFile"));
			  size = shmPtr->getSize();
		   }
		}
		else
		{
		  ShmSegment::Ptr shmPtr;
		  {
		  shmPtr = new ShmSegment((chunkname).c_str(), SHM_OPEN_READ_WRIT);
		  if (!shmPtr->attach()) 
			throw(Bmco::Exception("Failed to attach ShmSegment"));

		  oldSize = shmPtr->getSize();
		  }

		  if(!ShmSegment::shrink((chunkname).c_str()))
				throw(Bmco::Exception("Failed to grow ShmSegment"));

		  {
			shmPtr = new ShmSegment((chunkname).c_str(), SHM_OPEN_READ_WRIT);
			if (!shmPtr->attach()) 
				throw(Bmco::Exception("Failed to attach ShmSegment"));
		    newSize = shmPtr->getSize();
		  }
		   if(oldSize-newSize > shrinksize)
		   {
			   if(!ShmSegment::grow((chunkname).c_str(),oldSize - newSize - shrinksize))
					throw(Bmco::Exception("Failed to grow ShmSegment"));
		   }

		   {
			shmPtr = new ShmSegment((chunkname).c_str(), SHM_OPEN_READ_WRIT);
			if (!shmPtr->attach()) 
				throw(Bmco::Exception("Failed to attach ShmSegment"));
		    size = shmPtr->getSize();
		   }
		}


		Bmco::Int64 usedbytes=0;
		MemTableOper::instance().GetReginonUsedBytesbyId(regionid,usedbytes);
		usedbytes  -=shrinksize;
		
		MemTableOper::instance().UpdateReginUseBytesById(regionid,usedbytes);
		
		
	}
	catch(Bmco::Exception &e){
		bmco_error_f3(theLogger,"%s|%s|:%s",std::string("0"),std::string(__FUNCTION__),e.displayText());

		OMC_LOG("40201001|%s|1|shrinkchunk|%s|%s|BolService|%d|1|%s",
	 	Bmco::Util::Application::instance().config().getString("info.bol_name","bol123").c_str(),
	 	Bmco::DateTimeFormatter::format(starttime.timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	Bmco::DateTimeFormatter::format(Bmco::LocalDateTime().timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	Bmco::Process::id(),
	 	e.displayText().c_str()
	 	);
		
		return false;
	}
	catch(...)
	{
		bmco_error_f2(theLogger,"%s|%s|unknown exception occured when doShrinkSegment!",
								std::string("0"),std::string(__FUNCTION__));

		OMC_LOG("40201001|%s|1|shrinkchunk|%s|%s|BolService|%d|1|unknown exception",
	 	Bmco::Util::Application::instance().config().getString("info.bol_name","bol123").c_str(),
	 	Bmco::DateTimeFormatter::format(starttime.timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	Bmco::DateTimeFormatter::format(Bmco::LocalDateTime().timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	Bmco::Process::id()
	 	);

		return false;
	}



	OMC_LOG("40201001|%s|1|shrinkchunk|%s|%s|BolService|%d|0|success",
 	Bmco::Util::Application::instance().config().getString("info.bol_name","bol123").c_str(),
 	Bmco::DateTimeFormatter::format(starttime.timestamp(), "%Y%m%d%H%M%S").c_str(),
 	Bmco::DateTimeFormatter::format(Bmco::LocalDateTime().timestamp(), "%Y%m%d%H%M%S").c_str(),
 	Bmco::Process::id()
 	);


	

	bmco_information_f3(theLogger,"shrink mem name %s regnid %?d size :%?d",chunkname,regionid,shrinksize);


    return true;
}




bool MemTaskLoop::chunkInfoRefresh()
{
	

	Matrix newChunkmtx;
	if(!MemTableOper::instance().QueryTableAllData(strMetaChunkInfo,newChunkmtx))
	{
		bmco_error_f2(theLogger,"%s|%s|query ChunkInfo is error",std::string("0"),std::string(__FUNCTION__));
		return false;
	}

	Matrix oldChunkmtx=m_chunkmtx;
	m_chunkmtx=newChunkmtx;

	///比较新老chunk信息
	bmco_information_f4(theLogger,"%s|%s| ,before chunkinfo compare,old chunk num is %?d, new chunk num is %?d.",
					std::string("0"),std::string(__FUNCTION__),oldChunkmtx.rowValueCount(),newChunkmtx.rowValueCount());


	std::set<int>  samechunkinnew;
	std::set<int>  samechunkinold;

	
	for(int inew=0;inew<newChunkmtx.rowValueCount();inew++)
	{
		for(int iold=0;iold<oldChunkmtx.rowValueCount();iold++)
		{

			Bmco::Int32 regidnew=0,regidold=0,isgrownew=0,isshrinknew=0;
			Bmco::Int64 bytesnew=0,bytesold=0;
			Bmco::Int32 ismappednew=0;
			
			newChunkmtx.getFieldValue(strregionId, inew).getInt32(regidnew);
			oldChunkmtx.getFieldValue(strregionId, iold).getInt32(regidold);

			newChunkmtx.getFieldValue(strbytes, inew).getInt64(bytesnew);
			oldChunkmtx.getFieldValue(strbytes, iold).getInt64(bytesold);

			newChunkmtx.getFieldValue(strisGrow, inew).getInt32(isgrownew);
			newChunkmtx.getFieldValue(strisShrink, inew).getInt32(isshrinknew);

			newChunkmtx.getFieldValue(strisMappedFile, inew).getInt32(ismappednew);
			
		
			if((newChunkmtx.getFieldValue(strname, inew).getString()== oldChunkmtx.getFieldValue(strname, iold).getString())
			&&(regidnew==regidold))
			{
				samechunkinnew.insert(inew);
				samechunkinold.insert(iold);
				bmco_information_f4(theLogger,"%s|%s| the same chunk: itNew[%s],itOld[%s].",
				std::string("0"),std::string(__FUNCTION__),
				newChunkmtx.getFieldValue(strname, inew).getString(),
				oldChunkmtx.getFieldValue(strname, iold).getString());

				if((bytesnew >bytesold)&&(isgrownew)&&(regidnew != REGN_CONTROL))
				{

					Bmco::LocalDateTime starttime;
					bool issuccess=doGrowSegment(regidnew,newChunkmtx.getFieldValue(strname, inew).getString(),
						bytesnew-bytesold,ismappednew);
					Bmco::LocalDateTime endtime;

					OMC_LOG("20101001|%s|%s|%s|%s|3|%d|%d||%s|%s",
				 	Bmco::Util::Application::instance().config().getString("info.bol_name","bol123").c_str(),
				 	Bmco::DateTimeFormatter::format(starttime.timestamp(), "%Y%m%d%H%M%S").c_str(),
				 	Bmco::DateTimeFormatter::format(endtime.timestamp(), "%Y%m%d%H%M%S").c_str(),
				 	m_chunkmtx.getFieldValue(strname, inew).getString().c_str(),
				 	bytesnew,
				 	issuccess ? 0:-1,
				 	m_chunkmtx.getFieldValue(strowner, inew).getString().c_str(),
				 	Bmco::Util::Application::instance().config().getString("info.ne_host","shanghai").c_str()
				 	);

					
						
				}
				else if((bytesnew < bytesold)&&(isshrinknew)&&(bytesnew> 0)&&(regidnew != REGN_CONTROL))
				{
					Bmco::LocalDateTime starttime;
					bool issuccess=doShrinkSegment(regidnew,newChunkmtx.getFieldValue(strname, inew).getString(),
						bytesold-bytesnew,ismappednew);
					Bmco::LocalDateTime endtime;

					OMC_LOG("20101001|%s|%s|%s|%s|4|%d|%d||%s|%s",
				 	Bmco::Util::Application::instance().config().getString("info.bol_name","bol123").c_str(),
				 	Bmco::DateTimeFormatter::format(starttime.timestamp(), "%Y%m%d%H%M%S").c_str(),
				 	Bmco::DateTimeFormatter::format(endtime.timestamp(), "%Y%m%d%H%M%S").c_str(),
				 	m_chunkmtx.getFieldValue(strname, inew).getString().c_str(),
				 	bytesnew,
				 	issuccess ? 0:-1,
				 	m_chunkmtx.getFieldValue(strowner, inew).getString().c_str(),
				 	Bmco::Util::Application::instance().config().getString("info.ne_host","shanghai").c_str()
				 	);


				}
				break;
			}
		}
	}	


	bmco_information_f3(theLogger,"%s|%s| the same chunk num is %?d.",
		std::string("0"),std::string(__FUNCTION__),samechunkinnew.size());
				
	bmco_information_f4(theLogger,"%s|%s| ,after task compare,leaving old chunk num is %?d, adding new chunk num is %?d.",
		std::string("0"),std::string(__FUNCTION__),oldChunkmtx.rowValueCount()-samechunkinold.size(),
		newChunkmtx.rowValueCount()-samechunkinnew.size());

	for(int i=0;i<newChunkmtx.rowValueCount();i++)
	{

		//jump same
		if(samechunkinnew.count(i) > 0)
			continue;

		Bmco::Int32 regidnew=0;
		Bmco::Int64 bytesnew=0;
		Bmco::Int32 ismappednew=0;
		
		newChunkmtx.getFieldValue(strregionId, i).getInt32(regidnew);
		newChunkmtx.getFieldValue(strbytes, i).getInt64(bytesnew);
		newChunkmtx.getFieldValue(strisMappedFile, i).getInt32(ismappednew);


		Bmco::LocalDateTime starttime;

		
		
		bool issuccess=doCreateSegment(regidnew,newChunkmtx.getFieldValue(strname, i).getString(),
			bytesnew,ismappednew);

		Bmco::LocalDateTime endtime;

		OMC_LOG("20101001|%s|%s|%s|%s|1|%d|%d||%s|%s",
	 	Bmco::Util::Application::instance().config().getString("info.bol_name","bol123").c_str(),
	 	Bmco::DateTimeFormatter::format(starttime.timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	Bmco::DateTimeFormatter::format(endtime.timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	newChunkmtx.getFieldValue(strname, i).getString().c_str(),
	 	bytesnew,
	 	issuccess ? 0:-1,
	 	newChunkmtx.getFieldValue(strowner, i).getString().c_str(),
	 	Bmco::Util::Application::instance().config().getString("info.ne_host","shanghai").c_str()
	 	);


		
	}


	try
	{
		for(int i=0;i<oldChunkmtx.rowValueCount();i++)
		{

			if(samechunkinold.count(i) > 0)
				continue;
			
		
			Bmco::Int32 regidold=0;
			Bmco::Int64 bytesold=0;
			Bmco::Int32 ismappedold=0;
			
			oldChunkmtx.getFieldValue(strregionId, i).getInt32(regidold);
			oldChunkmtx.getFieldValue(strbytes, i).getInt64(bytesold);
			oldChunkmtx.getFieldValue(strisMappedFile, i).getInt32(ismappedold);
		
			if(regidold == REGN_CONTROL)
			{
				bmco_warning_f2(theLogger,"%s|%s|管控区内存不能被删除.",
					std::string("0"),std::string(__FUNCTION__));
				continue;
			}

			
			Bmco::LocalDateTime starttime;
			bool issuccess=doDeleteSegment(regidold,oldChunkmtx.getFieldValue(strname, i).getString(),
				bytesold,ismappedold);

			Bmco::LocalDateTime endtime;

			OMC_LOG("20101001|%s|%s|%s|%s|2|0|%d||%s|%s",
		 	Bmco::Util::Application::instance().config().getString("info.bol_name","bol123").c_str(),
		 	Bmco::DateTimeFormatter::format(starttime.timestamp(), "%Y%m%d%H%M%S").c_str(),
		 	Bmco::DateTimeFormatter::format(endtime.timestamp(), "%Y%m%d%H%M%S").c_str(),
		 	oldChunkmtx.getFieldValue(strname, i).getString().c_str(),
		 	issuccess ? 0:-1,
		 	oldChunkmtx.getFieldValue(strowner, i).getString().c_str(),
		 	Bmco::Util::Application::instance().config().getString("info.ne_host","shanghai").c_str()
		 	);


			
		}
		
	}
	catch(Bmco::Exception &e)
	{
		bmco_error_f3(theLogger,"%s|%s| delete region  catch  exception %s ",std::string("0"),std::string(__FUNCTION__),
			e.displayText());
		return false;
	}
	 catch (std::exception& e) 
	 {
	 	bmco_error_f3(theLogger, "%s|%s| delete region  catch  exception %s."
			, std::string("0"),std::string(__FUNCTION__),std::string(e.what()));
    		return NULL;
	}
	catch(...)
	{
		bmco_error_f2(theLogger, "%s|%s| delete region unknown exception."
			, std::string("0"),std::string(__FUNCTION__));
	}
	

	return true;
}


bool MemTaskLoop::respondlParamterRefresh()
{
	
	if(! chunkInfoRefresh())
	{
		bmco_error_f2(theLogger,"%s|%s|Failed to execute chunkInfoRefresh.",
			std::string("0"),std::string(__FUNCTION__));
		return false;
	}
	
	return true;
}

bool MemTaskLoop::statMemUsage()
{
	
	Bmco::UInt64 freeBytes = 0;
	try
	{

		std::vector<std::vector<std::string> > omcvecs;
		omcvecs.clear();

		for(int i=0;i<m_chunkmtx.rowValueCount();i++)
		{
			int itismapfile=1;
			m_chunkmtx.getFieldValue(strisMappedFile, i).getInt32(itismapfile);

			Bmco::Int64 itbytes=0;
			m_chunkmtx.getFieldValue(strbytes, i).getInt64(itbytes);
		
			if(itismapfile)
			{
				ShmMappedFile::Ptr shmPtr;
				shmPtr = new ShmMappedFile((sChunkPath + m_chunkmtx.getFieldValue(strname, i).getString()).c_str(), 
					SHM_OPEN_READ_WRIT);
				if (shmPtr->attach()) 
				{
					freeBytes = shmPtr->getFreeBytes();
				}else
				{
					bmco_error_f3(theLogger,"%s|%s|Failed to attach chunk[%s]",std::string("0"),std::string(__FUNCTION__),
						m_chunkmtx.getFieldValue(strname, i).getString());
					continue;
				}
			}
			else
			{
				ShmSegment::Ptr shmPtr;
				shmPtr = new ShmSegment(( m_chunkmtx.getFieldValue(strname, i).getString()).c_str(), 
					SHM_OPEN_READ_WRIT);
				if (shmPtr->attach()) 
				{
					freeBytes = shmPtr->getFreeBytes();
				}else
				{
					bmco_error_f3(theLogger,"%s|%s|Failed to attach chunk[%s]",std::string("0"),std::string(__FUNCTION__),
						m_chunkmtx.getFieldValue(strname, i).getString());
					continue;
				}
			}

			
			if(!MemTableOper::instance().UpdateChunkUseBytesByName(m_chunkmtx.getFieldValue(strname, i).getString(),
				itbytes - freeBytes))
			{
				bmco_error_f2(theLogger,"%s|%s|Failed to update MetaShmChunkInfoTable",std::string("0"),std::string(__FUNCTION__));
			}



			
			#ifdef BOL_OMC_WRITER 
			if(Bmco::LocalDateTime().timestamp().epochMicroseconds()-
								m_OmcNextExeTime.timestamp().epochMicroseconds()> 0)
			{
				
				
				std::vector<std::string> omcvec;
				omcvec.push_back(std::string("40201005"));
				omcvec.push_back(Bmco::Util::Application::instance().config().getString("info.bol_name","bol123"));
				omcvec.push_back(m_chunkmtx.getFieldValue(strname, i).getString());
				omcvec.push_back(boost::lexical_cast<std::string>(itbytes));
				omcvec.push_back(boost::lexical_cast<std::string>(itbytes - freeBytes));
				omcvec.push_back(DateTimeFormatter::format(Bmco::LocalDateTime().timestamp(), "%Y%m%d%H%M%S"));
				omcvecs.push_back(omcvec);
				
			}
			#endif
			
			
			
		}

		//更新下次写信息点的时间
		#ifdef BOL_OMC_WRITER 
		if((Bmco::LocalDateTime().timestamp().epochMicroseconds()-
							m_OmcNextExeTime.timestamp().epochMicroseconds()> 0) && (omcvecs.size()>0))
		{
			
			if(MemTableOper::instance().GetisOmcFileValid("40201005"))
			{
				OMCLOGVEC(omcvecs);
			}
			

			
			omcvecs.clear();

			Bmco::Int32 omcinterval=MemTableOper::instance().GetOmcFileTime("40201005");
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
		
	}
	catch(Bmco::Exception &e)
	{
		bmco_error_f3(theLogger,"%s|%s|%s",std::string("0"),std::string(__FUNCTION__),e.displayText());
		return false;
	}
	catch(...)
	{
		bmco_error_f2(theLogger,"%s|%s|unknown exception occured when statMemUsage!",
								std::string("0"),std::string(__FUNCTION__));
		return false;
	}

	return true;
}

} //namespace BM35 

