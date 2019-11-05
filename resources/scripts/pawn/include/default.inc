//#include <core>
//#include <console>
/*
  default include file for STM32 AMX
*/

#pragma library -
#pragma amxlimit 32768
#pragma amxram 4096
#pragma dynamic (1024/4)


/* Float arithmetic
 *
 * (c) Copyright 1999, Artran, Inc.
 * Written by Greg Garner (gmg@artran.com)
 * Modified in March 2001 to include user defined
 * operators for the floating point functions.
 *
 * This file is provided as is (no warranties).
 */
//#pragma library Float
#pragma rational Float

/* Different methods of rounding */
const floatround_method: {
  floatround_round = 0,
  floatround_floor,
  floatround_ceil,
  floatround_tozero,
  floatround_unbiased
}
const anglemode: {
  radian = 0,
  degrees,
  grades
}
native Float:float(value) =-1;
native Float:strfloat(const string[]) =-2;
native Float:floatmul(Float:oper1, Float:oper2) =-3;
native Float:floatdiv(Float:dividend, Float:divisor) =-4;
native Float:floatadd(Float:oper1, Float:oper2) =-5;
native Float:floatsub(Float:oper1, Float:oper2) =-6;
native Float:floatfract(Float:value) =-7;
native       floatround(Float:value, floatround_method:method=floatround_round) =-8;
native       floatcmp(Float:oper1, Float:oper2) =-9;
native Float:floatsqroot(Float:value) =-10;
native Float:floatpower(Float:value, Float:exponent) =-11;
native Float:floatlog(Float:value, Float:base=10.0) =-12;
native Float:floatsin(Float:value, anglemode:mode=radian) =-13;
native Float:floatcos(Float:value, anglemode:mode=radian) =-14;
native Float:floattan(Float:value, anglemode:mode=radian) =-15;
native Float:floatabs(Float:value) =-16;
native       floatint(Float:value) =-17;

stock Float:operator*(Float:oper1, Float:oper2){return floatmul(oper1,oper2);}
stock Float:operator/(Float:oper1, Float:oper2){return floatdiv(oper1,oper2);}
stock Float:operator+(Float:oper1, Float:oper2){return floatadd(oper1,oper2);}
stock Float:operator-(Float:oper1, Float:oper2){return floatsub(oper1,oper2);}
stock Float:operator=(oper){return float(oper);}
stock       operator=(Float:oper){return floatint(oper);}

stock Float:fract(Float:value){return floatfract(value);}
stock       round(Float:value, floatround_method:method=floatround_round){return floatround(value,method);}
stock Float:sqrt(Float:value){return floatsqroot(value);}
stock Float:pow(Float:value, Float:exponent){return floatpower(value,exponent);}
stock Float:log(Float:value, Float:base=10.0){return floatlog(value,base);}
stock Float:sin(Float:value){return floatsin(value,radian);}
stock Float:cos(Float:value){return floatcos(value,radian);}
stock Float:tan(Float:value){return floattan(value,radian);}
stock Float:abs(Float:value){return floatabs(value);}
stock       int(Float:value){return floatint(value);}

stock Float:operator++(Float:oper) return oper+1.0;
stock Float:operator--(Float:oper) return oper-1.0;
stock Float:operator-(Float:oper) return oper^Float:cellmin;                  /* IEEE values are sign/magnitude */
stock Float:operator*(Float:oper1, oper2) return floatmul(oper1, float(oper2));       /* "*" is commutative */
stock Float:operator/(Float:oper1, oper2) return floatdiv(oper1, float(oper2));
stock Float:operator/(oper1, Float:oper2) return floatdiv(float(oper1), oper2);
stock Float:operator+(Float:oper1, oper2) return floatadd(oper1, float(oper2));       /* "+" is commutative */
stock Float:operator-(Float:oper1, oper2) return floatsub(oper1, float(oper2));
stock Float:operator-(oper1, Float:oper2) return floatsub(float(oper1), oper2);
stock bool:operator==(Float:oper1, Float:oper2) return floatcmp(oper1, oper2) == 0;
stock bool:operator==(Float:oper1, oper2) return floatcmp(oper1, float(oper2)) == 0;  /* "==" is commutative */
stock bool:operator!=(Float:oper1, Float:oper2) return floatcmp(oper1, oper2) != 0;
stock bool:operator!=(Float:oper1, oper2) return floatcmp(oper1, float(oper2)) != 0;  /* "!=" is commutative */
stock bool:operator>(Float:oper1, Float:oper2) return floatcmp(oper1, oper2) > 0;
stock bool:operator>(Float:oper1, oper2) return floatcmp(oper1, float(oper2)) > 0;
stock bool:operator>(oper1, Float:oper2) return floatcmp(float(oper1), oper2) > 0;
stock bool:operator>=(Float:oper1, Float:oper2) return floatcmp(oper1, oper2) >= 0;
stock bool:operator>=(Float:oper1, oper2) return floatcmp(oper1, float(oper2)) >= 0;
stock bool:operator>=(oper1, Float:oper2) return floatcmp(float(oper1), oper2) >= 0;
stock bool:operator<(Float:oper1, Float:oper2) return floatcmp(oper1, oper2) < 0;
stock bool:operator<(Float:oper1, oper2) return floatcmp(oper1, float(oper2)) < 0;
stock bool:operator<(oper1, Float:oper2) return floatcmp(float(oper1), oper2) < 0;
stock bool:operator<=(Float:oper1, Float:oper2) return floatcmp(oper1, oper2) <= 0;
stock bool:operator<=(Float:oper1, oper2) return floatcmp(oper1, float(oper2)) <= 0;
stock bool:operator<=(oper1, Float:oper2) return floatcmp(float(oper1), oper2) <= 0;
stock bool:operator!(Float:oper) return (_:oper & cellmax) == 0;
/* forbidden operations */
forward operator%(Float:oper1, Float:oper2);
forward operator%(Float:oper1, oper2);
forward operator%(oper1, Float:oper2);
//==============================================================================
native strlen(const string[]) = -18;
native strpack(dest[], const source[], maxlength=sizeof dest) = -19;
native strunpack(dest[], const source[], maxlength=sizeof dest) = -20;
native strcopy(dest[], const source[], maxlength=sizeof dest) = -21;
native strcat(dest[], const source[], maxlength=sizeof dest) = -22;
native strmid(dest[], const source[], start=0, end=cellmax, maxlength=sizeof dest) = -23;
native bool: strins(string[], const substr[], index, maxlength=sizeof string) = -24;
native bool: strdel(string[], start, end) = -25;
native strcmp(const string1[], const string2[], bool:ignorecase=false, length=cellmax) = -26;
native strfind(const string[], const sub[], bool:ignorecase=false, index=0) = -27;
native strval(const string[], index=0) = -28;
native valstr(dest[], value, bool:pack=true) = -29;
native bool: ispacked(const string[]) = -30;
native strformat(dest[], size=sizeof dest, bool:pack=true, const format[], {Fixed,Float,_}:...) = -31;
native uudecode(dest[], const source[], maxlength=sizeof dest) = -32;
native uuencode(dest[], const source[], numbytes, maxlength=sizeof dest) = -33;
native memcpy(dest[], const source[], index=0, numbytes, maxlength=sizeof dest) = -34;

stock bool: strequal(const string1[], const string2[], bool:ignorecase=false, length=cellmax)
    return strcmp(string1, string2, ignorecase, length) == 0;
//-34
//==============================================================================
native print(const string[]) = -35;
native printf(const format[], {Float,Fixed,_}:...) = -36;
//-36
//==============================================================================
//==============================================================================
native time() = -37
native wait(const ms) = -38
native Float:get_var(const varmsk) = -39
native       set_var(const varmsk, const Float:value, const bool:force_send=false) = -40
//==============================================================================
const serialmode: {
  NODE = 0,
  LAN,
  GCU
}
native       serial_listen(const portID,const publicFunc[]) = -41
native bool: serial_write(const portID, const src{}, cnt, serialmode:mode=NODE) = -42
native       serial_byte(index) = -43
native       serial_string(dest{}, size=sizeof dest) = -44 //packed
//==============================================================================
native Float:get_gpio(const index) = -45
native       set_gpio(const index, const Float:value) = -46
native       set_control(const index, const Float:value) = -47
native Float:get_ch(const index) = -48
//==============================================================================
// Math
native Float:floatatan(Float:value, anglemode:mode=radian) = -49
native Float:floatatan2(Float:y, Float:x) = -50

const Float: R2D =  57.29577951308232          // radians to degrees conversion factor
const Float: D2R =  0.01745329251994           // degrees to radians conversion factor
const Float: PI =   3.14159265358979           // PI

native       time_utc() = -51

//==============================================================================
forward @OnTask();
