#include "BolServiceServer.h"
#include "BolServiceRequestHandler.h"
#include "ResponseNotify.h"
#include "ResponseSend.h"
#include "WorkTask.h"
#include "BolServiceLog.h"
#include "DB.h"
//#include "BackupTask.h"
#include "Bmco/Observer.h"
#include "Bmco/Net/StreamSocket.h"
#include "Bmco/Net/SocketAddress.h"
#include "Bmco/NumberFormatter.h"
#include "Bmco/Util/Application.h"
#include "Bmco/Util/LayeredConfiguration.h"
#include "Bmco/Path.h"
#include "Bmco/File.h"
#include "Bmco/FileStream.h"
#include "Bmco/Task.h"
#include "Bmco/Exception.h"
#include "Bmco/Thread.h"
#include "Bmco/NumberParser.h"

namespace BM35 {

/// ChannelFileMonitor
class ChannelFileMonitorTask : public Bmco::Task {
public:
    ChannelFileMonitorTask(const std::string& taskName,
                           const std::string& channelFile,
                           int   port) 
                           : Bmco::Task(taskName),
                             _channelFile(channelFile), 
                             _port(port) {

    }

    virtual ~ChannelFileMonitorTask() {

    }

    virtual void runTask() {
        Bmco::File channelFile(_channelFile);
        while (!isCancelled()) {
            try {
                if (!channelFile.exists()) {
                    Bmco::FileOutputStream fstream(_channelFile);
                    fstream << _port << std::endl;                    
                }
            }
            catch (Bmco::Exception& e) {
                ERROR_LOG(0, "occur exception: %s", e.displayText().c_str());
            }
            catch (std::exception& e) {
                ERROR_LOG(0, "occur exception: %s", e.what());
            }
            catch (...) {
                ERROR_LOG(0, "occur unknown exception");
            }

            try {
                sleep(1000); // sleep 1s
            }
            catch (Bmco::Exception& e) {
                ERROR_LOG(0, "occur exception: %s", e.displayText().c_str());
            }
            catch (std::exception& e) {
                ERROR_LOG(0, "occur exception: %s", e.what());
            }
            catch (...) {
                ERROR_LOG(0, "occur unknown exception");
            }
        }
    }

private:
    ChannelFileMonitorTask();

    ChannelFileMonitorTask(const ChannelFileMonitorTask&);

    ChannelFileMonitorTask& operator = (const ChannelFileMonitorTask&);

private:
    std::string _channelFile;

    int _port;
}; // class ChannelFileMonitorTask

/// BolServiceServer
BolServiceServer::BolServiceServer(const std::string& bolName) 
                                   : _bolName(bolName), 
                                     _setAcceptor(false) {

}

BolServiceServer::~BolServiceServer() {
    if (_setAcceptor) {
        _reactor.removeEventHandler(_servSocket,
            Bmco::Observer<BolServiceServer, Bmco::Net::ReadableNotification>(*this, 
            &BolServiceServer::onAcceptor));
    }

    stopWorkThread();

    if (!_channelFile.empty()) {
        Bmco::File channelFile(_channelFile);
        if (channelFile.exists()) {
            channelFile.remove();
        }
    }    
}

bool BolServiceServer::start() {
    if (_bolName.empty()) {
        ERROR_LOG(0, "_bolName is empty");
        return false;
    }

    if (!startWorkThread()) {
        ERROR_LOG(0, "start work thread pool failed");
        return false;
    }

    try {        
        Bmco::Util::LayeredConfiguration& config = Bmco::Util::Application::instance().config();
        int port = config.getInt("BKernel.server.port", 2015);
        Bmco::Net::SocketAddress address("127.0.0.1", port);
        int times = 0;

	// _servSocket.setReusePort(true);
	 //_servSocket.setReuseAddress(true);
	// _servSocket.setLinger(true);
        while (true) {
            try {
                _servSocket.bind(address);
                break;
            }
            catch (Bmco::Exception& e) {
                ERROR_LOG(0, "occur exception: %s", e.displayText().c_str());                
            }
            catch (std::exception& e) {
                ERROR_LOG(0, "occur exception: %s", e.what());                
            }
            catch (...) {
                ERROR_LOG(0, "occur unknown exception");                
            }
            ERROR_LOG(0, "bind %s failed, try %d times, please check port", 
                      address.toString().c_str(), ++times);
            Bmco::Thread::sleep(1000); // sleep 1s
        }
                
        _servSocket.listen();
        ResponseNotify::instance().connect(address);

        _channelFile = Bmco::Path::temp() + "." + _bolName;
        Bmco::FileOutputStream fstream(_channelFile);
        fstream << port << std::endl;
        _pTaskManager->start(new ChannelFileMonitorTask("ChannelFileMonitorTask", _channelFile, port));       
        
        Bmco::Net::StreamSocket socket = _servSocket.acceptConnection();
        new ResponseSend(_reactor, socket, _responseQueue);               
    }
    catch (Bmco::Exception& e) {
        ERROR_LOG(0, "occur exception: %s", e.displayText().c_str());
        return false;
    }
    catch (std::exception& e) {
        ERROR_LOG(0, "occur exception: %s", e.what());
        return false;
    }
    catch (...) {
        ERROR_LOG(0, "occur unknown exception");
        return false;
    }    
    
    _reactor.addEventHandler(_servSocket,
        Bmco::Observer<BolServiceServer, Bmco::Net::ReadableNotification>(*this,
        &BolServiceServer::onAcceptor));
    _setAcceptor = true;

    _ioThread.start(_reactor);
    return true;
}

void BolServiceServer::stop() {
    ResponseNotify::instance().close();
    _reactor.stop();
    _ioThread.join();
    stopWorkThread();

    if (DBStatistics::enable()) {
        DBStatistics::instance().outputStatisticResult();
        PBStatistic::instance().outputStatisticResult();
        NetWorkStatistic::instance().outputStatisticResult();
    }
}

void BolServiceServer::onAcceptor(Bmco::Net::ReadableNotification* pNf) {
    if (NULL != pNf) {
        pNf->release();
    }

    try {
        Bmco::Net::StreamSocket acceptorSocket = _servSocket.acceptConnection();
        new BolServiceRequestHandler(_reactor, acceptorSocket, _requestQueue, _responseQueue);
    }
    catch (Bmco::Exception& e) {
        ERROR_LOG(0, "occur exception: %s", e.displayText().c_str());
    }
    catch (std::exception& e) {
        ERROR_LOG(0, "occur exception: %s", e.what());
    }
    catch (...) {
        ERROR_LOG(0, "occur unknown exception");
    }
}

bool BolServiceServer::startWorkThread() {
    try {
        Bmco::Util::LayeredConfiguration& config = Bmco::Util::Application::instance().config();
        std::string strThreadNum = config.getString("BKernel.workthread.num", "5");
        int workThreadNum = 5;
        if (!Bmco::NumberParser::tryParse(strThreadNum, workThreadNum)) {
            ERROR_LOG(0, "BKernel.workthread.num: %s can not parser to int", strThreadNum.c_str());
        }
        _pThreadPool = new Bmco::ThreadPool("BolServiceServerWorkThread",
            workThreadNum, workThreadNum + 1);
        _pTaskManager = new Bmco::TaskManager(*_pThreadPool);
        for (int index = 0; index < workThreadNum; ++index) {
            std::string name = "BolServiceServerWorkThread#" + Bmco::NumberFormatter::format(index);
            _pTaskManager->start(new WorkTask(name, _requestQueue, _responseQueue));
        }
        /*_pTaskManager->start(new BackupTask("BackupTask"));*/
        return true;
    }
    catch (Bmco::Exception& e) {
        ERROR_LOG(0, "occur exception: %s", e.displayText().c_str());
    }
    catch (std::exception& e) {
        ERROR_LOG(0, "occur exception: %s", e.what());
    }
    catch (...) {
        ERROR_LOG(0, "occur unknown exception");
    }

    return false;
}

void BolServiceServer::stopWorkThread() {    
    if (!_pThreadPool.isNull()) {
        if (!_pTaskManager.isNull()) {
            _pTaskManager->cancelAll();
            _requestQueue.wakeUpAll();
            _pTaskManager->joinAll();
            _pThreadPool = NULL;
            _pTaskManager = NULL;
        }
        else {
            _requestQueue.wakeUpAll();
            _pThreadPool->joinAll();
            _pThreadPool = NULL;
        }
    }
}

} // namespace BM35
