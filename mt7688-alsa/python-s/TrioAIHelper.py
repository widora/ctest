import sys
import json
import traceback
import paho.mqtt.client as mqtt
import paho.mqtt.publish as mqttpub
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
#tmp for test
from CommandsFunc import *



# Constants
DEFAULT_QOS = 2
DEFAULT_KEEP_ALIVE = 60
MQTT_BROKER_HOST = 'mq.imio.io'
MQTT_BROKER_PORT = 1883
libsc = cdll.LoadLibrary(os.getcwd() + '/libmain.so')

class SinglePub(object):

	@staticmethod
	def single_publish(topic, message, timeout=DEFAULT_KEEP_ALIVE):
		mqttpub.single(topic, message, qos=DEFAULT_QOS, hostname=MQTT_BROKER_HOST,
					   port=MQTT_BROKER_PORT, keepalive=timeout)
		print("-----------------already send")

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
		entry_msg = self.front_door_text_conv()
		if entry_msg is None:
			return None
		lock_state = entry_msg[0]
		welcome_msg = entry_msg[1]
		#print('lock state: ' + lock_state)
		#print(welcome_msg)
		spch = SpeechHelper(lang=self.__lang, gender=self.__gender)
		stream = spch.text_to_speech(welcome_msg, False)
		return stream

	def chat(self, query):
		params = urlparse.urlencode({'sentence': query.encode('utf-8')})
		try:
			url = self.__chat_url % params
			conn = httplib.HTTPConnection(self.__chat_host)
			#conn = httplib.HTTPSConnection(self.__chat_host)
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
			#conn = httplib.HTTPSConnection(self.__cmd_host)
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
		self.__visit_type = "stranger"
		self.is_postman = False
		
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
			if sys.version.startswith('2'):
				try:
					profile = json.loads(raw_msg).encode("utf-8")
				except:
					profile = json.loads(raw_msg)
			print(profile)
			self.__last_vid = profile["VisitorId".decode("utf-8")]
			self.__visit_type = profile["VisitorType".decode("utf-8")]
			if profile.get("LockRecogDeliveryFlag", None):
				self.is_postman = True
			#self.__visit_type = "host"
			if not self.__session_start:
				self.__trio_ai = TrioAIAgent(profile, lang=self.__lang, rob_gender=self.__gender)
				self.__trio_ai.welcome()
				self.__session_start = True
			return

	def visit_id(self):
		return self.__last_vid

	def visit_type(self):
		return self.__visit_type

	def is_session_started(self):
		return self.__session_start

	def session_end(self):
		self.__session_start = False

	def start(self, blocking=False, profile=None):
		
		self.__cli.connect(host='mq.imio.io', port=1883)
		if blocking:
			self.__cli.loop_forever()
		else:
			self.__cli.loop_start()
		'''
		self.__trio_ai = TrioAIAgent(profile, lang=self.__lang, rob_gender=self.__gender)
		self.__trio_ai.welcome()
		self.__session_start = True
		'''
		

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
	os.system('chmod +x *.wav')
	print ("Start!")
	# tmp_profile = {"UserId": "10001"}
	#my_profile = {"UserId":"10003","VisitorId":"Jim","VisitorType":"host"}
	#my_profile = {"UserId":"10004","VisitorId":"Bob","VisitorType":"family"}
	#postman_profile = {"UserId":"10001","VisitorId":"unknown","VisitorType":"stranger",
					   #"LockRecogDeliveryFlag": True}
	visit = FrontDoorScene(lang='en-GB', rob_gender='male')
	visit.start()
	#visit.start(profile=my_profile)
	#visit.start(profile=postman_profile)
	#print("---------------------1")
	while True:
		if visit.is_session_started():
			'''
			#waiting for push the ring button
			print ("Waiting the button push!")
			#while (0 == libsc.get_button()):
				#sleep(0.1)
			print ("The button push!")
			ring_bell = {'BellRingFlag': True}
			visit.tts(visit.ask_and_answer(ring_bell), True)
			sleep(0.5)
			os.system('chmod +x *.wav')
			#wf = open('temp.wav', 'rb')
			#spt = visit.stt('temp.wav')
			#open_door = {'Text': spt}
			#print (spt)
			# libsc.thread_rec()
			# wf = open('test.wav', 'rb')
			# spt = visit.stt('test.wav')
			# open_door = {'Text': spt}
			open_door = {'Text': 'Yes, I forgot my password'}
			visit.tts(visit.ask_and_answer(open_door), True)
			sleep(0.5)
			unclocked = {"PasswdValidFlag": True}
			visit.tts(visit.ask_and_answer(unclocked), True)
			'''
			print (visit.visit_type())
			if visit.visit_type() == "host":
				tmp_ = "hosttest"
				print("-----------------------")
			elif visit.visit_type() == "family":
				tmp_ = "familytest"
			else:
				pass#if visit.is_postman:
			tmp_ = "strangertest"
				#else:
					#tmp_ = "stranger_newtest"
					#tmp_ = "strangertest"
			func = ALL_FUNCS.get(tmp_, None)
			if func:
				print(func.__name__)
				func(visit, True)
			visit.session_end()
			print("session end------------------------")


if __name__ == '__main__':
	main()
