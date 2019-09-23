import time, os, json
import paho.mqtt.client as mqtt
from seclock import SmartClock
from sevideo import SmartEye

client = mqtt.Client()
videoStartCommand = 'beginmotion'
videoEndCommand = 'endmotion'
se = None
sehost = None

def processMsg(msg):
    global se
    global sehost
    jmsg = json.loads(msg)
    if videoStartCommand in jmsg.keys():
        se = SmartEye(sehost, 480, 320)
        se.start()
    elif videoEndCommand in jmsg.keys():
        se.loop_active = False
        se.join()
        se.videoStop()

######## mqtt events begin ###############
def on_connect(client, userdata, flags, rc):
    client.subscribe("video/lobby")

def on_message(client, userdata, msg):
    processMsg(msg.payload.decode("utf-8"))

def on_disconnect(client, userdata, rc):
    print("disconnected with result code="+str(rc))
######## mqtt events end ###############

if __name__ == '__main__':
    sc = SmartClock('black', 'white', 85)
    sc.start()

    __location__ = os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__)))
    with open(os.path.join(__location__, 'private.json'), 'r') as f:
        private = json.load(f)
    sehost = private['sehost']
    client.username_pw_set(private['mqtt_login'], private['mqtt_password'])
    client.on_connect = on_connect
    client.on_message = on_message
    client.on_disconnect = on_disconnect
    client.connect(private['mqtt_host'], private['mqtt_port'], 60)
    client.loop_forever()
