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
#ifndef LORAWAN_GATEWAY_APPLICATION_H
#define LORAWAN_GATEWAY_APPLICATION_H

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/data-rate.h"
#include "ns3/traced-callback.h"
#include "ns3/traced-value.h"
#include "ns3/simple-ref-count.h"
#include "ns3/random-variable-stream.h"
#include <unordered_map>
#include <deque>

namespace ns3 {

namespace TracedValueCallback {
/**
 * \ingroup lorawan
 * TracedCallback signature for tracing NS message events: DS msg transmitted, DS msg ackd and DS msg Dropped
 *
 * \param [in] deviceAddr The device address to whom the DS msg is addressed.
 * \param [in] txRemaining Number of remaining transmissions for message.
 * \param [in] msgType MAC message type of message.
 * \param [in] packet The DS message.
 */

  typedef void (* LoRaWANDSMessageTransmittedTracedCallback) (uint32_t deviceAddr, uint8_t txRemaining, uint8_t msgType, Ptr<const Packet> packet);

/**
 * \ingroup lorawan
 * TracedCallback signature for tracing NS message events: DS msg transmitted, DS msg ackd and DS msg Dropped
 *
 * \param [in] deviceAddr The device address to whom the DS msg is addressed.
 * \param [in] txRemaining Number of remaining transmissions for message.
 * \param [in] msgType MAC message type of message.
 * \param [in] packet The DS message.
 */

  typedef void (* LoRaWANDSMessageTracedCallback) (uint32_t deviceAddr, uint8_t txRemaining, uint8_t msgType, Ptr<const Packet> packet);
}  // namespace TracedValueCallback

class Address;
class RandomVariableStream;
class Socket;
class LoRaWANGatewayApplication;

typedef struct LoRaWANNSDSQueueElement {
  Ptr<Packet>     m_downstreamPacket;
  uint8_t         m_downstreamFramePort;
  LoRaWANMsgType  m_downstreamMsgType;
  uint8_t 	  m_downstreamTransmissionsRemaining;
  bool 		  m_isRetransmission;
} LoRaWANNSDSQueueElement;

typedef struct LoRaWANEndDeviceInfoNS {
  LoRaWANEndDeviceInfoNS () : m_deviceAddress(), m_rx1DROffset(0), m_lastDSGW(nullptr), m_lastGWs(),
	m_lastDataRateIndex(0), m_lastChannelIndex(0), m_lastCodeRate(0), m_lastSeen(0),
	m_framePending(false),m_setAck(false), m_fCntUp(0), m_fCntDown(0),
	m_nUSPackets(0), m_nUniqueUSPackets(0), m_nUSRetransmission(0), m_nUSDuplicates(0), m_nUSAcks(0),
	m_nDSPacketsGenerated(0), m_nDSPacketsSent(0), m_nDSPacketsSentRW1(0), m_nDSPacketsSentRW2(0), m_nDSRetransmission(0), m_nDSAcks(0),
	m_rw1Timer(), m_rw2Timer(), m_downstreamQueue(),m_downstreamTimer() {}

  Ipv4Address     m_deviceAddress;
  uint8_t 	  m_rx1DROffset;
  Ptr<LoRaWANGatewayApplication> m_lastDSGW;
  std::vector< Ptr<LoRaWANGatewayApplication> > m_lastGWs;
  uint8_t         m_lastDataRateIndex;
  uint8_t         m_lastChannelIndex;
  uint8_t         m_lastCodeRate;
  Time            m_lastSeen;
  bool            m_framePending;
  bool            m_setAck;
  uint32_t        m_fCntUp;       //!< Uplink frame counter
  uint32_t        m_fCntDown;     //!< Downlink frame counter

  uint32_t 	  m_nUSPackets;   //!< The total number of received US packets
  uint32_t 	  m_nUniqueUSPackets;   //!< Number of received unique US packets (i.e. with a new US frame counter)
  uint32_t        m_nUSRetransmission;  //!< Number of upstream retransmission received
  uint32_t        m_nUSDuplicates;  //!< Number of upstream duplicates received
  uint32_t        m_nUSAcks;  //!< Number of upstream acks received

  uint32_t 	  m_nDSPacketsGenerated;   //!< The total number of generated DS packets
  uint32_t 	  m_nDSPacketsSent;   //!< The total number of sent DS packets
  uint32_t 	  m_nDSPacketsSentRW1;   //!< The number of sent DS packets in RW1
  uint32_t 	  m_nDSPacketsSentRW2;   //!< The number of sent DS packets in RW2
  uint32_t 	  m_nDSRetransmission;   //!< Number of retransmissions sent for of DS packets
  uint32_t        m_nDSAcks;  //!< Number of downstream acks sent

  EventId	  m_rw1Timer;
  EventId	  m_rw2Timer;

  // Pending downstream traffic
  std::deque<LoRaWANNSDSQueueElement* > m_downstreamQueue;

  EventId 	  m_downstreamTimer; // DS traffic generator timer
} LoRaWANEndDeviceInfoNS;

//class LoRaWANNetworkServer : public SimpleRefCount<LoRaWANNetworkServer>
class LoRaWANNetworkServer : public Object
{
public:
  LoRaWANNetworkServer ();

  static TypeId GetTypeId (void);
  virtual void DoInitialize (void);
  virtual void DoDispose (void);

  void PopulateEndDevices (void);
  LoRaWANEndDeviceInfoNS InitEndDeviceInfo (Ipv4Address);

  static void clearLoRaWANNetworkServerPointer () { LoRaWANNetworkServer::m_ptr = nullptr; }
  static bool haveLoRaWANNetworkServerObject () { return LoRaWANNetworkServer::m_ptr != NULL; }
  static Ptr<LoRaWANNetworkServer> getLoRaWANNetworkServerPointer ();

  void SetConfirmedDataDown (bool confirmedData);
  bool GetConfirmedDataDown (void) const;

  void HandleUSPacket (Ptr<LoRaWANGatewayApplication>, Address from, Ptr<Packet> packet);
  void RW1TimerExpired (uint32_t deviceAddr);
  void RW2TimerExpired (uint32_t deviceAddr);
  void SendDSPacket (uint32_t deviceAddr, Ptr<LoRaWANGatewayApplication> gatewayPtr, bool RW1, bool RW2);
  bool HaveSomethingToSendToEndDevice (uint32_t deviceAddr);
  void DSTimerExpired (uint32_t deviceAddr);
  void DeleteFirstDSQueueElement (uint32_t deviceAddr);

  int64_t AssignStreams (int64_t stream);
private:
  static Ptr<LoRaWANNetworkServer> m_ptr;
  std::unordered_map <uint32_t, LoRaWANEndDeviceInfoNS> m_endDevices;
  uint16_t m_pktSize;
  bool m_generateDataDown;
  bool m_confirmedData;
  bool m_endDevicesPopulated;
  Ptr<RandomVariableStream> m_downstreamIATRandomVariable;
  TracedValue<uint32_t> m_nrRW1Sent; // number of times that a DS packet was sent in RW1 by this NS
  TracedValue<uint32_t> m_nrRW2Sent; // number of times that a DS packet was sent in RW2 by this NS
  TracedValue<uint32_t> m_nrRW1Missed; // number of times that RW1 was missed for all end devices served by this NS
  TracedValue<uint32_t> m_nrRW2Missed; // number of times that RW2 was missed for all end devices served by this NS

  TracedCallback<uint32_t, uint8_t, uint8_t, Ptr<const Packet> > m_dsMsgGeneratedTrace;
  TracedCallback<uint32_t, uint8_t, uint8_t, Ptr<const Packet>, uint8_t > m_dsMsgTransmittedTrace;
  TracedCallback<uint32_t, uint8_t, uint8_t, Ptr<const Packet> > m_dsMsgAckdTrace;
  TracedCallback<uint32_t, uint8_t, uint8_t, Ptr<const Packet> > m_dsMsgDroppedTrace;
  TracedCallback<uint32_t, uint8_t, Ptr<const Packet>> m_usMsgReceivedTrace;
};

class LoRaWANGatewayApplication : public Application
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  LoRaWANGatewayApplication ();

  virtual ~LoRaWANGatewayApplication();

  /**
   * \brief Set the total number of bytes to send.
   *
   * Once these bytes are sent, no packet is sent again, even in on state.
   * The value zero means that there is no limit.
   *
   * \param maxBytes the total number of bytes to send
   */
  void SetMaxBytes (uint64_t maxBytes);

  /**
   * \brief Return a pointer to associated socket.
   * \return pointer to associated socket
   */
  Ptr<Socket> GetSocket (void) const;

 /**
  * \brief Assign a fixed random variable stream number to the random variables
  * used by this model.
  *
  * \param stream first stream index to use
  * \return the number of stream indices assigned by this model
  */
  int64_t AssignStreams (int64_t stream);

  /**
   * \brief Handle a packet received by the application
   * \param socket the receiving socket
   */
  void HandleRead (Ptr<Socket> socket);

  bool CanSendImmediatelyOnChannel (uint8_t channelIndex, uint8_t dataRateIndex);
  void SendDSPacket (Ptr<Packet> p);
protected:
  virtual void DoInitialize (void);
  virtual void DoDispose (void);

private:
  // inherited from Application base class.
  virtual void StartApplication (void);    // Called at time specified by Start
  virtual void StopApplication (void);     // Called at time specified by Stop

  //helpers
  /**
   * \brief Send a packet
   */
  void SendPacket ();

  Ptr<Socket>     m_socket;       //!< Associated socket
  bool            m_connected;    //!< True if connected
  uint32_t        m_pktSize;      //!< Size of packets
  uint32_t        m_residualBits; //!< Number of generated, but not sent, bits
  Time            m_lastStartTime; //!< Time last packet sent
  uint64_t        m_maxBytes;     //!< Limit total number of bytes sent
  uint64_t        m_totBytes;     //!< Total bytes sent so far

  uint8_t         m_framePort;	  //!< Frame port
  uint32_t        m_fCntUp;       //!< Uplink frame counter
  uint32_t        m_fCntDown;     //!< Downlink frame counter
  bool            m_setAck;      //!< Set the Ack bit in the next transmission

  /// Traced Callback: transmitted packets.
  TracedCallback<Ptr<const Packet> > m_txTrace;

  Ptr<LoRaWANNetworkServer> m_lorawanNSPtr; //!< Pointer to LoRaWANNetworkServer singleton

private:
  /**
   * \brief Schedule the next packet transmission
   */
  void ScheduleNextTx ();
  /**
   * \brief Schedule the next On period start
   */
  void ScheduleStartEvent ();
  /**
   * \brief Schedule the next Off period start
   */
  void ScheduleStopEvent ();
  /**
   * \brief Handle a Connection Succeed event
   * \param socket the connected socket
   */
  void ConnectionSucceeded (Ptr<Socket> socket);
  /**
   * \brief Handle a Connection Failed event
   * \param socket the not connected socket
   */
  void ConnectionFailed (Ptr<Socket> socket);

  uint64_t        m_totalRx;      //!< Total bytes received
};

} // namespace ns3

#endif /* LORAWAN_ENDDEVICE_APPLICATION_H */
