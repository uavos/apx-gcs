@OnTask()
{
  new Float:v;
  v=get_gpio(0);
  set_gpio(1,v);

  set_control(1,v*2-1);
  return 100;   //100ms period for next iterations
}
