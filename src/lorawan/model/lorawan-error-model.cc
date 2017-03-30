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
#include "lorawan-error-model.h"
#include <ns3/log.h>

#include <cmath>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LoRaWANErrorModel");

NS_OBJECT_ENSURE_REGISTERED (LoRaWANErrorModel);

TypeId
LoRaWANErrorModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LoRaWANErrorModel")
    .SetParent<Object> ()
    .SetGroupName ("LoRaWAN")
    .AddConstructor<LoRaWANErrorModel> ()
  ;
  return tid;
}

LoRaWANErrorModel::LoRaWANErrorModel (void)
{
  /**
  * Coefficients from exp1_log_model_curvefit_truncated_output.txt
  */

  // SF7, CR1
  m_aCoefficients[0] = -30.25798896;
  m_bCoefficients[0] = 0.28570229;

  // SF7, CR3
  m_aCoefficients[1] = -105.19660816;
  m_bCoefficients[1] = 0.37455655;

  // SF8, CR1
  m_aCoefficients[2] =-77.10020378;
  m_bCoefficients[2] = 0.29933678;

  // SF8, CR3
  m_aCoefficients[3] = -289.81333927;
  m_bCoefficients[3] = 0.37560498;

  // SF9, CR1
  m_aCoefficients[4] = -244.64237268;
  m_bCoefficients[4] = 0.32227064;

  // SF9, CR3
  m_aCoefficients[5] = -1114.33115567;
  m_bCoefficients[5] = 0.39694465;

  // SF10, CR1
  m_aCoefficients[6] = -725.95557882;
  m_bCoefficients[6] = 0.33393115;

  // SF10, CR3
  m_aCoefficients[7] = -4285.44400727;
  m_bCoefficients[7] = 0.41164155;

  // SF11, CR1
  m_aCoefficients[8] = -2109.80642246;
  m_bCoefficients[8] = 0.34073142;

  // SF11, CR3
  m_aCoefficients[9] = -20771.69446082;
  m_bCoefficients[9] = 0.43318930;

  // SF12, CR1
  m_aCoefficients[10] = -4452.36530463;
  m_bCoefficients[10] = 0.33174696;

  // SF12, CR3
  m_aCoefficients[11] = -98658.11656301;
  m_bCoefficients[11] = 0.44852713;
}

double
LoRaWANErrorModel::getBER (double snr_db, uint32_t bandWidth, LoRaSpreadingFactor spreadingFactor, uint8_t codeRate) const
{
  NS_ASSERT( bandWidth == 125e3 );
  NS_ASSERT( spreadingFactor == LORAWAN_SF7 || spreadingFactor == LORAWAN_SF8 || spreadingFactor == LORAWAN_SF9 || spreadingFactor == LORAWAN_SF10 || spreadingFactor == LORAWAN_SF11 || spreadingFactor == LORAWAN_SF12);
  NS_ASSERT( codeRate == 1 || codeRate == 3 );
  // Note only bandwidth 125kHz is supported

  double snr_db_rounded = snr_db;

  // We checked the BER curves between snr_min and 0dB (where snr_min depends
  // on the SF) and all curves are monotonically decreasing functions between
  // these bounds (for increasing SNR_DB)
  if (spreadingFactor == LORAWAN_SF7 || spreadingFactor == LORAWAN_SF8 || spreadingFactor == LORAWAN_SF9 || spreadingFactor == LORAWAN_SF10) {
    if (snr_db < -20)
      snr_db_rounded = -20;
    if (snr_db > 0)
      snr_db_rounded = 0;
  } else if (spreadingFactor == LORAWAN_SF11) {
    if (snr_db < -23)
      snr_db_rounded = -23;
    if (snr_db > 0)
      snr_db_rounded = 0;
  } else if (spreadingFactor == LORAWAN_SF12) {
    if (snr_db < -26)
      snr_db_rounded = -26;
    if (snr_db > 0)
      snr_db_rounded = 0;
  }

  // Get index for coeffients in arrays:
  uint8_t coefIndex = (spreadingFactor-7)*2;
  if (codeRate == 3)
    coefIndex+=1;

  if (coefIndex >= LORAWAN_ERROR_MODEL_NR_COEFF) {
    NS_FATAL_ERROR (this << "invalid coef index");
  }

  // log10(BER) = a*exp(b*x) + c*exp(d*x) where x is the SNR in dB
  double log_ber = m_aCoefficients[coefIndex]*exp (m_bCoefficients[coefIndex]*snr_db_rounded);
  double ber = pow (10.0, log_ber);

  NS_LOG_LOGIC (this << " snr_db = " << snr_db << ", snr_db_rounded = " << snr_db_rounded << ", log_ber = " << log_ber << ", ber = " << ber);

  return ber;
}

double
LoRaWANErrorModel::GetChunkSuccessRate (double snr_db, uint32_t nbits, uint32_t bandWidth, LoRaSpreadingFactor spreadingFactor, uint8_t codeRate) const
{
  NS_ASSERT( bandWidth == 125e3 );
  NS_ASSERT( spreadingFactor == LORAWAN_SF7 || spreadingFactor == LORAWAN_SF8 || spreadingFactor == LORAWAN_SF9 || spreadingFactor == LORAWAN_SF10 || spreadingFactor == LORAWAN_SF11 || spreadingFactor == LORAWAN_SF12);
  NS_ASSERT( codeRate == 1 || codeRate == 3 );

  double ber = getBER (snr_db, bandWidth, spreadingFactor, codeRate);

  if (ber > 1.0)
    NS_FATAL_ERROR (this << "BER is great than 1.0");

  ber = std::min (ber, 1.0);
  double retval = pow (1.0 - ber, nbits);

  NS_LOG_LOGIC (this << " snr_db = " << snr_db << ", nbits = " << nbits << ", spreadingFactor = " << static_cast<uint32_t>(spreadingFactor) << ", codeRate = " << static_cast<uint32_t>(codeRate) << ". ChunkSuccesRate = " << retval);

  return retval;
}

double
LoRaWANErrorModel::getSNRCutoffForRX (uint32_t bandWidth, LoRaSpreadingFactor spreadingFactor, uint8_t codeRate) const
{
  NS_ASSERT( bandWidth == 125e3);
  NS_ASSERT( spreadingFactor == LORAWAN_SF7 || spreadingFactor == LORAWAN_SF8 || spreadingFactor == LORAWAN_SF9 || spreadingFactor == LORAWAN_SF10 || spreadingFactor == LORAWAN_SF11 || spreadingFactor == LORAWAN_SF12);
  NS_ASSERT( codeRate == 1 || codeRate == 3);

  if (spreadingFactor == LORAWAN_SF7) {
    if (codeRate == 1) {
      return -12.2833;
    } else if (codeRate == 3) {
      return -12.6982;
    }
  } else if (spreadingFactor == LORAWAN_SF8) {
    if (codeRate == 1) {
      return -14.8485;
    } else if (codeRate == 3) {
      return -15.3588;
    }
  } else if (spreadingFactor == LORAWAN_SF9) {
    if (codeRate == 1) {
      return -17.3749;
    } else if (codeRate == 3) {
      return -17.9260;
    }
  } else if (spreadingFactor == LORAWAN_SF10) {
    if (codeRate == 1) {
      return -20.0254;
    } else if (codeRate == 3) {
      return -20.5581;
    }
  } else if (spreadingFactor == LORAWAN_SF11) {
    if (codeRate == 1) {
      return -22.7568;
    } else if (codeRate == 3) {
      return -23.1791;
    }
  } else if (spreadingFactor == LORAWAN_SF12) {
    if (codeRate == 1) {
      return -25.6243;
    } else if (codeRate == 3) {
      return -25.8602;
    }
  }

  NS_FATAL_ERROR (this << "Unsupported SF/CR parameters for SNR Cut off");
  return 0;
}
} // namespace ns3
