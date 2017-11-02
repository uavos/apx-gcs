//==============================================================================
#include <string.h>
#include <dmsg.h>
#include <string.h>
#include <fcntl.h>
#ifdef __APPLE__
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#else
#include <sys/sendfile.h>
#endif
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include "drv_sim.h"
#include "conf_node.h"
#include "Mandala.h"
#include "time_ms.h"
#include "tcp_ports.h"
extern _node node;
extern bool do_sim;
extern bool do_sim_imu;
static const char *firmware_path="/usr/local/bin/shiva";
static const char *firmware_path_tmp="/tmp/fw-update";
//==============================================================================
_drv_sim::_drv_sim(Bus *b,_files *afiles,_drv_ports *aports,const char *host_sim)
: _drv_conf(b,&sim_sn,&sim_fw,true),
  _tcp_client("sim"),
  wtime_s(0),rcounter(0),time_s(time),
  files(afiles),ports(aports)
{
  //tcpdebug=true;
  memcpy(&sim_sn,node.sn,sizeof(_node_sn));
  memcpy(&sim_fw,&node.info,sizeof(_node_info));
  strcpy((char*)sim_fw.name,"nav");

  if(!files->loadFile(&conf,sizeof(conf),true)){
    node.info.flags.conf_reset=1;
    dmsg("Conf error\n");
  }

  bus->BIND(evt_conf_reset);
  bus->BIND(evt_var_request);
  bus->BIND(evt_service);

  n_idx[0]=idx_acc;
  n_idx[1]=idx_gyro;
  n_idx[2]=idx_gps_pos;
  n_idx[3]=idx_gps_vel;
  n_idx[4]=idx_mag;
  n_idx[5]=idx_airspeed;
  memset(n_sz,0,sizeof(n_sz));
  memset(n_req,0,sizeof(n_req));
  sim_pwm_init=true;
  sim_pwm_crc=0;

  memset(&ldr_file,0,sizeof(_flash_file));
  ldr_mem.start_address=0;  //available flash for user program
  ldr_mem.size=0xFFFFFFFF;

  if(do_sim){
    //tcpdebug=true;
    connect(host_sim,TCP_PORT_SIM,false,"/sim");
  }
}
//==============================================================================
void _drv_sim::OnEvent(evt_conf_reset&)
{
  strncpy((char*)conf.xpl[0],"sim/joystick/yoke_roll_ratio",sizeof(_ft_lstr));
  strncpy((char*)conf.xpl[1],"sim/joystick/yoke_pitch_ratio",sizeof(_ft_lstr));
  strncpy((char*)conf.xpl[3],"sim/flightmodel/engine/ENGN_thro_use[0]",sizeof(_ft_lstr));
  strncpy((char*)conf.xpl[2],"sim/joystick/yoke_heading_ratio",sizeof(_ft_lstr));
  strncpy((char*)conf.xpl[4],"sim/flightmodel/controls/flaprqst",sizeof(_ft_lstr));
  strncpy((char*)conf.xpl[5],"sim/flightmodel/controls/parkbrake",sizeof(_ft_lstr));
  get_conf_crc();
}
//=============================================================================
void _drv_sim::conf_save(void)
{
  _drv_conf::conf_save();
  files->saveFile(&conf,sizeof(conf));
  get_conf_crc();
}
//==============================================================================
void _drv_sim::conf_reset(void)
{
  memset(&conf,0,sizeof(conf));
}
//==============================================================================
uint _drv_sim::conf_cmd(uint8_t *buf)
{
  uint sz=0;
  #ifndef LOADER
  #define NCONF(...)
  #define NCONFA(...)
  #define NCMD(aname,adescr) _conf_cmd_NCMD(aname,adescr)
  #include <config.h>
  NCMD(reconf,"Reset configuration")
  #undef NCONF
  #undef NCONFA
  #undef NCMD
  #endif
  return sz;
}
//-------------------------------------
uint _drv_sim::conf_dsc(uint8_t *buf,uint8_t num)
{
  #ifndef LOADER
  #define NCMD(...)
  #define NCONFA(atype,aname,asize,adescr,...) NCONF(atype,aname[asize],adescr, __VA_ARGS__ )
  #define NCARGS(...) #__VA_ARGS__
  #define NCONF(atype,aname,adescr,...) _conf_dsc_NCONF(atype,aname,adescr,__VA_ARGS__)
  #include <config_default.h>
  #include <config.h>
  #undef NCONF
  #undef NCMD
  #undef NCARGS
  #undef NCONFA
  #endif
  return 0;
}
//-------------------------------------
bool _drv_sim::conf_ptr(uint8_t **ptr,uint *sz,uint8_t num)
{
#ifndef LOADER
#define NCMD(...)
#define NCONF(atype,aname,adescr,...) NCONFA(atype,aname,1,adescr, __VA_ARGS__ )
#define NCONFA(atype,aname,asize,adescr,...) _conf_ptr_NCONF(conf,atype,aname,asize,adescr,__VA_ARGS__)
  switch(num){
#include <config_default.h>
#include <config.h>
#undef NCONF
#undef NCONFA
#undef NCMD
#endif
  }
  return false;
}
//-------------------------------------
void _drv_sim::get_conf_crc(void)
{
  uint16_t crc=0;
  for(uint i=0;i<sizeof(conf);i++)
    crc+=((uint8_t*)&conf)[i];
  if(crc!=sim_pwm_crc)sim_pwm_init=true;
  sim_pwm_crc=crc;
}
//==============================================================================
void _drv_sim::OnEvent(evt_var_request&)
{
  if(bus->packet.id==idx_sim){
    sim_pwm_init=true;
    dmsg("X-Plane controls assignments for PWM requested.\n");
  }
}
//==============================================================================
//==============================================================================
bool _drv_sim::writePacket(const uint8_t *buf,uint cnt)
{
  if(buf[0]==idx_downstream){
    //sim_packet.id=idx_sim;
    //sim_packet.data[0]=2;//mandala var
    //memcpy(&(sim_packet.data[1]),buf,cnt);
    //_if_udp::writePacket((uint8_t*)&sim_packet,cnt+bus_packet_size_hdr+1);
  }
  //write to simulator (xpl & pwm plain data struct only)
  if((time-wtime_s)<100)return true;
  wtime_s=time;
  sim_packet.id=idx_sim;
  if(sim_pwm_init){
    sim_pwm_init=false;
    sim_packet.data[0]=0;//controls
    memcpy(&(sim_packet.data[1]),conf.xpl,sizeof(conf.xpl));
    _tcp_client::write((uint8_t*)&sim_packet,sizeof(conf.xpl)+bus_packet_size_hdr+1);
  }
  sim_packet.data[0]=1;//pwm data
  memcpy(&(sim_packet.data[1]),(uint8_t*)&(ports->local_data.ch),sizeof(ports->local_data.ch));
  _tcp_client::write((uint8_t*)&sim_packet,sizeof(ports->local_data.ch)+bus_packet_size_hdr+1);
  return true;
}
//==============================================================================
uint _drv_sim::readPacket(uint8_t *buf,uint sz)
{
  uint ahrs_freq=AHRS_FREQ;
  if((time-time_s)>(1000/ahrs_freq)){
    time_s+=(1000/ahrs_freq);//=time;
    rcounter++;
    n_req[0]=true;
    n_req[1]=true;
    n_req[2]|=(rcounter%(ahrs_freq/GPS_FREQ))==0;
    n_req[3]|=(rcounter%(ahrs_freq/GPS_FREQ))==0;
    n_req[4]|=(rcounter%(ahrs_freq/50))==0;
    n_req[5]|=(rcounter%(ahrs_freq/30))==0;
    //dmsg("rx: %u\n",buf[0]);
  }
  for(uint i=0;i<n_cnt;i++)
    if(n_req[i] && n_sz[i]){
      return read_n(i,buf,sz);
    }
  //read from simulator (idx_sim wrapped vars)
  uint cnt=0;
  while(1){
    cnt=_tcp_client::read(buf,sz);
    if(!cnt)return 0;
    _bus_packet *packet=(_bus_packet*)buf;
    if(packet->id!=idx_sim)continue;//return 0;
    uint8_t *np=(uint8_t*)memchr(n_idx,packet->data[0],sizeof(n_idx));
    if(np){
      uint i=np-n_idx;
      memcpy(n_buf[i],buf,cnt);
      n_sz[i]=cnt;
      //n_req[0]|=i==0;
      //return 0;
    }else{
      if(packet->data[0]!=idx_theta || do_sim_imu==false)
        return cnt;
    }
  }
  return cnt;
}
//==============================================================================
uint _drv_sim::read_n(uint n,uint8_t *buf,uint sz)
{
  n_req[n]=false;
  memcpy(buf,n_buf[n],n_sz[n]);
  return n_sz[n];
}
//==============================================================================
//==============================================================================
void _drv_sim::OnEvent(evt_service&)
{
  //linux executable firmware update support
  uint dcnt=bus->packet_cnt-bus_packet_size_hdr_srv;
  if (bus->packet.srv.cmd!=apc_loader)return;
  if (bus->packet_flags.broadcast)return;
  if (!dcnt) return; //jump loader
  dcnt--;
  uint8_t *data=&(bus->packet.srv.data[1]);
  //received loader command with data
  switch(bus->packet.srv.data[0]){
    case ldc_init:{
      if(dcnt)break;
      //create tmp file..
      fd = fopen(firmware_path_tmp,"w");
      if(!fd){
        dmsg("Error: can't open file for writing (%s)\n",firmware_path_tmp);
        break;
      }
      dmsg("File created: %s\n",firmware_path_tmp);
      ldr_reply(bus,ldc_init,(uint8_t*)&ldr_mem,sizeof(_flash_file));
      break; //accept for timeout in LOADER
    }
    case ldc_file:{
      if(dcnt!=sizeof(_flash_file))break;
      memcpy(&ldr_file,data,sizeof(_flash_file));
      if(ldr_file.start_address!=ldr_mem.start_address)break;
      if(ldr_file.size>ldr_mem.size)break;
      ldr_mem.xor_crc=0;  //will be calculated by write
      file_size=ldr_file.size;
      ldr_reply(bus,ldc_file,(uint8_t*)&ldr_file,sizeof(_flash_file));
      break;
    }
    /*case ldc_write:{
      if(dcnt!=sizeof(_flash_data))break;
      if(!ldr_file.size)break;
      _flash_data *wp=(_flash_data*)data;
      if(ldr_file.size<wp->hdr.data_size)break; //error
      file_size-=wp->hdr.data_size;
      //write to file..
      if(fseek(fd,wp->hdr.start_address,SEEK_SET)!=0){
        dmsg("Error: can't seek file (%s)\n",firmware_path_tmp);
        break;
      }
      if(fwrite(wp->data,wp->hdr.data_size,1,fd)!=1){
        dmsg("Error: can't write to file (%s)\n",firmware_path_tmp);
        break;
      }
      //verify data & calc crc
      for(uint i=0;i<wp->hdr.data_size;i++) ldr_mem.xor_crc^=wp->data[i];
      //next ack..
      if((!file_size) && (ldr_mem.xor_crc!=ldr_file.xor_crc))break;
      ldr_reply(bus,ldc_write,(uint8_t*)&wp->hdr,sizeof(wp->hdr));
      if(file_size)break;
      if((wp->hdr.start_address+wp->hdr.data_size-ldr_mem.start_address)<ldr_file.size)
        break; //next
      //======= file flash finished ==========
      dmsg("File copy: %s -> %s\n",firmware_path_tmp,firmware_path);
      //ldr_done(bus);
      fclose(fd);
      //copy file
      int read_fd;
      int write_fd;
      struct stat stat_buf;
      off_t offset = 0;
      read_fd = ::open (firmware_path_tmp, O_RDONLY);
      ::fstat (read_fd, &stat_buf);
      ::remove(firmware_path);
      write_fd = ::open (firmware_path, O_WRONLY | O_CREAT, stat_buf.st_mode);
      if(sendfile (write_fd, read_fd, &offset, stat_buf.st_size)<=0){
        dmsg("Error: can't copy file to %s\n",firmware_path);
      }else{
        dmsg("Updated. Please, REBOOT the device.\n");
      }
      chmod(firmware_path, S_IRWXU|S_IRWXG|S_IRWXO);
      ::close (read_fd);
      ::close (write_fd);
      break;
    }*/
  }
}
//==============================================================================
bool _drv_sim::ldr_reply(Bus *bus,uint8_t cmd,const uint8_t *buf,uint cnt)
{
  bus->packet.srv.data[0]=cmd;
  if(buf&&cnt) memcpy(&(bus->packet.srv.data[1]),buf,cnt);
  return bus->write_srv(sn,apc_loader,bus->packet.srv.data,cnt+1);
}
//==============================================================================
