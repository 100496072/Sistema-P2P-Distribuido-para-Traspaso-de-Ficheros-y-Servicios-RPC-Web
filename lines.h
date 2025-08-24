/* Marcos Romo Poveda, Luca Petidier Iglesias */
/* 100496072@alumnos.uc3m.es 100496633@alumnos.uc3m.es */

#include <unistd.h>

int sendMessage(int socket, char *buffer, int len);
ssize_t readLine(int fd, void *buffer, size_t n);
