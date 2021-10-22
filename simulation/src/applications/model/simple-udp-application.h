#ifndef NS3_UDP_ARQ_APPLICATION_H
#define NS3_UDP_ARQ_APPLICATION_H
#include <queue>
#include "ns3/socket.h"
#include "ns3/application.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/nstime.h"
#include <iostream>

using namespace ns3;

namespace ns3
{
  struct task{
      int src;
      int dest;
      int type;
      // void (*fptr) (void);
      void* fun_arg;
      void (*msg_handler)(void* fun_arg);
    };
  class SimpleUdpApplication : public Application 
  {
    public:
      SimpleUdpApplication ();
      virtual ~SimpleUdpApplication ();

      static TypeId GetTypeId ();
      virtual TypeId GetInstanceTypeId () const;

      /** \brief handles incoming packets on port 7777
       */
      // void HandleReadOne (Ptr<Socket> socket);

      /** \brief handles incoming packets on port 9999
       */
      void HandleRead (Ptr<Socket> socket);

      void ScheduleTransmit(Time dt, int dest, void* fun_arg,
          void (*msg_handler)(void* fun_arg), int count, int tag);

      /** \brief Send an outgoing packet. This creates a new socket every time
      */
      void SendPacket(int dest, void* fun_arg,
          void (*msg_handler)(void* fun_arg), int count, int tag);

      void InitializeAppSend(int nodes, int index, Ipv4InterfaceContainer interfaces);

      void InitializeAppRecv(int nodes, int index);

      void FinishTask();

      // void processQueue(int &finishFlag, std::queue<task> &dataQueue,Ptr<Socket> m_send_socket[]);
      void processQueue();
    private:
      
      
      void SetupReceiveSocket (Ptr<Socket> socket, uint16_t port);
      virtual void StartApplication ();
      // void SimpleUdpApplication::SendPacketInternal();

      // Ptr<Socket> m_recv_socket1; /**< A socket to receive on a specific port */
      Ptr<Socket> m_recv_socket[100]; /**< A socket to receive on a specific port */
      // uint16_t m_port1; 
      uint16_t m_port_send;
      // uint16_t m_port1; 
      uint16_t m_port_recv;

      Ptr<Socket> m_send_socket[100]; /**< A socket to listen on a specific port */
      std::queue<struct task> dataQueue;
      int finishFlag;
      int mtu;

  };
}

#endif
