// ServerHyperion.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include <iostream>
#include <string>
#include "ServerHyperion.h"

using namespace std;

const UINT16 SERVER_PORT = 11021;
const UINT16 MAX_CLIENT = 3;		//총 접속할수 있는 클라이언트 수
const UINT32 MAX_IO_WORKER_THREAD = 4;  //쓰레드 풀에 넣을 쓰레드 수

/*
int main()
{
	ServerHyperion server;

	// 적절한 작업 스레드의 갯수는 (CPU * 2) + 1
	server.Init(thread::hardware_concurrency() * 2 + 1);
	server.BindandListen(SERVER_PORT);

	server.Run(MAX_CLIENT);

	printf("아무 키나 누를 때까지 대기합니다\n");
	while (true)
	{
		std::string inputCmd;
		std::getline(std::cin, inputCmd);

		if (inputCmd == "quit")
		{
			break;
		}
	}

	server.End();
}
*/

int main()
{
	ServerHyperion server;

	//소켓을 초기화
	server.Init(MAX_IO_WORKER_THREAD);

	//소켓과 서버 주소를 연결하고 등록 시킨다.
	server.BindandListen(SERVER_PORT);

	server.Run(MAX_CLIENT);

	printf("아무 키나 누를 때까지 대기합니다\n");
	while (true)
	{
		std::string inputCmd;
		std::getline(std::cin, inputCmd);

		if (inputCmd == "quit")
		{
			break;
		}
	}

	server.End();
	return 0;
}
