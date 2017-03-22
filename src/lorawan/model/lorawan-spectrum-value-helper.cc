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
#include "lorawan-spectrum-value-helper.h"
#include "lorawan.h"
#include <ns3/log.h>
#include <ns3/spectrum-value.h>

#include <cmath>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LoRaWANSpectrumValueHelper");

Ptr<SpectrumModel> g_LoRaWANSpectrumModel; //!< Global object that stores the LoRaWAN Spectrum Model

/**
 * \ingroup lorawan
 * \brief Helper class used to automatically initialize the LoRaWAN Spectrum Model objects
 */
class LoRaWANSpectrumModelInitializer
{
public:
  LoRaWANSpectrumModelInitializer(void)
  {
    NS_LOG_FUNCTION (this);

    Bands bands;
    // The three default 125kHz data channels available in EU863-870:
    // 868.10, 868.30 & 868.50MHz
    for (int i = 0; i < 3; i++)
      {
        BandInfo bi;
        bi.fc = 868.10e6 + i * 200.0e3;
        bi.fl = bi.fc - 62.5e3; // 62.5 kHz is half of 125 kHz bandwidth
        bi.fh = bi.fc + 62.5e3;
        bands.push_back (bi);
      }

    // Four 125kHz channels available in EU863-870:
    // 867.10, 867.30 & 867.50 & 867.70MHz
    for (int i = 0; i < 4; i++)
      {
        BandInfo bi;
        bi.fc = 867.10e6 + i * 200.0e3;
        bi.fl = bi.fc - 62.5e3; // 62.5 kHz is half of 125 kHz bandwidth
        bi.fh = bi.fc + 62.5e3;
        bands.push_back (bi);
      }

    // One 125kHz high power (27dBm) channel on 869.525MHz
    {
        BandInfo bi;
        bi.fc = 869.525e6;
        bi.fl = bi.fc - 62.5e3; // 62.5 kHz is half of 125 kHz bandwidth
        bi.fh = bi.fc + 62.5e3;
        bands.push_back (bi);
    }
    g_LoRaWANSpectrumModel = Create<SpectrumModel> (bands);
  }

} g_LoRaWANSpectrumModelInitializerInstance; //!< Global object used to initialize the LoRaWAN Spectrum Model

/* ... */
LoRaWANSpectrumValueHelper::LoRaWANSpectrumValueHelper(void)
{
  NS_LOG_FUNCTION (this);
  m_noiseFactor = 1.0;
}

LoRaWANSpectrumValueHelper::~LoRaWANSpectrumValueHelper(void)
{
  NS_LOG_FUNCTION (this);
}

Ptr<SpectrumValue>
LoRaWANSpectrumValueHelper::CreateTxPowerSpectralDensity (double txPower, uint32_t freq)
{
  // TODO: how do we model this for LoRa? The actual center frequency shifts
  // during a symbol period in LoRa, so the power density also shifts during
  // a symbol transmission
  // Should make a simplification: e.g. power density is constant for a 125kHz
  // region and then we have some roll-off nearby... Didn't really find
  // anything in semtech docs, TODO: measure
  // For now: assume constant SPD over 125kHz bandwidth of channel and assume
  // all signal power is concentrated in the channel

  NS_LOG_FUNCTION (this);
  Ptr<SpectrumValue> txPsd = Create <SpectrumValue> (g_LoRaWANSpectrumModel);

  // txPower is expressed in dBm. We must convert it into natural unit (W).
  txPower = pow (10.0, (txPower - 30) / 10);

  double txPowerDensity = txPower / 125e3;

  (*txPsd)[LoRaWANSpectrumValueHelper::GetPsdIndexForCenterFrequency(freq)] = txPowerDensity;

  return txPsd;
}

Ptr<SpectrumValue>
LoRaWANSpectrumValueHelper::CreateNoisePowerSpectralDensity (uint32_t freq)
{
  // TODO
  NS_LOG_FUNCTION (this);
  Ptr<SpectrumValue> noisePsd = Create <SpectrumValue> (g_LoRaWANSpectrumModel);

  static const double BOLTZMANN = 1.3803e-23;
  // Nt  is the power of thermal noise in W
  double Nt = BOLTZMANN * 290.0;
  // noise Floor (W) which accounts for thermal noise and non-idealities of the receiver
  double noisePowerDensity = m_noiseFactor * Nt;

  (*noisePsd)[LoRaWANSpectrumValueHelper::GetPsdIndexForCenterFrequency(freq)] = noisePowerDensity;

  return noisePsd;
}

uint32_t
LoRaWANSpectrumValueHelper::GetPsdIndexForCenterFrequency(uint32_t freq)
{
  NS_LOG_FUNCTION ("LoRaWANSpectrumValueHelper::GetPsdIndexForCenterFrequency");

  for (uint8_t i = 0; i < LoRaWAN::m_supportedChannels.size (); i++) {
    if (freq == LoRaWAN::m_supportedChannels[i].m_fc) {
      return i;
    }
  }

  NS_LOG_ERROR ("LoRaWANSpectrumValueHelper::GetPsdIndexForCenterFrequency: " << "Invalid center frequency:" << freq);
  NS_ASSERT(0);

  return LoRaWAN::m_supportedChannels.size ();
}

double
LoRaWANSpectrumValueHelper::TotalAvgPower (Ptr<const SpectrumValue> psd, uint32_t freq)
{
  NS_LOG_FUNCTION (psd);
  double totalAvgPower = 0.0;

  NS_ASSERT (psd->GetSpectrumModel () == g_LoRaWANSpectrumModel);

  // numerically integrate to get area under psd using 125kHz resolution

  totalAvgPower += (*psd)[LoRaWANSpectrumValueHelper::GetPsdIndexForCenterFrequency(freq)];
  totalAvgPower *= 125e3;

  return totalAvgPower;
}

} // namespace ns3
