#include "../fs/operations.h"
#include <assert.h>
#include <string.h>
#include <pthread.h>
#define THREAD_QUANTITY 20
#define LENGTH 128
#define COUNT 20


typedef struct {
    int fhandle;
    void const *buffer;
    size_t len;
} tfs_read_paramts;

void *tfs_read_api(void* arg){
    tfs_read_paramts *paramts = (tfs_read_paramts *)arg;
    assert(tfs_write(paramts->fhandle, paramts->buffer, paramts->len) == paramts->len);

    return NULL;
}

int main() {

    pthread_t tid[THREAD_QUANTITY];

    tfs_read_paramts input[THREAD_QUANTITY];

    char buffer[LENGTH];

    assert(tfs_init() != -1);

    /* Write input COUNT times into a new file */
    int fhandle = tfs_open("/f1", TFS_O_CREAT);
    assert(fhandle != -1);

    for (int i = 0; i < THREAD_QUANTITY; i++) {
        input[i].fhandle = fhandle;
        input[i].buffer = buffer;
        input[i].len = LENGTH;
    }

    for (int i = 0; i < COUNT; i++) {
        assert(tfs_write(fhandle, input, LENGTH) == LENGTH);
    }
    assert(tfs_close(fhandle) != -1);

    /* Open again to check if contents are as expected */
    fhandle = tfs_open("/f1", 0);
    assert(fhandle != -1 );

    
    for (int i =0; i < THREAD_QUANTITY; i++) {
        if (pthread_create(&tid[i], NULL, tfs_read_api, (void*)&input[i]) == 0) {
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

    printf("Sucessful test\n");

    assert(tfs_destroy() != -1);

    return 0;
}