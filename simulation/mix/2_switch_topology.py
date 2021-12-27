nodes = 7
switch_nodes = 3
links = 8
latency = "0.0005ms"
bandwidth = "200Gbps"
error_rate = "0"
per_switch_node  = 2
file_name = str(nodes)+"_nodes_"+str(switch_nodes)+"_switch_topology.txt"
with open(file_name, 'w') as f:
    first_line = str(nodes)+" "+str(switch_nodes)+" "+str(links)
    f.write(first_line)
    f.write('\n')
    switch_list = []
    sec_line = ""
    nnodes = nodes - switch_nodes
    tor = nodes-1
    for i in range(nnodes, nnodes+switch_nodes):
        sec_line = sec_line + str(i) + " "
        if i!= tor:
            switch_list.append(i)
    f.write(sec_line)
    f.write('\n')
    node = 0;
    #print(switch_list)
    for switch in switch_list:
        for i in range(node, node+per_switch_node):
            line = str(switch)+" "+str(i)+" "+bandwidth+" "+latency+" "+error_rate
            f.write(line)
            f.write('\n')
            line = str(tor)+" "+str(i)+" "+bandwidth+" "+latency+" "+error_rate
            f.write(line)
            f.write('\n')
        node = node + per_switch_node
