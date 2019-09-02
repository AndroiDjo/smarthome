import json

def get_config():
    with open('config.json', 'r') as f:
        config = json.load(f)
    return config

def save_config(jsonobj):
    with open('config.json', 'w') as f:
        json.dump(jsonobj, f)
        
def update_config(name, value):
    config = get_config()
    try:
        config[name].update(value)
    except:
        config[name] = value
    save_config(config)