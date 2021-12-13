nodes = 137
switch_nodes = 9
links = 136
latency = "0.0005ms"
bandwidth = "200Gbps"
error_rate = "0"
per_switch_node  = 16
file_name = str(nodes)+"_nodes_"+str(switch_nodes)+"_switch_topology.txt"
with open(file_name, 'w') as f:
    first_line = str(nodes)+" "+str(switch_nodes)+" "+str(links)
    f.write(first_line)
    f.write('\n')
    switch_list = []
    sec_line = ""
    for i in range(switch_nodes):
        sec_line = sec_line + str(i) + " "
        if i!=0:
            switch_list.append(i)
    f.write(sec_line)
    f.write('\n')
    node = switch_nodes+1;
    for switch in switch_list:
        if switch==0:
            continue
        for i in range(node,node+per_switch_node):
            line = str(switch)+" "+str(i)+" "+bandwidth+" "+latency+" "+error_rate
            f.write(line)
            f.write('\n')
        node = node + per_switch_node
    for switch in switch_list:
        if switch == 0:
            continue
        line = str(0)+" "+str(switch)+" "+bandwidth+" "+latency+" "+error_rate
        f.write(line)
        f.write('\n')


