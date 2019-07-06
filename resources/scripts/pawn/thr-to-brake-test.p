main()
{
	for(;;){
		new Float: thr=get_var(f_ctr_throttle,true);
		if(thr>=0.98)set_var(f_ctr_brake,1);
		else if(thr<=0.5 && get_var(f_ctr_brake)>0)set_var(f_ctr_brake,0);
		//wait(100);
	}
}