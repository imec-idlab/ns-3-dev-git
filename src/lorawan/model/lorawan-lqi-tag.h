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
 *
 *  Note this packet tag class is an exact copy of the LrWpanLqiTag class
 */
#ifndef LORWAN_LQI_TAG_H
#define LORWAN_LQI_TAG_H

#include <ns3/tag.h>

namespace ns3 {

class LoRaWANLqiTag : public Tag
{
public:
  /**
   * Get the type ID.
   *
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  virtual TypeId GetInstanceTypeId (void) const;

  /**
   * Create a LoRaWANLqiTag with the default LQI 0.
   */
  LoRaWANLqiTag (void);

  /**
   * Create a LoRaWANLqiTag with the given LQI value.
   */
  LoRaWANLqiTag (uint8_t lqi);

  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual void Print (std::ostream &os) const;

  /**
   * Set the LQI to the given value.
   *
   * \param lqi the value of the LQI to set
   */
  void Set (uint8_t lqi);

  /**
   * Get the LQI value.
   *
   * \return the LQI value
   */
  uint8_t Get (void) const;
private:
  /**
   * The current LQI value of the tag.
   */
  uint8_t m_lqi;
};


}
#endif /* LORWAN_LQI_TAG_H */
