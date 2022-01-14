#include "../fs/operations.h"
#include <assert.h>
#include <string.h>
#include <pthread.h>
#define THREAD_QUANTITY 20
#define LENGTH 128


typedef struct {
    int fhandle;
    void const *buffer;
    size_t len;
} tfs_write_paramts;

void *tfs_write_api(void* arg){
    tfs_write_paramts *paramts = (tfs_write_paramts *)arg;
    assert(tfs_write(paramts->fhandle, paramts->buffer, paramts->len) == paramts->len);

    return NULL;
}


int main() {

    pthread_t tid[THREAD_QUANTITY];

    tfs_write_paramts input;

    char buffer[LENGTH];
    memset(buffer, 'A', LENGTH);

    char output[LENGTH];


    assert(tfs_init() != -1);

    int fhandle = tfs_open("/f1", TFS_O_CREAT);

    input.fhandle = fhandle;
    input.buffer = buffer;
    input.len = LENGTH;

    
    for (int i =0; i < THREAD_QUANTITY; i++) {
        if (pthread_create(&tid[i], NULL, tfs_write_api, (void*)&input) == 0) {
            printf("Thread %d criada com sucesso!\n", i + 1);
        }
        else {
            printf("Erro na criação da tarefa\n");
            exit(1);
        }
    }

    for (int i = 0; i < THREAD_QUANTITY; i++) {
        pthread_join(tid[i], NULL);    
    }

    assert(tfs_close(fhandle) != -1);

    fhandle = tfs_open("/f1", 0);
    assert(fhandle != -1 );

    for (int i = 0; i < THREAD_QUANTITY; i++) {
        assert(tfs_read(fhandle, output, LENGTH) == LENGTH);
        assert(memcmp(buffer, output, LENGTH) == 0);
    }

    assert(tfs_close(fhandle) != -1);


    printf("Sucessful test\n");

    assert(tfs_destroy() != -1);

    return 0;
}