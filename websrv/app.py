from flask import Flask, render_template, request, jsonify
import schedule_my, conf
import threading, json
import schedule, time
import paho.mqtt.client as mqtt

app = Flask(__name__)
client = mqtt.Client()

######## web requests begin ###############
@app.route('/')
def index():
    json_string = json.dumps(conf.get_config())
    schedule_string = json.dumps(schedule_my.getSchedule())
    return render_template('index.html', config=json_string, schedule=schedule_string)

@app.route('/dev')
def indexdev():
    json_string = json.dumps(conf.get_config())
    schedule_string = json.dumps(schedule_my.getSchedule())
    return render_template('indexdev.html', config=json_string, schedule=schedule_string)

@app.route('/example')
def example():
    return render_template('example.html')

@app.route('/mqttpub')
def mqttpub():
    req_t = request.args.get('topic')
    req_m = request.args.get('msg')
    client.publish(req_t, req_m)
    conf.update_config(req_t, json.loads(req_m))
    return jsonify({"response":"OK"})

@app.route('/addtask')
def addtask():
    req_tag = request.args.get('tag')
    req_msg = request.args.get('msg')
    schedule_my.processTask(req_tag, json.loads(req_msg))
    return jsonify({"response":"OK"})

@app.route('/deltask')
def deltask():
    req_tag = request.args.get('tag')
    schedule_my.delTask(req_tag)
    return jsonify({"response":"OK"})
######## web requests end ###############

######## mqtt events begin ###############
def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))
    client.subscribe("sensor/resp")

def on_message(client, userdata, msg):
    print(msg.topic+" "+str(msg.payload))

def on_disconnect(client, userdata, rc):
    print("disconnected with result code="+str(rc))
######## mqtt events end ###############

def initMqtt():
    with open('private.json', 'r') as f:
        private = json.load(f)
    client.username_pw_set(private['mqtt_login'], private['mqtt_password'])
    client.on_connect = on_connect
    client.on_message = on_message
    client.on_disconnect = on_disconnect
    client.connect(private['mqtt_host'], private['mqtt_port'], 60)
    client.loop_start()
    print("mqtt initialized")

def shedule_monitor():
    print("start shedule_monitor")
    schedule_my.init()
    
if __name__ == '__main__':
    initMqtt()    
    schedule_thread = threading.Thread(target=shedule_monitor, args=())
    schedule_thread.daemon = True
    schedule_thread.start()
    
    app.run(debug=False, host='0.0.0.0')

