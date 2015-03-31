#!/usr/bin/env python

import socket

#Create which contains all the servers and maps they are running. This string is then send to clients.

def pollServerList(serverList):
  addr = ""
  port = int()
  response = ""
  newlist = []
  for server in serverList:
    addr = server[0].split(' ')[0]
    port = int(server[0].split(' ')[1])
    try:
      sock = socket.create_connection((addr, port), 1)
    except:
      continue
    sock.send("P")
    response = sock.recv(1024)
    server[2] = response[1]
    newlist.append(server)
    sock.close()
  return newlist

def createServerList(serverList):
  ser_string = ""
  for server in serverList:
    ser_string = ser_string + server[0] + " Map number: " + server[1] + " Player count: " + str(server[2]) + "\200"
  return ser_string

def getOwnAddr():
  #Connect to google server
  try:
    ownAddr = socket.create_connection(('8.8.8.8', 53), 5)
    #Retrieve own IP
    my_IP = ownAddr.getsockname()[0]
    ownAddr.close()
    print "Retrieved own IP: ", my_IP
  except socket.timeout:
    print "No connection, creating localserver"
    my_IP = 'localhost'
  return my_IP
  
def main():

  my_IP = getOwnAddr()
  port = 3500
  clientSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
  #All servers that have announced themselves are added to this list:
  serverList = []
  
  #Look for open port:
  while True:
    try:
      clientSocket.bind((my_IP, port))
      break
    except socket.error:
      port += 1

  print "Listening for connection on: ", clientSocket.getsockname()
  #print "Redirection message: ", serverAddr
  
  clientSocket.listen(10)
  
  while 1:
  
    (client, addr) = clientSocket.accept()
    serverList = pollServerList(serverList)
    recv = client.recv(1024)
    if recv[0] == 'H':
      print "Client connected from: ", addr
      print "Responding with serverlist of %d servers" % len(serverList)
      sendMe = createServerList(serverList)
      sendMe = "L%x " % (len(serverList)) + sendMe
      #sendMe = "L{:02X} ".format(len(serverList)) + sendMe
      client.send(sendMe);
    elif recv[0] == 'S':
      print "Map server connected: ", addr
      client.send("O");
      # Close this connection as soon as possible:
      client.close()
      addrString = addr[0] + " " + str(addr[1])
      serverList.append([addrString, chr(ord(recv[1])+48), 0])
      print "Added: ", serverList[-1]
      print "Serverlist is now %d long" % len(serverList)
    else:
      print "Got unexpected message from client: %s msg: %s" % (addr, recv)

    client.close()

if __name__ == "__main__":
  main()
