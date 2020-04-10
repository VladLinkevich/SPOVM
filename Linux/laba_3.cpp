#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstring>
#include <string.h>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <limits>


const int kKeyIdSem = 40;
const int kKeyIdShMem = 41;
const int kSize = 100;
const int kSemaphoreAmount = 4;
const int kServerSemaphoreIndex = 0;
const int kClientSemaphoreIndex = 1;
const int kKillSemaphoreIndex = 2;
const int kClientErrorSemaphoreIndex = 3;
const short kSetArray[kSemaphoreAmount] = { 0 };
const char kInitialPath[] = "/dev/null";


int createSemaphoreSet(key_t key)
{
    int id;
    int check = 0;

    id = semget(key, kSemaphoreAmount, IPC_CREAT | SHM_R | SHM_W);
    if (id != -1)
    {
        check = semctl(id, 0, SETALL,
                       const_cast<short*>(kSetArray)); // SETALL ignores
                                                       // semnum (second) argument
    }

    return (check == -1) ? check : id;                 //
}

void deleteSemaphoreSet(int semid)
{
    semctl(semid, 0, IPC_RMID, NULL);
}

void* mapSharedMemory(int shmId)
{
    void* memoryAddress;

    memoryAddress = shmat(shmId, NULL, 0);

    return memoryAddress;
}

using namespace std;

int main()
{
    pid_t pid;
    int clientStatus;
    int semaphoreId, shMemoryId;
    key_t semaphoreKey, shMemoryKey;
    void *shMemoryAddress;
    struct sembuf semaphoreSet;
    struct shmid_ds shMemoryStruct;

    semaphoreKey = ftok(kInitialPath, kKeyIdSem);
    if (semaphoreKey == (key_t)-1)
    {
        cerr << "Error: " << strerror(errno) << endl;
        exit(EXIT_FAILURE);
    }
    shMemoryKey = ftok(kInitialPath, kKeyIdShMem);
    if (shMemoryKey == (key_t)-1)
    {
        cerr << "Error: " << strerror(errno) << endl;
        exit(EXIT_FAILURE);
    }

    semaphoreId = createSemaphoreSet(semaphoreKey);
    if (semaphoreId == -1)
    {
        cerr << "Error: " << strerror(errno) << endl;
        exit(EXIT_FAILURE);
    }
    shMemoryId = shmget(shMemoryKey, kSize, IPC_CREAT | SHM_R | SHM_W);
    if (shMemoryId == -1)
    {
        cerr << "Error: " << strerror(errno) << endl;

        deleteSemaphoreSet(semaphoreId);

        exit(EXIT_FAILURE);
    }

    shMemoryAddress = (char* )mapSharedMemory(shMemoryId);
    if (shMemoryAddress == NULL)
    {
        cerr << "Error: " << strerror(errno) << endl;

        deleteSemaphoreSet(semaphoreId);
        shmctl(shMemoryId, IPC_RMID, &shMemoryStruct);

        exit(EXIT_FAILURE);
    }

    pid = fork();
    switch(pid)
    {
    case -1:
    {
        cerr << "Error: " << strerror(errno) << endl;

        deleteSemaphoreSet(semaphoreId);
        shmdt(shMemoryAddress);
        shmctl(shMemoryId, IPC_RMID, &shMemoryStruct);

        exit(EXIT_FAILURE);
    }
    case 0:
    {
        while(true)
        {
            // ждем ввода на сервер
            semaphoreSet.sem_num = kServerSemaphoreIndex;
            semaphoreSet.sem_op  = -1;
            semaphoreSet.sem_flg = SEM_UNDO;
            semop(semaphoreId, &semaphoreSet, 1);

            // если вызвали kill-команду
            if(semctl(semaphoreId, kKillSemaphoreIndex, GETVAL) == 1)
            {
                shmdt(shMemoryAddress);
                break;
            }

            shMemoryAddress = (char* )mapSharedMemory(shMemoryId);
            if (shMemoryAddress == NULL)
            {
                cerr << "Error: " << strerror(errno) << endl;

                // установка семофора ошибок
                semaphoreSet.sem_num = kClientErrorSemaphoreIndex;
                semaphoreSet.sem_op  = 1;
                semaphoreSet.sem_flg = SEM_UNDO;
                semop(semaphoreId, &semaphoreSet, 1);

                // говорим что клинет закончил работу
                semaphoreSet.sem_num = kClientSemaphoreIndex;
                semaphoreSet.sem_op  = 1;
                semaphoreSet.sem_flg = SEM_UNDO;
                semop(semaphoreId, &semaphoreSet, 1);

                exit(EXIT_FAILURE);
            }

            cout << "Client got: " << (char*)shMemoryAddress << endl;

            // говорим что клиент прочитал из памяти
            semaphoreSet.sem_num = kClientSemaphoreIndex;
            semaphoreSet.sem_op  = 1;
            semaphoreSet.sem_flg = SEM_UNDO;
            semop(semaphoreId, &semaphoreSet, 1);
        }
        exit(EXIT_SUCCESS);
    }
    default:
    {

        string buffStr;


        while(true)
        {
            memset(shMemoryAddress, '\0', 1);


            cout << "Server: ";
            getline(cin, buffStr);




            strcpy((char*)shMemoryAddress, const_cast<char*>(buffStr.c_str()));

            // говорим что сервер принял данные
            semaphoreSet.sem_num = kServerSemaphoreIndex;
            semaphoreSet.sem_op  = 1;
            semaphoreSet.sem_flg = SEM_UNDO;
            semop(semaphoreId, &semaphoreSet, 1);

            // ждем пока клиент прочтет информацию
            semaphoreSet.sem_num = kClientSemaphoreIndex;
            semaphoreSet.sem_op  = -1;
            semaphoreSet.sem_flg = SEM_UNDO;
            semop(semaphoreId, &semaphoreSet, 1);

            // смотрим небыло ли ошибки в клиенте
            if(semctl(semaphoreId, kClientErrorSemaphoreIndex, GETVAL) > 0)
            {
                break;
            }

            // хотим выйти из приложения
            if(buffStr.empty())

                {
                    semaphoreSet.sem_num = kKillSemaphoreIndex;
                    semaphoreSet.sem_op  = 1;
                    semaphoreSet.sem_flg = SEM_UNDO;
                    semop(semaphoreId, &semaphoreSet, 1);

                    semaphoreSet.sem_num =kServerSemaphoreIndex;
                    semaphoreSet.sem_op  = 1;
                    semaphoreSet.sem_flg = SEM_UNDO;
                    semop(semaphoreId, &semaphoreSet, 1);

                    waitpid(pid, &clientStatus, 0);
                    if (WIFEXITED(clientStatus))
                    {
                        cout << "Client has exited with status value: " << WEXITSTATUS(clientStatus) << endl;
                    }
                    break;



            }
        }
    }
    }

    deleteSemaphoreSet(semaphoreId);
    shmdt(shMemoryAddress);
    shmctl(shMemoryId, IPC_RMID, &shMemoryStruct);

    return 0;
}