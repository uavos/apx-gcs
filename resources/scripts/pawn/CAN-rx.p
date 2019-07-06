
main()
{
  serial_listen(12,"@OnSerialTest");
}



forward @OnSerialTest(cnt);

@OnSerialTest(cnt)
{
    new address = serial_byte(0) + serial_byte(1) * 256 + serial_byte(2) * 65536 + serial_byte(3) * 16777216;
    printf("rx(%d): %d\n",cnt,address);
}



