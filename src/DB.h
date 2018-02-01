#ifndef __DB_H
#define __DB_H

#include "sqlite3.h"
#include "Bmco/Mutex.h"
#include "Bmco/SharedPtr.h"
#include "Bmco/Timestamp.h"
#include <string>
#include <map>
#include <vector>

namespace BM35 {

class DB {
public:
    DB();

    virtual ~DB();    

    virtual bool connect() = 0;

    virtual bool execute(const std::string& sql) = 0;

    virtual bool beginTransaction() = 0;

    virtual void commitTransaction() = 0;

    virtual void rollbackTransaction() = 0;

    virtual int affectRow() = 0; // return affect rows by execute sql

    virtual int fieldNum() = 0;  // return field number of select result

    virtual bool select(const std::string& sql) = 0;

    virtual bool next() = 0;

    virtual std::string fieldName(int index) = 0;

    virtual std::string fieldValue(int index) = 0;

private:
    DB(const DB&);

    DB& operator = (const DB&);

protected:
    int _affectRow;
}; // class DB

class SQLiteDB : public DB {
public:
    SQLiteDB(const std::string& dbConnect);

    virtual ~SQLiteDB();

    virtual bool connect();

    virtual bool execute(const std::string& sql);

    virtual bool beginTransaction();

    virtual void commitTransaction();

    virtual void rollbackTransaction();

    virtual int affectRow(); // return affect rows by execute sql

    virtual int fieldNum();  // return field number of select result

    virtual bool select(const std::string& sql);

    virtual bool next();

    virtual std::string fieldName(int index);

    virtual std::string fieldValue(int index);

private:
    SQLiteDB();

    SQLiteDB(const SQLiteDB&);

    SQLiteDB& operator = (const SQLiteDB&);

private:
    std::string _dbConnect;

    sqlite3 *_db;

    sqlite3_stmt *_stmt;

    Bmco::Mutex _mtx;

    bool _connected;
}; // class SQLiteDB

// SQLite的内存数据库
#define SQLITE_MEMORY_TABLE ":memory:"
class MemorySQLiteDB : public DB {
public:
    MemorySQLiteDB();

    virtual ~MemorySQLiteDB();

    virtual bool connect();

    virtual bool execute(const std::string& sql);

    virtual bool beginTransaction();

    virtual void commitTransaction();

    virtual void rollbackTransaction();

    virtual int affectRow(); // return affect rows by execute sql

    virtual int fieldNum();  // return field number of select result

    virtual bool select(const std::string& sql);

    virtual bool next();

    virtual std::string fieldName(int index);

    virtual std::string fieldValue(int index);

private:
    MemorySQLiteDB(const MemorySQLiteDB&);

    MemorySQLiteDB& operator = (const MemorySQLiteDB&);

private:
    static SQLiteDB _db;
    
    static bool _connected;
}; // class MemorySQLiteDB

/// DBManger
class DBManager {
public:
    ~DBManager();

    static DBManager& instance();

    enum DB_TYPE {
        SQLITE_DB,
        MYSQL_DB,
        ORACLE_DB
    };

    // 目前仅支持SQLITE_DB
    DB* getDB(DB_TYPE dbType, const std::string& dbConnect);

private:
    DBManager();

    DBManager(const DBManager&);

    DBManager& operator = (const DBManager&);

    std::string getFullName(const std::string& name);

private:
    typedef std::map<std::string, Bmco::SharedPtr<DB> > DBMAP;
    DBMAP _dbMap;

    Bmco::Mutex _mtx;
}; // class DBManger

/// DBStatisticData
struct DBStatisticData {
    Bmco::Int64 totalCost;
    Bmco::Int64 maxCost;
    Bmco::Int64 minCost;
    Bmco::Int64 times;

    DBStatisticData() {
        totalCost = 0;
        maxCost   = 0;
        minCost   = 0;
        times     = 0;
    }
}; // DBStatisticData

/// DBStatisticsTool
class DBStatisticsTool {
public:
    DBStatisticsTool(const std::string& operatorName, const std::string& tableName);

    ~DBStatisticsTool();

private:
    DBStatisticsTool();

    DBStatisticsTool(const DBStatisticsTool&);

    DBStatisticsTool& operator = (const DBStatisticsTool&);

private:
    Bmco::SharedPtr<Bmco::Timestamp> _pNow;

    std::string _operatorName;
    std::string _tableName;
}; // DBStatisticsTool

/// DBStatistics
class DBStatistics {
public:
    ~DBStatistics();

    static DBStatistics& instance();

    static bool enable();

    void statistics(const std::string& operatorName, const std::string& tableName, 
                    const Bmco::Int64& cost);

    void outputStatisticResult();

private:
    DBStatistics();

    DBStatistics(const DBStatistics&);

    DBStatistics& operator = (const DBStatistics&);

private:
    typedef std::map<std::string, Bmco::SharedPtr<DBStatisticData> > DBSTATISTICMAP;
    DBSTATISTICMAP _addMap;
    DBSTATISTICMAP _queryMap;
    DBSTATISTICMAP _eraseMap;
    DBSTATISTICMAP _updateMap;
    DBSTATISTICMAP _replaceMap;

    Bmco::FastMutex _addMtx;
    Bmco::FastMutex _queryMtx;
    Bmco::FastMutex _eraseMtx;
    Bmco::FastMutex _updateMtx;
    Bmco::FastMutex _replaceMtx;

    static bool _enable;
    static bool _judgeEnable;

public:
    static const std::string ADD_OPERATOR;
    static const std::string QUERY_OPERATOR;
    static const std::string ERASE_OPERATOR;
    static const std::string UPDATE_OPERATOR;
    static const std::string REPLACE_OPERATOR;
}; // DBStatistics

/// PBStatistic
class PBStatistic {
public:
    ~PBStatistic();

    static PBStatistic& instance();

    void statistics(bool serialize, const Bmco::Int64& cost);

    void outputStatisticResult();

private:
    PBStatistic();

    PBStatistic(const PBStatistic&);

    PBStatistic& operator = (const PBStatistic&);

private:
    typedef std::vector<DBStatisticData> PBSTATISTICVEC;
    PBSTATISTICVEC _pbVec;

    Bmco::FastMutex _serializeMTX;
    Bmco::FastMutex _pareseMTX;
}; // PBStatistic

/// NetWorkStatistic
class NetWorkStatistic {
public:
    ~NetWorkStatistic();

    static NetWorkStatistic& instance();

    void statistics(bool send, const Bmco::Int64& cost);

    void outputStatisticResult();

private:
    NetWorkStatistic();

    NetWorkStatistic(const NetWorkStatistic&);

    NetWorkStatistic& operator = (const NetWorkStatistic&);

private:
    typedef std::vector<DBStatisticData> NETWORKSTATISTICVEC;
    NETWORKSTATISTICVEC _networkVec;

    Bmco::FastMutex _sendMTX;
    Bmco::FastMutex _receiveMTX;
}; // NetWorkStatistic

} // namespace BM35

#endif // __DB_H
