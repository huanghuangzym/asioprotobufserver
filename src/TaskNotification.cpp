#include "TaskNotification.h"

namespace BM35 {

TaskNotification::TaskNotification(const std::string& data,
                                   Bmco::Net::Socket& socket)
                                   : _data(data), _socket(socket) {

}

TaskNotification::~TaskNotification() {

}

void TaskNotification::setData(const std::string& data) {
    _data = data;
}

const std::string& TaskNotification::data() {
    return _data;
}

Bmco::Net::Socket& TaskNotification::socket() {
    return _socket;
}

} // namespace BM35
