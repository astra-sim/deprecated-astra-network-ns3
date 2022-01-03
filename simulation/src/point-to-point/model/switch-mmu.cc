#include <iostream>
#include <fstream>
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/object-vector.h"
#include "ns3/uinteger.h"
#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3/global-value.h"
#include "ns3/boolean.h"
#include "ns3/simulator.h"
#include "ns3/random-variable.h"
#include "switch-mmu.h"

NS_LOG_COMPONENT_DEFINE("SwitchMmu");
namespace ns3 {
	TypeId SwitchMmu::GetTypeId(void){
		static TypeId tid = TypeId("ns3::SwitchMmu")
			.SetParent<Object>()
			.AddConstructor<SwitchMmu>();
		return tid;
	}

	SwitchMmu::SwitchMmu(void){
		buffer_size = 12 * 1024 * 1024;
		reserve = 4 * 1024;
		resume_offset = 3 * 1024;

		// headroom
		shared_used_bytes = 0;
		memset(hdrm_bytes, 0, sizeof(hdrm_bytes));
		memset(ingress_bytes, 0, sizeof(ingress_bytes));
		memset(paused, 0, sizeof(paused));
		memset(egress_bytes, 0, sizeof(egress_bytes));
	}
	bool SwitchMmu::CheckIngressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize){
		//std::cout<<"buffer size in check is "<<buffer_size<<"\n";
		std::cout<<"check ingress psize hdrm_bytes[port][qindex] headrooom[port] getsharedused(port, qindex), getpfcthreshold(port) for port qindex "<<psize<<" "<<hdrm_bytes[port][qIndex]<<" "<<headroom[port]<<" "<<GetSharedUsed(port, qIndex)<<" "<<GetPfcThreshold(port)<<" "<<port<<" "<<qIndex<<"\n";
		if (psize + hdrm_bytes[port][qIndex] > headroom[port] && psize + GetSharedUsed(port, qIndex) > GetPfcThreshold(port)){
			printf("%lu %u Drop: queue:%u,%u: Headroom full\n", Simulator::Now().GetTimeStep(), node_id, port, qIndex);
			for (uint32_t i = 1; i < 64; i++)
				printf("(%u,%u)", hdrm_bytes[i][3], ingress_bytes[i][3]);
			printf("\n");
			return false;
		}
		return true;
	}
	bool SwitchMmu::CheckEgressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize){
		return true;
	}
	void SwitchMmu::UpdateIngressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize){
		uint32_t new_bytes = ingress_bytes[port][qIndex] + psize;
		std::cout<<"update ingress new bytes reserve shared_used_bytes are "<<new_bytes<<" "<<reserve<<" "<<shared_used_bytes<<"\n";
		if (new_bytes <= reserve){
			ingress_bytes[port][qIndex] += psize;
			std::cout<<"current byte "<<new_bytes<<" less than reserve "<<reserve<<"\n";
		}else {
			uint32_t thresh = GetPfcThreshold(port);
			std::cout<<"reserve + thresh = "<<reserve+thresh<<"\n";
			if (new_bytes - reserve > thresh){
				std::cout<<"total bytes greater than resrve+thresh goes into headroom\n";
				hdrm_bytes[port][qIndex] += psize;

			}else {
				std::cout<<"total bytes less than reserve+thresh goes into ingress and shared per port byte\n";
				ingress_bytes[port][qIndex] += psize;
				shared_used_bytes += std::min(psize, new_bytes - reserve);
			}
		}
		std::cout<<"in update ingress hdrm_bytes[port][qindex] shared_used_bytes ingress_bytes[port][qIndex] "<< hdrm_bytes[port][qIndex]<<" "<<shared_used_bytes<<" "<<ingress_bytes[port][qIndex]<<" "<<port<<" "<<qIndex<<"\n";
	}
	void SwitchMmu::UpdateEgressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize){
		egress_bytes[port][qIndex] += psize;
	}
	void SwitchMmu::RemoveFromIngressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize){
		std::cout<<"ingress port and queue is "<<port<<" "<<qIndex<<"\n";
		uint32_t from_hdrm = std::min(hdrm_bytes[port][qIndex], psize);
		uint32_t from_shared = std::min(psize - from_hdrm, ingress_bytes[port][qIndex] > reserve ? ingress_bytes[port][qIndex] - reserve : 0);
		hdrm_bytes[port][qIndex] -= from_hdrm;
		ingress_bytes[port][qIndex] -= psize - from_hdrm;
		shared_used_bytes -= from_shared;
	}
	void SwitchMmu::RemoveFromEgressAdmission(uint32_t port, uint32_t qIndex, uint32_t psize){
		std::cout<<"egress port and queue is "<<port<<" "<<qIndex<<"\n";
		egress_bytes[port][qIndex] -= psize;
	}
	bool SwitchMmu::CheckShouldPause(uint32_t port, uint32_t qIndex){
		return !paused[port][qIndex] && (hdrm_bytes[port][qIndex] > 0 || GetSharedUsed(port, qIndex) >= GetPfcThreshold(port));
	}
	bool SwitchMmu::CheckShouldResume(uint32_t port, uint32_t qIndex){
		if (!paused[port][qIndex])
			return false;
		uint32_t shared_used = GetSharedUsed(port, qIndex);
		return hdrm_bytes[port][qIndex] == 0 && (shared_used == 0 || shared_used + resume_offset <= GetPfcThreshold(port));
	}
	void SwitchMmu::SetPause(uint32_t port, uint32_t qIndex){
		paused[port][qIndex] = true;
	}
	void SwitchMmu::SetResume(uint32_t port, uint32_t qIndex){
		paused[port][qIndex] = false;
	}

	uint32_t SwitchMmu::GetPfcThreshold(uint32_t port){
		std::cout<<"get pfc threshold "<<buffer_size - total_hdrm - total_rsrv - shared_used_bytes<<"\n";
		return (buffer_size - total_hdrm - total_rsrv - shared_used_bytes) >> pfc_a_shift[port];
	}
	uint32_t SwitchMmu::GetSharedUsed(uint32_t port, uint32_t qIndex){
		uint32_t used = ingress_bytes[port][qIndex];
		return used > reserve ? used - reserve : 0;
	}
	bool SwitchMmu::ShouldSendCN(uint32_t ifindex, uint32_t qIndex){
		if (qIndex == 0)
			return false;
		if (egress_bytes[ifindex][qIndex] > kmax[ifindex]){
			std::cout<<"congestion send 1\n";
			return true;
		}
			//return true;
		if (egress_bytes[ifindex][qIndex] > kmin[ifindex]){
			std::cout<<"congestion send\n";
			double p = pmax[ifindex] * double(egress_bytes[ifindex][qIndex] - kmin[ifindex]) / (kmax[ifindex] - kmin[ifindex]);
			if (UniformVariable(0, 1).GetValue() < p)
				return true;
			else
				std::cout<<"congestion not send\n";
		}
	//	std::cout<<"not send cn \n";
		return false;
	}
	void SwitchMmu::ConfigEcn(uint32_t port, uint32_t _kmin, uint32_t _kmax, double _pmax){
		kmin[port] = _kmin * 1000;
		kmax[port] = _kmax * 1000;
		pmax[port] = _pmax;
	}
	void SwitchMmu::ConfigHdrm(uint32_t port, uint32_t size){
		headroom[port] = size;
	}
	void SwitchMmu::ConfigNPort(uint32_t n_port){
		total_hdrm = 0;
		total_rsrv = 0;
		for (uint32_t i = 1; i <= n_port; i++){
			total_hdrm += headroom[i];
			total_rsrv += reserve;
		}
	}
	void SwitchMmu::ConfigBufferSize(uint32_t size){
		buffer_size = size;
		std::cout<<"buffer size in configbuffersize is "<<buffer_size<<"\n";
	}
}
