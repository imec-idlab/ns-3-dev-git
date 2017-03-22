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
#ifndef LORAWAN_SPECTRUM_SIGNAL_PARAMETERS_H
#define LORAWAN_SPECTRUM_SIGNAL_PARAMETERS_H

#include <ns3/packet.h>
#include <ns3/spectrum-signal-parameters.h>
#include "ns3/lorawan.h"

namespace ns3 {

/**
 * \ingroup lorawan
 *
 * Signal parameters for LoRaWAN.
 */
struct LoRaWANSpectrumSignalParameters : public SpectrumSignalParameters
{

  // inherited from SpectrumSignalParameters
  virtual Ptr<SpectrumSignalParameters> Copy (void);

  /**
   * default constructor
   */
  LoRaWANSpectrumSignalParameters (void);

  /**
   * copy constructor
   */
  LoRaWANSpectrumSignalParameters (const LoRaWANSpectrumSignalParameters& p);

  /**
   * The packet being transmitted with this signal
   */
  Ptr<Packet> packet;

  /**
   * The index of the channel of the transmission
   */
  uint8_t channelIndex;

  /**
   * The index of the data rate (i.e. the spreading factor and bandwidth) of the transmission
   */
  uint8_t dataRateIndex;

  /**
   * The code rate of the transmission
   */
  uint8_t codeRate;
};

}  // namespace ns3

#endif /* LORAWA_SPECTRUM_SIGNAL_PARAMETERS_H*/
