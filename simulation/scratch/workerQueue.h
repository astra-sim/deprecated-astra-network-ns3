#ifndef __WORKERQEUE_HH__
#define __WORKERQEUE_HH__
#include <queue>
#include <iostream> 
#include <string>
#include <map>

using namespace std;
struct task1{
    int src;
    int dest;
    int type;
    int count;
    void* fun_arg;
    void (*msg_handler)(void* fun_arg);
    double schTime; //in sec
};
extern unsigned long long cnt;
extern unsigned long long tempcnt;
extern queue<struct task1> workerQueue;
extern map<pair<int,pair<int,int> >, struct task1> expeRecvHash;
extern map<pair<int,pair<int,int> >,int > recvHash;
extern map<pair<int,pair<int,int> >,struct task1> sentHash;
extern map<pair<int,int>,int> nodeHash;//mapping of <node,type> to data sent/received. 

#endif
