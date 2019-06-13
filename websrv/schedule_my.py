import schedule
import time
import json
import mqtt_init, conf

def getSchedule():
    with open('schedule.json', 'r') as f:
        tasks_json = json.load(f)
    return tasks_json

def saveSchedule(tasks_json):
    with open('schedule.json', 'w') as f:
        json.dump(tasks_json, f)
    
def saveTask(name, value):
    tasks_json = getSchedule()
    tasks_json[name] = value
    saveSchedule(tasks_json)

def loadTasks():
    tasklist = getSchedule()
    print('tasklist', tasklist)
    for task in tasklist:
        addTask(task, tasklist[task])

def delTask(tag):
    schedule.clear(tag)
    tasklist = getSchedule()
    tasklist.pop(tag, None)
    saveSchedule(tasklist)
        
def mqttpost(topic, msg):
    mqtt_init.publish(topic, msg)
    conf.update_config(topic, json.loads(msg))

def processTask(name, value):
    addTask(name, value)
    saveTask(name, value)
    
def addTask(tag, schedobj):
    schedule.clear(tag)
    if schedobj['type'] == 'daily':
        schedule.every().day.at(schedobj['time']).do(mqttpost, schedobj['topic'], schedobj['msg']).tag(tag)

def init():
    loadTasks()
    while True:
        schedule.run_pending()
        time.sleep(1)
        
print("begin of shedule_my")