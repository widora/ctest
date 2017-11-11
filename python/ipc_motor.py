#!/usr/bin/env python

'This is a client for MOTOR control server'

import socket
import struct
import sys

#g_msg_dat=int(argv[1])

#---------------------------------
# create an Local IPC Sock
# return socket
#------------------------------------
def create_IPCSock():
	sock=socket.socket(socket.AF_UNIX,socket.SOCK_STREAM)
	sock.connect("/tmp/ipc_socket")
	#------ receive MOTOR server reply -----
	srep=sock.recv(4)
	reply_code,=struct.unpack("i",srep)
	print "reply_code: %d" % reply_code
	if ( reply_code == 1 ):
		print "The server is busy,try later..."
		sock.close()
		return -1
	else:
		print "connect to MOTOR server successfully!"
		return sock


#---------------------------------------------------
# send msg to MOTOR server
# msg_id=1 : set seed
# msg_dat  : speed value 50(fastest) - 400(standstill)
#------------------------------------------------
def send_IPCMsg(sock,msg_id,msg_dat):
	msg=struct.pack("ii",msg_id,msg_dat)
	sock.sendall(msg)

def test():
	sock=create_IPCSock()
	if(sock > 0):
		send_IPCMsg(sock,1,int(sys.argv[1]))
	sock.close()


if __name__ == '__main__':
	test()
