//============================================================================
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/timeb.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/msg.h>
#include <wordexp.h>
//--------------------------------------------------------
#include <version.h>
#include "preprocessor.h"
#include "Mandala.h"
#include "conf_node.h"
#include "bus.h"
#include "tcp_ports.h"
#include "if_tcp_client.h"
#include "if_comm.h"
#include "drv_dlink.h"
#include "files_sys.h"
//--------------------------------------------------------
#include "drv_mandala.h"
#include "drv_conf_apcfg.h"
#include "drv_ahrs.h"
#include "drv_nav.h"
#include "drv_sim.h"
//#include "drv_hil.h"
#include "drv_rec.h"

//#include "MandalaFields.h"
//#include "Datalink.h"
//--------------------------------------------------------
Mandala var;
_node node;
//_node_conf conf;
//============================================================================
// options
bool do_sim = false;
bool do_hil = false;
bool do_sim_imu = false;
bool do_sim_noise = false;
bool do_tcp = false;
const char *comm_dev = nullptr;
const char *simhost = "127.0.0.1";
const char *gcuhost = "127.0.0.1";
const char *rec_file = nullptr;
int getoptions(int argc, char *argv[]);
void sendStdout(bool do_open = false);
//============================================================================
int main(int argc, char *argv[])
{
    if (!getoptions(argc, argv))
        return EXIT_FAILURE;

    //== fill Info structure ==
    memset(&node, 0, sizeof(_node));
    //fill node name
    static const char *str_name = APX_NODE;
    static const char *str_version = VERSION;
    static const char *str_hardware = APX_HW;
    strncpy((char *) node.info.name, str_name, sizeof(_node_name) - 1);
    strncpy((char *) node.info.version, str_version, sizeof(_node_version) - 1);
    strncpy((char *) node.info.hardware, str_hardware, sizeof(_node_hardware) - 1);
    //serial number
    node.sn[0] = 0xFE; //sim_sn

    static Bus bus;
    static _drv_dmsg drv_dmsg(&bus);

    dmsg("--------------------\n");
    dmsg("Shiva (v%s)\n", str_version);

    //-----------------------------------------
    //Comm port for BUS
    if (comm_dev) {
        static _if_comm if_comm("comm", comm_dev, 460800);
        bus.add_interface(&if_comm);
    }

    //-----------------------------------------
    //Files
    static _files_sys files;

    //-----------------------------------------
    //sim and conf
    static _drv_ports drv_ports(&bus);
    static _drv_sim drv_sim(&bus, &files, &drv_ports, simhost);
    if (do_sim) {
        bus.add_interface(&drv_sim);
        dmsg("X-plane sim: %s\n", simhost);
    }

    //-----------------------------------------
    //TCP telemetry
    if (do_tcp) {
        static _if_tcp_client if_tcp_client("tcp", gcuhost, TCP_PORT_SERVER);
        static _drv_dlink dlink(&bus, &if_tcp_client);
        dmsg("TCP datalink: %s:%u\n", gcuhost, TCP_PORT_SERVER);
    }
    dmsg("--------------------\n");
    dmsg("APX init...\n");

    //-----------------------------------------
    //AP cfg
    static _drv_conf_apcfg confAP(&bus, &files);

    //-----------------------------------------
    //Mandala extract vars
    static _drv_mandala drv_mandala(&bus, true, false, false);

    //-----------------------------------------
    //AHRS
    static _drv_ahrs ahrs(&bus);
    if (do_sim)
        ahrs.is_simulated = 1;
    else
        do_sim_imu = do_sim_noise = false;
    if (do_sim_imu)
        ahrs.is_simulated |= 2;
    if (do_sim_noise)
        ahrs.is_simulated |= 4;
    //Nav
    static _drv_nav nav(&bus, &files);

    //Recording
    if (rec_file) {
        static _drv_rec rec(&bus, rec_file);
    }

    //-----------------------------------------

    if (do_sim)
        bus.write(idx_ping, bus.packet.data, 0); //run sim plugin not display
    //try{
    while (1) {
        bus.default_task();
        usleep(1000);

        //timeout
        if (do_sim) {
            if ((time - drv_sim.time_sim) > 120000) {
                dmsg("Simulation timeout\n");
                bus.default_task();
                break;
            }
        }
    } //FOREVER
      //}catch(...){fdmsg(stdout,"Exception!!!\n");}
}
//============================================================================
int getoptions(int argc, char *argv[])
{
    extern char *optarg;
    int c;
    uint u;
    while ((c = getopt(argc, argv, "s::wu::rinat:d:b:x")) != -1) {
#ifdef __APPLE__
        if (optarg && optarg[0] == '-') {
            optarg = nullptr;
            optind--;
            argc++;
        }
#endif
        switch (c) {
        case 's':
            do_sim = true;
            if (optarg)
                simhost = optarg;
            break;
        case 'w':
            do_hil = true;
            break;
        case 'u':
            do_tcp = true;
            if (optarg)
                gcuhost = optarg;
            break;
        case 'i':
            do_sim_imu = true;
            dmsg("AHRS enabled for simulation.\n");
            break;
        case 'n':
            do_sim_noise = true;
            dmsg("Noise added to simulation.\n");
            break;
        case 'a':
            var.cmode |= cmode_dlhd;
            dmsg("Downlink HD stream.\n");
            break;
        case 't':
            sscanf(optarg, "%u", &u);
            var.dl_period = u;
            dmsg("Downlink period set to %u ms (%g Hz).\n", u, 1000.0 / u);
            break;
        case 'd':
            comm_dev = optarg;
            dmsg("Using serial port %s\n", comm_dev);
            break;
        case 'b':
            rec_file = optarg;
            dmsg("Recording real-time data to %s\n", rec_file);
            break;
        case 'h':
        default:
            dmsg("Usage:\n\
        %s [-sXXX.XXX.XXX.XXX] [-uXXX.XXX.XXX.XXX] [options]\n\
        -s <IP_ADDR> start simulation with x-plane machine (at IP_ADDR:9000/9001)\n\
        -w HIL simulation packets management\n\
        -u <IP_ADDR> use network for datalink\n\
        -d </dev/ttyXXX> change communication port device\n\
        -i enable AHRS in simulation (sim only)\n\
        -n enable noise in simulation (sim only)\n\
        -a higher precision for downstream\n\
        -t <dl_period> downstream period [ms]\n\
        -b <filename> record real-time data to file\
        \n",
                 (char *) node.info.name);
            return 0;
        }
    }
    return 1;
}
//=============================================================================
//=============================================================================
