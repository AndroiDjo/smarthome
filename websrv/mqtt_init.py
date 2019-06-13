import paho.mqtt.client as mqtt
import json
# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))

# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    print(msg.topic+" "+str(msg.payload))

def on_disconnect(client, userdata, rc):
    print("disconnected with result code="+str(rc))

def publish(topic, msg):
    client.publish(topic, msg)

def init():
    with open('private.json', 'r') as f:
        private = json.load(f)
    client.username_pw_set(private['mqtt_login'], private['mqtt_password'])
    client.on_connect = on_connect
    client.on_message = on_message
    client.on_disconnect = on_disconnect
    client.connect(private['mqtt_host'], private['mqtt_port'], 60)
    client.loop_forever()

print("begin of mqtt_init")
client = mqtt.Client()
