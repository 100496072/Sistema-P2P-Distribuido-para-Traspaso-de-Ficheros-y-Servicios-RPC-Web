int register_user(char *username);
int unregister_user(char *username);
int connect_user(char *username, char *IP, char *PORT);
int disconnect_user(char *username);
int publish_file(char *username, char *filename, char *filedescr);
int delete_file(char *username, char *filename);
int usuarioconectado(char *username);


int contar(char *direccion);

typedef struct {
    char nombre[256];
    char linea1[256];
    char puerto[256];
} UserFileEntry;

UserFileEntry* lista_usuarios(int *conectados, char *direccion);

UserFileEntry* lista_contenido(int *conectados, char *direccion);
