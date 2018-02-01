#include "BolServiceRequestHandler.h"
#include "BolServiceLog.h"
#include "DB.h"
#include "Bmco/Observer.h"
#include "Bmco/Exception.h"
#include "TaskNotification.h"
#include <cstring>

namespace BM35 {

/// BolServiceRequestHandler
const int BolServiceRequestHandler::BUFFER_SIZE = 4096;

const int BolServiceRequestHandler::PACKET_HEAR_SIZE = 4;

BolServiceRequestHandler::BolServiceRequestHandler(Bmco::Net::SocketReactor& reactor,
    const Bmco::Net::StreamSocket& streamSocket, Bmco::NotificationQueue& requestQueue,
    Bmco::NotificationQueue& responseQueue) 
    : _reactor(reactor), _socket(streamSocket), 
      _requestQueue(requestQueue), _responseQueue(responseQueue),
      _inBuffer(BUFFER_SIZE), _outBuffer(BUFFER_SIZE), _statistic(false) {
    _reactor.addEventHandler(_socket, Bmco::Observer<BolServiceRequestHandler,
        Bmco::Net::ReadableNotification>(*this, &BolServiceRequestHandler::onRead));
    /*_reactor.addEventHandler(_socket, Bmco::Observer<BolServiceRequestHandler,
        Bmco::Net::TimeoutNotification>(*this, &BolServiceRequestHandler::onTimeOut));*/
    _reactor.addEventHandler(_socket, Bmco::Observer<BolServiceRequestHandler,
        Bmco::Net::ShutdownNotification>(*this, &BolServiceRequestHandler::onShutDown));

    HandlerManager::instance().addHandler(_socket, this);

    if (DBStatistics::enable()) {
        try {
            _pTime = new Bmco::Timestamp;
            _statistic = true;
        }
        catch (...) {}
    }
}

BolServiceRequestHandler::~BolServiceRequestHandler() {
    _reactor.removeEventHandler(_socket, Bmco::Observer<BolServiceRequestHandler,
        Bmco::Net::ReadableNotification>(*this, &BolServiceRequestHandler::onRead));
    /*_reactor.removeEventHandler(_socket, Bmco::Observer<BolServiceRequestHandler,
        Bmco::Net::TimeoutNotification>(*this, &BolServiceRequestHandler::onTimeOut));*/
    _reactor.removeEventHandler(_socket, Bmco::Observer<BolServiceRequestHandler,
        Bmco::Net::ShutdownNotification>(*this, &BolServiceRequestHandler::onShutDown));

    HandlerManager::instance().deleteHandler(_socket);
}

void BolServiceRequestHandler::onRead(Bmco::Net::ReadableNotification* pNf) {
    if (NULL != pNf) {
        pNf->release();
    }
    //sendResponse();

    int inBytes = _socket.available();
    if (0 == inBytes) {
        shutdown();
        return;
    }

    if (inBytes > _inBuffer.available_at_end()) {
        _inBuffer.resize(_inBuffer.size() + inBytes, true);
    }
    else if ((inBytes + _inBuffer.used()) < BUFFER_SIZE &&
              _inBuffer.size() > BUFFER_SIZE) {
        _inBuffer.resize(BUFFER_SIZE, !_inBuffer.isEmpty());
    }

    if (_statistic) {
        _pTime->update();
    }
    int recvBytes = _socket.receiveBytes(_inBuffer.next(), inBytes);
    if (_statistic) {
        NetWorkStatistic::instance().statistics(false, _pTime->elapsed());
    }

    _inBuffer.advance(recvBytes);
    if (0 == recvBytes) {
        shutdown();
        return;
    }

    while (_inBuffer.used() > PACKET_HEAR_SIZE) {
        int packetLen;
        _inBuffer.peek(reinterpret_cast<char *>(&packetLen), PACKET_HEAR_SIZE);
        packetLen = ntohl(packetLen);

        if (0 == packetLen) {
            shutdown();
            return;
        }

        if (packetLen + PACKET_HEAR_SIZE <= _inBuffer.used()) {
            std::string data(_inBuffer.begin() + PACKET_HEAR_SIZE, packetLen);
            Bmco::Notification::Ptr pNf = new TaskNotification(data, _socket);
            _requestQueue.enqueueNotification(pNf);

            _inBuffer.drain(packetLen + PACKET_HEAR_SIZE);
        }
        else {
            return;
        }
    }
}

void BolServiceRequestHandler::onTimeOut(Bmco::Net::TimeoutNotification* pNf) {
    if (NULL != pNf) {
        pNf->release();
    }
    //sendResponse();
    //shutdown();
}

void BolServiceRequestHandler::onShutDown(Bmco::Net::ShutdownNotification* pNf) {
    if (NULL != pNf) {
        pNf->release();
    }
    //sendResponse();
    shutdown();
}

//void BolServiceRequestHandler::sendResponse() {
//    while (!_responseQueue.empty()) {
//        Bmco::Notification *pNf = _responseQueue.waitDequeueNotification();
//        TaskNotification *pTaskNf = dynamic_cast<TaskNotification *>(pNf);
//        if (NULL != pTaskNf) {
//            try {
//                const std::string& data = pTaskNf->data();
//                int dataLen = data.length();
//                int totalLen = dataLen + PACKET_HEAR_SIZE;
//                if (totalLen > BUFFER_SIZE) {
//                    _outBuffer.resize(totalLen, false);
//                }
//                int packetLen = htonl(dataLen);
//                memcpy(_outBuffer.begin(), &packetLen, PACKET_HEAR_SIZE);
//                memcpy(_outBuffer.begin() + PACKET_HEAR_SIZE, data.c_str(), dataLen); 
//
//                pTaskNf->socket().sendBytes(_outBuffer.begin(), totalLen);
//            }
//            catch (Bmco::Exception& e) {
//                ERROR_LOG(0, "occur exception: %s", e.displayText().c_str());
//            }
//            catch (std::exception& e) {
//                ERROR_LOG(0, "occur exception: %s", e.what());
//            }
//            catch (...) {
//                ERROR_LOG(0, "occur unknown exception");
//            }
//            pTaskNf->release();
//        }
//    }
//
//    if (_outBuffer.size() > BUFFER_SIZE) {
//        _outBuffer.resize(BUFFER_SIZE, false);
//    }
//}

void BolServiceRequestHandler::shutdown() {
    try {
        INFORMATION_LOG("socket shutdown");
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

Bmco::Net::StreamSocket& BolServiceRequestHandler::socket() {
    return _socket;
}


Bmco::Int32 HandlerManager::_ConnNum = 0;

/// HandlerManager
HandlerManager::HandlerManager() {

}

HandlerManager::~HandlerManager() {

}

HandlerManager& HandlerManager::instance() {
    static HandlerManager manager;
    return manager;
}

BolServiceRequestHandler* HandlerManager::getHandler(const Bmco::Net::Socket& socket) {
    HANDLERMAP::iterator itr = _handlerMap.find(socket);
    if (itr != _handlerMap.end()) {
        return itr->second;
    }
    else {
        return NULL;
    }
}

void HandlerManager::addHandler(const Bmco::Net::Socket& socket, 
                                BolServiceRequestHandler* pHandler) {
    HANDLERMAP::iterator itr = _handlerMap.find(socket);
    if (itr == _handlerMap.end()) {
        _handlerMap.insert(std::make_pair(socket, pHandler));
	_ConnNum++;
    }
}

void HandlerManager::deleteHandler(const Bmco::Net::Socket& socket) {
    HANDLERMAP::iterator itr = _handlerMap.find(socket);
    if (itr != _handlerMap.end()) {
        _handlerMap.erase(itr);
	_ConnNum--;
    }
}

} // namespce BM35
