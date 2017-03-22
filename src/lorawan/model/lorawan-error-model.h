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
#ifndef LORAWAN_ERROR_MODEL_H
#define LORAWAN_ERROR_MODEL_H

#include "lorawan.h"
#include <ns3/object.h>

namespace ns3 {

/**
 * \ingroup lorawan
 *
 * Model the error rate for LoRaWAN based on LoRa baseband simulations in an AWGN channel:
 * y(n) = x(n) + sigma*(N(0,1)+i*N(0,1)) where
 * y(n) is the AWGN channel output
 * x(n) is the complex LoRa baseband symbol
 * sigma = sqrt(N0/2) = sqrt(Es/SNR/2) with Es=1 (normalized symbols) and SNR=power(10, (SNR_dB/10))
 * N(0,1) is a random scalar drawn from the standard normal distribution
 *
 * Curves were fitted for the base 10 logarithmic of the Bit Error Rate (BER)
 * for different LoRa spreading factors and coding rates.
 * For the curve fit the input BER was limited to maximum 0.1244 (rounded up)
 * as this BER results in a PDR of 1E-6 for a 13B LoRaWAN packet
 *
 * For all SFs and CRs tested the best fit was achieved for a curve which consists of one exponential ('Exp1' matlab fit):
 * log10(BER) = a*exp(b*x) where x is the SNR in dB
 * Exponents of these curves are stored as class members of LoRaWANErrorModel
 *
 * Note that spreading factors 7, 8, 9, 10, 11 and 12 and CR=1 and CR=3 were
 * part of the baseband simulation.
 */
#define LORAWAN_ERROR_MODEL_NR_COEFF 2*6

class LoRaWANErrorModel : public Object
{
public:
  /**
   * Get the type ID.
   *
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  LoRaWANErrorModel (void);

  /**
   * Return chunk success rate for given SNR.
   *
   * \return success rate (i.e. 1 - chunk error rate)
   * \param snr_db SNR expressed expressed in dB
   * \param nbits number of bits in the chunk
   */
  double GetChunkSuccessRate (double snr_db, uint32_t nbits, uint32_t bandwidth, LoRaSpreadingFactor spreadingFactor, uint8_t codeRate) const;

  double getBER(double snr_db, uint32_t bandwidth, LoRaSpreadingFactor spreadingFactor, uint8_t codeRate) const;

  /**
   * Return the SNR value (in dB) below which the BER is higher or equal to 0.1244
   * A receiver won't even attempt packet reception when the SINR is below this SNR cutoff
   *
   * \return SNR cutoff in dB
   */
  double getSNRCutoffForRX (uint32_t bandwidth, LoRaSpreadingFactor spreadingFactor, uint8_t codeRate) const;
private:
  /**
   * Array of precalculated curve fitting coefficients.
   */
  double m_aCoefficients[LORAWAN_ERROR_MODEL_NR_COEFF];
  double m_bCoefficients[LORAWAN_ERROR_MODEL_NR_COEFF];
};


} // namespace ns3

#endif /* LORAWAN_ERROR_MODEL_H */
