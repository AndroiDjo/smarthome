import schedule
import time
import json
import conf
import datetime
import paho.mqtt.client as mqtt

client = mqtt.Client()

def initMqtt():
    with open('private.json', 'r') as f:
        private = json.load(f)
    client.username_pw_set(private['mqtt_login'], private['mqtt_password'])
    client.connect(private['mqtt_host'], private['mqtt_port'], 60)
    print("schedule mqtt initialized")
    
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
    client.publish(topic, msg)
    conf.update_config(topic, json.loads(msg))

def mqttpostWorkday(topic, msg):
    weekno = datetime.datetime.today().weekday()
    if datetime.datetime.today().weekday() < 5:
        mqttpost(topic, msg)
        
def mqttpostWeekend(topic, msg):
    if datetime.datetime.today().weekday() >= 5:
        mqttpost(topic, msg)

def processTask(name, value):
    addTask(name, value)
    saveTask(name, value)
    
def addTask(tag, schedobj):
    schedule.clear(tag)
    if schedobj['type'] == 'daily':
        schedule.every().day.at(schedobj['time']).do(mqttpost, schedobj['topic'], schedobj['msg']).tag(tag)
    elif schedobj['type'] == 'workday':
        schedule.every().day.at(schedobj['time']).do(mqttpostWorkday, schedobj['topic'], schedobj['msg']).tag(tag)
    elif schedobj['type'] == 'weekend':
        schedule.every().day.at(schedobj['time']).do(mqttpostWeekend, schedobj['topic'], schedobj['msg']).tag(tag)

def init():
    initMqtt()
    loadTasks()
    while True:
        schedule.run_pending()
        time.sleep(1)
        
print("begin of shedule_my")