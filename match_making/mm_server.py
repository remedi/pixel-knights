#!/usr/bin/env python

import socket
import signal

globalServerList = []

#Poll each server if they are still running. Get the amount of players from the server as response
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
    except socket.error:
      print "Following server is no longer running: %s %s" % (addr, str(port))
      continue
    sock.send("P")
    #Get the number of players as response
    response = sock.recv(1024)
    if not response[0] == 'R':
      print "Server returned unexpected response"
      continue
    server[2] = response[1]
    newlist.append(server)
    sock.close()
  return newlist

#Signal handler. Terminate every map server before quitting
def killServerList(signal, frame):
  print "Received system signal. Will kill all map server, then exit."
  addr = ""
  port = int()
  response = ""
  newlist = []
  for server in globalServerList:
    addr = server[0].split(' ')[0]
    port = int(server[0].split(' ')[1])
    try:
      sock = socket.create_connection((addr, port), 1)
    except socket.error:
      print "Couldn't send KILL order to: %s %s" % (addr, str(port))
      continue
    sock.send("KILL")
    sock.close()
  exit(0)

def createServerList(serverList):
  ser_string = ""
  for server in serverList:
    ser_string = ser_string + server[0] + " Map number: " + server[1] + " Player count: " + str(server[2]) + "\200"
  return ser_string

def getOwnAddr():
  #Connect to google server
  try:
    #IPv4 server:
    ownAddr = socket.create_connection(('8.8.8.8', 53), 5)

    #Uncomment this for creating IPv6 server:
    ownAddr = socket.create_connection(('2001:4860:4860::8888', 53), 5)

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
  global globalServerList
  signal.signal(2, killServerList)
  #Determine if we're IPv4 or IPv6:
  if(my_IP.count('.') > my_IP.count(':')):
    clientSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
  else:
    clientSocket = socket.socket(socket.AF_INET6, socket.SOCK_STREAM)
  #All servers that have announced themselves are added to this list:
  
  #Look for open port:
  while True:
    try:
      clientSocket.bind((my_IP, port))
      break
    except socket.error:
      port += 1

  print "Listening for connection on: ", clientSocket.getsockname()
  #print "Redirection message: ", serverAddr
  
  clientSocket.listen(1)
  
  while 1:
    (client, addr) = clientSocket.accept()
    globalServerList = pollServerList(globalServerList)
    recv = client.recv(1024)
    if recv[0] == 'H':
      print "Client connected from: ", addr
      print "Responding with serverlist of %d servers" % len(globalServerList)
      sendMe = createServerList(globalServerList)
      sendMe = "L%x " % (len(globalServerList)) + sendMe
      client.send(sendMe);
    elif recv[0] == 'S':
      print "Map server connected: ", addr
      client.send("O");
      # Close this connection as soon as possible:
      client.close()
      addrString = addr[0] + " " + str(addr[1])
      globalServerList.append([addrString, chr(ord(recv[1])+48), 0])
      print "Serverlist is now %d long" % len(globalServerList)
    else:
      print "Got unexpected message from client: %s msg: %s" % (addr, recv)

    client.close()

if __name__ == "__main__":
  main()
