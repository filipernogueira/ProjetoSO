#include "../fs/operations.h"
#include <assert.h>
#include <pthread.h>
#define THREAD_QUANTITY 20


typedef struct {
    char const *path;
    int flag;
    int fhandle; 
} tfs_open_paramts;

void *tfs_open_api(void* arg){
    tfs_open_paramts *paramts = (tfs_open_paramts *)arg;
    paramts->fhandle = tfs_open(paramts->path, paramts->flag);
    assert(paramts->fhandle != -1);

    return NULL;
}


int main() {

    pthread_t tid[THREAD_QUANTITY];

    tfs_open_paramts input[THREAD_QUANTITY];

    input[0].path = "/f1";
    input[1].path = "/f2";
    input[2].path = "/f3";
    input[3].path = "/f4";
    input[4].path = "/f5";
    input[5].path = "/f6";
    input[6].path = "/f7";
    input[7].path = "/f8";
    input[8].path = "/f9";
    input[9].path = "/f10";
    input[10].path = "/f11";
    input[11].path = "/f12";
    input[12].path = "/f13";
    input[13].path = "/f14";
    input[14].path = "/f15";
    input[15].path = "/f16";
    input[16].path = "/f17";
    input[17].path = "/f18";
    input[18].path = "/f19";
    input[19].path = "/f20";

    for (int i = 0; i < THREAD_QUANTITY; i++) {
        input[i].flag = TFS_O_CREAT;
    }

    assert(tfs_init() != -1);
    
    for (int i =0; i < THREAD_QUANTITY; i++) {
        if (pthread_create(&tid[i], NULL, tfs_open_api, (void*)&input[i]) == 0) {
            printf("Thread %d criada com sucesso!\n", i + 1);
        }
        else {
            printf("Erro na criação da tarefa\n");
            exit(1);
        }
    }

    for (int i = 0; i < THREAD_QUANTITY; i++) {
        pthread_join(tid[i], NULL);
        assert(tfs_close(input[i].fhandle) != -1);
    }

    printf("Sucessful test\n");

    assert(tfs_destroy() != -1);

    return 0;
}