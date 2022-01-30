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
    char buffer[BUFFER_SIZE];
    tx = open(server_pipe_path, O_WRONLY);
    if (tx == -1) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        return -1;
    }

    char const opcode = TFS_OP_CODE_MOUNT;
    memcpy(buffer, &opcode, sizeof(char));
    memcpy(buffer + 1, client_pipe_path, sizeof(char) * strlen(client_pipe_path));

    if (write(tx, buffer, BUFFER_SIZE * sizeof(char)) == -1){
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
    int const opcode = TFS_OP_CODE_UNMOUNT;
    int buffer[2];

    buffer[0] = opcode;
    buffer[1] = session_id;

    if(write(tx, buffer, 2 * sizeof(int)) == -1){
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

    return 0;
}

int tfs_open(char const *name, int flags) {
    
}

int tfs_close(int fhandle) {
    
}

ssize_t tfs_write(int fhandle, void const *buffer, size_t len) {
    
}

ssize_t tfs_read(int fhandle, void *buffer, size_t len) {
    /* TODO: Implement this */
    return -1;
}

int tfs_shutdown_after_all_closed() {
    /* TODO: Implement this */
    return -1;
}
