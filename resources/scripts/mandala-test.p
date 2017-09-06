//#include <float>
//native test(a)
new x{}="12345";
main(){
  new Float: xf=0.5,Float: f2=0.5;
  print "Hello world\n"

  new rv=0, cnt=1;
  for (new i = 0; i < cnt; i++) {
    rv += float(((i/xf)));
    //test();
    xf=cos((rv));
    xf+=f2
    xf=(f2*xf)/1.234;
    rv+= (xf)+x{i%5};
    printf("i=%d\n",i)
  }
  
  rv*=2.5*cos(0)
  rv=strfloat(x);
  printf("f:%.2f\n",f2);

  printf("idx=%d\n",f_cmd_altitude);
  printf("val=%.2f",get_var(f_cmd_altitude));
  set_var(f_cmd_altitude,123);
  printf("val=%.2f\n",get_var(f_cmd_altitude));
  set_var(f_mode,mode_LANDING,true);
  
  /*for (new i = 0; i < 10; i++){
    new Float: v=get_var(f_cmd_altitude,true);
    printf("wait=%.2f\n",v);
    set_var(f_airspeed,v*0.1,true);
  }*/
  //return rv
}


