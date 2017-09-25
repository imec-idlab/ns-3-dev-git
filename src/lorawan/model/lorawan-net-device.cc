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
#include "lorawan-net-device.h"
#include "lorawan-error-model.h"
#include <ns3/abort.h>
#include <ns3/assert.h>
#include <ns3/node.h>
#include <ns3/log.h>
#include <ns3/spectrum-channel.h>
#include <ns3/pointer.h>
#include <ns3/boolean.h>
#include <ns3/mobility-model.h>
#include <ns3/packet.h>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LoRaWANNetDevice");

NS_OBJECT_ENSURE_REGISTERED (LoRaWANNetDevice);

TypeId
LoRaWANNetDevice::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LoRaWANNetDevice")
    .SetParent<NetDevice> ()
    .SetGroupName ("LoRaWAN")
    .AddConstructor<LoRaWANNetDevice> ()
    .AddAttribute ("Channel", "The channel attached to this device",
                   PointerValue (),
                   MakePointerAccessor (&LoRaWANNetDevice::DoGetChannel),
                   MakePointerChecker<SpectrumChannel> ())
    .AddAttribute ("Phy", "The PHY layer attached to this device.",
                   PointerValue (),
                   MakePointerAccessor (&LoRaWANNetDevice::GetPhy,
                                        &LoRaWANNetDevice::SetPhy),
                   MakePointerChecker<LoRaWANPhy> ())
    .AddAttribute ("Mac", "The MAC layer attached to this device.",
                   PointerValue (),
                   MakePointerAccessor (&LoRaWANNetDevice::GetMac,
                                        &LoRaWANNetDevice::SetMac),
                   MakePointerChecker<LoRaWANMac> ())
    .AddAttribute("NbRep", "The number of repetitions for each UNC uplink message",
                   UintegerValue (1), // default value is one
                   MakeUintegerAccessor (&LoRaWANNetDevice::m_nbRep),
                   MakeUintegerChecker<uint8_t> (1, 15))
  ;
  return tid;
}

LoRaWANNetDevice::LoRaWANNetDevice () : m_deviceType (LORAWAN_DT_END_DEVICE_CLASS_A), m_configComplete(false)
{}

LoRaWANNetDevice::LoRaWANNetDevice (LoRaWANDeviceType deviceType)
  : m_deviceType (deviceType), m_configComplete (false)
{
  NS_LOG_FUNCTION (this);

  if (deviceType == LORAWAN_DT_END_DEVICE_CLASS_A) {
    uint8_t index = 0;
    m_phy = CreateObject<LoRaWANPhy> (index);
    m_mac = CreateObject<LoRaWANMac> (index);
    m_macRDC = CreateObject<LoRaWANMac::LoRaWANMacRDC> ();
  } else if (deviceType == LORAWAN_DT_GATEWAY) {
    uint8_t index = 0;
    for (uint8_t i = 0; i < LoRaWAN::m_supportedChannels.size (); i++) {
      for (uint8_t j = 0; j < LoRaWAN::m_supportedDataRates.size (); j++) {
        index = i*LoRaWAN::m_supportedDataRates.size () + j;
        Ptr<LoRaWANPhy> phy = CreateObject<LoRaWANPhy> (index);
        Ptr<LoRaWANMac> mac = CreateObject<LoRaWANMac> (index);
        // index in std::vector is i*LoRaWAN::m_supportedDataRates.size () + j
        // phy and mac belong together
        m_phys.push_back (phy);
        m_macs.push_back (mac);
      }
    }
    m_macRDC = CreateObject<LoRaWANMac::LoRaWANMacRDC> ();
  }
  CompleteConfig ();
}

LoRaWANNetDevice::~LoRaWANNetDevice ()
{
  NS_LOG_FUNCTION (this);
}


void
LoRaWANNetDevice::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  if (m_deviceType == LORAWAN_DT_END_DEVICE_CLASS_A) {
    m_mac->Dispose ();
    m_phy->Dispose ();
    m_phy = 0;
    m_mac = 0;
  } else if (m_deviceType == LORAWAN_DT_GATEWAY) {
    for (uint8_t i = 0; i < m_phys.size(); i++) {
      m_phys[i]->Dispose();
      m_macs[i]->Dispose();
    }

    m_phys.clear ();
    m_macs.clear ();
  }
  m_macRDC = 0;
  m_node = 0;
  // chain up.
  NetDevice::DoDispose ();
}

void
LoRaWANNetDevice::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
  if (m_deviceType == LORAWAN_DT_END_DEVICE_CLASS_A) {
    m_phy->Initialize ();
    m_mac->Initialize ();
  } else if (m_deviceType == LORAWAN_DT_GATEWAY) {
    for (uint8_t i = 0; i < m_phys.size(); i++) {
      m_phys[i]->Initialize();
      m_macs[i]->Initialize();
    }
  }
  NetDevice::DoInitialize ();
}


void
LoRaWANNetDevice::CompleteConfig (void)
{
  // TODO: this function could share more code the CLASS_A and GW cases
  NS_LOG_FUNCTION (this);

  if (m_deviceType == LORAWAN_DT_END_DEVICE_CLASS_A) {
    if (m_mac == 0
        || m_macRDC == 0
        || m_phy == 0
        || m_node == 0
        || m_configComplete)
      {
        return;
      }

    m_mac->SetPhy (m_phy);
    m_mac->SetRDC (m_macRDC);
    m_mac->SetDeviceType (m_deviceType);

    m_mac->SetDataIndicationCallback (MakeCallback (&LoRaWANNetDevice::DataIndication, this));

    Ptr<MobilityModel> mobility = m_node->GetObject<MobilityModel> ();
    if (!mobility)
      {
        NS_LOG_WARN ("LoRaWANNetDevice: no Mobility found on the node, probably it's not a good idea.");
      }
    m_phy->SetMobility (mobility);
    Ptr<LoRaWANErrorModel> model = CreateObject<LoRaWANErrorModel> ();
    m_phy->SetErrorModel (model);
    m_phy->SetDevice (this);

    m_phy->SetPdDataIndicationCallback (MakeCallback (&LoRaWANMac::PdDataIndication, m_mac));
    m_phy->SetPdDataDestroyedCallback (MakeCallback (&LoRaWANMac::PdDataDestroyed, m_mac));
    m_phy->SetPdDataConfirmCallback (MakeCallback (&LoRaWANMac::PdDataConfirm, m_mac));
    m_phy->SetSetTRXStateConfirmCallback (MakeCallback (&LoRaWANMac::SetTRXStateConfirm, m_mac));

    m_configComplete = true;
  } else if (m_deviceType == LORAWAN_DT_GATEWAY) {
    if (m_macs.size () == 0
        || m_macRDC == 0
        || m_phys.size () == 0
        || m_node == 0
        || m_configComplete)
      {
        return;
      }
    for (uint8_t i = 0; i < m_macs.size (); i++) {
      Ptr<LoRaWANPhy> phy = m_phys[i];
      Ptr<LoRaWANMac> mac = m_macs[i];
      NS_ASSERT(phy);
      NS_ASSERT(mac);

      // Phy: set channel and data rate for listining (using SetTxConf):
      uint8_t channelIndex = i / LoRaWAN::m_supportedDataRates.size();
      uint8_t dataRateIndex = i % LoRaWAN::m_supportedDataRates.size();
      if (!phy->SetTxConf (2, channelIndex, dataRateIndex, 3, 8, false, true) ) {
        NS_LOG_ERROR (this << " Phy #" << static_cast<uint16_t>(i) << ": failed setting channelIndex to " << static_cast<uint16_t>(channelIndex) << " and dataRateIndex to " << static_cast<uint16_t>(dataRateIndex));
      }

      mac->SetPhy (phy);
      mac->SetRDC (m_macRDC);
      mac->SetDeviceType (m_deviceType);
      mac->SetDataIndicationCallback (MakeCallback (&LoRaWANNetDevice::DataIndication, this));

      // Set begin and end tx callbacks (only for gateway)
      mac->SetBeginTxCallback (MakeCallback (&LoRaWANNetDevice::MacBeginsTx, this));
      mac->SetEndTxCallback (MakeCallback (&LoRaWANNetDevice::MacEndsTx, this));

      Ptr<MobilityModel> mobility = m_node->GetObject<MobilityModel> ();
      if (!mobility)
        {
          NS_LOG_WARN ("LoRaWANNetDevice: no Mobility found on the node, probably it's not a good idea.");
        }
      phy->SetMobility (mobility);
      Ptr<LoRaWANErrorModel> model = CreateObject<LoRaWANErrorModel> ();
      phy->SetErrorModel (model);
      phy->SetDevice (this);

      phy->SetPdDataIndicationCallback (MakeCallback (&LoRaWANMac::PdDataIndication, mac));
      phy->SetPdDataDestroyedCallback (MakeCallback (&LoRaWANMac::PdDataDestroyed,  mac));
      phy->SetPdDataConfirmCallback (MakeCallback (&LoRaWANMac::PdDataConfirm, mac));
      phy->SetSetTRXStateConfirmCallback (MakeCallback (&LoRaWANMac::SetTRXStateConfirm, mac));
    }
    m_configComplete = true;
  }
}

void
LoRaWANNetDevice::SetMac (Ptr<LoRaWANMac> mac)
{
  NS_LOG_FUNCTION (this);
  if (m_deviceType == LORAWAN_DT_END_DEVICE_CLASS_A) {
    m_mac = mac;
    CompleteConfig ();
  } else {
    NS_ASSERT_MSG (0, "Not implemented for non Class A end devices");
  }
}

void
LoRaWANNetDevice::SetPhy (Ptr<LoRaWANPhy> phy)
{
  NS_LOG_FUNCTION (this);
  if (m_deviceType == LORAWAN_DT_END_DEVICE_CLASS_A) {
    m_phy = phy;
    CompleteConfig ();
  } else {
    NS_ASSERT_MSG (0, "Not implemented for non Class A end devices");
  }
}

void
LoRaWANNetDevice::SetChannel (Ptr<SpectrumChannel> channel)
{
  NS_LOG_FUNCTION (this << channel);
  if (m_deviceType == LORAWAN_DT_END_DEVICE_CLASS_A) {
    m_phy->SetChannel (channel);
    channel->AddRx (m_phy);
  } else if (m_deviceType == LORAWAN_DT_GATEWAY) {
    for (uint8_t i = 0; i < m_phys.size (); i++) {
      Ptr<LoRaWANPhy> phy = m_phys[i];
      phy->SetChannel (channel);
      channel->AddRx (phy);
    }
  } else {
    NS_ASSERT_MSG (0, "Not implemented for non Class A end devices");
  }
  CompleteConfig ();
}

Ptr<LoRaWANMac>
LoRaWANNetDevice::GetMac (void) const
{
   NS_LOG_FUNCTION (this);
  if (m_deviceType == LORAWAN_DT_END_DEVICE_CLASS_A) {
    return m_mac;
  } else {
    NS_ASSERT_MSG (0, "Not implemented for non Class A end devices");
    return NULL;
  }
}

std::vector<Ptr<LoRaWANMac> >
LoRaWANNetDevice::GetMacs (void) const
{
   NS_LOG_FUNCTION (this);
  if (m_deviceType == LORAWAN_DT_GATEWAY) {
    return m_macs;
  } else {
    NS_ASSERT_MSG (0, "Not implemented for non-gateway devices");
    return m_macs;
  }
}

Ptr<LoRaWANPhy>
LoRaWANNetDevice::GetPhy (void) const
{
  NS_LOG_FUNCTION (this);
  if (m_deviceType == LORAWAN_DT_END_DEVICE_CLASS_A) {
    return m_phy;
  } else {
    NS_ASSERT_MSG (0, "Not implemented for non Class A end devices");
    return NULL;
  }
}

std::vector<Ptr<LoRaWANPhy> >
LoRaWANNetDevice::GetPhys (void) const
{
  NS_LOG_FUNCTION (this);
  if (m_deviceType == LORAWAN_DT_GATEWAY) {
    return m_phys;
  } else {
    NS_ASSERT_MSG (0, "Not implemented for non-gateway devices");
    return m_phys;
  }
}
void
LoRaWANNetDevice::SetIfIndex (const uint32_t index)
{
  NS_LOG_FUNCTION (this << index);
  m_ifIndex = index;
}

uint32_t
LoRaWANNetDevice::GetIfIndex (void) const
{
  NS_LOG_FUNCTION (this);
  return m_ifIndex;
}

Ptr<Channel>
LoRaWANNetDevice::GetChannel (void) const
{
  NS_LOG_FUNCTION (this);
  if (m_deviceType == LORAWAN_DT_END_DEVICE_CLASS_A) {
    return m_phy->GetChannel ();
  } else if (m_deviceType == LORAWAN_DT_GATEWAY) {
    return m_phys[0]->GetChannel (); // assume all phys are on same Channel
  } else {
    NS_ASSERT_MSG (0, "Not implemented");
    return NULL;
  }
}

void
LoRaWANNetDevice::LinkUp (void)
{
  NS_LOG_FUNCTION (this);
  m_linkUp = true;
  m_linkChanges ();
}

void
LoRaWANNetDevice::LinkDown (void)
{
  NS_LOG_FUNCTION (this);
  m_linkUp = false;
  m_linkChanges ();
}

Ptr<SpectrumChannel>
LoRaWANNetDevice::DoGetChannel (void) const
{
  NS_LOG_FUNCTION (this);
  if (m_deviceType == LORAWAN_DT_END_DEVICE_CLASS_A) {
    return m_phy->GetChannel ();
  } else if (m_deviceType == LORAWAN_DT_GATEWAY) {
    return m_phys[0]->GetChannel (); // assume all phys are on same Channel
  } else {
    NS_ASSERT_MSG (0, "Not implemented");
    return NULL;
  }
}

void
LoRaWANNetDevice::SetAddress (Address address)
{
  NS_LOG_FUNCTION (this);
  if (m_deviceType == LORAWAN_DT_END_DEVICE_CLASS_A) {
    // LoRaWANMac uses ns3::Ipv4Address to store the 32-bit LoRaWAN Network addresses
    m_mac->SetDevAddr (Ipv4Address::ConvertFrom (address));
  } else {
    NS_ASSERT_MSG (0, "Only Class A end devices have a network address");
  }
}

Address
LoRaWANNetDevice::GetAddress (void) const
{
  NS_LOG_FUNCTION (this);
  if (m_deviceType == LORAWAN_DT_END_DEVICE_CLASS_A) {
    return m_mac->GetDevAddr ();
  } else if (m_deviceType == LORAWAN_DT_GATEWAY) {
    return Ipv4Address(0xffffffff); // gateways don't really have addresses, but ns3 expects most net devices to have an adresses (TODO: is this true?) so we allocated the all ones address for all gateways ...
  } else {
    NS_ASSERT_MSG (0, "Unsupported LoRaWAN device type");
    return Ipv4Address ();
  }
}

bool
LoRaWANNetDevice::SetMtu (const uint16_t mtu)
{
  NS_ABORT_MSG ("Unsupported");
  return false;
}

uint16_t
LoRaWANNetDevice::GetMtu (void) const
{
  NS_LOG_FUNCTION (this);
  // TODO: Depends on the spreading factor:
  switch(m_mtuSpreadingFactor) {
  case LORAWAN_SF6:
  case LORAWAN_SF7:
  case LORAWAN_SF8:
	  return 222;
  case LORAWAN_SF9:
	  return 115;
  default:
  case LORAWAN_SF10:
  case LORAWAN_SF11:
  case LORAWAN_SF12:
	  return 51;

  }
}

bool
LoRaWANNetDevice::IsLinkUp (void) const
{
  NS_LOG_FUNCTION (this);
  return m_phy != 0 && m_linkUp;
}

void
LoRaWANNetDevice::AddLinkChangeCallback (Callback<void> callback)
{
  NS_LOG_FUNCTION (this);
  m_linkChanges.ConnectWithoutContext (callback);
}

bool
LoRaWANNetDevice::IsBroadcast (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}

Address
LoRaWANNetDevice::GetBroadcast (void) const
{
  NS_ABORT_MSG ("Unsupported");
  return Address ();
}

bool
LoRaWANNetDevice::IsMulticast (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}

Address
LoRaWANNetDevice::GetMulticast (Ipv4Address multicastGroup) const
{
  NS_ABORT_MSG ("Unsupported");
  return Address ();
}

Address
LoRaWANNetDevice::GetMulticast (Ipv6Address addr) const
{
  NS_LOG_FUNCTION (this);
  NS_ABORT_MSG ("Unsupported");
  return Address ();
}

bool
LoRaWANNetDevice::IsBridge (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}

bool
LoRaWANNetDevice::IsPointToPoint (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}

LoRaWANDeviceType
LoRaWANNetDevice::GetDeviceType (void) const
{
  return m_deviceType;
}

//void
//LoRaWANNetDevice::SetDeviceType (LoRaWANDeviceType type)
//{
//  m_deviceType = type;
//
//  // LoRaWANMac uses ns3::Ipv4Address to store the 32-bit LoRaWAN Network addresses
//  m_mac->SetDeviceType (type);
//}

bool
LoRaWANNetDevice::Send (Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber)
{
  // This method basically assumes an 802.3-compliant device, but a raw
  // lorawan device does not have an ethertype, and requires specific
  // LoRaWANDataRequestParams parameters.
  NS_LOG_FUNCTION (this << packet << dest << protocolNumber);

  if (packet->GetSize () > GetMtu ())
    {
      NS_LOG_ERROR ("Fragmentation is needed for this packet, drop the packet ");
      return false;
    }

  LoRaWANPhyParamsTag phyParamsTag;
  uint8_t channelIndex = 0;
  uint8_t dataRateIndex = 0;
  uint8_t codeRate = 0;
  if (packet->RemovePacketTag (phyParamsTag)) {
    channelIndex = phyParamsTag.GetChannelIndex ();
    dataRateIndex = phyParamsTag.GetDataRateIndex ();
    codeRate = phyParamsTag.GetCodeRate ();
  }

  LoRaWANMsgType msgType = LORAWAN_UNCONFIRMED_DATA_UP;
  LoRaWANMsgTypeTag msgTypeTag;
  if (packet->RemovePacketTag (msgTypeTag)) {
    msgType = msgTypeTag.GetMsgType ();
  }

  LoRaWANDataRequestParams loRaWANDataRequestParams;
  loRaWANDataRequestParams.m_msgType = msgType;
  loRaWANDataRequestParams.m_loraWANChannelIndex = channelIndex;
  loRaWANDataRequestParams.m_loraWANDataRateIndex = dataRateIndex;
  loRaWANDataRequestParams.m_loraWANCodeRate = codeRate;
  loRaWANDataRequestParams.m_requestHandle = 0; // TODO
  loRaWANDataRequestParams.m_numberOfTransmissions = 1;
  if (msgType == LORAWAN_CONFIRMED_DATA_UP) // note that for LORAWAN_CONFIRMED_DATA_DOWN the network server will call ::Send for every retransmission, for end devices ::Send is only called once (app retransmissions vs mac retransmissions)
    loRaWANDataRequestParams.m_numberOfTransmissions = DEFAULT_NUMBER_US_TRANSMISSIONS;
  else if (msgType == LORAWAN_UNCONFIRMED_DATA_UP)
    loRaWANDataRequestParams.m_numberOfTransmissions = m_nbRep;


  if (m_deviceType == LORAWAN_DT_END_DEVICE_CLASS_A) {
    m_mac->sendMACPayloadRequest (loRaWANDataRequestParams, packet);
    return true;
  } else if (m_deviceType == LORAWAN_DT_GATEWAY) {
    // select appropiate MAC/Phy based on channel and data rate of TX
    uint8_t macIndex = 0;
    if (getMACSIndexForChannelAndDataRate (macIndex, channelIndex, dataRateIndex)) {
      if (macIndex >= 0 && macIndex < this->m_macs.size ()) {
        this->m_macs[macIndex]->sendMACPayloadRequest (loRaWANDataRequestParams, packet);
        return true;
      } else {
        NS_LOG_ERROR (this << " Requested channel/datarate is not supported on gateway: channelIndex = " << channelIndex << ", dataRateIndex = " << dataRateIndex);
        return false;
      }
    } else {
      NS_LOG_ERROR (this << " Requested channel/datarate is not supported on gateway: channelIndex = " << channelIndex << ", dataRateIndex = " << dataRateIndex);
      return false;
    }
  } else {
    NS_LOG_ERROR (this << " Unsupported device type");
    return false;
  }
}

bool
LoRaWANNetDevice::getMACSIndexForChannelAndDataRate (uint8_t& macsIndex, uint8_t channelIndex, uint8_t dataRateIndex)
{
  if (channelIndex >= LoRaWAN::m_supportedChannels.size() || dataRateIndex >= LoRaWAN::m_supportedDataRates.size())
    return false;

  macsIndex = channelIndex*LoRaWAN::m_supportedDataRates.size() + dataRateIndex;
  return true;
}

void
LoRaWANNetDevice::MacBeginsTx (Ptr<LoRaWANMac> macPtr)
{
  NS_ASSERT (m_deviceType == LORAWAN_DT_GATEWAY);

  // Switch all MACs (including macPtr) to MAC_UNAVAILABLE state
  for (uint8_t i = 0; i < m_macs.size (); i++) {
    Ptr<LoRaWANMac> mac = m_macs[i];
    NS_ASSERT(mac);

    // include macPtr, as we need to switch the PHY corresponding to macPtr OFF first, before we can switch it to TX_ON
    // if (mac == macPtr)
    //   continue;

    mac->SwitchToUnavailableState();
  }
}

void
LoRaWANNetDevice::MacEndsTx (Ptr<LoRaWANMac> macPtr)
{
  NS_ASSERT (m_deviceType == LORAWAN_DT_GATEWAY);

  // Switch all MACs and Phys except macPtr to MAC_IDLE state
  for (uint8_t i = 0; i < m_macs.size (); i++) {
    Ptr<LoRaWANMac> mac = m_macs[i];
    NS_ASSERT(mac);

    if (mac == macPtr)
      continue;

    mac->SwitchToIdleState ();
  }
}

bool
LoRaWANNetDevice::CanSendImmediatelyOnChannel (uint8_t channelIndex, uint8_t dataRateIndex)
{
  if (this->m_macRDC) {
    int8_t subBandIndex = this->m_macRDC->GetSubBandIndexForChannelIndex (channelIndex);
    NS_ASSERT (subBandIndex >= 0);
    // step 1: check RDC restrictions
    if (this->m_macRDC->IsSubBandAvailable (subBandIndex)) {
      uint8_t macIndex = 0;
      if (getMACSIndexForChannelAndDataRate (macIndex, channelIndex, dataRateIndex)) {
        // step2: check whether MAC object is in Idle state (could be in TX or unavailable)
        if (this->m_macs[macIndex]->GetLoRaWANMacState () == MAC_IDLE) {
          // step3: check whether a MAC event is scheduled (MAC state could be scheduled to go to TX state)
          if (!this->m_macs[macIndex]->IsLoRaWANMacStateRunning ()) {
            return true;
          }
        }
      }
    }
  } else {
    NS_LOG_ERROR (this << " m_macRDC is not set");
    return false;
  }

  return false;
}

bool
LoRaWANNetDevice::SendFrom (Ptr<Packet> packet, const Address& source, const Address& dest, uint16_t protocolNumber)
{
  NS_ABORT_MSG ("Unsupported");
  return false;
}

Ptr<Node>
LoRaWANNetDevice::GetNode (void) const
{
  NS_LOG_FUNCTION (this);
  return m_node;
}

void
LoRaWANNetDevice::SetNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION (this);
  m_node = node;
  CompleteConfig ();
}

bool
LoRaWANNetDevice::NeedsArp (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}

void
LoRaWANNetDevice::SetReceiveCallback (ReceiveCallback cb)
{
  NS_LOG_FUNCTION (this);
  m_receiveCallback = cb;
}

void
LoRaWANNetDevice::DataIndication (LoRaWANDataIndicationParams params, Ptr<Packet> pkt)
{
  NS_LOG_FUNCTION (this);

  // Add LoRaWANPhyParamsTag to packet
  LoRaWANPhyParamsTag phyParamsTag;
  phyParamsTag.SetChannelIndex (params.m_channelIndex);
  phyParamsTag.SetDataRateIndex (params.m_dataRateIndex);
  phyParamsTag.SetCodeRate (params.m_codeRate);
  pkt->AddPacketTag (phyParamsTag);

  // Add MsgTypeTag to packet
  LoRaWANMsgTypeTag msgTypeTag;
  msgTypeTag.SetMsgType (params.m_msgType);
  pkt->AddPacketTag (msgTypeTag);

  Address senderAddress(params.m_endDeviceAddress);

  m_receiveCallback (this, pkt, 0, senderAddress);
}

void
LoRaWANNetDevice::SetPromiscReceiveCallback (PromiscReceiveCallback cb)
{
  // This method basically assumes an 802.3-compliant device, but a raw
  // lorawan device does not have an ethertype, and requires specific
  // LoRaWANDataRequestParams parameters.
  NS_LOG_WARN ("Unsupported; use LoRaWAN MAC APIs instead");
}

bool
LoRaWANNetDevice::SupportsSendFrom (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return false;
}

int64_t
LoRaWANNetDevice::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (stream);
  int64_t streamIndex = stream;
  if (m_deviceType == LORAWAN_DT_END_DEVICE_CLASS_A) {
    streamIndex += m_phy->AssignStreams (stream);
  } else if (m_deviceType == LORAWAN_DT_GATEWAY) {
    for (uint8_t i = 0; i < m_phys.size (); i++) {
      Ptr<LoRaWANPhy> phy = m_phys[i];
      streamIndex += phy->AssignStreams (stream + i);
    }
  } else {
    NS_ASSERT_MSG (0, "Not implemented for non Class A end devices");
  }
  NS_LOG_DEBUG ("Number of assigned RV streams:  " << (streamIndex - stream));
  return (streamIndex - stream);
}

} // namespace ns3
