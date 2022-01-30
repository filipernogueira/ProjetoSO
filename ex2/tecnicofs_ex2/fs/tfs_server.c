#include "operations.h"
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int session_ids[S];
int static rx;
int open_sessions = 0;

int server_mount(){
    char client_pipe_path[40];

    if(read(rx, &client_pipe_path, sizeof(char) * 40) == -1){
        fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
        return -1;
    }

    int tx = open(client_pipe_path, O_WRONLY);
    if (tx == -1) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        return -1;
    }

    if(open_sessions >= S){
        int error = -1;
        write(tx, &error, sizeof(int));
        return -1;
    }

    int id = 0;
    for(;id < S; id++){
        if(session_ids[id] == -1){
            session_ids[id] = tx;
            open_sessions++;
        }
    }

    if (write(tx, &id, sizeof(int)) == -1){
        fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

int server_unmount(){
    int id;

    if(read(rx, &id, sizeof(int)) == -1){
        fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
        return -1;
    }

    if(close(session_ids[id]) == -1){
        fprintf(stderr, "[ERR]: close failed: %s\n", strerror(errno));
        return -1;
    }
    
    session_ids[id] == -1;
    open_sessions--;
    
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Please specify the pathname of the server's pipe.\n");
        return 1;
    }

    char *pipename = argv[1];
    printf("Starting TecnicoFS server with pipe called %s\n", pipename);

    if (unlink(pipename) != 0 && errno != ENOENT) {
        fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", pipename, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (mkfifo(pipename, 0640) != 0) {
        fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    for(int i = 0; i < S; i++)
        session_ids[i] = -1;

    rx = open(pipename, O_RDONLY);

    if(rx == -1){
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        return -1;
    }

    while(true){
        char op_code;
        int ret = read(rx, &op_code, sizeof(char));

        if(ret == -1){
            fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
            return -1;
        } else if (ret == 0){
            continue;
        }
        switch(op_code){
            case TFS_OP_CODE_MOUNT:
                server_mount();
                break;
            case TFS_OP_CODE_UNMOUNT:
                server_unmount();
                break;
            case TFS_OP_CODE_OPEN:
                server_open();
                break;
            case TFS_OP_CODE_CLOSE:
                server_close();
                break;
            case TFS_OP_CODE_WRITE:
                server_write();
                break;
            case TFS_OP_CODE_READ:
                server_read();
                break;
            case TFS_OP_CODE_SHUTDOWN_AFTER_ALL_CLOSED:
                server_shutdown();
                break;
            default:
                break;
        }
    }


    return 0;
}