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
import os 
import sys

#----- set char. encoding -------
reload (sys)
sys.setdefaultencoding("utf-8")

#定义地址及端口
host = '121.42.180.30'
port = 8181

#设备ID及key==================
DEVICEID='551'
APIKEY='ec7472145'

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
s.setblocking(0) #--equivalent to settimeout(0)

def LogErr(error):
        os.environ['error']=str(error) 
	os.system("date >> /home/client_err.txt")
	os.system("echo $error >> /home/client_err.txt")

def ConnectSocket(host,port):
	global s
	ntry=0
 	while True:
		try:
			#print "try s.connect(host,port)..."
			LogErr("start s.close()...")
			s.close()
			LogErr("start s=socket.socket()...")
			s = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
			LogErr("s.settimeout(10)")
			s.settimeout(10)
			LogErr("Start s.connect((host,port))")
			s.connect((host,port)) #--one argument only!
			LogErr("start s.settimeout(0)...")
			s.settimeout(0)#--equivalent to setblocking(0)???
			s.setblocking(0)
			LogErr("starting break...")
			break
		except Exception, error:
			ntry+=1
			LogErr('ntry='+str(ntry))
			LogErr(error)
			LogErr("Err: ConnectSocket(host,port)")
			print 's.connect(host,port) ntry=',ntry
			time.sleep(2)

def Checkin(json_checkin):
	global s,t

	try:
		s.send(json_checkin.encode('utf-8'))
		s.send(b'\n')
		print "Send msg=%s finish" %(json_checkin)

	except Exception,error:
		print error
		LogErr(error)
		LogErr('ERR: Checkin(json_checkin)')

def Checkout(json_checkout):
	try:
		s.send(json_checkout.encode('utf-8'))
		s.send(b'\n')
		print "Send msg=%s finish" %(json_checkout)
	except Exception,error:
		print error
		LogErr(error)
		LogErr("ERR: Checkout(json_checkout)")

def keepOnline():
   global KEEP_ONLINE_TIMEOUT
   global t_start_keeponline,t_start_keepcheckin
   global flag_during_keeponline
   global count_try_keeponline
   global flag_status_checkin

   try:
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

   except Exception,error:
	  print error
	  LogErr(error)
	  LogErr("ERR: keepOnline()") 

def keepCheckin():
	global s
	global CHECKIN_TIMEOUT
	global t_start_keepcheckin
	global flag_status_checkin
	
	#print "-----enter keepChechin()"
	try:
	   if  flag_status_checkin == False and time.time()-t_start_keepcheckin > CHECKIN_TIMEOUT:
                print "------------Start s.close()"
                #s = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
                #s.settimeout(0) #--!!!!!!!! This will let s.recv(1) raise exception when receive nothing !!!!!
                print "------------Start connecting to  socket(host,port)..."
                ConnectSocket(host,port)
                print "------------Start checking in to BIGIOT..."
		LogErr("------------Start checking in to BIGIOT...")
                Checkin(json_checkin)
		LogErr("-----finish Checkin()") 
                t_start_keepcheckin=time.time()
	except Exception,error:
	   print error
	   LogErr(error)
	   LogErr("ERR: keepCheckin()")

def say(s, id, coutent):
	try:
		saydata = {"M":"say", "ID":id, "C":coutent }
		json_say = json.dumps(saydata)
		s.send(json_say.encode('utf-8'))
		s.send(b'\n')

	except Exception,error:
		print error
		LogErr(error)
		LogErr("ERR: say(s, id, coutent)")

def getvoice(words):
	try:
		words=words.replace(" ","") #--remove all blanks
		print "收到的回复:  ",words.encode()
		strCMD='/home/getvoice '+'"'+words+'啊啊'+'"'
		print strCMD.encode()
		os.system(strCMD)

	except Exeption,error:
		print error
		LogErr(error)
		LogErr("ERR: getvoice(words)")

def process(msg, s, json_checkin):
   global t
   global flag_during_keeponline
   global count_try_keeponline	
   global flag_status_checkin

   try:
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
		#-------- speak input words
		getvoice(json_data['C'].encode()) 
		chat=cc.chitchat(json_data['C'])
		print "收到的回复:  ",chat.encode()
		#------- speak tuling replying words
		getvoice(chat)	
		#-------  rely to BIGIOT 	
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

   except Exception,error:
	print error	 
	LogErr(error)
	LogErr("ERR: process(msg, s, json_checkin)")

#--------------------------  MAIN FUNC  -------------------------------
ConnectSocket(host,port)
Checkin(json_checkin)		
while True:
	try:
		#------ try to receive msg ------------
                #print 'start d=s.rec()'
		d=s.recv(1)
		flag_data_received=True
	except Exception,error:
		print error
		#---- recv nothing will trigger an exectpion
		#LogErr(error)
		if d==0:
			print "-----Network broken during s.recv()..."
			LogErr("----Network broken during s.recv()...")
		flag_data_received=False
		time.sleep(2)
		print "start keepOnline()..."
		keepOnline() #--- try to sustain login if connected
		print "start keepcheckin()..."
		keepCheckin() #---- try to re-checkin if disconnected 		

	if flag_data_received:
		if d==0 or d==None:
			print "***** Network broken during s.recv()..."
			LogErr("****Network broken during s.recv()...")
		if d!=b'\n': 
			data+=d
			if len(data)>1000: #--in case recv() trapped in dead loop
			    print "-----Data length=%d bytes!"%len(data)
			    keepOnline()
			    keepCheckin()
		elif d=='\n':
			#do something here...
			print "start str(data)..."
			msg=str(data) #,encoding='utf-8')
			print "Receive msg=%s"%msg
                        process(msg,s,json_checkin)
			print "Finish processing msg."
			data=b''

