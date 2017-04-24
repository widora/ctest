import sys
import json
import traceback
import paho.mqtt.client as mqtt
from time import sleep
from ctypes import *
import os
import time

try:
    import http.client as httplib
    import urllib.parse as urlparse
except:
    import httplib
    import urllib as urlparse

from SpeechHelper import SpeechHelper


class TrioAIAgent(object):
    def __init__(self, profile, lang='en-US', rob_gender='female'):
        self.__profile = profile
        self.__lang = lang
        self.__gender = rob_gender
        self.__uid = profile['UserId']
        self.__vid = profile['VisitorId']
        self.__ownership = profile['VisitorType']
        self.__chat_host = 'sandbox.sanjiaoshou.net'
        self.__chat_url = 'http://sandbox.sanjiaoshou.net/Api/chat?who=demo&userID=imio_111&%s'
        self.__cmd_lock_url = 'http://service.sanjiaoshou.net/Locker/index.php?f=locker'
        self.__cmd_host = 'service.sanjiaoshou.net'
        self.__headers = {'Content-Type': 'application/json',}
        self.__session_start = False
        return

    def set_userid(self, uid):
        self.__uid = uid

    def set_visitor_id(self, vid):
        self.__vid = vid

    def set_ownership(self, ownership):
        self.__ownership = ownership

    def welcome(self):
        entry_msg = self.front_door_text_conv(play=True)
        if entry_msg is None:
            return None
        lock_state = entry_msg[0]
        welcome_msg = entry_msg[1]
        #print('lock state: ' + lock_state)
        #print(welcome_msg)
        spch = SpeechHelper(lang=self.__lang, gender=self.__gender)
        stream = spch.text_to_speech(welcome_msg)
        return stream

    def chat(self, query):
        params = urlparse.urlencode({'sentence': query.encode('utf-8')})
        try:
            url = self.__chat_url % params
            conn = httplib.HTTPConnection(self.__chat_host)
            conn.request("GET", url)
            response = conn.getresponse().read()
            if response is None or len(response) == 0:
                return None
            data = json.loads(response.decode('utf-8'))
            print(data)
            conn.close()
            return data['reply']
        except Exception as e:
            print(e)
            return None

    def front_door_text_conv(self, query=None, play=False):
        params = self.__profile
        if query is not None:
            params.update(query)
        body = json.dumps(params)
        try:
            url = self.__cmd_lock_url
            conn = httplib.HTTPConnection(self.__cmd_host)
            conn.request(method="POST", url=url, body=body, headers=self.__headers)
            response = conn.getresponse().read()
            print(response)
            if response is None or len(response) == 0:
                return None
            data = json.loads(response.decode('utf-8'))
            conn.close()
            spch = SpeechHelper(lang=self.__lang, gender=self.__gender)
            spch.text_to_speech(text_in=data['reply'], play=play)
            return data['lockStatus'], data['reply']
        except Exception as e:
            traceback.print_exc()
            print(e)
            return None


class FrontDoorScene(object):
    def __init__(self, lang='en-US', rob_gender='female'):
        self.__QoS = 2
        self.__lang = lang
        self.__gender = rob_gender
        self.__TOPIC = 'visitor_profile'
        self.__cli = mqtt.Client()
        self.__cli.on_connect = self.__connect_cb
        self.__cli.on_message = self.__message_cb
        self.__session_start = False

    def __connect_cb(self, client, userdata, flags, rc):
        self.__cli.subscribe(self.__TOPIC, self.__QoS)
        return

    def __message_cb(self, client, data, msg):
        if msg.topic == self.__TOPIC:
            print('Incoming message!')
            payload = msg.payload
            print(type(payload))
            raw_msg = payload.decode('utf-8')
            print(type(raw_msg))
            print(raw_msg)
            profile = json.loads(raw_msg)
            print(profile)
            if not self.__session_start:
                self.__trio_ai = TrioAIAgent(profile, lang=self.__lang, rob_gender=self.__gender)
                self.__trio_ai.welcome()
                self.__session_start = True
            return

    def is_session_started(self):
        return self.__session_start

    def session_end(self):
        self.__session_start = False

    def start(self, blocking=False, profile=None):
        '''
        self.__cli.connect(host='mq.imio.io', port=1883)
        if blocking:
            self.__cli.loop_forever()
        else:
            self.__cli.loop_start()
        '''
        self.__trio_ai = TrioAIAgent(profile, lang=self.__lang, rob_gender=self.__gender)
        self.__trio_ai.welcome()
        self.__session_start = True
        
        

    def stt(self, wf):
        spch = SpeechHelper(lang=self.__lang, gender=self.__gender)
        return spch.speech_to_text(wf)

    def tts(self, text, play=False):
        spch = SpeechHelper(lang=self.__lang, gender=self.__gender)
        return spch.text_to_speech(text_in=text, play=play)

    def ask_and_answer(self, query):
        ans = self.__trio_ai.front_door_text_conv(query=query)
        if ans is not None:
            reply = ans[1]
            print(reply)
            return reply
        else:
            return None


def main():
    #libsc = cdll.LoadLibrary(os.getcwd() + '/libmain.so')

    print ("Start!")
    my_profile = {"UserId":"10003","VisitorId":"Jim","VisitorType":"host"}
    visit = FrontDoorScene(lang='en-GB', rob_gender='male')
    #visit.start()
    visit.start(profile=my_profile)
    sleep(0.5)
    while True:
        if visit.is_session_started():
            #waiting for push the ring button
            print ("Waiting the button push!")
            # while (0 == libsc.get_button()):
                # sleep(0.1)
            print ("The button push!")
            ring_bell = {'BellRingFlag': True}
            visit.tts(visit.ask_and_answer(ring_bell), True)
            sleep(0.5)
            # os.system('chmod +x *.wav')
            # wf = open('temp.wav', 'rb')
            # spt = visit.stt('temp.wav')
            # open_door = {'Text': spt}
            print (spt)
            # libsc.thread_rec()
            # wf = open('test.wav', 'rb')
            # spt = visit.stt('test.wav')
            # open_door = {'Text': spt}
            open_door = {'Text': 'Yes, I forgot my password'}
            visit.tts(visit.ask_and_answer(open_door), True)
            sleep(0.5)
            unclocked = {"PasswdValidFlag": True}
            visit.tts(visit.ask_and_answer(unclocked), True)
            visit.session_end()


if __name__ == '__main__':
    main()
