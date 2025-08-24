/* Marcos Romo Poveda, Luca Petidier Iglesias */
/* 100496072@alumnos.uc3m.es 100496633@alumnos.uc3m.es */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/file.h>
char *pathFicheros = "./Ficheros/";
char *pathUsers = "./Users/";



// Función que comprueba si un determinado usuario está conectado
int usuarioconectado(char *username) {
    int errcode = 0;

    DIR *dir = opendir("./Users/");
    if (dir == NULL) {
        errcode = 1;  // Directorio no existe, usuario no registrado

    } else {
        if (dir) closedir(dir);

        char user[512];
        snprintf(user, sizeof(user), "./Users/%s", username);
        if (access(user, F_OK) != 0) {
            errcode = 1;  // Usuario no registrado

        } else {
            FILE* file = fopen(user, "r");
            if (file == NULL) {
                errcode = 3;  // Error al abrir el archivo del usuario

            } else {
                fseek(file, 0L, SEEK_END);
                size_t sz = ftell(file);
                fseek(file, 0L, SEEK_SET);

                if (sz == 0) {
                    errcode = 2;  // Usuario no está conectado

                } else {

                   return 0;
                }

                fclose(file);
            }
        }
    }
    return errcode;
}


int register_user(char *username) {

    int errcode = 0;

    //Abrimos el directorio, si no existe se crea
    DIR * dir = opendir(pathUsers);
    if (dir == NULL){
        mkdir(pathUsers, 0700);
    }
    closedir(dir);

    // Creamos la ruta completa del archivo
    char user[256];
    snprintf(user, sizeof(user), "%s%s", pathUsers, username);

    // Si el fichero existe es que el usuario ya estaba registrado
    if (access(user, F_OK) == 0) {
        errcode = -1;
    }

    FILE* file = fopen(user, "w");
    if (file == NULL) {
        errcode = -1;
    }

    //Escribimos en el fichero
    int fd = fileno(file);
    flock(fd, LOCK_EX);
    flock(fd, LOCK_UN);
    fclose(file);

    return errcode;
}


int unregister_user(char *username) {

    char filename[256];
    snprintf(filename, sizeof(filename), "%s%s", pathUsers, username);

    int errcode = 0;

    // Si el archivo no se puede abrir es que no existia el usuario
    int fd = open(filename, O_RDONLY);
    if (fd == -1){
        errcode = -1;
    }

    flock(fd, LOCK_EX);
    if (remove(filename) != 0){
        errcode = -1;
    }

    flock(fd, LOCK_UN);
    close(fd);
    return errcode;
}



int connect_user(char *username, char *IP, char *PORT) {

    // Compruebo si el usuario está registrado y/0 conectado
	int errcode = usuarioconectado(username);

    if (errcode != 2) {
        if (errcode == 0) {
            errcode = 2;  // Usuario conectado
        }
        return errcode;  // Usuario no registrado

    } else {

      errcode = 0;

      char user[256];
      snprintf(user, sizeof(user), "%s%s", pathUsers, username);

      FILE *file = fopen(user, "r+");
      int fd = fileno(file);
      flock(fd, LOCK_EX);

      fprintf(file, "%s\n%s", IP, PORT);
      fflush(file);

      flock(fd, LOCK_UN);
      fclose(file);

    }
    return errcode;
}


int disconnect_user(char *username) {
    int errcode = usuarioconectado(username);

    if (errcode != 0) {
        return errcode;  // Usuario no conectado

    } else {

      char user[256];
      snprintf(user, sizeof(user), "%s%s", pathUsers, username);

      FILE *file = fopen(user, "r+");

      int fd = fileno(file);
      flock(fd, LOCK_EX);

      // Borra los datos del archivo
      if (ftruncate(fd, 0) == -1) {
          flock(fd, LOCK_UN);
          fclose(file);
          errcode = 4;
      }

      fseek(file, 0, SEEK_SET);

      flock(fd, LOCK_UN);
      fclose(file);
    }
    return errcode;
}



int publish_file(char *username, char *filename, char *filedescr) {
    int errcode = usuarioconectado(username);
    if (errcode != 0) {
        return errcode;  // Usuario no conectado

    } else {

        DIR *dir = opendir(pathFicheros);
        if (dir == NULL) {
            mkdir(pathFicheros, 0700);
        }
        if (dir) closedir(dir);

        // Crear el directorio de los ficheros del usuario
        char userdir[256];
        snprintf(userdir, sizeof(userdir), "%s/%s", pathFicheros, username);
        if (access(userdir, F_OK) != 0) {
            mkdir(userdir, 0700);
        }

        DIR *user_dir = opendir(userdir);

        if (user_dir != NULL) {
            struct dirent *entry;

            // Aqui se leeran todos los archivos del directorio correspondiente
            // Se comprobará la primera línea de todos para ver si la ruta pasada por argumento ya está guardada

            while ((entry = readdir(user_dir)) != NULL) {
                int num;

                if (sscanf(entry->d_name, "%d.txt", &num) == 1) {
                    char filepath[512];
                    snprintf(filepath, sizeof(filepath), "%s/%s", userdir, entry->d_name);
                    FILE *fp = fopen(filepath, "r");

                    if (fp != NULL) {
                        char line[256];
                        if (fgets(line, sizeof(line), fp)) {

                            // Elimina salto de línea si existe
                            line[strcspn(line, "\n")] = 0;

                            // Comprueba si lo guardado es igual a la ruta pasada por algumento
                            if (strcmp(line, filename) == 0) {
								fclose(fp);
								closedir(user_dir);  // Cierra el directorio
                                return 3;
                            }
                        }
                        fclose(fp);
                    }
					else {
						fclose(fp);
					}
                }
            }
            closedir(user_dir);
        }


        int max_num = 0;
        user_dir = opendir(userdir);

        // Guardamos el número del último archivo publicado
        if (user_dir != NULL) {
            struct dirent *entry;
            while ((entry = readdir(user_dir)) != NULL) {
                int num;
                if (sscanf(entry->d_name, "%d.txt", &num) == 1) {
                    if (num > max_num) {
                        max_num = num;
                    }
                }
            }
            closedir(user_dir);
        }

        int new_file_num = max_num + 1;

        char file[512];
        snprintf(file, sizeof(file), "%s/%d.txt", userdir, new_file_num);


        FILE *filep = fopen(file, "w");
        if (filep == NULL) {
            printf("%s\n", file);
            printf("Error al crear el archivo\n");
            errcode = 4; // Error al crear el archivo
        }

        int fd = fileno(filep);
        flock(fd, LOCK_EX);
        fprintf(filep, "%s\n%s", filename, filedescr); // Guardar filename y filedescr
        flock(fd, LOCK_UN);
        fclose(filep);
    }
    return errcode;
}


int delete_file(char *username, char *filename) {
    int errcode = usuarioconectado(username);
    if (errcode != 0) {
        return errcode;  // Usuario no conectado

    } else {

        char userdir[256];
        snprintf(userdir, sizeof(userdir), "%s/%s", pathFicheros, username);

        // Comrpuebo si el directorio donde se guardan los ficheros del cliente existe
        DIR *dir = opendir(userdir);
        if (!dir) return 3;

        struct dirent *entry;
        char file_path[512] = "";

        // Compruebo todos los ficheros
        while ((entry = readdir(dir)) != NULL) {
            int num;
            if (sscanf(entry->d_name, "%d.txt", &num) == 1) {
                char temp_path[512];
                snprintf(temp_path, sizeof(temp_path), "%s/%s", userdir, entry->d_name);
                FILE *fp = fopen(temp_path, "r");
                if (!fp) {
                    continue;
                }

                char line[256];

                // Comrpuebo la primera línea del archivo y veo si se corresponde con la ruta pasada por argumento
                if (fgets(line, sizeof(line), fp)) {
                    line[strcspn(line, "\n")] = 0;
                    if (strcmp(line, filename) == 0) {
                        fclose(fp);
                        strncpy(file_path, temp_path, sizeof(file_path));
                        break;
                    }
                }
                fclose(fp);
            }
        }
        closedir(dir);

        // Si no existía el archivo con dicha ruta devuelvo 3 que indicará error
        if (strlen(file_path) == 0) {
            errcode = 3;
        }

        // Abro el archivo y lo elimino
        int fd = open(file_path, O_RDONLY);
        if (fd == -1) {
            errcode = 4;
        } else {
            flock(fd, LOCK_EX);
            if (remove(file_path) != 0) {
                errcode = 4;
            }
            flock(fd, LOCK_UN);
            close(fd);
        }
    }
    return errcode;
}


// Funcion que cuenta el número de clientes conectados
int contar (char *direccion){
    struct dirent *entry;
    DIR *dir = opendir(direccion);

    int conectados = 0;
	if (dir == NULL) {
        conectados = 0;  // Si el directorio no existe no hay ningún cliente conectado

    } else {

        // Leo todos los archivos
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] == '.') {
                continue;
            }

            char filepath[1024];
            snprintf(filepath, sizeof(filepath), "%s/%s", direccion, entry->d_name);

            FILE *file = fopen(filepath, "r");
            if (file == NULL) {
                continue;
            }

            // Compruebo si hay algo escrito en el archivo
            int lleno = 0;
            int c = fgetc(file);
            if (c != EOF) {
                lleno = 1;
            }

            fclose(file);

            // Si el archivo tenía contenido le sumo 1 al contador de clientes conectados
            if (lleno) {
                conectados++;
            }
        }
	}
    closedir(dir);
    return conectados;
}


// Estructura que cuardará nombre, la primera línea (IP o ruta completa) y puerto
typedef struct {
    char nombre[256];
    char linea1[256];
    char puerto[256];
} UserFileEntry;

UserFileEntry* lista_usuarios(int *conectados, char *direccion) {
    DIR *dir = opendir(direccion);
    struct dirent *entry;
    *conectados = 0;


    // Reservamos memoria para la lista de archivos
    UserFileEntry *userList = malloc(100 * sizeof(UserFileEntry));

    // Recorremos los archivos del directorio
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;

        // Me guardo el nombre del archivo
        char filepath[1024];
        snprintf(filepath, sizeof(filepath), "%s/%s", direccion, entry->d_name);

        FILE *file = fopen(filepath, "r");
        if (file == NULL) {
            continue;
        }

        // Creo variables locales linea1 que será la IP y puerto
        char linea1[256] = {0}, puerto[256] = {0};

        // Leer las dos primeras líneas del archivo y guardo los datos
        fgets(linea1, sizeof(linea1), file);
        linea1[strcspn(linea1, "\n")] = 0;
        fgets(puerto, sizeof(puerto), file);
        puerto[strcspn(puerto, "\n")] = 0;
        fclose(file);

        // Si el archivo está vacío, lo ignoramos
        if (linea1[0] == '\0' && puerto[0] == '\0') {
            continue;
        }

        // Guardo los datos en la lista
        strncpy(userList[*conectados].nombre, entry->d_name, 256);
        strncpy(userList[*conectados].linea1, linea1, 256);
        strncpy(userList[*conectados].puerto, puerto, 256);

        // Puntero que se suma 1 para ir guardando en otra posición de la lista
        (*conectados)++;
    }

    closedir(dir);
    return userList;
}


UserFileEntry* lista_contenido(int *conectados, char *direccion) {
    DIR *dir = opendir(direccion);
    struct dirent *entry;
    *conectados = 0;


    // Reservamos memoria para la lista de archivos
    UserFileEntry *userList = malloc(100 * sizeof(UserFileEntry));

    // Recorremos los archivos del directorio
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') {
            continue;
        }

        // Ruta completa de cada fichero
        char filepath[1024];
        snprintf(filepath, sizeof(filepath), "%s/%s", direccion, entry->d_name);

        FILE *file = fopen(filepath, "r");
        if (file == NULL) {
            continue;
        }

        char linea1[256] = {0};

        fgets(linea1, sizeof(linea1), file);
        linea1[strcspn(linea1, "\n")] = 0;  // Eliminar '\n' final
        fclose(file);

        if (linea1[0] == '\0') {
            continue;
        }
        // Guardar los datos en la lista
        strncpy(userList[*conectados].nombre, linea1, 256);


        (*conectados)++;
    }

    closedir(dir);
    return userList;
}