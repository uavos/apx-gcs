
@OnTask()
{
  new msg{4+1+8};
  new canID=1821;
  msg{0}=canID;         //ID_0_7
  msg{1}=canID>>8;      //ID_8_15
  msg{2}=canID>>16;     //ID_16_23
  msg{3}=canID>>24;     //ID_24_31
  msg{4}=8;             //DLC - data length
  msg{4}|=0x80;         //IDE (bit 7) 1=ext,0=std

  //data
  msg{5}=0x31;
  msg{6}=0x32;
  msg{7}=0x33;
  msg{8}=0x34;
  msg{9}=0x35;
  msg{10}=0x36;
  msg{11}=0x37;
  msg{12}=0x38;

  serial_write(11,msg,4+1+msg{4},serialmode:NODE);
  //printf("can sent..");

  return 1000; //repeat every second
}


