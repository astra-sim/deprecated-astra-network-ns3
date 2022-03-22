 #include "ns3/ptr.h"
 #include "ns3/packet.h"
 #include "ns3/header.h"
#pragma once
#include <iostream>
 using namespace ns3;
class MyHeader : public Header 
 {
 public:
  
   MyHeader ();
   virtual ~MyHeader ();
  
   void SetData (uint16_t data);
   uint16_t GetData (void) const;
   void SetTag (uint16_t data);
   uint16_t GetTag (void) const;
  
   static TypeId GetTypeId (void);
   virtual TypeId GetInstanceTypeId (void) const;
   virtual void Print (std::ostream &os) const;
   virtual void Serialize (Buffer::Iterator start) const;
   virtual uint32_t Deserialize (Buffer::Iterator start);
   virtual uint32_t GetSerializedSize (void) const;
 private:
   uint16_t m_data;
   uint16_t m_tag;  
 };
  
 MyHeader::MyHeader ()
 {
   // we must provide a public default constructor, 
   // implicit or explicit, but never private.
 }
 MyHeader::~MyHeader ()
 {
 }
  
 TypeId
 MyHeader::GetTypeId (void)
 {
   static TypeId tid = TypeId ("ns3::MyHeader")
     .SetParent<Header> ()
     .AddConstructor<MyHeader> ()
   ;
   return tid;
 }
 TypeId
 MyHeader::GetInstanceTypeId (void) const
 {
   return GetTypeId ();
 }
  
 void
 MyHeader::Print (std::ostream &os) const
 {
   // This method is invoked by the packet printing
   // routines to print the content of my header.
   //os << "data=" << m_data << std::endl;
   os << "data=" << m_data;
   os << "tag=" << m_tag;
 }
 uint32_t
 MyHeader::GetSerializedSize (void) const
 {
   // we reserve 4 bytes for our header.
   return 4;
 }
 void
 MyHeader::Serialize (Buffer::Iterator start) const
 {
   // we can serialize two bytes at the start of the buffer.
   // we write them in network byte order.
   start.WriteHtonU16 (m_data);
   start.WriteHtonU16 (m_tag);
 }
uint32_t
 MyHeader::Deserialize (Buffer::Iterator start)
 {
   // we can deserialize two bytes from the start of the buffer.
   // we read them in network byte order and store them
   // in host byte order.
   m_data = start.ReadNtohU16 ();
   m_tag = start.ReadNtohU16();
  
   // we return the number of bytes effectively read.
   return 4;
 }
  
 void 
 MyHeader::SetData (uint16_t data)
 {
   m_data = data;
 }
 uint16_t 
 MyHeader::GetData (void) const
 {
   return m_data;
 }

 void
 MyHeader::SetTag (uint16_t tag)
 {
   m_tag = tag;
 }
 uint16_t
 MyHeader::GetTag (void) const
 {
   return m_tag;
 }
