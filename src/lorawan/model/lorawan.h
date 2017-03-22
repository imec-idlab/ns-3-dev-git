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
#ifndef LORAWAN_H
#define LORAWAN_H

#include <ns3/uinteger.h>
#include <ns3/packet.h>
#include <ns3/flow-id-tag.h>

#include <vector>

// For confirmed messages: number of transmissions
#define DEFAULT_NUMBER_US_TRANSMISSIONS 4
#define DEFAULT_NUMBER_DS_TRANSMISSIONS DEFAULT_NUMBER_US_TRANSMISSIONS

// Default settings for EU863-870
#define RECEIVE_DELAY1 1000000 // in uS
#define RECEIVE_DELAY2 2000000 // in uS

namespace ns3 {

/* ... */

  /**
   * \ingroup lorawan
   *
   * LoRaWAN channels
   */
  typedef struct
  {
    uint8_t m_channelIndex;
    uint32_t m_fc; // in Hz
    uint32_t m_bw;
    uint8_t m_subBandIndex;
  } LoRaWANChannel;

  /**
   * \ingroup lorawan
   *
   * LoRa spreading factors
   */
  typedef enum
  {
   LORAWAN_SF6 = 6,
   LORAWAN_SF7,
   LORAWAN_SF8,
   LORAWAN_SF9,
   LORAWAN_SF10,
   LORAWAN_SF11,
   LORAWAN_SF12,
  } LoRaSpreadingFactor;

  /**
   * \ingroup lorawan
   *
   * LoRaWAN Data Rate per $7.1.3
   */
  typedef struct
  {
    uint8_t dataRateIndex;
    LoRaSpreadingFactor spreadingFactor;
    uint32_t bandWith;
  } LoRaWANDataRate;


  /**
   * \ingroup lorawan
   *
   * LoRaWAN message types as per $4.2.1 in LoRaWAN spec
   */
  typedef enum
  {
    LORAWAN_DT_GATEWAY = 0,
    LORAWAN_DT_END_DEVICE_CLASS_A,
    LORAWAN_DT_END_DEVICE_CLASS_B,
    LORAWAN_DT_END_DEVICE_CLASS_C
  } LoRaWANDeviceType;

  /**
   * \ingroup lorawan
   *
   * LoRaWAN message types as per $4.2.1 in LoRaWAN spec
   */
  typedef enum
  {
   LORAWAN_JOIN_REQUEST = 0,
   LORAWAN_JOIN_ACCEPT,
   LORAWAN_UNCONFIRMED_DATA_UP,
   LORAWAN_UNCONFIRMED_DATA_DOWN,
   LORAWAN_CONFIRMED_DATA_UP,
   LORAWAN_CONFIRMED_DATA_DOWN,
   LORAWAN_RFU,
   LORAWAN_PROPRIETARY,
  } LoRaWANMsgType;

  class LoRaWAN {

  public:
    /**
     * The supported LoRa channels
     */
    static const std::vector<LoRaWANChannel> m_supportedChannels;

    /**
     * The supported LoRaWAN data rates
     */
    static const std::vector<LoRaWANDataRate> m_supportedDataRates;

    /*
     * Get the RX1 receive window data rate, note frequency is always the same for RW1
     */
    static uint8_t GetRX1DataRateIndex (uint8_t upstreamDRIndex, uint8_t rx1DROffset);

    /**
     * The channel and data rate index for transmissions in the second receive
     * window (RW2) of a class A end device
     */
    static uint8_t m_RW2ChannelIndex;
    static uint8_t m_RW2DataRateIndex;

  }; // class LoRaWAN

  class LoRaWANMsgTypeTag : public Tag {
  public:
    LoRaWANMsgTypeTag (void);

    void SetMsgType (LoRaWANMsgType);
    LoRaWANMsgType GetMsgType (void) const;

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId (void);

    // inherited function, no need to doc.
    virtual TypeId GetInstanceTypeId (void) const;

    // inherited function, no need to doc.
    virtual uint32_t GetSerializedSize (void) const;

    // inherited function, no need to doc.
    virtual void Serialize (TagBuffer i) const;

    // inherited function, no need to doc.
    virtual void Deserialize (TagBuffer i);

    // inherited function, no need to doc.
    virtual void Print (std::ostream &os) const;
  private:
    LoRaWANMsgType m_msgType;
  }; // class LoRaWANMsgTypeTag

  class LoRaWANPhyParamsTag : public Tag {
  public:
    LoRaWANPhyParamsTag (void);

    void SetChannelIndex (uint8_t);
    uint8_t GetChannelIndex (void) const;

    void SetDataRateIndex (uint8_t);
    uint8_t GetDataRateIndex (void) const;

    void SetCodeRate (uint8_t);
    uint8_t GetCodeRate (void) const;

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId (void);

    // inherited function, no need to doc.
    virtual TypeId GetInstanceTypeId (void) const;

    // inherited function, no need to doc.
    virtual uint32_t GetSerializedSize (void) const;

    // inherited function, no need to doc.
    virtual void Serialize (TagBuffer i) const;

    // inherited function, no need to doc.
    virtual void Deserialize (TagBuffer i);

    // inherited function, no need to doc.
    virtual void Print (std::ostream &os) const;
  private:
    uint8_t m_channelIndex;
    uint8_t m_dataRateIndex;
    uint8_t m_codeRate;
  }; // class LoRaWANPhyParamsTag

  typedef FlowIdTag LoRaWANPhyTraceIdTag;

  class LoRaWANCounterSingleton {
  public:
    LoRaWANCounterSingleton ();
    //static LoRaWANCounterSingleton* GetPtr ();
    static uint64_t GetCounter () { return m_counter--; } // return current value and decrement afterwards
  private:
    static LoRaWANCounterSingleton* m_ptr;
    static uint64_t m_counter;
  }; // class LoRaWANCounterSingleton

} // namespace ns3

#endif /* LORAWAN_H */

