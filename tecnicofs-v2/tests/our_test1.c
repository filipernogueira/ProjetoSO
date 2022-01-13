#include "../fs/operations.h"
#include <assert.h>
#include <string.h>
#include <pthread.h>
#define COUNT 109 // a partir de = 109 ele fica em loop infinito (é quando ultrapassa o tamamnho máximo 272384)
                  // este ciclo infinito ocorre pelo menos no write...
#define SIZE 2500

typedef struct {
    char const *pth;
    int flg;
    int rtn_value;
} tfs_open_paramts;

int main() {

    pthread_t tid[20];

    tfs_open_paramts input[20];

    input[0].pth = "/f1";
    input[0].flg = TFS_O_CREAT;
    input[1].pth = "/f2";
    input[1].flg = TFS_O_CREAT;
    input[2].pth = "/f3";
    input[2].flg = TFS_O_CREAT;
    input[3].pth = "/f4";
    input[3].flg = TFS_O_CREAT;
    input[4].pth = "/f5";
    input[4].flg = TFS_O_CREAT;
    input[5].pth = "/f6";
    input[5].flg = TFS_O_CREAT;
    input[6].pth = "/f7";
    input[6].flg = TFS_O_CREAT;
    input[7].pth = "/f8";
    input[7].flg = TFS_O_CREAT;
    input[8].pth = "/f9";
    input[8].flg = TFS_O_CREAT;
    input[9].pth = "/f10";
    input[9].flg = TFS_O_CREAT;
    input[10].pth = "/f11";
    input[10].flg = TFS_O_CREAT;
    input[11].pth = "/f12";
    input[11].flg = TFS_O_CREAT;
    input[12].pth = "/f13";
    input[12].flg = TFS_O_CREAT;
    input[13].pth = "/f14";
    input[13].flg = TFS_O_CREAT;
    input[14].pth = "/f15";
    input[14].flg = TFS_O_CREAT;
    input[15].pth = "/f16";
    input[15].flg = TFS_O_CREAT;
    input[16].pth = "/f17";
    input[16].flg = TFS_O_CREAT;
    input[17].pth = "/f18";
    input[17].flg = TFS_O_CREAT;
    input[18].pth = "/f19";
    input[18].flg = TFS_O_CREAT;
    input[19].pth = "/f20";
    input[19].flg = TFS_O_CREAT;

    
    assert(tfs_init() != -1);
    
    for (int i =0; i <20; i++) {
        pthread_create(&tid[i], NULL, (void*)tfs_open, (void *)&input[i]); // FAZER UM IF != 0
    }

    for (int i =0; i <20; i++) {
        pthread_join(tid[i], NULL);    
    }

    assert(tfs_destroy() != -1);

    return 0;
}