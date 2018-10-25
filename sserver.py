# TCP Server
import socket
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
host_name = socket.gethostname()
print(host_name)
s.bind((host_name, 60000))
s.listen(10)
conn, addr = s.accept()
while 1:

    data = conn.recv(2048)
    print(data)
    
#    if not data:
#        break
    dataS = raw_input('Enter text for client: ')
    conn.send(dataS)
conn.close()
print("closed")
