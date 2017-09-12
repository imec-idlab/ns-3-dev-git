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

#ifndef LORAWAN_HELPER_H
#define LORAWAN_HELPER_H

#include <ns3/lorawan.h>
#include <ns3/lorawan-phy.h>
#include <ns3/node-container.h>
#include <ns3/net-device-container.h>
#include <ns3/log.h>

namespace ns3 {

/**
 * \brief helps to create LoRaWANNetDevice objects
 *
 * This class can help to create a large set of similar
 * LoRaWANNetDevice objects and to configure a large set of
 * their attributes during creation.
 */
class LoRaWANHelper {
public:
  /**
   * \brief Create a LoRaWAN helper in an empty state.  By default, a
   * SingleModelSpectrumChannel is created, with a 
   * LogDistancePropagationLossModel and a ConstantSpeedPropagationDelayModel.
   *
   * To change the channel type, loss model, or delay model, the Get/Set
   * Channel methods may be used.
   */
  LoRaWANHelper (void);

  /**
   * \brief Create a LoRaWAN helper in an empty state with either a
   * SingleModelSpectrumChannel or a MultiModelSpectrumChannel.
   * \param useMultiModelSpectrumChannel use a MultiModelSpectrumChannel if true, a SingleModelSpectrumChannel otherwise
   *
   * A LogDistancePropagationLossModel and a 
   * ConstantSpeedPropagationDelayModel are added to the channel.
   */
  LoRaWANHelper (bool useMultiModelSpectrumChannel);

  virtual ~LoRaWANHelper (void);

  /**
   * \brief Get the channel associated to this helper
   * \returns the channel
   */
  Ptr<SpectrumChannel> GetChannel (void);

  /**
   * \brief Set the channel associated to this helper
   * \param channel the channel
   */
  void SetChannel (Ptr<SpectrumChannel> channel);

  /**
   * \brief Set the channel associated to this helper
   * \param channelName the channel name
   */
  void SetChannel (std::string channelName);

  /**
   * \brief Add mobility model to a physical device
   * \param phy the physical device
   * \param m the mobility model
   */
  void AddMobility (Ptr<LoRaWANPhy> phy, Ptr<MobilityModel> m);

  /**
   * \brief Set the device type of the LoRaWANNetDevice objects created by this helper
   */
  void SetDeviceType (LoRaWANDeviceType deviceType);

  /**
   * \brief Set the NbRep attribute of the LoRaWANNetDevice objects created by this helper (only for end device net devices)
   */
  void SetNbRep (uint8_t rep);

  /**
   * \brief Install a LoRaWANNetDevice and the associated structures (e.g., channel) in the nodes.
   * \param c a set of nodes
   * \returns A container holding the added net devices.
   */
  NetDeviceContainer Install (NodeContainer c);

  /**
   * Assign a fixed random variable stream number to the random variables
   * used by this model. Return the number of streams that have been
   * assigned. The Install() method should have previously been
   * called by the user.
   *
   * \param c NetDeviceContainer of the set of net devices for which the
   *          CsmaNetDevice should be modified to use a fixed stream
   * \param stream first stream index to use
   * \return the number of stream indices assigned by this helper
   */
  int64_t AssignStreams (NetDeviceContainer c, int64_t stream);

  void EnableLogComponents (enum LogLevel level = LOG_LEVEL_ALL);
private:
  // Disable implicit constructors
  /**
   * \brief Copy constructor - defined and not implemented.
   */
  LoRaWANHelper (LoRaWANHelper const &);
  /**
   * \brief Copy constructor - defined and not implemented.
   * \returns
   */
  LoRaWANHelper& operator= (LoRaWANHelper const &);

private:
  Ptr<SpectrumChannel> m_channel; //!< channel to be used for the devices
  LoRaWANDeviceType m_deviceType; //!< the device type to use when creating new LoRaWANNetDevice objects
  uint8_t m_nbRep; //!< number of repetitions for unconfirmed us data (only for end devices)
};

}

#endif /* LORAWAN_HELPER_H */
