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
#include "lorawan-lqi-tag.h"
#include <ns3/integer.h>

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (LoRaWANLqiTag);

TypeId
LoRaWANLqiTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LoRaWANLqiTag")
    .SetParent<Tag> ()
    .SetGroupName ("LoRaWAN")
    .AddConstructor<LoRaWANLqiTag> ()
    .AddAttribute ("Lqi", "The lqi of the last packet received",
                   IntegerValue (0),
                   MakeIntegerAccessor (&LoRaWANLqiTag::Get),
                   MakeIntegerChecker<uint8_t> ())
  ;
  return tid;
}

TypeId
LoRaWANLqiTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

LoRaWANLqiTag::LoRaWANLqiTag (void)
  : m_lqi (0)
{
}

LoRaWANLqiTag::LoRaWANLqiTag (uint8_t lqi)
  : m_lqi (lqi)
{
}

uint32_t
LoRaWANLqiTag::GetSerializedSize (void) const
{
  return sizeof (uint8_t);
}

void
LoRaWANLqiTag::Serialize (TagBuffer i) const
{
  i.WriteU8 (m_lqi);
}

void
LoRaWANLqiTag::Deserialize (TagBuffer i)
{
  m_lqi = i.ReadU8 ();
}

void
LoRaWANLqiTag::Print (std::ostream &os) const
{
  os << "Lqi = " << m_lqi;
}

void
LoRaWANLqiTag::Set (uint8_t lqi)
{
  m_lqi = lqi;
}

uint8_t
LoRaWANLqiTag::Get (void) const
{
  return m_lqi;
}

}
