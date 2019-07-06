//==============================================================================
#ifndef drv_sim_H
#define drv_sim_H
//==============================================================================
#include <dmsg.h>
#include "files.h"
#include "drv_conf.h"
#include "drv_ports.h"
#include "tcp_client.h"
//==============================================================================
class _drv_sim : public _drv_conf,
                 public _tcp_client,
                 public _if_base,
                 public Dispatcher<evt_conf_reset>::Listener,
                 public Dispatcher<evt_var_request>::Listener,
                 public Dispatcher<evt_service>::Listener
{
public:
    _drv_sim(Bus *b, _files *afiles, _drv_ports *aports, const char *host_sim);
    uint time_sim;

protected:
    using _drv_conf::OnEvent;
    void OnEvent(evt_conf_reset &);
    void OnEvent(evt_var_request &);
    void OnEvent(evt_service &);
    //iface override
    bool writePacket(const uint8_t *buf, uint cnt);
    uint readPacket(uint8_t *buf, uint sz);

private:
    _bus_packet sim_packet;
    _drv_ports *ports;
    //conf
    _files *files;
    _node_sn sim_sn;
    _node_info sim_fw;
    uint conf_cmd(uint8_t *buf);
    uint conf_dsc(uint8_t *buf, uint8_t num);
    bool conf_ptr(uint8_t **ptr, uint *sz, uint8_t num);
    void conf_reset(void);
    void conf_save(void);

    //iface
    uint16_t sim_pwm_crc;
    void get_conf_crc(void);
    bool sim_pwm_init;
    uint wtime_s, rcounter, time_s;
    uint read_n(uint n, uint8_t *buf, uint sz);
    enum { n_cnt = 6 };
    uint8_t n_idx[n_cnt];
    uint8_t n_buf[n_cnt][BUS_MAX_PACKET];
    uint n_sz[n_cnt];
    bool n_req[n_cnt];

    //loader
    _flash_file ldr_mem, ldr_file;
    uint32_t file_size;
    bool ldr_reply(Bus *bus, uint8_t cmd, const uint8_t *buf, uint cnt);
    FILE *fd;
};
//==============================================================================
#endif
