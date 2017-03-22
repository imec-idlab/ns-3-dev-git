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
#include <ns3/log.h>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LoRaWAN");

/* ... */

const std::vector<LoRaWANChannel> LoRaWAN::m_supportedChannels = {
  {0, 868100000, 125000, 1},
  {1, 868300000, 125000, 1},
  {2, 868500000, 125000, 1},
  {3, 867100000, 125000, 1},
  {4, 867300000, 125000, 1},
  {5, 867500000, 125000, 1},
  {6, 867700000, 125000, 1},
  // {7, 867900000, 125000, 1}, // sacrifice for high power channel on 869525000
  {7, 869525000, 125000, 3}, // NOTE: always keep this special high power channel as the last element in the m_supportedChannels vector
};

const std::vector<LoRaWANDataRate> LoRaWAN::m_supportedDataRates = {
  {0, LORAWAN_SF12, 125000},
  {1, LORAWAN_SF11, 125000},
  {2, LORAWAN_SF10, 125000},
  {3, LORAWAN_SF9, 125000},
  {4, LORAWAN_SF8, 125000},
  {5, LORAWAN_SF7, 125000},
  {6, LORAWAN_SF7, 250000}
}; // other indexes are RFU

uint8_t LoRaWAN::m_RW2ChannelIndex = LoRaWAN::m_supportedChannels.size () - 1; // high power channel, assume this is last channel in m_supportedChannels
uint8_t LoRaWAN::m_RW2DataRateIndex = 0; // lowest spreading factor

uint8_t
LoRaWAN::GetRX1DataRateIndex (uint8_t upstreamDRIndex, uint8_t rx1DROffset)
{
  if (rx1DROffset == 0 || rx1DROffset == 1 ||rx1DROffset == 2 ||rx1DROffset == 3 ||rx1DROffset == 4 ||rx1DROffset == 5)
    if (upstreamDRIndex <= rx1DROffset)
      return 0;
    else
      return upstreamDRIndex - rx1DROffset;
  else {
    NS_LOG_WARN ("LoRaWAN::GetRX1DataRateIndex Invalid rx1DROffset: " << static_cast<uint16_t>(rx1DROffset));
    return upstreamDRIndex;
  }
}
/****************************************************************************
 ************************ LoRaWANMsgTypeTag *********************************
 ****************************************************************************/

LoRaWANMsgTypeTag::LoRaWANMsgTypeTag () {}

void
LoRaWANMsgTypeTag::SetMsgType (LoRaWANMsgType type)
{
  m_msgType = type;
}

LoRaWANMsgType
LoRaWANMsgTypeTag::GetMsgType (void) const
{
  return m_msgType;
}

TypeId
LoRaWANMsgTypeTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LoRaWANMsgTypeTag")
    .SetParent<Tag> ()
    .SetGroupName("LoRaWAN")
    .AddConstructor<LoRaWANMsgTypeTag> ()
    ;
  return tid;
}

TypeId
LoRaWANMsgTypeTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
LoRaWANMsgTypeTag::GetSerializedSize (void) const
{
  return sizeof (uint8_t);
}

void
LoRaWANMsgTypeTag::Serialize (TagBuffer i) const
{
  i.WriteU8 (m_msgType);
}

void
LoRaWANMsgTypeTag::Deserialize (TagBuffer i)
{
  m_msgType = static_cast<LoRaWANMsgType>(i.ReadU8());
}

void
LoRaWANMsgTypeTag::Print (std::ostream &os) const
{
  os << "LORWAN_MAC_MSG_TYPE = " << m_msgType;
}

/****************************************************************************
 *********************** LoRaWANPhyParamsTag ************************************
 ****************************************************************************/

LoRaWANPhyParamsTag::LoRaWANPhyParamsTag () {}

void
LoRaWANPhyParamsTag::SetChannelIndex (uint8_t index)
{
  m_channelIndex = index;
}

uint8_t
LoRaWANPhyParamsTag::GetChannelIndex (void) const
{
  return m_channelIndex;
}

void
LoRaWANPhyParamsTag::SetDataRateIndex (uint8_t index)
{
  m_dataRateIndex = index;
}

uint8_t
LoRaWANPhyParamsTag::GetDataRateIndex (void) const
{
  return m_dataRateIndex;
}

void
LoRaWANPhyParamsTag::SetCodeRate (uint8_t codeRate)
{
  m_codeRate = codeRate;
}

uint8_t
LoRaWANPhyParamsTag::GetCodeRate (void) const
{
  return m_codeRate;
}

TypeId
LoRaWANPhyParamsTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LoRaWANPhyParamsTag")
    .SetParent<Tag> ()
    .SetGroupName("LoRaWAN")
    .AddConstructor<LoRaWANPhyParamsTag> ()
    ;
  return tid;
}

TypeId
LoRaWANPhyParamsTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
LoRaWANPhyParamsTag::GetSerializedSize (void) const
{
  return 3 * sizeof (uint8_t);
}

void
LoRaWANPhyParamsTag::Serialize (TagBuffer i) const
{
  i.WriteU8 (m_channelIndex);
  i.WriteU8 (m_dataRateIndex);
  i.WriteU8 (m_codeRate);
}

void
LoRaWANPhyParamsTag::Deserialize (TagBuffer i)
{
  m_channelIndex = i.ReadU8();
  m_dataRateIndex = i.ReadU8();
  m_codeRate = i.ReadU8();
}

void
LoRaWANPhyParamsTag::Print (std::ostream &os) const
{
  os << "LORWAN_PHY_RX_PARMS: channelIndex = " << m_channelIndex << ", dataRateIndex = " << m_dataRateIndex << ", codeRate = " << m_codeRate;
}

uint64_t LoRaWANCounterSingleton::m_counter = -1; // highest possible 64 bit number: 0xffffffffffffffff

//LoRaWANCounterSingleton*
//LoRaWANCounterSingleton::GetPtr ()
//{
//  if (!LoRaWANCounterSingleton::m_ptr)
//    LoRaWANCounterSingleton::m_ptr = new LoRaWANCounterSingleton ();
//
//  return LoRaWANCounterSingleton::m_ptr;
//}
} // namespace ns3
