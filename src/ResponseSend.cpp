#include "ResponseSend.h"
#include "TaskNotification.h"
#include "BolServiceRequestHandler.h"
#include "BolServiceLog.h"
#include "DB.h"
#include "Bmco/Exception.h"

namespace BM35 {

const int ResponseSend::BUFFER_SIZE = 4096;

const int ResponseSend::PACKET_HEAR_SIZE = 4;

ResponseSend::ResponseSend(Bmco::Net::SocketReactor& reactor,
                           const Bmco::Net::StreamSocket& socket,
                           Bmco::NotificationQueue& responseQueue)
                           : _reactor(reactor), 
                             _socket(socket),
                             _responseQueue(responseQueue), 
                             _buffer(BUFFER_SIZE),
                             _statistic(false) {
    _reactor.addEventHandler(_socket,
        Bmco::Observer<ResponseSend, Bmco::Net::ReadableNotification>(
        *this, &ResponseSend::onRead));
    _reactor.addEventHandler(_socket,
        Bmco::Observer<ResponseSend, Bmco::Net::ShutdownNotification>(
        *this, &ResponseSend::onShutDown));
    /*_reactor.addEventHandler(_socket,
        Bmco::Observer<ResponseSend, Bmco::Net::TimeoutNotification>(
        *this, &ResponseSend::onTimeOut));*/

    if (DBStatistics::enable()) {
        try {
            _pTime = new Bmco::Timestamp;
            _statistic = true;
        }
        catch (...) {}
    }
}

ResponseSend::~ResponseSend() {
    _reactor.removeEventHandler(_socket,
        Bmco::Observer<ResponseSend, Bmco::Net::ReadableNotification>(
        *this, &ResponseSend::onRead));
    _reactor.removeEventHandler(_socket,
        Bmco::Observer<ResponseSend, Bmco::Net::ShutdownNotification>(
        *this, &ResponseSend::onShutDown));
    /*_reactor.removeEventHandler(_socket,
        Bmco::Observer<ResponseSend, Bmco::Net::TimeoutNotification>(
        *this, &ResponseSend::onTimeOut));*/
}

void ResponseSend::onRead(Bmco::Net::ReadableNotification* pNf) {
    if (NULL != pNf) {
        pNf->release();
    }

    try {
        char buf;
        _socket.receiveBytes(&buf, 1);
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

    sendResponse();
}

void ResponseSend::onShutDown(Bmco::Net::ShutdownNotification* pNf) {
    if (NULL != pNf) {
        pNf->release();
    }

    try {
        _socket.shutdown();
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
    _socket.close();

    delete this;
}

void ResponseSend::onTimeOut(Bmco::Net::TimeoutNotification* pNf) {
    if (NULL != pNf) {
        pNf->release();
    }
}

void ResponseSend::sendResponse() {
    while (!_responseQueue.empty()) {
        Bmco::Notification *pNf = _responseQueue.waitDequeueNotification();
        TaskNotification *pTaskNf = dynamic_cast<TaskNotification *>(pNf);
        if (NULL != pTaskNf) {
            try {
                BolServiceRequestHandler* pHandler =
                    HandlerManager::instance().getHandler(pTaskNf->socket());
                if (NULL != pHandler) {
                    const std::string& data = pTaskNf->data();
                    int dataLen = data.length();
                    if (0 == dataLen) {
                        ERROR_LOG(0, "response data is empty, close the request");
                        pHandler->shutdown();
                    }
                    else {
                        int totalLen = dataLen + PACKET_HEAR_SIZE;
                        if (totalLen > BUFFER_SIZE) {
                            _buffer.resize(totalLen, false);
                        }
                        int packetLen = htonl(dataLen);
                        memcpy(_buffer.begin(), &packetLen, PACKET_HEAR_SIZE);
                        memcpy(_buffer.begin() + PACKET_HEAR_SIZE, data.c_str(), dataLen);

                        if (_statistic) {
                            _pTime->update();
                        }
                        pHandler->socket().sendBytes(_buffer.begin(), totalLen);
                        if (_statistic) {
                            NetWorkStatistic::instance().statistics(true, _pTime->elapsed());
                        }
                    }
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

            pTaskNf->release();
        }
    }

    if (_buffer.size() > BUFFER_SIZE) {
        _buffer.resize(BUFFER_SIZE, false);
    }
}

} // namespace BM35
