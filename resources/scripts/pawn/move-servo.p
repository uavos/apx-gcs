main()
{
  new Float: step=0.01;
  new Float: v=0;
  for(;;){
    if(v>=1.0 || v<=-1.0) step=-step;
    v+=step;
    set_var(f_rc_roll,v);
    wait(100);
  }
}
