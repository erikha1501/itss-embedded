#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <time.h>
// should be in a global config file
#define SHMSIZE 9
#define ERROR_VALUE -1
#define SUCCESS_VALUE 1
#define SHM_PATHNAME "o"
#define SHM_PROJ_ID 'k'
#define SALES_HISTORY "database/salesHistory.txt"
#define INVENTORY_INFO "database/inventoryInfo.txt"

int readFromDb = 0;

char commondityCode[3][10] = {"Fanta", "Pepsi", "7-Up"};

int* returnValue = NULL;

int* equipInfoAccess(int opt, int changedCommondity, int inventoryID)
{
    // free(returnValue);
    returnValue = (int*)malloc(sizeof(int*));
    returnValue[0] = ERROR_VALUE;
    switch (opt)
    {
    case 0: {
        int shmID;
        key_t keySHM = ftok(SHM_PATHNAME, (int)SHM_PROJ_ID);

        // Get the shared memory ID
        if ((shmID = shmget(keySHM, SHMSIZE * sizeof(int), IPC_CREAT | 0660)) == -1)
        {
            printf("Error: shmget.\n");
            return returnValue;
        }

        // Attach the shared memory
        int* shmPointer = (int*)shmat(shmID, NULL, 0);
        if (shmPointer == (int*)-1)
        {
            printf("Error: shmat.\n");
            return returnValue;
        }
        if (readFromDb == 0)
        {
            char dbLine[20] = "\0";
            char dbChar;
            FILE* fptr;
            if ((fptr = fopen(INVENTORY_INFO, "r")) == NULL)
            {
                printf("Error: Cannot open %s.\n", INVENTORY_INFO);
                // Program exits if the file pointer returns NULL.
                return returnValue;
            }
            returnValue = shmPointer;
            int charIndexInline = 0;
            while ((dbChar = fgetc(fptr)) != EOF)
            {
                if (dbChar != '\n')
                    dbLine[charIndexInline++] = dbChar;
                else
                {
                    char* numberToken = strtok(dbLine, ";");
                    numberToken = strtok(NULL, ";");
                    numberToken = strtok(NULL, ";");
                    *(shmPointer++) = atoi(numberToken);
                    charIndexInline = 0;
                    memset(dbLine, '\0', 20);
                }
            }
            printf("Success: Done writing from inventory information to shared mem.\n");
            readFromDb = 1;
            return returnValue;
        }
        else
        {
            returnValue = shmPointer;
            return returnValue;
        }
    }
    break;
    case 1: {
        if (inventoryID >= SHMSIZE)
        {
            printf("Error: Invalid Inventory ID -- Stock unchanged.\n");
            return returnValue;
        }

        if (changedCommondity == 0)
        {
            printf("Error: Zero Commondity Changed -- Stock unchanged.\n");
            return returnValue;
        }

        int shmID;
        key_t keySHM = ftok(SHM_PATHNAME, (int)SHM_PROJ_ID);

        // Get the shared memory ID
        if ((shmID = shmget(keySHM, SHMSIZE * sizeof(int), IPC_CREAT | 0660)) == -1)
        {
            printf("Error: shmget.\n");
            return returnValue;
        }

        // Attach the shared memory
        int* shmPointer = (int*)shmat(shmID, NULL, 0);
        if (shmPointer == (int*)-1)
        {
            printf("Error: shmat.\n");
            return returnValue;
        }

        printf("Demand: %s stock of vending machine %d needs to be %s by %d.\n", commondityCode[(int)(inventoryID % 3)],
               (int)floor(inventoryID / 3) + 1, (changedCommondity > 0 ? "decreased" : "increased"),
               changedCommondity > 0 ? changedCommondity : -changedCommondity);
        // Process the data in shared mem
        if (*(shmPointer += inventoryID) < changedCommondity)
        {
            printf("Error: Stock is less than demand -- Stock unchanged.\n");
            return returnValue;
        }
        else
        {
            *(shmPointer) -= changedCommondity;
            printf("Success: Stock is changed due to demand.\n");

            if (changedCommondity > 0)
            {
                FILE* fptr = fopen(SALES_HISTORY, "a");
                if (fptr == NULL)
                {
                    printf("Error: Cannot open %s.\n", SALES_HISTORY);
                    return returnValue;
                }
                time_t t = time(NULL);
                char* ptrTime = asctime(localtime(&t));
                char* saleTime = ptrTime;
                while (*ptrTime != '\0')
                {
                    if (*ptrTime == ' ')
                    {
                        *ptrTime = '-';
                    }
                    else if (*ptrTime == '\n')
                    {
                        *ptrTime = ' ';
                    }
                    ptrTime++;
                }
                fprintf(fptr, "Time: %sVending Machine: %d Sale: %d %s\n", saleTime, (int)floor(inventoryID / 3) + 1,
                        changedCommondity, commondityCode[(int)(inventoryID % 3)]);
                fclose(fptr);
                printf("Success: Done writing change to sales history.\n");
            }
            returnValue[0] = SUCCESS_VALUE;
            return returnValue;
        }
    }
    break;
    case -1: {
        int shmID;
        key_t keySHM = ftok(SHM_PATHNAME, (int)SHM_PROJ_ID);

        // Get the shared memory ID
        if ((shmID = shmget(keySHM, SHMSIZE * sizeof(int), IPC_CREAT | 0660)) == -1)
        {
            printf("Error: shmget.\n");
            return returnValue;
        }

        // Attach the shared memory
        int* shmPointer = (int*)shmat(shmID, NULL, 0);
        if (shmPointer == (int*)-1)
        {
            printf("Error: shmat.\n");
            return returnValue;
        }
        int* shmPointerHead = shmPointer;

        // Write to the file
        FILE* fptr = fopen(INVENTORY_INFO, "w");
        if (fptr == NULL)
        {
            printf("Error: Cannot open %s.\n", INVENTORY_INFO);
            return returnValue;
        }

        for (int i = 0; i < SHMSIZE; i++)
        {
            fprintf(fptr, "%d;%s;%d\n", (int)floor(i / 3) + 1, commondityCode[(int)(i % 3)], *(shmPointer + i));
        }

        fclose(fptr);
        printf("Success: Done writing from shared mem to inventory information.\n");

        // Detach the shared memory
        if (shmdt((void*)shmPointerHead) == -1)
        {
            printf("Error: shmdt.\n");
            return returnValue;
        }

        // Delete the shared memory
        if (shmctl(shmID, IPC_RMID, 0) == -1)
        {
            printf("Error: shmctl.\n");
            return returnValue;
        }
        readFromDb = 0;
        returnValue[0] = SUCCESS_VALUE;
        return returnValue;
    }
    break;
    default: {
        printf("Error: Invalid option.\n");
        return returnValue;
    }
    break;
    }
}