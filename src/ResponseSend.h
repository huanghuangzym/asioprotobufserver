#ifndef __RESPONSESEND_H
#define __RESPONSESEND_H

#include "Bmco/NotificationQueue.h"
#include "Bmco/Net/StreamSocket.h"
#include "Bmco/Net/SocketNotification.h"
#include "Bmco/Net/SocketReactor.h"
#include "Bmco/SharedPtr.h"
#include "Bmco/Timestamp.h"

namespace BM35 {

class ResponseSend {
public:
    ResponseSend(Bmco::Net::SocketReactor& reactor, 
        const Bmco::Net::StreamSocket& socket,
        Bmco::NotificationQueue& responseQueue);

    ~ResponseSend();

    void onRead(Bmco::Net::ReadableNotification* pNf);

    void onShutDown(Bmco::Net::ShutdownNotification* pNf);

    void onTimeOut(Bmco::Net::TimeoutNotification* pNf);

private:
    ResponseSend();

    ResponseSend(const ResponseSend&);

    ResponseSend& operator = (const ResponseSend&);

    void sendResponse();

private:
    Bmco::Net::SocketReactor& _reactor;

    Bmco::Net::StreamSocket _socket;

    Bmco::NotificationQueue& _responseQueue;

    Bmco::Buffer<char> _buffer;

    Bmco::SharedPtr<Bmco::Timestamp> _pTime;

    bool _statistic;

    static const int BUFFER_SIZE;

    static const int PACKET_HEAR_SIZE;
}; // class ResponseSend

} // namespace BM35

#endif // __RESPONSESEND_H
