// ServerHyperion.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#define GAMELIFT_USE_STD

#include <iostream>
#include <string>
#include "ServerHyperion.h"
#include <aws/gamelift/server/GameLiftServerAPI.h>

using namespace std;
using namespace Aws::GameLift::Server;

const UINT16 SERVER_PORT = 11021;
const UINT16 MAX_CLIENT = 100;		//총 접속할수 있는 클라이언트 수
const UINT32 MAX_IO_WORKER_THREAD = 4;  //쓰레드 풀에 넣을 쓰레드 수

int main()
{
    auto InitOutcome = InitSDK();
    if (!InitOutcome.IsSuccess())
    {
        cout << "[InitSDK()] : " << InitOutcome.GetError().GetErrorMessage() << endl;
        return -1;
    }

    ProcessParameters HyperionProcParams
    (
        // 게임 세션 생성 시 호출됨
        [](Model::GameSession gameSession)
        {
            cout << "Game session created: " << gameSession.GetGameSessionId() << endl;
            ActivateGameSession();
        },
        // 게임 세션 종료 요청 시 호출됨
        []()
        {
            cout << "Game session ending..." << endl;
            //TerminateGameSession(); // it makes compilation error. is it version problem?
            ProcessEnding();
        },
        // HealthCheck 콜백
        []() -> bool
        {
            return true; // true면 정상, false면 GameLift가 서버를 종료시킴
        },
        SERVER_PORT,
        LogParameters()
    );

    auto ReadyOutcome = ProcessReady(HyperionProcParams);
    if (!ReadyOutcome.IsSuccess())
    {
        cout << "[ProcessReady(...)] : " << ReadyOutcome.GetError().GetErrorMessage() << endl;
        return -1;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    ServerHyperion server;

    //소켓을 초기화
    server.Init(MAX_IO_WORKER_THREAD);

    //소켓과 서버 주소를 연결하고 등록 시킨다.
    server.BindandListen(SERVER_PORT);

    server.Run(MAX_CLIENT);

    printf("아무 키나 누를 때까지 대기합니다\n");
    while (true)
    {
        string inputCmd;
        getline(cin, inputCmd);

        if (inputCmd == "quit")
        {
            break;
        }
    }

    server.End();
    ///////////////////////////////////////////////////////////////////////////////////////////////

    Destroy();

    return 0;
}
