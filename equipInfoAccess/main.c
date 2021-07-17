#include "equipInfoAccess.h"

int main()
{
    int* res;

    ///////////////opt = 0////////////////
    res = equipInfoAccess(0, 0, 0);
    printf("Result: return value(opt 0): ");
    for (int i = 0; i < SHMSIZE; i++)
    {
        printf("%d ", res[i]);
    }
    printf("\n");
    ///////////////////////////////////////////

    ///////////////opt = 1/////////////////
    /// change the param of next line to test//
    res = equipInfoAccess(1, 5, 5);
    printf("Result: return value(opt 1): %s\n", (res[0] == 1 ? "SUCCESS" : "ERROR"));

    int shmID;
    key_t keySHM = ftok(SHM_PATHNAME, (int)SHM_PROJ_ID);
    if ((shmID = shmget(keySHM, SHMSIZE * sizeof(int), IPC_CREAT | 0660)) == -1)
    {
        perror("shmget");
    }
    int* shmPointer = (int*)shmat(shmID, NULL, 0);
    if (shmPointer == (int*)-1)
    {
        perror("shmat");
    }

    printf("Result: current stock value in shared mem: ");
    for (int i = 0; i < SHMSIZE; i++)
    {
        printf("%d ", *(shmPointer + i));
    }
    printf("\n");
    ///////////////////////////////////////////

    ///////////////opt = 0////////////////
    res = equipInfoAccess(0, 0, 0);
    printf("Result: return value(opt 0): ");
    for (int i = 0; i < SHMSIZE; i++)
    {
        printf("%d ", res[i]);
    }
    printf("\n");
    ///////////////////////////////////////////

    /////////////opt = -1////////////////////
    res = equipInfoAccess(-1, 0, 0);
    printf("Result: return value(opt -1): %s\n", (res[0] == 1 ? "SUCCESS" : "ERROR"));
    /////////////////////////////////////////
}
