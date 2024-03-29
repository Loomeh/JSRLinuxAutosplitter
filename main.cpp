#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/uio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <thread>

#include "readmem.hpp"
#include "client.hpp"

using std::string;
using std::cout;
using std::endl;
using std::string;
using std::cin;

ReadMemory readMemory;
LiveSplitClient lsClient;

string ipAddress = "";

uint32_t loading, newGame, rankingScreen, bossGraffiti;

uint32_t memLoading;
uint32_t memNewGame;
uint32_t memRankingScreen;
uint32_t memBossGraffiti;

struct iovec loadingLocal;
struct iovec loadingRemote;

struct iovec newGameLocal;
struct iovec newGameRemote;

struct iovec rankingScreenLocal;
struct iovec rankingScreenRemote;

struct iovec bossGraffitiLocal;
struct iovec bossGraffitiRemote;

struct StockPid
{
    pid_t pid;
    char buff[512];
    FILE *pid_pipe;
} stockthepid;

void Func_StockPid(const char *processtarget)
{
    stockthepid.pid_pipe = popen(processtarget, "r");
    if (!fgets(stockthepid.buff, 512, stockthepid.pid_pipe)) {
        cout << "Error reading process ID: " << strerror(errno) << endl;
    }

    stockthepid.pid = strtoul(stockthepid.buff, nullptr, 10);

    if (stockthepid.pid == 0) {
        cout << "Jet Set Radio isn't running.\n";
        pclose(stockthepid.pid_pipe);
    } else {
        cout << "Jet Set Radio is running - PID NUMBER -> " << stockthepid.pid << endl;
        pclose(stockthepid.pid_pipe);
    }
}

void readAddresses(int pid)
{
    loading = readMemory.readMem(memLoading, pid, 0x98FAAC, loadingLocal, loadingRemote);
    newGame = readMemory.readMem(memNewGame, pid, 0xB5A278, newGameLocal, newGameRemote);
    rankingScreen = readMemory.readMem(memRankingScreen, pid, 0x98FB1C, rankingScreenLocal, rankingScreenRemote);
    bossGraffiti = readMemory.readMem(memBossGraffiti, pid, 0x95D2B8, bossGraffitiLocal, bossGraffitiRemote);

}

void sendCommands(int pid)
{
    lsClient.Client(pid, ipAddress);

    uint32_t prevLoading;
    uint32_t prevNewGame;
    uint32_t prevRankingScreen;
    uint32_t prevBossGraffiti;

    while(true)
    {
        std::thread t1(readAddresses, pid);
        std::thread t2(readAddresses, pid);

        t1.join();
        t2.join();
        
        if (loading == 1 && prevLoading != 1) {
            lsClient.sendLSCommand("pausegametime\r\n");
        } else if (loading == 0 && prevLoading != 0) {
            lsClient.sendLSCommand("unpausegametime\r\n");
        }
        prevLoading = loading;

        if(newGame != 1 && prevNewGame == 1) {
            lsClient.sendLSCommand("starttimer\r\n");
        }
        prevNewGame = newGame;

        if(bossGraffiti == 7 && prevBossGraffiti != 7) {
            lsClient.sendLSCommand("split\r\n");
        } else if(rankingScreen == 1 && prevRankingScreen != 1) {
            lsClient.sendLSCommand("split\r\n");
        }
        prevRankingScreen = rankingScreen;
        prevBossGraffiti = bossGraffiti;

        sleep(0.0001); // Sleep to avoid CPU explosio
    }
}

int main(int argc, char *argv[]) {

    cout << "What is your local IP address? (LiveSplit Server settings will tell you if you don't know.)\n";
    cin >> ipAddress;

    const char *processName = "pidof jetsetradio.exe";
    while (true) {
        Func_StockPid(processName);
        if (stockthepid.pid == 0) {
            cout << "Jet Set Radio isn't running. Retrying in 5 seconds...\n";
            sleep(5);
            system("clear");
        } else {
            break;
        }
    }
    sendCommands(stockthepid.pid);
    return 0;
}