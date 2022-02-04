#include "tecnicofs_client_api.h"
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

int tx, rx, session_id;


int tfs_mount(char const *client_pipe_path, char const *server_pipe_path) {
    if (unlink(client_pipe_path) != 0 && errno != ENOENT) {
        fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", client_pipe_path, strerror(errno));
        return -1;
    }
    
    if (mkfifo(client_pipe_path, 0640) != 0) {
        fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
        return -1;
    }

    char buffer[41];
    memset(buffer, '\0', 41 * sizeof(char));

    tx = open(server_pipe_path, O_WRONLY);
    if (tx == -1) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        return -1;
    }

    char const opcode = TFS_OP_CODE_MOUNT;
    memcpy(buffer, &opcode, sizeof(char));
    memcpy(buffer + sizeof(char), client_pipe_path, sizeof(char) * strlen(client_pipe_path));

    if (write(tx, buffer, 41 * sizeof(char)) == -1){
        fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
        return -1;
    }

    rx = open(client_pipe_path, O_RDONLY);
    if (rx == -1) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        return -1;
    }  

    int ret = -1;

    if(read(rx, &ret, sizeof(int)) == -1 || ret < 0){
        fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
        return -1;
    }

    session_id = ret;

    return 0;
}

int tfs_unmount() {
    char const op_code = TFS_OP_CODE_UNMOUNT;
    void *buffer = malloc(sizeof(char) + sizeof(int));

    memcpy(buffer, &op_code, sizeof(char));
    memcpy(buffer + sizeof(char), &session_id, sizeof(int));

    if(write(tx, buffer, sizeof(char) + sizeof(int)) == -1){
        fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
        return -1;
    }
    
    if(close(tx) == -1){
        fprintf(stderr, "[ERR]: close failed: %s\n", strerror(errno));
        return -1;
    }

    if(close(rx) == -1){
        fprintf(stderr, "[ERR]: close failed: %s\n", strerror(errno));
        return -1;
    }

    free(buffer);

    return 0;
}

int tfs_open(char const *name, int flags) {
    char const op_code = TFS_OP_CODE_OPEN;
    void *buffer = malloc(sizeof(char) * 41 + 2 * sizeof(int)); 

    memcpy(buffer, &op_code, sizeof(char));
    memcpy(buffer + sizeof(char), &session_id, sizeof(int));
    memcpy(buffer + sizeof(char) + sizeof(int), name, sizeof(char) * 40);
    memcpy(buffer + sizeof(char) * 41 + sizeof(int), &flags, sizeof(int));

    if(write(tx, buffer, 2 * sizeof(int) + 41 * sizeof(char)) == -1){
        fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
        return -1;
    }

    int ret = -1;

    if(read(rx, &ret, sizeof(int)) == -1){
        fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
        return -1;
    }

    free(buffer);

    return ret;
}

int tfs_close(int fhandle) {
    char const op_code = TFS_OP_CODE_CLOSE;
    void *buffer = malloc(sizeof(char) + 2 * sizeof(int)); 

    memcpy(buffer, &op_code, sizeof(char));
    memcpy(buffer + sizeof(char), &session_id, sizeof(int));
    memcpy(buffer + sizeof(char) + sizeof(int), &fhandle, sizeof(int));

    if(write(tx, buffer, 2 * sizeof(int) + sizeof(char)) == -1){
        fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
        return -1;
    }

    int ret = -1;

    if(read(rx, &ret, sizeof(int)) == -1){
        fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
        return -1;
    }

    free(buffer);

    return ret;
}

ssize_t tfs_write(int fhandle, void const *buffer, size_t len) {
    char const op_code = TFS_OP_CODE_WRITE;
    void *initial_buffer = malloc(sizeof(char) + sizeof(size_t));

    memcpy(initial_buffer, &op_code, sizeof(char));
    memcpy(initial_buffer + sizeof(char), &len, sizeof(size_t));

    if(write(tx, initial_buffer, sizeof(char) + sizeof(size_t)) == -1){
        fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
        return -1;
    }

    void *local_buffer = malloc(sizeof(char) * len + 2 * sizeof(int)); 
    memcpy(local_buffer, &session_id, sizeof(int));
    memcpy(local_buffer + sizeof(int), &fhandle, sizeof(int));
    memcpy(local_buffer + sizeof(int) * 2, buffer, sizeof(char) * len);

    if(write(tx, local_buffer, sizeof(char) * len + 2 * sizeof(int)) == -1){
        fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
        return -1;
    }

    int ret = -1;

    if(read(rx, &ret, sizeof(int)) == -1){
        fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
        return -1;
    }

    free(local_buffer);

    return ret;
}

ssize_t tfs_read(int fhandle, void *buffer, size_t len) {
    char const op_code = TFS_OP_CODE_READ;
    void *local_buffer = malloc(sizeof(char) + 2 * sizeof(int) + sizeof(size_t)); 

    memcpy(local_buffer, &op_code, sizeof(char));
    memcpy(local_buffer + sizeof(char), &session_id, sizeof(int));
    memcpy(local_buffer + sizeof(char) + sizeof(int), &fhandle, sizeof(int));
    memcpy(local_buffer + sizeof(char) + 2 * sizeof(int), &len, sizeof(size_t));

    if(write(tx, local_buffer, sizeof(char) + 2 * sizeof(int) + sizeof(size_t)) == -1){
        fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
        return -1;
    }

    int num_bytes = 0;

    if(read(rx, &num_bytes, sizeof(int)) == -1){
        fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
        return -1;
    }

    void *ret = malloc(sizeof(char) * (size_t)num_bytes);

    if(read(rx, ret, sizeof(char) * sizeof(size_t)) == -1){
        fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
        return -1;
    }

    memcpy(buffer, ret, sizeof(char) * (size_t)num_bytes);

    free(local_buffer);
    free(ret);

    return num_bytes;
}

int tfs_shutdown_after_all_closed() {
    char const op_code = TFS_OP_CODE_SHUTDOWN_AFTER_ALL_CLOSED;
    void *buffer = malloc(sizeof(int) + sizeof(char)); 

    memcpy(buffer, &op_code, sizeof(char));
    memcpy(buffer + sizeof(char), &session_id, sizeof(int));

    if(write(tx, buffer, sizeof(int) + sizeof(char)) == -1){
        fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
        return -1;
    }

    int ret = -1;

    if(read(rx, &ret, sizeof(int)) == -1){
        fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
        return -1;
    }

    free(buffer);

    return ret;
}
