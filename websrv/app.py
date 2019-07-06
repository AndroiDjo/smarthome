from flask import Flask, render_template, request, jsonify
import mqtt_init, schedule_my, conf
import threading, json
import schedule, time

app = Flask(__name__)

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
    mqtt_init.publish(req_t, req_m)
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

def mqtt_monitor():
    print("start mqtt_monitor")
    mqtt_init.init()

def shedule_monitor():
    print("start shedule_monitor")
    schedule_my.init()
    
if __name__ == '__main__':
    mqtt_thread = threading.Thread(target=mqtt_monitor, args=())
    mqtt_thread.daemon = True
    mqtt_thread.start()
    
    schedule_thread = threading.Thread(target=shedule_monitor, args=())
    schedule_thread.daemon = True
    schedule_thread.start()
    
    app.run(debug=False, host='0.0.0.0')

