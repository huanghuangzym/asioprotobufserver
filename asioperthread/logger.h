#ifndef HTTP_SERVER_MYLOG_HPP
#define HTTP_SERVER_MYLOG_HPP
  
#include <string>  
#include <boost/log/trivial.hpp>  
#include <boost/log/sources/logger.hpp>
  
  
using std::string;  
  
#define LOG_DEBUG\
    BOOST_LOG_SEV((MyLog::s_slg),(boost::log::trivial::debug))  
#define LOG_INFO\
    BOOST_LOG_SEV((MyLog::s_slg),(boost::log::trivial::info))  
#define LOG_ERROR\
    BOOST_LOG_SEV((MyLog::s_slg),(boost::log::trivial::error))  
#define LOG_WARNING\
    BOOST_LOG_SEV((MyLog::s_slg),(boost::log::trivial::warning))  
// ��ʹ��֮ǰ�����ȵ��� init  
// ʹ�÷�ʽ  LOG_DEBUG<<"test string";  
// Ҳ������boost �еĺ�  BOOST_LOG_TRIVIAL(info)<<"test msg";  
class MyLog  
{  
public:  
  MyLog();  
  ~MyLog(void);  
   
  // ��ʹ��֮ǰ�����ȵ��ô˺���  
  static void Init(const string & dir);  
  static void Log(const string & msg );  
  static boost::log::sources::severity_logger<boost::log::trivial::severity_level > s_slg;  
protected:  
private:  
  
};  


#endif 

