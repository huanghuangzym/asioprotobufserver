#ifndef __DATAAPI_H
#define __DATAAPI_H

#include <string>
#include <vector>
#include <set>
#include <map>
#include <stdexcept>

namespace BM35 {

#if defined(_MSC_VER)
//
// Windows/Visual C++
//
typedef int      Int32;
typedef __int64  Int64;
#elif defined(__GNUC__) || defined(__clang__)
//
// Unix/GCC
//
typedef int                 Int32;
    #if defined(__LP64__)
        typedef long        Int64;
    #else
        typedef long long   Int64;
    #endif
#elif defined(__DECCXX)
//
// Compaq C++
//
typedef int        Int32;
typedef __int64    Int64;
#elif defined(__HP_aCC)
//
// HP Ansi C++
//
typedef int                 Int32;
    #if defined(__LP64__)
        typedef long        Int64;
    #else
        typedef long long   Int64;
    #endif
#elif defined(__SUNPRO_CC)
//
// SUN Forte C++
//
typedef signed int          Int32;
    #if defined(__sparcv9)
        typedef long        Int64;
    #else
        typedef long long   Int64;
    #endif
#elif defined(__IBMCPP__) 
//
// IBM XL C++
//
typedef int                 Int32;
    #if defined(__64BIT__)
        typedef long        Int64;
    #else
        typedef long long   Int64;
    #endif
#elif defined(__sgi) 
//
// MIPSpro C++
//
typedef int             Int32;
    #if _MIPS_SZLONG == 64
        typedef long        Int64;
    #else
        typedef long long   Int64;
    #endif
#elif defined(_DIAB_TOOL)
typedef int         Int32;
typedef long long   Int64;
#endif

/// 字段值 FieldValue
class FieldValue {
public:
    FieldValue();

    FieldValue(const std::string& value);    

    FieldValue(const Int32& value);

    FieldValue(const Int64& value);

    FieldValue(const double& value);

    ~FieldValue();

    FieldValue(const FieldValue& fieldValue);

    FieldValue& operator = (const FieldValue& fieldValue);

    bool operator == (const FieldValue& fieldValue) const;

    void setString(const std::string& value);
    const std::string& getString() const;

    void setInt32(const Int32& value);
    bool getInt32(Int32& value) const;
    Int32 getInt32() const throw (std::logic_error);

    void setInt64(const Int64& value);
    bool getInt64(Int64& value) const;
    Int64 getInt64() const throw (std::logic_error);

    void setDouble(const double& value);
    bool getDouble(double& value) const;
    double getDouble() const throw (std::logic_error);

    bool isNULL() const;

private:
    std::string _value;
    bool _isNULL;
}; // FieldValue

/// 字段的名称和字段的值组 FieldPair
struct FieldPair {
    FieldPair() {

    }

    FieldPair(const std::string& name, const Int32& value)
        : fieldName(name), fieldValue(value) {

    }

    FieldPair(const std::string& name, const Int64& value)
        : fieldName(name), fieldValue(value) {

    }

    FieldPair(const std::string& name, const double& value)
        : fieldName(name), fieldValue(value) {

    }

    FieldPair(const std::string& name, const std::string& value)
        : fieldName(name), fieldValue(value) {

    }

    FieldPair(const std::string& name, const FieldValue& value)
        : fieldName(name), fieldValue(value) {

    }

    FieldPair(const FieldPair& fieldCondition) {
        fieldName = fieldCondition.fieldName;
        fieldValue = fieldCondition.fieldValue;
    }

    ~FieldPair() {

    }

    FieldPair& operator = (const FieldPair& fieldCondition) {
        if (this != &fieldCondition) {
            fieldName = fieldCondition.fieldName;
            fieldValue = fieldCondition.fieldValue;
        }
        return *this;
    }

    std::string fieldName;
    FieldValue  fieldValue;
}; // FieldCondition

/// 字段的名称和字段的值组集合 Pair
class Pair {
public:
    Pair();

    ~Pair();

    int num() const; // 字段的名称和字段的值组的个数

    // 获取第i个字段的名称和字段的值组的字段名称, i从0开始, 如果i大于等于字段的名称和字段的值组集合的个数将抛出异常std::range_error
    const std::string& getName(int i) const throw (std::range_error);

    // 获取第i个字段的名称和字段的值组的字段值, i从0开始, 如果i大于等于字段的名称和字段的值组集合的个数将抛出异常std::range_error    
    const std::string& getValueString(int i) const throw (std::range_error);
    const FieldValue&  getValue(int i) const throw (std::range_error);

    // 添加字段的名称和字段的值组
    void addFieldPair(const std::string& name, const Int32& value);
    void addFieldPair(const std::string& name, const Int64& value);
    void addFieldPair(const std::string& name, const double& value);
    void addFieldPair(const std::string& name, const std::string& value);
    void addFieldPair(const std::string& name, const FieldValue& value);

private:
    std::vector<FieldPair> _pairSet;

    int _pairNum;
}; // Pair

typedef std::set<std::string>    FieldNameSet;

typedef std::vector<std::string> FieldNameList;

typedef std::vector<FieldValue>  RowValue;

/// 表的字段名和字段值的二维数组 Matrix
class Matrix {
public:
    Matrix();

    ~Matrix();

    Matrix(const Matrix& matrix);

    Matrix& operator = (const Matrix& matrix);

    int fieldNameCount() const; // 二维数组的字段名个数

    int rowValueCount() const;  //  二维数组的字段值的行数

    // 设置二维数组的字段名, 只可以执行1次, 字段名不区分大小写, 执行1次后, 后面执行无效
    void setFieldNameSet(const FieldNameSet& name);

    // 向二维数组添加数据, 待添加的字段名必须在二维数组中存在, 否则不进行添加, 返回false, 成功返回true, 没有添加的字段名的字段值为NULL
    bool addRowValue(const Pair& value);

    // 获取二维数组的字段值, 根据字段名和值的行数进行查找, 找到返回二维数组中FieldValue的地址, 找不到返回NULL
    const FieldValue& getFieldValue(const std::string& fieldName, int i) const throw (std::range_error);

    // 获取二维数组的字段值, 根据[i, j]进行查找, 找到返回二维数组中FieldValue的地址, 找不到返回NULL
    const FieldValue& getFieldValue(int i, int j) const throw (std::range_error);

    // 获取二维数组的第i行的行数据, 成功返回RowValue的地址, 失败返回NULL
    const RowValue& getRowValue(int i) const throw (std::range_error);

    // 获取二维数组的字段名列表, 字段名和跟getRowValue获取的行RowValue的字段值对应
    const FieldNameList& getFieldNameList() const;

    // 清空二维数组
    void clear();

private:
    void assign(const Matrix& matrix);

private:
    FieldNameList _fieldNameList;

    std::map<std::string, int> _nameToIndexMap;

    std::vector<RowValue> _matrixValue;

    int _fieldNameCount;

    int _rowValueCount;
}; /// Matrix

class DataAPI {
public:
    // 查询记录
    // pSelectedCondition 为查询条件, 如果没有可以设置为NULL
    // 成功返回0, 失败返回-1
    static int query(const std::string& tableName,
                     const Pair* pSelectedCondition,
                     Matrix& selectedResult);

    // 删除记录
    // pDeleteCondition 为删除条件, 如果没有可以设置为NULL
    // 成功返回删除的记录数, 失败返回-1
    static int erase(const std::string& tableName,
                     const Pair* pDeleteCondition);

    // 更新记录
    // pUpdateCondition 为更新条件, 如果没有可以设置为NULL
    // 成功返回更新的记录数, 失败返回-1
    static int update(const std::string& tableName,
                      const Pair* pUpdateCondition,
                      const Pair& updateValue);

    // 添加记录
    // 成功返回0, 失败返回-1
    static int add(const std::string& tableName,
                   const Matrix& addRecord);

    // 替换记录, 先清空记录, 再添加新记录
    // 成功返回0, 失败返回-1
    static int replace(const std::string& tableName,
                       const Matrix& replaceRecord); 

    // 创建表, sql为创建表的sql
    // 成功返回0, 失败返回-1
    static int create(const std::string& sql);
}; // class DataAPI

} // namespace BM35

#endif // __DATAAPI_H
