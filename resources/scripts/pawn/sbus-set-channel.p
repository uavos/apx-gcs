forward @SBUS()
@SBUS()
{
  //bits are reversed on each byte (MSB-LSB)
  //crc calculated before bit reverse
  //port baudrate 100000, even, 2 stop bits (same as control)
  new msg{17};
  msg{0}=0x9F;
  msg{1}=0x80;  //channel 1 (starts from 0)
  msg{2}=0x18;  //servo number 24-XXXXXX
  msg{3}=0xB9;  //servo number XX-40332 - MSB
  msg{4}=0x31;  //servo number XX-40332 - LSB
  msg{5}=0xE8;
  msg{6}=0x4E;
  msg{7}=0x0B;
  msg{8}=0x01;
  msg{9}=0x01;
  msg{10}=0x01;
  msg{11}=0x00;
  msg{12}=0x00;
  msg{13}=0x2C;
  msg{14}=0x80;
  msg{15}=0xB4;
  msg{16}=0xC1; //CRC8 (2's complement): 0x100-(sum of msg{1-15} before bit reverse)

  serial_write(10,msg,17);
  printf("SBUS channel set\n");
}