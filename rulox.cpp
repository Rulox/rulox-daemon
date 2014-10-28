/**
 * Rulox Daemon - Demonio simple para Linux
 * Raúl Marrero Rodríguez
 **/

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <stdarg.h>



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
        exit(EXIT_FAILURE);
    }

    // Escribimos el PID en /var/run/ para hacer stop/restart 
    int lfp;
    char str[10];
    lfp = open("/var/run/ruloxdaemon.pid", O_RDWR|O_CREAT, 0640);

    if (lfp < 0) {
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
    
    /* Bucle principal del demonio */ 
    while (1) {
       sleep(30); 
    }
    
    closelog();
    exit(EXIT_SUCCESS);
}
