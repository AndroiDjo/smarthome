import pyaudio, os, grpc, json, sys, signal
from datetime import datetime
import getiamtoken
from fuzzywuzzy import fuzz
import paho.mqtt.client as mqtt
import snowboydecoder
import urllib.request
from urllib.error import URLError, HTTPError

sys.path.insert(1,'/home/pi/google')
import googletts as tts

__location__ = os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__)))
websrvpath = '/home/pi/websrv'

SUCCESS_COMMAND_THRESHOLD = 65
interrupted = False
hotword_model = '/home/pi/snowboy/Алиса.pmdl'
hotword_sensivity = 0.5

client = mqtt.Client()

opts = {
    "alias": ('алиса','алис','малина','малинка','умный дом'),
    "tbr": ('скажи','расскажи','покажи','сколько','произнеси','на','пожалуйста','еще','ещё')
}

with open(os.path.join(websrvpath, 'mqttcommands.json'), 'r') as f:
    jcmds = json.load(f)

def signal_handler(signal, frame):
    global interrupted
    interrupted = True

def interrupt_callback():
    global interrupted
    return interrupted

def executeCmd(command):
    result = {'topic': '', 'msg': '', 'prc': 0}
    cmd_str = command
    for x in opts['alias']:
        cmd_str = cmd_str.replace(x, "").strip()
    
    for x in opts['tbr']:
        cmd_str = cmd_str.replace(x, "").strip()

    result = recognize_cmd(cmd_str)

    if result['prc'] >= SUCCESS_COMMAND_THRESHOLD:
        print("Final command: " + str(result))
        client.publish(result['topic'], result['msg'])

def prepareCmd(command_list):
    # попробуем исполнить сразу несколько команд, разделенных "и"
    or_list = command_list.split(' и ')
    for orval in or_list:
        executeCmd(orval)

def recognize_cmd(command):
    RC = {'topic': '', 'msg': '', 'prc': 0}
    for (topic, v) in jcmds.items():
        for cmd in v['commands']:
            if 'voicecmd' in cmd:
                for c in cmd['voicecmd']:
                    vrt = fuzz.ratio(command, c)
                    if vrt > RC['prc']:
                        RC['prc'] = vrt
                        RC['topic'] = topic
                        RC['msg'] = cmd['command']
    return RC

def recognize_short(fname):
    with open(fname, "rb") as f:
        data = f.read()
    
    with open(os.path.join(__location__, 'private.json'), 'r') as f:
        private = json.load(f)

    iamtoken = getiamtoken.get_token()
    
    req_params = "&".join([
        "topic=general",
        "folderId=%s" % private['folderid'],
        "lang=ru-RU",
        "format=lpcm",
        "sampleRateHertz=16000"
    ])

    url = urllib.request.Request("https://stt.api.cloud.yandex.net/speech/v1/stt:recognize?%s" % req_params, data=data)
    url.add_header("Authorization", "Bearer %s" % iamtoken)

    try:
        responseData = urllib.request.urlopen(url).read().decode('UTF-8')
        decodedData = json.loads(responseData)

        if decodedData.get("error_code") is None:
            print(decodedData.get("result"))
            prepareCmd(decodedData['result'])
    except HTTPError as e:
        print('Error code: ', e.code)
    except URLError as e:
        print('Reason: ', e.reason)

    os.remove(fname)

######## mqtt events begin ###############
def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))

def on_message(client, userdata, msg):
    print(msg.payload.decode("utf-8"))

def on_disconnect(client, userdata, rc):
    print("disconnected with result code="+str(rc))
######## mqtt events end ###############

def initMqtt():
    with open(os.path.join(websrvpath, 'private.json'), 'r') as f:
        webprivate = json.load(f)
    client.username_pw_set(webprivate['mqtt_login'], webprivate['mqtt_password'])
    client.on_connect = on_connect
    client.on_message = on_message
    client.on_disconnect = on_disconnect
    client.connect(webprivate['mqtt_host'], webprivate['mqtt_port'], 60)
    client.loop_start()
    print("mqtt initialized")

if __name__ == '__main__':
    initMqtt()
    signal.signal(signal.SIGINT, signal_handler)
    detector = snowboydecoder.HotwordDetector(hotword_model, sensitivity=hotword_sensivity)
    detector.start(audio_recorder_callback=recognize_short,
                   interrupt_check=interrupt_callback,
                   silent_count_threshold=2,
                   sleep_time=0.03)

    detector.terminate()