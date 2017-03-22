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
#ifndef LORAWAN_MAC_HEADER_H
#define LORAWAN_MAC_HEADER_H

#include "lorawan.h"
#include <ns3/header.h>

namespace ns3 {

/**
 * \ingroup lorawan
 * Represent the Mac Header in LoRaWAN
 */
class LoRaWANMacHeader : public Header
{

public:

  LoRaWANMacHeader (void);

  /**
   * Constructor
   * \param msgType LoRaWAN MAC message type
   * \param major LoRaWAN major version
   */
  LoRaWANMacHeader (LoRaWANMsgType msgType,
                   uint8_t major);

  ~LoRaWANMacHeader (void);

  LoRaWANMsgType getLoRaWANMsgType (void) const;
  void setLoRaWANMsgType(LoRaWANMsgType);

  uint8_t getMajor (void) const;
  void setMajor (uint8_t major);

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  void Print (std::ostream &os) const;
  uint32_t GetSerializedSize (void) const;
  void Serialize (Buffer::Iterator start) const;
  uint32_t Deserialize (Buffer::Iterator start);

  bool IsConfirmed() const;
  bool IsDownstream() const;
  bool IsUpstream() const;

private:
  LoRaWANMsgType m_msgType;
  uint8_t m_major;
}; //LoRaWANMacHeader

}; // namespace ns-3

#endif /* LORAWAN_MAC_HEADER_H */
