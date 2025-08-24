//
// Created by linuxr on 21/03/25.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <dirent.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include "users.h"
#include "lines.h"
#include "print_rpc.h"



#define MAX_PETICIONES 256
#define MAX_THREADS 10

int buffer_peticiones[MAX_PETICIONES];
int n_elementos;
int pos_servicio = 0;

pthread_mutex_t mutex;
pthread_cond_t no_lleno;
pthread_cond_t no_vacio;
pthread_t thid[MAX_THREADS];
pthread_attr_t t_attr;

pthread_mutex_t mfin;

int busy = TRUE;
int fin = FALSE;
int sd;

int get_sock_info(char **IP){
    *IP = getenv("LOG_RPC_IP");
    if (*IP == NULL){
        printf("Error al obtener la variable de entorno LOG_RPC_IP\n");
        return -1;
    }
    return 0;
}

void trat_pet(void *arg) {
    char *client_ip = (char *)arg;  // Recibir IP del cliente como argumento
    int s_local;

    CLIENT *clnt;
	enum clnt_stat retval_1;
	int result_1;
	char *print_operation_1_username;
	char *print_operation_1_operation;
	char *print_operation_1_date;
	char *print_operation_1_hour;
    print_operation_1_username = (char *)malloc(255);
    print_operation_1_operation = (char *)malloc(255);
    print_operation_1_date = (char *)malloc(255);
    print_operation_1_hour = (char *)malloc(255);
    char *IP ;

    // El hilo obtiene el socket de la cola
    pthread_mutex_lock(&mutex);
    while (n_elementos == 0){
        if (fin){
            pthread_mutex_unlock(&mutex);
            pthread_exit(NULL);
        }
        pthread_cond_wait(&no_vacio, &mutex);
    }
    s_local = buffer_peticiones[pos_servicio];
    pos_servicio = (pos_servicio + 1) % MAX_PETICIONES;
    n_elementos--;
    pthread_cond_signal(&no_lleno);
    pthread_mutex_unlock(&mutex);

    int errcode = 0;
    char buffer[1000], res[1000];
    char username[255], remoteusername[255], filename[255], filedescr[255], port[255], operacion[255], fechahora[255];
    memset(res, 0, 1000);

    // Leer el mensaje del cliente
    readLine(s_local, buffer, sizeof(buffer));
    strcpy(operacion, buffer);

    strcpy(print_operation_1_operation, operacion);

    

    readLine(s_local, buffer, sizeof(buffer));
    strcpy(fechahora, buffer);
    char hora[16], fecha[16];  // Tamaño suficiente para ambas cadenas

    readLine(s_local, buffer, sizeof(buffer));
    strcpy(username, buffer);
    strcpy(print_operation_1_username, username);
    char *token = strtok(fechahora, " ");
    if (token != NULL) {
        strcpy(print_operation_1_date, token);
        token = strtok(NULL, " ");
        if (token != NULL) {
            strcpy(print_operation_1_hour, token);
        }
    }

    // ****
    //FUNCIONALIDADES PRINCIPALES
    // ****

    // Primero se recibirán los mensajes correspondiente de los clientes
    // Segundo se enviarań las respuestas necesarias
    if (strcmp(operacion, "REGISTER")==0) {
        errcode = register_user(username);
        sprintf(res, "%d", errcode);

        sendMessage(s_local, res, strlen(res) + 1);
        printf("s> OPERATION %s FROM %s\n", operacion, username);
        printf("s>\n");
    }

    else if (strcmp(operacion, "UNREGISTER")==0) {
        errcode = unregister_user(username);
        sprintf(res, "%d", errcode);

        sendMessage(s_local, res, strlen(res) + 1);
        printf("s> OPERATION %s FROM %s\n", operacion, username);
        printf("s>\n");
    }



    else if (strcmp(operacion, "CONNECT") == 0) {
        readLine(s_local, buffer, sizeof(buffer));
        strcpy(port, buffer);
        errcode = connect_user(username, client_ip, port);
        sprintf(res, "%d", errcode);

        sendMessage(s_local, res, strlen(res) + 1);
        printf("s> OPERATION %s FROM %s\n", operacion, username);
        printf("s>\n");
    }


    else if (strcmp(operacion, "DISCONNECT")==0) {
        errcode = disconnect_user(username);
        sprintf(res, "%d", errcode);

        sendMessage(s_local, res, strlen(res) + 1);
        printf("s> OPERATION %s FROM %s\n", operacion, username);
        printf("s>\n");
    }



    else if (strcmp(operacion, "PUBLISH") == 0) {
        readLine(s_local, buffer, 256);
        strcpy(filename, buffer);
        strcat(print_operation_1_operation, " ");
        strcat(print_operation_1_operation, filename);

        readLine(s_local, buffer, 256);
        strcpy(filedescr, buffer);
        errcode = publish_file(username, filename, filedescr);
        sprintf(res, "%d", errcode);

        sendMessage(s_local, res, strlen(res) + 1);
        printf("s> OPERATION %s FROM %s\n", operacion, username);
        printf("s>\n");
    }


    else if (strcmp(operacion, "DELETE") == 0) {
        readLine(s_local, buffer, sizeof(buffer));
        strcpy(filename, buffer);
        strcat(print_operation_1_operation, " ");
        strcat(print_operation_1_operation, filename);
        errcode = delete_file(username, filename);
        sprintf(res, "%d", errcode);

        sendMessage(s_local, res, strlen(res) + 1);
        printf("s> OPERATION %s FROM %s\n", operacion, username);
        printf("s>\n");
    }



    else if (strcmp(operacion, "LIST_USERS")==0) {
        errcode = usuarioconectado(username);
        sprintf(res, "%d", errcode);
        sendMessage(s_local, res, strlen(res) + 1);

        if (errcode == 0) {
            char direccion[256];
            snprintf(direccion, sizeof(direccion), "./Users");

            // Cuento el número de clientes conectados del directorio "./Users"
          	int conectados = contar(direccion);

            sprintf(res, "%d", conectados);
            sendMessage(s_local, res, strlen(res) + 1);

            // Envio del nombre, IP y puerto de los clientes conectados recorriendo una lista dinámica
            UserFileEntry *lista = lista_usuarios(&conectados, direccion);
            for (int i = 0; i < conectados; i++){
            	snprintf(res, sizeof(res), "%s", lista[i].nombre);  // Solo el nombre del archivo
				sendMessage(s_local, res, strlen(res) + 1);

				snprintf(res, sizeof(res), "%s", lista[i].linea1);  // La primera línea del archivo
				sendMessage(s_local, res, strlen(res) + 1);

				snprintf(res, sizeof(res), "%s", lista[i].puerto);  // La segunda línea del archivo
				sendMessage(s_local, res, strlen(res) + 1);
            }

        }
        printf("s> OPERATION %s FROM %s\n", operacion, username);
        printf("s>\n");
    }


    else if (strcmp(operacion, "LIST_CONTENT")==0) {
        readLine(s_local, buffer, sizeof(buffer));
        strcpy(remoteusername, buffer);
        errcode = usuarioconectado(username);
        sprintf(res, "%d", errcode);

        if (errcode == 0) {
        	int registrado = 1;
			registrado = usuarioconectado(remoteusername);

            // Comprobamos si el cliente remoto está registrado
    		if (registrado == 1) {
				errcode = 3;
				sprintf(res, "%d", errcode);
				sendMessage(s_local, res, strlen(res) + 1);

			} else {
				sendMessage(s_local, res, strlen(res) + 1);

				char direccion[512];
            	snprintf(direccion, sizeof(direccion), "./Ficheros/%s", remoteusername);

                // Contamos ficheros publicados por el usuario remoto
          		int conectados = contar(direccion);

            	sprintf(res, "%d", conectados);
            	sendMessage(s_local, res, strlen(res) + 1);

                // Similar a la funcion de 'LIST_USER' solo que aquí solo enviamos un mensaje
				if (conectados != 0) {
					UserFileEntry *lista = lista_contenido(&conectados, direccion);

            		for (int i = 0; i < conectados; i++){
            			snprintf(res, 256, "%s", lista[i].nombre);
						sendMessage(s_local, res, strlen(res) + 1);
            		}
				}
			}
        }
        printf("s> OPERATION %s FROM %s\n", operacion, username);
        printf("s>\n");
    }
    else {
        strcpy(res, "Invalid operation");
        sendMessage(s_local, res, strlen(res) + 1);
    }
    
    
    get_sock_info(&IP);

    clnt = clnt_create (IP, PRINTRPC, PRINTRPCSERVER, "tcp");
	if (clnt == NULL) {
		clnt_pcreateerror (IP);
		exit (1);
	}

	retval_1 = print_operation_1(print_operation_1_username, print_operation_1_operation, print_operation_1_date, print_operation_1_hour, &result_1, clnt);
	if (retval_1 != RPC_SUCCESS) {
		clnt_perror (clnt, "call failed");
	}
	clnt_destroy (clnt);

    close(s_local);
    pthread_exit(0);
}

int write_all ( int newsd, char *buffer, size_t total )
{
    size_t escritos = 0 ;
    ssize_t result  = 0 ;

    while (escritos != total) // mientras queda por escribir...
    {
       result = write(newsd, buffer+escritos, total-escritos) ;
       // puede que write NO escriba todo lo solicitado de una vez
       if (-1 == result) {
           return -1 ;
       }

       escritos = escritos + result ;
    }

    return escritos ;
}

int main ( int argc, char **argv )
{
    int sd, newsd, ret;
    int pos = 0;
    socklen_t size;
    struct sockaddr_in server_addr, client_addr;

    if (argc != 3) {
        printf("Uso: %s -p <puerto>\n", argv[0]);
        return 0;
    }

    int puerto = atoi(argv[2]) ;

    // (1) Creación de un socket
    // * NO tiene dirección asignada aquí
    sd = socket(AF_INET, SOCK_STREAM, 0) ;
    if (sd < 0) {
        perror("Error en la creación del socket: ");
        return -1;
    }

    // (2) Obtener la dirección
    int val = 1;
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *) &val, sizeof(int));
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(puerto);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // (3) Asigna dirección a un socket:
    // * host = INADDR_ANY -> cualquier dirección del host
    // * port = 0 -> el sistema selecciona el primer puerto libre
    // * port = 1...1023 -> puertos reservados (puede necesitar ser root la ejecución)
    ret = bind(sd,(struct sockaddr *)& server_addr, sizeof(server_addr)) ;
    if (ret < 0) {
        perror("Error en bind: ") ;
        return -1 ;
    }

    // Con bind(port=0...) se buscaría el primer puerto libre y con
    // getsockname() se puede obtener el puerto asignado por bind
    size = sizeof(struct sockaddr_in) ;
    bzero(&client_addr, size);
    getsockname(sd, (struct sockaddr *) &client_addr, &size);
    printf("s> init server %s:%d\n",
            inet_ntoa(client_addr.sin_addr),
            ntohs(client_addr.sin_port));
    printf("s>\n");


    // (4) preparar para aceptar conexiones
    // * listen permite definir el número máximo de peticiones pendientes a encolar
    // * SOMAXCONN está en sys/socket.h
    ret = listen(sd, SOMAXCONN);
    if (ret < 0) {
        perror("Error en listen: ") ;
        return -1 ;
    }

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&no_lleno, NULL);
    pthread_cond_init(&no_vacio, NULL);
    pthread_mutex_init(&mfin, NULL);
    pthread_attr_init(&t_attr);

    pthread_attr_setdetachstate(&t_attr, PTHREAD_CREATE_DETACHED);


    while (1)
    {
        size = sizeof(struct sockaddr_in);
        newsd = accept(sd, (struct sockaddr *)&client_addr, &size);
        if (newsd < 0) {
            perror("Error en el accept");
            return -1;
        }

        // Obtener la IP del cliente
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);

        //printf("Conexión aceptada de IP: %s y puerto: %d\n", client_ip, ntohs(client_addr.sin_port));

        // Enviar socket y IP al hilo para que trate la petición
        pthread_mutex_lock(&mutex);
        while (n_elementos == MAX_PETICIONES) {
            pthread_cond_wait(&no_lleno, &mutex);
        }
        buffer_peticiones[pos] = newsd;
        pos = (pos + 1) % MAX_PETICIONES;
        n_elementos++;
        pthread_cond_signal(&no_vacio);
        pthread_mutex_unlock(&mutex);

        // Llamar al hilo con la IP del cliente
        pthread_create(&thid[pos], NULL, (void*)trat_pet, (void*)client_ip);
    }


    // (8) cerrar socket de servicio
    close(sd);

} /* fin main */