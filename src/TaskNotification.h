#ifndef __TASKNOTIFICATION_H
#define __TASKNOTIFICATION_H

#include "Bmco/Notification.h"
#include "Bmco/Net/Socket.h"
#include <string>

namespace BM35 {

class TaskNotification : public Bmco::Notification {
public:
    TaskNotification(const std::string& data, Bmco::Net::Socket& socket);

    virtual ~TaskNotification();

    void setData(const std::string& data);

    const std::string& data();

    Bmco::Net::Socket& socket();

private:
    TaskNotification();

    TaskNotification(const TaskNotification&);

    TaskNotification& operator = (const TaskNotification&);

private:
    std::string _data;

    Bmco::Net::Socket _socket;
}; // class TaskNotification

} // namespace BM35

#endif // __TASKNOTIFICATION_H
