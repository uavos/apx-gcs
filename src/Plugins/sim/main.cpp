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

#include "MandalaFields.h"
#include "Datalink.h"
//--------------------------------------------------------
Mandala var;
_node node;
_node_conf conf;
//============================================================================
// options
bool do_sim=false;
bool do_hil=false;
bool do_sim_imu=false;
bool do_sim_noise=false;
bool do_tcp=false;
const char *comm_dev="/dev/ttyUSB0";
const char *simhost="127.0.0.1";
const char *gcuhost="127.0.0.1";
const char *rec_file=NULL;
int getoptions(int argc, char *argv[ ]);
void sendStdout(bool do_open=false);
//============================================================================
_mandala mf;
static void mf_report(void)
{
  //_mandala::_test t;
  //t._mandala::_field_float_test::fromFloat(1.23);
  //mf.ap.altps=0;
  //mf.ap.sensor.air.airspeed=100;
  //dmsg("MFIELD: %.2f\n",(float)mf.ap.sensor.air.pt);
  //_mandala::_field_t<_mandala_float> *f=mf.ap.sensor.air.field(mf.ap.sensor.air.pt.index());
  //f->fromFloat(1.23);
  //fprintf(stdout,"%s=%.2f\n",f->name(),f->toFloat());
  //mf.altps=123.45;
  //mf.ap.sensor.imu.acc.x=1;
  //mf.ap.sensor.imu.gyro=Vect(1,2,3);
  //mf.ap.sensor.ils.dXY.dx=1;
  //mf.ap.sensor.ils.dXY.dy=-1;
  //mf.ap.state.maneuver.mode=mode_LANDING;
  /*fprintf(stdout,"\n***************************\n");
  for(_mandala::_field *f=mf.next_field(NULL);f;f=mf.next_field(f)){
    _mandala_index i=f->index();
    if(mf.is_grp0(i))fprintf(stdout,"\n%s: %s\n",mf.group(i)->name(),mf.group(i)->descr());
    if(mf.is_grp1(i))fprintf(stdout," %s: %s\n",mf.group(i)->group(i)->name(),mf.group(i)->group(i)->descr());
    if(mf.is_grp2(i))fprintf(stdout,"  %s: %s\n",mf.group(i)->group(i)->group(i)->name(),mf.group(i)->group(i)->group(i)->descr());
    fprintf(stdout,"%.4X %s (%s) %s %.2f\n",f->index(),f->name(),f->descr(),f->shortname(),mf.value(i));
  }*/

  uint fcnt=0,gcnt=0;
  fprintf(stdout,"\n\n\n\n***************************");
  for(_mandala::_group *g0=mf.next_group(NULL);g0;g0=mf.next_group(g0)){
    fprintf(stdout,"\n%s: %s\n",g0->name(),g0->descr());
    for(_mandala::_group *g1=g0->next_group(NULL);g1;g1=g0->next_group(g1)){
      fprintf(stdout,"  %s: %s\n",g1->name(),g1->descr());
      for(_mandala::_group *g2=g1->next_group(NULL);g2;g2=g1->next_group(g2)){
        fprintf(stdout,"    %s: %s\n",g2->name(),g2->descr());
        gcnt++;
        for(_mandala::_field *f=g2->next_field(NULL);f;f=g2->next_field(f)){
          fprintf(stdout,"      %.4X %s (%s) %.2f\n",f->index(),f->name(),f->descr(),f->toFloat());
          fcnt++;
        }
      }
    }
  }
  fprintf(stdout,"***************************\n");

  fprintf(stdout,"***************************\n");
  for(_mandala::_group *g0=mf.next_group(NULL);g0;g0=mf.next_group(g0)){
    fprintf(stdout,"\n%s: %s\n",g0->name(),g0->descr());
    for(_mandala::_group *g1=g0->next_group(NULL);g1;g1=g0->next_group(g1)){
      fprintf(stdout,"  %s: %s\n",g1->name(),g1->descr());
      for(_mandala::_group *g2=g1->next_group(NULL);g2;g2=g1->next_group(g2)){
        fprintf(stdout,"    %s: %s\n",g2->name(),g2->descr());
      }
    }
  }
  fprintf(stdout,"***************************\n");

  fprintf(stdout,"***************************\n");
  _mandala::_group *g=&mf.ap.control.stability;
  for(_mandala::_field *f=g->next_field(NULL);f;f=g->next_field(f)){
    fprintf(stdout,"%s\n",f->name());
  }
  fprintf(stdout,"***************************\n\n");
  fprintf(stdout,"Total fields: %u\n",fcnt);
  fprintf(stdout,"Total groups: %u\n",gcnt);
  fprintf(stdout,"Total size: %u bytes (%u per field)\n",(uint)sizeof(mf),(uint)sizeof(mf)/fcnt);
  fprintf(stdout,"\n\n");

  uint16_t idx=mf.pld.turret.tune.bias.yaw.index();
  fprintf(stdout,"%s\n",mf.field(idx)->path());
  //_mandala::_ap::_sensor::_imu &imu=mf.ap.sensor.imu;

  //test.imu.temp=12.23;
  fprintf(stdout,"%.4X: %.2f\n",mf.ap.sensor.imu.temp.index(),(double)mf.ap.sensor.imu.temp);

  //pack unpack test
  fprintf(stdout,"\n");
  fprintf(stdout,"\n");
  _mandala::_field *f=&mf.ap.control.power.servo;
  //_mandala::_field *f=&mf.payload.turret.cmd.att.roll;
  f->fromFloat(12.34);
  fprintf(stdout,"%.4X: %s=%.2f\n",f->index(),f->path(),f->toFloat());
  _mandala::_message data;
  memset(&data,0,sizeof(data));
  var.dump((uint8_t *)&data,MANDALA_MSGHDR_SIZE+f->pack(&data));
  //fprintf(stdout,"\n");
  f->fromFloat(0);
  uint rcnt=mf.ap.control.unpack(&data);
  fprintf(stdout,"%u: %s=%.2f\n",rcnt,f->path(),f->toFloat());

  //test custom field creation
  _mandala::_value fx;
  fprintf(stdout,"\n\nfx: %s\n",fx.path());
  rcnt=fx.unpack(&data);
  fprintf(stdout,"%u: %s=%.2f\n",rcnt,fx.path(),fx.toFloat());

  fprintf(stdout,"msg: %u\n",(uint)sizeof(_mandala::_message));

  {
    mf.ap.sensor.imu.gyro=Vect(1.234,2.345,3.456);
    _mandala_index idx=_mandala::index_ap_sensor_imu_gyro_x;
    _mandala::_group *g0=mf.group(idx);
    _mandala::_group *g1=g0->group(idx);
    _mandala::_group *g2=g1->group(idx);
    _mandala::_field *f=mf.field(idx);
    fprintf(stdout,"idx: %u\n",(uint)idx);
    fprintf(stdout,"group: %s.%s.%s\n",g0->name(),g1->name(),g2->name());
    fprintf(stdout,"path: %s\n",f->path());
    fprintf(stdout,"name: %s\n",f->name());
    fprintf(stdout,"value: %f\n",f->toFloat());
    fprintf(stdout,"value_direct: (%.2f,%.2f,%.2f)\n",(float)mf.ap.sensor.imu.gyro.x,(float)mf.ap.sensor.imu.gyro.y,(float)mf.ap.sensor.imu.gyro.z);

    //test unpacking
    memset(&data,0,sizeof(data));
    data.index=idx;
    data.size=1;
    data.dtype=_mandala::_message::dt_int;
    data.payload.data[0]=-12;
    var.dump((uint8_t *)&data,MANDALA_MSGHDR_SIZE+data.size);
    f->unpack(&data);
    fprintf(stdout,"value: %f\n",f->toFloat());

    //test vect pack-unpack
    {
      _mandala_index idx=_mandala::index_ap_sensor_imu_acc_x;
      fprintf(stdout,"\nvector: \n");
      Vect v(-180.0,22.345,181);
      mf.ap.sensor.imu.acc=v;
      data.index=idx;
      data.size=8;
      data.dtype=_mandala::_message::dt_vect;
      data.payload.vect.scale=_mandala::_message::vs_180;
      int32_t vmult=((1<<19));
      float vdiv=180.0;
      data.payload.vect.d20_1=(v[0]/vdiv)*vmult;
      data.payload.vect.d20_2=(v[1]/vdiv)*vmult;
      data.payload.vect.d20_3=(v[2]/vdiv)*vmult;
      //fprintf(stdout,"pack: %i (%X)\n",vd,(uint)data.payload.vect.d20_1);

      _mandala::_field *f=mf.field(idx);
      fprintf(stdout,"%s: (%f,%f,%f)\n",f->path(),mf.field(idx)->toFloat(),mf.field(idx+1)->toFloat(),mf.field(idx+2)->toFloat());
      var.dump((uint8_t *)&data,MANDALA_MSGHDR_SIZE+data.size);
      mf.ap.sensor.imu.acc.pack(&data,_mandala::_message::vs_180);
      var.dump((uint8_t *)&data,MANDALA_MSGHDR_SIZE+data.size);
      mf.ap.sensor.imu.acc.unpack(&data);
      fprintf(stdout,"%s: (%f,%f,%f)\n",f->path(),mf.field(idx)->toFloat(),mf.field(idx+1)->toFloat(),mf.field(idx+2)->toFloat());

      //test bundle (vectors) packing
      idx=_mandala::index_ap_sensor_gps_pos_lat;
      f=mf.field(idx);
      mf.ap.sensor.gps.pos=Vect(-51.4412345678,47.1234567890,15123.45678);
      fprintf(stdout,"%s: (%f,%f,%f)\n",f->path(),mf.field(idx)->toFloat(),mf.field(idx+1)->toFloat(),mf.field(idx+2)->toFloat());
      mf.ap.sensor.gps.pos.pack2(&data);
      var.dump((uint8_t *)&data,MANDALA_MSGHDR_SIZE+data.size);
      mf.ap.sensor.gps.pos.unpack(&data);
      fprintf(stdout,"%s: (%f,%f,%f)\n",f->path(),mf.field(idx)->toFloat(),mf.field(idx+1)->toFloat(),mf.field(idx+2)->toFloat());
      mf.ap.sensor.gps.pos=Vect();
      mf.unpack(&data);
      fprintf(stdout,"%s: (%f,%f,%f)\n",f->path(),mf.field(idx)->toFloat(),mf.field(idx+1)->toFloat(),mf.field(idx+2)->toFloat());

    }
  }
  Datalink dlink;
}
//============================================================================
int main(int argc, char *argv[])
{
  if (!getoptions(argc,argv)) return EXIT_FAILURE;

  //== fill Info structure ==
  memset(&node,0,sizeof(_node));
  //fill node name
  static const char *str_name="nav";//ASTRINGZ(TARGET);
  static const char *str_version=ASTRINGZ(VERSION);
  static const char *str_hardware=ASTRINGZ(HARDWARE);
  strncpy((char*)node.info.name,str_name,sizeof(_node_name)-1);
  strncpy((char*)node.info.version,str_version,sizeof(_node_version)-1);
  strncpy((char*)node.info.hardware,str_hardware,sizeof(_node_hardware)-1);
  //serial number
  node.sn[0]=0xFE;      //sim_sn


  static Bus bus;
  static _drv_dmsg drv_dmsg(&bus);

  dmsg("--------------------\n");
  dmsg("Shiva v%s\n",str_version);

  //-----------------------------------------
  //Comm port for BUS
  static _if_comm if_comm("comm",comm_dev,460800);
  bus.add_interface(&if_comm);

  //-----------------------------------------
  //Files
  static _files_sys files;

  //-----------------------------------------
  //sim and conf
  static _drv_ports drv_ports(&bus);
  static _drv_sim drv_sim(&bus,&files,&drv_ports,simhost);
  if(do_sim){
    bus.add_interface(&drv_sim);
    dmsg("X-plane sim: %s\n",simhost);
  }

  //-----------------------------------------
  //TCP telemetry
  if(do_tcp) {
    static _if_tcp_client if_tcp_client("tcp",gcuhost,TCP_PORT_SERVER);
    static _drv_dlink dlink(&bus,&if_tcp_client);
    dmsg("TCP datalink: %s:%u\n",gcuhost,TCP_PORT_SERVER);
  }
  dmsg("--------------------\n");
  dmsg("APx (linux) init...\n");

  //-----------------------------------------
  //AP cfg
  static _drv_conf_apcfg confAP(&bus,&files);

  //-----------------------------------------
  //Mandala extract vars
  static _drv_mandala drv_mandala(&bus,true,false,false);

  //-----------------------------------------
  //AHRS
  static _drv_ahrs ahrs(&bus);
  if(do_sim)ahrs.is_simulated=1;
  else do_sim_imu=do_sim_noise=false;
  if(do_sim_imu)ahrs.is_simulated|=2;
  if(do_sim_noise)ahrs.is_simulated|=4;
  //Nav
  static _drv_nav nav(&bus,&files);

  //Recording
  if(rec_file){
    static _drv_rec rec(&bus,rec_file);
  }

  //-----------------------------------------

  if(do_sim) bus.write(idx_ping,bus.packet.data,0); //run sim plugin not display
  //try{
  while(1) {
    bus.default_task();
  }//FOREVER
  //}catch(...){fdmsg(stdout,"Exception!!!\n");}
}
//============================================================================
int getoptions(int argc, char *argv[ ])
{
  extern char *optarg;
  int c;
  uint u;
  while ((c = getopt(argc, argv, "s::wu::rinat:d:b:x")) != -1){
    #ifdef __APPLE__
    if(optarg && optarg[0]=='-'){
      optarg=NULL;
      optind--;
      argc++;
    }
    #endif
    switch (c) {
      case 'x':
        mf_report();
        return 0;
      case 's':
        do_sim=true;
        if (optarg)simhost=optarg;
        break;
      case 'w':
        do_hil=true;
        break;
      case 'u':
        do_tcp=true;
        if (optarg)gcuhost=optarg;
        break;
      case 'i':
        do_sim_imu=true;
        dmsg("XKF enabled for simulation.\n");
        break;
      case 'n':
        do_sim_noise=true;
        dmsg("Noise added to simulation.\n");
        break;
      case 'a':
        var.cmode|=cmode_dlhd;
        dmsg("Downlink HD stream.\n");
        break;
      case 't':
        sscanf(optarg,"%u",&u);
        var.dl_period=u;
        dmsg("Downlink period set to %u ms (%g Hz).\n",u,1000.0/u);
        break;
      case 'd':
        comm_dev=optarg;
        dmsg("Using serial port %s\n",comm_dev);
        break;
      case 'b':
        rec_file=optarg;
        dmsg("Recording real-time data to %s\n",rec_file);
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
        \n",(char*)node.info.name);
        return 0;
    }
  }
  return 1;
}
//=============================================================================
//=============================================================================
