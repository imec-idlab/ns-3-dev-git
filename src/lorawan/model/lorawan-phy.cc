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
#include "lorawan-phy.h"
#include "lorawan-spectrum-signal-parameters.h"
#include "lorawan-spectrum-value-helper.h"
#include "lorawan-error-model.h"
#include "lorawan-lqi-tag.h"
#include <ns3/log.h>
#include <ns3/abort.h>
#include <ns3/simulator.h>
#include <ns3/spectrum-value.h>
#include <ns3/antenna-model.h>
#include <ns3/mobility-model.h>
#include <ns3/spectrum-channel.h>
#include <ns3/packet.h>
#include <ns3/net-device.h>
#include <ns3/random-variable-stream.h>
#include <ns3/double.h>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LoRaWANPhy");

NS_OBJECT_ENSURE_REGISTERED (LoRaWANPhy);

//const m_maxPhyPayloadSize[8] = {64, 64, 64, 128, 235, 235, 235, 235};

TypeId
LoRaWANPhy::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LoRaWANPhy")
    .SetParent<SpectrumPhy> ()
    .SetGroupName ("LoRaWAN")
    .AddConstructor<LoRaWANPhy> ()
    .AddTraceSource ("TrxState",
                     "The state of the transceiver",
                     MakeTraceSourceAccessor (&LoRaWANPhy::m_trxState),
                     "ns3::TracedValueCallback::LoRaWANPhyEnumeration")
    //.AddTraceSource ("TrxState",
    //                 "The state of the transceiver",
    //                 MakeTraceSourceAccessor (&LoRaWANPhy::m_trxStateLogger),
    //                 "ns3::LoRaWANPhy::StateTracedCallback")
    .AddTraceSource ("PhyTxBegin",
                     "Trace source indicating a packet has "
                     "begun transmitting over the channel medium",
                     MakeTraceSourceAccessor (&LoRaWANPhy::m_phyTxBeginTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("PhyTxEnd",
                     "Trace source indicating a packet has been "
                     "completely transmitted over the channel.",
                     MakeTraceSourceAccessor (&LoRaWANPhy::m_phyTxEndTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("PhyTxDrop",
                     "Trace source indicating a packet has been "
                     "dropped by the device during transmission",
                     MakeTraceSourceAccessor (&LoRaWANPhy::m_phyTxDropTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("PhyRxBegin",
                     "Trace source indicating a packet has begun "
                     "being received from the channel medium by the device",
                     MakeTraceSourceAccessor (&LoRaWANPhy::m_phyRxBeginTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("PhyRxEnd",
                     "Trace source indicating a packet has been "
                     "completely received from the channel medium "
                     "by the device",
                     MakeTraceSourceAccessor (&LoRaWANPhy::m_phyRxEndTrace),
                     "ns3::Packet::SinrTracedCallback")
    .AddTraceSource ("PhyRxDrop",
                     "Trace source indicating a packet has been "
                     "dropped by the device during reception",
                     MakeTraceSourceAccessor (&LoRaWANPhy::m_phyRxDropTrace),
                     "ns3::TracedValueCallback::LoRaWANDroppedPacketTracedCallback")
  ;
  return tid;
}

LoRaWANPhy::LoRaWANPhy (void)
{
  LoRaWANPhy (0);
}

LoRaWANPhy::LoRaWANPhy (uint8_t index)
    : m_setTRXState (), m_index (index)
{
  NS_LOG_FUNCTION (this << index);

  ChangeTrxState (LORAWAN_PHY_TRX_OFF);
  //m_trxStatePending = LORAWAN_PHY_IDLE;

  // default PHY layer properties
  m_currentChannelIndex = 0;
  m_currentDataRateIndex  = 5; // SF7, 125KHz
  m_txPower = 2; // 2 dBm
  m_codeRate = 3;
  m_preambleLength = 8;
  m_crcOn = true;

  // receiver sensitivity depends on LoRa modulation parameters according to Semtech
  // However, we don't use sensitivity in our PHY modelling as we don't do any
  // energy detection or carrier sensing
  //m_rxSensitivity = pow (10.0, -106.58 / 10.0) / 1000.0;

  LoRaWANSpectrumValueHelper psdHelper;
  const uint32_t freq = LoRaWAN::m_supportedChannels [m_currentChannelIndex].m_fc;
  m_txPsd = psdHelper.CreateTxPowerSpectralDensity (m_txPower,
                                                    freq);
  m_noise = psdHelper.CreateNoisePowerSpectralDensity (freq);
  m_signal = Create<LoRaWANInterferenceHelper> (m_noise->GetSpectrumModel ());
  m_rxLastUpdate = Seconds (0);
  Ptr<Packet> none_packet = 0;
  Ptr<LoRaWANSpectrumSignalParameters> none_params = 0;
  m_currentRxPacket = std::make_pair (none_params, LoRaWANPhyRxStatus (true, false));
  m_currentTxPacket = std::make_pair (none_packet, true);
  m_errorModel = 0;

  m_random = CreateObject<UniformRandomVariable> ();
  m_random->SetAttribute ("Min", DoubleValue (0.0));
  m_random->SetAttribute ("Max", DoubleValue (1.0));


  ChangeTrxState (LORAWAN_PHY_TRX_OFF);
}

LoRaWANPhy::~LoRaWANPhy (void)
{
}

void
LoRaWANPhy::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  // Cancel pending transceiver state change, if one is in progress.
  m_setTRXState.Cancel ();
  m_trxState = LORAWAN_PHY_TRX_OFF;
  // m_trxStatePending = LORAWAN_PHY_IDLE;

  m_mobility = 0;
  m_device = 0;
  m_channel = 0;
  m_txPsd = 0;
  m_noise = 0;
  m_signal = 0;
  m_errorModel = 0;
  m_pdDataIndicationCallback = MakeNullCallback< void, uint32_t, Ptr<Packet>, uint8_t, uint8_t, uint8_t, uint8_t > ();
  m_pdDataConfirmCallback = MakeNullCallback< void, LoRaWANPhyEnumeration > ();
  m_setTRXStateConfirmCallback = MakeNullCallback< void, LoRaWANPhyEnumeration > ();

  SpectrumPhy::DoDispose ();
}

Ptr<NetDevice>
LoRaWANPhy::GetDevice (void) const
{
  NS_LOG_FUNCTION (this);
  return m_device;
}


Ptr<MobilityModel>
LoRaWANPhy::GetMobility (void)
{
  NS_LOG_FUNCTION (this);
  return m_mobility;
}


void
LoRaWANPhy::SetDevice (Ptr<NetDevice> d)
{
  NS_LOG_FUNCTION (this << d);
  m_device = d;
}


void
LoRaWANPhy::SetMobility (Ptr<MobilityModel> m)
{
  NS_LOG_FUNCTION (this << m);
  m_mobility = m;
}


void
LoRaWANPhy::SetChannel (Ptr<SpectrumChannel> c)
{
  NS_LOG_FUNCTION (this << c);
  m_channel = c;
}


Ptr<SpectrumChannel>
LoRaWANPhy::GetChannel (void)
{
  NS_LOG_FUNCTION (this);
  return m_channel;
}


Ptr<const SpectrumModel>
LoRaWANPhy::GetRxSpectrumModel (void) const
{
  NS_LOG_FUNCTION (this);
  if (m_txPsd)
    {
      return m_txPsd->GetSpectrumModel ();
    }
  else
    {
      return 0;
    }
}

Ptr<AntennaModel>
LoRaWANPhy::GetRxAntenna (void)
{
  NS_LOG_FUNCTION (this);
  return m_antenna;
}

void
LoRaWANPhy::SetAntenna (Ptr<AntennaModel> a)
{
  NS_LOG_FUNCTION (this << a);
  m_antenna = a;
}


void
LoRaWANPhy::SetErrorModel (Ptr<LoRaWANErrorModel> e)
{
  NS_LOG_FUNCTION (this << e);
  NS_ASSERT (e);
  m_errorModel = e;
}

Ptr<LoRaWANErrorModel>
LoRaWANPhy::GetErrorModel (void) const
{
  NS_LOG_FUNCTION (this);
  return m_errorModel;
}

uint8_t
LoRaWANPhy::GetIndex (void) const
{
  return m_index;
}

void
LoRaWANPhy::PrintCurrentTxConf () const
{
  NS_LOG_INFO(this
      << "(" << (double)m_txPower << ", "
      << (uint16_t)m_currentChannelIndex << ", "
      << (uint16_t)m_currentDataRateIndex << ", "
      << (uint16_t)m_codeRate << ", "
      << (uint16_t)m_preambleLength << ", "
      << "0, "
      << m_crcOn << ")");
}

bool
LoRaWANPhy::SetTxConf (int8_t power, uint8_t channelIndex, uint8_t dataRateIndex, uint8_t codeRate, uint8_t preambleLength, bool implicitHeader, bool crcOn)
{
  NS_LOG_FUNCTION (this << (int16_t)power << (uint16_t)channelIndex << (uint16_t)dataRateIndex << (uint16_t)codeRate << (uint16_t)preambleLength << implicitHeader << crcOn);

  PrintCurrentTxConf();

  const LoRaWANChannel* channel = &LoRaWAN::m_supportedChannels[channelIndex];
  const LoRaWANDataRate* dataRate = &LoRaWAN::m_supportedDataRates[dataRateIndex];
  if (channel == NULL || dataRate == NULL) {
    NS_LOG_ERROR(this << " Cannot set TX config due to invalid channel or data rate index");
    return false;
  }

  // Can only update TxConf when radio is not already transmitting
  if (m_trxState == LORAWAN_PHY_BUSY_TX || m_trxState == LORAWAN_PHY_TX_ON || m_setTRXState.IsRunning() ) {
    NS_LOG_ERROR(this << " Cannot set TX config while radio is in state " << m_trxState << ", or while the state is scheduled to be changed");
    return false;
  }

  // validate input:
  bool validConf = true;
  if (!(power == 27 || power == 20 || power == 14 || power == 11 || power == 8 || power == 5 || power == 2)) // as per LoRaWAN $7.1.3
    validConf = false;

  if (channel->m_bw != 125e3) // only 125KHz is supported for the moment
    validConf = false;

  if (codeRate != 1 && codeRate != 2 && codeRate != 3 && codeRate != 4)
    validConf = false;

  if (implicitHeader) // only explicit header is supported in LoRaWAN
    validConf = false;

  if (!validConf) {
    NS_LOG_ERROR(this << "Invalid TX config supplied, aborting.");
    return false;
  }

  m_txPower = power;
  // TODO: changing the channel should corrupt any ongoing packet reception/transmission
  m_currentChannelIndex = channelIndex;
  m_currentDataRateIndex = dataRateIndex;
  m_codeRate = codeRate;
  m_preambleLength = preambleLength;
  m_crcOn = crcOn;

  // update TX PSD
  LoRaWANSpectrumValueHelper psdHelper;
  m_txPsd = psdHelper.CreateTxPowerSpectralDensity (m_txPower,
                                                    channel->m_fc);

  NS_LOG_DEBUG (this << ": updated TxConf");

  return true;
}

void
LoRaWANPhy::SetTRXStateRequest (LoRaWANPhyEnumeration state)
{
  NS_LOG_FUNCTION (this << state);

  NS_LOG_LOGIC ("Trying to set m_trxState from " << m_trxState << " to " << state);

  // Check valid states
  NS_ABORT_IF ( (state != LORAWAN_PHY_RX_ON)
                && (state != LORAWAN_PHY_TRX_OFF)
                && (state != LORAWAN_PHY_FORCE_TRX_OFF)
                && (state != LORAWAN_PHY_IDLE)
                && (state != LORAWAN_PHY_TX_ON) );

  if (state == m_trxState)
    {
      if (!m_setTRXStateConfirmCallback.IsNull ())
        {
          m_setTRXStateConfirmCallback (state);
        }
      return;
    }

  if (state == LORAWAN_PHY_TRX_OFF ) {
    NS_ABORT_IF ( (m_trxState != LORAWAN_PHY_RX_ON ) );
    ChangeTrxState (LORAWAN_PHY_TRX_OFF);
    if (!m_setTRXStateConfirmCallback.IsNull ())
      {
        m_setTRXStateConfirmCallback (state);
      }

    return;

  }

  if (state == LORAWAN_PHY_IDLE) {
    NS_ABORT_IF ( (m_trxState != LORAWAN_PHY_IDLE)
                && (m_trxState != LORAWAN_PHY_BUSY_TX)
                && (m_trxState != LORAWAN_PHY_BUSY_RX)
                && (m_trxState != LORAWAN_PHY_RX_ON) );


    ChangeTrxState (LORAWAN_PHY_IDLE);
    if (!m_setTRXStateConfirmCallback.IsNull ())
      {
        m_setTRXStateConfirmCallback (state);
      }

    return;
  }

  if (state == LORAWAN_PHY_RX_ON) {
    NS_ABORT_IF ( (m_trxState != LORAWAN_PHY_TRX_OFF)
                && (m_trxState != LORAWAN_PHY_IDLE)
                && (m_trxState != LORAWAN_PHY_BUSY_TX)
                && (m_trxState != LORAWAN_PHY_BUSY_RX)
                && (m_trxState != LORAWAN_PHY_RX_ON) );

    ChangeTrxState (LORAWAN_PHY_RX_ON);
    if (!m_setTRXStateConfirmCallback.IsNull ())
      {
        m_setTRXStateConfirmCallback (state);
      }

      return;
  }


  if (state == LORAWAN_PHY_TX_ON) {
    NS_ABORT_IF ( (m_trxState != LORAWAN_PHY_TRX_OFF)
                && (m_trxState != LORAWAN_PHY_IDLE)
                && (m_trxState != LORAWAN_PHY_RX_ON) );

    ChangeTrxState (LORAWAN_PHY_TX_ON);
    // PdDataRequest will start the actual transmission
    if (!m_setTRXStateConfirmCallback.IsNull ())
      {
        m_setTRXStateConfirmCallback (state);
      }

    return;
  }

  if (state == LORAWAN_PHY_FORCE_TRX_OFF)
    {
      if (m_trxState == LORAWAN_PHY_TRX_OFF)
        {
          NS_LOG_DEBUG ("force TRX_OFF, was already off");
        }
      else
        {
          NS_LOG_DEBUG ("force TRX_OFF, SUCCESS");
          if (m_currentRxPacket.first)
            {   //terminate reception if needed
                //incomplete reception -- force packet discard
              NS_LOG_DEBUG ("force TRX_OFF, terminate reception");
              m_currentRxPacket.second.aborted = true;
            }
          if (m_trxState == LORAWAN_PHY_BUSY_TX)
            {
              NS_LOG_DEBUG ("force TRX_OFF, terminate transmission");
              m_currentTxPacket.second = true;
            }
          ChangeTrxState (LORAWAN_PHY_TRX_OFF);
        }
      if (!m_setTRXStateConfirmCallback.IsNull ())
      {
        m_setTRXStateConfirmCallback (LORAWAN_PHY_TRX_OFF);
      }
      return;
    }

  // TODO: remove?
  // - changing state while Phy is receiving or transmitting, use StatePending
  // - Forcing off the Phy, this should cancel any ongoing RX or TX
  if ( ((state == LORAWAN_PHY_RX_ON)
        || (state == LORAWAN_PHY_TRX_OFF))
       && (m_trxState == LORAWAN_PHY_BUSY_TX) )
    {
      NS_LOG_DEBUG ("Phy is busy; setting state pending to " << state);
      // m_trxStatePending = state;
      return;  // Send PlmeSetTRXStateConfirm later
    }

  NS_FATAL_ERROR ("Unexpected transition from state " << m_trxState << " to state " << state);
  return;
}

void
LoRaWANPhy::ChangeTrxState (LoRaWANPhyEnumeration newState)
{
  NS_LOG_LOGIC (this << " state: " << m_trxState << " -> " << newState);
  //m_trxStateLogger (Simulator::Now (), m_trxState, newState);
  m_trxState = newState;
}

bool
LoRaWANPhy::preambleDetected (void) const
{
  NS_LOG_FUNCTION (this);

  return (m_trxState == LORAWAN_PHY_BUSY_RX);
}

void
LoRaWANPhy::SetPdDataIndicationCallback (PdDataIndicationCallback c)
{
  NS_LOG_FUNCTION (this);
  m_pdDataIndicationCallback = c;
}

void
LoRaWANPhy::SetPdDataDestroyedCallback (PdDataDestroyedCallback c)
{
  NS_LOG_FUNCTION (this);
  m_pdDataDestroyedCallback = c;
}

void
LoRaWANPhy::SetPdDataConfirmCallback (PdDataConfirmCallback c)
{
  NS_LOG_FUNCTION (this);
  m_pdDataConfirmCallback = c;
}

void
LoRaWANPhy::SetSetTRXStateConfirmCallback (SetTRXStateConfirmCallback c)
{
  NS_LOG_FUNCTION (this);
  m_setTRXStateConfirmCallback = c;
}

void
LoRaWANPhy::StartRx (Ptr<SpectrumSignalParameters> spectrumRxParams)
{
  NS_LOG_FUNCTION (this << spectrumRxParams);

  LoRaWANSpectrumValueHelper psdHelper;

  Ptr<LoRaWANSpectrumSignalParameters> loraWanRxParams = DynamicCast<LoRaWANSpectrumSignalParameters> (spectrumRxParams);
  // If the channel of the transmission and the don't match, just return immediatly.
  // This is a workaround for a SpectrumPhy limitation where even in cases when the PSD of the incoming signalling has very very small power (-infinity in this case), we are still adding it as interference and calling EndRx (this clutters tracing output and wastes CPU time)
  // If the data rate of the transmission and the PHY don't match, do not attempt to receive the transmission; instead just count the transmission as noise
  // IRL the RX Phy would not lock onto a Preamble with different SF. In simulator we have to explicitly check the SF.
  bool channelMismatch = false;
  bool dataRateMismatch = false;
  if (loraWanRxParams) {
    channelMismatch = loraWanRxParams->channelIndex != m_currentChannelIndex;
    dataRateMismatch = loraWanRxParams->dataRateIndex != m_currentDataRateIndex;
  }

  if (channelMismatch) {
    return; // just do nothing
  }

  if (loraWanRxParams == 0 || dataRateMismatch)
    { // reception is not a LoRaWAN packet or is a LoRaWAN transmission with a different data rate
      CheckInterference ();
      m_signal->AddSignal (spectrumRxParams->psd);

      // Schedule EndRx to update m_signal when the transmission of the incoming signal has ended
      Simulator::Schedule (spectrumRxParams->duration, &LoRaWANPhy::EndRx, this, spectrumRxParams);
      return;
    }

  Ptr<Packet> p = loraWanRxParams->packet;
  NS_ASSERT (p != 0);

  // Prevent PHY from receiving another packet while switching the transceiver state.
  if (m_trxState == LORAWAN_PHY_RX_ON && !m_setTRXState.IsRunning ())
    {
      // The specification doesn't seem to refer to BUSY_RX, but vendor
      // data sheets suggest that this is a substate of the RX_ON state
      // that is entered after preamble detection when the digital receiver
      // is enabled.  Here, for now, we use BUSY_RX to mark the period between
      // StartRx() and EndRx() states.

      // We are going to BUSY_RX state when receiving the first bit of an SHR,
      // as opposed to real receivers, which should go to this state only after
      // successfully receiving the SHR.

      // If synchronizing to the packet is possible, change to BUSY_RX state,
      // otherwise drop the packet and stay in RX state. The actual synchronization
      // is not modeled.

      // Add any incoming packet to the current interference before checking the
      // SINR.
      const uint32_t freq = LoRaWAN::m_supportedChannels [m_currentChannelIndex].m_fc;
      const uint32_t bw = LoRaWAN::m_supportedChannels [m_currentChannelIndex].m_bw;
      const uint8_t transmissionDataRateIndex = loraWanRxParams->dataRateIndex;
      const uint8_t transmissionCodeRate = loraWanRxParams->codeRate;
      const LoRaSpreadingFactor sf = LoRaWAN::m_supportedDataRates [transmissionDataRateIndex].spreadingFactor;

      NS_LOG_DEBUG (this << " channel index = " << static_cast<uint16_t>(m_currentChannelIndex));
      NS_LOG_DEBUG (this << " receiving packet with power: " << 10 * log10 (LoRaWANSpectrumValueHelper::TotalAvgPower (loraWanRxParams->psd, freq)) + 30 << "dBm");

      m_signal->AddSignal (loraWanRxParams->psd);
      Ptr<SpectrumValue> interferenceAndNoise = m_signal->GetSignalPsd ();
      *interferenceAndNoise -= *loraWanRxParams->psd;
      *interferenceAndNoise += *m_noise;

      double sinr_db = 10.0 * log10 (LoRaWANSpectrumValueHelper::TotalAvgPower (loraWanRxParams->psd, freq) / LoRaWANSpectrumValueHelper::TotalAvgPower (interferenceAndNoise, freq));
      double sinr_cutoff_db = m_errorModel->getSNRCutoffForRX (bw, sf, transmissionCodeRate);

      // When the BER is higher than 0.1 do not even try and decode the packet
      // BER=0.1 is reached for a different SINR threshold depending on the spreading factor
      if (sinr_db > sinr_cutoff_db)
        {
          ChangeTrxState (LORAWAN_PHY_BUSY_RX);
          m_currentRxPacket = std::make_pair (loraWanRxParams, LoRaWANPhyRxStatus (false, false));
          m_phyRxBeginTrace (p);

          m_rxLastUpdate = Simulator::Now ();
        }
      else
        {
          m_phyRxDropTrace (p, LORAWAN_RX_DROP_SINR_TOO_LOW);
        }
    }
  else if (m_trxState == LORAWAN_PHY_BUSY_RX)
    {
      // Drop the new packet.
      NS_LOG_DEBUG (this << " packet collision");
      m_phyRxDropTrace (p, LORAWAN_RX_DROP_PHY_BUSY_RX);

      // Check if we correctly received the old packet up to now.
      CheckInterference ();

      // Add the incoming packet to the current interference after we have
      // checked for successfull reception of the current packet for the time
      // before the additional interference.
      m_signal->AddSignal (loraWanRxParams->psd);
    }
  else
    {
      // Simply drop the packet.
      NS_LOG_DEBUG (this << " transceiver not in RX state (state = " << m_trxState << ")");
      m_phyRxDropTrace (p, LORAWAN_RX_DROP_NOT_IN_RX_STATE);

      // Add the signal power to the interference, anyway.
      m_signal->AddSignal (loraWanRxParams->psd);
    }

  // Always call EndRx to update the interference.
  // \todo: Do we need to keep track of these events to unschedule them when disposing off the PHY?

  Simulator::Schedule (spectrumRxParams->duration, &LoRaWANPhy::EndRx, this, spectrumRxParams);
}

void
LoRaWANPhy::CheckInterference (void)
{
  // Calculate whether packet was lost.
  LoRaWANSpectrumValueHelper psdHelper;
  Ptr<LoRaWANSpectrumSignalParameters> currentRxParams = m_currentRxPacket.first;

  // We are currently receiving a packet.
  if (m_trxState == LORAWAN_PHY_BUSY_RX)
    {
      NS_ASSERT (currentRxParams); // && !m_currentRxPacket.second.destroyed);

      Ptr<Packet> currentPacket = currentRxParams->packet;
      if (m_errorModel != 0)
        {
          // How many bits did we receive since the last calculation?
          double t = (Simulator::Now () - m_rxLastUpdate).ToDouble (Time::MS);
          uint32_t chunkSize = ceil (t * (GetNominalDataRate () / 1000)); // divide by 1000, to get data rate per ms
          Ptr<SpectrumValue> interferenceAndNoise = m_signal->GetSignalPsd ();
          *interferenceAndNoise -= *currentRxParams->psd;
          *interferenceAndNoise += *m_noise;

          double sinr = LoRaWANSpectrumValueHelper::TotalAvgPower (currentRxParams->psd, LoRaWAN::m_supportedChannels [m_currentChannelIndex].m_fc) / LoRaWANSpectrumValueHelper::TotalAvgPower (interferenceAndNoise, LoRaWAN::m_supportedChannels [m_currentChannelIndex].m_fc);
          double sinr_db = 10.0*log10(sinr);

          const uint8_t transmissionDataRateIndex = currentRxParams->dataRateIndex;
          const uint8_t transmissionCodeRate = currentRxParams->codeRate;
          const LoRaSpreadingFactor sf = LoRaWAN::m_supportedDataRates [m_currentDataRateIndex].spreadingFactor;
          double per = 1.0 - m_errorModel->GetChunkSuccessRate (sinr_db, chunkSize, LoRaWAN::m_supportedDataRates [transmissionDataRateIndex].bandWith, sf, transmissionCodeRate);

          // The LQI is the total packet success rate scaled to 0-255.
          // If not already set, initialize to 255.
          LoRaWANLqiTag tag (std::numeric_limits<uint8_t>::max ());
          currentPacket->PeekPacketTag (tag);
          uint8_t lqi = tag.Get ();
          tag.Set (lqi - (per * lqi));
          currentPacket->ReplacePacketTag (tag);

          if (m_random->GetValue () < per)
            {
              // The packet was destroyed, drop the packet after reception.
              m_currentRxPacket.second.destroyed = true;
            }
        }
      else
        {
          NS_LOG_WARN ("Missing ErrorModel");
        }
    }
  m_rxLastUpdate = Simulator::Now ();
}

void
LoRaWANPhy::EndRx (Ptr<SpectrumSignalParameters> par)
{
  NS_LOG_FUNCTION (this);

  Ptr<LoRaWANSpectrumSignalParameters> params = DynamicCast<LoRaWANSpectrumSignalParameters> (par);

  Ptr<LoRaWANSpectrumSignalParameters> currentRxParams = m_currentRxPacket.first;
  if (currentRxParams == params)
    {
      CheckInterference ();
    }

  // Update the interference.
  m_signal->RemoveSignal (par->psd);

  // Check whether EndRx is called for the end of LoRaWAN TX with different data rate:
  bool dataRateMismatch = false;
  if (params)
    dataRateMismatch = params->dataRateIndex != m_currentDataRateIndex;

  if (params == 0 || dataRateMismatch)
    {
      NS_LOG_LOGIC ("Node: " << m_device->GetAddress() << " Removing interferent: " << *(par->psd));
      return;
    }

  // If this is the end of the currently received packet, check if reception was successful.
  if (currentRxParams == params)
    {
      Ptr<Packet> currentPacket = currentRxParams->packet;
      NS_ASSERT (currentPacket != 0);

      // If there is no error model attached to the PHY, we always report the maximum LQI value.
      LoRaWANLqiTag tag (std::numeric_limits<uint8_t>::max ());
      currentPacket->PeekPacketTag (tag);
      m_phyRxEndTrace (currentPacket, tag.Get ());

      if (!m_currentRxPacket.second.destroyed && !m_currentRxPacket.second.aborted)
        {
          // The packet was successfully received, push it up the stack.
          if (!m_pdDataIndicationCallback.IsNull ())
            {
              m_pdDataIndicationCallback (currentPacket->GetSize (), currentPacket, 0, m_currentChannelIndex, params->dataRateIndex, params->codeRate);
            }
        }
      else
        {
          // The packet was destroyed, drop it.
          if (m_currentRxPacket.second.destroyed) {
            m_phyRxDropTrace (currentPacket, LORAWAN_RX_DROP_PACKET_DESTOYED);
            m_pdDataDestroyedCallback ();
          } else if (m_currentRxPacket.second.aborted)
            m_phyRxDropTrace (currentPacket, LORAWAN_RX_DROP_PACKET_ABORTED);
          else
            NS_ASSERT (false);
        }
      Ptr<LoRaWANSpectrumSignalParameters> none = 0;
      m_currentRxPacket = std::make_pair (none, LoRaWANPhyRxStatus (true, false));

      // In case the ongoing reception was aborted by a transmission on this PHY,
      // then m_currentRxPacket.second will have been false but also the PHY state
      // will be BUSY_TX.
      // In case the ongoing reception was destroyed due to a BER, then
      // m_currentRxPacket.second will have been false but the PHY state will
      // be BUSY_RX
      // Note that m_currentRxPacket.second can not tell us which case is true,
      // so instead we need to look at the state of the PHY
      //
      // Only switch to RX_ON for cases where the Phy was receiving a
      // transmission (i.e. the BUSY_RX state). When the PHY switched to a
      // transmission during reception, don't change the state at all (this
      // will be after EndTx is called)
      // TODO: should we switch to IDLE here for end device PHYs?
      // Normally not, as MAC will switch PHY state according to state machine
      if (m_trxState == LORAWAN_PHY_BUSY_RX)
          ChangeTrxState (LORAWAN_PHY_RX_ON);
    }
}

void
LoRaWANPhy::PdDataRequest (const uint32_t phyPayloadLength, Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this);
  // TODO: Check length of payload
  //if (phyPayloadLength > m_maxPhyPayloadSize[m_spreadingFactor]) {
  //    if (!m_pdDataConfirmCallback.IsNull ())
  //      {
  //        m_pdDataConfirmCallback (LORAWAN_PHY_UNSPECIFIED);
  //      }
  //    NS_LOG_DEBUG ("Drop packet because Phy Payload is too long: " << phyPayloadLength);
  //    return;
  //}

  NS_LOG_DEBUG(this << " m_trxState = " << m_trxState);
  if (m_trxState == LORAWAN_PHY_TX_ON)
    {
      //send down
      NS_ASSERT (m_channel);

      // Tag the TX'd packet with a unique identifier that can be used for tracing:
      LoRaWANPhyTraceIdTag  traceIdTag;
      while (p->RemovePacketTag (traceIdTag)); // remove any tags that might have been present
      traceIdTag.SetFlowId (LoRaWANPhyTraceIdTag::AllocateFlowId ());
      p->AddPacketTag (traceIdTag);

      // Remove a possible LQI tag from a previous transmission of the packet.
      LoRaWANLqiTag lqiTag;
      p->RemovePacketTag (lqiTag);

      m_phyTxBeginTrace (p);
      m_currentTxPacket.first = p;
      m_currentTxPacket.second = false;

      Ptr<LoRaWANSpectrumSignalParameters> txParams = Create<LoRaWANSpectrumSignalParameters> ();
      txParams->duration = CalculateTxTime (p->GetSize());
      txParams->txPhy = GetObject<SpectrumPhy> ();
      txParams->psd = m_txPsd;
      txParams->txAntenna = m_antenna;
      txParams->packet = p;
      txParams->channelIndex = m_currentChannelIndex;
      txParams->dataRateIndex = m_currentDataRateIndex;
      txParams->codeRate = m_codeRate;

      m_channel->StartTx (txParams);
      m_pdDataRequest = Simulator::Schedule (txParams->duration, &LoRaWANPhy::EndTx, this);
      ChangeTrxState (LORAWAN_PHY_BUSY_TX);
      return;
    }
  else if ((m_trxState == LORAWAN_PHY_RX_ON)
            || (m_trxState == LORAWAN_PHY_TRX_OFF)
            || (m_trxState == LORAWAN_PHY_BUSY_TX ) )
    {
      if (!m_pdDataConfirmCallback.IsNull ())
        {
          m_pdDataConfirmCallback (LORAWAN_PHY_UNSPECIFIED);
        }
      // Drop packet, hit PhyTxDrop trace
      m_phyTxDropTrace (p);
      return;
    }
  else
    {
      NS_FATAL_ERROR ("This should be unreachable, or else state " << m_trxState << " should be added as a case");
    }
}

void
LoRaWANPhy::EndTx (void)
{
  NS_LOG_FUNCTION (this);

  // Either the transmission ended while we were busy transmitting or after the transceiver was forced to be switched off
  NS_ABORT_IF ( (m_trxState != LORAWAN_PHY_BUSY_TX) && (m_trxState != LORAWAN_PHY_TRX_OFF));

  if (m_currentTxPacket.second == false)
    {
      NS_LOG_DEBUG ("Packet successfully transmitted at " << Simulator::Now());

      m_phyTxEndTrace (m_currentTxPacket.first);
      if (!m_pdDataConfirmCallback.IsNull ())
        {
          m_pdDataConfirmCallback (LORAWAN_PHY_SUCCESS);
        }
    }
  else
    {
      NS_LOG_DEBUG ("Packet transmission aborted");
      m_phyTxDropTrace (m_currentTxPacket.first);
      if (!m_pdDataConfirmCallback.IsNull ())
        {
          // See if this is ever entered in another state
          NS_ASSERT (m_trxState ==  LORAWAN_PHY_TRX_OFF);
          m_pdDataConfirmCallback (m_trxState);
        }
    }
  m_currentTxPacket.first = 0;
  m_currentTxPacket.second = false;

  // TODO: switch PHY state?
}

/* \param p is the PHYPayload as per the LoRaWAN spec */
Time
LoRaWANPhy::CalculateTxTime (uint8_t payloadLength)
{
  // calculations per $4.1.1.7 'Time on air' in sx1272 data sheet
  const uint32_t bandwidth = LoRaWAN::m_supportedChannels [m_currentChannelIndex].m_bw;
  const LoRaSpreadingFactor sf = LoRaWAN::m_supportedDataRates [m_currentDataRateIndex].spreadingFactor;

  double symbolRate = ((double)bandwidth)/pow(2.0, sf);
  double symbolPeriod = 1.0e6/symbolRate; // the symbol period in microseconds

  double nSymbolsPreamble = m_preambleLength + 4.25;
  uint16_t nSymbolsPayload = 8;
  // LoRaWAN mandates no imlicit header, assume low data rate optimization (DE) is not used
  uint32_t crc = 1;
  if (!m_crcOn)
    crc = 0;

  uint16_t nConditionalSymbolsPayload = ceil((8.0*payloadLength - 4.0*sf + 28 + 16*crc)/4.0/(double)sf)*(m_codeRate + 4);
  if (nConditionalSymbolsPayload > 0.0)
    nSymbolsPayload += nConditionalSymbolsPayload;

  double txTime = (nSymbolsPreamble + nSymbolsPayload) * symbolPeriod;

  NS_LOG_DEBUG(this << ": " << sf  << "|" << (uint16_t)m_codeRate  << "|" << (uint16_t)payloadLength
      << "|" << (uint16_t) m_preambleLength  << "|" << nSymbolsPayload  << "|" << nConditionalSymbolsPayload
      << "|" << txTime << "uS");

  return MicroSeconds(txTime);
}

Time
LoRaWANPhy::CalculatePreambleTime ()
{
  const uint32_t bandwidth = LoRaWAN::m_supportedChannels [m_currentChannelIndex].m_bw;
  const LoRaSpreadingFactor sf = LoRaWAN::m_supportedDataRates [m_currentDataRateIndex].spreadingFactor;

  double symbolRate = ((double)bandwidth)/pow(2.0, sf);
  double symbolPeriod = 1.0e6/symbolRate; // the symbol period in microseconds

  double nSymbolsPreamble = m_preambleLength + 4.25;
  double timePreamble = nSymbolsPreamble * symbolPeriod;

  return MicroSeconds(timePreamble);
}

/* Returns the nominal PHY data rate */
double
LoRaWANPhy::GetNominalDataRate ()
{
  // calculations per $4.1.1.7 'Time on air' in sx1272 data sheet
  const uint32_t bandwidth = LoRaWAN::m_supportedChannels [m_currentChannelIndex].m_bw;
  const LoRaSpreadingFactor sf = LoRaWAN::m_supportedDataRates [m_currentDataRateIndex].spreadingFactor;

  double symbolRate = bandwidth/pow(2.0, sf);
  // TODO: return data rate of information stream, not code words stream

  // data rate is number of bits per symbol (i.e. SF) times number of symbols per second (i.e. Rs, symbol rate)
  return sf * symbolRate;
}

int64_t
LoRaWANPhy::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_random);
  m_random->SetStream (stream);
  return 1;
}

} // namespace ns3
