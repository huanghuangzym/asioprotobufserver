#ifndef __RESPONSENOTIFY_H
#define __RESPONSENOTIFY_H

#include "Bmco/Net/StreamSocket.h"

namespace BM35 {

class ResponseNotify {
public:
    ~ResponseNotify();

    static ResponseNotify& instance();

    void connect(const Bmco::Net::SocketAddress& address);

    void notify();

    void close();

private:
    ResponseNotify();

    ResponseNotify(const ResponseNotify&);

    ResponseNotify& operator = (const ResponseNotify&);

private:
    Bmco::Net::StreamSocket _notifySocket;
};

} // namespace BM35

#endif // __RESPONSENOTIFY_H
