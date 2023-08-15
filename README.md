# NS3-Interface
This repository contains an NS3-based network simulator that acts as a network backend for ASTRA-sim. 
ASTRA-sim is a distributed machine learning system simulator, developed as a joint collaboration between Georgia Tech, Meta, and Intel.
For more details on ASTRA-sim, please refer to [the ASTRA-sim repository](https://github.com/astra-sim/astra-sim)

This repository is extended from [https://github.com/alibaba-edu/High-Precision-Congestion-Control](https://github.com/alibaba-edu/High-Precision-Congestion-Control), and simulates various congestion control algorithms. 
With ASTRA-sim this repository was used to study the performance of different network congestion control algorithms in largescale collective communications in multi-gpu scenarios.
More details on the study can be found in our paper:

T. Khan, S. Rashidi, S. Sridharan, P. Shurpali, A. Akella and T. Krishna, "Impact of RoCE Congestion Control Policies on Distributed Training of DNNs," In proceedings of the IEEE Symposium on High-Performance Interconnects (HOTI), 2022 [[pdf]](https://arxiv.org/abs/2207.10898)

> This repository is only compatible with a previous version of ASTRA-sim \(Astra-sim 1.0\), which is available in the `ASTRA-sim-1.0` branch of the ASTRA-sim repository. A new version of this repository to fit with ASTRA-sim 2.0 is being developed. This repository will be maintained for backward compatibility purposes.
> This version of ns3 requires gcc-5, g++-5, and python2.7. It also requires the `python` command to default to python2.7. 


## Instructions to build and run with this repository
This repository is used as a submodule in the ASTRA-sim repository. 
Instead of cloning this repository directly, please refer to the build instructions in [the ASTRA-sim repository](https://github.com/astra-sim/astra-sim/tree/ASTRA-sim-1.0#instructions-for-compiling--running-ns3-as-the-network-simulator)

## Input Description
This section describes the knobs to tweak the NS3 network backend. 

### Network Topology
The network topology files are located in `simulation/mix`.
Let's take a sample file, `7_nodes_3_switch_topology.txt`, and examine its components. 
This file describes a topology where there are 7 nodes, 3 of which are network switches (4, 5, 6). There are 8 links in total. 

```
7 3 8 // {no. of nodes}  {no. of switches}  {no. of links}
4 5 6 // {List of node id for switches}
4 0 200Gbps 0.0005ms 0 // From here on each line corresponds to one link.
6 0 200Gbps 0.0005ms 0 // {id of one endpoint} {id of another endpoint} {bandwidth} {latency} {error rate of link}
4 1 200Gbps 0.0005ms 0
6 1 200Gbps 0.0005ms 0
5 2 200Gbps 0.0005ms 0
6 2 200Gbps 0.0005ms 0
5 3 200Gbps 0.0005ms 0
6 3 200Gbps 0.0005ms 0

```

### Configuration 
The execution script in the ASTRA-sim repository (`astra-sim/network/frontend/build.sh`) designates which configuration file to use. 
Refer to [this spreadsheet](https://docs.google.com/spreadsheets/d/1Xoo_QWgOuEJojnCOv-znwzARLjFArlR6tULKA2CBIqQ/edit?usp=sharing) for a detailed description of each knobs. 



## Contact Us
This project is a collaboration of dedicated professionals. Each core developer and contributor plays a unique role in the project. For any inquiries or questions, feel free to reach out to the corresponding developer based on their expertise. 
| Developer | Organization | Responsibility | Contact |
|-----------|--------------|----------------|---------|
| Tarannum Khan | Netflix | Development, connecting with ASTRA-sim 1.0 | tkhaniitr@gmail.com |
| Saeed Rashidi | Hewlett Packard Labs | ASTRA-sim 1.0 | rashidi1saeid@gmail.com |
| Jinsun Yoo | Georgia Tech | Connection with ASTRA-sim 2.0, Current Inquiries| jinsun@gatech.edu |
| Tushar Krishna | Georgia Tech | General inquiries | tushar@ece.gatech.edu |
