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
#ifndef LORAWAN_MAC_H
#define LORAWAN_MAC_H

#include "lorawan.h"
#include "lorawan-phy.h"
#include <ns3/object.h>
#include <ns3/traced-callback.h>
#include <ns3/traced-value.h>
#include <ns3/ipv4-address.h>
#include <ns3/nstime.h>
#include <ns3/event-id.h>
#include <ns3/timer.h>
#include <deque>

// Default settings for EU863-870
#define ACK_TIMEOUT 2000000 // in uS
#define ACK_TIMEOUT_RANDOM 1000000 // in uS

namespace ns3 {

class Packet;

/* ... */
/**
 * \ingroup lorawan
 *
 * MAC states
 */
typedef enum
{
  MAC_IDLE,              //!< MAC_IDLE
  MAC_TX,                //!< MAC_TX
  MAC_WAITFORRW1,        //!< MAC_WAITFORRW1
  MAC_RW1,               //!< MAC_RW1
  MAC_WAITFORRW2,        //!< MAC_WAITFORRW2
  MAC_RW2,		 //!< MAC_RW2
  //MAC_RX,              //!< MAC_RX
  MAC_ACK_TIMEOUT, 	 //!< MAC_ACK_TIMEOUT, MAC state during which the MAC is waiting for the ACK_TIMEOUT (no TX is allowed during this state)
  MAC_UNAVAILABLE, 	         //!< MAC_UNAVAILABLE, MAC is currently unavailable to perform any operation (e.g. other MAC on same device is currently sending)
} LoRaWANMacState;

namespace TracedValueCallback {

/**
 * \ingroup LoRaWAN
 * TracedValue callback signature for LoRaWANMacState.
 *
 * \param [in] oldValue original value of the traced variable
 * \param [in] newValue new value of the traced variable
 */
  typedef void (* LoRaWANMacState)(LoRaWANMacState oldValue,
                                  LoRaWANMacState newValue);
}  // namespace TracedValueCallback

/**
 * \ingroup lorawan
 *
 * Sub bands and their duty cycle limits
 */
typedef struct LoRaWANSubBand {
  //std::string subBandName;
  // double fl;
  // double fh;

  uint16_t dutyCycleLimit; // Actual limit is 1 over this value: 10 -> 10%; 100 -> 1%; 1000 => 0.1%
  int8_t maxTXPower;

  Time LastTxFinishedTimestamp; // Simulator Time of when last TX on this sub band finished
  Time timeoff; // Simulator time for timeoff duration on this sub band
} LoRaWANSubBand;

typedef enum
{
  LORAWAN_SUCCESS                = 0,
  LORAWAN_NO_ACK                 = 2,
} LoRaWANMcpsDataConfirmStatus;

/**
 * \ingroup lorawan
 *
 * Metadata to be provided when sending a MACPayload message
 */
struct LoRaWANDataRequestParams
{
  uint8_t m_loraWANChannelIndex; 	//!< Index of LoRaWAN channel
  uint8_t m_loraWANDataRateIndex; 	//!< Index of LoRa Data Rate
  //LoRaWANChannel m_loraWANChannel; 				//!< LoRaWAN channel
  //LoRaWANDataRate m_loraWANDataRate;
  uint8_t m_loraWANCodeRate;			//!< Code rate of transmission

  LoRaWANMsgType m_msgType; 			//!< Message Type
  //uint8_t m_fPort; 			//!< Frame Port
  //uint32_t m_endDeviceAddress; 	//!< End Device Address

  uint32_t m_requestHandle;			//!< Identifier for upper layer of data request
  uint8_t m_numberOfTransmissions;	//!< Number of remaining transmissions for this message

  friend std::ostream& operator<< (std::ostream& os, const LoRaWANDataRequestParams& p);
};

struct LoRaWANDataConfirmParams
{
  uint32_t m_requestHandle;			//!< Identifier for upper layer of data request
  LoRaWANMcpsDataConfirmStatus m_status; //!< The status of the last MSDU transmission
};

/**
 * \ingroup lorawan
 *
 * Metadata for a received MACPayload message
 */
struct LoRaWANDataIndicationParams
{
  uint8_t m_channelIndex;		//!< Channel index of received transmission
  uint8_t m_dataRateIndex;		//!< Data rate index of received transmission
  uint8_t m_codeRate;			//!< Code rate of received transmission

  LoRaWANMsgType m_msgType; 		//!< Message Type
  Ipv4Address m_endDeviceAddress; 	//!< End Device Address
  uint32_t m_MIC;			//!< MIC
};


/**
 * \ingroup lorawan
 *
 * This callback is called immediately before when this MAC object changes its state to the TX state.
 * Note: only for gateways.
 */
class LoRaWANMac;
typedef Callback<void, Ptr<LoRaWANMac> > BeginTxCallback;
/**
 * \ingroup lorawan
 *
 * This callback is called immediately after when a MAC object has finished a transmission.
 * Note: only for gateways.
 */
typedef Callback<void, Ptr<LoRaWANMac> > EndTxCallback;

/**
 * \ingroup lorawan
 *
 * This callback is called after a McpsDataRequest has been called from
 * the higher layer.  It returns a status of the outcome of the
 * transmission request
 */
typedef Callback<void, LoRaWANDataConfirmParams> DataConfirmCallback;

/**
 * \ingroup lorawan
 *
 * This callback is called after a Mcps has successfully received a
 *  frame and wants to deliver it to the higher layer.
 *
 *  \todo for now, we do not deliver all of the parameters in section
 *  7.1.1.3.1 but just send up the packet.
 */
typedef Callback<void, LoRaWANDataIndicationParams, Ptr<Packet> > DataIndicationCallback;

/**
 * \ingroup lorawan
 *
 * Class that implements the LoRaWAN Mac state machine
 * Note that a MAC object is strongly coupled with its underlying PHY, as such
 * there is one LoRaWANMac per PHY. End devices typically support one MAC/PHY
 * combo, but a gateway supports many MAC/PHY objects.
 */
class LoRaWANMac : public Object
{

public:
  /**
   * \ingroup lorawan
   *
   * Keeping track of RDC limitations accross multiple MAC objects for one lorawan node
   */
  class LoRaWANMacRDC : public Object
  {
  public:
    LoRaWANMacRDC (void);

    int8_t GetSubBandIndexForChannelIndex (uint8_t channelIndex) const;
    int8_t GetMaxPowerForSubBand (uint8_t subBandIndex) const;
    bool IsSubBandAvailable (uint8_t subBandIndex) const;

    void UpdateRDCTimerForSubBand (uint8_t subBandIndex, Time airTime);

    void ScheduleSubBandTimer (Ptr<LoRaWANMac> macObj, uint8_t subBandIndex);
    void SubBandTimerExpired (Ptr<LoRaWANMac> macObj, uint8_t subBandIndex);
  private:
    /**
     * The RDC limitations per sub-band
     */
    std::vector<LoRaWANSubBand> m_subBands;

    std::vector<EventId> m_subBandTimers;
  };

  /**
   * Get the type ID.
   *
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  /**
   * Default constructor.
   */
  LoRaWANMac (void);
  LoRaWANMac (uint8_t index);
  virtual ~LoRaWANMac (void);

  /**
   * Set the underlying PHY for the MAC.
   *
   * \param phy the PHY
   */
  void SetPhy (Ptr<LoRaWANPhy> phy);

  /**
   * Get the underlying PHY of the MAC.
   *
   * \return the PHY
   */
  Ptr<LoRaWANPhy> GetPhy (void);

  /**
   * Set the LoRaWANMacRDC object of this MAC.
   *
   * \param macRDC the RDC object
   */
  void SetRDC (Ptr<LoRaWANMacRDC> macRDC);

  void SetLoRaWANMacState (LoRaWANMacState macState);
  LoRaWANMacState GetLoRaWANMacState () const  { return this->m_LoRaWANMacState; }
  bool IsLoRaWANMacStateRunning () const { return this->m_setMacState.IsRunning (); }

  void ChangeMacState (LoRaWANMacState newState);

  void SetDataIndicationCallback (DataIndicationCallback c);

  void SetDataConfirmCallback (DataConfirmCallback c);

  void SetBeginTxCallback (BeginTxCallback c);
  void SetEndTxCallback (EndTxCallback c);

  void SwitchToUnavailableState ();
  void SwitchToIdleState ();

  void PdDataDestroyed (void);
  void PdDataIndication (uint32_t phyPayloadLength, Ptr<Packet> p, uint8_t lqi, uint8_t channelIndex, uint8_t dataRateIndex, uint8_t codeRate);

  /**
   *  Report status of Phy TRX state switch to MAC
   *  @param status of the state switch
   */
  void SetTRXStateConfirm (LoRaWANPhyEnumeration status);

  /**
   *  Confirm the end of transmission of an MPDU to MAC
   *  @param status to report to MAC
   */
  void PdDataConfirm (LoRaWANPhyEnumeration status);

  Ipv4Address GetDevAddr (void) const;
  void SetDevAddr (Ipv4Address);

  LoRaWANDeviceType GetDeviceType (void) const;
  void SetDeviceType (LoRaWANDeviceType);

  uint8_t GetIndex (void) const;

  uint8_t GetRX1DROffset (void) const;
  void SetRX1DROffset (uint8_t);

  /**
   *  Request to transfer a MAC payload.
   *
   *  \param params parameters that describe the transmission
   *  \param p the packet to be transmitted
   */
  void sendMACPayloadRequest (LoRaWANDataRequestParams params, Ptr<Packet> p);

  /**
   * TracedCallback signature for sent packets.
   *
   * \param [in] packet The packet.
   * \param [in] retries The number of retries.
   */
  typedef void (* SentTracedCallback)
    (Ptr<const Packet> packet, uint8_t retries);

  /**
   * Assign a fixed random variable stream number to the random variables
   * used by this model.  Return the number of streams that have been assigned.
   *
   * \param stream first stream index to use
   * \return the number of stream indices assigned by this model
   */
  int64_t AssignStreams (int64_t stream);

protected:
  // Inherited from Object.
  virtual void DoInitialize (void);
  virtual void DoDispose (void);

  void sendTRXStateRequestForIdleMAC ();
  Ptr<Packet> constructPhyPayload (LoRaWANDataRequestParams params, Ptr<Packet> p);

  void CheckQueue ();
  void CheckRetransmission ();
  void RemoveFirstTxQElement (bool sentPacket);

  bool ConfigurePhyForTX ();

  void SubBandTimerCallback ();

  void OpenRW ();
  void CloseRW ();
  void CheckPhyPreamble ();
  void StartAckTimeoutTimer ();
  void AckTimeoutExpired ();

  //void StartTransmission();
  //void EndTransmission();
private:

  /**
   * The trace source fired when packets are considered as successfully sent
   * or the transmission has been given up.
   *
   * The data should represent: packet, number of retries
   *
   * \see class CallBackTraceSource
   */
  TracedCallback<Ptr<const Packet>, uint8_t> m_sentPktTrace;

  /**
   * The trace source fired when packets come into the "top" of the device
   * at the L3/L2 transition, when being queued for transmission.
   *
   * \see class CallBackTraceSource
   */
  TracedCallback<Ptr<const Packet> > m_macTxEnqueueTrace;

  /**
   * The trace source fired when packets are dequeued from the
   * L3/l2 transmission queue.
   *
   * \see class CallBackTraceSource
   */
  TracedCallback<Ptr<const Packet> > m_macTxDequeueTrace;

  /**
   * The trace source fired when packets are being sent down to L1.
   *
   * \see class CallBackTraceSource
   */
  TracedCallback<Ptr<const Packet> > m_macTxTrace;

  /**
   * The trace source fired when packets where successfully transmitted, that is
   * an acknowledgment was received, if requested, or the packet was
   * successfully sent by L1, if no ACK was requested.
   *
   * \see class CallBackTraceSource
   */
  TracedCallback<Ptr<const Packet> > m_macTxOkTrace;

  /**
   * The trace source fired when packets are dropped due to missing ACKs or
   * because of transmission failures in L1.
   *
   * \see class CallBackTraceSource
   */
  TracedCallback<Ptr<const Packet> > m_macTxDropTrace;

  /**
   * The trace source fired for packets successfully received by the device
   * immediately before being forwarded up to higher layers (at the L2/L3
   * transition).
   *
   * \see class CallBackTraceSource
   */
  TracedCallback<Ptr<const Packet> > m_macRxTrace;

  /**
   * The trace source fired for packets successfully received by the device
   * but dropped before being forwarded up to higher layers (at the L2/L3
   * transition).
   *
   * \see class CallBackTraceSource
   */
  TracedCallback<Ptr<const Packet> > m_macRxDropTrace;

  /**
   * The index of this Mac object in the lorawan net device
   */
  uint8_t m_index;

  /**
   * The PHY's associated with this MAC.
   */
  Ptr<LoRaWANPhy> m_phy;

  /*
   * The RDC object belonging to this node
   */
  Ptr<LoRaWANMacRDC> m_lorawanMacRDC;

  /*
   * Maximum MACPayload size length as per $7.1.6
   */
  const static uint8_t maxMACPayloadSize[]; // defined for LoRaWAN DR0 to DR7

  /**
   * The type of device: End Device or Gateway
   */
  LoRaWANDeviceType m_deviceType;

  /*
   * The offset for the RW1 downstream data rate as per section 7.1.7
   * Only applicable to end devices
   */
  uint8_t m_RX1DROffset;

  /**
   * The current states of the MAC layer. One per Phy.
   */
  TracedValue<LoRaWANMacState> m_LoRaWANMacState;

  /**
   * This callback is used to switch off other MACs and PHYs objects, just prior to when this MAC object starts transmision
   * Only for gateways
   * */
  BeginTxCallback m_beginTxCallback;

  /**
   * This callback is used to switch other MACs and PHYs objects back to IDLE, just after when this MAC object ends transmision
   * Only for gateways
   * */
  EndTxCallback m_endTxCallback;

  /**
   * This callback is used to notify incoming packets to the upper layers.
   */
  DataIndicationCallback m_dataIndicationCallback;

  /**
   * This callback is used to report data transmission request status to the
   * upper layers.
   */
  DataConfirmCallback m_dataConfirmCallback;

  /**
   * Helper structure for managing transmission queue elements.
   */
  struct TxQueueElement
  {
    LoRaWANDataRequestParams lorawanDataRequestParams; //!< Data request Params
    Ptr<Packet> txQPkt;    //!< Queued packet
  };

  /**
   * The transmit queue used by the MAC.
   */
  std::deque<TxQueueElement*> m_txQueue;

  /**
   * The packet which is currently being sent by the MAC layer.
   * Maximum of one packet at a time, even for the Gateway.
   */
  Ptr<Packet> m_txPkt;  // XXX need packet buffer instead of single packet

  /**
   * The number of already used retransmission for the currently transmitted
   * packet.
   */
  uint8_t m_retransmission;

  /**
   * The short device address identifies the end-device with the current
   * network.
   */
  Ipv4Address m_devAddr;

  /**
   * Scheduler event for a deferred MAC state change.
   */
  EventId m_setMacState;

  /**
   * Scheduler event for a preamble detected check.
   */
  EventId m_preambleDetected;

  /**
   * Scheduler event for an Ack timeout that expired
   */
  EventId m_ackTimeOut;

  /**
   * The Time when the last uplink bit was transmitted
   * Only used in class A end devices for calculating the start of RW1 and RW2
   */
  Time m_lastUplinkBitTime;

  /**
   * The random variable used to calculate the random fraction of the Ack
   * time-out timer
   */
  Ptr<UniformRandomVariable> m_ackTimeOutRandomVariable;
}; // class LoRaWANMac

} // namespace ns3

#endif /* LORAWAN_MAC_H */
