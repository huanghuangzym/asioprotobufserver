#ifndef __WORKTASK_H
#define __WORKTASK_H

#include "Bmco/Task.h"
#include "Bmco/NotificationQueue.h"

namespace BM35 {

class WorkTask : public Bmco::Task {
public:
    WorkTask(const std::string& name,
             Bmco::NotificationQueue& requestQueue,
             Bmco::NotificationQueue& responseQueue);

    virtual ~WorkTask();

    virtual void runTask();

private:
    WorkTask();

    WorkTask(const WorkTask&);

    WorkTask& operator = (const WorkTask&);

private:
    Bmco::NotificationQueue& _requestQueue;

    Bmco::NotificationQueue& _responseQueue;    
}; // WorkTask

} // namespace BM35

#endif // __WORKTASK_H
