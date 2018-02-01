#include "BackupTask.h"
#include "ReDoLog.h"
#include "DB.h"
#include "Bmco/File.h"
#include "Bmco/FileStream.h"
#include "Bmco/SharedPtr.h"
#include "Bmco/NumberParser.h"
#include "Bmco/Util/Application.h"
#include "Bmco/Util/LayeredConfiguration.h"

namespace BM35 {

BackupTask::BackupTask(const std::string& name) : Bmco::Task(name) {

}

BackupTask::~BackupTask() {

}

void BackupTask::runTask() {
    init();
    while (!isCancelled()) {
        try {
            if (needBackup()) {
                backup();
            }
        }
        catch (...) {}
        sleep(1000); // sleep 1s
    }
}

void BackupTask::init() {
    Bmco::Util::LayeredConfiguration& config = Bmco::Util::Application::instance().config();
    _backupDB = config.getString("dbbackup.name", "");
    _redoLogPath = config.getString("redolog.path", "");

    _redoLogSize = 0;
    std::string strRedoLogSize = config.getString("redolog.size", "");
    if (!strRedoLogSize.empty()) {
        int len = strRedoLogSize.length();
        if ('G' == strRedoLogSize[len - 1]) {
            std::string strSize = strRedoLogSize.substr(0, len - 1);
            if (Bmco::NumberParser::tryParseUnsigned64(strSize, _redoLogSize)) {
                _redoLogSize <<= 30;
            }
        }
        else if ('M' == strRedoLogSize[len - 1]) {
            std::string strSize = strRedoLogSize.substr(0, len - 1);
            if (Bmco::NumberParser::tryParseUnsigned64(strSize, _redoLogSize)) {
                _redoLogSize <<= 20;
            }
        }
        else if ('K' == strRedoLogSize[len - 1]) {
            std::string strSize = strRedoLogSize.substr(0, len - 1);
            if (Bmco::NumberParser::tryParseUnsigned64(strSize, _redoLogSize)) {
                _redoLogSize <<= 10;
            }
        }
        else {
            if (2 <= len && 'B' == strRedoLogSize[len - 1]) {
                if ('G' == strRedoLogSize[len - 2]) {
                    std::string strSize = strRedoLogSize.substr(0, len - 2);
                    if (Bmco::NumberParser::tryParseUnsigned64(strSize, _redoLogSize)) {
                        _redoLogSize <<= 30;
                    }
                }
                else if ('M' == strRedoLogSize[len - 2]) {
                    std::string strSize = strRedoLogSize.substr(0, len - 2);
                    if (Bmco::NumberParser::tryParseUnsigned64(strSize, _redoLogSize)) {
                        _redoLogSize <<= 20;
                    }
                }
                else if ('K' == strRedoLogSize[len - 2]) {
                    std::string strSize = strRedoLogSize.substr(0, len - 2);
                    if (Bmco::NumberParser::tryParseUnsigned64(strSize, _redoLogSize)) {
                        _redoLogSize <<= 10;
                    }
                }
            }
            else {
                Bmco::NumberParser::tryParseUnsigned64(strRedoLogSize, _redoLogSize);
            }
        }
    }
    if (0 == _redoLogSize) {
        _redoLogSize = 2 << 20;
    }

    std::string strInterval = config.getString("redolog.interval", "");
    if (!Bmco::NumberParser::tryParse(strInterval, _intervalSeconds)) {
        _intervalSeconds = 24 * 60 * 60; // 24hour;
    }
}

bool BackupTask::needBackup() {
    Bmco::File redoLog(_redoLogPath);
    if (!redoLog.exists()) {
        return false;
    }

    if (_time.isElapsed(_intervalSeconds * 1000000)) {
        return true;
    }

    Bmco::UInt64 redoLogSize = redoLog.getSize();
    if (redoLogSize >= _redoLogSize) {
        return true;
    }

    return false;
}

void BackupTask::backup() {
    _time.update();

    ReDoLog::instance().backup();

    std::string redoLogBackup = _redoLogPath + ".bak";
    Bmco::File redoLog(redoLogBackup);
    if (!redoLog.exists()) {
        return;
    }

    Bmco::File backupFile(_backupDB);
    if (_backupDB.empty() || !backupFile.exists()) {
        redoLog.remove();
        return;
    }

    Bmco::SharedPtr<DB> pDB = new SQLiteDB(_backupDB);
    if (pDB.isNull() || !pDB->connect()) {
        redoLog.remove();
        return;
    }

    std::string line;
    Bmco::FileInputStream fstream(redoLogBackup);
    pDB->beginTransaction();
    while (std::getline(fstream, line)) {
        pDB->execute(line);
    }
    pDB->commitTransaction();

    redoLog.remove();
}

} // namespace BM35
