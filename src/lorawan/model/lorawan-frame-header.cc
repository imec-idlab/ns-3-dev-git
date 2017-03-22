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
#include "lorawan-frame-header.h"
#include "lorawan-mac.h"
#include <ns3/log.h>
#include <ns3/address-utils.h>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LoRaWANFrameHeader");

LoRaWANFrameHeader::LoRaWANFrameHeader () : m_devAddr((uint32_t)0), m_frameControl(0), m_frameCounter(0), m_serializeFramePort(false)
{
}

LoRaWANFrameHeader::LoRaWANFrameHeader (Ipv4Address devAddr, bool adr, bool adrAckReq, bool ack, bool framePending, uint8_t FOptsLen, uint16_t frameCounter, uint16_t framePort) : m_devAddr(devAddr), m_frameCounter(frameCounter), m_framePort(framePort)
{
  // adr, adrackreq and fopts are not implemented for now
  m_frameControl = 0;
  setAck(ack);
  setFramePending(framePending);

  if (framePort > 0)
    m_serializeFramePort = true; // note that the caller should set m_serializeFramePort to true in case of framePort == 0
}

LoRaWANFrameHeader::~LoRaWANFrameHeader ()
{
}

Ipv4Address
LoRaWANFrameHeader::getDevAddr (void) const
{
  return m_devAddr;
}

void
LoRaWANFrameHeader::setDevAddr (Ipv4Address addr)
{
  m_devAddr = addr;
}

bool
LoRaWANFrameHeader::getAck () const
{
  return (m_frameControl & LORAWAN_FHDR_ACK_MASK);
}

void
LoRaWANFrameHeader::setAck (bool ack)
{
  if (ack)
    m_frameControl |= LORAWAN_FHDR_ACK_MASK;
  else
    m_frameControl &= ~LORAWAN_FHDR_ACK_MASK;
}

bool
LoRaWANFrameHeader::getFramePending () const
{
  return (m_frameControl & LORAWAN_FHDR_FPENDING_MASK);
}

void
LoRaWANFrameHeader::setFramePending (bool framePending)
{
  if (framePending)
    m_frameControl |= LORAWAN_FHDR_FPENDING_MASK;
  else
    m_frameControl &= ~LORAWAN_FHDR_FPENDING_MASK;
}

uint16_t
LoRaWANFrameHeader::getFrameCounter () const
{
  return m_frameCounter;
}

void
LoRaWANFrameHeader::setFrameCounter (uint16_t frameCounter)
{
  m_frameCounter = frameCounter;
}

uint8_t
LoRaWANFrameHeader::getFramePort () const
{
  return m_framePort;
}

void
LoRaWANFrameHeader::setFramePort (uint8_t framePort)
{
  setSerializeFramePort(true);
  m_framePort = framePort;
}

bool
LoRaWANFrameHeader::getSerializeFramePort () const
{
  return m_serializeFramePort;
}

void
LoRaWANFrameHeader::setSerializeFramePort (bool serializeFramePort)
{
  m_serializeFramePort = serializeFramePort;
}

TypeId
LoRaWANFrameHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LoRaWANFrameHeader")
    .SetParent<Header> ()
    .SetGroupName ("LoRaWAN")
    .AddConstructor<LoRaWANFrameHeader> ();
  return tid;
}

TypeId
LoRaWANFrameHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
LoRaWANFrameHeader::Print (std::ostream &os) const
{
  os << "Device Address = " << std::hex << m_devAddr  << ", frameControl = " << (uint16_t)m_frameControl << ", Frame Counter = " << std::dec << (uint32_t) m_frameCounter;

  if (m_serializeFramePort)
    os << ", Frame Port = " << (uint32_t)m_framePort;
}

uint32_t
LoRaWANFrameHeader::GetSerializedSize (void) const
{
  /*
   * Each frame header will consist of 7 bytes plus any frame options (optional) plus a frame port (optional)
   */

  uint8_t frameOptionsLength = m_frameControl & LORAWAN_FHDR_FOPTSLEN_MASK;
  uint8_t framePortLength = m_serializeFramePort == true ? 1 : 0;

  return 7 + frameOptionsLength + framePortLength;
}

void
LoRaWANFrameHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU32 (m_devAddr.Get ());
  i.WriteU8 (m_frameControl);
  i.WriteU16 (m_frameCounter);
  // TODO: frame options ...
  if (m_serializeFramePort)
    i.WriteU8 (m_framePort);
}

uint32_t
LoRaWANFrameHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  uint32_t nBytes = 0;

  // Device address
  nBytes += 4;
  m_devAddr.Set (i.ReadU32());

  // Frame control field
  nBytes += 1;
  uint8_t frameControl = i.ReadU8();
  m_frameControl = 0;
  if (frameControl & LORAWAN_FHDR_ACK_MASK) {
    setAck(true);
  }
  if (frameControl & LORAWAN_FHDR_FPENDING_MASK) { // this is only valid for downstream frames ...
    setFramePending(true);
  }
  if (frameControl & LORAWAN_FHDR_FOPTSLEN_MASK) {
    NS_LOG_WARN(this << " Frame Options not supported in current LoRaWAN implementation");
  }

  // Frame counter
  nBytes += 2;
  uint16_t frameCounter = i.ReadU16();
  setFrameCounter(frameCounter);

  // TODO: frame options ...

  // The header does not indicate whether Frame Port is present, instead it
  // should be present if there is any Frame Payload. The caller should set
  // m_serializeFramePort to true if there is a frame port.
  if (m_serializeFramePort) {
    nBytes += 1;
    uint8_t framePort = i.ReadU8();
    m_framePort = framePort;
  }

  return nBytes;
}

bool
LoRaWANFrameHeader::IsAck () const
{
  return getAck();
}

bool
LoRaWANFrameHeader::IsFramePending () const
{
  return getFramePending();
}
// ----------------------------------------------------------------------------------------------------------

} //namespace ns3
