import os
import pyb
import time
from pyb import LCD
from pyb import LED
from pyb import Switch
state_r=Switch(1)
state_g=Switch(2)
state_b=Switch(3)

stop = Switch(4)
r = LED(LED.RED)
g = LED(LED.GREEN)
b = LED(LED.BLUE)

lcd=LCD()
sd=pyb.SD
v=os.VfsFat(sd)
a=v.open("image/off.bin",'rb')
off=a.read()
a.close()
a=v.open("image/r_on.bin",'rb')
r_on=a.read()
a.close()
a=v.open("image/g_on.bin",'rb')
g_on=a.read()
a.close()
a=v.open("image/b_on.bin",'rb')
b_on=a.read()
a.close()

sLed_r = 0x0;
sLed_g = 0x0;
sLed_b = 0x0;

r_x = 12;
b_x = r_x + 66 + 10;
g_x = b_x + 66 + 10;

y = (320 - 66)//2
while(True):
	if(stop()):
		break
		
	if(state_r()):
		r.toggle()
		sLed_r = ~sLed_r;
	if(state_g()):
		g.toggle()
		sLed_g = ~sLed_g;
	if(state_b()):
		b.toggle()
		sLed_b = ~sLed_b;
		
	if (sLed_r):
		lcd.put_img(r_on,66,66,r_x,y)
	else:
		lcd.put_img(off,66,66,r_x,y)
	if (sLed_g):
		lcd.put_img(g_on,66,66,g_x,y)
	else:
		lcd.put_img(off,66,66,g_x,y)
	if (sLed_b):
		lcd.put_img(b_on,66,66,b_x,y)
	else:
		lcd.put_img(off,66,66,b_x,y)
	time.sleep(20)