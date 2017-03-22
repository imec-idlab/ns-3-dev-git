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
#include "lorawan-spectrum-signal-parameters.h"
#include <ns3/log.h>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LoRaWANSpectrumSignalParameters");

LoRaWANSpectrumSignalParameters::LoRaWANSpectrumSignalParameters (void)
{
  NS_LOG_FUNCTION (this);
}

LoRaWANSpectrumSignalParameters::LoRaWANSpectrumSignalParameters (const LoRaWANSpectrumSignalParameters& p)
  : SpectrumSignalParameters (p)
{
  NS_LOG_FUNCTION (this << &p);
  packet = p.packet;
  channelIndex = p.channelIndex;
  dataRateIndex = p.dataRateIndex;
  codeRate = p.codeRate;
}

Ptr<SpectrumSignalParameters>
LoRaWANSpectrumSignalParameters::Copy (void)
{
  NS_LOG_FUNCTION (this);
  return Create<LoRaWANSpectrumSignalParameters> (*this);
}

} // namespace ns3
