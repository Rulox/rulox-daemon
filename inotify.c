#include <sys/inotify.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#define BUF_LEN (10 * sizeof(struct inotify_event) + NAME_MAX + 1)
/*
int inotify_init();  devuelve n del descriptor o -1 si es un error
int inotify_add_watch(int fd, const char* pathname, uint32_t mask);
int inotify_rm_watch(int fd, const wd);  wd es el descriptor del watch  0 si win, -1 si lose

struct inotify_event {
    int wd;
    uint32_t mask;
    uint32_t cookie;  cuando se renombra un fichero, identifica los dos ficheros origen y destino
    uint32_t length; longitud de name
    char name[]; nombre del fichero
}

para leer todo esto necesitamos un buffer

sizeof(struct inotify_event) + len;
sizeof(struct inotify_event) + NAME_MAX + 1;
para leer usamos 15/20/10 veces lo anterior. NAME_MAX es una cte de sistema, q dice lo maximo de nombre de fich

*/

void mostrarEvento(struct inotify_event* event) {
    printf ("wd=%d\n", event -> wd);
    // if cookie > 0 lo imprimimos
    printf ("mask= ");
    if (event -> mask& IN_CREATE)
        printf("IN_CREATE");
    if (event -> mask& IN_MODIFY)
        printf("IN_MODIFY");
    if (event -> mask& IN_DELETE)
        printf("IN_DELETE");
    if (event -> mask& IN_OPEN)
        printf("IN_OPEN");
    printf("\n");
    //si el len es > 0, es sobre otro fichero
    if (event -> len > 0)
        printf("name=%s\n", event -> name);
}

int main (int argc, char* argv[]) {
    int inotifyd, wd;
    char buf[BUF_LEN];
    ssize_t numread;
    char* b;
    struct inotify_event* event;

    inotifyd = inotify_init();
    wd = inotify_add_watch(inotifyd, "/home/raul/master/inotify/", IN_ALL_EVENTS);

    while(1) {
        numread = read(inotifyd, buf, BUF_LEN);
        //si es 0, no cabe en el buffer. si es -1 error por algo, sinom seguimos. 
        for (b = buf; b < buf+numread;)  {
            event = (struct inotify_event*)b;
            mostrarEvento(event);
            b += sizeof(struct inotify_event) + event -> len;
        }
    }
    exit(0);
}
