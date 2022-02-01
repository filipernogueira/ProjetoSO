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

//É SUPOSTO ASSUMIR QUE O OPCODE SAI DO BUFFER DEPOIS DE LIDO NO MAIN?
//COMO DEVEMOS TRATAR OS ERROS NO MAIN?

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
            break;
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

int server_open(){
    void *buffer = malloc(sizeof(char) * 40 + 2 * sizeof(int));

    if(read(rx, buffer, sizeof(int) * 2 + sizeof(char) * 40) == -1){
        fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
        return -1;
    }

    int session_id, flags;
    char client_path_name[40];

    memcpy(&session_id, buffer, sizeof(int));
    memcpy(&client_path_name, buffer + 1, sizeof(char) * 40);
    memcpy(&flags, buffer + 41, sizeof(int));

    int ret = tfs_open(client_path_name, flags);

    if(ret == -1){
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        return -1;
    }

    if (write(session_ids[session_id], &ret, sizeof(int)) == -1){
        fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

int server_close(){
    int buffer[2];

    if(read(rx, &buffer, sizeof(int) * 2) == -1){
        fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
        return -1;
    }

    int session_id, fhandle;

    memcpy(&session_id, buffer, sizeof(int));
    memcpy(&fhandle, buffer + 1, sizeof(int));

    int ret = tfs_close(fhandle);

    if(ret == -1){
        fprintf(stderr, "[ERR]: close failed: %s\n", strerror(errno));
        return -1;
    }

    if (write(session_ids[session_id], &ret, sizeof(int)) == -1){
        fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

int server_write(){
    size_t len;
    
    if(read(rx, &len, sizeof(size_t)) == -1){
        fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
        return -1;
    }

    void *local_buffer = malloc(sizeof(char) * len + 2 * sizeof(int));

    if(read(rx, local_buffer, sizeof(char) * len + 2 * sizeof(int)) == -1){
        fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
        return -1;
    }

    int session_id, fhandle;
    char buffer[len];

    memcpy(&session_id, local_buffer, sizeof(int));
    memcpy(&fhandle, local_buffer + 1, sizeof(int));
    memcpy(&buffer, local_buffer + 2, sizeof(char) * 40);
    

    int ret = tfs_write(fhandle, buffer, len);

    if(ret == -1){
        fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
        return -1;
    }

    if (write(session_ids[session_id], &ret, sizeof(int)) == -1){
        fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
        return -1;
    }

    free(local_buffer);

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