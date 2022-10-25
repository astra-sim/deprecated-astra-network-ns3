#include "ns3/AstraNetworkAPI.hh"
#include<iostream>
#include <stdio.h>
#include <execinfo.h>
#include <queue>
#include <string>
#include <thread>
#include <unistd.h>
#include "workerQueue.h"
#include "third.cc"
#include <vector>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-module.h"
#include "ns3/Sys.hh"

using namespace std;
using namespace ns3;

std::vector<int> physical_dims{8,16};
queue<struct task1> workerQueue;
unsigned long long tempcnt = 999;
unsigned long long  cnt = 0;
struct sim_event {
    void* buffer;
    uint64_t count;
    int type;
    int dst;
    int tag;
    string fnType;
};
Ptr<SimpleUdpApplication> *udp;
class ASTRASimNetwork:public AstraSim::AstraNetworkAPI{

    public:
        queue<sim_event> sim_event_queue;
        ASTRASimNetwork(int rank):AstraNetworkAPI(rank){
        }
        ~ASTRASimNetwork(){}
        int sim_comm_size(AstraSim::sim_comm comm, int* size){
            return 0;
        }
        int sim_finish(){
            for(auto it = nodeHash.begin();it!=nodeHash.end();it++){
                pair<int,int> p = it->first;
                if(p.second==0){
                    cout<<"All data sent from node "<<p.first<<" is "<<it->second<<"\n";
                }
                else{
                    cout<<"All data received by node "<<p.first<<" is "<<it->second<<"\n";
                }
            }
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
            timeSpec.time_val = Simulator::Now().GetNanoSeconds();
            return timeSpec;
        }
        virtual void sim_schedule(
            AstraSim::timespec_t delta,
            void (*fun_ptr)(void* fun_arg),
            void* fun_arg){
                task1 t;
                t.type = 2;
                t.fun_arg = fun_arg;
                t.msg_handler = fun_ptr;
                t.schTime = delta.time_val;
                Simulator::Schedule (NanoSeconds (t.schTime), t.msg_handler, t.fun_arg);
                return;
            }
        virtual int sim_send(
            void* buffer, //not yet used 
            uint64_t count, //number of bytes to be send
            int type,//not yet used 
            int dst,
            int tag, //not yet used 
            AstraSim::sim_request* request,//not yet used 
            void (*msg_handler)(void* fun_arg),
            void* fun_arg){
                if(rank==0 && dst == 1 && cnt == 0){
                    cout<<"starting new collective \n";
                }
                task1 t;
                t.src = rank;
                t.dest = dst;
                t.count = count; 
                t.type = 0;
                t.fun_arg = fun_arg;
                t.msg_handler = msg_handler;
		        sentHash[make_pair(tag,make_pair(t.src,t.dest))] = t;
                SendFlow(rank, dst , count, msg_handler, fun_arg,tag);
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
                task1 t;
                t.src = src;
                t.dest = rank;
                t.count = count; 
                t.type = 1;
                t.fun_arg = fun_arg;
                t.msg_handler = msg_handler;
                if(recvHash.find(make_pair(tag,make_pair(t.src,t.dest)))!=recvHash.end()){
                    int count = recvHash[make_pair(tag,make_pair(t.src,t.dest))];
                    if(count == t.count)
                    {
                        recvHash.erase(make_pair(tag,make_pair(t.src,t.dest)));
                        t.msg_handler(t.fun_arg);
                    }
                    else if (count > t.count){
                        recvHash[make_pair(tag,make_pair(t.src,t.dest))] = count - t.count;
                        t.msg_handler(t.fun_arg);
                    }
                    else{
                        recvHash.erase(make_pair(tag,make_pair(t.src,t.dest)));
                        t.count -= count;
                        expeRecvHash[make_pair(tag,make_pair(t.src,t.dest))] = t;
                    }
                }
                else{
		            if(expeRecvHash.find(make_pair(tag,make_pair(t.src,t.dest)))==expeRecvHash.end()){
                        expeRecvHash[make_pair(tag,make_pair(t.src,t.dest))] = t;;
                    }
                    else{
                        int expecount = expeRecvHash[make_pair(tag,make_pair(t.src,t.dest))].count;
                        t.count += expecount;
                        expeRecvHash[make_pair(tag,make_pair(t.src,t.dest))] = t;
                    }
                }
                return 0;
            }
        void  handleEvent(int dst,int cnt) {
        }
};

int main (int argc, char *argv[]){
    int num_gpus=1;
    for(auto &a:physical_dims){
        num_gpus*=a;
    }
    std::string system_input;
    if(physical_dims.size()==1){
        system_input="sample_a2a_sys.txt";
    }
    else if(physical_dims.size()==2){
        system_input="sample_2D_switch_sys.txt";
    }
    else if(physical_dims.size()==3){
        system_input="sample_3D_switch_sys.txt";
    }
    LogComponentEnable ("SimpleUdpApplication", LOG_LEVEL_INFO);
    LogComponentEnable("OnOffApplication", LOG_LEVEL_INFO);
    LogComponentEnable("PacketSink", LOG_LEVEL_INFO);
    std::vector<ASTRASimNetwork*> networks(num_gpus,nullptr);
    std::vector<AstraSim::Sys*> systems(num_gpus,nullptr);
    std::vector<int> queues_per_dim(physical_dims.size(),1);
    for(int i=0;i<num_gpus;i++){
	networks[i]=new ASTRASimNetwork(i);	
	systems[i] = new AstraSim::Sys(
        	networks[i], // AstraNetworkAPI
        	nullptr, // AstraMemoryAPI
        	i, // id
        	1, // num_passes
        	physical_dims, // dimensions
        	queues_per_dim, // queues per corresponding dimension
        	"../astra-sim/inputs/system/"+system_input, // system configuration
        	"../astra-sim/inputs/workload/microAllReduce.txt", //DLRM_HybridParallel.txt, // Resnet50_DataParallel.txt, // workload configuration
        	256, // communication scale
        	1, // computation scale
        	1, // injection scale
        	1,
        	0, // total_stat_rows and stat_row
        	"scratch/results/", // stat file path
        	"test1", // run name
        	true, // separate_log
        	false  // randezvous protocol
    	);	    
    }	
    main1(argc, argv);
    for(int i=0;i<num_gpus;i++){
	systems[i]->workload->fire();	
    }
    Simulator::Run ();
    Simulator::Stop(Seconds (2000000000));
    Simulator::Destroy();
    return 0;
}
