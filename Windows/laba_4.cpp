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
		WaitForSingleObject(hMutex, INFINITE);						// ���� ����� ������� �����������, ����� ������-�� ���   

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
		
		ReleaseMutex(hMutex);                                       // ��������������
		
	}
}

int main(int argc, char* argv[])
{
	list<HANDLE> threadIDlist;

	hMutex = CreateMutex(NULL,										// ������� ������������
		FALSE,														// �������. �������
		NULL);														// ��� ��������

	while (true) {
		WaitForSingleObject(hMutex, INFINITE);                      // ���� ����� ������� �����������, ����� �������� ���

		char symbol;
		//cout << "Print symbol" << endl;
		cin.get(symbol);

		switch (symbol) {

			case '+': 													// �������� ������ ������
				HANDLE hThr;
				// �������� ������ ������
				if ((hThr = CreateThread(NULL,							// �������� ������������ ������
					0,													// ������ �����, ����������� ��� �����
					(LPTHREAD_START_ROUTINE)Thread,						// ����� ��������� �������. ��� ������ ��������� ��������� DWORD WINAPI ThreadFunc(PVOID pvParam)
					NULL,												// ��������� �� ���������, ������������ ������� ������ ��� ��� ��������
					0,													// ���� �������� ������ (����� ����������� ���������� ��� ��������� � ������������� ���������)
					NULL))												// ���������, �� �������� ����� ������� ������������� ���������� ������
					== 0)

				{
					cout << "Thread create error" << endl;   break;
				}

				threadIDlist.push_back(hThr);							//�������� ID ������ � ������ �������
				Sleep(1000);
				SuspendThread(hThr);

				break;


			case '-':													// ������� �����
				if (!threadIDlist.empty()) {
					// ������� �����
					TerminateThread(threadIDlist.back(),				// ���������� ������
						NO_ERROR);										// ��� ���������� ������
					CloseHandle(threadIDlist.back());
					Sleep(1000);

					threadIDlist.pop_back();							// ������� ID ������ �� ������
				}
				else {
					cout << "List is empty." << endl;
				}
				break;

			case 'q':													// �����, ������ ��� ������
				if (!threadIDlist.empty())
				{
					for (HANDLE& childID : threadIDlist) {

						CloseHandle(childID);							// ��������� ���������� ���������� ������
					}
					threadIDlist.clear();								// �������� ����
				}
				exit(EXIT_SUCCESS);										// ��������� ��������� � ����� 0

		}

		for (HANDLE& childID : threadIDlist) {
			ResumeThread(childID);
			ReleaseMutex(hMutex);                                    // ����������
			Sleep(100);
			WaitForSingleObject(hMutex, INFINITE);                   // �������������
			SuspendThread(childID);
		}

		cin.ignore();
		ReleaseMutex(hMutex);
		//Sleep(1000);
		//cout << endl;
	}
	return 0;
}