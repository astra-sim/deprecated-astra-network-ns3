#include "ns3/AstraNetworkAPI.hh"
#include "third.hpp"
// #include "workerQueue.h"
#include <execinfo.h>
#include <iostream>
#include <map>
#include <queue>
#include <stdio.h>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

// #include "myTCPMultiple.h"
#include "ns3/Sys.hh"
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"

// #include "RoCE.h"
// #include<type_info>
using namespace std;
using namespace ns3;

// NS_LOG_COMPONENT_DEFINE ("ASTRASimNetwork");
// struct sim_comm {
//   std::string comm_name;
// };
// enum time_type_e { "SE" };

// struct timespec_t {
//   time_type_e time_res;
//   double time_val;
// };
// extern int global_variable;
// std::vector<int> physical_dims{8,8};
std::vector<int> physical_dims{64};
int job_num = 1;
// std::vector<std::string> ml_models =
// {"../astra-sim/inputs/workload/DLRM_HybridParallel.txt",
// "../astra-sim/inputs/workload/MLP_ModelParallel.txt",
// "../astra-sim/inputs/workload/Transformer_HybridParallel.txt"};
std::vector<std::string>
    ml_models; //= {   "../astra-sim/inputs/workload/microAllToAll.txt",
               //"../astra-sim/inputs/workload/microAllReduce.txt",  };

queue<struct task1> workerQueue;
unsigned long long tempcnt = 999;
unsigned long long cnt = 0;
// map<pair<int,int>, struct task1> expeRecvHash;
// yle: add the rank to network node map
map<int, int> rank_to_network_node_map;
struct sim_event {
  void *buffer;
  uint64_t count;
  int type;
  int dst;
  int tag;
  // AstraSim::sim_request* request;
  // void (*msg_handler)(void* fun_arg);
  // void* fun_arg;
  string fnType;
};
// Ptr<SimpleUdpApplication> *udp;
// std::vector<std::vector<int>> job_nodes_sets;
class ASTRASimNetwork : public AstraSim::AstraNetworkAPI {

public:
  queue<sim_event> sim_event_queue;
  int job_id;
  int network_node_id;
  ASTRASimNetwork(int rank, int _job_id, int _network_node_id)
      : AstraNetworkAPI(rank) {
    // cout<<"hello constructor\n";
    // workerQueue.push("avg");
    job_id = _job_id;
    network_node_id = _network_node_id;
  }
  ~ASTRASimNetwork() {}
  int sim_comm_size(AstraSim::sim_comm comm, int *size) { return 0; }
  int sim_finish() {
    std::cout << "sim finish\n";
    for (auto it = nodeHash.begin(); it != nodeHash.end(); it++) {
      std::pair<int, int> p = it->first;
      if (p.second == 0) {
        cout << "All data sent from node " << p.first << " is " << it->second
             << "\n";
      } else {
        cout << "All data received by node " << p.first << " is " << it->second
             << "\n";
      }
    }
    Simulator::Stop();
    std::cout << "sim finish\n";
    fflush(stdout);
    return 0;
  }
  double sim_time_resolution() { return 0; }
  int sim_init(AstraSim::AstraMemoryAPI *MEM) { return 0; }
  AstraSim::timespec_t sim_get_time() {
    AstraSim::timespec_t timeSpec;
    // timeSpec.time_type_e = "SE";
    timeSpec.time_val = Simulator::Now().GetNanoSeconds();
    return timeSpec;
  }
  virtual void sim_schedule(AstraSim::timespec_t delta,
                            void (*fun_ptr)(void *fun_arg), void *fun_arg) {
    // delta.time_val = 5; //trial
    task1 t;
    t.type = 2;
    t.fun_arg = fun_arg;
    t.msg_handler = fun_ptr;
    t.schTime = delta.time_val;
    // workerQueue.push(t);　
    Simulator::Schedule(NanoSeconds(t.schTime), t.msg_handler, t.fun_arg);
    std::cout << " ns3 sim schedule is called at "
              << Simulator::Now().GetTimeStep() << endl;
    return;
  }
  virtual int sim_send(void *buffer,   // not yet used
                       uint64_t count, // number of bytes to be send
                       int type,       // not yet used
                       int dst,
                       int tag,                        // not yet used
                       AstraSim::sim_request *request, // not yet used
                       void (*msg_handler)(void *fun_arg), void *fun_arg) {
    if (rank == 0 && dst == 1 && cnt == 0) {
      cout << "lets go \n";
    }
    // int src = 0;
    // populate task1 with the required arguments
    task1 t;
    t.src = network_node_id; // job_nodes_sets[job_id][rank]; // how to get src,
                             // is it the rank (starts from 0?)
    t.dest =
        rank_to_network_node_map[dst]; // dst; //job_nodes_sets[job_id][dst];

    std::cout << Simulator::Now().GetTimeStep() << " sim_send  job_id "
              << job_id << " rank " << rank << " to  dst " << dst
              << ", network src " << network_node_id << " network dest "
              << t.dest << " tag " << tag << " size " << count << std::endl;
    // std::cout<<"sim_send  job_id "<< job_id <<" rank " <<rank  << "
    // network_node_id "<< network_node_id <<" tag "<<tag <<std::endl;

    // std::cout<<"sim_send  job_id "<< job_id <<" src " <<t.src <<" dst
    // "<<t.dest <<" tag "<<tag; std::cout << " size " << count <<"\n"; int a =
    // 1;
    t.count = count;
    // t.fun_arg = &a;
    t.type = 0;
    t.fun_arg = fun_arg;
    t.msg_handler = msg_handler;
    if (count == 0) {
      std::cout << " the message is too small " << count << std::endl;
      NS_ASSERT_MSG(false, "the message is too small");
    }
    // workerQueue.push(t);
    // udp[t.src]->SendPacket(t.dest, t.fun_arg, t.msg_handler, t.count, tag);
    // cout<<"COUNT and PACKET is "<<count<<" "<<maxPacketCount<<"\n";
    // static int totalSends=0;
    // totalSends++;
    // cout<<"total sends: "<<totalSends<<" src dest count "<<rank<<" "<<dst<<"
    // "<<count<<endl; cout<<"src dst cOUNT IN SEND IS "<<rank<<" "<<dst<<"
    // "<<count<<"\n";

    sentHash[make_pair(tag, make_pair(t.src, t.dest))] = t;

    SendFlow(t.src, t.dest, count, msg_handler, fun_arg, tag);
    // cout << "event at sender pushed " << t.src << " "
    //      << " " << t.dest << " " << tag << " " << count << "\n";
    return 0;
  }
  virtual int sim_recv(void *buffer, uint64_t count, int type, int src, int tag,
                       AstraSim::sim_request *request,
                       void (*msg_handler)(void *fun_arg), void *fun_arg) {
    // populate task1 with the required arguments
    task1 t;
    t.src = rank_to_network_node_map[src]; // src;//job_nodes_sets[job_id][src];
                                           // //src;
    t.dest = network_node_id; // job_nodes_sets[job_id][rank]; //rank; // how to
                              // get dest, is it the rank (starts from 0?)
    std::cout << Simulator::Now().GetTimeStep() << " sim_recv  job_id "
              << job_id << " src " << src << " to  dst " << rank
              << ", network src " << t.src << " network dest " << t.dest
              << " tag " << tag << std::endl;
    // std::cout<<"sim_recv  job_id "<< job_id <<" src " <<src<<" rank
    // "<<rank<<" tag "<<tag<<"\n"; std::cout<<"sim_recv  job_id "<< job_id <<"
    // src " <<t.src<<" dst "<<t.dest<<" tag "<<tag<<"\n"; int a = 1;
    t.count = count;
    // std::cout<<"sim_recv  count "<< t.count <<t.src<<" "<<t.dest<<"
    // "<<tag<<"\n"; t.fun_arg = &a;
    t.type = 1;
    t.fun_arg = fun_arg;
    t.msg_handler = msg_handler;

    // workerQueue.push(t);
    if (recvHash.find(make_pair(tag, make_pair(t.src, t.dest))) !=
        recvHash.end()) {
      int count = recvHash[make_pair(tag, make_pair(t.src, t.dest))];
      if (count == t.count) {
        recvHash.erase(make_pair(tag, make_pair(t.src, t.dest)));
        std::cout << "already in recv hash " << t.src << " " << t.dest << " "
                  << tag << "\n";
        t.msg_handler(t.fun_arg);
      } else if (count > t.count) {
        recvHash[make_pair(tag, make_pair(t.src, t.dest))] = count - t.count;
        std::cout << "already in recv hash with more data " << t.src << " "
                  << t.dest << " " << tag << "\n";
        t.msg_handler(t.fun_arg);
      } else {
        recvHash.erase(make_pair(tag, make_pair(t.src, t.dest)));
        t.count -= count;
        expeRecvHash[make_pair(tag, make_pair(t.src, t.dest))] = t;
        // cout<<"partially in recv hash "<<t.src<<" "<<t.dest<<" "<<tag<<"\n";
      }
    } else {
      if (expeRecvHash.find(make_pair(tag, make_pair(t.src, t.dest))) ==
          expeRecvHash.end()) {
        expeRecvHash[make_pair(tag, make_pair(t.src, t.dest))] = t;
        std::cout << "not in recv hash " << t.src << " " << t.dest << "  "
                  << tag << "\n";
      } else {
        int expecount =
            expeRecvHash[make_pair(tag, make_pair(t.src, t.dest))].count;
        t.count += expecount;
        expeRecvHash[make_pair(tag, make_pair(t.src, t.dest))] = t;
        std::cout << "not in recv hash but in expected recv hash " << t.src
                  << " " << t.dest << " " << tag << "\n";
      }
      // expeRecvHash[make_pair(tag,make_pair(t.src,t.dest))] = t;
    }
    // cout<<"COUNT IN RECV IS "<<count<<"\n";
    // cout<<"event at receiver pushed\n";
    return 0;
  }
  void handleEvent(int dst, int cnt) {
    // cout<<"test\n";
    // cout<<"event pushed\n";
  }
};

int main(int argc, char *argv[]) {
  main1(argc, argv);
  std::cout << "job config: " << job_config << " sys " << sys_config
            << std::endl;

  jobf.open(job_config.c_str());

  int num_gpus = 1;
  int total_pause_times = 0;
  double communication_scale_factor = 1.0;
  int num_passes = 1;
  jobf >> job_num >> num_gpus >> total_pause_times >>
      communication_scale_factor >> num_passes;
  std::cout << " job_num " << job_num << " num_gpus " << num_gpus
            << " total_pause_times " << total_pause_times
            << " communication_scale_factor " << communication_scale_factor
            << " num_passes " << num_passes << std::endl;
  physical_dims[0] = num_gpus;
  vector<vector<int>> nodes_per_job;
  std::string physical_nodes;
  for (int job_id = 0; job_id < job_num; job_id++) {
    std::string model;
    jobf >> model;
    ml_models.push_back(model);
    vector<int> nodes;
    for (int i = 0; i < num_gpus; i++) {
      int node_id;
      jobf >> node_id;
      nodes.push_back(node_id);
    }
    nodes_per_job.push_back(nodes);
  }

  for (int j = 0; j < nodes_per_job.size(); j++) {
    for (int i = 0; i < num_gpus; i++) {
      std::cout << nodes_per_job[j][i] << std::endl;
      int x = j * num_gpus + i;
      int network_node_id =
          nodes_per_job[j][i]; //(int)(i/64) + 16*(i%64); //i*16 + j;
      rank_to_network_node_map[x] = network_node_id;
    }
  }
  std::cout << " num_gpus " << num_gpus << std::endl;

  LogComponentEnable("OnOffApplication", LOG_LEVEL_INFO);
  LogComponentEnable("PacketSink", LOG_LEVEL_INFO);
  // ASTRASimNetwork network0 = ASTRASimNetwork(255);
  // ASTRASimNetwork network1 = ASTRASimNetwork(254);
  std::vector<ASTRASimNetwork *> networks(num_gpus * job_num, nullptr);
  std::vector<AstraSim::Sys *> systems(num_gpus * job_num, nullptr);
  // std::vector<int> physical_dims(1,num_gpus);
  std::vector<int> queues_per_dim(physical_dims.size(), 1);
  for (int i : queues_per_dim)
    std::cout << " queue_per_dim " << i << std::endl;
  for (int i : physical_dims)
    std::cout << "physical_dims " << i << std::endl;

  for (int j = 0; j < job_num; j++) {
    int model_id = j % ml_models.size();
    for (int i = 0; i < num_gpus; i++) {
      int x = j * num_gpus + i;
      networks[x] = new ASTRASimNetwork(i, j, rank_to_network_node_map[x]);
      std::cout << "start new system " << x << std::endl;
      systems[x] = new AstraSim::Sys(
          networks[x],         // AstraNetworkAPI
          nullptr,             // AstraMemoryAPI
          x,                   // id
          num_passes,          // num_passes
          physical_dims,       // dimensions
          queues_per_dim,      // queues per corresponding dimension
          sys_config,          //"../astra-sim/inputs/system/"+sys_config, // +
                               //system_input, // system configuration
          ml_models[model_id], // microAllReduce.txt DLRM_HybridParallel.txt,
                               // microAllToAll.txt, Resnet50_DataParallel.txt,
          (int)physical_dims[0] *
              communication_scale_factor, // communication scale
          1,                              // computation scale
          1,                              // injection scale
          1,
          0,                           // total_stat_rows and stat_row
          "results/",                  // stat file path
          "test_" + std::to_string(j), // run name
          true,                        // separate_log
          false                        // randezvous protocol
      );
    }
  }
  std::cout << "workload fire starts " << std::endl;
  for (int i = 0; i < num_gpus * job_num; i++) {
    systems[i]->workload->fire();
  }
  Simulator::Run();
  // Simulator::Stop(TimeStep (0x7fffffffffffffffLL));
  Simulator::Stop(Seconds(2000000000));
  Simulator::Destroy();
  std::cout << "sim finish now \n";
  return 0;
}
