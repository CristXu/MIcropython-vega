import time,gc
import math
import micropython
from pyb import I2C;
from pyb import LED;
from pyb import SWIM;
from pyb import IMU;
i2c=I2C(3,I2C.MASTER,baudrate=10000); 
imu=IMU(0,0,delta_ms=0.02)
r=LED(LED.RED); g=LED(LED.GREEN); b=LED(LED.BLUE)
UPPER = 60
LOWER = 30
AXIES = 3
fontH = 13
h = fontH + 1
lTitle = ['A=', 'G=', 'M=', 'E=', 'P=', 'T=']

PI = 3.1415926
RadToDeg = 180 / PI
DegToRad = PI / 180

def draw_clock_1(gui, a=0, div = 2, cx=64, cy=64, amp = 1.0, rLen = 9, leaves=12):
	step = 3.14159*2.0/leaves
	for r in range(rLen):
		for t in range(leaves):
			ofs = (float(a)-float(r))/11.0
			rad = (7.0 + float(r) * 6.0) * amp
			x = int(cx + math.cos(t*step + ofs) * rad)
			y = int(cy + math.sin(t*step + ofs) * rad)
			w = r // div
			print(x,y,w)
			gui.put_box(x, y,x+w,y+w)

def OpenSwim(draw = 0, kbps=12000):
	gui = SWIM(0, 0, baudrate=kbps * 1000)
	gui.window_open();
	return gui

def SensorDisplayUpdate(itv=50):
	gui = OpenSwim()
	gui.set_font(gui.FONT6x13)
	gui.set_fill_color(SWIM.WHITE)
	gui.update_display()
	return gui
		
def i2c_conf():
	i2c.mem_write(0,0x1E,0x2A);
	i2c.mem_write(0x1F,0x1E,0x5B);
	i2c.mem_write(0x20,0x1E,0x5C);
	i2c.mem_write(0x01,0x1E,0x0E); 
	i2c.mem_write(0x0d,0x1E,0x2A);

def light_led(xAngle, yAngle, zAngle):
		#- to +/ (0-90)
	if xAngle < 0:
		xAngle = -1 * xAngle
	if yAngle < 0:
		yAngle = -1 * yAngle
	if zAngle < 0:
		zAngle = -1 * zAngle	
	# control the led
	if xAngle > UPPER:
		r.on();
	if yAngle > UPPER:
		g.on();
	if zAngle > UPPER:
		b.on();
	if xAngle < LOWER:
		r.off();
	if yAngle < LOWER:
		g.off();
	if zAngle < LOWER:
		b.off();

def process_data(status, is_acc=True):
	if is_acc:
		r_shift = 2;
	else:
		r_shift = 0;
		# xData yield
	xData = (status[0]<<8 | status[1]) 
	if xData & 0x8000:
		xData = xData - (1<<16)
	xData = xData / (1<<r_shift)
	# yData yield
	yData = (status[2]<<8 | status[3])
	if yData & 0x8000:
		yData = yData - (1<<16)
	yData = yData / (1<<r_shift)
	# zData yield
	zData = (status[4]<<8 | status[5])
	if zData & 0x8000:
		zData = zData - (1<<16)
	zData = zData / (1<<r_shift)     #the acc have only 8/14-bit data to store the value, no need to use all the 16bits. maybe the MSB is enough
	return (xData,yData,zData)

def sensor_fusion(acc, mag):
	g_Ax = acc[0]; g_Ay = acc[1]; g_Az = acc[2];
	g_Mx = mag[0]; g_My = mag[1]; g_Mz = mag[2];
	
	# Calcute roll angle
	g_Roll = math.atan2(g_Ay, g_Az) * RadToDeg;
	sinAngle = math.sin(g_Roll * DegToRad)
	cosAngle = math.cos(g_Roll * DegToRad)
	# De-rotate by roll angle
	By = g_My * cosAngle - g_Mz * sinAngle
	g_Mz = g_Mz * cosAngle + g_My * sinAngle
	g_Az = g_Ay * sinAngle + g_Az * cosAngle

	# Calcute pitch angle
	g_Pitch = math.atan2(-g_Ax, g_Az) * RadToDeg;
	sinAngle = math.sin(g_Pitch * DegToRad)
	cosAngle = math.cos(g_Pitch * DegToRad)
	# De-rotate by pitch angle
	Bx = g_Mx * cosAngle + g_Mz * sinAngle
	
	# Calcute aw angle
	g_Yaw = math.atan2(-By, Bx) * RadToDeg
	
	return [g_Roll, g_Pitch, g_Yaw]
	
i2c_conf()
sensorRange = i2c.mem_read(1,0x1E,0x0e)[0]
dataScale = 0
if sensorRange == 0x00:
	dataScale = 2
if sensorRange == 0x01:
	dataScale = 4
if sensorRange == 0x10:
	dataScale = 8
gui = SensorDisplayUpdate();
cnt = 0;
t1 = time.ticks()
while(True):
	gui.clear_screen()
	# get all the accl status
	status = i2c.mem_read(2*AXIES+1,0x1E,0x00)
	xData, yData, zData = process_data(status[1:],is_acc=True)
	# The Angle group
	xAngle = xData * dataScale * 90 / 8192
	yAngle = yData * dataScale * 90 / 8192
	zAngle = zData * dataScale * 90 / 8192
	light_led(xAngle, yAngle, zAngle)
	t2 = time.ticks()
	dt = t2 - t1
	t1 = t2
	e = imu.step(status[1:], status[1:], None, dt/1000.0)
	print("Eula Angle is [%.4f, %.4f, %.4f]"%(e[0], e[1], e[2]))
	# get all the magnet status and magnet offset
	status = i2c.mem_read(2*AXIES, 0x1E, 0x33)
	xMData, yMData, zMData = process_data(status,is_acc=False)
	status = i2c.mem_read(2*AXIES, 0x1E, 0x3F) 
	xOffset, yOffset, zOffset = process_data(status,is_acc=False)
	# The Magnet value
	xMagnet = xMData - xOffset
	yMagnet = yMData - yOffset
	zMagnet = zMData - zOffset
	# Fusion the Magnet & Acc
	fAngle = sensor_fusion([xData,yData,zData], [xMagnet, yMagnet, zMagnet])
	print("Fusion Angle : [%.4f, %.4f, %.4f]"%(fAngle[0],fAngle[1],fAngle[2]))
	gui.set_font(gui.FONT6x13)
	gui.put_text_xy("xAngle  = %d"%xAngle, 10, 0)
	gui.put_text_xy("yAngle  = %d"%yAngle, 10, 15)
	gui.put_text_xy("zAngle  = %d"%zAngle, 10, 30)
	gui.put_text_xy("xMagnet = %d"%xMagnet, 10, 45)
	gui.put_text_xy("yMagnet = %d"%yMagnet, 10, 60)
	gui.put_text_xy("zMagnet = %d"%zMagnet, 10, 75)
	gui.put_text_xy("fAngle  =", 10, 90)
	gui.put_text_xy("[%.1f,%.1f,%.1f]"%(fAngle[0],fAngle[1],fAngle[2]), 10, 105)
	#gui.set_font(gui.FONT6x13)
	#gui.put_text_xy("VEGA BOARD DEMO", 20,105)
	draw_clock_1(gui, cnt, div=1, cx=105, cy=30, amp = 0.8, rLen = 3, leaves=3)
	gui.update_display()
	print("xAngle is %.5f, yAngle is %.5f, zAngle is %.5f"%(xAngle,yAngle,zAngle))
	time.sleep(10)
	gc.collect()
	cnt += 1;
