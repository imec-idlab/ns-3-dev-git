/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 IDLab-imec
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Floris Van den Abeele <floris.vandenabeele@ugent.be>
 */

// Based on:
// ns3 - On/Off Data Source Application class
// George F. Riley, Georgia Tech, Spring 2007
// Adapted from ApplicationOnOff in GTNetS.
#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/packet-socket-address.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"
#include "ns3/random-variable-stream.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/boolean.h"
#include "ns3/trace-source-accessor.h"
#include "lorawan.h"
#include "lorawan-net-device.h"
#include "lorawan-enddevice-application.h"
#include "lorawan-frame-header.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/string.h"
#include "ns3/pointer.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LoRaWANEndDeviceApplication");

NS_OBJECT_ENSURE_REGISTERED (LoRaWANEndDeviceApplication);

TypeId
LoRaWANEndDeviceApplication::GetTypeId (void)
{
  // Construct default value for the channel random variable:
  std::stringstream channelRandomVariableSS;
  const uint32_t channelRandomVariableDefaultMin = 0;
  const uint32_t channelRandomVariableDefaultMax = (LoRaWAN::m_supportedChannels.size () - 1) - 1; // additional -1 as not to use the 10% RDC channel as an upstream channel
  channelRandomVariableSS << "ns3::UniformRandomVariable[Min=" << channelRandomVariableDefaultMin << "|Max=" << channelRandomVariableDefaultMax << "]";
  std::cout << "LoRaWANEndDeviceApplication::GetTypeId: " << channelRandomVariableSS.str() << std::endl;

  static TypeId tid = TypeId ("ns3::LoRaWANEndDeviceApplication")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<LoRaWANEndDeviceApplication> ()
    .AddAttribute ("DataRateIndex",
                   "DataRate index used for US transmissions of this end device.",
                   UintegerValue (0), // default data rate is SF12
                   MakeUintegerAccessor (&LoRaWANEndDeviceApplication::GetDataRateIndex, &LoRaWANEndDeviceApplication::SetDataRateIndex),
                   MakeUintegerChecker<uint16_t> (0, LoRaWAN::m_supportedDataRates.size ()))
    .AddAttribute ("PacketSize", "The size of packets sent in on state",
                   UintegerValue (21),
                   MakeUintegerAccessor (&LoRaWANEndDeviceApplication::m_pktSize),
                   MakeUintegerChecker<uint32_t> (1))
    .AddAttribute ("ConfirmedDataUp",
                   "Send Upstream data as Confirmed Data UP MAC packets."
                   "False means Unconfirmed data up packets are sent.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&LoRaWANEndDeviceApplication::m_confirmedData),
                   MakeBooleanChecker ())
    .AddAttribute ("ChannelRandomVariable", "A RandomVariableStream used to pick the channel for upstream transmissions.",
                   StringValue (channelRandomVariableSS.str ()),
                   MakePointerAccessor (&LoRaWANEndDeviceApplication::m_channelRandomVariable),
                   MakePointerChecker <RandomVariableStream>())
    .AddAttribute ("UpstreamIAT", "A RandomVariableStream used to pick the time between subsequent US transmissions from this end device.",
                   StringValue ("ns3::ConstantRandomVariable[Constant=600.0]"),
                   MakePointerAccessor (&LoRaWANEndDeviceApplication::m_upstreamIATRandomVariable),
                   MakePointerChecker <RandomVariableStream>())
    .AddAttribute ("MaxBytes",
                   "The total number of bytes to send. Once these bytes are sent, "
                   "no packet is sent again, even in on state. The value zero means "
                   "that there is no limit.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&LoRaWANEndDeviceApplication::m_maxBytes),
                   MakeUintegerChecker<uint64_t> ())
    .AddTraceSource ("USMsgTransmitted", "An US message is sent",
                     MakeTraceSourceAccessor (&LoRaWANEndDeviceApplication::m_usMsgTransmittedTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("DSMsgReceived", "An acknowledgement for an US message has been received.",
                     MakeTraceSourceAccessor (&LoRaWANEndDeviceApplication::m_dsMsgReceivedTrace),
                     "ns3::Packet::TracedCallback")
  ;
  return tid;
}


LoRaWANEndDeviceApplication::LoRaWANEndDeviceApplication ()
  : m_socket (0),
    m_connected (false),
    m_lastTxTime (Seconds (0)),
    m_totBytes (0),
    m_framePort (0),
    m_fCntUp (0),
    m_fCntDown (0),
    m_setAck (false),
    m_totalRx (0)
{
  NS_LOG_FUNCTION (this);

  //m_channelRandomVariable = CreateObject <UniformRandomVariable> (); // random variable between 0 and size(channels) - 2
  //m_channelRandomVariable->SetAttribute ("Min", DoubleValue (0.0));
  //const uint32_t max = (LoRaWAN::m_supportedChannels.size () - 1) - 1; // additional -1 as not to use the 10% RDC channel as an upstream channel
  //m_channelRandomVariable->SetAttribute ("Max", DoubleValue (max));
}

LoRaWANEndDeviceApplication::~LoRaWANEndDeviceApplication ()
{
  NS_LOG_FUNCTION (this);
}

void
LoRaWANEndDeviceApplication::SetMaxBytes (uint64_t maxBytes)
{
  NS_LOG_FUNCTION (this << maxBytes);
  m_maxBytes = maxBytes;
}

uint32_t
LoRaWANEndDeviceApplication::GetDataRateIndex (void) const
{
  return m_dataRateIndex;
}

void
LoRaWANEndDeviceApplication::SetDataRateIndex (uint32_t index)
{
  NS_LOG_FUNCTION (this << index);

  if (index <= LoRaWAN::m_supportedDataRates.size () - 1)
    m_dataRateIndex = index;
  else
    NS_LOG_ERROR (this << " " << index << " is an invalid data rate index");
}

Ptr<Socket>
LoRaWANEndDeviceApplication::GetSocket (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socket;
}

int64_t
LoRaWANEndDeviceApplication::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_channelRandomVariable->SetStream (stream);
  m_upstreamIATRandomVariable->SetStream (stream + 1);
  return 2;
}

void
LoRaWANEndDeviceApplication::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_socket = 0;
  // chain up
  Application::DoDispose ();
}

// Application Methods
void LoRaWANEndDeviceApplication::StartApplication () // Called at time specified by Start
{
  NS_LOG_FUNCTION (this);

  // Create the socket if not already
  if (!m_socket)
    {
      m_socket = Socket::CreateSocket (GetNode (), TypeId::LookupByName ("ns3::PacketSocketFactory"));
      m_socket->Bind ();

      PacketSocketAddress socketAddress;
      socketAddress.SetSingleDevice (GetNode ()->GetDevice (0)->GetIfIndex ()); // Set the address to match only a specified NetDevice...
      m_socket->Connect (Address (socketAddress)); // packet-socket documentation mentions: "Send: send the input packet to the underlying NetDevices with the default destination address. The socket must be bound and connected."

      m_socket->Listen ();
      //m_socket->SetAllowBroadcast (true); // TODO: does not work on packet socket?
      m_socket->SetRecvCallback (MakeCallback (&LoRaWANEndDeviceApplication::HandleRead, this));

      // TODO: remove?
      m_socket->SetConnectCallback (
        MakeCallback (&LoRaWANEndDeviceApplication::ConnectionSucceeded, this),
        MakeCallback (&LoRaWANEndDeviceApplication::ConnectionFailed, this));
    }

  // Insure no pending event
  CancelEvents ();
  // If we are not yet connected, there is nothing to do here
  // The ConnectionComplete upcall will start timers at that time
  //if (!m_connected) return;
  m_txEvent = Simulator::ScheduleNow (&LoRaWANEndDeviceApplication::SendPacket, this);
}

void LoRaWANEndDeviceApplication::StopApplication () // Called at time specified by Stop
{
  NS_LOG_FUNCTION (this);

  CancelEvents ();
  if(m_socket != 0)
    {
      m_socket->Close ();
    }
  else
    {
      NS_LOG_WARN ("LoRaWANEndDeviceApplication found null socket to close in StopApplication");
    }
}

void LoRaWANEndDeviceApplication::CancelEvents ()
{
  NS_LOG_FUNCTION (this);

  Simulator::Cancel (m_txEvent);
}


// Private helpers
void LoRaWANEndDeviceApplication::ScheduleNextTx ()
{
  NS_LOG_FUNCTION (this);

  if (m_maxBytes == 0 || m_totBytes < m_maxBytes)
    {
      Time nextTime (Seconds (this->m_upstreamIATRandomVariable->GetValue ()));
      NS_LOG_LOGIC (this << " nextTime = " << nextTime);
      m_txEvent = Simulator::Schedule (nextTime,
                                       &LoRaWANEndDeviceApplication::SendPacket, this);
    }
  else
    { // All done, cancel any pending events
      StopApplication ();
    }
}

void LoRaWANEndDeviceApplication::SendPacket ()
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT (m_txEvent.IsExpired ());

  Ipv4Address myAddress = Ipv4Address::ConvertFrom (GetNode ()->GetDevice (0)->GetAddress ());
  LoRaWANFrameHeader fhdr;
  fhdr.setDevAddr (myAddress);
  fhdr.setAck (m_setAck);
  fhdr.setFramePending (false);
  fhdr.setFrameCounter (m_fCntUp++); // increment frame counter
  // FPort: we will send FRMPayload so set the frame port
  fhdr.setFramePort (m_framePort);

  // Construct MACPayload
  // PHYPayload: MHDR | MACPayload | MIC
  // MACPayload: FHDR | FPort | FRMPayload
  Ptr<Packet> packet;
  uint8_t frmPayloadSize = m_pktSize  - fhdr.GetSerializedSize() - 1 - 4;  // subtract 8 bytes for frame header, 1B for MAC header and 4B for MAC MIC
  if (frmPayloadSize >= sizeof(uint64_t)) { // check whether payload size is large enough to hold 64 bit integer
    // send decrementing counter as payload (note: globally shared counter)
    uint8_t* payload = new uint8_t[frmPayloadSize](); // the parenthesis initialize the allocated memory to zero
    if (payload) {
      const uint64_t counter = LoRaWANCounterSingleton::GetCounter ();
      ((uint64_t*)payload)[0] = counter; // copy counter to beginning payload
      packet = Create<Packet> (payload, frmPayloadSize);
      delete[] payload;
    } else {
      packet = Create<Packet> (frmPayloadSize);
    }
  } else {
    packet = Create<Packet> (frmPayloadSize);
  }

  packet->AddHeader (fhdr); // Packet now represents MACPayload

  // Select channel to use:
  uint32_t channelIndex = m_channelRandomVariable->GetInteger ();
  NS_ASSERT (channelIndex <= LoRaWAN::m_supportedChannels.size () - 2); // -2 because end devices should not use the special high power channel for US traffic

  LoRaWANPhyParamsTag phyParamsTag;
  phyParamsTag.SetChannelIndex (channelIndex);
  phyParamsTag.SetDataRateIndex (m_dataRateIndex);
  phyParamsTag.SetCodeRate (3);
  packet->AddPacketTag (phyParamsTag);

  // Set Msg type
  LoRaWANMsgTypeTag msgTypeTag;
  if (m_confirmedData)
    msgTypeTag.SetMsgType (LORAWAN_CONFIRMED_DATA_UP);
  else
    msgTypeTag.SetMsgType (LORAWAN_UNCONFIRMED_DATA_UP);
  packet->AddPacketTag (msgTypeTag);

  uint32_t deviceAddress = myAddress.Get ();
  m_usMsgTransmittedTrace (deviceAddress, msgTypeTag.GetMsgType (), packet);

  // Set NetDevice MTU Data rate before calling socket::Send
  Ptr<LoRaWANNetDevice> netDevice = DynamicCast<LoRaWANNetDevice> (GetNode ()->GetDevice (0));
  netDevice->SetMTUSpreadingFactor(LoRaWAN::m_supportedDataRates [m_dataRateIndex].spreadingFactor);

  int16_t r = m_socket->Send (packet);
  if (r < 0) {
    NS_LOG_ERROR(this << "PacketSocket::Send failed and returned " << static_cast<int16_t>(r) << ". Errno is set to " << m_socket->GetErrno ());
  } else {
    m_setAck = false; // reset m_setAck
    m_totBytes += packet->GetSize ();

    NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
        << "s LoRaWANEndDevice application on node #"
        << GetNode()->GetId()
        << " sent "
        <<  packet->GetSize () << " bytes,"
        << " total Tx " << m_totBytes << " bytes");
  }

  m_lastTxTime = Simulator::Now ();
  ScheduleNextTx ();
}

void LoRaWANEndDeviceApplication::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
    {
      if (packet->GetSize () == 0)
        { //EOF
          break;
        }
      m_totalRx += packet->GetSize ();

      if (PacketSocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                       << "s end device on node #"
                       << GetNode()->GetId()
                       <<  " received " << packet->GetSize () << " bytes from "
                       << PacketSocketAddress::ConvertFrom(from).GetPhysicalAddress ()
                       << ", total Rx " << m_totalRx << " bytes");

          this->HandleDSPacket (packet, from);
        }
      else
        {
          NS_LOG_WARN (this << " Unexpected address type");
        }
    }
}

void
LoRaWANEndDeviceApplication::HandleDSPacket (Ptr<Packet> p, Address from)
{
  NS_LOG_FUNCTION(this << p);

  // set m_setAck to true in case a CONFIRMED_DATA_DOWN message was received:
  // Try to parse Packet tag:
  LoRaWANMsgTypeTag msgTypeTag;
  if (p->RemovePacketTag (msgTypeTag)) {
    LoRaWANMsgType msgType = msgTypeTag.GetMsgType ();
    if (msgType == LORAWAN_CONFIRMED_DATA_DOWN) {
      m_setAck = true; // next packet should set Ack bit
      NS_LOG_DEBUG (this << " Set Ack bit to 1");
    }
  } else {
    NS_LOG_WARN (this << " LoRaWANMsgTypeTag packet tag is missing from packet");
  }

  // Was packet received in first or second receive window?
  // -> Look at Mac state
  Ptr<LoRaWANNetDevice> netDevice = DynamicCast<LoRaWANNetDevice> (GetNode ()->GetDevice (0));
  Ptr<LoRaWANMac> mac = netDevice->GetMac ();
  LoRaWANMacState state = mac->GetLoRaWANMacState ();
  NS_ASSERT (state == MAC_RW1 || state == MAC_RW2);

  // Log packet reception
  Ipv4Address myAddress = Ipv4Address::ConvertFrom (GetNode ()->GetDevice (0)->GetAddress ());
  uint32_t deviceAddress = myAddress.Get ();
  if (state == MAC_RW1)
    m_dsMsgReceivedTrace (deviceAddress, msgTypeTag.GetMsgType(), p, 1);
  else if (state == MAC_RW2)
    m_dsMsgReceivedTrace (deviceAddress, msgTypeTag.GetMsgType(), p, 2);
}

void LoRaWANEndDeviceApplication::ConnectionSucceeded (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  m_connected = true;
}

void LoRaWANEndDeviceApplication::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_FATAL_ERROR (this << " Connection failed");
}

} // Namespace ns3
