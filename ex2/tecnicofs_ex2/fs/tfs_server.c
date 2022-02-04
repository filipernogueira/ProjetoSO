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
static int rx;
int open_sessions = 0;

/*typedef struct{

} request;

void worker_thread(){
    
    while()
    
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
                
                break;
            case TFS_OP_CODE_UNMOUNT:
                
                break;
            case TFS_OP_CODE_OPEN:
                
                break;
            case TFS_OP_CODE_CLOSE:
                
                break;
            case TFS_OP_CODE_WRITE:
                
                break;
            case TFS_OP_CODE_READ:
                
                break;
            case TFS_OP_CODE_SHUTDOWN_AFTER_ALL_CLOSED:
                
                return 0;
            default:
                break;
        }
    }
}*/

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
        if(write(tx, &error, sizeof(int)) == -1){
            fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
            return -1;
        }
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
    
    session_ids[id] = -1;
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
    char name[40];
    
    memset(name, '\0', 40 * sizeof(char));

    memcpy(&session_id, buffer, sizeof(int));
    memcpy(name, buffer + sizeof(int), sizeof(char) * 40);
    memcpy(&flags, buffer + sizeof(int) + sizeof(char) * 40, sizeof(int));

    int ret = tfs_open(name, flags);

    if(ret == -1){
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        return -1;
    }

    if (write(session_ids[session_id], &ret, sizeof(int)) == -1){
        fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
        return -1;
    }

    free(buffer);

    return 0;
}

int server_close(){
    void *buffer = malloc(2 * sizeof(int));

    if(read(rx, buffer, sizeof(int) * 2) == -1){
        fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
        return -1;
    }

    int session_id, fhandle;

    memcpy(&session_id, buffer, sizeof(int));
    memcpy(&fhandle, buffer + sizeof(int), sizeof(int));

    int ret = tfs_close(fhandle);

    if(ret == -1){
        fprintf(stderr, "[ERR]: close failed: %s\n", strerror(errno));
        return -1;
    }

    if (write(session_ids[session_id], &ret, sizeof(int)) == -1){
        fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
        return -1;
    }

    free(buffer);

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
    memcpy(&fhandle, local_buffer + sizeof(int), sizeof(int));
    memcpy(buffer, local_buffer + sizeof(int) * 2, sizeof(char) * len);
    

    ssize_t ret = tfs_write(fhandle, buffer, len);

    if(ret == -1){
        fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
        return -1;
    }

    if (write(session_ids[session_id], &ret, sizeof(ssize_t)) == -1){
        fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
        return -1;
    }

    free(local_buffer);

    return 0;
}

int server_read(){
    void *buffer = malloc(sizeof(int) * 2 + sizeof(size_t));

    if(read(rx, buffer, sizeof(int) * 2 + sizeof(size_t)) == -1){
        fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
        return -1;
    }

    int session_id, fhandle;
    size_t len;

    memcpy(&session_id, buffer, sizeof(int));
    memcpy(&fhandle, buffer + sizeof(int), sizeof(int));
    memcpy(&len, buffer + 2 * sizeof(int), sizeof(size_t));

    void *data = malloc(sizeof(char) * len);

    ssize_t num_bytes = tfs_read(fhandle, data, len);

    if(num_bytes == -1){
        fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
        return -1;
    }

    if (write(session_ids[session_id], &num_bytes, sizeof(ssize_t)) == -1){
        fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
        return -1;
    }

    if (write(session_ids[session_id], data, sizeof(char) * len) == -1){
        fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
        return -1;
    }

    free(buffer);
    free(data);

    return 0;
}

int server_shutdown(){
    int session_id;

    if(read(rx, &session_id, sizeof(int)) == -1){
        fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
        return -1;
    }

    int ret = tfs_destroy_after_all_closed();

    if(ret == -1){
        fprintf(stderr, "[ERR]: close failed: %s\n", strerror(errno));
        return -1;
    }

    if(write(session_ids[session_id], &ret, sizeof(int)) == -1){
        fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
        return -1;
    }

    if(close(rx) == -1){
        fprintf(stderr, "[ERR]: close failed: %s\n", strerror(errno));
        return -1;
    }

    for(int id = 0; id < S; id++){
        if(session_ids[id] != -1){
            if(close(session_ids[id]) == -1){
                fprintf(stderr, "[ERR]: close failed: %s\n", strerror(errno));
                return -1;
            }
        }
    }

    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Please specify the pathname of the server's pipe.\n");
        return 1;
    }

    char *pipename = argv[1];
    printf("Starting TecnicoFS server with pipe called %s\n", pipename);

    if(tfs_init() == -1)
        return -1;

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
        ssize_t ret = read(rx, &op_code, sizeof(char));

        if(ret == -1){
            fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
            return -1;
        } else if (ret == 0){
            continue;
        }
        switch(op_code){
            case TFS_OP_CODE_MOUNT:
                if(server_mount() == -1)
                    return -1;
                break;
            case TFS_OP_CODE_UNMOUNT:
                if(server_unmount() == -1)
                    return -1;
                break;
            case TFS_OP_CODE_OPEN:
                if(server_open() == -1)
                    return -1;
                break;
            case TFS_OP_CODE_CLOSE:
                if(server_close() == -1)
                    return -1;
                break;
            case TFS_OP_CODE_WRITE:
                if(server_write() == -1)
                    return -1;
                break;
            case TFS_OP_CODE_READ:
                if(server_read() == -1)
                    return -1;
                break;
            case TFS_OP_CODE_SHUTDOWN_AFTER_ALL_CLOSED:
                if(server_shutdown() == -1)
                    return -1;
                return 0;
            default:
                break;
        }
    }

    return 0;
}