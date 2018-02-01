#include "Bmco/Util/HelpFormatter.h"
#include "Bmco/Util/ServerApplication.h"
#include "Bmco/TaskManager.h"
#include "Bmco/NotificationQueue.h"
#include "MemUtil.h"
#include "KernelTaskLoop.h"
#include "MemTaskLoop.h"
#include "RegularProcessTaskLoop.h"
#include "TimeTaskLoop.h"
#include "BolServiceServer.h"
#include "BolServiceLog.h"
#include "MemTableOp.h"

#ifdef BOL_OMC_WRITER 
#include "OmcLog.h"
#endif

#if BMCO_OS == BMCO_OS_LINUX || BMCO_OS_FAMILY_UNIX == 1
#include <signal.h>
#endif

using namespace Bmco;
using namespace Bmco::Util;



namespace BM35 {

///
/// BOLKernel is the major class to implement process #0 of BOL
///
class BOLKernel: public ServerApplication
{
public:
	BOLKernel(): helpRequested(false),versionRequested(false),cfgfileReq(false)
		, fistRunner(false),RuleChkfileReq(false)
		, bolStatus(BOL_NORMAL)
		, bunlock(false){
	}    
    virtual ~BOLKernel(){
	}
protected:
    void initialize(Application& self)
    {

		#if BMCO_OS == BMCO_OS_LINUX || BMCO_OS == BMCO_OS_AIX || BMCO_OS == BMCO_OS_HPUX		
		signal(SIGCLD, SIG_IGN); 		
		#endif

	
  		loadConfiguration();// load default configuration files, if present

		if(!cfgfileReq)
		{
			loadConfiguration(config().getString("application.dir")+"../etc/BKernel.ini");
		}

		if(!RuleChkfileReq)
		{
			loadConfiguration(config().getString("application.dir")+"../etc/RuleCheck.ini");
		}

		loadConfiguration(config().getString("application.dir")+"../etc/BolDynamic.ini"); // load public config
		loadConfiguration(config().getString("application.dir")+"../etc/BolPublic.ini"); // load public config
		
        	ServerApplication::initialize(self);

		BolServiceLog::setLogger(&logger());

		bmco_information_f2(logger(),"%s|%s|BOLKernel is now starting up...",
			std::string("0"),std::string(__FUNCTION__));

		#ifdef DEF_BOL_TEST
		#if BMCO_OS == BMCO_OS_LINUX || BMCO_OS_FAMILY_UNIX == 1
		signal(SIGCLD, SIG_IGN); 
		#endif
		#endif
    }
        
    void uninitialize()
    {
        bmco_information_f2(logger(),"%s|%s|BOLKernel is now shutting down...",
			std::string("0"),std::string(__FUNCTION__));

	(logger().getChannel())->close();
		
        ServerApplication::uninitialize();
        if (fistRunner)	BIP::named_mutex::remove(uniqName.c_str());
    }
	
	 int main(const std::vector<std::string>& args)
    {		
        if (helpRequested)	{
            displayHelp();
        }
	else if(versionRequested)
	{
		showVersion();
	}
		else {
			try {
				std::string basename = config().getString("application.baseName");
				// retrieve bol name, no default value, it may throw
				std::string bolname  = config().getString("info.bol_name");
				if (bolname.empty()) throw NotFoundException("info.bol_name");

				uniqName = basename + "_" + bolname;
			}
			catch (Bmco::NotFoundException &e) {
				bmco_error_f3(logger(),"%s|%s|please make sure configure file has included item %s."
					, std::string("0"), std::string(__FUNCTION__), e.message());
				return Application::EXIT_CONFIG;
			}

			bmco_trace_f3(logger(),"%s|%s|the unique name is %s"
				, std::string("0"),std::string(__FUNCTION__),uniqName);
			
			// only unlock the singleton mutex
			if (bunlock) {
				BIP::named_mutex::remove(uniqName.c_str());
				//(logger().getChannel())->close();
				return Application::EXIT_OK;
			}
			
			// make sure only one process exists	
			if (!checkSinglton()) {
				bmco_error_f2(logger(),"%s|%s|please make sure current process is not running and call -u to release the lock if necessary.",
					std::string("0"),std::string(__FUNCTION__));
				return Application::EXIT_TEMPFAIL;
			}

			//初始化omc接口
			#ifdef BOL_OMC_WRITER 
			 if (!OmcLog::Instance().Init())
			 {
			      bmco_error_f2(logger(),"%s|%s|init  omc log writer failed!",
						std::string("0"),std::string(__FUNCTION__));
			     return Application::EXIT_TEMPFAIL;
			 }
			 else
			 {
			 	bmco_information_f2(logger(),"%s|%s|omc file init success",
						std::string("0"),std::string(__FUNCTION__));
			 }
			 #endif

			
			// get ready to initialize control region
			BolServiceServer server(config().getString("info.bol_name"));
			
			 try
			 {
			        if (!server.start()) {
			            bmco_error_f2(logger(),"%s|%s|start tcp server thread failed!",
						std::string("0"),std::string(__FUNCTION__));
			            return Application::EXIT_CANTCREAT;
			        }
			}
			catch(...)
			{
				bmco_error_f2(logger(),"%s|%s|start BolServiceServer thread catch exceptions.",
					std::string("0"),std::string(__FUNCTION__));
				return Application::EXIT_CANTCREAT;
			}

			//first set bol status
			try
			{
				if (!MemTableOper::instance().SetBolStatus(bolStatus)) 
				{
					bmco_error_f2(logger(),"%s|%s|failed to update bol status.",
						std::string("0"),std::string(__FUNCTION__));
					return Application::EXIT_CANTCREAT;
				}

				MemTableOper::instance().UpdateBpcbCreateTime(0);
				
			}
			catch(...)
			{
				bmco_error_f2(logger(),"%s|%s|set bol status catch exceptions.",
					std::string("0"),std::string(__FUNCTION__));
				return Application::EXIT_CANTCREAT;
			}
	

			
			// run threads to perform run time job
			TaskManager tm;
			try{
				tm.start(new KernelTaskLoop(bolStatus));
				tm.start(new MemTaskLoop());
				tm.start(new RegularProcessTaskLoop());
				
				
			}
			catch(...)
			{
				bmco_error_f2(logger(),"%s|%s|start work thread catch exceptions.",
					std::string("0"),std::string(__FUNCTION__));
				return Application::EXIT_CANTCREAT;
			}

			try
			{
				waitForTerminationRequest();
				server.stop();
				tm.cancelAll();
				tm.joinAll();
			}
			catch(...)
			{
				bmco_error_f2(logger(),"%s|%s|wait for child thread catch exceptions.",
					std::string("0"),std::string(__FUNCTION__));
				return Application::EXIT_CANTCREAT;
			}
        }
        return Application::EXIT_OK;
    }
    
    void defineOptions(OptionSet& options)
    {
        ServerApplication::defineOptions(options);

        options.addOption(
            Option("help", "h", "[exclusive] display help information on command line arguments.")
            .required(false)
            .repeatable(false));

        options.addOption(
            Option("define", "D", "[optional] define a configuration property.")
            .required(false)
            .repeatable(true)
            .argument("name=value")
            .callback(OptionCallback<BOLKernel>(this, &BOLKernel::handleDefine)));

        options.addOption(
            Option("unlock", "u", "[exclusive] unlock the global mutex manually.")
            .required(false)
            .repeatable(false)
            .callback(OptionCallback<BOLKernel>(this, &BOLKernel::handleUnlock)));


	 options.addOption(
            Option("version", "v", "[exclusive] show the version.")
            .required(false)
            .repeatable(false)
            .callback(OptionCallback<BOLKernel>(this, &BOLKernel::handleVersion)));
	 
		
		options.addOption(
			Option("status", "s", "[optional] start BOL with specified status N|M (Normal or Maintaince).")
			.required(false)
			.repeatable(false)
			.argument("N|M")
			.callback(OptionCallback<BOLKernel>(this, &BOLKernel::handleStart)));

		options.addOption(
            Option("config-file", "f", "[optional] load configuration data from a file.")
            .required(false)
            .repeatable(true)
            .argument("file")
            .callback(OptionCallback<BOLKernel>(this, &BOLKernel::handleConfig)));

		options.addOption(
            Option("rulecheck-file", "r", "[required] load rule check configuration data from a file.")
            .required(false)
            .repeatable(true)
            .argument("file")
            .callback(OptionCallback<BOLKernel>(this, &BOLKernel::handleRuleCheck)));
    }

    void handleOption(const std::string& name, const std::string& value)
    {
        ServerApplication::handleOption(name, value);

        if (name == "help") helpRequested = true;	  
    }

	void handleConfig(const std::string& name, const std::string& value)
	{
		cfgfileReq=true;
		loadConfiguration(value);
	}

	void handleRuleCheck(const std::string& name, const std::string& value)
	{
		RuleChkfileReq=true;
		loadConfiguration(value);
	}

	void handleVersion(const std::string& name, const std::string& value)
	{
		versionRequested = true;
		 stopOptionsProcessing();
	}

	void showVersion() 
	{
	    std::string version = "debug version";
	    #ifdef MODULE_VERSION
	    version = MODULE_VERSION;
	    #endif

	    std::cout << "version: " << version << std::endl;
	}

	void handleStart(const std::string& name, const std::string& value)
	{
		if (value == "M"){
			bolStatus=BOL_MAINTAIN;
		}
		else if (value == "N"){
			bolStatus=BOL_NORMAL;
		}
		else{
			bolStatus=BOL_NORMAL;
		}
	}

    void handleDefine(const std::string& name, const std::string& value)
    {
        defineProperty(value);
    }

    void handleUnlock(const std::string& name, const std::string& value)
    {
       bunlock=true;
    }
    void defineProperty(const std::string& def)
    {
        std::string name;
        std::string value;
        std::string::size_type pos = def.find('=');
        if (pos != std::string::npos)
        {
            name.assign(def, 0, pos);
            value.assign(def, pos + 1, def.length() - pos);
        }
        else name = def;
        config().setString(name, value);
    }

    void handleHelp(const std::string& name, const std::string& value)
    {
        helpRequested = true;
        displayHelp();
        stopOptionsProcessing();
    }

    void displayHelp()
    {
        HelpFormatter helpFormatter(options());
        helpFormatter.setCommand(commandName());
        helpFormatter.setUsage("OPTIONS");
        helpFormatter.setHeader("Billing-On-Line Kernel process, which is known as the 0# process of BOL, it will initialize BOL run-time environment and then start Memory Management process and Process Management process.");
        helpFormatter.format(std::cout);
    }    
    void printProperties(const std::string& base)
    {
        AbstractConfiguration::Keys keys;
        config().keys(base, keys);
        if (keys.empty())
        {
            if (config().hasProperty(base))
            {
                std::string msg;
                msg.append(base);
                msg.append(" = ");
                msg.append(config().getString(base));
				bmco_trace_f3(logger(),"%s,%s,%s",
					std::string("0"),std::string(__FUNCTION__),msg);
            }
        }
        else
        {
            for (AbstractConfiguration::Keys::const_iterator it = keys.begin(); it != keys.end(); ++it)
            {
                std::string fullKey = base;
                if (!fullKey.empty()) fullKey += '.';
                fullKey.append(*it);
                printProperties(fullKey);
            }
        }
    }
 
    bool checkSinglton()
	{
		try {
			BIP::named_mutex excl_mtx(BIP::create_only, uniqName.c_str());
			fistRunner = true;
			return true;
		}
		catch(...) {
			fistRunner = false;
			return false;
		}
	}
    
    
private:
	bool bunlock;
	bool helpRequested;
	bool versionRequested;
	bool cfgfileReq;
	bool RuleChkfileReq;
	
	bool fistRunner;
	std::string uniqName;
	BolStatus bolStatus;
};

} //namespace BM35 

BMCO_SERVER_MAIN(BM35::BOLKernel)

