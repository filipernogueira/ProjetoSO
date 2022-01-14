#include "../fs/operations.h"
#include <assert.h>
#include <string.h>
#include <pthread.h>
#define THREAD_QUANTITY 20
#define LENGHT 128


typedef struct {
    int fhandle;
    void const *buffer;
    size_t len;
} tfs_open_paramts;


int main() {

    pthread_t tid[THREAD_QUANTITY];

    tfs_open_paramts input[THREAD_QUANTITY];

    char buffer[LENGHT];
    memset(input, 'A', LENGHT);


    assert(tfs_init() != -1);

    int fhandle = tfs_open("/f1", TFS_O_CREAT);

    for (int i = 0; i < THREAD_QUANTITY; i++) {
        input[i].fhandle = fhandle;
        input[i].buffer = buffer;
        input[i].len = LENGHT;
    }

    
    for (int i =0; i < THREAD_QUANTITY; i++) {
        if (pthread_create(&tid[i], NULL, (void*)tfs_write, (void*)&input[i]) == 0) {
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

    assert(tfs_destroy() != -1);

    return 0;
}