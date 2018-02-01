#include "DB.h"
#include "BolServiceLog.h"
//#include "ReDoLog.h"
#include "Bmco/Exception.h"
#include "Bmco/Util/LayeredConfiguration.h"
#include "Bmco/Util/Application.h"
#include "APIElement.cpp"

namespace BM35 {

/// DataAPI
int DataAPI::query(const std::string& tableName,
                   const Pair* pSelectedCondition,
                   Matrix& selectedResult) {
    DBStatisticsTool tool(DBStatistics::QUERY_OPERATOR, tableName);
    try {
        Bmco::Util::LayeredConfiguration& config = Bmco::Util::Application::instance().config();
        std::string db = config.getString("dbfile.name", SQLITE_MEMORY_TABLE);
        DB *pDB = DBManager::instance().getDB(DBManager::SQLITE_DB, db);
        if (NULL == pDB || !pDB->connect()) {
            ERROR_LOG(0, "get db failed");
            return -1;
        }
        
        std::string sql = "select * from " + tableName;
        if (NULL != pSelectedCondition) {
            int conditionNum = pSelectedCondition->num();
            for (int index = 0; index < conditionNum; ++index) {
                if (0 == index) {
                    sql += " where ";
                }

                if (conditionNum - 1 == index) {
                    sql += pSelectedCondition->getName(index) + " = '" +
                           pSelectedCondition->getValueString(index) + "'";
                }
                else {
                    sql += pSelectedCondition->getName(index) + " = '" +
                           pSelectedCondition->getValueString(index) + "' and ";
                }
            }
        }        

        if (!pDB->beginTransaction()) {
            ERROR_LOG(0, "beginTransaction failed");
            return -1;
        }

        if (!pDB->select(sql)) {
            pDB->rollbackTransaction();
            return -1;
        }
        
        FieldNameSet name;
        int fieldNum = pDB->fieldNum();
        int fieldNameIndex = 0;
        for (; fieldNameIndex < fieldNum; ++fieldNameIndex) {
            name.insert(pDB->fieldName(fieldNameIndex));
        }

        selectedResult.clear();
        selectedResult.setFieldNameSet(name);
        while (pDB->next()) {
            Pair row;
            fieldNameIndex = 0;
            int fieldValueIndex = 0;
            for (; fieldValueIndex < fieldNum; ++fieldValueIndex, ++fieldNameIndex) {
                row.addFieldPair(pDB->fieldName(fieldNameIndex), 
                                 pDB->fieldValue(fieldValueIndex));                
            }
            selectedResult.addRowValue(row);
        }

        pDB->commitTransaction();
                
        return 0;
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

    return -1;
}

int DataAPI::erase(const std::string& tableName,
                   const Pair* pDeleteCondition) {
    DBStatisticsTool tool(DBStatistics::ERASE_OPERATOR, tableName);
    try {
        Bmco::Util::LayeredConfiguration& config = Bmco::Util::Application::instance().config();
        std::string db = config.getString("dbfile.name", SQLITE_MEMORY_TABLE);
        DB *pDB = DBManager::instance().getDB(DBManager::SQLITE_DB, db);
        if (NULL == pDB || !pDB->connect()) {
            ERROR_LOG(0, "get db failed");
            return -1;
        }
                
        std::string sql = "delete from " + tableName;
        if (NULL != pDeleteCondition) {
            int conditionNum = pDeleteCondition->num();
            for (int index = 0; index < conditionNum; ++index) {
                if (0 == index) {
                    sql += " where ";
                }

                if (conditionNum - 1 == index) {
                    sql += pDeleteCondition->getName(index) + " = '" +
                           pDeleteCondition->getValueString(index) + "'";
                }
                else {
                    sql += pDeleteCondition->getName(index) + " = '" +
                           pDeleteCondition->getValueString(index) + "' and ";
                }
            }
        }        

        if (!pDB->beginTransaction()) {
            ERROR_LOG(0, "beginTransaction failed");
            return -1;
        }

        if (!pDB->execute(sql)) {
            pDB->rollbackTransaction();
            return -1;
        }
        /*std::string redoSQL = sql + ";\n";
        if (!ReDoLog::instance().log(redoSQL)) {
            pDB->rollbackTransaction();
            return -1;
        }*/
        int affectRow = pDB->affectRow();
        pDB->commitTransaction();
        
        return affectRow;
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

    return -1;
}

int DataAPI::update(const std::string& tableName,
                    const Pair* pUpdateCondition,
                    const Pair& updateValue) {
    DBStatisticsTool tool(DBStatistics::UPDATE_OPERATOR, tableName);
    try {
        Bmco::Util::LayeredConfiguration& config = Bmco::Util::Application::instance().config();
        std::string db = config.getString("dbfile.name", SQLITE_MEMORY_TABLE);
        DB *pDB = DBManager::instance().getDB(DBManager::SQLITE_DB, db);
        if (NULL == pDB || !pDB->connect()) {
            ERROR_LOG(0, "get db failed");
            return -1;
        }

        std::string sql = "update " + tableName + " set ";
        int updateNum = updateValue.num();
        if (0 >= updateNum) {
            ERROR_LOG(0, "modify value number: %d <= 0", updateNum);
            return -1;
        }
        for (int updateIndex = 0; updateIndex < updateNum; ++updateIndex) {            
            if (updateNum - 1 == updateIndex) {
                sql += updateValue.getName(updateIndex) + " = '" + 
                       updateValue.getValueString(updateIndex) + "'";
            }
            else {
                sql += updateValue.getName(updateIndex) + " = '" + 
                       updateValue.getValueString(updateIndex) + "', ";
            }
        }
        if (NULL != pUpdateCondition) {
            int conditionNum = pUpdateCondition->num();
            for (int index = 0; index < conditionNum; ++index) {
                if (0 == index) {
                    sql += " where ";
                }

                if (conditionNum - 1 == index) {
                    sql += pUpdateCondition->getName(index) + " = '" +
                           pUpdateCondition->getValueString(index) + "'";
                }
                else {
                    sql += pUpdateCondition->getName(index) + " = '" +
                           pUpdateCondition->getValueString(index) + "' and ";
                }
            }
        }        

        if (!pDB->beginTransaction()) {
            ERROR_LOG(0, "beginTransaction failed");
            return -1;
        }

        if (!pDB->execute(sql)) {
            pDB->rollbackTransaction();
            return -1;
        }
        /*std::string redoSQL = sql + ";\n";
        if (!ReDoLog::instance().log(redoSQL)) {
            pDB->rollbackTransaction();
            return -1;
        }*/
        int affectRow = pDB->affectRow();
        pDB->commitTransaction();
        
        return affectRow;
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

    return -1;
}

int DataAPI::add(const std::string& tableName, 
                 const Matrix& addRecord) {
    DBStatisticsTool tool(DBStatistics::ADD_OPERATOR, tableName);
    try {
        Bmco::Util::LayeredConfiguration& config = Bmco::Util::Application::instance().config();
        std::string db = config.getString("dbfile.name", SQLITE_MEMORY_TABLE);
        DB *pDB = DBManager::instance().getDB(DBManager::SQLITE_DB, db);
        if (NULL == pDB || !pDB->connect()) {
            ERROR_LOG(0, "get db failed");
            return -1;
        }

        int fieldNum  = addRecord.fieldNameCount();
        int recordNum = addRecord.rowValueCount();
        if (0 == fieldNum) {
            ERROR_LOG(0, "matrix field num: %d%s", fieldNum, 0 == fieldNum ? " == 0" : "");
            return -1;
        }

        const FieldNameList& nameList = addRecord.getFieldNameList();
        //std::string redoSQL;
        if (!pDB->beginTransaction()) {
            ERROR_LOG(0, "beginTransaction failed");
            return -1;
        }

        for (int recordIndex = 0; recordIndex < recordNum; ++recordIndex) {
            std::string sql = "insert into " + tableName + "(";
            std::string sqlSuffix = " values(";
            bool first = true;
            const RowValue& record = addRecord.getRowValue(recordIndex);             
            for (int fieldValueIndex = 0; fieldValueIndex < fieldNum; ++fieldValueIndex) {
                const FieldValue& fieldValue = record[fieldValueIndex];
                if (fieldValue.isNULL()) {
                    continue;
                }

                if (first) {
                    sql += nameList[fieldValueIndex];
                    sqlSuffix += "'" + fieldValue.getString() + "'";
                    first = false;
                }
                else {
                    sql += ", " + nameList[fieldValueIndex];
                    sqlSuffix += ", '" + fieldValue.getString() + "'";
                }                
            }
            sql = sql + ")" + sqlSuffix + ")";
            if (!pDB->execute(sql)) {
                pDB->rollbackTransaction();
                return -1;
            }
                        
            //redoSQL += sql + ";\n";
        }
        /*if (!ReDoLog::instance().log(redoSQL)) {
            pDB->rollbackTransaction();
            return -1;
            }*/
        pDB->commitTransaction();

        return 0;
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

    return -1;
}

int DataAPI::replace(const std::string& tableName, 
                     const Matrix& replaceRecord) {
    DBStatisticsTool tool(DBStatistics::REPLACE_OPERATOR, tableName);
    try {
        Bmco::Util::LayeredConfiguration& config = Bmco::Util::Application::instance().config();
        std::string db = config.getString("dbfile.name", SQLITE_MEMORY_TABLE);
        DB *pDB = DBManager::instance().getDB(DBManager::SQLITE_DB, db);
        if (NULL == pDB || !pDB->connect()) {
            ERROR_LOG(0, "get db failed");
            return -1;
        }

        int fieldNum = replaceRecord.fieldNameCount();
        int recordNum = replaceRecord.rowValueCount();
        if (0 == fieldNum) {
            ERROR_LOG(0, "matrix field num: %d%s", fieldNum, 0 == fieldNum ? " == 0" : "");
            return -1;
        }        

        // 先清空数据
        //std::string redoSQL = "delete from " + tableName;
        std::string clearSQL = "delete from " + tableName;
        if (!pDB->beginTransaction()) {
            ERROR_LOG(0, "beginTransaction failed");
            return -1;
        }

        if (!pDB->execute(clearSQL)) {
            pDB->rollbackTransaction();
            return -1;
        }
        //redoSQL += ";\n";

        // 再添加数据
        const FieldNameList& nameList = replaceRecord.getFieldNameList();
        for (int recordIndex = 0; recordIndex < recordNum; ++recordIndex) {
            std::string sql = "insert into " + tableName + "(";
            std::string sqlSuffix = " values(";
            bool first = true;
            const RowValue& record = replaceRecord.getRowValue(recordIndex);
            for (int fieldValueIndex = 0; fieldValueIndex < fieldNum; ++fieldValueIndex) {
                const FieldValue& fieldValue = record[fieldValueIndex];
                if (fieldValue.isNULL()) {
                    continue;
                }

                if (first) {
                    sql += nameList[fieldValueIndex];
                    sqlSuffix += "'" + fieldValue.getString() + "'";
                    first = false;
                }
                else {
                    sql += ", " + nameList[fieldValueIndex];
                    sqlSuffix += ", '" + fieldValue.getString() + "'";
                }
            }
            sql = sql + ")" + sqlSuffix + ")";
            if (!pDB->execute(sql)) {
                pDB->rollbackTransaction();
                return -1;
            }

            //redoSQL += sql + ";\n";
        }
        /*if (!ReDoLog::instance().log(redoSQL)) {
            pDB->rollbackTransaction();
            return -1;
        }*/
        pDB->commitTransaction();

        return 0;
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

    return -1;
}

int DataAPI::create(const std::string& sql) {
    if (sql.empty()) {
        ERROR_LOG(0, "sql is empty");
        return -1;
    }

    try {
        Bmco::Util::LayeredConfiguration& config = Bmco::Util::Application::instance().config();
        std::string db = config.getString("dbfile.name", SQLITE_MEMORY_TABLE);
        DB *pDB = DBManager::instance().getDB(DBManager::SQLITE_DB, db);
        if (NULL == pDB || !pDB->connect()) {
            ERROR_LOG(0, "get db failed");
            return -1;
        }

        if (!pDB->beginTransaction()) {
            ERROR_LOG(0, "beginTransaction failed");
            return -1;
        }

        if (!pDB->execute(sql)) {
            pDB->rollbackTransaction();
            return -1;
        }
        pDB->commitTransaction();

        return 0;
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

    return -1;
}

} // namespace BM35
