#ifndef __BOLSERVICEREQUESTHANDLER_H
#define __BOLSERVICEREQUESTHANDLER_H

#include "Bmco/Net/SocketReactor.h"
#include "Bmco/Net/StreamSocket.h"
#include "Bmco/Net/SocketNotification.h"
#include "Bmco/Net/Socket.h"
#include "Bmco/NotificationQueue.h"
#include "Bmco/Buffer.h"
#include "Bmco/FIFOBuffer.h"
#include "Bmco/SharedPtr.h"
#include "Bmco/Timestamp.h"
#include <map>

namespace BM35 {

class BolServiceRequestHandler {
public:
    BolServiceRequestHandler(Bmco::Net::SocketReactor& reactor,
        const Bmco::Net::StreamSocket& streamSocket, Bmco::NotificationQueue& requestQueue,
        Bmco::NotificationQueue& responseQueue);

    ~BolServiceRequestHandler();

    void onRead(Bmco::Net::ReadableNotification* pNf);

    void onTimeOut(Bmco::Net::TimeoutNotification* pNf);

    void onShutDown(Bmco::Net::ShutdownNotification* pNf);

    void shutdown();

    Bmco::Net::StreamSocket& socket();

private:
    BolServiceRequestHandler();

    BolServiceRequestHandler(const BolServiceRequestHandler&);

    BolServiceRequestHandler& operator = (const BolServiceRequestHandler&);

    void sendResponse();    

private:
    Bmco::Net::SocketReactor& _reactor;

    Bmco::Net::StreamSocket _socket;
       
    Bmco::NotificationQueue& _requestQueue;

    Bmco::NotificationQueue& _responseQueue;

    Bmco::FIFOBuffer _inBuffer;

    Bmco::Buffer<char> _outBuffer;

    Bmco::SharedPtr<Bmco::Timestamp> _pTime;

    bool _statistic;

    static const int BUFFER_SIZE;

    static const int PACKET_HEAR_SIZE;
}; // class BolServiceRequestHandler

class HandlerManager {
public:
    ~HandlerManager();

    static HandlerManager& instance();

    BolServiceRequestHandler* getHandler(const Bmco::Net::Socket& socket);

    void addHandler(const Bmco::Net::Socket& socket, BolServiceRequestHandler* pHandler);

    void deleteHandler(const Bmco::Net::Socket& socket);

	static Bmco::Int32 _ConnNum;

private:
    HandlerManager();

    HandlerManager(const HandlerManager&);

    HandlerManager& operator = (const HandlerManager&);

private:
    typedef std::map<Bmco::Net::Socket, BolServiceRequestHandler*> HANDLERMAP;
    HANDLERMAP _handlerMap;    
}; // class HandlerManger

} // namespace BM35

#endif // __BOLSERVICEREQUESTHANDLER_H
