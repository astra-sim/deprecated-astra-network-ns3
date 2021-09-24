#include "/Users/tarannumkhan/Desktop/astra-sim/astra-sim/system/AstraNetworkAPI.hh"
#include<iostream>
#include <stdio.h>
#include <execinfo.h>
#include <queue>
#include <string>
#include <thread>
#include <unistd.h>
#include "workerQueue.h"
#include "myTCPMultiple.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-module.h"

// #include "RoCE.h"
//#include<type_info>
using namespace std;
using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("ASTRASimNetwork");
// struct sim_comm {
//   std::string comm_name;
// };
// enum time_type_e { "SE" };

// struct timespec_t {
//   time_type_e time_res;
//   double time_val;
// };
// extern int global_variable;
queue<struct task1> workerQueue;
// map<pair<int,int>, struct task1> expeRecvHash;
struct sim_event {
    void* buffer;
    uint64_t count;
    int type;
    int dst;
    int tag;
    // AstraSim::sim_request* request;
    // void (*msg_handler)(void* fun_arg);
    // void* fun_arg;
    string fnType;
};
class ASTRASimNetwork: AstraSim::AstraNetworkAPI{

    public:
        queue<sim_event> sim_event_queue;
        ASTRASimNetwork(int rank):AstraNetworkAPI(rank){
            cout<<"hello constructor\n";
            // workerQueue.push("avg");
        }
        ~ASTRASimNetwork(){}
        int sim_comm_size(AstraSim::sim_comm comm, int* size){
            return 0;
        }
        int sim_finish(){
            cout<<"sim finish\n";
            Simulator::Destroy ();
            return 0;
        }
        double sim_time_resolution(){
            return 0;
        }
        int sim_init(AstraSim::AstraMemoryAPI* MEM){
            return 0;
        }
        AstraSim::timespec_t sim_get_time(){
            AstraSim::timespec_t timeSpec;
            // timeSpec.time_type_e = "SE";
            timeSpec.time_val = 0.0;
            return timeSpec;
        }
        virtual void sim_schedule(
            AstraSim::timespec_t delta,
            void (*fun_ptr)(void* fun_arg),
            void* fun_arg){
                return;
            }
        virtual int sim_send(
            void* buffer,
            uint64_t count,
            int type,
            int dst,
            int tag,
            AstraSim::sim_request* request,
            void (*msg_handler)(void* fun_arg),
            void* fun_arg){
                // int src = 0;
                // workerQueue.push(make_pair(src, dst));
                cout<<"event pushed\n";
                // Simulator::Schedule (Seconds (2), &SimpleUdpApplication::SendPacket, udp0, packet1, dest_ip, 7777);
                // RoCE roce;
                // roce.sim_init(2);
                // roce.sim_send();
                return 0;
            }
        virtual int sim_recv(
            void* buffer,
            uint64_t count,
            int type,
            int src,
            int tag,
            AstraSim::sim_request* request,
            void (*msg_handler)(void* fun_arg),
            void* fun_arg){
                return 0;
            }
        void  handleEvent(int dst,int cnt) {
            cout<<"test\n";
            cout<<"event pushed\n";
        }
};

void foo(void) {
    myTCPMultiple example;
    example.init_t();
}

void fun(void* a) {
    cout<<*(int *)a<<"\n";
    cout<<"Having fun!"<<endl;
}

void sim_init(int n){
    NodeContainer nodes;
    nodes.Create (n);
    CsmaHelper csma;  
    csma.SetChannelAttribute ("DataRate", StringValue ("1Gbps"));
    csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds(6560)));

    NetDeviceContainer csmaDevs;
    csmaDevs = csma.Install (nodes);
    csma.EnableAsciiAll("simple-udp");

    InternetStackHelper stack;
    stack.Install (nodes);

    Ipv4AddressHelper address;
    address.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer ifaces;
    ifaces = address.Assign (csmaDevs);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
    Packet::EnablePrinting (); 

    //Create Two UDP applications
    // //Set the start & stop times
    Ptr<SimpleUdpApplication> udp[n];
    //assumes astra sim pass from index 0;
    for(int i = 0;i<n;i++){
        udp[i] = CreateObject <SimpleUdpApplication> ();
        udp[i]->SetStartTime(Seconds(0));
        udp[i]->SetStopTime(Seconds(100));
        // udp[i]->InitializeAppRecv(n,i);    
        nodes.Get(i)->AddApplication(udp[i]);    
    }
    for(int i = 0;i<n;i++){      
        udp[i]->InitializeAppRecv(n,i); 
    }
    for(int i = 0;i<n;i++){
        udp[i]->InitializeAppSend(n,i,ifaces);    
        
    }
    Ipv4Address dest_ip = ifaces.GetAddress(1);
    cout<<"dest ip is "<<dest_ip<<"\n";

    task1 t;
    t.src = 1;
    t.dest = 2;
    int a = 1;
    t.fun_arg = &a;
    t.msg_handler = &fun;
    workerQueue.push(t);
    while(!workerQueue.empty()){
        task1 t1 = workerQueue.front();
        Ipv4Address dest_ip1 = ifaces.GetAddress(t1.dest);
        cout<<"dest ip is "<<dest_ip1<<"\n";
        Simulator::Schedule (Seconds (4), &SimpleUdpApplication::SendPacket, udp[t1.src], t1.dest, t1.fun_arg, t1.msg_handler);
        // t1.msg_handler(t1.fun_arg);
        workerQueue.pop();
    }
    //WHENEVER SENDING TO RECEIVE FUNCTION first check if that pair already in receive hash. 
    if(recvHash.find(make_pair(1,2))!=recvHash.end()){
        recvHash.erase(make_pair(1,2));
        t.msg_handler(t.fun_arg);
        cout<<"already in recv hash\n";
    }
    else{
        expeRecvHash[make_pair(1,2)] = t;
        cout<<"not in recv hash\n";
    }
    for(std::map<std::pair<int, int>, struct task1>::iterator it = expeRecvHash.begin();it!=expeRecvHash.end();it++){
        std::cout<<it->first.first<<" "<<it->first.second<<"\n";
    }
//    Simulator::Schedule (Seconds (4), &SimpleUdpApplication::FinishTask, udp[t.src]); 
//    Simulator::Schedule (Seconds (5), &SimpleUdpApplication::processQueue, udp[t.src]);
    // Ptr <Packet> packet2 = Create <Packet> (800);
    // LogComponentEnable ("SimpleUdpApplication", LOG_LEVEL_INFO);
    // Simulator::Schedule (Seconds (2), &SimpleUdpApplication::SendPacket, udp[0], packet2, dest_ip, 9999);

    // Simulator::Stop (Seconds (10));

    // pair<int, int> p1 = workerQueue.front();

    Simulator::Run ();
    // pair<int,int> p = workerQueue.front();
    // cout<<p.first<<" "<<p.second<<"\n";
    // while(1)
    // {
    //     while(!workerQueue.empty()){
    //         cout<<"sdkfl\n";
    //         pair<int,int> p = workerQueue.front();
    //         cout<<p.first<<" "<<p.second;
    //         workerQueue.pop();
    //         cout<<"xxxx\n";
    //         dest_ip = ifaces.GetAddress(p.second);
    //         cout<<"xxxx1 "<<dest_ip<<"\n";
    //         udp[p.first]->SendPacket(packet1,1);
    //         // udp[p.first]->SendPacket(packet2,dest_ip,9999);
    //         // Simulator::Schedule (Seconds (0), &SimpleUdpApplication::SendPacket, udp[p.first],packet1,1);
    //         cout<<"xxxx2\n";
    //     }
    // }
    
    Simulator::Stop (Seconds (10));
    // while(1){}
}

void MovieQuote(void* fun_arg)
{
    cout<<"in movie quote\n";
}


int main (int argc, char *argv[]){
    LogComponentEnable ("SimpleUdpApplication", LOG_LEVEL_INFO);
    cout << "Hello world!\n";
    // LogComponentEnable("myTCPMultiple",LOG_LEVEL_INFO);
    LogComponentEnable("OnOffApplication", LOG_LEVEL_INFO);
    LogComponentEnable("PacketSink", LOG_LEVEL_INFO);
    // ASTRASimNetwork network = ASTRASimNetwork(1);
    // network.sim_init(4);
    // network.sim_finish();
    //AstraSim::sim_requests* request;
    // void (*SomethingToSay)(void* arg);
    // network.handleEvent(1,1);
    // foo();
    // RoCE roce;
    sim_init(4);
    // std::thread  t1(sim_init, 4);
    // sleep(2);
    // network.sim_send(nullptr,1,1,1,1,nullptr,&MovieQuote,nullptr);
    // network.sim_send(nullptr,1,1,1,1,nullptr,&MovieQuote,nullptr);
    // sleep(2);
    // t1.join();
    return 0;
}
