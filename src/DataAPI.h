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

/// �ֶ�ֵ FieldValue
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

/// �ֶε����ƺ��ֶε�ֵ�� FieldPair
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

/// �ֶε����ƺ��ֶε�ֵ�鼯�� Pair
class Pair {
public:
    Pair();

    ~Pair();

    int num() const; // �ֶε����ƺ��ֶε�ֵ��ĸ���

    // ��ȡ��i���ֶε����ƺ��ֶε�ֵ����ֶ�����, i��0��ʼ, ���i���ڵ����ֶε����ƺ��ֶε�ֵ�鼯�ϵĸ������׳��쳣std::range_error
    const std::string& getName(int i) const throw (std::range_error);

    // ��ȡ��i���ֶε����ƺ��ֶε�ֵ����ֶ�ֵ, i��0��ʼ, ���i���ڵ����ֶε����ƺ��ֶε�ֵ�鼯�ϵĸ������׳��쳣std::range_error    
    const std::string& getValueString(int i) const throw (std::range_error);
    const FieldValue&  getValue(int i) const throw (std::range_error);

    // ����ֶε����ƺ��ֶε�ֵ��
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

/// ����ֶ������ֶ�ֵ�Ķ�ά���� Matrix
class Matrix {
public:
    Matrix();

    ~Matrix();

    Matrix(const Matrix& matrix);

    Matrix& operator = (const Matrix& matrix);

    int fieldNameCount() const; // ��ά������ֶ�������

    int rowValueCount() const;  //  ��ά������ֶ�ֵ������

    // ���ö�ά������ֶ���, ֻ����ִ��1��, �ֶ��������ִ�Сд, ִ��1�κ�, ����ִ����Ч
    void setFieldNameSet(const FieldNameSet& name);

    // ���ά�����������, ����ӵ��ֶ��������ڶ�ά�����д���, ���򲻽������, ����false, �ɹ�����true, û����ӵ��ֶ������ֶ�ֵΪNULL
    bool addRowValue(const Pair& value);

    // ��ȡ��ά������ֶ�ֵ, �����ֶ�����ֵ���������в���, �ҵ����ض�ά������FieldValue�ĵ�ַ, �Ҳ�������NULL
    const FieldValue& getFieldValue(const std::string& fieldName, int i) const throw (std::range_error);

    // ��ȡ��ά������ֶ�ֵ, ����[i, j]���в���, �ҵ����ض�ά������FieldValue�ĵ�ַ, �Ҳ�������NULL
    const FieldValue& getFieldValue(int i, int j) const throw (std::range_error);

    // ��ȡ��ά����ĵ�i�е�������, �ɹ�����RowValue�ĵ�ַ, ʧ�ܷ���NULL
    const RowValue& getRowValue(int i) const throw (std::range_error);

    // ��ȡ��ά������ֶ����б�, �ֶ����͸�getRowValue��ȡ����RowValue���ֶ�ֵ��Ӧ
    const FieldNameList& getFieldNameList() const;

    // ��ն�ά����
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
    // ��ѯ��¼
    // pSelectedCondition Ϊ��ѯ����, ���û�п�������ΪNULL
    // �ɹ�����0, ʧ�ܷ���-1
    static int query(const std::string& tableName,
                     const Pair* pSelectedCondition,
                     Matrix& selectedResult);

    // ɾ����¼
    // pDeleteCondition Ϊɾ������, ���û�п�������ΪNULL
    // �ɹ�����ɾ���ļ�¼��, ʧ�ܷ���-1
    static int erase(const std::string& tableName,
                     const Pair* pDeleteCondition);

    // ���¼�¼
    // pUpdateCondition Ϊ��������, ���û�п�������ΪNULL
    // �ɹ����ظ��µļ�¼��, ʧ�ܷ���-1
    static int update(const std::string& tableName,
                      const Pair* pUpdateCondition,
                      const Pair& updateValue);

    // ��Ӽ�¼
    // �ɹ�����0, ʧ�ܷ���-1
    static int add(const std::string& tableName,
                   const Matrix& addRecord);

    // �滻��¼, ����ռ�¼, ������¼�¼
    // �ɹ�����0, ʧ�ܷ���-1
    static int replace(const std::string& tableName,
                       const Matrix& replaceRecord); 

    // ������, sqlΪ�������sql
    // �ɹ�����0, ʧ�ܷ���-1
    static int create(const std::string& sql);
}; // class DataAPI

} // namespace BM35

#endif // __DATAAPI_H
