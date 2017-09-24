//#include <float>
//native test(a)


new x{}="12345";
main(){
  new Float: xf=0.5,Float: f2=0.5;
  //printf "Hello world\n"
  test(1)

  new rv=0, cnt=100000;
  for (new i = 0; i < cnt; i++)
  {
    rv += float(((i/xf)));
    //test();
    xf=cos((rv));
    xf+=f2
    xf=(f2*xf)/1.234;
    rv+= (xf)+x{i%5};
  }
  test(rv);
  rv*=2.5*cos(0)
  rv=strfloat(x);
  return rv
}


