#ifndef __BACKUPTASK_H
#define __BACKUPTASK_H

#include "Bmco/Task.h"
#include "Bmco/Timestamp.h"
#include <string>

namespace BM35 {

class BackupTask : public Bmco::Task {
public:
    BackupTask(const std::string& name);

    virtual ~BackupTask();

    virtual void runTask();

private:
    BackupTask();

    BackupTask(const BackupTask&);

    BackupTask& operator = (const BackupTask&);

    void init();

    bool needBackup();

    void backup();

private:
    std::string _backupDB;

    std::string _redoLogPath;

    Bmco::UInt64 _redoLogSize;

    int _intervalSeconds;

    Bmco::Timestamp _time;
}; // BackupTask

} // namespace BM35

#endif // __BACKUPTASK_H
