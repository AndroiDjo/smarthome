import sys, os, json
import paho.mqtt.publish as publish

__location__ = os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__)))
with open(os.path.join(__location__, 'private.json'), 'r') as f:
    private = json.load(f)
publish.single(sys.argv[1], sys.argv[2], hostname=private['mqtt_host'], port=private['mqtt_port'], auth={'username':private['mqtt_login'], 'password':private['mqtt_password']})