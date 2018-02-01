#include "WorkTask.h"
#include "TaskNotification.h"
#include "ResponseNotify.h"
#include "DB.h"
//#include "ReDoLog.h"
#include "DataAPI.h"
#include "BolServiceLog.h"
#include "Bmco/Notification.h"
#include "Bmco/SharedPtr.h"
#include "Bmco/LocalDateTime.h"
#include "Bmco/Util/LayeredConfiguration.h"
#include "Bmco/Util/Application.h"
#include "Bmco/Exception.h"
#include "Bmco/SharedPtr.h"
#include "Bmco/Timestamp.h"
#include "Bmco/DateTimeFormatter.h"
#include "Bmco/DateTimeFormat.h"
#include "OmcLog.h"
#include "BolService.pb.h"
#include "MemTableOp.h"


#define  DBFILE "E:\\CTC\\Debug\\bol.db"

namespace BM35 {

/// ResponseOp
class ResponseOp {
public:
    static void add(const BolServiceRequest_Request& request, 
                    BolServiceResponse& response, int sysPid,const std::string &exename);

    static void erase(const BolServiceRequest_Request& request,
                      BolServiceResponse& response, int sysPid,const std::string &exename);

    static void query(const BolServiceRequest_Request& request,
                      BolServiceResponse& response, int sysPid,const std::string &exename);

    static void update(const BolServiceRequest_Request& request,
                       BolServiceResponse& response, int sysPid,const std::string &exename);

    static void replace(const BolServiceRequest_Request& request,
                        BolServiceResponse& response, int sysPid,const std::string &exename);

    // 心跳
    static void heartBeat(const BolServiceRequest_Request& request,
                          BolServiceResponse& response, int sysPid,const std::string &exename,int heartbeattype);

private:
    static void setBolServiceHead(const BolServiceHead& head, 
                                  BolServiceResponse_Response& response);
};

void ResponseOp::add(const BolServiceRequest_Request& request,
                     BolServiceResponse& response,int sysPid,const std::string &exename) {

   Bmco::LocalDateTime starttime;
	 std::string exception;
	 std::string tableName;
	 
    BolServiceResponse_Response *pResponse = response.add_msgs();
    setBolServiceHead(request.msghead(), *pResponse);

    if (!request.has_add()) {
        ERROR_LOG(0, "request has no add");
        pResponse->set_status(1);
        return;
    }

    /*try {
        Bmco::Util::LayeredConfiguration& config = Bmco::Util::Application::instance().config();
        std::string db = config.getString("dbfile.name", "");
        Bmco::SharedPtr<DB> pDB = new SQLiteDB(db);        
        if (pDB.isNull() || !pDB->connect()) {
            pResponse->set_status(1);
            return;
        }        

        const BolServiceRequest_AddRequest& addRequest = request.add();
        std::string tableName = addRequest.table();
        int fieldNum = addRequest.fields_size();
        int recordNum = addRequest.records_size();
        std::string sqlPrefix = "insert into " + tableName + "(";
        for (int fieldIndex = 0; fieldIndex < fieldNum; ++fieldIndex) {
            if (fieldNum - 1 == fieldIndex) {
                sqlPrefix += addRequest.fields(fieldIndex) + ") values(";
            }
            else {
                sqlPrefix += addRequest.fields(fieldIndex) + ",";
            }
        }

        std::string redoSQL;
        pDB->beginTransaction();
        for (int recordIndex = 0; recordIndex < recordNum; ++recordIndex) {
            std::string sql = sqlPrefix;
            const Record& record = addRequest.records(recordIndex);
            for (int fieldValueIndex = 0; fieldValueIndex < fieldNum; ++fieldValueIndex) {
                if (fieldNum - 1 == fieldValueIndex) {
                    sql += "'" + record.values(fieldValueIndex) + "')";
                }
                else {
                    sql += "'" + record.values(fieldValueIndex) + "',";
                }
            }

            if (!pDB->execute(sql)) {
                pDB->rollbackTransaction();
                pResponse->set_status(1);
                return;
            }

            redoSQL += sql + ";\n";
        }
        if (!ReDoLog::instance().log(redoSQL)) {
            pDB->rollbackTransaction();
            pResponse->set_status(1);
            return;
        }
        pDB->commitTransaction();
                
        pResponse->set_status(0);
        return;
    }
    catch (...) {}*/

    try {
        const BolServiceRequest_AddRequest& addRequest = request.add();       
        int fieldNum = addRequest.fields_size();
        FieldNameSet nameSet;
        int fieldIndex = 0;
        for (; fieldIndex < fieldNum; ++fieldIndex) {
            nameSet.insert(addRequest.fields(fieldIndex));
        }
        Matrix addRecord;
        addRecord.setFieldNameSet(nameSet);

        int recordNum = addRequest.records_size();        
        for (int index = 0; index < recordNum; ++index) {            
            const Record& record = addRequest.records(index);
            Pair row;
            fieldIndex = 0;
            int fieldValueIndex = 0;
            for (; fieldValueIndex < fieldNum; ++fieldValueIndex, ++fieldIndex) {
                const ::FieldValue& fieldValue = record.values(fieldValueIndex);
                if (fieldValue.isnull()) {
                    continue;
                }
                row.addFieldPair(addRequest.fields(fieldIndex), fieldValue.value());                              
            }
            addRecord.addRowValue(row);
        }

        tableName = addRequest.table();

	 
	Bmco::LocalDateTime sqlstarttime;
        int ret = DataAPI::add(tableName, addRecord);
        if (0 != ret) {
            ERROR_LOG(0, "DataAPI::add return %d", ret);
            pResponse->set_status(1);

		OMC_LOG("40201001|%s|2|DataAPI::add|%s|%s|%s|%d|1|add error",
	 	Bmco::Util::Application::instance().config().getString("info.bol_name","bol123").c_str(),
	 	Bmco::DateTimeFormatter::format(sqlstarttime.timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	Bmco::DateTimeFormatter::format(Bmco::LocalDateTime().timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	exename.c_str(),
	 	sysPid
	 	);
		

		OMC_LOG("40201001|%s|1|%s|%s|%s|%s|%d|1|sqlite error",
	 	Bmco::Util::Application::instance().config().getString("info.bol_name","bol123").c_str(),
	 	tableName.c_str(),
	 	Bmco::DateTimeFormatter::format(starttime.timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	Bmco::DateTimeFormatter::format(Bmco::LocalDateTime().timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	exename.c_str(),
	 	sysPid
	 	);
            return;
        }

        pResponse->set_status(0);

	OMC_LOG("40201001|%s|2|DataAPI::add|%s|%s|%s|%d|0|success",
	 	Bmco::Util::Application::instance().config().getString("info.bol_name","bol123").c_str(),
	 	Bmco::DateTimeFormatter::format(sqlstarttime.timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	Bmco::DateTimeFormatter::format(Bmco::LocalDateTime().timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	exename.c_str(),
	 	sysPid
	 	);
		

	 OMC_LOG("40201001|%s|1|%s|%s|%s|%s|%d|0|success",
	 	Bmco::Util::Application::instance().config().getString("info.bol_name","bol123").c_str(),
	 	tableName.c_str(),
	 	Bmco::DateTimeFormatter::format(starttime.timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	Bmco::DateTimeFormatter::format(Bmco::LocalDateTime().timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	exename.c_str(),
	 	sysPid
	 	);
	 
	 
        return;
    }
    catch (Bmco::Exception& e) {
        ERROR_LOG(0, "occur exception: %s", e.displayText().c_str());
		exception=e.displayText();
    }
    catch (std::exception& e) {
        ERROR_LOG(0, "occur exception: %s", e.what());
		exception=e.what();
    }
    catch (...) {
        ERROR_LOG(0, "occur unknown exception");
		exception="exception";
    }

    pResponse->set_status(1);    

	OMC_LOG("40201001|%s|1|%s|%s|%s|%s|%d|1|%s",
	 	Bmco::Util::Application::instance().config().getString("info.bol_name","bol123").c_str(),
	 	tableName.c_str(),
	 	Bmco::DateTimeFormatter::format(starttime.timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	Bmco::DateTimeFormatter::format(Bmco::LocalDateTime().timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	exename.c_str(),
	 	sysPid,
	 	exception.c_str()
	 	);
}

void ResponseOp::erase(const BolServiceRequest_Request& request,
                       BolServiceResponse& response,int sysPid,const std::string &exename) {


	Bmco::LocalDateTime starttime;
	 std::string exception;
	 std::string tableName;
	
					   
    BolServiceResponse_Response *pResponse = response.add_msgs();
    setBolServiceHead(request.msghead(), *pResponse);

    if (!request.has_erase()) {
        ERROR_LOG(0, "request has no erase");
        pResponse->set_status(1);
        return;
    }

    /*try {
        Bmco::Util::LayeredConfiguration& config = Bmco::Util::Application::instance().config();
        std::string db = config.getString("dbfile.name", "");
        Bmco::SharedPtr<DB> pDB = new SQLiteDB(db);
        if (pDB.isNull() || !pDB->connect()) {
            pResponse->set_status(1);
            return;
        }

        const BolServiceRequest_EraseRequest& eraseRequest = request.erase();
        std::string tableName = eraseRequest.table();
        std::string sql = "delete from " + tableName;
        int conditionNum = eraseRequest.filters_size();
        for (int index = 0; index < conditionNum; ++index) {
            if (0 == index) {
                sql += " where ";
            }
            const BolServiceRequest_Filter& filter = eraseRequest.filters(index);
            if (conditionNum - 1 == index) {
                sql += filter.key() + " = '" + filter.value() + "'";
            }
            else {
                sql += filter.key() + " = '" + filter.value() + "' and ";
            }
        }
                
        pDB->beginTransaction();
        if (!pDB->execute(sql)) {
            pDB->rollbackTransaction();
            pResponse->set_status(1);
            return;
        }
        std::string redoSQL = sql + ";\n";
        if (!ReDoLog::instance().log(redoSQL)) {
            pDB->rollbackTransaction();
            pResponse->set_status(1);
            return;
        }
        pDB->commitTransaction();
        
        int affectRow = pDB->affectRow();
        
        BolServiceResponse_EraseResponse *pEraseResponse = pResponse->mutable_eraseresponse();
        pEraseResponse->set_num(affectRow);
        pResponse->set_status(0);
        return;
    }
    catch (...) {}*/

    try {
        const BolServiceRequest_EraseRequest& eraseRequest = request.erase();        
        int conditionNum = eraseRequest.filters_size();
        Pair condition;
        for (int index = 0; index < conditionNum; ++index) {
            const BolServiceRequest_Filter& filter = eraseRequest.filters(index);
            condition.addFieldPair(filter.key(), filter.value());            
        }

        tableName = eraseRequest.table();
	Bmco::LocalDateTime sqlstarttime;
        int ret = DataAPI::erase(tableName, &condition);
        if (0 > ret) {
            ERROR_LOG(0, "DataAPI::erase return %d", ret);
            pResponse->set_status(1);

		OMC_LOG("40201001|%s|2|DataAPI::erase|%s|%s|%s|%d|1|erase error",
	 	Bmco::Util::Application::instance().config().getString("info.bol_name","bol123").c_str(),
	 	Bmco::DateTimeFormatter::format(sqlstarttime.timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	Bmco::DateTimeFormatter::format(Bmco::LocalDateTime().timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	exename.c_str(),
	 	sysPid
	 	);
			
			
		OMC_LOG("40201001|%s|1|%s|%s|%s|%s|%d|1|sqlite error",
	 	Bmco::Util::Application::instance().config().getString("info.bol_name","bol123").c_str(),
	 	tableName.c_str(),
	 	Bmco::DateTimeFormatter::format(starttime.timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	Bmco::DateTimeFormatter::format(Bmco::LocalDateTime().timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	exename.c_str(),
	 	sysPid
	 	);
            return;
        }

        BolServiceResponse_EraseResponse *pEraseResponse = pResponse->mutable_eraseresponse();
        pEraseResponse->set_num(ret);
        pResponse->set_status(0);

	OMC_LOG("40201001|%s|2|DataAPI::erase|%s|%s|%s|%d|0|success",
	 	Bmco::Util::Application::instance().config().getString("info.bol_name","bol123").c_str(),
	 	Bmco::DateTimeFormatter::format(sqlstarttime.timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	Bmco::DateTimeFormatter::format(Bmco::LocalDateTime().timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	exename.c_str(),
	 	sysPid
	 	);
		
	 OMC_LOG("40201001|%s|1|%s|%s|%s|%s|%d|0|success",
	 	Bmco::Util::Application::instance().config().getString("info.bol_name","bol123").c_str(),
	 	tableName.c_str(),
	 	Bmco::DateTimeFormatter::format(starttime.timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	Bmco::DateTimeFormatter::format(Bmco::LocalDateTime().timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	exename.c_str(),
	 	sysPid
	 	);
        return;
    }
    catch (Bmco::Exception& e) {
        ERROR_LOG(0, "occur exception: %s", e.displayText().c_str());
		exception=e.displayText();
    }
    catch (std::exception& e) {
        ERROR_LOG(0, "occur exception: %s", e.what());
		exception=e.what();
    }
    catch (...) {
        ERROR_LOG(0, "occur unknown exception");
		exception="exception";
    }
    pResponse->set_status(1);

	OMC_LOG("40201001|%s|1|%s|%s|%s|%s|%d|1|%s",
	 	Bmco::Util::Application::instance().config().getString("info.bol_name","bol123").c_str(),
	 	tableName.c_str(),
	 	Bmco::DateTimeFormatter::format(starttime.timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	Bmco::DateTimeFormatter::format(Bmco::LocalDateTime().timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	exename.c_str(),
	 	sysPid,
	 	exception.c_str()
	 	);
}

void ResponseOp::query(const BolServiceRequest_Request& request,
                       BolServiceResponse& response,int sysPid,const std::string &exename) {

	 Bmco::LocalDateTime starttime;
	 std::string exception;
	 std::string tableName;

    BolServiceResponse_Response *pResponse = response.add_msgs();
    setBolServiceHead(request.msghead(), *pResponse);

    if (!request.has_query()) {
        ERROR_LOG(0, "request has no query");
        pResponse->set_status(1);
        return;
    }

    /*try {
        Bmco::Util::LayeredConfiguration& config = Bmco::Util::Application::instance().config();
        std::string db = config.getString("dbfile.name", "");
        Bmco::SharedPtr<DB> pDB = new SQLiteDB(db);
        if (pDB.isNull() || !pDB->connect()) {
        pResponse->set_status(1);
        return;
        }

        const BolServiceRequest_QueryRequest& queryRequest = request.query();
        std::string tableName = queryRequest.table();
        std::string sql = "select * from " + tableName;
        int conditionNum = queryRequest.filters_size();
        for (int index = 0; index < conditionNum; ++index) {
        if (0 == index) {
        sql += " where ";
        }

        const BolServiceRequest_Filter& filter = queryRequest.filters(index);
        if (conditionNum - 1 == index) {
        sql += filter.key() + " = '" + filter.value() + "'";
        }
        else {
        sql += filter.key() + " = '" + filter.value() + "' and ";
        }
        }

        if (!pDB->select(sql)) {
        pResponse->set_status(1);
        return;
        }

        BolServiceResponse_QueryResponse *pQueryResponse = pResponse->mutable_queryresult();
        Record *pFieldName = pQueryResponse->mutable_fields();
        int fieldNum = pDB->fieldNum();
        for (int fieldNameIndex = 0; fieldNameIndex < fieldNum; ++fieldNameIndex) {
        pFieldName->add_values(pDB->fieldName(fieldNameIndex));
        }

        while (pDB->next()) {
        Record *pFieldValue = pQueryResponse->add_records();
        for (int fieldValueIndex = 0; fieldValueIndex < fieldNum; ++fieldValueIndex) {
        pFieldValue->add_values(pDB->fieldValue(fieldValueIndex));
        }
        }

        pResponse->set_status(0);
        return;
        }
        catch (...) {}*/

    try {
        const BolServiceRequest_QueryRequest& queryRequest = request.query();        
        int conditionNum = queryRequest.filters_size();
        Pair condition;
        for (int index = 0; index < conditionNum; ++index) {
            const BolServiceRequest_Filter& filter = queryRequest.filters(index);
            condition.addFieldPair(filter.key(), filter.value());            
        }

       tableName = queryRequest.table();
        Matrix result;
	Bmco::LocalDateTime sqlstarttime;
        int ret = DataAPI::query(tableName, &condition, result);	
        if (0 != ret) {
            ERROR_LOG(0, "DataAPI::query return %d", ret);
            pResponse->set_status(1);


		OMC_LOG("40201001|%s|2|DataAPI::query|%s|%s|%s|%d|1|query error",
	 	Bmco::Util::Application::instance().config().getString("info.bol_name","bol123").c_str(),
	 	Bmco::DateTimeFormatter::format(sqlstarttime.timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	Bmco::DateTimeFormatter::format(Bmco::LocalDateTime().timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	exename.c_str(),
	 	sysPid
	 	);
		

	 OMC_LOG("40201001|%s|1|%s|%s|%s|%s|%d|1|sqlite error",
	 	Bmco::Util::Application::instance().config().getString("info.bol_name","bol123").c_str(),
	 	tableName.c_str(),
	 	Bmco::DateTimeFormatter::format(starttime.timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	Bmco::DateTimeFormatter::format(Bmco::LocalDateTime().timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	exename.c_str(),
	 	sysPid
	 	);
			
            return;
        }

        BolServiceResponse_QueryResponse *pQueryResponse = pResponse->mutable_queryresult();
        Record *pFieldName = pQueryResponse->mutable_fields();
        int fieldNum = result.fieldNameCount();
        const FieldNameList& nameList = result.getFieldNameList();
        for (int fieldNameIndex = 0; fieldNameIndex < fieldNum; ++fieldNameIndex) {
            ::FieldValue *pFieldValue = pFieldName->add_values();
            pFieldValue->set_value(nameList[fieldNameIndex]);
            pFieldValue->set_isnull(false);
        }

        int rowNum = result.rowValueCount();
        for (int rowIndex = 0; rowIndex < rowNum; ++rowIndex) {
            Record *pRecord = pQueryResponse->add_records();
            for (int fieldValueIndex = 0; fieldValueIndex < fieldNum; ++fieldValueIndex) {
                const FieldValue& value = result.getFieldValue(rowIndex, fieldValueIndex);
                ::FieldValue *pFieldValue = pRecord->add_values();
                if (value.isNULL()) {
                    pFieldValue->set_value("");
                    pFieldValue->set_isnull(true);
                }
                else {
                    pFieldValue->set_value(value.getString());
                    pFieldValue->set_isnull(false);
                }                
            }
        }

	 

        pResponse->set_status(0);

	OMC_LOG("40201001|%s|2|DataAPI::query|%s|%s|%s|%d|0|success",
	 	Bmco::Util::Application::instance().config().getString("info.bol_name","bol123").c_str(),
	 	Bmco::DateTimeFormatter::format(sqlstarttime.timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	Bmco::DateTimeFormatter::format(Bmco::LocalDateTime().timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	exename.c_str(),
	 	sysPid
	 	);
		
	 OMC_LOG("40201001|%s|1|%s|%s|%s|%s|%d|0|success",
	 	Bmco::Util::Application::instance().config().getString("info.bol_name","bol123").c_str(),
	 	tableName.c_str(),
	 	Bmco::DateTimeFormatter::format(starttime.timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	Bmco::DateTimeFormatter::format(Bmco::LocalDateTime().timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	exename.c_str(),
	 	sysPid
	 	);
        return;

    }
    catch (Bmco::Exception& e) {
        ERROR_LOG(0, "occur exception: %s", e.displayText().c_str());
		exception=e.displayText();
    }
    catch (std::exception& e) {
        ERROR_LOG(0, "occur exception: %s", e.what());
		exception=e.what();
    }
    catch (...) {
        ERROR_LOG(0, "occur unknown exception");
		exception="exception";
    }

    pResponse->set_status(1);

	OMC_LOG("40201001|%s|2|%s|%s|%s|%s|%d|1|%s",
	 	Bmco::Util::Application::instance().config().getString("info.bol_name","bol123").c_str(),
	 	tableName.c_str(),
	 	Bmco::DateTimeFormatter::format(starttime.timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	Bmco::DateTimeFormatter::format(Bmco::LocalDateTime().timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	exename.c_str(),
	 	sysPid,
	 	exception.c_str()
	 	);
}

void ResponseOp::update(const BolServiceRequest_Request& request,
                        BolServiceResponse& response,int sysPid,const std::string &exename) {
    BolServiceResponse_Response *pResponse = response.add_msgs();
    setBolServiceHead(request.msghead(), *pResponse);

    if (!request.has_update()) {
        ERROR_LOG(0, "request has no update");
        pResponse->set_status(1);
        return;
    }

	Bmco::LocalDateTime starttime;
	 std::string exception;
	 std::string tableName;

    /*try {
        Bmco::Util::LayeredConfiguration& config = Bmco::Util::Application::instance().config();
        std::string db = config.getString("dbfile.name", "");
        Bmco::SharedPtr<DB> pDB = new SQLiteDB(db);
        if (pDB.isNull() || !pDB->connect()) {
            pResponse->set_status(1);
            return;
        }

        const BolServiceRequest_UpdateRequest& updateRequest = request.update();
        std::string tableName = updateRequest.table();
        std::string sql = "update " + tableName + " set ";
        int updateNum = updateRequest.values_size();
        if (0 >= updateNum) {            
            pResponse->set_status(1);
            return;
        }
        for (int updateIndex = 0; updateIndex < updateNum; ++updateIndex) {
            const BolServiceRequest_Filter& updateValue = updateRequest.values(updateIndex);
            if (updateNum - 1 == updateIndex) {
                sql += updateValue.key() + " = '" + updateValue.value() + "'";
            }
            else {
                sql += updateValue.key() + " = '" + updateValue.value() + "', ";
            }
        }
        int conditionNum = updateRequest.filters_size();
        for (int index = 0; index < conditionNum; ++index) {
            if (0 == index) {
                sql += " where ";
            }
            const BolServiceRequest_Filter& filter = updateRequest.filters(index);
            if (conditionNum - 1 == index) {
                sql += filter.key() + " = '" + filter.value() + "'";
            }
            else {
                sql += filter.key() + " = '" + filter.value() + "' and ";
            }
        }
                
        pDB->beginTransaction();
        if (!pDB->execute(sql)) {
            pDB->rollbackTransaction();
            pResponse->set_status(1);
            return;
        }
        std::string redoSQL = sql + ";\n";
        if (!ReDoLog::instance().log(redoSQL)) {
            pDB->rollbackTransaction();
            pResponse->set_status(1);
            return;
        }
        pDB->commitTransaction();

        int affectRow = pDB->affectRow();
        
        BolServiceResponse_UpdateResponse *pUpdateResponse = pResponse->mutable_updateresult();
        pUpdateResponse->set_num(affectRow);
        pResponse->set_status(0);
        return;
    }
    catch (...) {}*/


	

    try {
        const BolServiceRequest_UpdateRequest& updateRequest = request.update();        
        int conditionNum = updateRequest.filters_size();
        Pair condition;
        for (int index = 0; index < conditionNum; ++index) {
            const BolServiceRequest_Filter& filter = updateRequest.filters(index);
            condition.addFieldPair(filter.key(), filter.value());            
        }

        int updateNum = updateRequest.values_size();
        Pair update;
        for (int updateIndex = 0; updateIndex < updateNum; ++updateIndex) {
            const BolServiceRequest_Filter& updateValue = updateRequest.values(updateIndex);
            update.addFieldPair(updateValue.key(), updateValue.value());
        }
        
        tableName = updateRequest.table();
	Bmco::LocalDateTime sqlstarttime;
        int ret = DataAPI::update(tableName, &condition, update);
        if (0 > ret) {
            ERROR_LOG(0, "DataAPI::update return %d", ret);
            pResponse->set_status(1);

		OMC_LOG("40201001|%s|2|DataAPI::update|%s|%s|%s|%d|1|update error",
	 	Bmco::Util::Application::instance().config().getString("info.bol_name","bol123").c_str(),
	 	Bmco::DateTimeFormatter::format(sqlstarttime.timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	Bmco::DateTimeFormatter::format(Bmco::LocalDateTime().timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	exename.c_str(),
	 	sysPid
	 	);
			
		OMC_LOG("40201001|%s|1|%s|%s|%s|%s|%d|1|sqlite error",
	 	Bmco::Util::Application::instance().config().getString("info.bol_name","bol123").c_str(),
	 	tableName.c_str(),
	 	Bmco::DateTimeFormatter::format(starttime.timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	Bmco::DateTimeFormatter::format(Bmco::LocalDateTime().timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	exename.c_str(),
	 	sysPid
	 	);
            return;
        }

        BolServiceResponse_UpdateResponse *pUpdateResponse = pResponse->mutable_updateresult();
        pUpdateResponse->set_num(ret);
        pResponse->set_status(0);

	OMC_LOG("40201001|%s|2|DataAPI::update|%s|%s|%s|%d|0|success",
	 	Bmco::Util::Application::instance().config().getString("info.bol_name","bol123").c_str(),
	 	Bmco::DateTimeFormatter::format(sqlstarttime.timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	Bmco::DateTimeFormatter::format(Bmco::LocalDateTime().timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	exename.c_str(),
	 	sysPid
	 	);
		
	 OMC_LOG("40201001|%s|1|%s|%s|%s|%s|%d|0|success",
	 	Bmco::Util::Application::instance().config().getString("info.bol_name","bol123").c_str(),
	 	tableName.c_str(),
	 	Bmco::DateTimeFormatter::format(starttime.timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	Bmco::DateTimeFormatter::format(Bmco::LocalDateTime().timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	exename.c_str(),
	 	sysPid
	 	);
        return;
    }
    catch (Bmco::Exception& e) {
        ERROR_LOG(0, "occur exception: %s", e.displayText().c_str());
		exception=e.displayText();
    }
    catch (std::exception& e) {
        ERROR_LOG(0, "occur exception: %s", e.what());
		exception=e.what();
    }
    catch (...) {
        ERROR_LOG(0, "occur unknown exception");
		exception="exception";
    }

	OMC_LOG("40201001|%s|1|%s|%s|%s|%s|%d|1|%s",
	 	Bmco::Util::Application::instance().config().getString("info.bol_name","bol123").c_str(),
	 	tableName.c_str(),
	 	Bmco::DateTimeFormatter::format(starttime.timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	Bmco::DateTimeFormatter::format(Bmco::LocalDateTime().timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	exename.c_str(),
	 	sysPid,
	 	exception.c_str()
	 	);

    pResponse->set_status(1);
}

void ResponseOp::replace(const BolServiceRequest_Request& request, 
                         BolServiceResponse& response,int sysPid,const std::string &exename) {


	Bmco::LocalDateTime starttime;
	 std::string exception;
	 std::string tableName;
	
						 
    BolServiceResponse_Response *pResponse = response.add_msgs();
    setBolServiceHead(request.msghead(), *pResponse);

    if (!request.has_replace()) {
        ERROR_LOG(0, "request has no replace");
        pResponse->set_status(1);
        return;
    }

    try {
        const BolServiceRequest_ReplaceRequest& replaceRequest = request.replace();
        int fieldNum = replaceRequest.fields_size();
        FieldNameSet nameSet;
        int fieldIndex = 0;
        for (; fieldIndex < fieldNum; ++fieldIndex) {
            nameSet.insert(replaceRequest.fields(fieldIndex));
        }
        Matrix replaceRecord;
        replaceRecord.setFieldNameSet(nameSet);

        int recordNum = replaceRequest.records_size();
        for (int index = 0; index < recordNum; ++index) {
            const Record& record = replaceRequest.records(index);
            Pair row;
            fieldIndex = 0;
            int fieldValueIndex = 0;
            for (; fieldValueIndex < fieldNum; ++fieldValueIndex, ++fieldIndex) {
                const ::FieldValue& fieldValue = record.values(fieldValueIndex);
                if (fieldValue.isnull()) {
                    continue;
                }
                row.addFieldPair(replaceRequest.fields(fieldIndex), fieldValue.value());
            }
            replaceRecord.addRowValue(row);
        }

        tableName = replaceRequest.table();
	Bmco::LocalDateTime sqlstarttime;
        int ret = DataAPI::replace(tableName, replaceRecord);
        if (0 != ret) {
            ERROR_LOG(0, "DataAPI::replace return %d", ret);
            pResponse->set_status(1);

		OMC_LOG("40201001|%s|2|DataAPI::replace|%s|%s|%s|%d|1|replace error",
	 	Bmco::Util::Application::instance().config().getString("info.bol_name","bol123").c_str(),
	 	Bmco::DateTimeFormatter::format(sqlstarttime.timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	Bmco::DateTimeFormatter::format(Bmco::LocalDateTime().timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	exename.c_str(),
	 	sysPid
	 	);
		
			
		OMC_LOG("40201001|%s|1|%s|%s|%s|%s|%d|1|sqlite error",
	 	Bmco::Util::Application::instance().config().getString("info.bol_name","bol123").c_str(),
	 	tableName.c_str(),
	 	Bmco::DateTimeFormatter::format(starttime.timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	Bmco::DateTimeFormatter::format(Bmco::LocalDateTime().timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	exename.c_str(),
	 	sysPid
	 	);
            return;
        }


	OMC_LOG("40201001|%s|2|DataAPI::replace|%s|%s|%s|%d|0|success",
	 	Bmco::Util::Application::instance().config().getString("info.bol_name","bol123").c_str(),
	 	Bmco::DateTimeFormatter::format(sqlstarttime.timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	Bmco::DateTimeFormatter::format(Bmco::LocalDateTime().timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	exename.c_str(),
	 	sysPid
	 	);

	 OMC_LOG("40201001|%s|1|%s|%s|%s|%s|%d|0|success",
	 	Bmco::Util::Application::instance().config().getString("info.bol_name","bol123").c_str(),
	 	tableName.c_str(),
	 	Bmco::DateTimeFormatter::format(starttime.timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	Bmco::DateTimeFormatter::format(Bmco::LocalDateTime().timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	exename.c_str(),
	 	sysPid
	 	);
        pResponse->set_status(0);
        return;
    }
    catch (Bmco::Exception& e) {
        ERROR_LOG(0, "occur exception: %s", e.displayText().c_str());
		exception=e.displayText();
    }
    catch (std::exception& e) {
        ERROR_LOG(0, "occur exception: %s", e.what());
		exception=e.what();
    }
    catch (...) {
        ERROR_LOG(0, "occur unknown exception");
		exception="exception";
    }

    pResponse->set_status(1);

	OMC_LOG("40201001|%s|1|%s|%s|%s|%s|%d|1|%s",
	 	Bmco::Util::Application::instance().config().getString("info.bol_name","bol123").c_str(),
	 	tableName.c_str(),
	 	Bmco::DateTimeFormatter::format(starttime.timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	Bmco::DateTimeFormatter::format(Bmco::LocalDateTime().timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	exename.c_str(),
	 	sysPid,
	 	exception.c_str()
	 	);
	
}

void ResponseOp::heartBeat(const BolServiceRequest_Request& request,
                           BolServiceResponse& response, int sysPid,const std::string &exename,int heartbeattype) {
    BolServiceResponse_Response *pResponse = response.add_msgs();
    setBolServiceHead(request.msghead(), *pResponse);

    Bmco::LocalDateTime starttime;
    std::string exception;

    if (!request.has_heartbeat()) {
        ERROR_LOG(0, "request has no heartbeat");
        pResponse->set_status(1);
        return;
    }

    try {
        const BolServiceRequest_HeartBeatRequest& heartbeatRequest = request.heartbeat();
        Int64 bpcbID = heartbeatRequest.bpcbid();
        //Int32 sysPid = heartbeatRequest.syspid();

        Pair condition;
        condition.addFieldPair("BpcbID", bpcbID);
        Bmco::LocalDateTime now;
        Int64 heartbeatTime = now.timestamp().epochMicroseconds();
        Pair value;
        value.addFieldPair("HeartBeat", heartbeatTime);
        value.addFieldPair("SysPID", sysPid);
	if(heartbeattype == 1)
	{
		value.addFieldPair("Status", 2);
	}
		
	Bmco::LocalDateTime sqlstarttime;
        int ret = DataAPI::update("MetaBpcbInfo", &condition, value);
        if (0 >= ret) {
            ERROR_LOG(0, "DataAPI::upate return %d", ret);
            pResponse->set_status(1);

	OMC_LOG("40201001|%s|2|DataAPI::HeartBeat|%s|%s|%s|%d|1|HeartBeat error",
	 	Bmco::Util::Application::instance().config().getString("info.bol_name","bol123").c_str(),
	 	Bmco::DateTimeFormatter::format(sqlstarttime.timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	Bmco::DateTimeFormatter::format(Bmco::LocalDateTime().timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	exename.c_str(),
	 	sysPid
	 	);
			
			
	 OMC_LOG("40201001|%s|1|MetaBpcbInfo|%s|%s|%s|%d|1|sqlite error",
	 	Bmco::Util::Application::instance().config().getString("info.bol_name","bol123").c_str(),
	 	Bmco::DateTimeFormatter::format(starttime.timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	Bmco::DateTimeFormatter::format(Bmco::LocalDateTime().timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	exename.c_str(),
	 	sysPid
	 	);
			
            return;
        }

        pResponse->set_status(0);

	
	OMC_LOG("40201001|%s|2|DataAPI::HeartBeat|%s|%s|%s|%d|0|success",
	 	Bmco::Util::Application::instance().config().getString("info.bol_name","bol123").c_str(),
	 	Bmco::DateTimeFormatter::format(sqlstarttime.timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	Bmco::DateTimeFormatter::format(Bmco::LocalDateTime().timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	exename.c_str(),
	 	sysPid
	 	);

	 OMC_LOG("40201001|%s|1|MetaBpcbInfo|%s|%s|%s|%d|0|success",
	 	Bmco::Util::Application::instance().config().getString("info.bol_name","bol123").c_str(),
	 	Bmco::DateTimeFormatter::format(starttime.timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	Bmco::DateTimeFormatter::format(Bmco::LocalDateTime().timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	exename.c_str(),
	 	sysPid
	 	);
		
        return;
    }
    catch (Bmco::Exception& e) {
        ERROR_LOG(0, "occur exception: %s", e.displayText().c_str());
		exception=e.displayText();
    }
    catch (std::exception& e) {
        ERROR_LOG(0, "occur exception: %s", e.what());
		exception=e.what();
    }
    catch (...) {
        ERROR_LOG(0, "occur unknown exception");
		exception="exception";
    }

    pResponse->set_status(1);

     OMC_LOG("40201001|%s|1|MetaBpcbInfo|%s|%s|%s|%d|1|%s",
	 	Bmco::Util::Application::instance().config().getString("info.bol_name","bol123").c_str(),
	 	Bmco::DateTimeFormatter::format(starttime.timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	Bmco::DateTimeFormatter::format(Bmco::LocalDateTime().timestamp(), "%Y%m%d%H%M%S").c_str(),
	 	exename.c_str(),
	 	sysPid,
	 	exception.c_str()
	 	);
     
	
}

void ResponseOp::setBolServiceHead(const BolServiceHead& head, 
                                   BolServiceResponse_Response& response) {
    BolServiceHead *pHead = response.mutable_msghead();
    pHead->set_msgid(head.msgid());
    if (head.has_timestamp()) {
        pHead->set_timestamp(head.timestamp());
    }
}

/// WorkTask
WorkTask::WorkTask(const std::string& name,
                   Bmco::NotificationQueue& requestQueue,
                   Bmco::NotificationQueue& responseQueue)
                   : Bmco::Task(name),
                     _requestQueue(requestQueue),
                     _responseQueue(responseQueue) {
   
}

WorkTask::~WorkTask() {
    
}

void WorkTask::runTask() {
    INFORMATION_LOG("thread: %s start to run", name().c_str());

    bool statistic = false;
    Bmco::SharedPtr<Bmco::Timestamp> pTime;
    if (DBStatistics::enable()) {
        try {
            pTime = new Bmco::Timestamp;
            statistic = true;
        }
        catch (...) {}
    }

    while (!isCancelled()) {
        try {
            Bmco::Notification *pNf = _requestQueue.waitDequeueNotification();
            if (NULL == pNf) {
                break;
            }

            TaskNotification *pTaskNf = dynamic_cast<TaskNotification *>(pNf);
            if (NULL != pTaskNf) {
                BolServiceResponse response;
                BolServiceRequest  request;
                bool failed = false;
                
                if (statistic) {
                    pTime->update();
                }

                if (request.ParseFromString(pTaskNf->data())) {
                    if (statistic) {
                        PBStatistic::instance().statistics(false, pTime->elapsed());
                    }

                    DEBUG_LOG("request: %s", request.DebugString().c_str());
                    int pid = request.syspid();
			int proid= request.programid();

			//INFORMATION_LOG("get the request proid %d", proid);

			Matrix prodefmatx; 
			std::string exename="";
			int heartbeattype=0;
			if(!MemTableOper::instance().GetProgramDefbyId(proid,prodefmatx))
			{
				ERROR_LOG(0,"proid[%?d]:Failed to execute queryProgramDefByID on  MetaShmProgramDefTable", proid);
			}
			
		      exename= prodefmatx.getFieldValue(strDisplayName, 0).getString();
		      heartbeattype=prodefmatx.getFieldValue(strHeartBeatType, 0).getInt32();
			
                    const BolServiceProto& requestProto = request.proto();                    
                    BolServiceProto *pResponseProto = response.mutable_proto();
                    pResponseProto->set_version(requestProto.version());
                    pResponseProto->set_id(requestProto.id());
                    pResponseProto->set_clientid(requestProto.clientid());

                    int requestNum = request.msgs_size();
                    for (int index = 0; index < requestNum; ++index) {
                        const BolServiceRequest_Request& rRequest = request.msgs(index);
                        switch (rRequest.optype()) {
                        case BolServiceRequest_OperationType_ADD:
                            ResponseOp::add(rRequest, response, pid,exename);
                            break;

                        case BolServiceRequest_OperationType_ERASE:
                            ResponseOp::erase(rRequest, response, pid,exename);
                            break;

                        case BolServiceRequest_OperationType_QUERY:
                            ResponseOp::query(rRequest, response, pid,exename);
                            break;

                        case BolServiceRequest_OperationType_UPDATE:
                            ResponseOp::update(rRequest, response, pid,exename);
                            break;

                        case BolServiceRequest_OperationType_REPLACE:
                            ResponseOp::replace(rRequest, response, pid,exename);
                            break;

                        case BolServiceRequest_OperationType_HEARTBEAT:
                            ResponseOp::heartBeat(rRequest, response, pid,exename,heartbeattype);
                            break;

                        default:
                            break;
                        }
                    }
                }
                else {
                    if (statistic) {
                        PBStatistic::instance().statistics(false, pTime->elapsed());
                    }

                    // 断掉连接
                    failed = true;
                }
                
                if (!failed) {
		
                    std::string responseData;
					
                    if (statistic) {
                        pTime->update();
                    }

                    if (response.SerializeToString(&responseData)) {
                        if (statistic) {
                            PBStatistic::instance().statistics(true, pTime->elapsed());
                        }

                        pTaskNf->setData(responseData);
                        DEBUG_LOG("response: %s", response.DebugString().c_str());
                    }
                    else {
                        if (statistic) {
                            PBStatistic::instance().statistics(true, pTime->elapsed());
                        }

                        // 断掉连接
                        pTaskNf->setData("");
                        ERROR_LOG(0, "respone data is invalid, can not be serialized");
                    }
                }
                else {
                    // 断掉连接
                    pTaskNf->setData("");
                    ERROR_LOG(0, "request data is invalid");
                }                

                Bmco::Notification::Ptr pResponseNf = pTaskNf;
                _responseQueue.enqueueNotification(pResponseNf);
                ResponseNotify::instance().notify();

            }

		
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

    INFORMATION_LOG("thread: %s exit to run", name().c_str());
}

} // namespace BM35
