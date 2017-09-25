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
#ifndef LORAWAN_NET_DEVICE_H
#define LORAWAN_NET_DEVICE_H

#include <ns3/net-device.h>
#include <ns3/traced-callback.h>
#include <ns3/lorawan-phy.h>
#include <ns3/lorawan-mac.h>
#include <ns3/lorawan.h>

namespace ns3 {

/* ... */


/**
 * \brief Hold together all LoRaWAN-related objects.
 * \ingroup lorawan
 *
 */
class LoRaWANNetDevice : public NetDevice
{
public:
  static TypeId GetTypeId (void);

  LoRaWANNetDevice ();
  LoRaWANNetDevice (LoRaWANDeviceType deviceType);
  virtual ~LoRaWANNetDevice ();

  /**
   * \param mac the mac layer to use.
   */
  void SetMac (Ptr<LoRaWANMac> mac);
  /**
   * \param phy the phy layer to use.
   */
  void SetPhy (Ptr<LoRaWANPhy> phy);

  void SetChannel (Ptr<SpectrumChannel> channel);

  /**
   * \returns the mac we are currently using.
   */
  Ptr<LoRaWANMac> GetMac (void) const;
  std::vector<Ptr<LoRaWANMac> > GetMacs (void) const;

  /**
   * \returns the phy we are currently using.
   */
  Ptr<LoRaWANPhy> GetPhy (void) const;
  std::vector<Ptr<LoRaWANPhy> > GetPhys (void) const;

  //inherited from NetDevice base class.
  virtual void SetIfIndex (const uint32_t index);
  virtual uint32_t GetIfIndex (void) const;
  virtual Ptr<Channel> GetChannel (void) const;
  virtual void SetAddress (Address address);
  virtual Address GetAddress (void) const;
  virtual bool SetMtu (const uint16_t mtu);
  virtual uint16_t GetMtu (void) const;
  virtual bool IsLinkUp (void) const;
  virtual void AddLinkChangeCallback (Callback<void> callback);
  virtual bool IsBroadcast (void) const;
  virtual Address GetBroadcast (void) const;
  virtual bool IsMulticast (void) const;
  virtual Address GetMulticast (Ipv4Address multicastGroup) const;
  virtual bool IsPointToPoint (void) const;
  virtual bool IsBridge (void) const;
  virtual bool Send (Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber);
  virtual bool SendFrom (Ptr<Packet> packet, const Address& source, const Address& dest, uint16_t protocolNumber);
  virtual bool SupportsSendFrom (void) const;
  virtual Ptr<Node> GetNode (void) const;
  virtual void SetNode (Ptr<Node> node);
  virtual bool NeedsArp (void) const;
  virtual void SetReceiveCallback (NetDevice::ReceiveCallback cb);
  virtual void SetPromiscReceiveCallback (PromiscReceiveCallback cb);

  bool getMACSIndexForChannelAndDataRate (uint8_t& macsIndex, uint8_t channelIndex, uint8_t dataRateIndex);

  void MacBeginsTx (Ptr<LoRaWANMac> macPtr);
  void MacEndsTx (Ptr<LoRaWANMac> macPtr);

  bool CanSendImmediatelyOnChannel (uint8_t channelIndex, uint8_t dataRateIndex);

  LoRaWANDeviceType GetDeviceType (void) const;
  // void SetDeviceType (LoRaWANDeviceType type);

  virtual Address GetMulticast (Ipv6Address addr) const;

  /**
   * The callback used by the MAC to hand over incoming packets to the
   * NetDevice. This callback will in turn use the ReceiveCallback set by
   * SetReceiveCallback() to notify upper layers.
   *
   * \param params LoRaWAN specific parameters
   * \param pkt the packet do be delivered
   */
  void DataIndication (LoRaWANDataIndicationParams params, Ptr<Packet> pkt);

  /**
   * Assign a fixed random variable stream number to the random variables
   * used by this model.  Return the number of streams that have been assigned.
   *
   * \param stream first stream index to use
   * \return the number of stream indices assigned by this model
   */
  int64_t AssignStreams (int64_t stream);

  void SetMTUSpreadingFactor (LoRaSpreadingFactor sf) { this->m_mtuSpreadingFactor = sf; }
  LoRaSpreadingFactor GetMTUSpreadingFactor () { return this->m_mtuSpreadingFactor; }

private:
  // Inherited from NetDevice/Object
  virtual void DoDispose (void);
  virtual void DoInitialize (void);

  /**
   * Mark NetDevice link as up.
   */
  void LinkUp (void);

  /**
   * Mark NetDevice link as down.
   */
  void LinkDown (void);

  /**
   * Attribute accessor method for the "Channel" attribute.
   *
   * \return the channel to which this NetDevice is attached
   */

  Ptr<SpectrumChannel> DoGetChannel (void) const;

  /**
   * Configure PHY and MAC.
   */
  void CompleteConfig (void);

  Ptr<Node> m_node;
  // For end device: One phy/mac
  Ptr<LoRaWANPhy> m_phy;
  Ptr<LoRaWANMac> m_mac;
  // For gateways: multiple phys/macs (note one mac per phy)
  std::vector<Ptr<LoRaWANPhy> > m_phys;
  std::vector<Ptr<LoRaWANMac> > m_macs;

  Ptr<LoRaWANMac::LoRaWANMacRDC> m_macRDC;
  LoRaWANDeviceType m_deviceType;

  /**
   * True if MAC, PHY and CSMA/CA where successfully configured and the
   * NetDevice is ready for being used.
   */
  bool m_configComplete;

  /**
   * Configure the NetDevice to request MAC layer acknowledgments when sending
   * packets using the Send() API.
   */
  //bool m_useAcks;

  /**
   * Is the link/device currently up and running?
   */
  bool m_linkUp;

  /**
   * The interface index of this NetDevice.
   */
  uint32_t m_ifIndex;

  /**
   * Trace source for link up/down changes.
   */
  TracedCallback<> m_linkChanges;

  /**
   * Upper layer callback used for notification of new data packet arrivals.
   */
  ReceiveCallback m_receiveCallback;

  /**
   * the number of repetitions (NbRep) field is the number of repetition
   * for each uplink message. This applies only to unconfirmed uplink frames.
   * The default value is 1. The valid range is [1:15]
   *
   * The end-device performs frequency hopping as usual between repeated
   * transmissions, it does wait after each repetition until the receive windows
   * have expired.
   */
  uint8_t m_nbRep;

  LoRaSpreadingFactor 	  m_mtuSpreadingFactor;	//!< The spreading factor to be used for checking MTU limitations.

}; // class LoRaWANNetDevice

} // namespace ns3

#endif /* LORAWAN_H */
