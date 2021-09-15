#include <iostream>
#include <string>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"

using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("myTCPMultiple");

int main (int argc, char *argv[])
{
    CsmaHelper csma;
    unsigned int MaxNodes = 4;
    unsigned int runtime = 3;
    unsigned int src = 0;
    unsigned int = 1;
    LogComponentEnable("OnOffApplication", LOG_LEVEL_INFO);
    LogComponentEnable("PacketSink", LOG_LEVEL_INFO);
    Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (2048));
    Config::SetDefault ("ns3::OnOffApplication::DataRate", StringValue ("8kbps"));

    CommandLine cmd;
    cmd.AddValue ("nodes", "Number of nodes in the network (must be > 1)", MaxNodes);
    cmd.AddValue ("runtime", "How long the applications should send data (default 3 seconds)", runtime);
    cmd.AddValue('source node', src);
    cmd.AddValue('destination node', dest);
    cmd.Parse (argc, argv);

    if (MaxNodes < 2)
    {
      std::cerr << "--nodes: must be >= 2" << std::endl;
      return 1;
    }
    csma.SetChannelAttribute ("DataRate", DataRateValue (DataRate (100 * 1000 * 1000)));
    csma.SetChannelAttribute ("Delay", TimeValue (MicroSeconds (200)));

    NodeContainer n;
    n.Create (MaxNodes);
    NetDeviceContainer ethInterfaces = csma.Install (n);

    InternetStackHelper internetStack;
    internetStack.Install(n);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase ("10.0.0.0", "255.255.255.0");
    Ipv4InterfaceContainer ipv4Interfaces = ipv4.Assign (ethInterfaces);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    uint16_t servPort = 8080;
    PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), servPort));

    ApplicationContainer sinkApp = sinkHelper.Install (n);
    sinkApp.Start (Seconds (0));
    sinkApp.Stop (Seconds (30.0));

    for (unsigned int i = 0; i < MaxNodes; i++)
    {
        for (unsigned int j = 0; j < MaxNodes; j++)
        {
            if (i == j)
            {
                continue;
            }
            Address remoteAddress (InetSocketAddress (ipv4Interfaces.GetAddress (j), servPort));
            OnOffHelper clientHelper ("ns3::TcpSocketFactory", remoteAddress);
            clientHelper.SetAttribute 
                ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
            clientHelper.SetAttribute 
                ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
            ApplicationContainer clientApp = clientHelper.Install (n.Get (i));
            clientApp.Start (Seconds (j)); 
            clientApp.Stop (Seconds (j + runtime));
        }
    }
    // For source and destination from the astra-sim test
    // Address remoteAddress (InetSocketAddress (ipv4Interfaces.GetAddress (dest), servPort));
    // OnOffHelper clientHelper ("ns3::TcpSocketFactory", remoteAddress);
    // clientHelper.SetAttribute 
    //     ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
    // clientHelper.SetAttribute 
    //     ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    // ApplicationContainer clientApp = clientHelper.Install (n.Get (src));
    // clientApp.Start (Seconds (dest)); 
    // clientApp.Stop (Seconds (dest + runtime));

    csma.EnablePcapAll ("myTCPMultiple");
    Simulator::Stop (Seconds (100));
    Simulator::Run ();
    Simulator::Destroy ();

    return 0;
}
