#ifndef __BOLSERVICESERVER_H
#define __BOLSERVICESERVER_H

#include "Bmco/Net/ServerSocket.h"
#include "Bmco/Net/SocketReactor.h"
#include "Bmco/NotificationQueue.h"
#include "Bmco/Net/SocketNotification.h"
#include "Bmco/SharedPtr.h"
#include "Bmco/TaskManager.h"
#include "Bmco/ThreadPool.h"
#include <string>

namespace BM35 {

class BolServiceServer {
public:
    BolServiceServer(const std::string& bolName);

    ~BolServiceServer();

    bool start();

    void stop();

    void onAcceptor(Bmco::Net::ReadableNotification* pNf);

private:
    BolServiceServer();

    BolServiceServer(const BolServiceServer&);

    BolServiceServer& operator = (const BolServiceServer&);

    bool startWorkThread();

    void stopWorkThread();

private:
    std::string _bolName;

    std::string _channelFile;

    bool _setAcceptor;

    Bmco::Net::ServerSocket _servSocket;

    Bmco::Net::SocketReactor _reactor;

    Bmco::Thread _ioThread;

    Bmco::NotificationQueue _requestQueue;

    Bmco::NotificationQueue _responseQueue;

    Bmco::SharedPtr<Bmco::TaskManager> _pTaskManager;

    Bmco::SharedPtr<Bmco::ThreadPool> _pThreadPool;
};

} // namespace BM35

#endif // __BOLSERVICESERVER_H
