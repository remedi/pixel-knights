#!/usr/bin/env python

import socket

#Create which contains all the servers and maps they are running. This string is then send to clients.
def createServerList(serverList):
  ser_string = ""
  for server in serverList:
    ser_string = ser_string + server[0] + " " + server[1]
  return ser_string

def getOwnAddr():
  #We're creating connection to google server, to find out our own IP
  googleAddr = socket.getaddrinfo('8.8.8.8', 53)
  #googleAddr = socket.getaddrinfo('www.google.fi', 80)
  #Choose last tuple from the received list of addresses to get an IPv4 connection
  googleAddr = googleAddr[-1][-1]
  #print "Connecting to google server:", googleAddr
  #Connect to google server
  ownAddr = socket.create_connection(googleAddr)
  #Retrieve own IP
  my_IP = ownAddr.getsockname()[0]
  print "Retrieved own IP: ", my_IP
  ownAddr.close()
  return my_IP
  
def main():

  my_IP = getOwnAddr()
  port = 3500
  #serverAddr = "M4 127.0.0.1 4375"
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

    
  
  print "Waiting for client on port: ", clientSocket.getsockname()
  #print "Redirection message: ", serverAddr
  
  clientSocket.listen(10)
  
  while 1:
  
    (client, addr) = clientSocket.accept()
    recv = client.recv(1024)
    if recv[0] == 'H':
      print "Client connected from: ", addr
      if len(serverList) == 0:
        print "Client connected but serverlist is empty"
        client.send('E')
      else:
        print "Responding with serverlist of %d servers" % len(serverList)
        sendMe = createServerList(serverList)
        client.send(sendMe);
    elif recv[0] == 'S':
      print "Map server connected: ", addr
      client.send("O");
      addrString = addr[0] + " " + str(addr[1])
      serverList.append([addrString, recv[1]])
      print "Serverlist is now %d long" % len(serverList)
    else:
      print "Got unexpected message from client: %s msg: %s" % (addr, recv)
    client.close()

if __name__ == "__main__":
  main()
