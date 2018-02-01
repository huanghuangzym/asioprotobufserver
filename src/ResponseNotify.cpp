#include "ResponseNotify.h"
#include "BolServiceLog.h"
#include "Bmco/Exception.h"

namespace BM35 {

ResponseNotify::ResponseNotify() {

}

ResponseNotify::~ResponseNotify() {

}

ResponseNotify& ResponseNotify::instance() {
    static ResponseNotify responseNotify;
    return responseNotify;
}

void ResponseNotify::connect(const Bmco::Net::SocketAddress& address) {
    _notifySocket.connect(address);
}

void ResponseNotify::notify() {
    try {
        char buf;
        _notifySocket.sendBytes(&buf, 1);
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

void ResponseNotify::close() {
    _notifySocket.close();
}

} // namespace BM35
