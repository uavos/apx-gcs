//=============================================================
#ifndef if_tcp_client_H
#define if_tcp_client_H
#include <fcntl.h>
#include <pthread.h>
#include "tcp_client.h"
#include "node.h"
#include "if_base.h"
//==============================================================================
// standard communication interface object
class _if_tcp_client : public _if_base, public _tcp_client
{
public:
    _if_tcp_client(const char *name, const char *host, uint port)
        : _if_base()
        , _tcp_client(name)
    {
        connect(host, port);
    }

protected:
    bool writePacket(const uint8_t *buf, uint cnt) { return write(buf, cnt); }
    uint readPacket(uint8_t *buf, uint sz) { return read(buf, sz); }
};
//==============================================================================
#endif
