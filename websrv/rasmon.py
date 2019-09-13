import RPi.GPIO as GPIO
from time import sleep, time, strftime

coolerPin = 26
tempLimit = 70.0
coolerOn = False
GPIO.setmode(GPIO.BCM)
GPIO.setup(coolerPin, GPIO.OUT, initial=0)
#включим экран
GPIO.setup(5, GPIO.OUT, initial=0)
last = time()
while True:	
	with open("/sys/class/thermal/thermal_zone0/temp") as tempf:
		cputemp = int(tempf.read()) / 1000.0
	if time() - last > 2:
		with open("/var/log/cpu_temp.csv", "a", buffering=1) as log:
			log.write("{0},{1}\n".format(strftime("%Y-%m-%d %H:%M:%S"),str(cputemp)))
		last = time()
	if cputemp > tempLimit and not coolerOn:
		coolerOn = True
		GPIO.output(coolerPin, coolerOn)
	elif coolerOn and cputemp < tempLimit - 20.0:
		coolerOn = False
		GPIO.output(coolerPin, coolerOn)
	sleep(0.1)
