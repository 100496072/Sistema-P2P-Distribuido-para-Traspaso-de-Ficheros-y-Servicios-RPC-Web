from enum import Enum
import socket
import argparse
import sys
import threading
from sqlite3 import connect
import zeep
wsdl = "http://localhost:8000/?wsdl"

def read_string(sock):
    a = ''
    while True:
        msg = sock.recv(1)
        if (msg == b'\0'):
            break
        a += msg.decode()
    return a

def read_number(sock):
    a = read_string(sock)
    return(int(a,10))

def write_string(sock, str):
    sock.sendall(str.encode() + b'\0')

def write_number(sock, num):
    a = str(num)
    write_string(sock, a)


#Servidor local del cliente
def iniciar_servidor(ServerSocket):
    ServerSocket.listen(5)
    print(f"Escuchando conexiones en el puerto {ServerSocket.getsockname()}...")

    while True:
        conn, addr = ServerSocket.accept()
        print(f"Conexión entrante de {addr}")

        try:
            comando = read_string(conn)
            print(comando)

            #Comprueba si el comando que ha recibido es GET_FILE
            if comando == "GET_FILE":
                
                try:
                    ruta = read_string(conn)
                    print(ruta)

                    #Comprueba si el archivo existe
                    with open(ruta, 'rb') as file:
                        write_number(conn, 0)
                        file_size = str(len(file.read()))
                        write_number(conn, file_size)


                        file.seek(0)
                        data = file.read(1024)

                        #Envia el archivo de 1024 bytes en 1024 bytes
                        while data:
                            conn.sendall(data)
                            data = file.read(1024)

                except FileNotFoundError:
                    write_number(conn, 1)

                except Exception as e:
                    write_number(conn, 2)

            else: 
                write_number(conn, 2)

        finally:
            conn.close()

class client :
    
    # ******************** TYPES *********************
    # *
    # * @brief Return codes for the protocol methods
    class RC(Enum) :
        OK = 0
        ERROR = 1
        USER_ERROR = 2
        

    # ****************** ATTRIBUTES ******************
    _server = None
    _port = -1

    #Variables para controlar usuario conectado y su servidor asociado
    connected_user = None
    register_user = None
    server_socket = None
    # ******************** METHODS *******************


    @staticmethod
    def register(user) :

        #Conexión con el servidor
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server_address = (client._server, client._port)
        sock.connect(server_address)

        #Envio y recibo de los mensajes correspondientes
        try:
            write_string(sock, "REGISTER")
            cliente = zeep.Client(wsdl=wsdl)
            write_string(sock, cliente.service.obtener_fecha_hora())
            write_string(sock, user)

            errcode = read_number(sock)
            if (errcode == 0):
                client.register_user = user
                print("c> REGISTER OK")

            elif (errcode == -1):
                print("c> USERNAME IN USE")

            elif (errcode == -2):
                print("c> UNREGISTER FAILED")

        finally:
            sock.close()

        return client.RC.ERROR



    @staticmethod
    def unregister(user) :
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server_address = (client._server, client._port)

        sock.connect(server_address)

        try:
            write_string(sock, "UNREGISTER")
            cliente = zeep.Client(wsdl=wsdl)
            write_string(sock, cliente.service.obtener_fecha_hora())
            write_string(sock, user)

            errcode = read_number(sock)

            if (errcode == 0):
                print("c> UNREGISTER OK")

                #Desconetados el usuario si es el que estaba conectado
                if user == client.connected_user:
                    client.server_socket.close()
                    client.register_user = None
                    client.connected_user = None
                    client.server_socket = None

            elif (errcode == -1):
                print("c> USER DOES NOT EXIST")

            elif (errcode == -2):
                print("c> UNREGISTER FAILED")
        finally:
            sock.close()

        return client.RC.ERROR

 

    @staticmethod
    def connect(user) :

        if client.connected_user != user and client.connected_user != None:
            client.disconnect(client.connected_user)

        ServerSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        ServerSocket.bind(("localhost",0))
        threadInfo = ServerSocket.getsockname()

        #Creamos el servidor local
        hilo_servidor = threading.Thread(target=iniciar_servidor, args=(ServerSocket,))
        hilo_servidor.daemon = True

        try:

            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            server_address = (client._server, client._port)
            sock.connect(server_address)

            try:

                write_string(sock, "CONNECT")
                cliente = zeep.Client(wsdl=wsdl)
                write_string(sock, cliente.service.obtener_fecha_hora())
                write_string(sock, user)
                write_number(sock, threadInfo[1])
                errcode = read_number(sock)

                if errcode == 0:

                    #Iniciamos el servidor local
                    hilo_servidor.start()
                    client.server_socket = ServerSocket
                    client.connected_user = user
                    print("c> CONNECT OK")

                elif errcode == 1:
                        print("c> CONNECT FAIL, USER DOES NOT EXIST")

                elif errcode == 2:
                    print("c> USER ALREADY CONNECTED")

                else:
                    print("c> CONNECT FAIL")

            finally:
                sock.close()
                

        except Exception as e:
            print("c> CONNECT FAIL")
            print(f"Error: {e}")
        
        return client.RC.ERROR



    @staticmethod
    def disconnect(user) :
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server_address = (client._server, client._port)
        #print('conectando a {} y puerto {}'.format(*server_address))
        sock.connect(server_address)
        

        try:
            if client.connected_user != None:
                threadInfo = client.server_socket.getsockname()[1]
            else:
                threadInfo = None

            write_string(sock, "DISCONNECT")
            cliente = zeep.Client(wsdl=wsdl)
            write_string(sock, cliente.service.obtener_fecha_hora())
            write_string(sock, user)
            write_number(sock, threadInfo)
            errcode = read_number(sock)

            if errcode == 0:
                print("c> DISCONNECT OK")

                #Cerramos el servidor local
                client.server_socket.close()
                client.register_user = None
                client.connected_user = None
                client.server_socket = None

            elif errcode == 1:
                print("c> DISCONNECT FAIL, USER DOES NOT EXIST")

            elif errcode == 2:
                print("c> DISCONNECT FAIL, USER NOT CONNECTED")

            else:
                print("c> DISCONNECT FAIL")
        finally:
            sock.close()

        return client.RC.ERROR



    @staticmethod
    def publish(fileName, description):
        
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server_address = (client._server, client._port)
        #print('conectando a {} y puerto {}'.format(*server_address))
        sock.connect(server_address)

        try:
            if client.connected_user != None:  
                username = client.connected_user
            elif client.register_user != None:
                username = client.register_user
            else:
                username = 'null'

            write_string(sock, "PUBLISH")
            cliente = zeep.Client(wsdl=wsdl)
            write_string(sock, cliente.service.obtener_fecha_hora())
            write_string(sock, username)
            write_string(sock, fileName)
            write_string(sock, description)

            errcode = read_number(sock)

            if errcode == 0:
                print("c> PUBLISH OK")

            elif errcode == 1:
                print("c> PUBLISH FAIL , USER DOES NOT EXIST")

            elif errcode == 2:
                print("c> PUBLISH FAIL , USER NOT CONNECTED")

            elif errcode == 3:
                print("c> PUBLISH FAIL , CONTENT ALREADY PUBLISHED")

            elif errcode == 4:
                print("c> PUBLISH FAIL")

            else:
                print("c> PUBLISH FAIL , UNKNOWN ERROR")

        finally:
            sock.close()

        return client.RC.ERROR



    @staticmethod
    def delete(fileName):
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server_address = (client._server, client._port)
        #print('conectando a {} y puerto {}'.format(*server_address))
        sock.connect(server_address)

        try:
            if client.connected_user != None:
                username = client.connected_user
            elif client.register_user != None:
                username = client.register_user
            else:
                username = 'null'

            write_string(sock, "DELETE")
            cliente = zeep.Client(wsdl=wsdl)
            write_string(sock, cliente.service.obtener_fecha_hora())
            write_string(sock, username)
            write_string(sock, fileName)

            errcode = read_number(sock)

            if errcode == 0:
                print("c> DELETE OK")

            elif errcode == 1:
                print("c> DELETE FAIL , USER DOES NOT EXIST")

            elif errcode == 2:
                print("c> DELETE FAIL , USER NOT CONNECTED")

            elif errcode == 3:
                print("c> DELETE FAIL , CONTENT NOT PUBLISHED")

            elif errcode == 4:
                print("c> DELETE FAIL")

            else:
                print("c> DELETE FAIL , UNKNOWN ERROR")

        finally:
            sock.close()

        return client.RC.ERROR



    @staticmethod
    def list_users() :
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server_address = (client._server, client._port)
        #print('conectando a {} y puerto {}'.format(*server_address))
        sock.connect(server_address)

        try:
            if client.connected_user != None:
                username = client.connected_user
            elif client.register_user != None:
                username = client.register_user
            else:
                username = 'null'

            write_string(sock, "LIST_USERS")
            cliente = zeep.Client(wsdl=wsdl)
            write_string(sock, cliente.service.obtener_fecha_hora())
            write_string(sock, username)

            errcode = read_number(sock)

            if errcode == 0:
                print("c> LIST_USERS OK")
                numero = read_number(sock)

                #Hago tantas lecturas de mensajes como usuarios conectados
                for i in range(numero):
                    name = read_string(sock)
                    IP = read_string(sock)
                    PORT = read_string(sock)
                    print(name, IP, PORT)

                    # Crear el archivo para cada usuario con el nombre 'name'
                    file_name = name
                    with open(file_name, 'w') as file:
                        file.write(f"{IP}\n")
                        file.write(f"{PORT}")

            elif errcode == 1:
                print("c> LIST_USERS FAIL , USER DOES NOT EXIST")

            elif errcode == 2:
                print("c> LIST_USERS FAIL , USER NOT CONNECTED")

            else:
                print("c> LIST_USERS FAIL , UNKNOWN ERROR")

        finally:
            sock.close()

        return client.RC.ERROR


    @staticmethod
    def list_content(user) :
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server_address = (client._server, client._port)
        #print('conectando a {} y puerto {}'.format(*server_address))
        sock.connect(server_address)

        try:
            if client.connected_user != None:
                username = client.connected_user
            elif client.register_user != None:
                username = client.register_user
            else:
                username = 'null'

            write_string(sock, "LIST_CONTENT")
            cliente = zeep.Client(wsdl=wsdl)
            write_string(sock, cliente.service.obtener_fecha_hora())
            write_string(sock, username)
            write_string(sock, user)

            errcode = read_number(sock)

            if errcode == 0:
                print("c> LIST_CONTENT OK")
                numero = read_number(sock)

                #Hago tantas lecturas como mensajes vaya a recibir que lo controla la variable 'numero'
                for i in range(numero):
                    name = read_string(sock)
                    print(name)

            elif errcode == 1:
                print("c> LIST_CONTENT FAIL , USER DOES NOT EXIST")

            elif errcode == 2:
                print("c> LIST_CONTENT FAIL , USER NOT CONNECTED")

            elif errcode == 3:
                print("c> LIST_CONTENT FAIL , REMOTE USER DOES NOT EXIST")

            else:
                print("c> LIST_CONTENT FAIL , UNKNOWN ERROR")

        finally:
            sock.close()

        return client.RC.ERROR


    @staticmethod
    def get_file(user, remote_FileName, local_FileName) :

        # Nos conectamos con el servidor del cliente remoto
        with open(f"{user}", 'r') as file:
            ip = file.readline().strip()  #IP
            port = int(file.readline().strip())  #PORT

        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client_address = (ip, port)

        try:
            sock.connect(client_address)

            write_string(sock, "GET_FILE")
            """cliente = zeep.Client(wsdl=wsdl)
            write_string(sock, cliente.service.obtener_fecha_hora())"""
            write_string(sock, remote_FileName)

            errcode = read_number(sock)
            if errcode == 0:
                print("c > GET_FILE OK")
                tamano = read_number(sock)
                
                # Abrir archivo local para escribir los datos recibidos
                with open(local_FileName, 'wb') as local_file_obj:
                    bytes_received = 0
                    while bytes_received < tamano:
                        data = sock.recv(1024)

                        if not data:
                            break
                        local_file_obj.write(data)
                        bytes_received += len(data)

                    print(f"Archivo '{remote_FileName}' descargado correctamente.")

            elif errcode == 1:
                print("c> GET_FILE FAIL, FILE NOT EXIST")

            else:
                print("c> GET_FILE FAIL, UNKNOWN ERROR")

        finally:
            sock.close()

        return client.RC.ERROR

    # *
    # **
    # * @brief Command interpreter for the client. It calls the protocol functions.
    @staticmethod
    def shell():

        while (True) :
            try :
                command = input("c> ")
                line = command.split(" ")
                if (len(line) > 0):

                    line[0] = line[0].upper()

                    if (line[0]=="REGISTER") :
                        if (len(line) == 2) :
                            client.register(line[1])
                        else :
                            print("Syntax error. Usage: REGISTER <userName>")

                    elif(line[0]=="UNREGISTER") :
                        if (len(line) == 2) :
                            client.unregister(line[1])
                        else :
                            print("Syntax error. Usage: UNREGISTER <userName>")

                    elif(line[0]=="CONNECT") :
                        if (len(line) == 2) :
                            client.connect(line[1])
                        else :
                            print("Syntax error. Usage: CONNECT <userName>")
                    
                    elif(line[0]=="PUBLISH") :
                        if (len(line) >= 3) :
                            #  Remove first two words
                            description = ' '.join(line[2:])
                            client.publish(line[1], description)
                        else :
                            print("Syntax error. Usage: PUBLISH <fileName> <description>")

                    elif(line[0]=="DELETE") :
                        if (len(line) == 2) :
                            client.delete(line[1])
                        else :
                            print("Syntax error. Usage: DELETE <fileName>")

                    elif(line[0]=="LIST_USERS") :
                        if (len(line) == 1) :
                            client.list_users()
                        else :
                            print("Syntax error. Use: LIST_USERS")

                    elif(line[0]=="LIST_CONTENT") :
                        if (len(line) == 2) :
                            client.list_content(line[1])
                        else :
                            print("Syntax error. Usage: LIST_CONTENT <userName>")

                    elif(line[0]=="DISCONNECT") :
                        if (len(line) == 2) :
                            client.disconnect(line[1])
                        else :
                            print("Syntax error. Usage: DISCONNECT <userName>")

                    elif(line[0]=="GET_FILE") :
                        if (len(line) == 4) :
                            client.get_file(line[1], line[2], line[3])
                        else :
                            print("Syntax error. Usage: GET_FILE <userName> <remote_fileName> <local_fileName>")

                    elif(line[0]=="QUIT") :
                        if (len(line) == 1) :
                            if client.connected_user != None:
                                client.disconnect(client.connected_user)
                            break
                        else :
                            print("Syntax error. Use: QUIT")
                    else :
                        print("Error: command " + line[0] + " not valid.")

            except Exception as e:
                print("Exception: " + str(e))

    # *
    # * @brief Prints program usage
    @staticmethod
    def usage() :
        print("Usage: python3 client.py -s <server> -p <port>")

# *
    # * @brief Parses program execution arguments
    @staticmethod
    def  parseArguments(argv) :
        parser = argparse.ArgumentParser()
        parser.add_argument('-s', type=str, required=True, help='Server IP')
        parser.add_argument('-p', type=int, required=True, help='Server Port')
        args = parser.parse_args()

        if (args.s is None):
            parser.error("Usage: python3 client.py -s <server> -p <port>")
            return False

        if ((args.p < 1024) or (args.p > 65535)):
            parser.error("Error: Port must be in the range 1024 <= port <= 65535");
            return False;
        
        client._server = args.s
        client._port = args.p

        return True


    # ******************** MAIN *********************
    @staticmethod
    def main(argv) :
        if (not client.parseArguments(argv)) :
            client.usage()
            return

        #  Write code here
        client.shell()
        print("+++ FINISHED +++")
    

if __name__=="__main__":
    client.main([])