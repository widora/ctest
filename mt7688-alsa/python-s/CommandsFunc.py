# -*- coding: utf-8 -*-
import os
from time import sleep
from ctypes import *
#from dingdingApi import *
from SpeechHelper import SpeechHelper

ALL_FUNCS_NAME = ["host_scene1", "host_scene2", "host_scene3", "host_scene4", "host_scene5",
				  "test_1", "test_2", "test_3", "test_4"]
ALL_FUNCS = {}



#------------------interface
def command(scene_name, vistor_type="host"):
	def deco_(func):
		def wraps(*args, **kwargs):
			print(func.__name__)
			ALL_FUNCS[vistor_type + scene_name] = func
		return wraps
	return deco_

	
	
def cmd(visit, is_play=True, key="", value=True):
	tmp_cmd = {key: value}
	print(tmp_cmd)
	visit.tts(visit.ask_and_answer(tmp_cmd), is_play)
	sleep(0.5)
	
	
	
def play_media(visit, is_play=True, text=None, filename="test.wav"):
	if text:
		print({"Text": text})
		visit.tts(visit.ask_and_answer({"Text": text}), is_play)
		return
	spt = visit.stt(filename)
	visit.tts(visit.ask_and_answer({"Text": spt}), is_play)


def bell_ring(visit, is_play=True, flag=True, cmd="BellRingFlag"):#libsc is not in this environment.so ignore it
	ring_bell = {cmd: is_play}
	print(ring_bell)
	# waiting for push the ring button
	print ("Waiting the button push!")
	if flag:
		from TrioAIHelper import libsc
		while (0 == libsc.get_button()):
			sleep(0.1)
	print ("The button push!")
	visit.tts(visit.ask_and_answer(ring_bell), is_play)
	sleep(0.5)

def recog_postman(visit, is_play=True, cmd="LockRecogDeliveryFlag"):
	recog = {cmd: is_play}
	visit.tts(visit.ask_and_answer(recog), is_play)
	sleep(0.5)

def passwd_valid(visit, is_play=True, cmd="PasswdValidFlag"):
	unclocked = {cmd: is_play}
	print(unclocked)
	visit.tts(visit.ask_and_answer(unclocked), is_play)


def host_at_home(visit, is_play=True, cmd="HostAtHomeFlag"):
	host_at_home = {cmd: is_play}
	print(host_at_home)
	visit.tts(visit.ask_and_answer(host_at_home), is_play)

def host_say(visit, is_play=True, cmd="SAY_NO_HOME"):
	host_say = {cmd: is_play}
	print(host_say)
	visit.tts(visit.ask_and_answer(host_say), is_play)

def host_recog_delivery(visit, is_play=True, cmd="HostRecogDeliveryFlag"):
	print("---------------------1")
	host_recog = {cmd: is_play}
	print(host_recog)
	visit.tts(visit.ask_and_answer(host_recog), is_play)
	

	
	
	
	
@command("scene1", "host")
def host_scene1(visit, is_play=True):
	passwd_valid(visit, is_play=is_play)


@command("scene2", "host")
def host_scene2(visit, is_play=True):
	bell_ring(visit, is_play=is_play)
	play_media(visit)

@command("scene3", "host")
def host_scene3(visit, is_play=True):
	bell_ring(visit, is_play=is_play)
	tmp_text = "Yes, I forgot my password"
	print (spt)
	play_media(visit, text=tmp_text)
	passwd_valid(visit, is_play=is_play)

@command("scene4", "host")
def host_scene4(visit, is_play=True):
	bell_ring(visit, is_play=is_play)
	tmp_text = "Yes, I forgot my password"
	play_media(visit, text=tmp_text)
	tmp_text = "I forgot my phone"
	play_media(visit, text=tmp_text)

@command("scene5", "host")
def host_scene5(visit, is_play=True):
	bell_ring(visit, is_play=is_play)
	tmp_text = "i just try the bell"
	play_media(visit, text=tmp_text)

@command("test", "host")
def test_1(visit, is_play=True):
	#tmp_text = "Welcome back home! "
	from TrioAIHelper import libsc
	libsc.display_oled_string()
	bell_ring(visit, is_play=is_play)
	print ("record and play: ")
	libsc.thread_rec()
	# os.system("chmod +x ./test.wav")
	# wf = open('test.wav', 'rb')
	# spt = visit.stt('test.wav')
	# print ("spt = " + spt)
	# tmp_text = spt
	tmp_text = "Yes, I forgot my password"
	play_media(visit, text=tmp_text,is_play=False)
	SpeechHelper(lang='en-GB', gender='male').text_to_speech("The password is 1 2 3 4 5 6", True)
	passwd_valid(visit, is_play=is_play)

@command("test", "family")
def test_2(visit, is_play=True):
	print ("record and play: ")
	from TrioAIHelper import libsc, SinglePub
	topic = "incoming_visiting"
	msg = "Sheldon is here"
	SinglePub.single_publish(topic, message=msg)
	libsc.thread_rec()
	# os.system("chmod +x ./test.wav")
	# wf = open('test.wav', 'rb')
	# spt = visit.stt('test.wav')
	# print ("spt = " + spt)
	# tmp_text = spt
	tmp_text = "i come to send something"
	play_media(visit, text=tmp_text)
	cmd(visit, is_play=is_play, key="HostAtHomeFlag", value=True)
	cmd(visit, is_play=is_play, key="HostSay", value="OPEN_MYSELF")

@command("test", "stranger")
def test_3(visit, is_play=True, topic="mt7687expressbox", msg="open the expressbox"):
	SpeechHelper(lang='en-GB', gender='male').text_to_speech("Hi postman, nice to see you, pickup or delivery?", True)
	# recog_postman(visit, is_play=is_play)
	#cmd(visit, is_play=is_play, key="LockRecogDeliveryFlag", value=True)
	#print ("record and play: ") #I am here to send a package
	from TrioAIHelper import libsc, SinglePub
	topic = "incoming_visiting"
	msg = "Delivery boy is here"
	SinglePub.single_publish(topic, message=msg)
	libsc.thread_rec()
	# os.system("chmod +x ./test.wav")
	# wf = open('test.wav', 'rb')
	# spt = visit.stt('test.wav')
	# print ("spt = " + spt)
	# tmp_text = spt
	#tmp_text = "i am a postman"
	#play_media(visit, text=tmp_text)
	#SpeechHelper(lang='en-GB', gender='male').text_to_speech("pickup or delivery?", True)
	#todo
	#from TrioAIHelper import libsc
	#libsc.thread_rec()
	# os.system("chmod +x ./test.wav")
	# wf = open('test.wav', 'rb')
	# spt = visit.stt('test.wav')
	# print ("spt = " + spt)
	# tmp_text = spt
	tmp_text = "im here to send a package"
	#play_media(visit, text=tmp_text)
	SpeechHelper(lang='en-GB', gender='male').text_to_speech("mailbox will be opened soon", True)
	#for test
	test = True
	from TrioAIHelper import SinglePub
	while True:
		SinglePub.single_publish(topic, message=msg)
		if test:
			break

@command("test", "stranger_new")
def test_4(visit, is_play=True, topic="mt7687expressbox", msg="open the expressbox"):
	cmd(visit, is_play=is_play, key="NoResponseFlag", value=True)
	from TrioAIHelper import libsc, SinglePub
	topic = "incoming_visiting"
	msg = "stranger is here"
	SinglePub.single_publish(topic, message=msg)
	libsc.thread_rec()
	# os.system("chmod +x ./test.wav")
	# wf = open('test.wav', 'rb')
	# spt = visit.stt('test.wav')
	# print ("spt = " + spt)
	# tmp_text = spt
	tmp_text = "My name is Tom."
	play_media(visit, text=tmp_text)

def init_all_funcs():
	for func_name in ALL_FUNCS_NAME:
		print(func_name)
		exec(func_name + "()")


init_all_funcs()
