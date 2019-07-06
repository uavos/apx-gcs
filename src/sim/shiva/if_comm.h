//=============================================================
#ifndef _if_comm_H
#define _if_comm_H
#include <fcntl.h>
#include <pthread.h>
#include "if_base.h"
#include "comm.h"
#include "node.h"
//==============================================================================
// standard communication interface object
class _if_comm : public _if_base, public Comm
{
public:
    _if_comm(const char *name, const char *filename, int baudrate)
        : _if_base()
        , Comm()
    {
        open(filename, baudrate, name);
        msgRx.type = mtypeRxM = 1;
        msqidRx = msqidRxM = msgget(ftok("/dev/random", 'a'), 0644 | IPC_CREAT);
        if (isOpen())
            pthread_create(&thrRx, 0, &start_threadRx, this);
    }
    bool writePacket(const uint8_t *buf, uint cnt)
    {
        if (!isOpen())
            return false;
        writeEscaped(buf, cnt);
        return true;
    }

    uint readPacket(uint8_t *buf, uint sz)
    {
        int cnt = msgrcv(msqidRxM, &msgRxM, BUS_MAX_PACKET, mtypeRxM, IPC_NOWAIT);
        if (cnt <= 0) {
            usleep(1000);
            return 0;
        }
        if (cnt > (int) sz)
            return 0;
        memcpy(buf, msgRxM.data, cnt);
        //printf("%s\t< %u\n",name,cnt);
        return cnt;
    }

private:
    pthread_t thrRx;
    static void *start_threadRx(void *obj)
    {
        reinterpret_cast<_if_comm *>(obj)->threadRx();
        pthread_exit(nullptr);
    }
    void threadRx()
    {
        while (1) {
            uint cnt = readEscaped(msgRx.data, sizeof(msgRx.data));
            if (!cnt)
                continue;
            //printf("%s\t< %u\n",name,cnt);
            msgsnd(msqidRx, &msgRx, cnt, 0);
        } // forever
    }

    int msqidRx, msqidRxM;
    long mtypeRxM;
    typedef struct
    {
        long type; //must be long (used by filter)
        uint8_t data[BUS_MAX_PACKET];
    } _ipc_msg;
    _ipc_msg msgRx, msgRxM;
};
//==============================================================================
#endif
