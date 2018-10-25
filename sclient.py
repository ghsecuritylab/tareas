# TCP client
import socket
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
host_name = socket.gethostname()
print(host_name)
s.connect((host_name, 60000))
while 1:

    dataC = raw_input('Enter text for server: ')
    s.send(dataC)

    data = s.recv(2048)
    print(data)

  #  if not data:
   #     break

s.close()
print("closed")
