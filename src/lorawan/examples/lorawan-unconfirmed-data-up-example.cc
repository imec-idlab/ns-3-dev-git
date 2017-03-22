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

/*
 * Try to send data from two class A end devices to a gateway, data is
 * unconfirmed upstream data. Chain is LoRaWANMac -> LoRaWANPhy ->
 * SpectrumChannel -> LoRaWANPhy -> LoRaWANMac
 *
 * Trace Phy state changes, and Mac DataIndication and DataConfirm events
 * to stdout
 */
#include <ns3/log.h>
#include <ns3/core-module.h>
#include <ns3/ipv4-address.h>
#include <ns3/lorawan-module.h>
#include <ns3/propagation-loss-model.h>
#include <ns3/propagation-delay-model.h>
#include <ns3/simulator.h>
#include <ns3/single-model-spectrum-channel.h>
#include <ns3/constant-position-mobility-model.h>
#include <ns3/node.h>
#include <ns3/packet.h>

#include <iostream>

using namespace ns3;

static void DataIndication (LoRaWANDataIndicationParams params, Ptr<Packet> p)
{
  NS_LOG_UNCOND ("DataIndication: Received packet of size " << p->GetSize ());
}

static void DataConfirm (LoRaWANDataConfirmParams params)
{
  NS_LOG_UNCOND ("DataConfirm: LoRaWANMcpsDataConfirmStatus = " << params.m_status);
}

static void StateChangeNotification (std::string context, Time now, LoRaWANPhyEnumeration oldState, LoRaWANPhyEnumeration newState)
{
  NS_LOG_UNCOND (context << " state change at " << now.GetSeconds ()
                         << " from " << oldState
                         << " to " << newState);
}

int main (int argc, char *argv[])
{
  // Create 2 nodes, and a NetDevice for each one
  Ptr<Node> n0 = CreateObject <Node> ();
  Ptr<Node> n1 = CreateObject <Node> ();
  // Create a gateway node and corresponding NetDevice
  Ptr<Node> gw0 = CreateObject <Node> ();

  Ptr<LoRaWANNetDevice> dev0 = CreateObject<LoRaWANNetDevice> (LORAWAN_DT_END_DEVICE_CLASS_A);
  Ptr<LoRaWANNetDevice> dev1 = CreateObject<LoRaWANNetDevice> (LORAWAN_DT_END_DEVICE_CLASS_A);
  Ptr<LoRaWANNetDevice> dev2 = CreateObject<LoRaWANNetDevice> (LORAWAN_DT_GATEWAY);

  dev0->SetAddress (Ipv4Address (0x00000001));
  dev1->SetAddress (Ipv4Address (0x00000002));
  // dev2->SetAddress (Ipv4Address (0x00000003));

  // Each device must be attached to the same channel
  Ptr<SingleModelSpectrumChannel> channel = CreateObject<SingleModelSpectrumChannel> ();
  Ptr<LogDistancePropagationLossModel> propModel = CreateObject<LogDistancePropagationLossModel> ();
  Ptr<ConstantSpeedPropagationDelayModel> delayModel = CreateObject<ConstantSpeedPropagationDelayModel> ();
  channel->AddPropagationLossModel (propModel);
  channel->SetPropagationDelayModel (delayModel);

  dev0->SetChannel (channel);
  dev1->SetChannel (channel);
  dev2->SetChannel (channel);

  // To complete configuration, a LoRaWANNetDevice must be added to a node
  n0->AddDevice (dev0);
  n1->AddDevice (dev1);
  gw0->AddDevice (dev2);

  // Trace state changes in the phy
  dev0->GetPhy ()->TraceConnect ("TrxState", std::string ("phy0"), MakeCallback (&StateChangeNotification));
  dev1->GetPhy ()->TraceConnect ("TrxState", std::string ("phy1"), MakeCallback (&StateChangeNotification));
  //dev2->GetPhy ()->TraceConnect ("TrxState", std::string ("phy2"), MakeCallback (&StateChangeNotification));
  for (auto &i : dev2->GetPhys()) {
    i->TraceConnect ("TrxState", std::string ("phy2"), MakeCallback (&StateChangeNotification));
  }

  Ptr<ConstantPositionMobilityModel> sender0Mobility = CreateObject<ConstantPositionMobilityModel> ();
  sender0Mobility->SetPosition (Vector (0,5,0));
  dev0->GetPhy ()->SetMobility (sender0Mobility);
  Ptr<ConstantPositionMobilityModel> sender1Mobility = CreateObject<ConstantPositionMobilityModel> ();
  // Configure position 10 m distance
  sender1Mobility->SetPosition (Vector (0,10,0));
  dev1->GetPhy ()->SetMobility (sender1Mobility);

  Ptr<ConstantPositionMobilityModel> gatewayMobility = CreateObject<ConstantPositionMobilityModel> ();
  gatewayMobility->SetPosition (Vector (0,0,0));
  for (auto &i : dev2->GetPhys()) {
    i->SetMobility (gatewayMobility);
  }

  DataConfirmCallback cb0;
  cb0 = MakeCallback (&DataConfirm);
  dev0->GetMac ()->SetDataConfirmCallback (cb0);

  DataIndicationCallback cb1;
  cb1 = MakeCallback (&DataIndication);
  dev0->GetMac ()->SetDataIndicationCallback (cb1);

  DataConfirmCallback cb2;
  cb2 = MakeCallback (&DataConfirm);
  dev1->GetMac ()->SetDataConfirmCallback (cb2);

  DataIndicationCallback cb3;
  cb3 = MakeCallback (&DataIndication);
  dev1->GetMac ()->SetDataIndicationCallback (cb3);

  DataConfirmCallback cb4;
  cb4 = MakeCallback (&DataConfirm);
  DataIndicationCallback cb5;
  cb5 = MakeCallback (&DataIndication);
  for (auto &it : dev2->GetMacs() ) {
    it->SetDataConfirmCallback (cb4);
    it->SetDataIndicationCallback (cb5);
  }

  Ptr<Packet> p0 = Create<Packet> (20);  // 20 bytes of dummy data

  LoRaWANDataRequestParams params;
  params.m_loraWANChannelIndex = 0;
  params.m_loraWANDataRateIndex = 5;
  params.m_msgType = LORAWAN_UNCONFIRMED_DATA_UP;
  params.m_requestHandle = 1;
  params.m_numberOfTransmissions = 0;

  // Send a packet from n0 to gw0
  Simulator::ScheduleWithContext (0, Seconds (0.0),
                                  &LoRaWANMac::sendMACPayloadRequest,
                                  dev0->GetMac (), params, p0);

  // Send a packet from n1 to gw0
  Ptr<Packet> p1 = Create<Packet> (30);  // 30 bytes of dummy data
  Simulator::ScheduleWithContext (2, Seconds (2.0),
                                  &LoRaWANMac::sendMACPayloadRequest,
                                  dev1->GetMac (), params, p1);

  Simulator::Stop (Seconds (10.0));

  Simulator::Run ();

  Simulator::Destroy ();
  return 0;
}
