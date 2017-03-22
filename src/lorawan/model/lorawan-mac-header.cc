/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
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
#include "lorawan-mac-header.h"
#include "lorawan-mac.h"
#include <ns3/address-utils.h>

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (LoRaWANMacHeader);

LoRaWANMacHeader::LoRaWANMacHeader ()
{
  setLoRaWANMsgType (LoRaWANMsgType::LORAWAN_UNCONFIRMED_DATA_UP);
  setMajor (0);
}

LoRaWANMacHeader::LoRaWANMacHeader (LoRaWANMsgType mType,
                                  uint8_t major)
{
  setLoRaWANMsgType (mType);
  setMajor (major);
}

LoRaWANMacHeader::~LoRaWANMacHeader ()
{
}

LoRaWANMsgType
LoRaWANMacHeader::getLoRaWANMsgType (void) const
{
  return m_msgType;
}

void
LoRaWANMacHeader::setLoRaWANMsgType (LoRaWANMsgType type)
{
  m_msgType = type;
}

uint8_t
LoRaWANMacHeader::getMajor (void) const
{
  return m_major;
}

void
LoRaWANMacHeader::setMajor (uint8_t major)
{
  m_major = major;
}

TypeId
LoRaWANMacHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LoRaWANMacHeader")
    .SetParent<Header> ()
    .SetGroupName ("LoRaWAN")
    .AddConstructor<LoRaWANMacHeader> ();
  return tid;
}

TypeId
LoRaWANMacHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
LoRaWANMacHeader::Print (std::ostream &os) const
{
  os << "  Message Type = " << (uint32_t) m_msgType << ", Major = " << (uint32_t) m_major;
}

uint32_t
LoRaWANMacHeader::GetSerializedSize (void) const
{
  /*
   * Each mac header will have 1 octet
   */

  return 1;
}

void
LoRaWANMacHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;

  uint8_t mhdr = 0; // mhdr: msg type (3 bits), RFU (3 bits) and Major version (2 bits)
  mhdr |= (m_msgType & 0x07) << 5;
  mhdr |= (m_major & 0x3);

  i.WriteU8 (mhdr);
}

uint32_t
LoRaWANMacHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;

  uint8_t mhdr = i.ReadU8();
  LoRaWANMsgType msgType = static_cast<LoRaWANMsgType>((mhdr >> 5) & 0x07);

  setLoRaWANMsgType (msgType);
  setMajor ((uint8_t)mhdr & 0x03);

  return 1;
}

bool
LoRaWANMacHeader::IsConfirmed () const
{
  return (m_msgType == LORAWAN_CONFIRMED_DATA_UP || m_msgType == LORAWAN_CONFIRMED_DATA_DOWN);
}

bool
LoRaWANMacHeader::IsDownstream() const
{
  return (m_msgType == LORAWAN_UNCONFIRMED_DATA_DOWN || m_msgType == LORAWAN_CONFIRMED_DATA_DOWN);
}

bool
LoRaWANMacHeader::IsUpstream() const
{
  return (m_msgType == LORAWAN_CONFIRMED_DATA_UP || m_msgType == LORAWAN_UNCONFIRMED_DATA_UP);
}

// ----------------------------------------------------------------------------------------------------------

} //namespace ns3
