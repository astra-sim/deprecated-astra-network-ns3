#include "ns3/log.h"
#include "simple-udp-application.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/csma-net-device.h"
#include "ns3/ethernet-header.h"
#include "ns3/arp-header.h"
#include "ns3/ipv4-header.h"
#include "ns3/udp-header.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include <iostream>
#include <queue>
#include <thread>
#include "/Users/tarannumkhan/Desktop/ns-allinone-3.34/ns-3.34/scratch/workerQueue.h"
#include "/Users/tarannumkhan/Desktop/ns-allinone-3.34/ns-3.34/scratch/myHeader.h"
#define PURPLE_CODE "\033[95m"
#define CYAN_CODE "\033[96m"
#define TEAL_CODE "\033[36m"
#define BLUE_CODE "\033[94m"
#define GREEN_CODE "\033[32m"
#define YELLOW_CODE "\033[33m"
#define LIGHT_YELLOW_CODE "\033[93m"
#define RED_CODE "\033[91m"
#define BOLD_CODE "\033[1m"
#define END_CODE "\033[0m"
set<pair<int,int> > recvHash;
map<pair<int,int>, struct task1> expeRecvHash;
namespace ns3
{
  NS_LOG_COMPONENT_DEFINE("SimpleUdpApplication");
  NS_OBJECT_ENSURE_REGISTERED(SimpleUdpApplication);

  TypeId
  SimpleUdpApplication::GetTypeId()
  {
    static TypeId tid = TypeId("ns3::SimpleUdpApplication")
                            .AddConstructor<SimpleUdpApplication>()
                            .SetParent<Application>();
    return tid;
  }

  TypeId
  SimpleUdpApplication::GetInstanceTypeId() const
  {
    return SimpleUdpApplication::GetTypeId();
  }

  SimpleUdpApplication::SimpleUdpApplication()
  {
    m_port_send = 7000;
    m_port_recv = 9000;
    finishFlag = 0;
  }

  SimpleUdpApplication::~SimpleUdpApplication()
  {
  }

  void SimpleUdpApplication::SetupReceiveSocket(Ptr<Socket> socket, uint16_t port)
  {
    InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), port);
    if (socket->Bind(local) == -1)
    {
      NS_FATAL_ERROR("Failed to bind socket");
    }
  }
  void SimpleUdpApplication::StartApplication()
  {
    //Receive sockets
    // TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    // m_recv_socket1 = Socket::CreateSocket(GetNode(), tid);
    // processQueue(&finishFlag, &dataQueue, m_send_socket);
  }

  void SimpleUdpApplication::InitializeAppRecv(int nodes, int index){
    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    for(int i = 0;i<nodes;i++){
      m_recv_socket[i] = Socket::CreateSocket(GetNode(), tid);
      SetupReceiveSocket(m_recv_socket[i],m_port_recv+i);
      m_recv_socket[i]->SetRecvCallback(MakeCallback(&SimpleUdpApplication::HandleRead,this));
      m_send_socket[i] = Socket::CreateSocket(GetNode(), tid);
    }
  }

  void SimpleUdpApplication::InitializeAppSend(int nodes, int index, Ipv4InterfaceContainer interfaces){
    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    // m_recv_socket1 = Socket::CreateSocket(GetNode(), tid);
    for(int i = 0;i<nodes;i++){
      if(index!=i)
      {
        SetupReceiveSocket(m_send_socket[i],m_port_send+i);
        m_send_socket[i]->Connect(InetSocketAddress(Ipv4Address::ConvertFrom(interfaces.GetAddress(i)),m_port_recv+index));
      }
    }
    // std::thread childThread1 (&SimpleUdpApplication::processQueue,finishFlag, dataQueue, m_send_socket);
  }

  void SimpleUdpApplication::HandleRead(Ptr<Socket> socket)
  {
    NS_LOG_FUNCTION(this << socket);
    Ptr<Packet> packet;
    Address from;
    Address localAddress;
    int receiver_node = GetNode()->GetId();
    std::cout<<"receiver node "<<receiver_node<<"\n";
    int sender_node;
    // std::cout<<socket->GetBoundNetDevice()->GetAddress()<<"\n";
    while ((packet = socket->RecvFrom(from)))
    {
      MyHeader destinationHeader;
      packet->RemoveHeader (destinationHeader);
      sender_node = destinationHeader.GetData ();
      cout<<"sender node is" << sender_node<<"\n";
      NS_LOG_INFO(PURPLE_CODE << "HandleRead : Received a Packet of size: " << packet->GetSize() << " at time " << Now().GetSeconds() << END_CODE);
      //NS_LOG_INFO("Content: " << packet->ToString());
      std::cout<<"handleread "<<packet->GetSize() << " at time " << Now().GetSeconds()<<"\n";
    }
    for(std::map<std::pair<int, int>, struct task1>::iterator it = expeRecvHash.begin();it!=expeRecvHash.end();it++){
        std::cout<<it->first.first<<" "<<it->first.second<<"\n";
    }
    if(expeRecvHash.find(make_pair(sender_node, receiver_node))!=expeRecvHash.end()){
      task1 t2 = expeRecvHash[make_pair(sender_node, receiver_node)];
      t2.msg_handler(t2.fun_arg);
      expeRecvHash.erase(make_pair(sender_node, receiver_node));
      cout<<"already in expected recv hash\n";
    }
    else{
      recvHash.insert(make_pair(sender_node, receiver_node));
      cout<<"not in expected recv hash\n";
    }
    // recvHash.insert();
  }

  void SimpleUdpApplication::SendPacket(int dest, void* fun_arg,
    void (*msg_handler)(void* fun_arg)){
    // NS_LOG_FUNCTION (this << packet << destination << port);
    Ptr<Packet> packet1 = Create <Packet> (400);
    MyHeader sourceHeader;
    sourceHeader.SetData (GetNode()->GetId());
    packet1->AddHeader (sourceHeader);
    packet1->Print (std::cout);
    std::cout << std::endl;
    m_send_socket[dest]->Send(packet1);
    std::cout<<"packet send to port of sender "<<m_port_send+dest<<" and recevied to node"<<m_port_recv+GetNode()->GetId()<<"\n";
    msg_handler(fun_arg);
  }

  // void SimpleUdpApplication::SendPacket(int dest, void* fun_arg,
  //   void (*msg_handler)(void* fun_arg))
  // {
  //   task t;
  //   t.dest = dest;
  //   t.fun_arg = fun_arg;
  //   t.msg_handler = msg_handler;
  //   dataQueue.push(t);
  // }

  void SimpleUdpApplication::processQueue(){
    Ptr <Packet> packet1 = Create <Packet> (400);
    while(finishFlag==0 || !dataQueue.empty()){
      while(!dataQueue.empty()){
        task t1 = dataQueue.front();
        int nodeId = t1.dest;
        m_send_socket[nodeId]->Send(packet1);
        std::cout<<"inside the queue\n";
        // t1.msg_handler(t1.fun_arg);
      }
    }
  }

  void SimpleUdpApplication::FinishTask(){
    finishFlag = 1;
    if(dataQueue.empty()){
      std::cout<<"dataQueue emptied by processQueue\n";
    }
    else{
      std::cout<<"dataQueue not emptied by processQueue\n";
    }
  }

} // namespace ns3
