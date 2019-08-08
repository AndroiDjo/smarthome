from schedule import Scheduler
import time
import json
import conf
import datetime
import paho.mqtt.client as mqtt
import os

class MqttScheduler:

    schedulePath = 'schedule.json'
    
    def __init__(self):
        self.prevModified = 0
        self.lastModified = 0
        self.mqttclient = mqtt.Client()
        self.initMqtt()
        self.ss = Scheduler()
    
    def loop(self):
        while True:
            self.checkModified()
            self.ss.run_pending()
            time.sleep(1)
        
    def checkModified(self):
        self.lastModified = os.path.getmtime(MqttScheduler.schedulePath)
        if self.lastModified != self.prevModified:
            print('modified')
            print(self.lastModified)
            self.loadTasks()
            self.prevModified = self.lastModified

    def initMqtt(self):
        with open('private.json', 'r') as f:
            private = json.load(f)
        self.mqttclient.username_pw_set(private['mqtt_login'], private['mqtt_password'])
        self.mqttclient.connect(private['mqtt_host'], private['mqtt_port'], 60)
        self.mqttclient.loop_start()
        
    @staticmethod
    def getSchedule():
        with open(MqttScheduler.schedulePath, 'r') as f:
            tasks_json = json.load(f)
        return tasks_json
    
    @staticmethod
    def delTask(tag):
        tasklist = MqttScheduler.getSchedule()
        tasklist.pop(tag, None)
        MqttScheduler.saveSchedule(tasklist)

    @staticmethod
    def saveSchedule(tasks_json):
        with open(MqttScheduler.schedulePath, 'w') as f:
            json.dump(tasks_json, f)
            
    @staticmethod
    def addTask(name, value):
        tasks_json = MqttScheduler.getSchedule()
        tasks_json[name] = value
        MqttScheduler.saveSchedule(tasks_json)

    def loadTasks(self):
        tasklist = MqttScheduler.getSchedule()
        print('tasklist', tasklist)
        self.ss.clear()
        for task in tasklist:
            self.processTask(task, tasklist[task])
            
    def mqttpost(self, topic, msg):
        self.mqttclient.publish(topic, msg)
        conf.update_config(topic, json.loads(msg))

    def mqttpostWorkday(self, topic, msg):
        weekno = datetime.datetime.today().weekday()
        if datetime.datetime.today().weekday() < 5:
            self.mqttpost(topic, msg)
            
    def mqttpostWeekend(self, topic, msg):
        if datetime.datetime.today().weekday() >= 5:
            self.mqttpost(topic, msg)
        
    def processTask(self, tag, schedobj):
        if schedobj['type'] == 'daily':
            self.ss.every().day.at(schedobj['time']).do(self.mqttpost, schedobj['topic'], schedobj['msg'])
        elif schedobj['type'] == 'workday':
            self.ss.every().day.at(schedobj['time']).do(self.mqttpostWorkday, schedobj['topic'], schedobj['msg'])
        elif schedobj['type'] == 'weekend':
            self.ss.every().day.at(schedobj['time']).do(self.mqttpostWeekend, schedobj['topic'], schedobj['msg'])
        elif schedobj['type'] == 'hour':
            self.ss.every().hour.do(self.mqttpost, schedobj['topic'], schedobj['msg'])
        elif schedobj['type'] == 'minute':
            self.ss.every().minute.do(self.mqttpost, schedobj['topic'], schedobj['msg'])
        elif schedobj['type'] == 'second':
            self.ss.every().second.do(self.mqttpost, schedobj['topic'], schedobj['msg'])
            
if __name__ == '__main__':
    mqttscheduler = MqttScheduler()
    mqttscheduler.loop()
