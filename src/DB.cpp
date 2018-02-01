#include "DB.h"
#include "BolServiceLog.h"
#include "Bmco/File.h"
#include "Bmco/Path.h"
#include "Bmco/Exception.h"
#include <sstream>
#include <cstdio>
#include <cstring>

namespace BM35 {
/// DB
DB::DB()
    : _affectRow(0) {

} 

DB::~DB() {

}

/// SQLiteDB
SQLiteDB::SQLiteDB(const std::string& dbConnect)
    : _dbConnect(dbConnect), _db(NULL), _stmt(NULL), _connected(false) {
    
}

SQLiteDB::~SQLiteDB() {
    if (NULL != _stmt) {
        sqlite3_finalize(_stmt);
    }

    if (NULL != _db) {
        sqlite3_close(_db);
    }    
}

bool SQLiteDB::connect() {
    if (_connected) {
        return true;
    }

    int ret = sqlite3_open(_dbConnect.c_str(), &_db);
    if (SQLITE_OK == ret) {
        _connected = true;
        return true;
    }
    else {
        ERROR_LOG(0, "open db: %s failed, return: %d", _dbConnect.c_str(), ret);
        _db = NULL;
        return false;
    }
}

bool SQLiteDB::execute(const std::string& sql) {
    if (NULL == _db) {
        return false;
    }

    char *errmsg;
    int ret = sqlite3_exec(_db, sql.c_str(), NULL, NULL, &errmsg);
    if (SQLITE_OK == ret) {
        _affectRow = sqlite3_changes(_db);
        DEBUG_LOG("execute sql: %s succeed, affect row num: %d", sql.c_str(), _affectRow);
        return true;
    }
    else {
        ERROR_LOG(0, "execute sql: %s failed, return: %d, errmsg: %s", sql.c_str(), ret, errmsg);
        return false;
    }
}

bool SQLiteDB::beginTransaction() { 
    if (!_mtx.tryLock(5000)) { // 5s
        ERROR_LOG(0, "get lock failed due to time out 5 seconds");
        return false;
    }
    
    if (NULL != _db) {
        char *errmsg;
        sqlite3_exec(_db, "begin transaction", NULL, NULL, &errmsg);
    }

    return true;
}

void SQLiteDB::commitTransaction() {
    if (NULL != _db) {
        char *errmsg;
        sqlite3_exec(_db, "commit transaction", NULL, NULL, &errmsg);
    }   

    _mtx.unlock();
}

void SQLiteDB::rollbackTransaction() {
    if (NULL != _db) {
        char *errmsg;
        sqlite3_exec(_db, "rollback transaction", NULL, NULL, &errmsg);
    }

    _mtx.unlock();
}

int SQLiteDB::affectRow() {
    if (NULL == _db) {
        ERROR_LOG(0, "_db is NULL");
        return -1;
    }
    else {
        return _affectRow;
    }    
}

int SQLiteDB::fieldNum() {
    if (NULL == _stmt) {
        ERROR_LOG(0, "_stmt is NULL");
        return -1;
    }

    return sqlite3_column_count(_stmt);
}

bool SQLiteDB::select(const std::string& sql) {
    if (NULL == _db) {
        return false;
    }

    if (NULL != _stmt) {
        sqlite3_finalize(_stmt);
    }

    int ret = sqlite3_prepare(_db, sql.c_str(), sql.length(), &_stmt, NULL);
    if (SQLITE_OK == ret) {
        DEBUG_LOG("prepare sql: %s succeed", sql.c_str());
        return true;
    }
    else {
        ERROR_LOG(0, "prepare sql: %s failed, return: %d", sql.c_str(), ret);
        return false;
    }
}

bool SQLiteDB::next() {
    if (NULL == _stmt) {
        return false;
    }

    while (true) {
        int ret = sqlite3_step(_stmt);
        if (SQLITE_ROW == ret) {
            return true;
        }
        else if (SQLITE_DONE == ret) {
            sqlite3_finalize(_stmt);
            _stmt = NULL;
            return false;
        }
    }    
}

std::string SQLiteDB::fieldName(int index) {
    std::string fieldName;
    if (NULL != _stmt) {
        const char *name = sqlite3_column_origin_name(_stmt, index);
        if (NULL != name) {
            fieldName = name;
        }        
    }
    
    return fieldName;
}

std::string SQLiteDB::fieldValue(int index) {
    std::string fieldValue;
    if (NULL != _stmt) {
        const char *value = (const char *)sqlite3_column_text(_stmt, index);
        if (NULL != value) {
            fieldValue = value;
        }
    }

    return fieldValue;
}

/// MemorySQLiteDB
SQLiteDB MemorySQLiteDB::_db(SQLITE_MEMORY_TABLE);

bool MemorySQLiteDB::_connected = false;

MemorySQLiteDB::MemorySQLiteDB() {
    
}

MemorySQLiteDB::~MemorySQLiteDB() {
    
}

bool MemorySQLiteDB::connect() {
    if (_connected) {
        return true;
    }
    else if (_db.connect()) {
        _connected = true;
        return true;
    }
    else {
        return false;
    }
}

bool MemorySQLiteDB::execute(const std::string& sql) {
    return _db.execute(sql);
}

bool MemorySQLiteDB::beginTransaction() {
    return _db.beginTransaction();
}

void MemorySQLiteDB::commitTransaction() {
    _db.commitTransaction();
}

void MemorySQLiteDB::rollbackTransaction() {
    _db.rollbackTransaction();
}

int MemorySQLiteDB::affectRow() {
    return _db.affectRow();
}

int MemorySQLiteDB::fieldNum() {
    return _db.fieldNum();
}

bool MemorySQLiteDB::select(const std::string& sql) {
    return _db.select(sql);
}

bool MemorySQLiteDB::next() {
    return _db.next();
}

std::string MemorySQLiteDB::fieldName(int index) {
    return _db.fieldName(index);
}

std::string MemorySQLiteDB::fieldValue(int index) {
    return _db.fieldValue(index);
}

/// DBManager
DBManager::DBManager() {

}

DBManager::~DBManager() {
    _dbMap.clear();
}

DBManager& DBManager::instance() {
    static DBManager manager;
    return manager;
}

DB* DBManager::getDB(DB_TYPE dbType, const std::string& dbConnect) {
    Bmco::Mutex::ScopedLock lock(_mtx);

    if (SQLITE_DB == dbType) {
        std::string strConnect = getFullName(dbConnect);
        DBMAP::iterator itr = _dbMap.find(strConnect);
        if (itr == _dbMap.end()) {
            try {
                Bmco::SharedPtr<DB> pDB;
                if (SQLITE_MEMORY_TABLE == strConnect) {
                    pDB = new MemorySQLiteDB;
                }
                else {
                    pDB = new SQLiteDB(strConnect);
                }
                _dbMap.insert(std::make_pair(strConnect, pDB));
                return pDB.get();
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
        else {
            return itr->second.get();
        }
    }
    
    return NULL;
}

std::string DBManager::getFullName(const std::string& name) {
    try {
        Bmco::File file(name);
        if (!file.exists()) {
            return name;
        }

        Bmco::Path path(name);
        path.makeAbsolute();
        return path.toString();
    }
    catch (Bmco::Exception& e) {
        DEBUG_LOG("occur exception: %s", e.displayText().c_str());
    }
    catch (std::exception& e) {
        DEBUG_LOG("occur exception: %s", e.what());
    }
    catch (...) {
        DEBUG_LOG("occur unknown exception");
    }
    
    return name;
}

/// DBStatisticsTool
DBStatisticsTool::DBStatisticsTool(const std::string& operatorName, const std::string& tableName)
                                   : _operatorName(operatorName), _tableName(tableName) {
    if (DBStatistics::enable()) {
        _pNow = new Bmco::Timestamp;
    }
}

DBStatisticsTool::~DBStatisticsTool() {
    if (DBStatistics::enable() && !_pNow.isNull()) {
        DBStatistics::instance().statistics(_operatorName, _tableName, _pNow->elapsed());
    }
}

/// DBStatistics
bool DBStatistics::_enable;
bool DBStatistics::_judgeEnable = false;

const std::string DBStatistics::ADD_OPERATOR     = "add";
const std::string DBStatistics::QUERY_OPERATOR   = "query";
const std::string DBStatistics::ERASE_OPERATOR   = "erase";
const std::string DBStatistics::UPDATE_OPERATOR  = "update";
const std::string DBStatistics::REPLACE_OPERATOR = "replace";


DBStatistics::DBStatistics() {

}

DBStatistics::~DBStatistics() {

}

DBStatistics& DBStatistics::instance() {
    static DBStatistics statistics;
    return statistics;
}

bool DBStatistics::enable() {
    if (!_judgeEnable) {
        std::string swichFileName = Bmco::Path::temp() + "bolservice_statistics";
        Bmco::File switchFile(swichFileName);
        if (switchFile.exists()) {
            _enable = true;
        }
        else {
            _enable = false;
        }
        _judgeEnable = true;
    }    

    return _enable;
}

void DBStatistics::statistics(const std::string& operatorName, const std::string& tableName, 
                              const Bmco::Int64& cost) {
    if (ADD_OPERATOR == operatorName) {
        Bmco::FastMutex::ScopedLock lock(_addMtx);
        DBSTATISTICMAP::iterator itr = _addMap.find(tableName);
        if (itr == _addMap.end()) {
            Bmco::SharedPtr<DBStatisticData> pData = new DBStatisticData;
            pData->totalCost = cost;
            pData->maxCost   = cost;
            pData->minCost   = cost;            
            pData->times     = 1;
            _addMap.insert(std::make_pair(tableName, pData));
        }
        else {
            itr->second->totalCost += cost;
            if (cost > itr->second->maxCost) {
                itr->second->maxCost = cost;
            }
            if (cost < itr->second->minCost) {
                itr->second->minCost = cost;
            }
            itr->second->times += 1;
        }
    }
    else if (QUERY_OPERATOR == operatorName) {
        Bmco::FastMutex::ScopedLock lock(_queryMtx);
        DBSTATISTICMAP::iterator itr = _queryMap.find(tableName);
        if (itr == _queryMap.end()) {
            Bmco::SharedPtr<DBStatisticData> pData = new DBStatisticData;
            pData->totalCost = cost;
            pData->maxCost   = cost;
            pData->minCost   = cost;
            pData->times     = 1;
            _queryMap.insert(std::make_pair(tableName, pData));
        }
        else {
            itr->second->totalCost += cost;
            if (cost > itr->second->maxCost) {
                itr->second->maxCost = cost;
            }
            if (cost < itr->second->minCost) {
                itr->second->minCost = cost;
            }
            itr->second->times += 1;
        }
    }
    else if (ERASE_OPERATOR == operatorName) {
        Bmco::FastMutex::ScopedLock lock(_eraseMtx);
        DBSTATISTICMAP::iterator itr = _eraseMap.find(tableName);
        if (itr == _eraseMap.end()) {
            Bmco::SharedPtr<DBStatisticData> pData = new DBStatisticData;
            pData->totalCost = cost;
            pData->maxCost   = cost;
            pData->minCost   = cost;
            pData->times     = 1;
            _eraseMap.insert(std::make_pair(tableName, pData));
        }
        else {
            itr->second->totalCost += cost;
            if (cost > itr->second->maxCost) {
                itr->second->maxCost = cost;
            }
            if (cost < itr->second->minCost) {
                itr->second->minCost = cost;
            }
            itr->second->times += 1;
        }
    }
    else if (UPDATE_OPERATOR == operatorName) {
        Bmco::FastMutex::ScopedLock lock(_updateMtx);
        DBSTATISTICMAP::iterator itr = _updateMap.find(tableName);
        if (itr == _updateMap.end()) {
            Bmco::SharedPtr<DBStatisticData> pData = new DBStatisticData;
            pData->totalCost = cost;
            pData->maxCost   = cost;
            pData->minCost   = cost;
            pData->times     = 1;
            _updateMap.insert(std::make_pair(tableName, pData));
        }
        else {
            itr->second->totalCost += cost;
            if (cost > itr->second->maxCost) {
                itr->second->maxCost = cost;
            }
            if (cost < itr->second->minCost) {
                itr->second->minCost = cost;
            }
            itr->second->times += 1;
        }
    }
    else if (REPLACE_OPERATOR == operatorName) {
        Bmco::FastMutex::ScopedLock lock(_replaceMtx);
        DBSTATISTICMAP::iterator itr = _replaceMap.find(tableName);
        if (itr == _replaceMap.end()) {
            Bmco::SharedPtr<DBStatisticData> pData = new DBStatisticData;
            pData->totalCost = cost;
            pData->maxCost = cost;
            pData->minCost = cost;
            pData->times = 1;
            _replaceMap.insert(std::make_pair(tableName, pData));
        }
        else {
            itr->second->totalCost += cost;
            if (cost > itr->second->maxCost) {
                itr->second->maxCost = cost;
            }
            if (cost < itr->second->minCost) {
                itr->second->minCost = cost;
            }
            itr->second->times += 1;
        }
    }
}

#ifdef BMCO_OS_FAMILY_WINDOWS
#define SNPRINTF _snprintf
#else
#define SNPRINTF snprintf
#endif

void DBStatistics::outputStatisticResult() {
    /*std::ostringstream str;
    str << "DB statistics result: \n";

    if (!_addMap.empty()) {
        str << "add operator: \n";
        str << "\ttablename\ttotal_cost\ttimes\tavg_cost\tmax_cost\tmin_cost\n";
        DBSTATISTICMAP::iterator itr = _addMap.begin();
        for (; itr != _addMap.end(); ++itr) {
            Bmco::Int64 avg = itr->second->totalCost / itr->second->times;
            str << "\t" << itr->first << "\t"
                << itr->second->totalCost / 1000 << "." << itr->second->totalCost % 1000 << "\t"
                << itr->second->times << "\t"
                << avg / 1000 << "." << avg % 1000 << "\t"
                << itr->second->maxCost / 1000 << "." << itr->second->maxCost % 1000 << "\t"
                << itr->second->minCost / 1000 << "." << itr->second->minCost % 1000 << "\n";
        }
    }

    if (!_queryMap.empty()) {
        str << "query operator: \n";
        str << "\ttablename\ttotal_cost\ttimes\tavg_cost\tmax_cost\tmin_cost\n";
        DBSTATISTICMAP::iterator itr = _queryMap.begin();
        for (; itr != _queryMap.end(); ++itr) {
            Bmco::Int64 avg = itr->second->totalCost / itr->second->times;
            str << "\t" << itr->first << "\t"
                << itr->second->totalCost / 1000 << "." << itr->second->totalCost % 1000 << "\t"
                << itr->second->times << "\t"
                << avg / 1000 << "." << avg % 1000 << "\t"
                << itr->second->maxCost / 1000 << "." << itr->second->maxCost % 1000 << "\t"
                << itr->second->minCost / 1000 << "." << itr->second->minCost % 1000 << "\n";
        }
    }

    if (!_eraseMap.empty()) {
        str << "erase operator: \n";
        str << "\ttablename\ttotal_cost\ttimes\tavg_cost\tmax_cost\tmin_cost\n";
        DBSTATISTICMAP::iterator itr = _eraseMap.begin();
        for (; itr != _eraseMap.end(); ++itr) {
            Bmco::Int64 avg = itr->second->totalCost / itr->second->times;
            str << "\t" << itr->first << "\t"
                << itr->second->totalCost / 1000 << "." << itr->second->totalCost % 1000 << "\t"
                << itr->second->times << "\t"
                << avg / 1000 << "." << avg % 1000 << "\t"
                << itr->second->maxCost / 1000 << "." << itr->second->maxCost % 1000 << "\t"
                << itr->second->minCost / 1000 << "." << itr->second->minCost % 1000 << "\n";
        }
    }

    if (!_updateMap.empty()) {
        str << "update operator: \n";
        str << "\ttablename\ttotal_cost\ttimes\tavg_cost\tmax_cost\tmin_cost\n";
        DBSTATISTICMAP::iterator itr = _updateMap.begin();
        for (; itr != _updateMap.end(); ++itr) {
            Bmco::Int64 avg = itr->second->totalCost / itr->second->times;
            str << "\t" << itr->first << "\t"
                << itr->second->totalCost / 1000 << "." << itr->second->totalCost % 1000 << "\t"
                << itr->second->times << "\t"
                << avg / 1000 << "." << avg % 1000 << "\t"
                << itr->second->maxCost / 1000 << "." << itr->second->maxCost % 1000 << "\t"
                << itr->second->minCost / 1000 << "." << itr->second->minCost % 1000 << "\n";
        }
    }

    if (!_replaceMap.empty()) {
        str << "replace operator: \n";
        str << "\ttablename\ttotal_cost\ttimes\tavg_cost\tmax_cost\tmin_cost\n";
        DBSTATISTICMAP::iterator itr = _replaceMap.begin();
        for (; itr != _replaceMap.end(); ++itr) {
            Bmco::Int64 avg = itr->second->totalCost / itr->second->times;
            str << "\t" << itr->first << "\t"
                << itr->second->totalCost / 1000 << "." << itr->second->totalCost % 1000 << "\t"
                << itr->second->times << "\t"
                << avg / 1000 << "." << avg % 1000 << "\t"
                << itr->second->maxCost / 1000 << "." << itr->second->maxCost % 1000 << "\t"
                << itr->second->minCost / 1000 << "." << itr->second->minCost % 1000 << "\n";
        }
    }
    printf("%s", str.str().c_str());
    INFORMATION_LOG("%s", str.str().c_str());*/

    char *buf = NULL;
    try {
        int blockSize = 1 << 12;
        buf = (char *)malloc(blockSize);
        if (NULL == buf) {
            return;
        }

        int used = 0, len, remain = blockSize - 1;
        len = SNPRINTF(buf + used, remain, "DB statistics result: \n");
        if (0 > len) {
            free(buf);
            return;
        }
        used += len;
        remain -= len;

        int minLen = 77;
        char totalCost[12];
        char avgCost[12];
        char maxCost[12];
        char minCost[12];
        if (!_addMap.empty()) {
            if (minLen > remain) {
                remain += blockSize;
                char *new_buf = (char *)realloc(buf, blockSize * 2);
                if (NULL == new_buf) {
                    free(buf);
                    return;
                }
                buf = new_buf;
            }
            len = SNPRINTF(buf + used, remain, "add operator: \n");
            if (0 > len) {
                free(buf);
                return;
            }
            used += len;
            remain -= len;

            if (minLen > remain) {
                remain += blockSize;
                char *new_buf = (char *)realloc(buf, blockSize * 2);
                if (NULL == new_buf) {
                    free(buf);
                    return;
                }
                buf = new_buf;
            }
            len = SNPRINTF(buf + used, remain, "\t%-20s %-10s %-10s %-10s %-10s %-10s\n",
                           "tablename", "total_cost", "times", "avg_cost", "max_cost", "min_cost");
            if (0 > len) {
                free(buf);
                return;
            }
            used += len;
            remain -= len;

            DBSTATISTICMAP::iterator itr = _addMap.begin();
            for (; itr != _addMap.end(); ++itr) {
                if (minLen > remain) {
                    remain += blockSize;
                    char *new_buf = (char *)realloc(buf, blockSize * 2);
                    if (NULL == new_buf) {
                        free(buf);
                        return;
                    }
                    buf = new_buf;
                }
                memset(totalCost, 0, 12);
                SNPRINTF(totalCost, 11, "%ld.%03d", itr->second->totalCost / 1000,
                         int(itr->second->totalCost % 1000));
                Bmco::Int64 avg = itr->second->totalCost / itr->second->times;
                memset(avgCost, 0, 12);                
                SNPRINTF(avgCost, 11, "%ld.%03d", avg / 1000, int(avg % 1000));                    
                memset(maxCost, 0, 12);
                SNPRINTF(maxCost, 11, "%ld.%03d", itr->second->maxCost / 1000,
                         int(itr->second->maxCost % 1000));
                memset(minCost, 0, 12);
                SNPRINTF(minCost, 11, "%ld.%03d", itr->second->minCost / 1000,
                         int(itr->second->minCost % 1000));
                len = SNPRINTF(buf + used, remain, "\t%-20s %-10s %-10ld %-10s %-10s %-10s\n",
                               itr->first.c_str(), totalCost, itr->second->times, 
                               avgCost, maxCost, minCost);
                if (0 > len) {
                    free(buf);
                    return;
                }
                used += len;
                remain -= len;
            }
        }

        if (!_queryMap.empty()) {
            if (minLen > remain) {
                remain += blockSize;
                char *new_buf = (char *)realloc(buf, blockSize * 2);
                if (NULL == new_buf) {
                    free(buf);
                    return;
                }
                buf = new_buf;
            }
            len = SNPRINTF(buf + used, remain, "query operator: \n");
            if (0 > len) {
                free(buf);
                return;
            }
            used += len;
            remain -= len;

            if (minLen > remain) {
                remain += blockSize;
                char *new_buf = (char *)realloc(buf, blockSize * 2);
                if (NULL == new_buf) {
                    free(buf);
                    return;
                }
                buf = new_buf;
            }
            len = SNPRINTF(buf + used, remain, "\t%-20s %-10s %-10s %-10s %-10s %-10s\n",
                           "tablename", "total_cost", "times", "avg_cost", "max_cost", "min_cost");
            if (0 > len) {
                free(buf);
                return;
            }
            used += len;
            remain -= len;

            DBSTATISTICMAP::iterator itr = _queryMap.begin();
            for (; itr != _queryMap.end(); ++itr) {
                if (minLen > remain) {
                    remain += blockSize;
                    char *new_buf = (char *)realloc(buf, blockSize * 2);
                    if (NULL == new_buf) {
                        free(buf);
                        return;
                    }
                    buf = new_buf;
                }
                memset(totalCost, 0, 12);
                SNPRINTF(totalCost, 11, "%ld.%03d", itr->second->totalCost / 1000,
                         int(itr->second->totalCost % 1000));
                Bmco::Int64 avg = itr->second->totalCost / itr->second->times;
                memset(avgCost, 0, 12);
                SNPRINTF(avgCost, 11, "%ld.%03d", avg / 1000, int(avg % 1000));
                memset(maxCost, 0, 12);
                SNPRINTF(maxCost, 11, "%ld.%03d", itr->second->maxCost / 1000,
                         int(itr->second->maxCost % 1000));
                memset(minCost, 0, 12);
                SNPRINTF(minCost, 11, "%ld.%03d", itr->second->minCost / 1000,
                         int(itr->second->minCost % 1000));
                len = SNPRINTF(buf + used, remain, "\t%-20s %-10s %-10ld %-10s %-10s %-10s\n",
                               itr->first.c_str(), totalCost, itr->second->times,
                               avgCost, maxCost, minCost);
                if (0 > len) {
                    free(buf);
                    return;
                }
                used += len;
                remain -= len;
            }
        }

        if (!_eraseMap.empty()) {
            if (minLen > remain) {
                remain += blockSize;
                char *new_buf = (char *)realloc(buf, blockSize * 2);
                if (NULL == new_buf) {
                    free(buf);
                    return;
                }
                buf = new_buf;
            }
            len = SNPRINTF(buf + used, remain, "erase operator: \n");
            if (0 > len) {
                free(buf);
                return;
            }
            used += len;
            remain -= len;

            if (minLen > remain) {
                remain += blockSize;
                char *new_buf = (char *)realloc(buf, blockSize * 2);
                if (NULL == new_buf) {
                    free(buf);
                    return;
                }
                buf = new_buf;
            }
            len = SNPRINTF(buf + used, remain, "\t%-20s %-10s %-10s %-10s %-10s %-10s\n",
                           "tablename", "total_cost", "times", "avg_cost", "max_cost", "min_cost");
            if (0 > len) {
                free(buf);
                return;
            }
            used += len;
            remain -= len;

            DBSTATISTICMAP::iterator itr = _eraseMap.begin();
            for (; itr != _eraseMap.end(); ++itr) {
                if (minLen > remain) {
                    remain += blockSize;
                    char *new_buf = (char *)realloc(buf, blockSize * 2);
                    if (NULL == new_buf) {
                        free(buf);
                        return;
                    }
                    buf = new_buf;
                }
                memset(totalCost, 0, 12);
                SNPRINTF(totalCost, 11, "%ld.%03d", itr->second->totalCost / 1000,
                         int(itr->second->totalCost % 1000));
                Bmco::Int64 avg = itr->second->totalCost / itr->second->times;
                memset(avgCost, 0, 12);
                SNPRINTF(avgCost, 11, "%ld.%03d", avg / 1000, int(avg % 1000));
                memset(maxCost, 0, 12);
                SNPRINTF(maxCost, 11, "%ld.%03d", itr->second->maxCost / 1000,
                         int(itr->second->maxCost % 1000));
                memset(minCost, 0, 12);
                SNPRINTF(minCost, 11, "%ld.%03d", itr->second->minCost / 1000,
                         int(itr->second->minCost % 1000));
                len = SNPRINTF(buf + used, remain, "\t%-20s %-10s %-10ld %-10s %-10s %-10s\n",
                               itr->first.c_str(), totalCost, itr->second->times,
                               avgCost, maxCost, minCost);
                if (0 > len) {
                    free(buf);
                    return;
                }
                used += len;
                remain -= len;
            }
        }

        if (!_updateMap.empty()) {
            if (minLen > remain) {
                remain += blockSize;
                char *new_buf = (char *)realloc(buf, blockSize * 2);
                if (NULL == new_buf) {
                    free(buf);
                    return;
                }
                buf = new_buf;
            }
            len = SNPRINTF(buf + used, remain, "update operator: \n");
            if (0 > len) {
                free(buf);
                return;
            }
            used += len;
            remain -= len;
        
            if (minLen > remain) {
                remain += blockSize;
                char *new_buf = (char *)realloc(buf, blockSize * 2);
                if (NULL == new_buf) {
                    free(buf);
                    return;
                }
                buf = new_buf;
            }
            len = SNPRINTF(buf + used, remain, "\t%-20s %-10s %-10s %-10s %-10s %-10s\n",
                           "tablename", "total_cost", "times", "avg_cost", "max_cost", "min_cost");
            if (0 > len) {
                free(buf);
                return;
            }
            used += len;
            remain -= len;
        
            DBSTATISTICMAP::iterator itr = _updateMap.begin();
            for (; itr != _updateMap.end(); ++itr) {
                if (minLen > remain) {
                    remain += blockSize;
                    char *new_buf = (char *)realloc(buf, blockSize * 2);
                    if (NULL == new_buf) {
                        free(buf);
                        return;
                    }
                    buf = new_buf;
                }
                memset(totalCost, 0, 12);
                SNPRINTF(totalCost, 11, "%ld.%03d", itr->second->totalCost / 1000,
                         int(itr->second->totalCost % 1000));
                Bmco::Int64 avg = itr->second->totalCost / itr->second->times;
                memset(avgCost, 0, 12);
                SNPRINTF(avgCost, 11, "%ld.%03d", avg / 1000, int(avg % 1000));
                memset(maxCost, 0, 12);
                SNPRINTF(maxCost, 11, "%ld.%03d", itr->second->maxCost / 1000,
                         int(itr->second->maxCost % 1000));
                memset(minCost, 0, 12);
                SNPRINTF(minCost, 11, "%ld.%03d", itr->second->minCost / 1000,
                         int(itr->second->minCost % 1000));
                len = SNPRINTF(buf + used, remain, "\t%-20s %-10s %-10ld %-10s %-10s %-10s\n",
                               itr->first.c_str(), totalCost, itr->second->times,
                               avgCost, maxCost, minCost);
                if (0 > len) {
                    free(buf);
                    return;
                }
                used += len;
                remain -= len;
            }
        }

        if (!_replaceMap.empty()) {
            if (minLen > remain) {
                remain += blockSize;
                char *new_buf = (char *)realloc(buf, blockSize * 2);
                if (NULL == new_buf) {
                    free(buf);
                    return;
                }
                buf = new_buf;
            }
            len = SNPRINTF(buf + used, remain, "replace operator: \n");
            if (0 > len) {
                free(buf);
                return;
            }
            used += len;
            remain -= len;
        
            if (minLen > remain) {
                remain += blockSize;
                char *new_buf = (char *)realloc(buf, blockSize * 2);
                if (NULL == new_buf) {
                    free(buf);
                    return;
                }
                buf = new_buf;
            }
            len = SNPRINTF(buf + used, remain, "\t%-20s %-10s %-10s %-10s %-10s %-10s\n",
                           "tablename", "total_cost", "times", "avg_cost", "max_cost", "min_cost");
            if (0 > len) {
                free(buf);
                return;
            }
            used += len;
            remain -= len;
        
            DBSTATISTICMAP::iterator itr = _replaceMap.begin();
            for (; itr != _replaceMap.end(); ++itr) {
                if (minLen > remain) {
                    remain += blockSize;
                    char *new_buf = (char *)realloc(buf, blockSize * 2);
                    if (NULL == new_buf) {
                        free(buf);
                        return;
                    }
                    buf = new_buf;
                }
                memset(totalCost, 0, 12);
                SNPRINTF(totalCost, 11, "%ld.%03d", itr->second->totalCost / 1000,
                         int(itr->second->totalCost % 1000));
                Bmco::Int64 avg = itr->second->totalCost / itr->second->times;
                memset(avgCost, 0, 12);
                SNPRINTF(avgCost, 11, "%ld.%03d", avg / 1000, int(avg % 1000));
                memset(maxCost, 0, 12);
                SNPRINTF(maxCost, 11, "%ld.%03d", itr->second->maxCost / 1000,
                         int(itr->second->maxCost % 1000));
                memset(minCost, 0, 12);
                SNPRINTF(minCost, 11, "%ld.%03d", itr->second->minCost / 1000,
                         int(itr->second->minCost % 1000));
                len = SNPRINTF(buf + used, remain, "\t%-20s %-10s %-10ld %-10s %-10s %-10s\n",
                               itr->first.c_str(), totalCost, itr->second->times,
                               avgCost, maxCost, minCost);
                if (0 > len) {
                    free(buf);
                    return;
                }
                used += len;
                remain -= len;
            }
        }

        buf[used] = '\0';
        printf("%s", buf);
        INFORMATION_LOG("%s", buf);
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

    free(buf);
}

/// PBStatistic
PBStatistic::PBStatistic() {

}

PBStatistic::~PBStatistic() {

}

PBStatistic& PBStatistic::instance() {
    static PBStatistic pb;
    return pb;
}

void PBStatistic::statistics(bool serialize, const Bmco::Int64& cost) {
    if (_pbVec.empty()) {
        _pbVec.resize(2);
    }

    if (serialize) {
        Bmco::FastMutex::ScopedLock lock(_serializeMTX);
        
        _pbVec[0].totalCost += cost;
        if (cost > _pbVec[0].maxCost) {
            _pbVec[0].maxCost = cost;
        }
        if (cost < _pbVec[0].minCost) {
            _pbVec[0].minCost = cost;
        }
        _pbVec[0].times += 1;
    }
    else {
        Bmco::FastMutex::ScopedLock lock(_pareseMTX);

        _pbVec[1].totalCost += cost;
        if (cost > _pbVec[1].maxCost) {
            _pbVec[1].maxCost = cost;
        }
        if (cost < _pbVec[1].minCost) {
            _pbVec[1].minCost = cost;
        }
        _pbVec[1].times += 1;
    }
}

void PBStatistic::outputStatisticResult() {
    char *buf = NULL;
    try {
        int blockSize = 1 << 12;
        buf = (char *)malloc(blockSize);
        if (NULL == buf) {
            return;
        }

        int used = 0, len, remain = blockSize - 1;
        len = SNPRINTF(buf + used, remain, "PB statistics result: \n");
        if (0 > len) {
            free(buf);
            return;
        }
        used += len;
        remain -= len; 

        if (!_pbVec.empty()) {
            len = SNPRINTF(buf + used, remain, "\t%-20s %-10s %-10s %-10s %-10s %-10s\n",
                           "operator", "total_cost", "times", "avg_cost", "max_cost", "min_cost");
            if (0 > len) {
                free(buf);
                return;
            }
            used += len;
            remain -= len;

            char totalCost[12];
            char avgCost[12];
            char maxCost[12];
            char minCost[12];

            memset(totalCost, 0, 12);
            SNPRINTF(totalCost, 11, "%ld.%03d", _pbVec[0].totalCost / 1000,
                     int(_pbVec[0].totalCost % 1000));
            Bmco::Int64 avg = _pbVec[0].totalCost / _pbVec[0].times;
            memset(avgCost, 0, 12);
            SNPRINTF(avgCost, 11, "%ld.%03d", avg / 1000, int(avg % 1000));
            memset(maxCost, 0, 12);
            SNPRINTF(maxCost, 11, "%ld.%03d", _pbVec[0].maxCost / 1000,
                     int(_pbVec[0].maxCost % 1000));
            memset(minCost, 0, 12);
            SNPRINTF(minCost, 11, "%ld.%03d", _pbVec[0].minCost / 1000,
                     int(_pbVec[0].minCost % 1000));
            len = SNPRINTF(buf + used, remain, "\t%-20s %-10s %-10ld %-10s %-10s %-10s\n",
                           "serialize", totalCost, _pbVec[0].times,
                           avgCost, maxCost, minCost);
            if (0 > len) {
                free(buf);
                return;
            }
            used += len;
            remain -= len;

            memset(totalCost, 0, 12);
            SNPRINTF(totalCost, 11, "%ld.%03d", _pbVec[1].totalCost / 1000,
                     int(_pbVec[1].totalCost % 1000));
            avg = _pbVec[1].totalCost / _pbVec[1].times;
            memset(avgCost, 0, 12);
            SNPRINTF(avgCost, 11, "%ld.%03d", avg / 1000, int(avg % 1000));
            memset(maxCost, 0, 12);
            SNPRINTF(maxCost, 11, "%ld.%03d", _pbVec[1].maxCost / 1000,
                     int(_pbVec[1].maxCost % 1000));
            memset(minCost, 0, 12);
            SNPRINTF(minCost, 11, "%ld.%03d", _pbVec[1].minCost / 1000,
                     int(_pbVec[1].minCost % 1000));
            len = SNPRINTF(buf + used, remain, "\t%-20s %-10s %-10ld %-10s %-10s %-10s\n",
                           "parse", totalCost, _pbVec[1].times,
                           avgCost, maxCost, minCost);
            if (0 > len) {
                free(buf);
                return;
            }
            used += len;
            remain -= len;
        }
        
        buf[used] = '\0';
        printf("%s", buf);
        INFORMATION_LOG("%s", buf);
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

    free(buf);
}

/// NetWorkStatistic
NetWorkStatistic::NetWorkStatistic() {

}

NetWorkStatistic::~NetWorkStatistic() {

}

NetWorkStatistic& NetWorkStatistic::instance() {
    static NetWorkStatistic network;
    return network;
}

void NetWorkStatistic::statistics(bool send, const Bmco::Int64& cost) {
    if (_networkVec.empty()) {
        _networkVec.resize(2);
    }

    if (send) {
        Bmco::FastMutex::ScopedLock lock(_sendMTX);

        _networkVec[0].totalCost += cost;
        if (cost > _networkVec[0].maxCost) {
            _networkVec[0].maxCost = cost;
        }
        if (cost < _networkVec[0].minCost) {
            _networkVec[0].minCost = cost;
        }
        _networkVec[0].times += 1;
    }
    else {
        Bmco::FastMutex::ScopedLock lock(_receiveMTX);

        _networkVec[1].totalCost += cost;
        if (cost > _networkVec[1].maxCost) {
            _networkVec[1].maxCost = cost;
        }
        if (cost < _networkVec[1].minCost) {
            _networkVec[1].minCost = cost;
        }
        _networkVec[1].times += 1;
    }
}

void NetWorkStatistic::outputStatisticResult() {
    char *buf = NULL;
    try {
        int blockSize = 1 << 12;
        buf = (char *)malloc(blockSize);
        if (NULL == buf) {
            return;
        }

        int used = 0, len, remain = blockSize - 1;
        len = SNPRINTF(buf + used, remain, "NetWork statistics result: \n");
        if (0 > len) {
            free(buf);
            return;
        }
        used += len;
        remain -= len;

        if (!_networkVec.empty()) {
            len = SNPRINTF(buf + used, remain, "\t%-20s %-10s %-10s %-10s %-10s %-10s\n",
                           "operator", "total_cost", "times", "avg_cost", "max_cost", "min_cost");
            if (0 > len) {
                free(buf);
                return;
            }
            used += len;
            remain -= len;

            char totalCost[12];
            char avgCost[12];
            char maxCost[12];
            char minCost[12];

            memset(totalCost, 0, 12);
            SNPRINTF(totalCost, 11, "%ld.%03d", _networkVec[0].totalCost / 1000,
                     int(_networkVec[0].totalCost % 1000));
            Bmco::Int64 avg = _networkVec[0].totalCost / _networkVec[0].times;
            memset(avgCost, 0, 12);
            SNPRINTF(avgCost, 11, "%ld.%03d", avg / 1000, int(avg % 1000));
            memset(maxCost, 0, 12);
            SNPRINTF(maxCost, 11, "%ld.%03d", _networkVec[0].maxCost / 1000,
                     int(_networkVec[0].maxCost % 1000));
            memset(minCost, 0, 12);
            SNPRINTF(minCost, 11, "%ld.%03d", _networkVec[0].minCost / 1000,
                     int(_networkVec[0].minCost % 1000));
            len = SNPRINTF(buf + used, remain, "\t%-20s %-10s %-10ld %-10s %-10s %-10s\n",
                           "send", totalCost, _networkVec[0].times,
                           avgCost, maxCost, minCost);
            if (0 > len) {
                free(buf);
                return;
            }
            used += len;
            remain -= len;

            memset(totalCost, 0, 12);
            SNPRINTF(totalCost, 11, "%ld.%03d", _networkVec[1].totalCost / 1000,
                     int(_networkVec[1].totalCost % 1000));
            avg = _networkVec[1].totalCost / _networkVec[1].times;
            memset(avgCost, 0, 12);
            SNPRINTF(avgCost, 11, "%ld.%03d", avg / 1000, int(avg % 1000));
            memset(maxCost, 0, 12);
            SNPRINTF(maxCost, 11, "%ld.%03d", _networkVec[1].maxCost / 1000,
                     int(_networkVec[1].maxCost % 1000));
            memset(minCost, 0, 12);
            SNPRINTF(minCost, 11, "%ld.%03d", _networkVec[1].minCost / 1000,
                     int(_networkVec[1].minCost % 1000));
            len = SNPRINTF(buf + used, remain, "\t%-20s %-10s %-10ld %-10s %-10s %-10s\n",
                           "receive", totalCost, _networkVec[1].times,
                            avgCost, maxCost, minCost);
            if (0 > len) {
                free(buf);
                return;
            }
            used += len;
            remain -= len;
        }

        buf[used] = '\0';
        printf("%s", buf);
        INFORMATION_LOG("%s", buf);
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

    free(buf);
}

} // namespace BM35
