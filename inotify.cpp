#include <sys/inotify.h>
#include <sys/types.h>
#include <iostream>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <string>
#include <syscall.h>
#include <unistd.h>
#include <vector>

#define BUF_LEN (10 * sizeof(struct inotify_event) + NAME_MAX + 1)

#include <dirent.h>
#include <limits.h>

using namespace std;
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
    printf ("wd=%d ", event -> wd);
    // if cookie > 0 lo imprimimos
    printf ("mask= ");
    if (event -> mask& IN_CREATE)
        printf("IN_CREATE");
    if (event -> mask& IN_MODIFY)
        printf("IN_MODIFY");
    if (event -> mask& IN_MOVE_SELF)
        printf("IN_MOVE_SELF");
    if (event -> mask& IN_MOVED_FROM)
        printf("IN_MOVED_FROM");
    if (event -> mask& IN_MOVED_TO)
        printf("IN_MOVED_TO");
    if (event -> mask& IN_DELETE)
        printf("IN_DELETE");
    if (event -> mask& IN_DELETE_SELF)
        printf("IN_DELETE_SELF");
    if (event -> mask& IN_OPEN)
        printf("IN_OPEN");
    if (event -> mask& IN_ACCESS)
        printf("IN_ACCESS");
    if (event -> mask& IN_ATTRIB)
        printf("IN_ATTRIB");
    if (event -> mask& IN_CLOSE_WRITE)
        printf("IN_CLOSE_WRITE");
    if (event -> mask& IN_CLOSE_NOWRITE)
        printf("IN_CLOSE_NOWRITE");
    printf(" -- ");
    //si el len es > 0, es sobre otro fichero
    if (event -> len > 0)
        printf("name=%s\n", event -> name);
    else 
        printf("root (watched) directory\n");
}

/**
 * Función que explora un directorio recursivamente.
 * Para cada subdirectorio agrega su ruta
 * a un vector de rutas donde lanzar el add_watch  
 * */
void explorar(const char * dir_name, std::vector<string> &dirVector) {
    DIR * d;
    d = opendir (dir_name);

    if (! d) {
        exit (EXIT_FAILURE);
    }

    while (1) {
        struct dirent * entry;
        const char * d_name;

        entry = readdir (d);
        if (! entry) {
            break;
        }
        d_name = entry->d_name;

        if ((entry->d_type & DT_DIR)) { 
            if (strcmp (d_name, "..") != 0 && strcmp (d_name, ".") != 0) { 
                string dirPath = string(dir_name); 
                dirPath += "/";
                dirPath += d_name;
                dirVector.push_back(dirPath);
            }
        }

        if (entry->d_type & DT_DIR) {
            
            if (strcmp (d_name, "..") != 0 &&
                strcmp (d_name, ".") != 0) {
                int path_length;
                char path[PATH_MAX];
 
                path_length = snprintf (path, PATH_MAX,
                                        "%s/%s", dir_name, d_name);
                if (path_length >= PATH_MAX) {
                    exit (EXIT_FAILURE);
                }
                
                explorar(path, dirVector);
            }
        }
    }
    if (closedir (d)) {
        exit (EXIT_FAILURE);
    }
}

int main (int argc, char* argv[]) {
    int inotifyd, wd;
    char buf[BUF_LEN];
    ssize_t numread;
    char* b;
    struct inotify_event* event;

    inotifyd = inotify_init();
    std::vector<string> dirVector;  // Vector con las rutas de los subdirectorios
    for (int i = 1; i < argc; i++) {
        dirVector.push_back(argv[i]);
        explorar(argv[i], dirVector);
    }

    for (int i = 0; i < dirVector.size(); i++) {
        const char * dir = dirVector[i].c_str();
        cout << dir << endl;
        wd = inotify_add_watch(inotifyd, dir, IN_ALL_EVENTS); 
    }

    while(1) {
        numread = read(inotifyd, buf, BUF_LEN);

        if (numread < 0)
            exit(EXIT_FAILURE);
        else {
            for (b = buf; b < buf + numread;)  {
                event = (struct inotify_event*)b;
                //if (event -> mask& IN_CREATE) // Antes de enviarlo al LOG
                mostrarEvento(event);
                b += sizeof(struct inotify_event) + event -> len;
            }
        }
    }
    //rmwatch
    exit(0);
}
