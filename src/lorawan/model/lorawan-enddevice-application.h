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

#ifndef LORAWAN_ENDDEVICE_APPLICATION_H
#define LORAWAN_ENDDEVICE_APPLICATION_H

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/data-rate.h"
#include "ns3/traced-callback.h"

namespace ns3 {

class Address;
class RandomVariableStream;
class Socket;

/**
 * \ingroup lorawan
 * \defgroup onoff LoRaWANEndDeviceApplication
 *
 * This application is intended to run on LoRaWAN end devices class A and was
 * based of the OnOffApplication, though it has changed drastically in that
 * US messages are generated according to a random variable (can be fixed) and
 * not according to a CBR requirement.
*/
class LoRaWANEndDeviceApplication : public Application
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  LoRaWANEndDeviceApplication ();

  virtual ~LoRaWANEndDeviceApplication();

  /**
   * \brief Set the total number of bytes to send.
   *
   * Once these bytes are sent, no packet is sent again, even in on state.
   * The value zero means that there is no limit.
   *
   * \param maxBytes the total number of bytes to send
   */
  void SetMaxBytes (uint64_t maxBytes);

  uint32_t GetDataRateIndex (void) const;
  void SetDataRateIndex (uint32_t index);

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

protected:
  virtual void DoDispose (void);
private:
  // inherited from Application base class.
  virtual void StartApplication (void);    // Called at time specified by Start
  virtual void StopApplication (void);     // Called at time specified by Stop

  //helpers
  /**
   * \brief Cancel all pending events.
   */
  void CancelEvents ();

  // Event handlers
  /**
   * \brief Send a packet
   */
  void SendPacket ();

  void HandleRead (Ptr<Socket> socket);

  void HandleDSPacket (Ptr<Packet> p, Address from);

  Ptr<Socket>     m_socket;       //!< Associated socket
  bool            m_connected;    //!< True if connected
  Ptr<RandomVariableStream> m_channelRandomVariable;	//!< rng for channel selection for upstream TX
  Ptr<RandomVariableStream> m_upstreamIATRandomVariable;	//!< rng for inter arrival timing for upstream TX
  uint32_t        m_pktSize;      //!< Size of packets
  uint32_t 	  m_dataRateIndex;	//!< Data rate index to use for US transmissions
  Time            m_lastTxTime; //!< Time last packet sent
  uint64_t        m_maxBytes;     //!< Limit total number of bytes sent
  uint64_t        m_totBytes;     //!< Total bytes sent so far
  EventId         m_txEvent;     //!< Event id for next start or stop event
  bool 		  m_confirmedData; //<! Send upstream data as Confirmed Data Up MAC packets

  uint8_t         m_framePort;	  //!< Frame port
  uint32_t        m_fCntUp;       //!< Uplink frame counter
  uint32_t        m_fCntDown;     //!< Downlink frame counter
  bool            m_setAck;      //!< Set the Ack bit in the next transmission
  uint64_t        m_totalRx;      //!< Total bytes received

  /// Traced Callback: transmitted packets.
  TracedCallback<uint32_t, uint8_t, Ptr<const Packet>> m_usMsgTransmittedTrace;

  /// Traced Callback: received packets, source address, receive window.
  TracedCallback<uint32_t, uint8_t, Ptr<const Packet>, uint8_t> m_dsMsgReceivedTrace;
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
};

} // namespace ns3

#endif /* LORAWAN_ENDDEVICE_APPLICATION_H */
