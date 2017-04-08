#!/usr/bin/env python
import time
import socket

HOST=''
PORT=55124
#PORT=55124
BUFSIZE=100
ADDR=(HOST ,PORT)

strDATA=(''*50)

tcpSerSock=socket.socket(socket.AF_INET,socket.SOCK_STREAM)
tcpSerSock.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)
tcpSerSock.bind(ADDR)
tcpSerSock.listen(5)

try:
 while True:
   print 'Waiting for connection from Matlab ... ....' 
   tcpCliSock,addr = tcpSerSock.accept()
   tcpCliSock.send('Welcome! You are connected with WIDORA.\n')
   print '....connected from: ',addr

   while True:
      data = tcpCliSock.recv(30)
      if not data:
          break          # 11111------------ no data to indicate  client exiting
      print 'Data received: ',str(data)    
      
      strTime=str(time.ctime()) 
      strTime=strTime+'\n'  #-------- Matlab needs a \n to end fgets
      tcpCliSock.send(strTime)

      strDATA="11.111 22.222 33.333"
      strDATA=strDATA+'\n'         
      tcpCliSock.send(strDATA)

   tcpCliSock.close()  # 11111---------break to close client socket

 tcpSerSock.close()

except KeyboardInterrupt:
  print 'User exit by keyboard!'
  tcpCliSock.close()
  tcpSerSock.close()


