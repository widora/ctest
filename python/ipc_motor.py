#!/usr/bin/env python

'This is a client for MOTOR control server'

import socket
import struct
import sys
import time
import threading

#----- global var.-----!!!! but for module caller, they are module's private variables !!!!!
g_IPC_Sock=None
g_msg_dat = [0,0] #--[id,dat]

path_ipc_socket="/tmp/ipc_socket"

#------------------------------------
# create an Local IPC Sock
# return socket
#------------------------------------
def create_IPCSock():
   while True:
	try:
		sock=socket.socket(socket.AF_UNIX,socket.SOCK_STREAM)
		sock.connect(str(path_ipc_socket))
		#------ receive MOTOR server reply -----
		srep=sock.recv(4)
		reply_code,=struct.unpack("i",srep)
		print "reply_code: %d" % reply_code
		if ( reply_code == 1 ):
			print "The server is busy,try later..."
		else:
			print "connect to MOTOR server successfully!"
			return sock

	except socket.error:
		print "Fail to connect to the server, wait for retry..."
		time.sleep(1)
		pass
		

#--------------     thread function   ------------------------
# send msg to MOTOR server
# id=0 : dat is invalid or obselete
# id=1 : set seed
# dat  : speed value -400(fast) - 0(standstill) +400(fast)
#-------------------------------------------------------------
def send_IPCMsg(g_IPC_Sock,g_msg_dat):
   while True:
      try:
	   if(g_msg_dat[0] > 0): #--only if msg_dat is valid
		msg=struct.pack("ii",g_msg_dat[0],g_msg_dat[1])
		ret=g_IPC_Sock.sendall(msg)
		#---- mark dat as obselete
		g_msg_dat[0]=0
		if(ret != None):
			print "socket send fails! ret=%d" % ret 
		else:
			print "msg id=%d dat=%d send out!" % (g_msg_dat[0],g_msg_dat[1])
	   time.sleep(0.1)

      except socket.error, err:
	   print "%s" % err
	   print "IPC Socket server close the connection! try to reconnect..."
	   g_IPC_Sock.close()
	   g_IPC_Sock=create_IPCSock()

      except KeyboardInterrupt:
	   print "sendIPCMsg(): user interrupt to exit!"
	   #sys.exit(-1)
	   #raise 

#--------------------------------
#               test
#--------------------------------
def test():
#	global g_IPC_Sock

	#----- create IPC Sock connection -----
	g_IPC_Sock=create_IPCSock()

	#----- create msg-sending threading -----
	thread_sendmsg=threading.Thread(target=send_IPCMsg,args=(g_IPC_Sock,g_msg_dat))
	thread_sendmsg.setDaemon(True) #--kill thread when main func. exits
	thread_sendmsg.start()
	print "thread_sendmsg starts..."

	#----- loop changing msg_dat ------
	threshold=-400 #(-400 ~ +400)
	bool_stepup=True
	#----- set msg_dat as pwm threshold -----
	try:
	   while True:
		time.sleep(1)
		#--- enable msg dat
		if(bool_stepup):
			threshold += 50
			g_msg_dat[1]=threshold
			g_msg_dat[0]=1
			if( threshold > 400 ):
				threshold = 400
				g_msg_dat[1]=threshold
				g_msg_dat[0]=1
				bool_stepup=False	
		else:
			threshold -= 50
			g_msg_dat[1]=threshold
			g_msg_dat[0]=1
			if(threshold < -400):	
				threshold = -400
				g_msg_dat[1]=threshold
				g_msg_dat[0]=1
				bool_stepup=True

	except KeyboardInterrupt:
		print "test(): user interrupt to exit"
		#sys.exit(-1)
		raise		

	g_IPC_Sock.close()
	thread_sendmsg.join()

if __name__ == '__main__':
	test()
