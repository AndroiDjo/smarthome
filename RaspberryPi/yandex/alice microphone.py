import pyaudio, os, grpc, json, sys, signal
from datetime import datetime
import getiamtoken
from fuzzywuzzy import fuzz
import paho.mqtt.client as mqtt
import snowboydecoder
import yandex.cloud.ai.stt.v2.stt_service_pb2 as stt_service_pb2
import yandex.cloud.ai.stt.v2.stt_service_pb2_grpc as stt_service_pb2_grpc

sys.path.insert(1,'/home/pi/google')
import googletts as tts

__location__ = os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__)))
websrvpath = '/home/pi/websrv'

CHUNK_SIZE = 4000
FORMAT = pyaudio.paInt16
CHANNELS = 1
RATE = 48000
RECORD_SECONDS = 10
break_recognition = False
SUCCESS_COMMAND_THRESHOLD = 65
interrupted = False
hotword_model = '/home/pi/snowboy/Алиса.pmdl'
hotword_sensivity = 0.4

client = mqtt.Client()
p = pyaudio.PyAudio()

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

def executeCmd(command_list):
    result = {'topic': '', 'msg': '', 'prc': 0}
    for command in command_list:
        cmd_str = command
        for x in opts['alias']:
            cmd_str = cmd_str.replace(x, "").strip()
        
        for x in opts['tbr']:
            cmd_str = cmd_str.replace(x, "").strip()

        cmd = recognize_cmd(cmd_str)
        print("Recognize command: " + str(cmd))
        if cmd['prc'] > result['prc']:
            result = cmd
    if result['prc'] >= SUCCESS_COMMAND_THRESHOLD:
        print("Final command: " + str(result))
        client.publish(result['topic'], result['msg'])

def prepareCmd(command_list):
    # если пришла 1 команда - попробуем исполнить сразу несколько команд, разделенных "и"
    if len(command_list) == 1:
        or_list = command_list[0].split(' и ')
        for orval in or_list:
            executeCmd([orval])
    # иначе ищем среди всех команд более подходящую и выполняем только ее
    elif len(command_list) > 1:
        executeCmd(command_list)

def recognize_cmd(command):
    RC = {'topic': '', 'msg': '', 'prc': 0}
    for (topic, v) in jcmds.items():
        print("Topic: " + topic)
        for cmd in v['commands']:
            if 'voicecmd' in cmd:
                for c in cmd['voicecmd']:
                    vrt = fuzz.ratio(command, c)
                    print("command: "+c+" "+str(vrt)+"%")
                    if vrt > RC['prc']:
                        RC['prc'] = vrt
                        RC['topic'] = topic
                        RC['msg'] = cmd['command']
    return RC

def gen(folder_id):
    global break_recognition
    # Задать настройки распознавания.
    specification = stt_service_pb2.RecognitionSpec(
        language_code='ru-RU',
        profanity_filter=True,
        model='general',
        partial_results=True,
        audio_encoding='LINEAR16_PCM',
        sample_rate_hertz=RATE
    )
    streaming_config = stt_service_pb2.RecognitionConfig(specification=specification, folder_id=folder_id)

    # Отправить сообщение с настройками распознавания.
    yield stt_service_pb2.StreamingRecognitionRequest(config=streaming_config)

    stream = p.open(format=FORMAT,
                    channels=CHANNELS,
                    rate=RATE,
                    input=True,
                    frames_per_buffer=CHUNK_SIZE,
                    input_device_index=2)

    print("* recording")

    for i in range(0, int(RATE / CHUNK_SIZE * RECORD_SECONDS)):
        if break_recognition:
            break_recognition = False
            break
        data = stream.read(CHUNK_SIZE)
        yield stt_service_pb2.StreamingRecognitionRequest(audio_content=data)

    stream.stop_stream()
    stream.close()

def run(folder_id, iam_token):
    global break_recognition

    cred = grpc.ssl_channel_credentials()
    channel = grpc.secure_channel('stt.api.cloud.yandex.net:443', cred)
    stub = stt_service_pb2_grpc.SttServiceStub(channel)

    # Отправить данные для распознавания.
    it = stub.StreamingRecognize(gen(folder_id), metadata=(('authorization', 'Bearer %s' % iam_token),))

    # Обработать ответы сервера и вывести результат в консоль.
    try:
        for r in it:
            try:
                print('Start chunk: ')
                command_arr = []
                for alternative in r.chunks[0].alternatives:
                    print('alternative: ', alternative.text)
                    command_arr.append(alternative.text.lower())
                break_recognition = r.chunks[0].final
                if r.chunks[0].final:
                    prepareCmd(command_arr)
                print('Is final: ', r.chunks[0].final)
                print('')
            except LookupError:
                print('Not available chunks')
    except grpc._channel._Rendezvous as err:
        print('Error code %s, message: %s' % (err._state.code, err._state.details))

def recognize(fname):
    print('fname='+fname)
    iamtoken = getiamtoken.get_token()
    with open(os.path.join(__location__, 'private.json'), 'r') as f:
        private = json.load(f)
    run(private['folderid'], iamtoken)
    os.remove(fname)

def hotwordCallback():
    print('listening command ...')

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
    detector.start(detected_callback=hotwordCallback,
                   audio_recorder_callback=recognize,
                   interrupt_check=interrupt_callback,
                   sleep_time=0.03)

    detector.terminate()