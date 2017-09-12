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

#include "lorawan-helper.h"
#include <ns3/lorawan-net-device.h>
#include <ns3/simulator.h>
#include <ns3/mobility-model.h>
#include <ns3/single-model-spectrum-channel.h>
#include <ns3/multi-model-spectrum-channel.h>
#include <ns3/propagation-loss-model.h>
#include <ns3/propagation-delay-model.h>
#include <ns3/names.h>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LoRaWANHelper");

/* ... */
LoRaWANHelper::LoRaWANHelper (void) : m_deviceType (LORAWAN_DT_END_DEVICE_CLASS_A)
{
  m_channel = CreateObject<SingleModelSpectrumChannel> ();

  Ptr<LogDistancePropagationLossModel> lossModel = CreateObject<LogDistancePropagationLossModel> ();
  m_channel->AddPropagationLossModel (lossModel);

  Ptr<ConstantSpeedPropagationDelayModel> delayModel = CreateObject<ConstantSpeedPropagationDelayModel> ();
  m_channel->SetPropagationDelayModel (delayModel);
}

LoRaWANHelper::LoRaWANHelper (bool useMultiModelSpectrumChannel) : m_deviceType (LORAWAN_DT_END_DEVICE_CLASS_A)
{
  if (useMultiModelSpectrumChannel)
    {
      m_channel = CreateObject<MultiModelSpectrumChannel> ();
    }
  else
    {
      m_channel = CreateObject<SingleModelSpectrumChannel> ();
    }
  Ptr<LogDistancePropagationLossModel> lossModel = CreateObject<LogDistancePropagationLossModel> ();
  m_channel->AddPropagationLossModel (lossModel);

  Ptr<ConstantSpeedPropagationDelayModel> delayModel = CreateObject<ConstantSpeedPropagationDelayModel> ();
  m_channel->SetPropagationDelayModel (delayModel);
}

LoRaWANHelper::~LoRaWANHelper (void)
{
  //m_channel->Dispose ();
  m_channel = 0;
}

void
LoRaWANHelper::AddMobility (Ptr<LoRaWANPhy> phy, Ptr<MobilityModel> m)
{
  phy->SetMobility (m);
}

void
LoRaWANHelper::SetDeviceType (LoRaWANDeviceType deviceType)
{
  m_deviceType = deviceType;
}

void
LoRaWANHelper::SetNbRep (uint8_t nbRep)
{
  m_nbRep = nbRep;
}

void
LoRaWANHelper::EnableLogComponents (enum LogLevel level)
{
  //LogComponentEnableAll (LOG_PREFIX_TIME);
  //LogComponentEnableAll (LOG_PREFIX_FUNC);
  LogComponentEnableAll (LOG_PREFIX_ALL);

  LogComponentEnable ("LoRaWANSpectrumValueHelper", level);
  LogComponentEnable ("LoRaWANPhy", level);
  LogComponentEnable ("LoRaWANGatewayApplication", level);
  LogComponentEnable ("LoRaWANErrorModel", level);
  LogComponentEnable ("LoRaWANMac", level);
  LogComponentEnable ("LoRaWANNetDevice", level);
  LogComponentEnable ("LoRaWANInterferenceHelper", level);
  LogComponentEnable ("LoRaWANSpectrumSignalParameters", level);
  LogComponentEnable ("LoRaWANEndDeviceApplication", level);
  LogComponentEnable ("LoRaWANFrameHeader", level);
}

NetDeviceContainer
LoRaWANHelper::Install (NodeContainer c)
{
  NetDeviceContainer devices;
  static uint32_t addressCounter = 1;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); i++)
    {
      Ptr<Node> node = *i;

      Ptr<LoRaWANNetDevice> netDevice = CreateObject<LoRaWANNetDevice> (m_deviceType);

      netDevice->SetChannel (m_channel); // will also set channel on underlying phy(s)
      netDevice->SetNode (node);

      if (m_deviceType != LORAWAN_DT_GATEWAY) {
        netDevice->SetAddress (Ipv4Address(addressCounter++)); // will also set channel on underlying phy(s)
        netDevice->SetAttribute ("NbRep", UintegerValue (m_nbRep)); // set number of repetitions
      }

      node->AddDevice (netDevice);
      devices.Add (netDevice);
    }
  return devices;
}


Ptr<SpectrumChannel>
LoRaWANHelper::GetChannel (void)
{
  return m_channel;
}

void
LoRaWANHelper::SetChannel (Ptr<SpectrumChannel> channel)
{
  m_channel = channel;
}

void
LoRaWANHelper::SetChannel (std::string channelName)
{
  Ptr<SpectrumChannel> channel = Names::Find<SpectrumChannel> (channelName);
  m_channel = channel;
}

int64_t
LoRaWANHelper::AssignStreams (NetDeviceContainer c, int64_t stream)
{
  int64_t currentStream = stream;
  Ptr<NetDevice> netDevice;
  for (NetDeviceContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      netDevice = (*i);
      Ptr<LoRaWANNetDevice> lorawan = DynamicCast<LoRaWANNetDevice> (netDevice);
      if (lorawan)
        {
          currentStream += lorawan->AssignStreams (currentStream);
        }
    }
  return (currentStream - stream);
}
}
