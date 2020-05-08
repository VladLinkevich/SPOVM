#include <iostream>
#include <pthread.h>
#include <list>
#include <csignal>
#include <unistd.h>
#include <stdlib.h>
using namespace std;

// g++ main.cpp -o main -lpthread
// ./main

pthread_mutex_t mp;

void term_handler(int i){
    pthread_exit(NULL);
}

void* newthread(void* arg){
    // устанавливаю обработчик для SIGTERM
    struct sigaction sa;
    sa.sa_handler = term_handler;
    sigaction(SIGTERM, &sa, 0);


    while (true)
    {
        pthread_mutex_lock(&mp);
        cout << "Thread" << pthread_self() << endl;
        pthread_mutex_unlock(&mp);
        sleep(1);
    }
}

int main(int argc, char *argv[])
{
    list<pthread_t> threadIDlist;
    if(pthread_mutex_init(&mp, NULL)!=0){
        cout << "Mutex create error" << endl;
    }
    while(true) {

        pthread_mutex_lock(&mp);
        char symbol;
        cin.get(symbol);                                                               // сичтать символ


        switch(symbol) {

            case '+': {                                                                   // создание нового потока

                pthread_t id;
                if(pthread_create(&id,NULL, newthread, NULL)!=0){
                    cout << "Thread create error" << endl;
                    break;
                }

                threadIDlist.push_back(id);                                               // добавить ID потока в список потоков
                sleep(1);

            }break;


            case '-': {                                                                   //удалить процесс
                if (!threadIDlist.empty()) {

                    pthread_kill(threadIDlist.back(), SIGTERM);                           // отправляем процесс на завершиние
                    sleep(1);

                    threadIDlist.pop_back();                                              // удалить PID процесса из листа процессов
                } else {
                    cout <<  "List is empty." << endl;
                }
            } break;

            case 'q':                                                                     // выйти удалив все процессы
                if(!threadIDlist.empty())
                {
                    for(pthread_t &childID: threadIDlist) {
                        pthread_kill(childID, SIGTERM);                                   // отправляем процесс на завершиние
                    }

                    threadIDlist.clear();                                                 // очистить список
                } exit(EXIT_SUCCESS);                                                     // завершить программу с кодом 0

        }
        cin.ignore();
        pthread_mutex_unlock(&mp);
        sleep(1);

    }

    return 0;
}
