import json
from datetime import datetime

def get_config():
    with open('sensor.json', 'r') as f:
        config = json.load(f)
    return config

def save_config(jsonobj):
    with open('sensor.json', 'w') as f:
        json.dump(jsonobj, f)
        
def update_config(name, value):
    config = get_config()
    if name in config:
        config[name].update(value)
    else:
        config[name] = value
    save_config(config)

def logToFile(msg):
    with open("/var/log/sensors.log", "a", buffering=1) as log:
        log.write(json.dumps(msg)+"\n")

def process(msg):
    jmsg = json.loads(msg)
    for key in jmsg.keys():
        jmsg[key]['date'] = datetime.now().strftime('%d.%m.%Y %H:%M:%S')
        update_config(key, jmsg[key])
        logToFile(jmsg[key])