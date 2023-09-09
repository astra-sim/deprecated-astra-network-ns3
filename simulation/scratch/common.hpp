/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#undef PGO_TRAINING
#define PATH_TO_PGO_CONFIG "path_to_pgo_config"

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <time.h>
#include "ns3/core-module.h"
#include "ns3/qbb-helper.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/global-route-manager.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/packet.h"
#include "ns3/error-model.h"
#include <ns3/rdma.h>
#include <ns3/rdma-client.h>
#include <ns3/rdma-client-helper.h>
#include <ns3/rdma-driver.h>
#include <ns3/switch-node.h>
#include <ns3/sim-setting.h>

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE("GENERIC_SIMULATION");

uint32_t cc_mode = 1;
uint32_t fc_mode = 2; //Full-PFC
uint32_t lc_mode = 0;
bool enable_qcn = true, use_dynamic_pfc_threshold = true, enable_packetspray = false;
uint32_t packet_payload_size = 1000, l2_chunk_size = 0, l2_ack_interval = 0;
double pause_time = 5, simulator_stop_time = 3.01;
std::string data_rate, link_delay, topology_file, flow_file, trace_file, trace_output_file;
std::string fct_output_file = "fct.txt";
std::string pfc_output_file = "pfc.txt";

double alpha_resume_interval = 55, rp_timer, ewma_gain = 1 / 16;
double rate_decrease_interval = 4;
uint32_t fast_recovery_times = 5;
std::string rate_ai, rate_hai, min_rate = "100Mb/s";
std::string dctcp_rate_ai = "1000Mb/s";

bool clamp_target_rate = false, l2_back_to_zero = false;
double error_rate_per_link = 0.0;
uint32_t has_win = 1;
uint32_t global_t = 1;
uint32_t mi_thresh = 5;
bool var_win = false, fast_react = true;
bool multi_rate = true;
bool sample_feedback = false;
double pint_log_base = 1.05;
double pint_prob = 1.0;
double u_target = 0.95;
uint32_t int_multi = 1;
bool rate_bound = true;
int nic_total_pause_time =
    0; // slightly less than finish time without inefficiency in us

uint32_t ack_high_prio = 0;
uint64_t link_down_time = 0;
uint32_t link_down_A = 0, link_down_B = 0;

uint32_t enable_trace = 1;

uint32_t buffer_size = 16;

uint32_t qlen_dump_interval = 1000, qlen_mon_interval = 100;
uint64_t qlen_mon_start = 0, qlen_mon_end = 2100000000;

uint32_t sfc_queue_num = 4;
uint32_t sfc_trigger_threshold = 100*1000;
uint32_t sfc_target_queue_depth = 10*1000;
uint32_t sfc_opt = 0;
string qlen_mon_file;

unordered_map<uint64_t, uint32_t> rate2kmax, rate2kmin;
unordered_map<uint64_t, double> rate2pmax;
std::map<uint8_t, uint8_t> swc_qos_map;
std::map<uint8_t, uint8_t> flow_qos_map;

// Smart-Pause
bool enableSP = false;
uint64_t SPthres = 30000;
uint64_t SPmaxPauseLen = 50000;
double SP_noPausePeriodRatio = 0;
bool enableSPcor = true;
double SPcorGain = 16.0;
double clockOffsetStd = 0.001; // In nanoseconds

//astra-sim 
std::string job_config, sys_config; 
/************************************************
 * Runtime varibles
 ***********************************************/
std::ifstream topof, flowf, tracef, jobf;

NodeContainer n;

uint64_t nic_rate;

uint64_t maxRtt, maxBdp;

std::vector<Ipv4Address> serverAddress;

// maintain port number for each host pair
std::unordered_map<uint32_t, unordered_map<uint32_t, uint16_t> > portNumber;

struct Interface
{
    uint32_t idx;
    bool up;
    uint64_t delay;
    uint64_t bw;

    Interface() : idx(0), up(false) {}
};
map<Ptr<Node>, map<Ptr<Node>, Interface>> nbr2if;
// Mapping destination to next hop for each node: <node, <dest, <nexthop0, ...> > >
map<Ptr<Node>, map<Ptr<Node>, vector<Ptr<Node>>>> nextHop;
map<Ptr<Node>, map<Ptr<Node>, uint64_t>> pairDelay;
map<Ptr<Node>, map<Ptr<Node>, uint64_t>> pairTxDelay;
map<uint32_t, map<uint32_t, uint64_t>> pairBw;
map<Ptr<Node>, map<Ptr<Node>, uint64_t>> pairBdp;
map<uint32_t, map<uint32_t, uint64_t>> pairRtt;




struct FlowInput{
	uint32_t src, dst, pg, appid, maxPacketCount, port, dport;
	uint64_t start_time;
	uint32_t idx;
};

FlowInput flow_input = {0};
uint32_t flow_num;
uint32_t finished_flow_num;



void SendFlow(int src, int dst , int maxPacketCount, void (*msg_handler)(void* fun_arg), void* fun_arg, int tag){
    uint32_t port = portNumder[src][dst]++; // get a new port number
    int pg = 3,dport = 100;
	flow_input.idx++;
    RdmaClientHelper clientHelper(pg, serverAddress[src], serverAddress[dst], port, dport, maxPacketCount, has_win?(global_t==1?maxBdp:pairBdp[n.Get(src)][n.Get(dst)]):0, global_t==1?maxRtt:pairRtt[src][dst],
    msg_handler, fun_arg, tag, src, dst);
    ApplicationContainer appCon = clientHelper.Install(n.Get(src));
    appCon.Start(Time(0));
}



Ipv4Address node_id_to_ip(uint32_t id)
{
    return Ipv4Address(0x0b000001 + ((id / 256) * 0x00010000) + ((id % 256) * 0x00000100));
}

uint32_t ip_to_node_id(Ipv4Address ip)
{
    return (ip.Get() >> 8) & 0xffff;
}

void get_pfc(FILE *fout, Ptr<QbbNetDevice> dev, uint32_t type)
{
    fprintf(fout, "%lu %u %u %u %u\n", Simulator::Now().GetTimeStep(), dev->GetNode()->GetId(), dev->GetNode()->GetNodeType(), dev->GetIfIndex(), type);
}

struct QlenDistribution
{
    vector<uint32_t> cnt; // cnt[i] is the number of times that the queue len is i KB

    void add(uint32_t qlen)
    {
        uint32_t kb = qlen / 1000;
        if (cnt.size() < kb + 1)
            cnt.resize(kb + 1);
        cnt[kb]++;
    }
};
map<uint32_t, map<uint32_t, uint32_t>> queue_result;
void monitor_buffer(FILE *qlen_output, NodeContainer *n)
{
    for (uint32_t i = 0; i < n->GetN(); i++)
    {
        if (n->Get(i)->GetNodeType() == 1)
        { // is switch
            Ptr<SwitchNode> sw = DynamicCast<SwitchNode>(n->Get(i));
            if (queue_result.find(i) == queue_result.end())
                queue_result[i];
			//fprintf(qlen_output, "\n");
			//fprintf(qlen_output, "time: %lu\n", Simulator::Now().GetTimeStep());
			int test = 0;
            for (uint32_t j = 1; j < sw->GetNDevices(); j++)
            {
                uint32_t size = 0;
                for (uint32_t k = 0; k < SwitchMmu::qCnt; k++)
                    size += sw->m_mmu->egress_bytes[j][k];
                				//if (queue_result[i].find(j) == queue_result[i].end())
				//{
				//	vector<uint32_t> v;
				//	queue_result[i][j] = v;
				//}
				if(size >= 1000){
				queue_result[i][j] = size;// .push_back(size);
				if(test==0){
				test = 1;
				fprintf(qlen_output, "time %lu %u ", Simulator::Now().GetTimeStep(), i);
				}
				//if(j==1){
				//fprintf(qlen_output, "t %lu %u j %u %u ", Simulator::Now().GetTimeStep(), i, j, size);
				if (j<sw->GetNDevices()-1){
				test = 2;
				fprintf(qlen_output, "j %u %u ", j, size);
				}else if (j==sw->GetNDevices()-1){
				fprintf(qlen_output, "j %u %u\n", j, size);
				test = 3;
				}
				}
				if(j == sw->GetNDevices()-1 && test==2){
 				fprintf(qlen_output, "\n");
				}
				//else
				//	queue_result[i][j]+=size;
				//queue_result[i][j].add(size);
            }
		fflush(qlen_output);
			//fprintf(qlen_output, "\n");
		}
    }
		fflush(qlen_output);
        Simulator::Schedule(NanoSeconds(qlen_mon_interval), &monitor_buffer, qlen_output, n);
}

void CalculateRoute(Ptr<Node> host)
{
    // queue for the BFS.
    vector<Ptr<Node>> q;
    // Distance from the host to each node.
    map<Ptr<Node>, int> dis;
    map<Ptr<Node>, uint64_t> delay;
    map<Ptr<Node>, uint64_t> txDelay;
    map<Ptr<Node>, uint64_t> bw;
    // init BFS.
    q.push_back(host);
    dis[host] = 0;
    delay[host] = 0;
    txDelay[host] = 0;
    bw[host] = 0xfffffffffffffffflu;
    // BFS.
    for (int i = 0; i < (int)q.size(); i++)
    {
        Ptr<Node> now = q[i];
        int d = dis[now];
        for (auto it = nbr2if[now].begin(); it != nbr2if[now].end(); it++)
        {
            // skip down link
            if (!it->second.up)
                continue;
            Ptr<Node> next = it->first;
            if (dis.find(next) == dis.end())
            {
                dis[next] = d + 1;
                delay[next] = delay[now] + it->second.delay;
                txDelay[next] = txDelay[now] + packet_payload_size * 1000000000lu * 8 / it->second.bw;
                bw[next] = std::min(bw[now], it->second.bw);
                if (next->GetNodeType() == 1)
                    q.push_back(next);
            }
            if (d + 1 == dis[next])
            {
                nextHop[next][host].push_back(now);
            }
        }
    }
    for (auto it : delay){
		// std::cout << "pairDelay first "<< it.first->GetId() << " host " << host->GetId() << " delay " << it.second << std::endl;
        pairDelay[it.first][host] = it.second;
	}
    for (auto it : txDelay)
        pairTxDelay[it.first][host] = it.second;
    for (auto it : bw){
		// std::cout << "pairBw first "<< it.first->GetId() << " host " << host->GetId() << " bw " << it.second << std::endl;
        pairBw[it.first->GetId()][host->GetId()] = it.second;
	}
}

void CalculateRoutes(NodeContainer &n)
{
    for (int i = 0; i < (int)n.GetN(); i++)
    {
        Ptr<Node> node = n.Get(i);
        if (node->GetNodeType() == 0)
            CalculateRoute(node);
    }
}

void SetRoutingEntries()
{
    for (auto i = nextHop.begin(); i != nextHop.end(); i++)
    {
        Ptr<Node> node = i->first;
        auto &table = i->second;
        for (auto j = table.begin(); j != table.end(); j++)
        {
            Ptr<Node> dst = j->first;
            Ipv4Address dstAddr = dst->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
            vector<Ptr<Node>> nexts = j->second;
            for (int k = 0; k < (int)nexts.size(); k++)
            {
                Ptr<Node> next = nexts[k];
                uint32_t interface = nbr2if[node][next].idx;
                if (node->GetNodeType() == 1)
                    DynamicCast<SwitchNode>(node)->AddTableEntry(dstAddr, interface);
                else
                {
                    node->GetObject<RdmaDriver>()->m_rdma->AddTableEntry(dstAddr, interface);
                }
            }
        }
    }
}

// take down the link between a and b, and redo the routing
void TakeDownLink(NodeContainer n, Ptr<Node> a, Ptr<Node> b)
{
    if (!nbr2if[a][b].up)
        return;
    // take down link between a and b
    nbr2if[a][b].up = nbr2if[b][a].up = false;
    nextHop.clear();
    CalculateRoutes(n);
    // clear routing tables
    for (uint32_t i = 0; i < n.GetN(); i++)
    {
        if (n.Get(i)->GetNodeType() == 1)
            DynamicCast<SwitchNode>(n.Get(i))->ClearTable();
        else
            n.Get(i)->GetObject<RdmaDriver>()->m_rdma->ClearTable();
    }
    DynamicCast<QbbNetDevice>(a->GetDevice(nbr2if[a][b].idx))->TakeDown();
    DynamicCast<QbbNetDevice>(b->GetDevice(nbr2if[b][a].idx))->TakeDown();
    // reset routing table
    SetRoutingEntries();

    // redistribute qp on each host
    for (uint32_t i = 0; i < n.GetN(); i++)
    {
        if (n.Get(i)->GetNodeType() == 0)
            n.Get(i)->GetObject<RdmaDriver>()->m_rdma->RedistributeQp();
    }
}

uint64_t get_nic_rate(NodeContainer &n)
{
    for (uint32_t i = 0; i < n.GetN(); i++)
        if (n.Get(i)->GetNodeType() == 0)
            return DynamicCast<QbbNetDevice>(n.Get(i)->GetDevice(1))->GetDataRate().GetBitRate();
}

bool ReadConf(int argc, char *argv[])
{

	#ifndef PGO_TRAINING
		if (argc > 1)
	#else
		if (true)
	#endif
		{
			//Read the configuration file
			std::ifstream conf;
	#ifndef PGO_TRAINING
			conf.open(argv[1]);
	#else
			conf.open(PATH_TO_PGO_CONFIG);
	#endif
		while (!conf.eof())
		{
			std::string key;
			conf >> key;

			// std::cout << conf.cur << "\n";
			// std::cout << key << std::endl;
			// Helper function for parsing k:v lists
			auto qos_map_parse_elem = [](std::map<uint8_t, uint8_t> &qos_map, 
										 std::string elem) {
				std::string elem_delim = ":";

				uint16_t app;
				uint16_t pg;
				size_t elem_pos = 0;

				elem_pos = elem.find(elem_delim);
				app = static_cast<uint16_t>(std::stoi(elem.substr(0, elem_pos)));
				pg = static_cast<uint16_t>(std::stoi(elem.substr(elem_pos+1, elem.length())));

				NS_ASSERT_MSG(qos_map.find(app) == qos_map.end(), "QOS_MAP value provided twice.");
				qos_map[app] = pg;
			};

			if (key.compare("ENABLE_QCN") == 0)
			{
				uint32_t v;
				conf >> v;
				enable_qcn = v;
			}
			else if (key.compare("ENABLE_PACKETSPRAY") == 0)
			{
				uint32_t v;
				conf >> v;
				enable_packetspray = v;
				if (enable_packetspray)
					std::cout << "ENABLE_PACKETSPRAY\t\t\t" << "Yes" << "\n";
				else
					std::cout << "ENABLE_PACKETSPRAY\t\t\t" << "No" << "\n";
			}
			else if (key.compare("USE_DYNAMIC_PFC_THRESHOLD") == 0)
			{
				uint32_t v;
				conf >> v;
				use_dynamic_pfc_threshold = v;
			}
			else if (key.compare("CLAMP_TARGET_RATE") == 0)
			{
				uint32_t v;
				conf >> v;
				clamp_target_rate = v;
			}
			else if (key.compare("PAUSE_TIME") == 0)
			{
				double v;
				conf >> v;
				pause_time = v;
			}
			else if (key.compare("DATA_RATE") == 0)
			{
				std::string v;
				conf >> v;
				data_rate = v;
			}
			else if (key.compare("LINK_DELAY") == 0)
			{
				std::string v;
				conf >> v;
				link_delay = v;
			}
			else if (key.compare("PACKET_PAYLOAD_SIZE") == 0)
			{
				uint32_t v;
				conf >> v;
				packet_payload_size = v;
			}
			else if (key.compare("L2_CHUNK_SIZE") == 0)
			{
				uint32_t v;
				conf >> v;
				l2_chunk_size = v;
			}
			else if (key.compare("L2_ACK_INTERVAL") == 0)
			{
				uint32_t v;
				conf >> v;
				l2_ack_interval = v;
			}
			else if (key.compare("L2_BACK_TO_ZERO") == 0)
			{
				uint32_t v;
				conf >> v;
				l2_back_to_zero = v;
			}
			else if (key.compare("TOPOLOGY_FILE") == 0)
			{
				std::string v;
				conf >> v;
				topology_file = v;
			}
			else if (key.compare("FLOW_FILE") == 0)
			{
				std::string v;
				conf >> v;
				flow_file = v;
			}
			else if (key.compare("TRACE_FILE") == 0)
			{
				std::string v;
				conf >> v;
				trace_file = v;
			}
			else if (key.compare("TRACE_OUTPUT_FILE") == 0)
			{
				std::string v;
				conf >> v;
				trace_output_file = v;
				if (argc > 2)
				{
					trace_output_file = trace_output_file + std::string(argv[2]);
				}
			}
			else if (key.compare("SIMULATOR_STOP_TIME") == 0)
			{
				double v;
				conf >> v;
				simulator_stop_time = v;
			}
			else if (key.compare("ALPHA_RESUME_INTERVAL") == 0)
			{
				double v;
				conf >> v;
				alpha_resume_interval = v;
			}
			else if (key.compare("RP_TIMER") == 0)
			{
				double v;
				conf >> v;
				rp_timer = v;
			}
			else if (key.compare("EWMA_GAIN") == 0)
			{
				double v;
				conf >> v;
				ewma_gain = v;
			}
			else if (key.compare("FAST_RECOVERY_TIMES") == 0)
			{
				uint32_t v;
				conf >> v;
				fast_recovery_times = v;
			}
			else if (key.compare("RATE_AI") == 0)
			{
				std::string v;
				conf >> v;
				rate_ai = v;
			}
			else if (key.compare("RATE_HAI") == 0)
			{
				std::string v;
				conf >> v;
				rate_hai = v;
			}
			else if (key.compare("ERROR_RATE_PER_LINK") == 0)
			{
				double v;
				conf >> v;
				error_rate_per_link = v;
			}
			else if (key.compare("CC_MODE") == 0){
				conf >> cc_mode;
			}else if (key.compare("RATE_DECREASE_INTERVAL") == 0){
				double v;
				conf >> v;
				rate_decrease_interval = v;
			}else if (key.compare("MIN_RATE") == 0){
				conf >> min_rate;
			}else if (key.compare("FCT_OUTPUT_FILE") == 0){
				conf >> fct_output_file;
			}else if (key.compare("HAS_WIN") == 0){
				conf >> has_win;
			}else if (key.compare("GLOBAL_T") == 0){
				conf >> global_t;
			}else if (key.compare("MI_THRESH") == 0){
				conf >> mi_thresh;
			}else if (key.compare("VAR_WIN") == 0){
				uint32_t v;
				conf >> v;
				var_win = v;
			}else if (key.compare("FAST_REACT") == 0){
				uint32_t v;
				conf >> v;
				fast_react = v;
			}else if (key.compare("U_TARGET") == 0){
				conf >> u_target;
			}else if (key.compare("INT_MULTI") == 0){
				conf >> int_multi;
			}else if (key.compare("RATE_BOUND") == 0){
				uint32_t v;
				conf >> v;
				rate_bound = v;
			}else if (key.compare("ACK_HIGH_PRIO") == 0){
				conf >> ack_high_prio;
			}else if (key.compare("DCTCP_RATE_AI") == 0){
				conf >> dctcp_rate_ai;
      } else if (key.compare("NIC_TOTAL_PAUSE_TIME") == 0) {
        conf >> nic_total_pause_time;
      } else if (key.compare("PFC_OUTPUT_FILE") == 0) {
				conf >> pfc_output_file;
			}else if (key.compare("LINK_DOWN") == 0){
				conf >> link_down_time >> link_down_A >> link_down_B;
			}else if (key.compare("ENABLE_TRACE") == 0){
				conf >> enable_trace;
			}else if (key.compare("KMAX_MAP") == 0){
				int n_k ;
				conf >> n_k;
				for (int i = 0; i < n_k; i++){
					uint64_t rate;
					uint32_t k;
					conf >> rate >> k;
					rate2kmax[rate] = k;
				}
			}else if (key.compare("KMIN_MAP") == 0){
				int n_k ;
				conf >> n_k;
				for (int i = 0; i < n_k; i++){
					uint64_t rate;
					uint32_t k;
					conf >> rate >> k;
					rate2kmin[rate] = k;
				}
			}else if (key.compare("PMAX_MAP") == 0){
				int n_k ;
				conf >> n_k;
				for (int i = 0; i < n_k; i++){
					uint64_t rate;
					double p;
					conf >> rate >> p;
					rate2pmax[rate] = p;
				}
			}else if (key.compare("BUFFER_SIZE") == 0){
				conf >> buffer_size;
			}else if (key.compare("QLEN_MON_FILE") == 0){
				conf >> qlen_mon_file;
			}else if (key.compare("QLEN_MON_START") == 0){
				conf >> qlen_mon_start;
			}else if (key.compare("QLEN_MON_END") == 0){
				conf >> qlen_mon_end;
			}else if (key.compare("MULTI_RATE") == 0){
				int v;
				conf >> v;
				multi_rate = v;
			}else if (key.compare("SAMPLE_FEEDBACK") == 0){
				int v;
				conf >> v;
				sample_feedback = v;
			}else if(key.compare("PINT_LOG_BASE") == 0){
				conf >> pint_log_base;
			}else if (key.compare("PINT_PROB") == 0){
				conf >> pint_prob;
			}else if (key.compare("SFC_QUEUE_NUM")==0){
                conf >> sfc_queue_num;
                std::cout << "SFC_QUEUE_NUM\t\t\t\t" << sfc_queue_num << "\n";
            }else if (key.compare("FC_MODE") == 0){
				conf >> fc_mode;
				std::cout << "FC_MODE\t\t" << fc_mode << '\n';
            }else if (key.compare("LC_MODE") == 0){
				conf >> lc_mode;
				std::cout << "LC_MODE\t\t" << lc_mode << '\n';
            }else if (key.compare("SFC_TRIGGER") == 0){
				conf >> sfc_trigger_threshold;
				std::cout << "SFC_TRIGGER\t\t" << sfc_trigger_threshold << '\n';
            }else if (key.compare("SFC_TARGET_QUEUE_DEPTH") == 0){
				conf >> sfc_target_queue_depth;
				std::cout << "SFC_TARGET_QUEUE_DEPTH\t\t" << sfc_target_queue_depth << '\n';
            }else if (key.compare("SFC_OPTIMIZATION") == 0){
				conf >> sfc_opt;
				std::cout << "SFC_OPTMIZAITON\t\t" << sfc_opt << '\n';
            } else if (key.compare("SWC_QOS_MAP") == 0) {
				std::string swc_qos_map_raw;
				conf >> swc_qos_map_raw;

				std::string delim = ",";
				size_t pos = 0;

				while ((pos = swc_qos_map_raw.find(delim)) != string::npos) {
					qos_map_parse_elem(swc_qos_map,
									   swc_qos_map_raw.substr(0, pos));
					swc_qos_map_raw.erase(0, pos + 1);
				}
				qos_map_parse_elem(swc_qos_map,
								   swc_qos_map_raw);

				std::cout << "SWC_QOS_MAP\t\t"; 
				for (auto const& elem : swc_qos_map) {
					std::cout << (uint32_t)elem.first
							  << "->"
							  << (uint32_t)elem.second
							  << ",";
				}
				std::cout << std::endl;
            } else if (key.compare("FLOW_QOS_MAP") == 0) {
				std::string flow_qos_map_raw;
				conf >> flow_qos_map_raw;

				std::string delim = ",";
				size_t pos = 0;

				while ((pos = flow_qos_map_raw.find(delim)) != string::npos) {
					qos_map_parse_elem(flow_qos_map,
									   flow_qos_map_raw.substr(0, pos));
					flow_qos_map_raw.erase(0, pos + 1);
				}
				qos_map_parse_elem(flow_qos_map,
								   flow_qos_map_raw);

				std::cout << "FLOW_QOS_MAP\t\t"; 
				for (auto const& elem : flow_qos_map) {
					std::cout << (uint32_t)elem.first
							  << "->"
							  << (uint32_t)elem.second
							  << ",";
				}
				std::cout << std::endl;
            }else if (key.compare("SP_THRESHOLD") == 0){
				int64_t v;
				conf >> v;
				SPthres = v;
				std::cout << "SP_THRESHOLD\t\t\t\t" << SPthres << '\n';
			}else if (key.compare("SP_MAX_PAUSE_LEN") == 0){
				int64_t v;
				conf >> v;
				SPmaxPauseLen = v;
				std::cout << "SP_MAX_PAUSE_LEN\t\t\t\t" << SPmaxPauseLen << '\n';
			}else if (key.compare("SP_NO_PAUSE_PERIOD_RATIO") == 0){
				double v;
				conf >> v;
				SP_noPausePeriodRatio = v;
				std::cout << "SP_NO_PAUSE_PERIOD_RATIO\t\t" << SP_noPausePeriodRatio << "\n";
			}else if (key.compare("ENABLE_CORRECTION_IN_SP") == 0){
				int v;
				conf >> v;
				enableSPcor = v;
				std::cout << "ENABLE_CORRECTION_IN_SP\t\t\t\t" << enableSPcor << '\n';
			}else if (key.compare("SP_CORRECTION_GAIN") == 0){
				double v;
				conf >> v;
				SPcorGain = v;
				std::cout << "SP_CORRECTION_GAIN\t\t" << SPcorGain << "\n";
			}else if (key.compare("SP_CLOCK_OFFSET_STD") == 0){
				double v;
				conf >> v;
				clockOffsetStd = v;
				std::cout << "SP_CLOCK_OFFSET_STD\t\t" << clockOffsetStd << "\n";
			}else if (key.compare("JOB_CONFIG") == 0){
				std::string v;
				conf >> v;
				job_config = v;
				std::cout << "JOB_CONFIG\t\t" << job_config << "\n";
			}else if (key.compare("SYS_CONFIG") == 0){
				std::string v;
				conf >> v;
				sys_config = v;
				std::cout << "SYS_CONFIG\t\t" << sys_config << "\n";
			}
			
			fflush(stdout);
		}
		return true;
	}
	else
	{
		std::cout << "Error: require a config file\n";
		fflush(stdout);
		return false;
	}
}

void SetConfig(){
	bool dynamicth = use_dynamic_pfc_threshold;

	Config::SetDefault("ns3::QbbNetDevice::SfcQCnt",UintegerValue(sfc_queue_num));
	Config::SetDefault("ns3::QbbNetDevice::PauseTime", UintegerValue(pause_time));
	Config::SetDefault("ns3::QbbNetDevice::QcnEnabled", BooleanValue(enable_qcn));
	Config::SetDefault("ns3::QbbNetDevice::DynamicThreshold", BooleanValue(dynamicth));

	// set int_multi
	IntHop::multi = int_multi;
	// IntHeader::mode
	if (cc_mode == 7) // timely, use ts
		IntHeader::mode = IntHeader::TS;
	else if (cc_mode == 3) // hpcc, use int
		IntHeader::mode = IntHeader::NORMAL;
	else if (cc_mode == 10) // hpcc-pint
		IntHeader::mode = IntHeader::PINT;
	else // others, no extra header
		IntHeader::mode = IntHeader::NONE;

	// Set Pint
	if (cc_mode == 10){
		Pint::set_log_base(pint_log_base);
		IntHeader::pint_bytes = Pint::get_n_bytes();
		printf("PINT bits: %d bytes: %d\n", Pint::get_n_bits(), Pint::get_n_bytes());
	}
}

void SetupNetwork(void (*qp_finish)(FILE*, Ptr<RdmaQueuePair>)){


	topof.open(topology_file.c_str());
	flowf.open(flow_file.c_str());
	tracef.open(trace_file.c_str());
	uint32_t node_num, switch_num, link_num, trace_num;
	topof >> node_num >> switch_num >> link_num;
	flowf >> flow_num;
	tracef >> trace_num;

	std::vector<uint32_t> node_type(node_num, 0);
	for (uint32_t i = 0; i < switch_num; i++)
	{
		uint32_t sid;
		topof >> sid;
		node_type[sid] = 1;
	}
	for (uint32_t i = 0; i < node_num; i++){
		if (node_type[i] == 0)
			n.Add(CreateObject<Node>());
		else{
			Ptr<SwitchNode> sw = CreateObject<SwitchNode>();
			n.Add(sw);
			sw->SetAttribute("EcnEnabled", BooleanValue(enable_qcn));
			sw->SetAttribute("PacketSprayEnabled", BooleanValue(enable_packetspray));
		}
	}

	NS_LOG_INFO("Create nodes.");

	InternetStackHelper internet;
	internet.Install(n);

	//
	// Assign IP to each server
	//
	for (uint32_t i = 0; i < node_num; i++){
		if (n.Get(i)->GetNodeType() == 0){ 
			serverAddress.resize(i + 1);
			serverAddress[i] = node_id_to_ip(i);
		}
	}

	NS_LOG_INFO("Create channels.");

	Ptr<RateErrorModel> rem = CreateObject<RateErrorModel>();
	Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable>();
	rem->SetRandomVariable(uv);
	uv->SetStream(50);
	rem->SetAttribute("ErrorRate", DoubleValue(error_rate_per_link));
	rem->SetAttribute("ErrorUnit", StringValue("ERROR_UNIT_PACKET"));

	FILE *pfc_file = fopen(pfc_output_file.c_str(), "w");

	QbbHelper qbb;
	Ipv4AddressHelper ipv4;
	for (uint32_t i = 0; i < link_num; i++)
	{
		uint32_t src, dst;
		std::string data_rate, link_delay;
		double error_rate;
		topof >> src >> dst >> data_rate >> link_delay >> error_rate;
		Ptr<Node> snode = n.Get(src), dnode = n.Get(dst);

		qbb.SetDeviceAttribute("DataRate", StringValue(data_rate));
		qbb.SetChannelAttribute("Delay", StringValue(link_delay));

		if (error_rate > 0)
		{
			Ptr<RateErrorModel> rem = CreateObject<RateErrorModel>();
			Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable>();
			rem->SetRandomVariable(uv);
			uv->SetStream(50);
			rem->SetAttribute("ErrorRate", DoubleValue(error_rate));
			rem->SetAttribute("ErrorUnit", StringValue("ERROR_UNIT_PACKET"));
			qbb.SetDeviceAttribute("ReceiveErrorModel", PointerValue(rem));
		}
		else
		{
			qbb.SetDeviceAttribute("ReceiveErrorModel", PointerValue(rem));
		}

		fflush(stdout);

		// Assigne server IP
		// Note: this should be before the automatic assignment below (ipv4.Assign(d)),
		// because we want our IP to be the primary IP (first in the IP address list),
		// so that the global routing is based on our IP
		NetDeviceContainer d = qbb.Install(snode, dnode);
		if (snode->GetNodeType() == 0){
			Ptr<Ipv4> ipv4 = snode->GetObject<Ipv4>();
			ipv4->AddInterface(d.Get(0));
			ipv4->AddAddress(1, Ipv4InterfaceAddress(serverAddress[src], Ipv4Mask(0xff000000)));
            
            Ptr<SwitchNode> sw = DynamicCast<SwitchNode>(dnode);            //Destination Node is switch
			sw->SetAttribute("IsEdge", BooleanValue(true));
			// This is hardcoded,  for santity check, based on my knowledge for the topology.
			// when we use sfc_fat.txt as the topoplogy, the ToR switches numbers are 0-20;
			// when we use fat_hpcc.txt as the topology, the ToR switches numbers are 320-339;
			// if (dst > 339 || dst < 320)
			// {
			// 	std::cout << "ToR switches numbering is wrong" << std::endl;
			// 	// return -1;
            //     exit(-1);
			// }
		}
		if (dnode->GetNodeType() == 0){
			Ptr<Ipv4> ipv4 = dnode->GetObject<Ipv4>();
			ipv4->AddInterface(d.Get(1));
			ipv4->AddAddress(1, Ipv4InterfaceAddress(serverAddress[dst], Ipv4Mask(0xff000000)));
            Ptr<SwitchNode> sw = DynamicCast<SwitchNode>(snode);            //Destination Node is switch
			sw->SetAttribute("IsEdge", BooleanValue(true));
			/*
			if (src > 339 || dst < 320)
			{
				std::cout << "ToR switches numbering is wrong" << std::endl;
				return -1;
                exit(-1);
			}
			*/
		}

		// used to create a graph of the topology
		nbr2if[snode][dnode].idx = DynamicCast<QbbNetDevice>(d.Get(0))->GetIfIndex();
		nbr2if[snode][dnode].up = true;
		nbr2if[snode][dnode].delay = DynamicCast<QbbChannel>(DynamicCast<QbbNetDevice>(d.Get(0))->GetChannel())->GetDelay().GetTimeStep();
		nbr2if[snode][dnode].bw = DynamicCast<QbbNetDevice>(d.Get(0))->GetDataRate().GetBitRate();
		nbr2if[dnode][snode].idx = DynamicCast<QbbNetDevice>(d.Get(1))->GetIfIndex();
		nbr2if[dnode][snode].up = true;
		nbr2if[dnode][snode].delay = DynamicCast<QbbChannel>(DynamicCast<QbbNetDevice>(d.Get(1))->GetChannel())->GetDelay().GetTimeStep();
		nbr2if[dnode][snode].bw = DynamicCast<QbbNetDevice>(d.Get(1))->GetDataRate().GetBitRate();

		// This is just to set up the connectivity between nodes. The IP addresses are useless
		char ipstring[16];
		sprintf(ipstring, "10.%d.%d.0", i / 254 + 1, i % 254 + 1);
		ipv4.SetBase(ipstring, "255.255.255.0");
		ipv4.Assign(d);

		// setup PFC trace
		DynamicCast<QbbNetDevice>(d.Get(0))->TraceConnectWithoutContext("QbbPfc", MakeBoundCallback (&get_pfc, pfc_file, DynamicCast<QbbNetDevice>(d.Get(0))));
		DynamicCast<QbbNetDevice>(d.Get(1))->TraceConnectWithoutContext("QbbPfc", MakeBoundCallback (&get_pfc, pfc_file, DynamicCast<QbbNetDevice>(d.Get(1))));
	}

	nic_rate = get_nic_rate(n);
	// config switch
	for (uint32_t i = 0; i < node_num; i++){
		if (n.Get(i)->GetNodeType() == 1){ // is switch
			Ptr<SwitchNode> sw = DynamicCast<SwitchNode>(n.Get(i));
			uint32_t shift = 3; // by default 1/8

			for (uint32_t j = 1; j < sw->GetNDevices(); j++){
				Ptr<QbbNetDevice> dev = DynamicCast<QbbNetDevice>(sw->GetDevice(j));
				// set ecn
				uint64_t rate = dev->GetDataRate().GetBitRate();
				NS_ASSERT_MSG(rate2kmin.find(rate) != rate2kmin.end(), "must set kmin for each link speed");
				NS_ASSERT_MSG(rate2kmax.find(rate) != rate2kmax.end(), "must set kmax for each link speed");
				NS_ASSERT_MSG(rate2pmax.find(rate) != rate2pmax.end(), "must set pmax for each link speed");
                sw->m_mmu->ConfigEcn(j, rate2kmin[rate], rate2kmax[rate], rate2pmax[rate]);
				// set pfc
				uint64_t delay = DynamicCast<QbbChannel>(dev->GetChannel())->GetDelay().GetTimeStep();
				// uint32_t headroom = 150000; // rate * delay / 8 / 1000000000 * 3;
				uint32_t headroom = rate * delay / 8 / 1000000000 * 3;
				sw->m_mmu->ConfigHdrm(j, headroom);

				// set pfc alpha, proportional to link bw
				sw->m_mmu->pfc_a_shift[j] = shift;
				while (rate > nic_rate && sw->m_mmu->pfc_a_shift[j] > 0){
					sw->m_mmu->pfc_a_shift[j]--;
					rate /= 2;
				}
			}
			sw->m_mmu->ConfigNPort(sw->GetNDevices()-1);
			sw->m_mmu->ConfigBufferSize(buffer_size* 1024 * 1024);
			sw->m_mmu->node_id = sw->GetId();
		}
	}
 	std::default_random_engine ran_generator(1); // Seed == 1
	std::normal_distribution<double> std_dist(0.0, clockOffsetStd);

	srand (1);
	#if ENABLE_QP
	FILE *fct_output = fopen(fct_output_file.c_str(), "w");
	std::cout << "QP is enabled " << std::endl;
	jobf.open(job_config.c_str());

	int job_num = 1;
  	int num_gpus = 1;
  	int total_pause_times = 0; 
	double communication_scale_factor = 1.0;
	int num_passes = 1;
	jobf >> job_num >> num_gpus >> total_pause_times >> communication_scale_factor >> num_passes;
	jobf.close();
	//
	// install RDMA driver
	//
	for (uint32_t i = 0; i < node_num; i++){
		if (n.Get(i)->GetNodeType() == 0){ // is server
			// create RdmaHw
			Ptr<RdmaHw> rdmaHw = CreateObject<RdmaHw>();
			rdmaHw->SetAttribute("ClampTargetRate", BooleanValue(clamp_target_rate));
			rdmaHw->SetAttribute("AlphaResumInterval", DoubleValue(alpha_resume_interval));
			rdmaHw->SetAttribute("RPTimer", DoubleValue(rp_timer));
			rdmaHw->SetAttribute("FastRecoveryTimes", UintegerValue(fast_recovery_times));
			rdmaHw->SetAttribute("EwmaGain", DoubleValue(ewma_gain));
			rdmaHw->SetAttribute("RateAI", DataRateValue(DataRate(rate_ai)));
			rdmaHw->SetAttribute("RateHAI", DataRateValue(DataRate(rate_hai)));
			rdmaHw->SetAttribute("L2BackToZero", BooleanValue(l2_back_to_zero));
      rdmaHw->SetAttribute(
          "TotalPauseTimes", UintegerValue(nic_total_pause_time));
			rdmaHw->SetAttribute("L2ChunkSize", UintegerValue(l2_chunk_size));
			rdmaHw->SetAttribute("L2AckInterval", UintegerValue(l2_ack_interval));
			rdmaHw->SetAttribute("CcMode", UintegerValue(cc_mode));
			rdmaHw->SetAttribute("RateDecreaseInterval", DoubleValue(rate_decrease_interval));
			rdmaHw->SetAttribute("MinRate", DataRateValue(DataRate(min_rate)));
			rdmaHw->SetAttribute("Mtu", UintegerValue(packet_payload_size));
			rdmaHw->SetAttribute("MiThresh", UintegerValue(mi_thresh));
			rdmaHw->SetAttribute("VarWin", BooleanValue(var_win));
			rdmaHw->SetAttribute("FastReact", BooleanValue(fast_react));
			rdmaHw->SetAttribute("MultiRate", BooleanValue(multi_rate));
			rdmaHw->SetAttribute("SampleFeedback", BooleanValue(sample_feedback));
			rdmaHw->SetAttribute("TargetUtil", DoubleValue(u_target));
			rdmaHw->SetAttribute("RateBound", BooleanValue(rate_bound));
			rdmaHw->SetAttribute("DctcpRateAI", DataRateValue(DataRate(dctcp_rate_ai)));
			rdmaHw->SetPintSmplThresh(pint_prob);
			rdmaHw->SetAttribute("IRNEnabled", BooleanValue(lc_mode));
			rdmaHw->SetAttribute("TotalPauseTimes", UintegerValue(total_pause_times));
			// rdmaHw->SetAttribute("SfcOptimizationEnabled", UintegerValue(sfc_opt));
			// rdmaHw->SetAttribute("SfcQCnt", UintegerValue(sfc_queue_num));
			if(fc_mode == 5 ){
				enableSP = true;
				rdmaHw->SetAttribute("EnableSP", BooleanValue(enableSP));
				rdmaHw->SetAttribute("SPthreshold", UintegerValue(SPthres));
				rdmaHw->SetAttribute("SPmaxPauseLen", UintegerValue(SPmaxPauseLen));
				rdmaHw->SetAttribute("SPnoPausePeriodRatio", DoubleValue(SP_noPausePeriodRatio));
				rdmaHw->SetAttribute("EnableSPCorrection", BooleanValue(enableSPcor));
				rdmaHw->SetAttribute("SPcorGain", DoubleValue(SPcorGain));

				double thisClockOffset = std_dist(ran_generator);
				rdmaHw->SetAttribute("ClockOffset", DoubleValue(thisClockOffset));
				std::cout << "Node " << i << " ClockOffset " << thisClockOffset <<'\n';	
			}

			// create and install RdmaDriver
			Ptr<RdmaDriver> rdma = CreateObject<RdmaDriver>();
			Ptr<Node> node = n.Get(i);
			rdma->SetNode(node);
			rdma->SetRdmaHw(rdmaHw);

			node->AggregateObject (rdma);
			rdma->Init();
			rdma->TraceConnectWithoutContext("QpComplete", MakeBoundCallback (qp_finish, fct_output));
		}
	}
	#endif

	// set ACK priority on hosts
	if (ack_high_prio)
		RdmaEgressQueue::ack_q_idx = 0;
	else
		RdmaEgressQueue::ack_q_idx = 3;

	// setup routing
	CalculateRoutes(n);
	SetRoutingEntries();

	//
	// get BDP and delay
	//
	maxRtt = maxBdp = 0;
	for (uint32_t i = 0; i < node_num; i++){
		if (n.Get(i)->GetNodeType() != 0)
			continue;
		for (uint32_t j = 0; j < node_num; j++){
			if (n.Get(j)->GetNodeType() != 0)
				continue;
			uint64_t delay = pairDelay[n.Get(i)][n.Get(j)];
			uint64_t txDelay = pairTxDelay[n.Get(i)][n.Get(j)];
			uint64_t rtt = delay * 2 + txDelay;
			uint64_t bw = pairBw[i][j];
			uint64_t bdp = rtt * bw / 1000000000/8; 
			// if(i == 0 && j== 17){
			// 	std::cout << "i " << i << " j " << j << " delay " << delay << " txDelay " << txDelay << std::endl;
			// }

			pairBdp[n.Get(i)][n.Get(j)] = bdp;
			pairRtt[i][j] = rtt;
			if (bdp > maxBdp)
				maxBdp = bdp;
			if (rtt > maxRtt)
				maxRtt = rtt;
		}
	}
	printf("maxRtt=%lu maxBdp=%lu\n", maxRtt, maxBdp);

	//
	// setup switch CC
	//
	for (uint32_t i = 0; i < node_num; i++){
		if (n.Get(i)->GetNodeType() == 1){ // switch
			Ptr<SwitchNode> sw = DynamicCast<SwitchNode>(n.Get(i));
			sw->SetAttribute("CcMode", UintegerValue(cc_mode));
			sw->SetAttribute("FcMode", UintegerValue(fc_mode));
			sw->SetAttribute("SfcQueueNum", UintegerValue(sfc_queue_num));
			sw->SetAttribute("TargetQueueDepth", UintegerValue(sfc_target_queue_depth));
			sw->SetAttribute("SfcTriggerThreshold", UintegerValue(sfc_trigger_threshold));
			sw->SetAttribute("SfcOptimizationEnabled", UintegerValue(sfc_opt));
			sw->SetAttribute("MaxRtt", UintegerValue(maxRtt));
			sw->SetAttribute("AckHighPrio", UintegerValue(ack_high_prio));

			sw->SetQosMap(swc_qos_map);
		}
	}

	//
	// add trace
	//

	NodeContainer trace_nodes;
	for (uint32_t i = 0; i < trace_num; i++)
	{
		uint32_t nid;
		tracef >> nid;
		if (nid >= n.GetN()){
			continue;
		}
		trace_nodes = NodeContainer(trace_nodes, n.Get(nid));
	}

	FILE *trace_output = fopen(trace_output_file.c_str(), "w");
	if (enable_trace)
		qbb.EnableTracing(trace_output, trace_nodes);

	// dump link speed to trace file
	{
		SimSetting sim_setting;
		for (auto i: nbr2if){
			for (auto j : i.second){
				uint16_t node = i.first->GetId();
				uint8_t intf = j.second.idx;
				uint64_t bps = DynamicCast<QbbNetDevice>(i.first->GetDevice(j.second.idx))->GetDataRate().GetBitRate();
				sim_setting.port_speed[node][intf] = bps;
			}
		}
		sim_setting.win = maxBdp;
		sim_setting.Serialize(trace_output);
	}

	Ipv4GlobalRoutingHelper::PopulateRoutingTables();

	NS_LOG_INFO("Create Applications.");

	Time interPacketInterval = Seconds(0.0000005 / 2);
	// maintain port number for each host
	for (uint32_t i = 0; i < node_num; i++){
		if (n.Get(i)->GetNodeType() == 0)
			for (uint32_t j = 0; j < node_num; j++){
				if (n.Get(j)->GetNodeType() == 0)
					portNumber[i][j] = 10000; // each host pair use port number from 10000
			}
	}

	topof.close();
	tracef.close();

	// schedule link down
	if (link_down_time > 0){
		Simulator::Schedule(Seconds(2) + MicroSeconds(link_down_time), &TakeDownLink, n, n.Get(link_down_A), n.Get(link_down_B));
	}

	// schedule buffer monitor
	FILE* qlen_output = fopen(qlen_mon_file.c_str(), "w");
	Simulator::Schedule(NanoSeconds(qlen_mon_start), &monitor_buffer, qlen_output, &n);
}