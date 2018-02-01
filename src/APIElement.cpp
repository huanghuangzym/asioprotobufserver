#include "Bmco/NumberParser.h"
#include "Bmco/NumberFormatter.h"
#include "Bmco/String.h"
#include <sstream>
#include <exception>
#ifdef __USE_AS_CLIENT
#include "BolServiceAPI.h"
#else
#include "DataAPI.h"
#include "BolServiceLog.h"
#endif

namespace BM35 {

/// FieldValue
FieldValue::FieldValue() : _isNULL(true) {

}

FieldValue::FieldValue(const std::string& value)
    : _value(value), _isNULL(false) {

}

FieldValue::FieldValue(const Int32& value) {
    setInt32(value);
}

FieldValue::FieldValue(const Int64& value) {
    setInt64(value);
}

FieldValue::FieldValue(const double& value) {
    setDouble(value);
}

FieldValue::~FieldValue() {

}

FieldValue::FieldValue(const FieldValue& fieldValue) {
    _value = fieldValue._value;
    _isNULL = fieldValue._isNULL;
}

FieldValue& FieldValue::operator =(const FieldValue& fieldValue) {
    if (this != &fieldValue) {
        _value = fieldValue._value;
        _isNULL = fieldValue._isNULL;
    }

    return *this;
}

bool FieldValue::operator ==(const FieldValue& fieldValue) const {
    return _value == fieldValue._value;
}

void FieldValue::setString(const std::string& value) {
    _value = value;
    _isNULL = false;
}

const std::string& FieldValue::getString() const {
    return _value;
}

void FieldValue::setInt32(const Int32& value) {
    _value = Bmco::NumberFormatter::format(value);
    _isNULL = false;
}

Int32 FieldValue::getInt32() const throw (std::logic_error) {
    Int32 value;
    if (!Bmco::NumberParser::tryParse(_value, value)) {
        std::string msg = "can not parser " + _value + " to int32";
#if !defined(__USE_AS_CLIENT)
        ERROR_LOG(0, "%s", msg.c_str());        
#endif
        throw std::logic_error(msg);
    }    

    return value;
}

void FieldValue::setInt64(const Int64& value) {
    _value = Bmco::NumberFormatter::format(value);
    _isNULL = false;
}

Int64 FieldValue::getInt64() const throw (std::logic_error) {
    Int64 value;
    if (!Bmco::NumberParser::tryParse64(_value, value)) {
        std::string msg = "can not parser " + _value + " to int64";
#if !defined(__USE_AS_CLIENT)
        ERROR_LOG(0, "%s", msg.c_str());
#endif
        throw std::logic_error(msg);
    }
    
    return value;
}

void FieldValue::setDouble(const double& value){
    _value = Bmco::NumberFormatter::format(value);
    _isNULL = false;
}

double FieldValue::getDouble() const throw (std::logic_error) {
    double value;
    if (!Bmco::NumberParser::tryParseFloat(_value, value)) {
        std::string msg = "can not parser " + _value + " to double";
#if !defined(__USE_AS_CLIENT)
        ERROR_LOG(0, "%s", msg.c_str());
#endif
        throw std::logic_error(msg);
    }
    
    return value;
}

bool FieldValue::isNULL() const {
    return _isNULL;
}

#if !defined(__USE_AS_CLIENT)

bool FieldValue::getInt32(Int32& value) const {
    try {
        value = getInt32();
        return true;
    }
    catch (...) {
        return false;
    }
}

bool FieldValue::getInt64(Int64& value) const {
    try {
        value = getInt64();
        return true;
    }
    catch (...) {
        return false;
    }
}

bool FieldValue::getDouble(double& value) const {
    try {
        value = getDouble();
        return true;
    }
    catch (...) {
        return false;
    }
}

#endif

/// 字段的名称和字段的值组集合 Pair
Pair::Pair() : _pairNum(0) {

}

Pair::~Pair() {
    _pairSet.clear();
}

int Pair::num() const {
    return _pairNum;
}

const std::string& Pair::getName(int i) const throw(std::range_error) {
    if (i >= _pairNum || i < 0) {
        std::ostringstream msg;
        msg << "pair set number: " << _pairNum << ", i: "
            << i << ", i out range of pair set number";
        throw std::range_error(msg.str());
    }
    else {
        return _pairSet[i].fieldName;
    }
}

const std::string& Pair::getValueString(int i) const throw (std::range_error) {
    if (i >= _pairNum || i < 0) {
        std::ostringstream msg;
        msg << "pair set number: " << _pairNum << ", i: "
            << i << ", i out range of pair set number";
        throw std::range_error(msg.str());
    }

    return _pairSet[i].fieldValue.getString();
}

const FieldValue& Pair::getValue(int i) const throw (std::range_error) {
    if (i >= _pairNum) {
        std::ostringstream msg;
        msg << "condition number: " << _pairNum << ", i: "
            << i << ", i out range of condition number";
        throw std::range_error(msg.str());
    }
    
    return _pairSet[i].fieldValue;
}

void Pair::addFieldPair(const std::string& name, const Int32& value) {
    _pairSet.push_back(FieldPair(name, value));
    ++_pairNum;
}

void Pair::addFieldPair(const std::string& name, const Int64& value) {
    _pairSet.push_back(FieldPair(name, value));
    ++_pairNum;
}

void Pair::addFieldPair(const std::string& name, const double& value) {
    _pairSet.push_back(FieldPair(name, value));
    ++_pairNum;
}

void Pair::addFieldPair(const std::string& name, const std::string& value) {
    _pairSet.push_back(FieldPair(name, value));
    ++_pairNum;
}

void Pair::addFieldPair(const std::string& name, const FieldValue& value) {
    _pairSet.push_back(FieldPair(name, value));
    ++_pairNum;
}

/// 表的字段名和字段值的二维数组 Matrix
Matrix::Matrix() : _fieldNameCount(0), _rowValueCount(0) {

}

Matrix::~Matrix() {

}

Matrix::Matrix(const Matrix& matrix) {
    assign(matrix);
}

Matrix& Matrix::operator = (const Matrix& matrix) {
    if (this != &matrix) {
        assign(matrix);
    }
    return *this;
}

void Matrix::assign(const Matrix& matrix) {
    clear();

    FieldNameSet nameSet;
    const FieldNameList& nameList = matrix.getFieldNameList();
    int fieldNum = matrix.fieldNameCount();
    int fieldIdx = 0;
    for (; fieldIdx < fieldNum; ++fieldIdx) {
        nameSet.insert(nameList[fieldIdx]);
    }
    setFieldNameSet(nameSet);

    bool failed = false;
    int rowNum = matrix.rowValueCount();
    for (int index = 0; index < rowNum; ++index) {
        Pair record;
        for (fieldIdx = 0; fieldIdx < fieldNum; ++fieldIdx) {
            const FieldValue& field = matrix.getFieldValue(index, fieldIdx);
            record.addFieldPair(nameList[fieldIdx], field);
        }
        if (failed) {
            break;
        }
        addRowValue(record);
    }
}

int Matrix::fieldNameCount() const {
    return _fieldNameCount;
}

int Matrix::rowValueCount() const {
    return _rowValueCount;
}

void Matrix::setFieldNameSet(const FieldNameSet& name) {
    if (0 != _fieldNameCount) {
#if !defined(__USE_AS_CLIENT)
        ERROR_LOG(0, "_fieldNameCount = %d != 0", _fieldNameCount);
#endif
        return;
    }

    int index = 0;
    FieldNameSet::const_iterator itr = name.begin();
    for (; itr != name.end(); ++itr) {
        _fieldNameList.push_back(*itr);
        std::string fieldName = *itr;
        fieldName = Bmco::toLower(fieldName);
        _nameToIndexMap.insert(std::make_pair(fieldName, index++));
    }
    _fieldNameCount = name.size();
}

bool Matrix::addRowValue(const Pair& value) {
    if (0 == _fieldNameCount) {
#if !defined(__USE_AS_CLIENT)
        ERROR_LOG(0, "_fieldNameCount = 0");
#endif
        return false;
    }

    RowValue row;
    row.resize(_fieldNameCount);
    int addValueNum = value.num();
    for (int index = 0; index < addValueNum; ++index) {
        std::string name = value.getName(index);
        name = Bmco::toLower(name);
        std::map<std::string, int>::const_iterator itr = _nameToIndexMap.find(name);
        if (itr == _nameToIndexMap.end()) {
#if !defined(__USE_AS_CLIENT)
            ERROR_LOG(0, "field name: %s in not set in matrix", name.c_str());
#endif
            return false;
        }
        else {
            if (!value.getValue(index).isNULL()) {
                row[itr->second].setString(value.getValueString(index));
            }            
        }
    }
    _matrixValue.push_back(row);
    ++_rowValueCount;

    return true;
}

const FieldValue& Matrix::getFieldValue(int i, int j) const throw (std::range_error) {
    if (0 == _fieldNameCount) {
        throw std::range_error("has no field");
    }

    if (j < 0 || j >= _fieldNameCount) {
        std::ostringstream msg;
        msg << "field num : " << _fieldNameCount << ", j: " << j << " out of range";
        throw std::range_error(msg.str());
    }

    if (0 == _rowValueCount) {
        throw std::range_error("has no record");
    }

    if (i < 0 || i >= _rowValueCount) {
        std::ostringstream msg;
        msg << "record num : " << _rowValueCount << ", i: " << i << " out of range";
        throw std::range_error(msg.str());
    }    
    
    const RowValue& row = _matrixValue[i];
    return row[j];
}

const FieldValue& Matrix::getFieldValue(const std::string& fieldName, int i) const throw (std::range_error) {
    if (0 == _fieldNameCount) {
        throw std::range_error("has no field");
    }

    if (0 == _rowValueCount) {
        throw std::range_error("has no record");
    }

    if (i < 0 || i >= _rowValueCount) {
        std::ostringstream msg;
        msg << "record num : " << _rowValueCount << ", i: " << i << " out of range";
        throw std::range_error(msg.str());
    }

    std::string name = Bmco::toLower(fieldName);
    std::map<std::string, int>::const_iterator itr = _nameToIndexMap.find(name);
    if (itr == _nameToIndexMap.end()) {
        std::string msg = fieldName + " can not found";
        throw std::range_error(msg);
    }
    
    int j = itr->second;
    if (j < 0 || j >= _fieldNameCount) {
        std::ostringstream msg;
        msg << "fieldname: " << fieldName << " out of range";
        throw std::range_error(msg.str());
    }
    return getFieldValue(i, j);
}

const RowValue& Matrix::getRowValue(int i) const throw (std::range_error) {
    if (i < 0 || i >= _rowValueCount) {
        std::ostringstream msg;
        msg << "i: " << i << " out of range";
        throw std::range_error(msg.str());
    }

    return _matrixValue[i];
}

const FieldNameList& Matrix::getFieldNameList() const {
    return _fieldNameList;
}

void Matrix::clear() {
    _fieldNameList.clear();
    _nameToIndexMap.clear();
    _matrixValue.clear();
    _fieldNameCount = 0;
    _rowValueCount = 0;
}

} // namespace BM35
