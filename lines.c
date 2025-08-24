/* Marcos Romo Poveda, Luca Petidier Iglesias */
/* 100496072@alumnos.uc3m.es 100496633@alumnos.uc3m.es */

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>



ssize_t readLine(int fd, void *buffer, size_t n){
    ssize_t numRead; /* num of bytes fetched by last read() */
    size_t totRead; /* total bytes read so far */
    char *buf;
    char ch;
    if (n <= 0 || buffer == NULL) {
    errno = EINVAL;
    return -1;
    }
    buf = buffer;
    totRead = 0;
    for (;;) {
        numRead = read(fd, &ch, 1); /* read a byte */
        if (numRead == -1) {
            if (errno == EINTR) /* interrupted -> restart read() */
                continue;
            else
                return -1; /* some other error */
        } else if (numRead == 0) { /* EOF */
            if (totRead == 0) /* no byres read; return 0 */
                return 0;
            else
                break;
        } else { /* numRead must be 1 if we get here*/
            if (ch == '\n')
                break;
            if (ch == '\0')
                break;
            if (totRead < n - 1) { /* discard > (n-1) bytes */
                totRead++;
                *buf++ = ch;
            }
        }
    }
    *buf = '\0';
    return totRead;
    }

int sendMessage(int socket, char *buffer, int len)
    {
        int r;
        int l = len;
        do {
            r = write(socket, buffer, l);
            l = l - r;
            buffer = buffer + r;
        } while ((l>0) && (r>=0));
        if (r < 0)
            return (-1); /* fallo */
        else
            return(0); /* se ha enviado longitud */
    }