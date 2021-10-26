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
    // void (*fptr) (void);
    int count;
    void* fun_arg;
    void (*msg_handler)(void* fun_arg);
    double schTime; //in sec
};
extern queue<struct task1> workerQueue;
extern map<pair<int,pair<int,int> >, struct task1> expeRecvHash;
extern map<pair<int,pair<int,int> >,int > recvHash;
//extern map<pair<int,int>, struct task1> expeRecvHash;
//extern map<pair<int,int>,int > recvHash;

// class workerQueue{
//     public:
//         extern queue<string> q;
//         workerQueue(){
//             // q = new queue<string>();
//             q.push("0");
//         }
//         ~workerQueue(){}
//         void pushItem(string str){
//             q.push(str);
//             cout<<"que\n";
//         }
// };
// int main (int argc, char *argv[]){
//     cout<<"hello\n";
//     workerQueue que;
//     que.pushItem("etest");
//     while(!que.q.empty()){
//         cout<<que.q.front()<<"\n";
//         que.q.pop();
//     }
// }
#endif
