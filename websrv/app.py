from flask import Flask, render_template, request
import mqtt_init
import threading, json

app = Flask(__name__)

@app.route('/')
def index():
    json_string = json.dumps(get_settings())
    return render_template('index.html', config=json_string)

@app.route('/dev')
def indexdev():
    json_string = json.dumps(get_settings())
    return render_template('indexdev.html', config=json_string)

@app.route('/example')
def example():
    return render_template('example.html')

@app.route('/mqttpub')
def mqttpub():
    req_t = request.args.get('topic')
    req_m = request.args.get('msg')
    mqtt_init.publish(req_t, req_m)
    set_settings(req_t, json.loads(req_m))
    return "OK"

def get_settings():
    with open('config.json', 'r') as f:
        config = json.load(f)
    return config

def set_settings(name, value):
    config = get_settings()
    config[name].update(value)
    with open('config.json', 'w') as f:
        json.dump(config, f)

def mqtt_monitor():
    print("start mqtt_monitor")
    mqtt_init.init()

if __name__ == '__main__':
    mqtt_thread = threading.Thread(target=mqtt_monitor, args=())
    mqtt_thread.daemon = True
    mqtt_thread.start()
    app.run(debug=True, host='0.0.0.0')

