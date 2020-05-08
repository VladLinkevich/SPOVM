#include <iostream>
#include <list>
#include <windows.h>
#include <stdlib.h>
#include <sstream>
#include <conio.h>
using namespace std;
HANDLE hMutex;



void* Thread(void* pParams) {

	
	while (true)
	{
		WaitForSingleObject(hMutex, INFINITE);						// ждем когда мьютекс освободится, чтобы заблок-ть его   

		int IdThread = GetCurrentThreadId();
		string message = "Thread ";
		stringstream ss;
		ss << IdThread;
		message += ss.str();
		
		for (int i = 0; i < message.length(); i++) {
			cout << message[i];
			Sleep(50);
		}

 		cout << endl;
		
		ReleaseMutex(hMutex);                                       // разблокировать
		
	}
}

int main(int argc, char* argv[])
{
	list<HANDLE> threadIDlist;

	hMutex = CreateMutex(NULL,										// атрибут безопасности
		FALSE,														// разблок. мьютекс
		NULL);														// имя мьютекса

	while (true) {
		WaitForSingleObject(hMutex, INFINITE);                      // ждем когда мьютекс освободится, чтобы залочить его

		char symbol;
		//cout << "Print symbol" << endl;
		cin.get(symbol);

		switch (symbol) {

			case '+': 													// создание нового потока
				HANDLE hThr;
				// создание нового потока
				if ((hThr = CreateThread(NULL,							// атрибуты безопасности потока
					0,													// размер стека, выделяемого под поток
					(LPTHREAD_START_ROUTINE)Thread,						// адрес потоковой функции. Она должна следовать прототипу DWORD WINAPI ThreadFunc(PVOID pvParam)
					NULL,												// указатель на параметры, передаваемые функции потока при его создании
					0,													// флаг создания потока (поток запускается немедленно или создается в остановленном состоянии)
					NULL))												// указатель, по которому будет записан идентификатор созданного потока
					== 0)

				{
					cout << "Thread create error" << endl;   break;
				}

				threadIDlist.push_back(hThr);							//добавить ID потока в список потоков
				Sleep(1000);
				SuspendThread(hThr);

				break;


			case '-':													// удалить поток
				if (!threadIDlist.empty()) {
					// удаляем поток
					TerminateThread(threadIDlist.back(),				// дескриптор потока
						NO_ERROR);										// код завершения потока
					CloseHandle(threadIDlist.back());
					Sleep(1000);

					threadIDlist.pop_back();							// удалить ID потока из списка
				}
				else {
					cout << "List is empty." << endl;
				}
				break;

			case 'q':													// выйти, удалив все потоки
				if (!threadIDlist.empty())
				{
					for (HANDLE& childID : threadIDlist) {

						CloseHandle(childID);							// закрывает дескриптор последнего потока
					}
					threadIDlist.clear();								// очистить лист
				}
				exit(EXIT_SUCCESS);										// завершить программу с кодом 0

		}

		for (HANDLE& childID : threadIDlist) {
			ResumeThread(childID);
			ReleaseMutex(hMutex);                                    // освободить
			Sleep(100);
			WaitForSingleObject(hMutex, INFINITE);                   // заблоикровать
			SuspendThread(childID);
		}

		cin.ignore();
		ReleaseMutex(hMutex);
		//Sleep(1000);
		//cout << endl;
	}
	return 0;
}