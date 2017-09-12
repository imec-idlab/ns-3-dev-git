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
#include "lorawan.h"
#include "lorawan-mac.h"
#include "lorawan-mac-header.h"
#include "lorawan-net-device.h"
#include "lorawan-frame-header.h"
#include <ns3/simulator.h>
#include <ns3/log.h>
#include <ns3/packet.h>
#include <ns3/random-variable-stream.h>
#include <ns3/double.h>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LoRaWANMac");

NS_OBJECT_ENSURE_REGISTERED (LoRaWANMac);

const uint8_t LoRaWANMac::maxMACPayloadSize[] = {59, 59, 59, 123, 230, 230, 230}; // we don't take the FSK row (for DR7) into account

std::ostream&
operator<< (std::ostream& os, const LoRaWANDataRequestParams& p)
{
  os << " LoRaWANDataRequestParams ("
    << static_cast<uint32_t> (p.m_loraWANChannelIndex)
    << "," << static_cast<uint32_t> (p.m_loraWANDataRateIndex)
    << "," << static_cast<uint32_t> (p.m_loraWANCodeRate)
    << "," << static_cast<uint32_t> (p.m_msgType)
    << "," << static_cast<uint32_t> (p.m_requestHandle)
    << "," << static_cast<uint32_t> (p.m_numberOfTransmissions)
    << ")";

  return os;
}

TypeId
LoRaWANMac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LoRaWANMac")
    .SetParent<Object> ()
    .SetGroupName ("LoRaWAN")
    .AddConstructor<LoRaWANMac> ()
    .AddTraceSource ("MacTxEnqueue",
                     "Trace source indicating a packet has been "
                     "enqueued in the transaction queue",
                     MakeTraceSourceAccessor (&LoRaWANMac::m_macTxEnqueueTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("MacTxDequeue",
                     "Trace source indicating a packet has was "
                     "dequeued from the transaction queue",
                     MakeTraceSourceAccessor (&LoRaWANMac::m_macTxDequeueTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("MacTx",
                     "Trace source indicating a packet has "
                     "arrived for transmission by this device",
                     MakeTraceSourceAccessor (&LoRaWANMac::m_macTxTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("MacTxOk",
                     "Trace source indicating a packet has been "
                     "successfully sent",
                     MakeTraceSourceAccessor (&LoRaWANMac::m_macTxOkTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("MacTxDrop",
                     "Trace source indicating a packet has been "
                     "dropped during transmission",
                     MakeTraceSourceAccessor (&LoRaWANMac::m_macTxDropTrace),
                     "ns3::Packet::TracedCallback")
    //.AddTraceSource ("MacPromiscRx",
    //                 "A packet has been received by this device, "
    //                 "has been passed up from the physical layer "
    //                 "and is being forwarded up the local protocol stack.  "
    //                 "This is a promiscuous trace,",
    //                 MakeTraceSourceAccessor (&LoRaWANMac::m_macPromiscRxTrace),
    //                 "ns3::Packet::TracedCallback")
    .AddTraceSource ("MacRx",
                     "A packet has been received by this device, "
                     "has been passed up from the physical layer "
                     "and is being forwarded up the local protocol stack.  "
                     "This is a non-promiscuous trace,",
                     MakeTraceSourceAccessor (&LoRaWANMac::m_macRxTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("MacRxDrop",
                     "Trace source indicating a packet was received, "
                     "but dropped before being forwarded up the stack",
                     MakeTraceSourceAccessor (&LoRaWANMac::m_macRxDropTrace),
                     "ns3::Packet::TracedCallback")
    //.AddTraceSource ("Sniffer",
    //                 "Trace source simulating a non-promiscuous "
    //                 "packet sniffer attached to the device",
    //                 MakeTraceSourceAccessor (&LoRaWANMac::m_snifferTrace),
    //                 "ns3::Packet::TracedCallback")
    //.AddTraceSource ("PromiscSniffer",
    //                 "Trace source simulating a promiscuous "
    //                 "packet sniffer attached to the device",
    //                 MakeTraceSourceAccessor (&LoRaWANMac::m_promiscSnifferTrace),
    //                 "ns3::Packet::TracedCallback")
// TODO: broken...
    .AddTraceSource ("MacState",
                     "The state of LoRaWAN Mac",
                     MakeTraceSourceAccessor (&LoRaWANMac::m_LoRaWANMacState),
                     "ns3::TracedValueCallback::LoRaWANMacState")
    .AddTraceSource ("MacSentPkt",
                     "Trace source reporting some information about "
                     "the sent packet",
                     MakeTraceSourceAccessor (&LoRaWANMac::m_sentPktTrace),
                     "ns3::LoRaWANMac::SentTracedCallback")
  ;
  return tid;
}

LoRaWANMac::LoRaWANMac ()
{
  LoRaWANMac (0); // index not provided, assume 0
}

LoRaWANMac::LoRaWANMac (uint8_t index) : m_index (index)
{
  // First set the state to a known value, call ChangeMacState to fire trace source.
  //m_LoRaWANMacState.push_back (MAC_IDLE);
  //m_phy.push_back (NULL);
  //m_LoRaWANMacState [0] = MAC_IDLE;
  ChangeMacState (MAC_IDLE);

  m_deviceType = LORAWAN_DT_END_DEVICE_CLASS_A;
  m_RX1DROffset = 0; // default value is zero

  // m_macPromiscuousMode = false;
  m_retransmission = 0;
  m_txPkt = 0;

  m_ackTimeOutRandomVariable = CreateObject<UniformRandomVariable> ();
}

LoRaWANMac::~LoRaWANMac ()
{
}

void
LoRaWANMac::SetPhy (Ptr<LoRaWANPhy> phy)
{
  this->m_phy = phy;
}

Ptr<LoRaWANPhy>
LoRaWANMac::GetPhy (void)
{
  return this->m_phy;
}

void
LoRaWANMac::SetRDC (Ptr<LoRaWANMacRDC> macRDC)
{
  this->m_lorawanMacRDC = macRDC;
}

void
LoRaWANMac::DoInitialize ()
{
  this->sendTRXStateRequestForIdleMAC ();

  Object::DoInitialize ();
}

void
LoRaWANMac::sendTRXStateRequestForIdleMAC ()
{
  if (m_deviceType == LORAWAN_DT_END_DEVICE_CLASS_A) {
      m_phy->SetTRXStateRequest (LORAWAN_PHY_TRX_OFF);
  } else if (m_deviceType == LORAWAN_DT_GATEWAY) {
      m_phy->SetTRXStateRequest (LORAWAN_PHY_RX_ON);
  }
}

void
LoRaWANMac::DoDispose ()
{
  m_txPkt = 0;
  for (uint32_t i = 0; i < m_txQueue.size (); i++)
    {
      m_txQueue[i]->txQPkt = 0;
      delete m_txQueue[i];
    }
  m_txQueue.clear ();
  m_phy = 0;
  m_dataIndicationCallback = MakeNullCallback< void, LoRaWANDataIndicationParams, Ptr<Packet> > ();
  m_dataConfirmCallback = MakeNullCallback< void, LoRaWANDataConfirmParams > ();

  Object::DoDispose ();
}

Ipv4Address
LoRaWANMac::GetDevAddr (void) const
{
  return m_devAddr;
}

void
LoRaWANMac::SetDevAddr (Ipv4Address devAddr)
{
  m_devAddr = devAddr;
}

LoRaWANDeviceType
LoRaWANMac::GetDeviceType (void) const
{
  return m_deviceType;
}

void
LoRaWANMac::SetDeviceType (LoRaWANDeviceType type)
{
  m_deviceType = type;
}

uint8_t
LoRaWANMac::GetIndex (void) const
{
  return m_index;
}

uint8_t
LoRaWANMac::GetRX1DROffset (void) const
{
  return m_RX1DROffset;
}

void
LoRaWANMac::SetRX1DROffset (uint8_t offset)
{
  if (offset >=0 && offset <= 5)
    this->m_RX1DROffset = offset;
  else
    NS_LOG_WARN (this << "Invalid RX1DROffset: " << static_cast<uint32_t>(offset));
}

void
LoRaWANMac::SetLoRaWANMacState (LoRaWANMacState macState)
{
  NS_LOG_FUNCTION (this << macState);

  if (macState == MAC_IDLE) {
      NS_ASSERT (m_LoRaWANMacState == MAC_TX || m_LoRaWANMacState == MAC_RW1 || m_LoRaWANMacState == MAC_RW2 || m_LoRaWANMacState == MAC_ACK_TIMEOUT || m_LoRaWANMacState == MAC_UNAVAILABLE);

      ChangeMacState (macState);

      // Request to Put Phy trx into state corresponding to this device type
      this->sendTRXStateRequestForIdleMAC ();

      // Ptr<LoRaWANPhy> phy = m_phy;
      // phy->SetTRXStateRequest (LORAWAN_PHY_IDLE);

      // In case we might be able to send again call CheckRetransmission or CheckQueue
      if (m_txPkt != 0) {
        CheckRetransmission ();
      } else {
        CheckQueue ();
      }
  } else if (macState == MAC_TX) {
      NS_ASSERT (m_LoRaWANMacState == MAC_IDLE);

      // for gateways: switch off other PHY/MACs on this net-device
      if (m_deviceType == LORAWAN_DT_GATEWAY) {
        NS_ASSERT (!this->m_beginTxCallback.IsNull ());
        this->m_beginTxCallback (this);
      }

      ChangeMacState (macState);

      if (ConfigurePhyForTX ()) {
        // Set Phy to TX ON
        m_phy->SetTRXStateRequest (LORAWAN_PHY_TX_ON);

        // Wait for a state confirmation via SetTRXStateConfirm before starting transmission
        //StartTransmission ();
      }
  } else if (macState == MAC_WAITFORRW1) {
      NS_ASSERT (m_LoRaWANMacState == MAC_TX);

      ChangeMacState (macState);

      // Request to Put Phy into IDLE
      m_phy->SetTRXStateRequest (LORAWAN_PHY_IDLE);

      // schedule a MAC event to open RW1
      Time receiveDelay = MicroSeconds (RECEIVE_DELAY1);
      m_setMacState = Simulator::Schedule (receiveDelay, &LoRaWANMac::SetLoRaWANMacState, this, MAC_RW1);
  } else if (macState == MAC_RW1) {
      NS_ASSERT (m_LoRaWANMacState == MAC_WAITFORRW1);

      ChangeMacState (macState);
      OpenRW ();
  } else if (macState == MAC_WAITFORRW2) {
      NS_ASSERT (m_LoRaWANMacState == MAC_RW1);

      ChangeMacState (macState);

      // Request to Put Phy into IDLE
      m_phy->SetTRXStateRequest (LORAWAN_PHY_IDLE);

      // schedule a MAC event to open RW2
      // RW2 starts RECEIVE_DELAY2 after the end of the uplink modulation
      Time receiveDelay = (m_lastUplinkBitTime + MicroSeconds (RECEIVE_DELAY2)) - Simulator::Now ();
      if (receiveDelay >= 0)
        m_setMacState = Simulator::Schedule (receiveDelay, &LoRaWANMac::SetLoRaWANMacState, this, MAC_RW2);
      else {
        // the node missed the start of RW2 (e.g. a long packet was received in RW1 but was dropped after or during reception)
        // TODO: what to do?
        // For now try to continue gracefully by switching mac state to RW2 and immediately calling OpenRW() and CloseRW()
        NS_LOG_WARN (this << " MAC missed the start of RW2");
        ChangeMacState (MAC_RW2);
        OpenRW ();
        CloseRW ();
      }

  } else if (macState == MAC_RW2) {
      NS_ASSERT (m_LoRaWANMacState == MAC_WAITFORRW2);

      ChangeMacState (macState);
      OpenRW ();
  } else if (macState == MAC_ACK_TIMEOUT) { // in this state the MAC is waiting for the Mac Ack timeout timer (m_ackTimeOut) to expire
      NS_ASSERT (m_LoRaWANMacState == MAC_TX || m_LoRaWANMacState == MAC_RW2); // MAC_TX for non Class devices

      ChangeMacState (macState);

      // Request to Put Phy into IDLE
      // m_phy->SetTRXStateRequest (LORAWAN_PHY_IDLE);
      this->sendTRXStateRequestForIdleMAC (); // MAC_ACK_TIMEOUT mac state means that the MAC has to wait before it can resume sending, from a PHY point of view this just means that the PHY is switched off (as in MAC idle state)
  } else if (macState == MAC_UNAVAILABLE) {
      NS_ASSERT (m_LoRaWANMacState == MAC_IDLE || m_LoRaWANMacState == MAC_TX);
      ChangeMacState (macState);

      // Request to Put Phy into LORAWAN_PHY_FORCE_TRX_OFF
      m_phy->SetTRXStateRequest (LORAWAN_PHY_FORCE_TRX_OFF);
  } else {
    NS_FATAL_ERROR (this << " unknown MAC state " << macState);
  }
}


void
LoRaWANMac::ChangeMacState (LoRaWANMacState newState)
{
  NS_LOG_LOGIC (this << " change lorawan mac state from "
                     << m_LoRaWANMacState << " to "
                     << newState);
  m_LoRaWANMacState = newState;
}

void
LoRaWANMac::SetDataIndicationCallback (DataIndicationCallback c)
{
  m_dataIndicationCallback = c;
}

void
LoRaWANMac::SetDataConfirmCallback (DataConfirmCallback c)
{
  m_dataConfirmCallback = c;
}

void
LoRaWANMac::SetBeginTxCallback (BeginTxCallback c)
{
  m_beginTxCallback = c;
}

void
LoRaWANMac::SetEndTxCallback (EndTxCallback c)
{
  m_endTxCallback = c;
}

void
LoRaWANMac::SwitchToUnavailableState ()
{
  NS_ASSERT (m_deviceType == LORAWAN_DT_GATEWAY);

  this->SetLoRaWANMacState (MAC_UNAVAILABLE);
}

void
LoRaWANMac::SwitchToIdleState ()
{
  NS_ASSERT (m_deviceType == LORAWAN_DT_GATEWAY);
  NS_ASSERT (m_LoRaWANMacState == MAC_UNAVAILABLE); // this function should only be called to go from MAC_UNAVAILABLE to MAC_IDLE state

  this->SetLoRaWANMacState (MAC_IDLE);
}

void
LoRaWANMac::PdDataDestroyed (void)
{
  NS_LOG_FUNCTION (this);

  if (m_deviceType == LORAWAN_DT_END_DEVICE_CLASS_A) { // end device started receiving a frame in its RW, but the frame was destroyed => always close RW
      CloseRW ();
  }
}

void
LoRaWANMac::PdDataIndication (uint32_t phyPayloadLength, Ptr<Packet> p, uint8_t lqi, uint8_t channelIndex, uint8_t dataRateIndex, uint8_t codeRate)
{
  // TODO: which state?
  //
  if (m_deviceType == LORAWAN_DT_END_DEVICE_CLASS_A) {
    NS_ASSERT (m_LoRaWANMacState == MAC_RW1 || m_LoRaWANMacState == MAC_RW2); // gateway would be in MAC_IDLE, class A in either RW1 or RW2
  } else if (m_deviceType == LORAWAN_DT_GATEWAY) {
    NS_ASSERT (m_LoRaWANMacState == MAC_IDLE);
  }  else {
    NS_FATAL_ERROR ( this << " Invalid device type " << m_deviceType);
    return;
  }

  NS_LOG_FUNCTION (this << phyPayloadLength << p << lqi);

  // Some considerations:
  // Class A: is the frame downstream traffic?
  // Class A: is the frame addressed to me? <-> Gateway: accept all upstream traffic, drop downstream traffic
  // Is the Ack bit set in the frame header? Ack frames received by gateway should be delivered to the upper layer (i.e. network server) whereas for end devices acks are handled in the MAC layer, however data frames should be sent to upper layer anyway...
  // Is the frame pending bit set in the frame header?
  // Encryption? Is the MIC correct?
  // Frame counters?
  // For class A, if we receive a frame in RW1 we do not have to open RW2

  bool acceptFrame = true;

  Ptr<Packet> pktCopy = p->Copy (); // don't alter the original packet when removing headers
  LoRaWANMacHeader macHdr;
  pktCopy->RemoveHeader (macHdr);

  // Check MAC:
  // 1) Header: msg type
  if (m_deviceType == LORAWAN_DT_END_DEVICE_CLASS_A) { // Class A only accepts downstream
    if (!macHdr.IsDownstream ()) {
      acceptFrame = false;
    }
  } else if (m_deviceType == LORAWAN_DT_GATEWAY) { // Gateway only accepts upstream
    if (!macHdr.IsUpstream ()) {
      acceptFrame = false;
    }

  } else {
    NS_FATAL_ERROR ( this << " Invalid device type " << m_deviceType);
    return;
  }
  // 2) MIC: ignore, no encryption at the moment
  // Remove MIC from footer of frame:
  uint32_t MIC = 0;
  pktCopy->RemoveAtEnd (4);

  LoRaWANFrameHeader frameHdr;
  pktCopy->PeekHeader (frameHdr);
  // For end devices check FHDR:
  if (m_deviceType != LORAWAN_DT_GATEWAY) {
    // 1) DevAddr
    if (m_devAddr != frameHdr.getDevAddr ())
      acceptFrame = false;
    // 2) Frame counter?
  }

  if (acceptFrame) {
    m_macRxTrace (p);
    if (m_deviceType == LORAWAN_DT_END_DEVICE_CLASS_A) {
      // Check Ack bit (?) -> for class A, can remove frame that is pending in TX queue
      // Class A: check FPending bit (?) -> should schedule a new TX op soon
      // Class A: we are freed from waiting on RW2.
      if (frameHdr.IsAck ()) { // process Ack for Class A device
        if (m_txPkt != 0) {
          m_macTxOkTrace (m_txPkt);
          m_ackTimeOut.Cancel ();
          if (!m_dataConfirmCallback.IsNull ())
          { // Call callback, informing succesfull delivery of frame
              TxQueueElement *txQElement = m_txQueue.front ();
              LoRaWANDataConfirmParams confirmParams;
              confirmParams.m_requestHandle = txQElement->lorawanDataRequestParams.m_requestHandle;
              confirmParams.m_status = LORAWAN_SUCCESS;
              m_dataConfirmCallback (confirmParams);
          }

          // Remove TX frame from queue
          NS_LOG_DEBUG( this << " Received Ack, removing packet from queue.");
          RemoveFirstTxQElement (true);
        } else {
          NS_LOG_ERROR ( this << " Received Ack but do not have a frame queued for delivery." );
        }
      }

      // Update MAC state from RW1 or RW2 to IDLE, this will set the Phy TRX state to OFF
      m_setMacState.Cancel ();
      m_setMacState = Simulator::ScheduleNow (&LoRaWANMac::SetLoRaWANMacState, this, MAC_IDLE);
    } else if (m_deviceType == LORAWAN_DT_GATEWAY) {
      // MAC state does not change (remains IDLE),
      // When Phy reaches EndRx it will switch its state to RX_ON, which is fine for the gateway
    }

    // Deliver frame
    // TODO: What if the frame contains no Data, still deliver it?
    // Ack frames should  get delivered anyway for gateways (?)
    LoRaWANDataIndicationParams params;
    params.m_channelIndex = channelIndex;
    params.m_dataRateIndex = dataRateIndex;
    params.m_codeRate = codeRate;
    params.m_msgType = macHdr.getLoRaWANMsgType ();
    params.m_endDeviceAddress = frameHdr.getDevAddr (); // Note that a gateway can not access the Dev Addr due to encryption of the MACPayload
    params.m_MIC = MIC;
    if (!m_dataIndicationCallback.IsNull ())
    {
      NS_LOG_DEBUG ("PdDataIndication ():  Packet is for me; forwarding up");
      m_dataIndicationCallback (params, pktCopy);
    }
  } else {
    m_macRxDropTrace (p);
    if (m_deviceType == LORAWAN_DT_END_DEVICE_CLASS_A) { // An end device received a frame in its RW, but the frame was not destined to this end device
      // Just close the receive window
      CloseRW ();
    }
  }
}

void
LoRaWANMac::SetTRXStateConfirm (LoRaWANPhyEnumeration status)
{
  NS_LOG_FUNCTION (this << status);

  if (m_LoRaWANMacState == MAC_IDLE)
    {
      NS_ASSERT (status == LORAWAN_PHY_RX_ON || status == LORAWAN_PHY_SUCCESS || status == LORAWAN_PHY_TRX_OFF);
      // Do nothing special when going idle.
    }
  else if (m_LoRaWANMacState == MAC_TX && status == LORAWAN_PHY_TX_ON)
    {
      NS_ASSERT (m_txPkt);

      // Start sending if we are in state SENDING and the PHY transmitter was enabled.
      //m_promiscSnifferTrace (m_txPkt);
      //m_snifferTrace (m_txPkt);
      m_macTxTrace (m_txPkt);

      Ptr<Packet> p = m_txPkt;

      // Get airtime for TX of PHY frame and update RDC
      TxQueueElement *txQElement = m_txQueue.front ();
      Time airTime = m_phy->CalculateTxTime (p->GetSize ()); // which PHY does not matter here
      uint8_t subBandIndex = LoRaWAN::m_supportedChannels [txQElement->lorawanDataRequestParams.m_loraWANChannelIndex].m_subBandIndex;
      m_lorawanMacRDC->UpdateRDCTimerForSubBand (subBandIndex, airTime);

      // Ask Phy to send Phy payload
      m_phy->PdDataRequest (p->GetSize (), p);
    }
  else if (m_LoRaWANMacState == MAC_WAITFORRW1 || m_LoRaWANMacState == MAC_WAITFORRW2)
    {
      NS_ASSERT (status == LORAWAN_PHY_IDLE);
      // Do nothing special when waiting for RW1/RW2
    }
  else if (m_LoRaWANMacState == MAC_RW1 || m_LoRaWANMacState == MAC_RW2)
    {
      // Either we are at the beginning of the RW, during which the transceiver
      // is switched in RX ON or we are at the end of RW where the transceiver
      // will switch itself to IDLE before the MAC state has witched away from
      // the RW1/RW2 state
      NS_ASSERT (status == LORAWAN_PHY_RX_ON);// || status == LORAWAN_PHY_IDLE);
      // Do nothing special when waiting for RW1/RW2
    }
  else if (m_LoRaWANMacState == MAC_ACK_TIMEOUT)
    {
      // When MAC is in the ACK_TIMEOUT state, then the Phy should be OFF
      NS_ASSERT (status == LORAWAN_PHY_TRX_OFF);
    }
  else if (m_LoRaWANMacState == MAC_UNAVAILABLE)
    {
      NS_ASSERT (status == LORAWAN_PHY_TRX_OFF);
    }
  else
    {
      // TODO: What to do when we receive an error?
      // If we want to transmit a packet, but switching the transceiver on results
      // in an error, we have to recover somehow (and start sending again).
      NS_FATAL_ERROR ("Error changing transceiver state");
    }
}

void
LoRaWANMac::PdDataConfirm (LoRaWANPhyEnumeration status)
{
  NS_ASSERT (m_LoRaWANMacState == MAC_TX);

  NS_LOG_FUNCTION (this << status << m_txQueue.size ());

  NS_ASSERT (m_txPkt);
  LoRaWANMacHeader macHdr;
  m_txPkt->PeekHeader (macHdr);

  //std::ostringstream os;
  //m_txPkt->Print (os);
  //NS_LOG_DEBUG (this << " " << os.str () );

  if (status == LORAWAN_PHY_SUCCESS)
    {
      NS_ASSERT_MSG (m_txQueue.size () > 0, "TxQsize = 0");
      TxQueueElement *txQElement = m_txQueue.front ();
      // As no Ack is comming, notify upper layer that packet was sent and check if packet can be removed from queue
      if (!macHdr.IsConfirmed ())
      {
        m_macTxOkTrace (m_txPkt);
        if (!m_dataConfirmCallback.IsNull ())
          {
            LoRaWANDataConfirmParams confirmParams;
            confirmParams.m_requestHandle = txQElement->lorawanDataRequestParams.m_requestHandle;
            confirmParams.m_status = LORAWAN_SUCCESS;
            m_dataConfirmCallback (confirmParams);
          }

        // Reduce number of transmissions by one
        txQElement->lorawanDataRequestParams.m_numberOfTransmissions--;

        // Check if we can remove the packet from the queue
        if (txQElement->lorawanDataRequestParams.m_numberOfTransmissions == 0) {
          // UNC packet has reached 0 tx attempts, so we can remove it from our queue
          NS_LOG_DEBUG (this << " UNC packet reached zero transmissions, removing packet from queue.");
          RemoveFirstTxQElement (true);
        }
      } else {
        if (m_deviceType == LORAWAN_DT_END_DEVICE_CLASS_A) {
          // For confirmed messages, decrease the number of transmissions
          NS_ASSERT (txQElement->lorawanDataRequestParams.m_numberOfTransmissions > 0);
          NS_LOG_DEBUG( this << " Decreasing number of transmission for packet from " << static_cast<int> (txQElement->lorawanDataRequestParams.m_numberOfTransmissions) << " to " << static_cast<int> (txQElement->lorawanDataRequestParams.m_numberOfTransmissions) - 1);
          txQElement->lorawanDataRequestParams.m_numberOfTransmissions--;
        } else if (m_deviceType == LORAWAN_DT_GATEWAY) {
          // As retransmissions are handled by the network server, it does not make sense to keep the packet in the queue on this gateway MAC
          RemoveFirstTxQElement (true);
        } else {
          NS_FATAL_ERROR ( this << " Invalid device type " << m_deviceType);
          return;
        }
      }

      // Update MAC and PHY state: depending on device class go to either WAITFORRW1 or directly to IDLE
      if (m_deviceType == LORAWAN_DT_END_DEVICE_CLASS_A) { // always go to WAITFORRW1 for Class A
        // Note that the Ack timeout timer will only start running at the beginning of RW2
        m_lastUplinkBitTime = Simulator::Now ();
        m_setMacState = Simulator::ScheduleNow (&LoRaWANMac::SetLoRaWANMacState, this, MAC_WAITFORRW1);
      } else if (m_deviceType == LORAWAN_DT_GATEWAY) { // Gateway
        // Always go to IDLE state for gateway, retransmissions are handled by the network server
        m_setMacState = Simulator::ScheduleNow (&LoRaWANMac::SetLoRaWANMacState, this, MAC_IDLE);

        // Switch other PHY/MACs on this net-device to IDLE
        NS_ASSERT (!this->m_endTxCallback.IsNull());
        this->m_endTxCallback (this);
      } else {
        NS_FATAL_ERROR ( this << " Invalid device type " << m_deviceType);
        return;
      }

    }
  else
  {
    // What to do in this case?
    NS_FATAL_ERROR ("Transmission attempt failed with PHY status " << status);
  }
}

void
LoRaWANMac::sendMACPayloadRequest (LoRaWANDataRequestParams params, Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << params << p);

  // check MAC state, we can only accept a packet in case the MAC idle
  // TODO: what if we are retransmitting a packet, drop the current retransmission?
  //if (!(m_LoRaWANMacState == MAC_IDLE && m_txPkt == 0)) {
  //  // MAC is busy
  //  NS_LOG_ERROR (this << " MAC layer is not Idle, can not send packet with size " << p->GetSize ()". MAC_STATE is " << m_LoRaWANMacState);
  //  return;
  //}

  // TODO: inform upper layers via DataConfirmCallback
  // For now Join request/Join Accept messages are not supported
  if (!(params.m_msgType == LORAWAN_UNCONFIRMED_DATA_UP || params.m_msgType == LORAWAN_UNCONFIRMED_DATA_DOWN || params.m_msgType == LORAWAN_CONFIRMED_DATA_UP || params.m_msgType == LORAWAN_CONFIRMED_DATA_DOWN) ) {
    // We only know how to send (un)confirmed data up or down
    NS_LOG_ERROR (this << " unsupported LoRaWAN Message type: " << params.m_msgType);
    return;
  }

  if ((params.m_msgType == LORAWAN_CONFIRMED_DATA_UP)  || (params.m_msgType == LORAWAN_CONFIRMED_DATA_DOWN)) {
    if (params.m_numberOfTransmissions == 0) {
      NS_LOG_WARN (this << "Number of transmissions is not set on CON frame, setting to " << DEFAULT_NUMBER_US_TRANSMISSIONS);
      params.m_numberOfTransmissions = DEFAULT_NUMBER_US_TRANSMISSIONS;
    }
  } else if (params.m_msgType == LORAWAN_UNCONFIRMED_DATA_UP) {
    if (params.m_numberOfTransmissions == 0) {
      Ptr<LoRaWANNetDevice> netDevice = DynamicCast<LoRaWANNetDevice>(this->GetPhy()->GetDevice());
      if (netDevice) {
        UintegerValue nbRep(1);
        netDevice->GetAttribute("NbRep", nbRep);

        NS_LOG_WARN (this << "Number of transmissions is not set on UNC US frame, setting to " << nbRep.Get());
        params.m_numberOfTransmissions = nbRep.Get();
      } else {
        NS_FATAL_ERROR (this << " Unable to get LoRaWANNetDevice from Phy object attached to this mac");
      }
    }
  } else {
      params.m_numberOfTransmissions = 1; // DS UNC is always 1 tx
  }

  // Gateways may send downstream, end devices only send upstream data
  if (m_deviceType == LORAWAN_DT_GATEWAY) {
    if (!(params.m_msgType == LORAWAN_CONFIRMED_DATA_DOWN || params.m_msgType == LORAWAN_UNCONFIRMED_DATA_DOWN) ) {
      NS_LOG_ERROR (this << " Gateway only supports downstream data, requested LoRaWAN Message type: " << params.m_msgType);
      return;
    }
  } else if (m_deviceType == LORAWAN_DT_END_DEVICE_CLASS_A) {
    if (!(params.m_msgType == LORAWAN_CONFIRMED_DATA_UP || params.m_msgType == LORAWAN_UNCONFIRMED_DATA_UP) ) {
      NS_LOG_ERROR (this << " End device only supports upstream data, requested LoRaWAN Message type: " << params.m_msgType);
      return;
    }
  } else {
      NS_LOG_ERROR (this << " Unsupported device type: " << m_deviceType);
      return;
  }

  // Is there a known channel for the frequency of the data request?

  bool channelFound = params.m_loraWANChannelIndex >= 0 && params.m_loraWANChannelIndex < LoRaWAN::m_supportedChannels.size();
  if (!channelFound) {
    NS_LOG_ERROR (this << " Requested to transmit on channelIndex = " << params.m_loraWANChannelIndex << ", but there is no channel defined for this index.");
    return;
  }

  // Check length of MACPayload
  if (p->GetSize () > maxMACPayloadSize[params.m_loraWANDataRateIndex]) {
    NS_LOG_ERROR (this << " Requested to transmit MACPayload with length = " << p->GetSize () << ", maxiumum size is limited to " << maxMACPayloadSize[params.m_loraWANDataRateIndex] << " for DataRate " << params.m_loraWANDataRateIndex);
    return;
  }

  // Construct Phy Payload
  Ptr<Packet> phyPayload = constructPhyPayload (params, p);

  m_macTxEnqueueTrace (phyPayload);

  // All checks have been passed, add packet to the queue
  TxQueueElement *txQElement = new TxQueueElement;
  txQElement->lorawanDataRequestParams = params;
  txQElement->txQPkt = phyPayload;
  m_txQueue.push_back (txQElement);

  CheckQueue ();
}

Ptr<Packet>
LoRaWANMac::constructPhyPayload (LoRaWANDataRequestParams params, Ptr<Packet> p)
{
  // PHYPayload consists of MAC header, MACPayload and MIC
  LoRaWANMacHeader lorawanMacHdr (params.m_msgType, 0);
  p->AddHeader (lorawanMacHdr);

  // 4B MIC
  uint32_t size = p->GetSize ();
  p->AddPaddingAtEnd (4); // as MIC support is not implemented, we do not add a MIC trailer
  NS_ASSERT (p->GetSize () == (uint32_t)(size + 4)); // make sure the MIC is accounted for in the packet

  return p;
}

void
LoRaWANMac::CheckQueue ()
{
  NS_LOG_FUNCTION (this);

  // Check if we can send a packet: MAC State, Phy state and RDC

  NS_LOG_DEBUG (this << " INFO: tx queue size is equal to " << m_txQueue.size());

  if (m_LoRaWANMacState == MAC_IDLE && !m_txQueue.empty () && m_txPkt == 0 && !m_setMacState.IsRunning ())
  {
    // Check RDC constraints for first packet in the queue
    TxQueueElement *txQElement = m_txQueue.front ();
    int8_t subBandIndex = m_lorawanMacRDC->GetSubBandIndexForChannelIndex (txQElement->lorawanDataRequestParams.m_loraWANChannelIndex);
    NS_ASSERT (subBandIndex >= 0);
    if (m_lorawanMacRDC->IsSubBandAvailable (subBandIndex))
    {
      NS_LOG_DEBUG (this << " sub band #" << (uint16_t)subBandIndex << " is available");

      // we can sent the next frame
      m_txPkt = txQElement->txQPkt;
      m_setMacState = Simulator::ScheduleNow (&LoRaWANMac::SetLoRaWANMacState, this, MAC_TX);

      // in case of gateway, we should set the other MACs to BUSY and the other PHYs to BUSY, see SetLoRaWANMacState

      return; // return, otherwise we will remove the first tx queue element below
    } else {
      NS_LOG_DEBUG (this << " Cannot sent packet because sub band #" << static_cast<uint16_t>(subBandIndex) << " is not available");

      if (m_deviceType != LORAWAN_DT_GATEWAY) {
        m_lorawanMacRDC->ScheduleSubBandTimer (this, subBandIndex); // schedule RDC timer
      }
    }
  } else {
    if (m_LoRaWANMacState != MAC_IDLE)
      NS_LOG_DEBUG (this << " Cannot sent packet because MAC is not idle, MAC state is equal to " << m_LoRaWANMacState);
    if (m_txQueue.empty ())
      NS_LOG_DEBUG (this << " tx queue is empty, so there is no packet to send.");
    if (m_txPkt)
      NS_LOG_DEBUG (this << " Cannot sent packet because of ongoing tx (m_txPkt is set)");
    if (m_setMacState.IsRunning ())
      NS_LOG_DEBUG (this << " Cannot sent packet because set MAC state event is running");
  }

  // If a gateway can not send a packet immediately, then there is no use in trying to send it later as the RW of the end device will not be open later
  if (m_deviceType == LORAWAN_DT_GATEWAY) {
    if (!m_txQueue.empty ()) {
      // this is a dangereous state to be in, experience has shown that the gateway MACs gets stuck at this point
      this->RemoveFirstTxQElement (false);
      NS_FATAL_ERROR (this << " Gateway is unable to send packet immediately, aborting packet transmission.");
    }
  }
}

void
LoRaWANMac::CheckRetransmission ()
{
  NS_ASSERT (m_LoRaWANMacState == MAC_IDLE && m_txPkt != 0);

  NS_ASSERT_MSG (m_deviceType != LORAWAN_DT_GATEWAY, this << "FATAL ERROR: m_deviceType != LORAWAN_DT_GATEWAY -> retransmissions are not handled in the MAC layer for gateways");

  NS_LOG_FUNCTION (this);

  // The packet:
  TxQueueElement *txQElement = m_txQueue.front ();
  NS_ASSERT (txQElement != 0);
  LoRaWANDataRequestParams params = txQElement->lorawanDataRequestParams;

  // This function is only callable for CON US/DS and UNC UP
  if ((params.m_msgType != LORAWAN_CONFIRMED_DATA_UP) && (params.m_msgType != LORAWAN_CONFIRMED_DATA_DOWN) && (params.m_msgType != LORAWAN_UNCONFIRMED_DATA_UP)) {
    NS_LOG_ERROR (this << " CheckRetransmission() should only be called when the queued message is CON US/DS or UNC US. message type is equal to " << (uint32_t)params.m_msgType);
    return;
  }

  // Check if we can send a packet: MAC State, Phy state and RDC

  // check whether there are still transmissions remaining for m_txPkt in case of confirmed data
  if ((params.m_msgType == LORAWAN_CONFIRMED_DATA_UP)  || (params.m_msgType == LORAWAN_CONFIRMED_DATA_DOWN)) {
    if (params.m_numberOfTransmissions == 0) { // confirmed frame has reached its number of transmission attempts, remove it
      m_macTxDropTrace (txQElement->txQPkt);
      // TODO: Inform upper layer:
      if (!m_dataConfirmCallback.IsNull ())
      {
        LoRaWANDataConfirmParams confirmParams;
        confirmParams.m_requestHandle = params.m_requestHandle;
        confirmParams.m_status = LORAWAN_NO_ACK;
        m_dataConfirmCallback (confirmParams);
      }

      NS_LOG_DEBUG (this << " Confirmed packet has reached zero transmissions, removing packet from queue.");
      RemoveFirstTxQElement (false);

      // We can recheck the queue in case any other frames are waiting
      CheckQueue ();
      return;
    }
  }

  // We still have one or more transmissions left for m_txPkt
  if (!m_setMacState.IsRunning ()) {
    // TODO: frequency hopping between retransmissions ?
    // Standard mentions "This resend must be done on another channel and must obey the duty cycle limitation as any other normal transmission."
    // Note that enddevice-application may limit the number of channels via its random variable ...
    // TODO: frequency hopping also applies to UNC US with NbRep > 1
    int8_t subBandIndex = m_lorawanMacRDC->GetSubBandIndexForChannelIndex (params.m_loraWANChannelIndex);
    NS_ASSERT (subBandIndex >= 0);
    if (m_lorawanMacRDC->IsSubBandAvailable (subBandIndex)) { // we can sent the next frame
      m_retransmission++;
      m_setMacState = Simulator::ScheduleNow (&LoRaWANMac::SetLoRaWANMacState, this, MAC_TX);
    } else {
      m_lorawanMacRDC->ScheduleSubBandTimer (this, subBandIndex);
    }
  } else {
      NS_LOG_ERROR ( this << "  called eventhough there is a mac state change scheduled.");
  }
}

void
LoRaWANMac::RemoveFirstTxQElement (bool sentPacket)
{
  NS_LOG_FUNCTION (this);

  TxQueueElement *txQElement = m_txQueue.front ();
  Ptr<const Packet> p = txQElement->txQPkt;

  if (sentPacket)
    m_sentPktTrace (p, m_retransmission + 1);

  txQElement->txQPkt = 0;
  delete txQElement;
  m_txQueue.pop_front ();
  m_txPkt = 0;
  m_retransmission = 0;
  m_macTxDequeueTrace (p);
}

bool
LoRaWANMac::ConfigurePhyForTX () {
  NS_LOG_FUNCTION (this);

  if (m_txPkt != 0) {
    TxQueueElement *txQElement = m_txQueue.front ();
    // Select and configure PHY
    uint8_t channelIndex = txQElement->lorawanDataRequestParams.m_loraWANChannelIndex;
    uint8_t dataRateIndex = txQElement->lorawanDataRequestParams.m_loraWANDataRateIndex;
    uint8_t codeRate = txQElement->lorawanDataRequestParams.m_loraWANCodeRate;

    uint8_t subBandIndex = LoRaWAN::m_supportedChannels[txQElement->lorawanDataRequestParams.m_loraWANChannelIndex].m_subBandIndex; // Sub band belonging to channel
    uint8_t maxTxPower = m_lorawanMacRDC->GetMaxPowerForSubBand (subBandIndex);
    if (!m_phy->SetTxConf (maxTxPower, channelIndex, dataRateIndex, codeRate, 8, false, true) ) {
      NS_LOG_ERROR (this << " unable to configure Phy");
      return false;
    } else {
      return true;
    }
  }
  return false;
}

//void
//LoRaWANMac::StartTransmission()
//{
//  NS_LOG_FUNCTION (this);
//
//  // Construct Phy Payload
//  Ptr<Packet> p = m_txPkt;
//  Ptr<Packet> phyPayload = constructPhyPayload(params, m_txPkt);
//
//  // Select and configure PHY?
//  Ptr<LoRaWANPhy> phy = m_phy[0];
//  if (!phy->SetTxConf(m_subBands[1].maxTXPower, 0, 7, 5, 8, false, true) ) {
//    NS_LOG_ERROR (this << " unable to configure Phy");
//    return;
//  }
//
//  // Get airtime for TX of PHY frame and update RDC
//  Time airTime = phy->CalculateTxTime(phyPayload->GetSize());
//
//  // Instruct Phy to send frame
//
//  // Schedule MAC event for end of transmission after TX time
//  m_setMacState = Simulator::Schedule(airTime, &LoRaWANMac::EndTransmission, this);
//}
//
//void
//LoRaWANMac::EndTransmission()
//{
//  // Put Phy into deep sleep (?)
//  
//  // After a transmission, we should always open RW1
//  m_setMacState = Simulator::ScheduleNow (&LoRaWANMac::SetLoRaWANMacState, this, MAC_WAITFORRW1);
//}

void
LoRaWANMac::SubBandTimerCallback ()
{
  NS_LOG_FUNCTION (this);

  if (m_txPkt != 0) {
    CheckRetransmission ();
  } else {
    CheckQueue ();
  }
}
void
LoRaWANMac::OpenRW ()
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT (m_deviceType == LORAWAN_DT_END_DEVICE_CLASS_A);

  if (m_LoRaWANMacState == MAC_RW1) {
    // RW1 uses the same channel as the preceding uplink
    // The data rate is a function of the uplink data rate and the RX1DROffset
    uint8_t channelIndex = m_phy->GetCurrentChannelIndex ();
    uint8_t dataRateIndex = LoRaWAN::GetRX1DataRateIndex (m_phy->GetCurrentDataRateIndex (), m_RX1DROffset);

    uint8_t subBandIndex = LoRaWAN::m_supportedChannels [channelIndex].m_subBandIndex;
    uint8_t maxTxPower = m_lorawanMacRDC->GetMaxPowerForSubBand (subBandIndex);

    if (!m_phy->SetTxConf (maxTxPower, channelIndex, dataRateIndex, 3, 8, false, true) ) {
      NS_LOG_ERROR (this << " unable to configure Phy");
      return;
    }
  } else if (m_LoRaWANMacState == MAC_RW2) {
    // The default fixed RW2 channel is 869.525 MHz / DR0 (SF12, 125kHz)
    uint8_t channelIndex = LoRaWAN::m_RW2ChannelIndex;
    uint8_t dataRateIndex = LoRaWAN::m_RW2DataRateIndex; // fixed

    uint8_t subBandIndex = LoRaWAN::m_supportedChannels [channelIndex].m_subBandIndex;
    uint8_t maxTxPower = m_lorawanMacRDC->GetMaxPowerForSubBand (subBandIndex);

    if (!m_phy->SetTxConf (maxTxPower, channelIndex, dataRateIndex, 3, 8, false, true) ) {
      NS_LOG_ERROR (this << " unable to configure Phy");
      return;
    }

    // For confirmed frames, start the ACK_TIMEOUT timer at the beginning of RW2
    if (m_txPkt) { // if the transmitted packet was unconfirmed, then it has been removed in RemoveFirstTxQElement which set m_txPkt to NULL
      LoRaWANMacHeader macHdr;
      m_txPkt->PeekHeader (macHdr);
      if (macHdr.IsConfirmed ()) {
        StartAckTimeoutTimer ();
      }
    }
  } else {
      NS_LOG_ERROR (this << " MAC state incorrect " << m_LoRaWANMacState);
    return;
  }

  // Instruct Phy to listen for LoRa frame preamble
  m_phy->SetTRXStateRequest (LORAWAN_PHY_RX_ON);

  // Schedule timer to ask PHY layer whether it has detected a preamble
  // i) Preamble not detected -> Close RW, continue to RW2
  // ii) Preamble detected -> Continue receiving frame. PHY should contact MAC
  // when entire frame has been received.
  // ii) a. frame is intended for end device -> Close RW1, process RX and go back to idle (skip RW2)
  // ii) b. frame is not inteded for ED -> Close RW1, continue to RW2

  Time preambleTime = m_phy->CalculatePreambleTime ();
  m_preambleDetected = Simulator::Schedule (preambleTime, &LoRaWANMac::CheckPhyPreamble, this);

  // TODO: schedule backup timer to close RW in case Phy detects preamble but does not deliver a frame to the MAC?
}

void
LoRaWANMac::CloseRW ()
{
  // This function is called to close the RW in case no frame was received during the RW
  NS_LOG_FUNCTION (this);

  NS_ASSERT (m_deviceType == LORAWAN_DT_END_DEVICE_CLASS_A);

  // Update MAC state?
  if (m_LoRaWANMacState == MAC_RW1) { // no frame received, so should continue to RW2
    m_setMacState = Simulator::ScheduleNow (&LoRaWANMac::SetLoRaWANMacState, this, MAC_WAITFORRW2);
    return;

  } else if (m_LoRaWANMacState == MAC_RW2) { // no frame received, retransmissions or idle?
    bool confirmed = false;
    if (m_txPkt) { // if the transmitted packet was unconfirmed, then it has been removed in RemoveFirstTxQElement which set m_txPkt to NULL
      LoRaWANMacHeader macHdr;
      m_txPkt->PeekHeader (macHdr);
      confirmed = macHdr.IsConfirmed ();
    }

    if (confirmed) {
      // We didn't receive a frame in RW2, so assume that no Ack is comming: go to MAC_ACK_TIMEOUT state
      // Note that the Ack Timeout timer was started at the beginning of RW2
      // Also note that in some cases the RW might only be closed after the Ack timeout timer has already
      // expired (e.g. due to a long packet reception and a short ack timeout time interval),
      // in these cases just go immediatly to MAC_IDLE
      if (m_ackTimeOut.IsRunning ()) {
        m_setMacState = Simulator::ScheduleNow (&LoRaWANMac::SetLoRaWANMacState, this, MAC_ACK_TIMEOUT);
      } else {
        NS_LOG_WARN (this << " Closing RW2 after Ack Timeout timer has expired. Skipping MAC_ACK_TIMEOUT state and going directly to MAC_IDLE state.");
        m_setMacState = Simulator::ScheduleNow (&LoRaWANMac::SetLoRaWANMacState, this, MAC_IDLE);
      }
    } else {
      m_setMacState = Simulator::ScheduleNow (&LoRaWANMac::SetLoRaWANMacState, this, MAC_IDLE);
    }
  } else {
    NS_LOG_ERROR (this << " MAC state incorrect " << m_LoRaWANMacState);
    return;
  }


  // Ask Phy to go into IDLE mode
  // uint8_t phyIndex = 0;
  // Ptr<LoRaWANPhy> phy = m_phy[phyIndex];
  // phy->SetTRXStateRequest(LORAWAN_PHY_IDLE);
}

void
LoRaWANMac::CheckPhyPreamble ()
{
  NS_LOG_FUNCTION (this);

  if (m_phy->preambleDetected ()) {
    // Keep on receiving the Frame, so do nothing for now.
    // Phy should contact MAC once it has finished receiving the frame
    // Either
    // 1) The frame was succesfully received by the phy and phy calls data indication callback (where MAC might or might not accept the frame) or
    // 2) The frame was destroyed during reception (e.g. due to interference) and phy calls data destroyed callback (allowing MAC to handle this)
  } else {
    // No ongoing transmission, in case we are in RW1 or RW2 state. Close RW
    if (m_LoRaWANMacState == MAC_RW1 || m_LoRaWANMacState == MAC_RW2) {
      CloseRW ();
    }
  }
}

void
LoRaWANMac::StartAckTimeoutTimer ()
{
  NS_LOG_FUNCTION (this);

  double min = -ACK_TIMEOUT_RANDOM;
  double max =  ACK_TIMEOUT_RANDOM;
  m_ackTimeOutRandomVariable->SetAttribute ("Min", DoubleValue (min));
  m_ackTimeOutRandomVariable->SetAttribute ("Max", DoubleValue (max));
  // The values returned by a uniformly distributed random
  // variable should always be within the range
  //
  //     [min, max)  .
  //
  double value = m_ackTimeOutRandomVariable->GetValue ();

  Time ackTimeout = MicroSeconds (ACK_TIMEOUT + value);

  NS_LOG_LOGIC (this << " Starting ACK_TIMEOUT of " << ACK_TIMEOUT << " + " << m_ackTimeOutRandomVariable << " = " << ackTimeout);

  m_ackTimeOut = Simulator::Schedule (ackTimeout, &LoRaWANMac::AckTimeoutExpired, this);
}

void
LoRaWANMac::AckTimeoutExpired ()
{
  NS_LOG_FUNCTION (this);

  // Timer should only expire when MAC is in:
  // MAC_RW2: the timer expired before we finished receiving a frame in RW2, in this case AckTimeoutExpired should do nothing. PdDataIndication/PdDataDestroyed will switch the MAC state to IDLE
  // MAC_ACK_TIMEOUT state: the timer expired after we closed RW2 -> switch state to MAC_IDLE
  NS_ASSERT (m_LoRaWANMacState == MAC_RW2 ||  m_LoRaWANMacState == MAC_ACK_TIMEOUT);

  // Switch MAC state to IDLE, so that next frame can be sent. The to be retransmitted frame should still be at the head of the frame queue.
  if (m_LoRaWANMacState == MAC_ACK_TIMEOUT)
    m_setMacState = Simulator::ScheduleNow (&LoRaWANMac::SetLoRaWANMacState, this, MAC_IDLE);
}

// LoRaWANMacRDC class implementation:
LoRaWANMac::LoRaWANMacRDC::LoRaWANMacRDC (void) {
  // init sub bands, EU868
  LoRaWANSubBand g0 = {100, 14, Time (), Time ()}; // g(Note 7), 14dBm?
  LoRaWANSubBand g1 = {100, 14, Time (), Time ()}; // 1%
  LoRaWANSubBand g2 = {1000, 14, Time (), Time ()}; // 0.1%
  LoRaWANSubBand g3 = {10, 27, Time (), Time ()}; // 10%, high power subband
  LoRaWANSubBand g4 = {100, 14, Time (), Time ()}; // 1%

  this->m_subBands.push_back (g0);
  this->m_subBands.push_back (g1);
  this->m_subBands.push_back (g2);
  this->m_subBands.push_back (g3);
  this->m_subBands.push_back (g4);

  // init sub band timers:
  this->m_subBandTimers.push_back (EventId ());
  this->m_subBandTimers.push_back (EventId ());
  this->m_subBandTimers.push_back (EventId ());
  this->m_subBandTimers.push_back (EventId ());
  this->m_subBandTimers.push_back (EventId ());
}

int8_t
LoRaWANMac::LoRaWANMacRDC::GetSubBandIndexForChannelIndex (uint8_t channelIndex) const
{
  const LoRaWANChannel* channelPtr = &LoRaWAN::m_supportedChannels [channelIndex];
  if (channelPtr != NULL)
    return channelPtr->m_subBandIndex;
  else
    return -1;
}

int8_t
LoRaWANMac::LoRaWANMacRDC::GetMaxPowerForSubBand (uint8_t subBandIndex) const
{
  return m_subBands[subBandIndex].maxTXPower;
}

bool
LoRaWANMac::LoRaWANMacRDC::IsSubBandAvailable (uint8_t subBandIndex) const
{
  NS_LOG_FUNCTION (this << (uint16_t)subBandIndex);

  if (m_subBands[subBandIndex].timeoff == 0) // when sending for the first time on this sub band timeoff will be zero, so then the sub band is always available
    return true;

  // For non-zero timeoff check whether enough simtime has expired for this sub band to become available again
  Time simTime = Simulator::Now ();

  bool result = simTime >= (m_subBands[subBandIndex].LastTxFinishedTimestamp + m_subBands[subBandIndex].timeoff);
  NS_LOG_LOGIC (this << " Is " << simTime << " greater than the sum of " << m_subBands[subBandIndex].LastTxFinishedTimestamp << " AND " << m_subBands[subBandIndex].timeoff << "(subbandindex=" << (uint32_t)subBandIndex << "): " << result);
  return result;
}

void
LoRaWANMac::LoRaWANMacRDC::ScheduleSubBandTimer (Ptr<LoRaWANMac> macObj, uint8_t subBandIndex)
{
  NS_LOG_FUNCTION (this << static_cast<int> (subBandIndex));

  if (!m_subBandTimers[subBandIndex].IsRunning ()) {
    Time subBandAvailable = m_subBands[subBandIndex].LastTxFinishedTimestamp + m_subBands[subBandIndex].timeoff;
    // As Simulator::Schedule expects a delay as its time argument we need to subtract Simulator::now() from SubBandAvailable
    Time delay = subBandAvailable  - Simulator::Now ();
    NS_ASSERT (delay > 0); // if equal to zero, then ns3 will get stuck in a loop checking whether the band available ...

    m_subBandTimers[subBandIndex] = Simulator::Schedule (delay, &LoRaWANMac::LoRaWANMacRDC::SubBandTimerExpired, this, macObj, subBandIndex);
    NS_LOG_LOGIC (this << " scheduled timer for subBand #" << (uint16_t)subBandIndex
                     << " at " << delay);
  }
}

void
LoRaWANMac::LoRaWANMacRDC::SubBandTimerExpired (Ptr<LoRaWANMac> macObj, uint8_t subBandIndex)
{
  NS_LOG_FUNCTION (this << (uint16_t) subBandIndex);
  NS_ASSERT(macObj);

  // call SubBandTimerCallback of Mac object that scheduled the timer:
  macObj->SubBandTimerCallback ();
}

void
LoRaWANMac::LoRaWANMacRDC::UpdateRDCTimerForSubBand (uint8_t subBandIndex, Time airTime)
{
  NS_LOG_FUNCTION (this << (uint16_t)subBandIndex << airTime);

  // Assume that this function is called before the frame is transmitted, i.e. the transmission starts at Simulator::Now ()
  Time LastTxFinishedTimestamp = Simulator::Now () + airTime;
  Time timeoff = airTime*m_subBands[subBandIndex].dutyCycleLimit - airTime;

  m_subBands[subBandIndex].LastTxFinishedTimestamp = LastTxFinishedTimestamp;
  m_subBands[subBandIndex].timeoff = timeoff;

  NS_LOG_LOGIC (this << " updated RDC for subBand " << (uint16_t)subBandIndex << ": "
                     << LastTxFinishedTimestamp << ", "
                     << timeoff);
}

int64_t
LoRaWANMac::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_ackTimeOutRandomVariable);
  m_ackTimeOutRandomVariable->SetStream (stream);
  return 1;
}
} // namespace ns3
