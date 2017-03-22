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
#ifndef LORAWAN_PHY_H
#define LORAWAN_PHY_H

#include "lorawan.h"
#include "lorawan-interference-helper.h"
#include <ns3/spectrum-phy.h>
#include <ns3/traced-callback.h>
#include <ns3/traced-value.h>
#include <ns3/event-id.h>

namespace ns3 {
/* ... */

class Packet;
class SpectrumValue;
class LoRaWANErrorModel;
struct LoRaWANSpectrumSignalParameters;
class MobilityModel;
class SpectrumChannel;
class SpectrumModel;
class AntennaModel;
class NetDevice;
class UniformRandomVariable;
/**
 * \ingroup lorawan
 *
 * LoRaWAN PHY states
 */
typedef enum
{
  LORAWAN_PHY_TRX_OFF = 0x00,
  LORAWAN_PHY_IDLE = 0x01,
  LORAWAN_PHY_RX_ON = 0x02,
  LORAWAN_PHY_TX_ON = 0x03,
  LORAWAN_PHY_BUSY_RX = 0x04,
  LORAWAN_PHY_BUSY_TX = 0x05,
  LORAWAN_PHY_SUCCESS = 0x06,
  LORAWAN_PHY_UNSPECIFIED = 0x07,
  LORAWAN_PHY_FORCE_TRX_OFF = 0x08,
} LoRaWANPhyEnumeration;

typedef enum
{
  LORAWAN_RX_DROP_PHY_BUSY_RX = 0x00,
  LORAWAN_RX_DROP_SINR_TOO_LOW = 0x01,
  LORAWAN_RX_DROP_NOT_IN_RX_STATE = 0x02,
  LORAWAN_RX_DROP_PACKET_DESTOYED = 0x03,
  LORAWAN_RX_DROP_ABORTED = 0x04,
  LORAWAN_RX_DROP_PACKET_ABORTED = 0x05,
} LoRaWANPhyDropRxReason;

typedef struct LoRaWANPhyRxStatus {
  LoRaWANPhyRxStatus() : LoRaWANPhyRxStatus (true, false) {}
  LoRaWANPhyRxStatus(bool d, bool a) : destroyed(d), aborted (a) {};
  bool destroyed; // was packet destroyed during reception (e.g. due to interference)
  bool aborted; // was packet reception aborted (e.g. due to transmission on Phy)
} LoRaWANPhyRxStatus;

namespace TracedValueCallback {

/**
 * \ingroup lorawan
 * TracedValue callback signature for LoRaWANPhyEnumeration.
 *
 * \param [in] oldValue original value of the traced variable
 * \param [in] newValue new value of the traced variable
 */
  typedef void (* LoRaWANPhyEnumeration)(LoRaWANPhyEnumeration oldValue,
                                  LoRaWANPhyEnumeration newValue);
/**
 * \ingroup lorawan
 * TracedCallback signature for dropped RX packets and reason why packet was dropped.
 *
 * \param [in] packet The packet.
 * \param [in] sinr The received SINR.
 */
  typedef void (* LoRaWANDroppedPacketTracedCallback) (Ptr<const Packet> packet, LoRaWANPhyDropRxReason reason);

}  // namespace TracedValueCallback

/**
 * \ingroup lorawan
 *
 * This method implements the PD SAP: PdDataIndication
 *
 *  @param psduLength number of bytes in the PSDU
 *  @param p the packet to be transmitted
 *  @param lqi Link quality (LQI) value measured during reception of the PPDU
 *  @param channelIndex index of the channel on which transmission was received
 *  @param dataRateIndex index of the data rate on which transmission was  received
 *  @param codeRate index of the code rate on which transmission was received
 */
typedef Callback< void, uint32_t, Ptr<Packet>, uint8_t, uint8_t, uint8_t, uint8_t> PdDataIndicationCallback;

/**
 * \ingroup lorawan
 *
 * This method implements the PD SAP: PdDataIndication
 *
 *  @param psduLength number of bytes in the PSDU
 *  @param p the packet to be transmitted
 *  @param lqi Link quality (LQI) value measured during reception of the PPDU
 *  @param channelIndex index of the channel on which transmission was received
 *  @param dataRateIndex index of the data rate on which transmission was  received
 *  @param codeRate index of the code rate on which transmission was received
 */
typedef Callback< void > PdDataDestroyedCallback;

/**
 * \ingroup lorawan
 *
 * This method implements the PD SAP: PdDataConfirm
 *
 * @param status the status to be transmitted
 */
typedef Callback< void, LoRaWANPhyEnumeration > PdDataConfirmCallback;

/**
 * \ingroup lorawan
 *
 * This method implements the PD SAP: SetTRXStateConfirm
 *
 * @param status the status of SetTRXStateRequest
 */
typedef Callback< void, LoRaWANPhyEnumeration > SetTRXStateConfirmCallback;

/**
 * \ingroup lorawan
 *
 * Phy layer for LoRaWAN, LoRa,  based on the SPD Phy model available in ns-3
 */
class LoRaWANPhy : public SpectrumPhy
{

public:
  /**
   * Get the type ID.
   *
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  /**
   * Default constructor.
   */
  LoRaWANPhy (void);
  LoRaWANPhy (uint8_t);
  virtual ~LoRaWANPhy (void);

  void PrintCurrentTxConf () const;
  bool SetTxConf (int8_t power, uint8_t channelIndex, uint8_t dataRateIndex, uint8_t codeRate, uint8_t preambleLength, bool implicitHeader, bool crcOn);

  uint8_t GetCurrentChannelIndex () const { return m_currentChannelIndex; }
  uint8_t GetCurrentDataRateIndex () const { return m_currentDataRateIndex; }

  /**
   * Calculate the time for transmitting the given packet in microseconds
   *
   * \param packet the packet for which the transmit time should be calculated
   * \return the transmit time in MicroSeconds
   */
  Time CalculateTxTime (uint8_t payloadLength);

  /*
   * Calculate the time for transmittin a preamble in microseconds
   * Note number of preamble symbols is supposed to be already configured in the PHY
   */
  Time CalculatePreambleTime();

  /**
   * Check whether PHY has detected a premable since it switched its state to RX_ON
   */
  bool preambleDetected (void) const;

  // inherited from SpectrumPhy
  void SetMobility (Ptr<MobilityModel> m);
  Ptr<MobilityModel> GetMobility (void);
  void SetChannel (Ptr<SpectrumChannel> c);

  /**
   * Get the currently attached channel.
   *
   * \return the channel
   */
  Ptr<SpectrumChannel> GetChannel (void);
  void SetDevice (Ptr<NetDevice> d);
  Ptr<NetDevice> GetDevice (void) const;

  /**
   * Set the attached antenna.
   *
   * \param a the antenna
   */
  void SetAntenna (Ptr<AntennaModel> a);
  Ptr<AntennaModel> GetRxAntenna (void);
  virtual Ptr<const SpectrumModel> GetRxSpectrumModel (void) const;

  /**
   * Set the Power Spectral Density of outgoing signals in W/Hz.
   *
   * @param txPsd the Power Spectral Density value
   */
  void SetTxPowerSpectralDensity (Ptr<SpectrumValue> txPsd);

  /**
   * Set the noise power spectral density.
   *
   * @param noisePsd the Noise Power Spectral Density in power units
   * (Watt, Pascal...) per Hz.
   */
  void SetNoisePowerSpectralDensity (Ptr<const SpectrumValue> noisePsd);

  /**
   * Get the noise power spectral density.
   *
   * @return the Noise Power Spectral Density
   */
  Ptr<const SpectrumValue> GetNoisePowerSpectralDensity (void);

  /**
    * Notify the SpectrumPhy instance of an incoming waveform.
    *
    * @param params the SpectrumSignalParameters associated with the incoming waveform
    */
  virtual void StartRx (Ptr<SpectrumSignalParameters> params);

  /**
   * set the error model to use
   *
   * @param e pointer to LoRaWANErrorModel to use
   */
  void SetErrorModel (Ptr<LoRaWANErrorModel> e);

  /**
   * get the error model in use
   *
   * @return pointer to LoRaWANErrorModel in use
   */
  Ptr<LoRaWANErrorModel> GetErrorModel (void) const;

  /**
   *  Ask Phy to switch state
   */
  void SetTRXStateRequest (LoRaWANPhyEnumeration state);

  /**
   * set the callback for the end of a RX, as part of the
   * interconnections between the PHY and the MAC. The callback
   * implements PD Indication SAP.
   * @param c the callback
   */
  void SetPdDataIndicationCallback (PdDataIndicationCallback c);

  void SetPdDataDestroyedCallback (PdDataDestroyedCallback c);

  /**
   * set the callback for the end of a TX, as part of the
   * interconnections between the PHY and the MAC. The callback
   * implements PD SAP.
   * @param c the callback
   */
  void SetPdDataConfirmCallback (PdDataConfirmCallback c);

  /**
   * set the callback for the end of an SetTRXState, as part of the
   * interconnections betweenthe PHY and the MAC. The callback
   * implement PLME SetTRXState confirm SAP
   * @param c the callback
   */
  void SetSetTRXStateConfirmCallback (SetTRXStateConfirmCallback c);

  /**
   * Get the duration of the SHR (preamble and SFD) in symbols, depending on
   * the currently selected channel.
   *
   * \return the SHR duration in symbols
   */
  uint64_t GetPhySHRDuration (void) const;

  /**
   * Get the number of symbols per octet, depending on the currently selected
   * channel.
   *
   * \return the number of symbols per octet
   */
  double GetPhySymbolsPerOctet (void) const;

  /**
   * Assign a fixed random variable stream number to the random variables
   * used by this model.  Return the number of streams that have been assigned.
   *
   * \param stream first stream index to use
   * \return the number of stream indices assigned by this model
   */
  int64_t AssignStreams (int64_t stream);

//  /**
//   * TracedCallback signature for Trx state change events.
//   *
//   * \param [in] time The time of the state change.
//   * \param [in] oldState The old state.
//   * \param [in] newState The new state.
//   * \deprecated The LoRaWANPhyEnumeration state is now accessible as the
//   * TracedValue \c TrxStateValue.  The \c TrxState TracedCallback will
//   * be removed in a future release.
//   */
//  typedef void (* StateTracedCallback)
//    (Time time, LoRaWANPhyEnumeration oldState, LoRaWANPhyEnumeration newState);

  /**
   *  Request to transfer PHY Payload from MAC (transmitting)
   *  @param phyPayloadLength number of bytes in the PHY Payload
   *  @param p the packet to be transmitted
   */
  void PdDataRequest (const uint32_t phyPayloadLength, Ptr<Packet> p);

  uint8_t GetIndex (void) const;
protected:

private:
  /**
   * The second is true if the first is flagged as error/invalid.
   */
  typedef std::pair<Ptr<Packet>, bool>  PacketAndStatus;

  // Inherited from Object.
  virtual void DoDispose (void);

  /**
   * Change the PHY state to the given new state, firing the state change trace.
   *
   * \param newState the new state
   */
  void ChangeTrxState (LoRaWANPhyEnumeration newState);

  /**
   * Finish the transmission of a frame. This is called at the end of a frame
   * transmission, applying possibly pending PHY state changes and fireing the
   * appropriate trace sources and confirm callbacks to the MAC.
   */
  void EndTx (void);

  /**
   * Check if the interference destroys a frame currently received. Called
   * whenever a change in interference is detected.
   */
  void CheckInterference (void);

  /**
   * Finish the reception of a frame. This is called at the end of a frame
   * reception, applying possibly pending PHY state changes and fireing the
   * appropriate trace sources and indication callbacks to the MAC. A frame
   * destroyed by noise/interference is dropped here, but not during reception.
   * This method is also called for every packet which only contributes to
   * interference.
   *
   * \param params signal parameters of the packet
   */
  void EndRx (Ptr<SpectrumSignalParameters> params);

  /**
   * Called after applying a deferred transceiver state switch. The result of
   * the state switch is reported to the MAC.
   */
  //void EndSetTRXState (void);

  /**
   * Check if the PHY is busy, which is the case if the PHY is currently sending
   * or receiving a frame.
   *
   * \return true, if the PHY is busy, false otherwise
   */
  //bool PhyIsBusy (void) const;

  double GetNominalDataRate();
  // Trace sources
  /**
   * The trace source fired when a packet begins the transmission process on
   * the medium.
   *
   * \see class CallBackTraceSource
   */
  TracedCallback<Ptr<const Packet> > m_phyTxBeginTrace;

  /**
   * The trace source fired when a packet ends the transmission process on
   * the medium.
   *
   * \see class CallBackTraceSource
   */
  TracedCallback<Ptr<const Packet> > m_phyTxEndTrace;

  /**
   * The trace source fired when the phy layer drops a packet as it tries
   * to transmit it.
   *
   * \see class CallBackTraceSource
   */
  TracedCallback<Ptr<const Packet> > m_phyTxDropTrace;

  /**
   * The trace source fired when a packet begins the reception process from
   * the medium.
   *
   * \see class CallBackTraceSource
   */
  TracedCallback<Ptr<const Packet> > m_phyRxBeginTrace;

  /**
   * The trace source fired when a packet ends the reception process from
   * the medium.  Second quantity is received SINR.
   *
   * \see class CallBackTraceSource
   */
  TracedCallback<Ptr<const Packet>, double > m_phyRxEndTrace;

  /**
   * The trace source fired when the phy layer drops a packet it has received.
   *
   * \see class CallBackTraceSource
   */
  TracedCallback<Ptr<const Packet>, LoRaWANPhyDropRxReason > m_phyRxDropTrace;

  /**
   * The mobility model used by the PHY.
   */
  Ptr<MobilityModel> m_mobility;

  /**
   * The configured net device.
   */
  Ptr<NetDevice> m_device;

  /**
   * The channel attached to this transceiver.
   */
  Ptr<SpectrumChannel> m_channel;

  /**
   * The antenna used by the transceiver.
   */
  Ptr<AntennaModel> m_antenna;

  /**
   * The transmit power spectral density.
   */
  Ptr<SpectrumValue> m_txPsd;

  /**
   * The spectral density for for the noise.
   */
  Ptr<const SpectrumValue> m_noise;

  /**
   * The error model describing the bit and packet error rates.
   */
  Ptr<LoRaWANErrorModel> m_errorModel;

  // State variables
  /**
   * The current transceiver state.
   */
  TracedValue<LoRaWANPhyEnumeration> m_trxState;

  /**
   * The next pending state to applied after the current action of the PHY is
   * completed.
   */
  //LoRaWANPhyEnumeration m_trxStatePending;

  // Callbacks
  /**
   * This callback is used to notify incoming packets to the MAC layer.
   */
  PdDataIndicationCallback m_pdDataIndicationCallback;

  /**
   * This callback is used to notify when an incoming packet was destroyed to
   * the MAC layer
   */
  PdDataDestroyedCallback m_pdDataDestroyedCallback;

  /**
   * This callback is used to report packet transmission status to the MAC layer.
   */
  PdDataConfirmCallback m_pdDataConfirmCallback;

  /**
   * This callback is used to report transceiver state change status to the MAC.
   */
  SetTRXStateConfirmCallback m_setTRXStateConfirmCallback;

  /**
   * Helper value for the peak power value during CCA.
   */
  double m_ccaPeakPower;

  /**
   * The receiver sensitivity.
   */
  double m_rxSensitivity;

  /**
   * The accumulated signals currently received by the transceiver, including
   * the signal of a possibly received packet, as well as all signals
   * considered noise.
   */
  Ptr<LoRaWANInterferenceHelper> m_signal;

  /**
   * Timestamp of the last calculation of the PER of a packet currently received.
   */
  Time m_rxLastUpdate;

  /**
   * Statusinformation of the currently received packet. The first parameter
   * contains the frame, as well the signal power of the frame. The second
   * parameter is set to false, if the frame is either invalid or destroyed
   * due to interference.
   */
  std::pair<Ptr<LoRaWANSpectrumSignalParameters>, LoRaWANPhyRxStatus>  m_currentRxPacket;

  /**
   * Statusinformation of the currently transmitted packet. The first parameter
   * contains the frame. The second parameter is set to false, if the frame not
   * completely transmitted, in the event of a force transceiver switch, for
   * example.
   */
  PacketAndStatus m_currentTxPacket;

  /**
   * Scheduler event of a currently running CCA request.
   */
  EventId m_ccaRequest;

  /**
   * Scheduler event of a currently running ED request.
   */
  EventId m_edRequest;

  /**
   * Scheduler event of a currently running deferred transceiver state switch.
   */
  EventId m_setTRXState;

  /**
   * Scheduler event of a currently running data transmission request.
   */
  EventId m_pdDataRequest;

  /**
   * Uniform random variable stream.
   */
  Ptr<UniformRandomVariable> m_random;

  /**
   * The index of this Phy object in the lorawan net device
   */
  uint8_t m_index;

  //uint8_t m_phyDataRate; // LoRaWAN DataRate as per $7.1.3
  uint8_t m_currentChannelIndex; // for RX and TX
  uint8_t m_currentDataRateIndex; // only for TX
  double m_txPower; // in dBm
  uint8_t m_codeRate; // only for TX
  uint8_t m_preambleLength;
  bool m_crcOn;
}; // class LoRaWANPhy


} // namespace ns3
#endif /* LORAWAN_PHY_H */
