#!/usr/bin/env python
# -*- coding: utf-8 -*-
#---------------------------------------------------
# ----- www.BIGIOT.net ------
# turing robot by @feng
# Referring to: https://github.com/bigiot/bigiotRaspberryPi/tree/master/python/turing
# An example of logging on to www.bigiot.net and 
# chatting with www.tuling123.com AI.
# You also need chitchat.py to run this script.	
#
# 2017-4-27  Modified for automatic re-checkin
#                 		         Midas Zhou
#---------------------------------------------------

import socket
import json
import time
from datetime import datetime
import chitchat as cc
import sys

#----- set char. encoding -------
reload (sys)
sys.setdefaultencoding("utf-8")

#定义地址及端口
host = '121.42.180.30'
port = 8181

#设备ID及key
DEVICEID='421'
APIKEY='f80ea043e'

data = b''

#--------- init flags -------
flag_data_received = False
flag_status_checkin = False
flag_during_checkout = False 
flag_during_keeponline = False #--flag to indicate keeponline() action is finished or not!

#-------- init timer ---------
#t=time.time()
t_start_keeponline=time.time()
t_start_keepcheckin=time.time()

#-------- init counter ------
count_try_keeponline = 0 #--- record how many times of trying keeponlien()

#-------- init parameters -----
CHECKIN_TIMEOUT=20
KEEP_ONLINE_TIMEOUT=40
MAX_TRY_KEEPOL_NUMBER=2  #--MAX try-keep-online times

checkin = {"M":"checkin","ID":DEVICEID, "K":APIKEY}
checkout = {"M":"checkout","ID":DEVICEID,"K":APIKEY}
json_checkin = json.dumps(checkin)
json_checkout = json.dumps(checkout)

#--------  init socket ---------
s = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
s.settimeout(0)  #--!!!!!!!! This will let s.recv(1) raise exception when receive nothing !!!!!

def ConnectSocket(host,port):
	global s

 	while True:
		try:
			#print "try s.connect(host,port)..."
			s.connect((host,port)) #--one argument only!
			break
		except Exception, error:
			#print error
			#print('s.connect(host,port) fails,wait for 2 seconds!')
			time.sleep(2)

def Checkin(json_checkin):
	global s,t

	try:
		s.send(json_checkin.encode('utf-8'))
		s.send(b'\n')
		print "Send msg=%s finish" %(json_checkin)

	except Exception,error:
		print error

def Checkout(json_checkout):
	try:
		s.send(json_checkout.encode('utf-8'))
		s.send(b'\n')
		print "Send msg=%s finish" %(json_checkout)
	except Exception,error:
		print error

def keepOnline():
	global KEEP_ONLINE_TIMEOUT
	global t_start_keeponline,t_start_keepcheckin
	global flag_during_keeponline
	global count_try_keeponline
	global flag_status_checkin
	
	if flag_status_checkin == True and time.time()-t_start_keeponline >KEEP_ONLINE_TIMEOUT:
		#----------- check whether last keeponline action is finished -----
		if flag_during_keeponline == True:
			print "Warning: last keeponline actions have not finished yet!"

		#----------- print time --------
		tmp=time.time()
		timeArray = time.localtime(tmp)
		strTime = time.strftime("%Y-%m-%d %H:%M:%S",timeArray)
		print strTime
		
		#------------ try to sustain conncetion with BIGIOT server --------
		count_try_keeponline+=1
		if count_try_keeponline > MAX_TRY_KEEPOL_NUMBER:
			count_try_keeponline = 0
			flag_status_checkin = False
			t_start_keepcheckin=t_start_keeponline  #--- pass start time to keepcheckin() to trigger checkin
			return   

		bsend=s.sendall(b'{\"M\":\"status\"}\n')
			
		print 'count_try_keeponline=%d'%count_try_keeponline
		if bsend == None:
			print('send msg to sustain conncetion... sendall() succeed!')
			flag_during_keeponline = True
		else:
			print('send check stauts to sustain connection... sendall() fail!')
		t_start_keeponline= time.time() #--renew t

def keepCheckin():
	global s
	global CHECKIN_TIMEOUT
	global t_start_keepcheckin
	global flag_status_checkin
	
	#print "-----enter keepChechin()"
	if  flag_status_checkin == False and time.time()-t_start_keepcheckin > CHECKIN_TIMEOUT:
                print "------------Start s.close()"
                s.close()
                s = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
                s.settimeout(0) #--!!!!!!!! This will let s.recv(1) raise exception when receive nothing !!!!!
                print "------------Start connecting to  socket(host,port)..."
                ConnectSocket(host,port)
                print "------------Start checking in to BIGIOT..."
                Checkin(json_checkin) 
                t_start_keepcheckin=time.time()
		
	
def say(s, id, coutent):
	saydata = {"M":"say", "ID":id, "C":coutent }
	json_say = json.dumps(saydata)
	s.send(json_say.encode('utf-8'))
	s.send(b'\n')

def process(msg, s, json_checkin):
	global t
	global flag_during_keeponline
	global count_try_keeponline	
	global flag_status_checkin

	#print "Begin json.loads(msg)"
	json_data = json.loads(msg)
	#print(json_data)
	if flag_during_keeponline == True:
		if json_data['M'] == 'checked':
			print "Keeponline action is confirmed by the server!"
			flag_during_keeponline = False
			count_try_keeponline = 0
	if json_data['M'] == 'checkout':
		flag_during_checkout = False
		print("Checkout succeed! msg=",json_data)
	if json_data['M'] == 'say':
		print("--------------------------------")

		print "接收到的数据：", json_data
		print "平台指令：:  ",json_data['C'].encode()
		#say(s, json_data['ID'], cc.chitchat(json_data['C']))
		chat=cc.chitchat(json_data['C'])
		chat=chat.replace(" ","")
		print "收到的回复:  ",chat.encode()
		#say(s, json_data['ID'], cc.chitchat(json_data['C']))
		say(s, json_data['ID'], chat)
	if json_data['M'] == 'connected':
		s.send(json_checkin.encode('utf-8'))
		s.send(b'\n')
	if json_data['M'] == 'login':
		say(s, json_data['ID'], '你好！我是小冰，请问有什么可以帮你！')
		#say(s, json_data['ID'], 'Welcome! Your public ID is '+json_data['ID'])
        if json_data['M'] == "WELCOME TO BIGIOT":
		print("WELCOME message from BIGIOT received")
	if json_data['M'] == "checkinok":
		flag_status_checkin = True
		#t=time.time()
		print("--------- checkin to BIGIOT succeed! -------")

#--------------------------  MAIN FUNC  -------------------------------
ConnectSocket(host,port)
Checkin(json_checkin)		
while True:
	try:
		#------ try to receive msg ------------
		d=s.recv(1)
		flag_data_received=True
	except:
		flag_data_received=False
		time.sleep(2)
		keepOnline() #--- try to sustain login if connected
		keepCheckin() #---- try to re-checkin if disconnected 		

	if flag_data_received:
		if d!=b'\n': 
			data+=d
		elif d=='\n':
			#do something here...
			msg=str(data) #,encoding='utf-8')
			print "Receive msg=%s"%msg
                        process(msg,s,json_checkin)
			print "Finish processing msg."
			data=b''
