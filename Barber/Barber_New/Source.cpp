#include <iostream>
#include <windows.h>
#include <time.h>
#include <queue>
#include <conio.h>

int ClientsCount;
int HairCutCount;
int BarbersCount;
int SeatsCount;

std::queue <int> ClientsTable;
std::queue <int> BarbersTable;
HANDLE CheckClientsTable;
HANDLE CheckBarbersTable;
HANDLE WorkRoomSemaphore;
HANDLE NotifyClientReady;
HANDLE NotifyClientGone;
HANDLE* NotifyHaircutReady;

DWORD WINAPI ClientsThread(LPVOID lpParam) 
{
	int ClientNumber = (int)lpParam;
	srand(ClientNumber);
	for (int i = 0; i < HairCutCount;) 
	{
		WaitForSingleObject(CheckClientsTable, INFINITE);
		if (ClientsTable.empty())
		{
			ReleaseMutex(CheckClientsTable);
			printf("There are no free seats in the BarberShop, the client number %d left \n", ClientNumber);
			Sleep(1000*(rand() % 5 + 1));
			continue;
		}
		int Ticket = ClientsTable.front();
		ClientsTable.pop();
		ReleaseMutex(CheckClientsTable);
		printf("The client number %d took ticket number %d \n", ClientNumber, Ticket);
		WaitForSingleObject(WorkRoomSemaphore, INFINITE);
		WaitForSingleObject(CheckBarbersTable, INFINITE);
		BarbersTable.push(Ticket);
		ReleaseMutex(CheckBarbersTable);
		printf("The client number %d sits in a chair and wait \n", ClientNumber);
		ReleaseSemaphore(NotifyClientReady, 1, NULL);
		WaitForSingleObject(NotifyHaircutReady[Ticket], INFINITE);
		ReleaseSemaphore(NotifyClientGone, 1, NULL);
		printf("The client number %d gets a haircut and leaves \n", ClientNumber);
		Sleep(1000 * (rand() % 5 + 1));
		i++;
	}
	return 0;
}

DWORD WINAPI BarbersThread(LPVOID lpParam) 
{
	int BarberNumber = (int)lpParam;
	srand(BarberNumber);
	while (true)
	{
		WaitForSingleObject(NotifyClientReady, INFINITE);
		WaitForSingleObject(CheckBarbersTable, INFINITE);
		int Ticket = BarbersTable.front();
		BarbersTable.pop();
		ReleaseMutex(CheckBarbersTable);
		printf("Barber number %d began to cut the client with the ticket number %d \n", BarberNumber, Ticket);
		Sleep(1000 * (rand() % 3 + 1));
		ReleaseSemaphore(NotifyHaircutReady[Ticket], 1, NULL);
		WaitForSingleObject(NotifyClientGone, INFINITE);
		WaitForSingleObject(CheckClientsTable, INFINITE);
		ClientsTable.push(Ticket);
		ReleaseMutex(CheckClientsTable);
		printf("The Barber number %d is ready to accept another client \n", BarberNumber);
		ReleaseSemaphore(WorkRoomSemaphore, 1, NULL);
	}
	return 0;

}

int main() 
{
	std::cin >> ClientsCount >> HairCutCount >> BarbersCount >> SeatsCount;

	NotifyHaircutReady = new HANDLE[BarbersCount + SeatsCount];
	HANDLE* CThreads = new HANDLE[ClientsCount];
	HANDLE* BThreads = new HANDLE[BarbersCount];

	CheckClientsTable = CreateMutex(NULL, FALSE, NULL);
	CheckBarbersTable = CreateMutex(NULL, FALSE, NULL);
	WorkRoomSemaphore = CreateSemaphore(NULL, BarbersCount, BarbersCount, NULL);
	NotifyClientReady = CreateSemaphore(NULL, 0, BarbersCount, NULL);
	NotifyClientGone = CreateSemaphore(NULL, 0, BarbersCount, NULL);

	for (int i = 0; i < SeatsCount + BarbersCount; i++)
	{
		ClientsTable.push(i);
		NotifyHaircutReady[i] = CreateSemaphore(NULL, 0, 1, NULL);
	}

	for (int i = 0; i < ClientsCount; i++)
		CThreads[i] = CreateThread(NULL, 0, ClientsThread, (LPVOID)i, 0, 0);


	for (int i = 0; i < BarbersCount; i++)
		BThreads[i] = CreateThread(NULL, 0, BarbersThread, (LPVOID)i, 0, 0);


	WaitForMultipleObjects(ClientsCount, CThreads, TRUE, INFINITE);
	for (int i = 0; i < ClientsCount; i++)
		CloseHandle(CThreads[i]);

	//«акрытие дескриптора потока не завершает работу св€занного потока.
	//–абота потока завершаетс€, когда завершаетс€ процесс содержащий поток
	//for (int i = 0; i < BarbersCount; i++)
	//	CloseHandle(BThreads[i]);

	for (int i = 0; i < BarbersCount + SeatsCount; i++)
		CloseHandle(NotifyHaircutReady[i]);

	CloseHandle(CheckClientsTable);
	CloseHandle(CheckBarbersTable);
	CloseHandle(WorkRoomSemaphore);
	CloseHandle(NotifyClientReady);
	CloseHandle(NotifyClientGone);

	delete[](NotifyHaircutReady);
	delete[](CThreads);
	delete[](BThreads);

	_getch();
}