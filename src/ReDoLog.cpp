#include "ReDoLog.h"
#include "Bmco/Util/LayeredConfiguration.h"
#include "Bmco/Util/Application.h"
#include "Bmco/File.h"

namespace BM35 {

ReDoLog::ReDoLog(const std::string& path) : _fstream(path, std::ios::app), _path(path) {

}

ReDoLog::~ReDoLog() {

}

ReDoLog& ReDoLog::instance() {
    static ReDoLog *pReDoLog = NULL;
    if (NULL == pReDoLog) {
        Bmco::Util::LayeredConfiguration& config = Bmco::Util::Application::instance().config();
        std::string path = config.getString("redolog.path", "");
        static ReDoLog redoLog(path);
        pReDoLog = &redoLog;
    }

    return *pReDoLog;
}

bool ReDoLog::log(const std::string& sql) {
    try {
        Bmco::FastMutex::ScopedLock lock(_mtx);

        _fstream << sql;
        _fstream.flush();
        return _fstream.good();
    }
    catch (...) {
        return false;
    }
}

void ReDoLog::backup() {
    Bmco::FastMutex::ScopedLock lock(_mtx);

    _fstream.close();    
    std::string redoLogBackup = _path + ".bak";
    Bmco::File redoLog(_path);
    if (!redoLog.exists()) {
        return;
    }

    try {
        redoLog.renameTo(redoLogBackup);
    }
    catch (...) {}
    
    _fstream.open(_path, std::ios::app);
}

} // namespace BM35
