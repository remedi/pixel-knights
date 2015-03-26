#!/usr/bin/env python

import socket

port = 3500

serverAddr = "M4 127.0.0.1 4375"

clientSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

#Look for open port:
while True:
  try:
    clientSocket.bind(('localhost', port))
    break
  except:
    port += 1
  

print "Waiting for client on port: ", clientSocket.getsockname()
print "Redirection message: ", serverAddr

clientSocket.listen(1)

while 1:

  (client, addr) = clientSocket.accept()
  print "Received connection from addr: %s port: %s" % (addr[0], addr[1])
  client.send(serverAddr);
  recv = client.recv(1024)
  print "Got return from client: ", recv

