/**
 * Rulox Daemon - Demonio simple para Linux
 * Raúl Marrero Rodríguez
 **/
#include <sys/inotify.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <stdarg.h>
#include <string>
#include <syscall.h>
#include <vector>

#define BUF_LEN (10 * sizeof(struct inotify_event) + NAME_MAX + 1)

#include <dirent.h>

using namespace std;


const char * mostrarEvento(struct inotify_event* event) {
    string message;
    message +=  event -> wd;

    message += (" \nAction= ");
    if (event -> mask& IN_CREATE)
        message += ("IN_CREATE");
    if (event -> mask& IN_MODIFY)
        message += ("IN_MODIFY");
    if (event -> mask& IN_MOVE_SELF)
        message += ("IN_MOVE_SELF");
    if (event -> mask& IN_MOVED_FROM)
        message += ("IN_MOVED_FROM");
    if (event -> mask& IN_MOVED_TO)
        message += ("IN_MOVED_TO");
    if (event -> mask& IN_DELETE)
        message += ("IN_DELETE");
    if (event -> mask& IN_DELETE_SELF)
        message += ("IN_DELETE_SELF");
    if (event -> mask& IN_OPEN)
        message += ("IN_OPEN");
    if (event -> mask& IN_ACCESS)
        message += ("IN_ACCESS");
    if (event -> mask& IN_ATTRIB)
        message += ("IN_ATTRIB");
    if (event -> mask& IN_CLOSE_WRITE)
        message += ("IN_CLOSE_WRITE");
    if (event -> mask& IN_CLOSE_NOWRITE)
        message += ("IN_CLOSE_NOWRITE");
    message += (" -- ");
    //si el len es > 0, es sobre otro fichero
    if (event -> len > 0)
        message += string(" file/folder named ->") + (event -> name);
    else 
        message += (" root (watched) folder\n");
    const char * messageChar = message.c_str();
    syslog(LOG_NOTICE, messageChar);         
    return messageChar;
}


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

int main(int argc, char *argv[]) {
    pid_t pid, sid;
    
    pid = fork();

    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    umask(0);
            
    openlog(argv[0],LOG_NOWAIT|LOG_PID,LOG_USER); 
    syslog(LOG_NOTICE, "[Rulox Daemon] - Creado correctamente\n");         
    sid = setsid();
    

    if (sid < 0) {
        syslog(LOG_NOTICE, "[Rulox Daemon] - Error sid\n");
        exit(EXIT_FAILURE);
    }


    // Escribimos el PID en /var/run/ para hacer stop/restart 
    int lfp;
    char str[10];
    lfp = open("/var/run/ruloxdaemon.pid", O_RDWR|O_CREAT, 0640);

    if (lfp < 0) {
        syslog(LOG_NOTICE, "[Rulox Daemon] - ERROR abriendo fichero .pid\n");         
        exit(1); 
    }


    sprintf(str,"%d\n",getpid());
    write(lfp, str, strlen(str)); 

    if ((chdir("/")) < 0) {
        exit(EXIT_FAILURE);
    }
    
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    
    int inotifyd, wd;
    char buf[BUF_LEN];
    ssize_t numread;
    char* b;
    struct inotify_event* event;

    if (argc <= 1) {
        cout << "[Error] Help: Parameters: Full Path to Folder" << endl;
        exit(0);
    }
    inotifyd = inotify_init();
    std::vector<string> dirVector;  // Vector con las rutas de los subdirectorios
    for (int i = 1; i < argc; i++) {
        dirVector.push_back(argv[i]);
        explorar(argv[i], dirVector);
    }

    for (int i = 0; i < dirVector.size(); i++) {
        const char * dir = dirVector[i].c_str();
        wd = inotify_add_watch(inotifyd, dir, IN_ALL_EVENTS); 
    }
    /* SOCKET */
    int sockfd;
    int portnumber = 8000;                // Puerto por defecto
    struct sockaddr_in serv_addr;
    struct hostent * server;
    char buffer[256];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    server = gethostbyname("localhost");   // Cambiar esto 
    if (server == NULL)
        exit(1); // error

    serv_addr.sin_family = AF_INET;
    bcopy((char *)server -> h_addr, (char *) & serv_addr.sin_addr.s_addr, server -> h_length);

    serv_addr.sin_port = htons(portnumber);
    connect(sockfd, (struct sockaddr *) & serv_addr, sizeof(serv_addr));
    /* END SOCKET */


    /* Bucle principal del demonio */ 
    while (1) {
        numread = read(inotifyd, buf, BUF_LEN);

        if (numread < 0) {
            //exit(EXIT_FAILURE);
        } else {
            for (b = buf; b < buf + numread;)  {
                event = (struct inotify_event*)b;
                const char * message = mostrarEvento(event);
                bzero(buffer, 256);
                strncpy(buffer, message, 255);
                write(sockfd, buffer, strlen(buffer));
                b += sizeof(struct inotify_event) + event -> len;
            }
        }
    }
    close(sockfd); 
    closelog();
    exit(EXIT_SUCCESS);
}
