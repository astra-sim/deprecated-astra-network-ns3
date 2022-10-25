nodes = 144 #128nodes+16switches
switch_nodes = 16 #8(tor) + 96(nvswitch 6 per server(8 gpus)) + 8(spine switch) + 0(route switch)
links = 192 #6(nvswitch)*8(gpus)*2(servers)*8(tors)+16(gpus)*8(tors)+8(tors)*8(spine)
latency = "0.0005ms"
bandwidth = "200Gbps"
error_rate = "0"
gpu = nodes-switch_nodes
nodes_per_nv_switch = 8
tor_switch_num = 8
level3_switch_num = 8 #spine_switch
agg_switch_num = 1 #0 for now
nv_switch_num = 96
nv_switch_per_tor = 12
tor_per_level3_switch = 8
level3_switch_per_agg = 0
nodes_per_tor = 16 #nodes_per_nv_switch * nv_switch_per_tor 
file_name = str(gpu)+"_nodes_"+str(switch_nodes)+"_switch_topology.txt"
print(file_name)
with open(file_name, 'w') as f:
    first_line = str(nodes)+" "+str(switch_nodes)+" "+str(links)
    f.write(first_line)
    f.write('\n')
    nv_switch = []
    tor_switch = []
    level3_switch = []
    agg_switch = []
    sec_line = ""
    nnodes = nodes - switch_nodes
    for i in range(nnodes, nodes):
        sec_line = sec_line + str(i) + " "
        #if len(nv_switch) < nv_switch_num:
        #    nv_switch.append(i)
        if len(tor_switch) < tor_switch_num:
            tor_switch.append(i)
        elif len(level3_switch) < level3_switch_num:
            level3_switch.append(i)
        #else:
        #    agg_switch.append(i)
    f.write(sec_line)
    f.write('\n')
    #print("nv switch ",nv_switch)
    #print("tor switch", tor_switch)
    #print("level 3 switch", level3_switch)
    #print("agg switch", agg_switch)
    ind_nv = 0
    ind_tor = 0
    ind_level3 = 0
    curr_node = 0
    tor_node = 0
    for i in range(gpu):
        curr_node = curr_node + 1
        tor_node = tor_node + 1
        #if curr_node > nodes_per_nv_switch:
        #    curr_node = 1
        #    ind_nv = ind_nv + 6
        if tor_node > nodes_per_tor:
            tor_node = 1
            ind_tor = ind_tor + 1
        #for ii in range(6): #nvswitches per gpu
         #   line = str(i)+" "+str(nv_switch[ind_nv+ii])+" 2400Gbps 0.000025ms "+error_rate
         #   f.write(line)
         #   f.write('\n')
        print("tor node and ind tor curr node {} {}",tor_node, ind_tor, curr_node)
        line = str(i)+" "+str(tor_switch[ind_tor])+" 200Gbps 0.005ms "+error_rate
        f.write(line)
        f.write('\n')
    for i in tor_switch:
        for j in level3_switch:
            line = str(i)+" "+str(j)+" 200Gbps 0.0125ms "+error_rate
            f.write(line)
            f.write('\n')
    #for i in level3_switch:
    #    line = str(i)+" "+str(agg_switch[0])+" 100Gbps 0.01ms "+error_rate
    #    f.write(line)
    #    f.write('\n')
