#ifndef __REDOLOG_H
#define __REDOLOG_H

#include "Bmco/FileStream.h"
#include "Bmco/Mutex.h"
#include <string>

namespace BM35 {

class ReDoLog {
public:
    ~ReDoLog();

    static ReDoLog& instance();

    bool log(const std::string& sql);

    void backup();

private:
    ReDoLog();

    ReDoLog(const std::string& path);

    ReDoLog(const ReDoLog&);

    ReDoLog& operator = (const ReDoLog&);

private:
    Bmco::FileOutputStream _fstream;

    Bmco::FastMutex _mtx;

    std::string _path;
}; // class ReDoLog

} // namespace BM35

#endif // __REDOLOG_H
