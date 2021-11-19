nodes = 65
switch_nodes = 1
links = 64
latency = "0.0005ms"
bandwidth = "200Gbps"
error_rate = "0"
file_name = str(nodes)+"_nodes_"+str(switch_nodes)+"_switch_topology.txt"
with open(file_name, 'w') as f:
    first_line = str(nodes)+" "+str(switch_nodes)+" "+str(links)
    f.write(first_line)
    f.write('\n')
    f.write(str(links))
    f.write('\n')
    for i in range(links):
        line = str(links)+" "+str(i)+" "+bandwidth+" "+latency+" "+error_rate
        f.write(line)
        f.write('\n')

