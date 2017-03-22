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
  // Create 3 nodes, and a NetDevice for each one
  Ptr<Node> n0 = CreateObject <Node> ();
  Ptr<Node> n1 = CreateObject <Node> ();
  Ptr<Node> n2 = CreateObject <Node> ();
  Ptr<Node> n3 = CreateObject <Node> ();
  // Create a gateway node and corresponding NetDevice
  Ptr<Node> gw0 = CreateObject <Node> ();

  Ptr<LoRaWANNetDevice> dev0 = CreateObject<LoRaWANNetDevice> (LORAWAN_DT_END_DEVICE_CLASS_A);
  Ptr<LoRaWANNetDevice> dev1 = CreateObject<LoRaWANNetDevice> (LORAWAN_DT_END_DEVICE_CLASS_A);
  Ptr<LoRaWANNetDevice> dev2 = CreateObject<LoRaWANNetDevice> (LORAWAN_DT_END_DEVICE_CLASS_A);
  Ptr<LoRaWANNetDevice> dev3 = CreateObject<LoRaWANNetDevice> (LORAWAN_DT_END_DEVICE_CLASS_A);
  Ptr<LoRaWANNetDevice> devgw = CreateObject<LoRaWANNetDevice> (LORAWAN_DT_GATEWAY);

  dev0->SetAddress (Ipv4Address (0x00000001));
  dev1->SetAddress (Ipv4Address (0x00000002));
  dev2->SetAddress (Ipv4Address (0x00000003));
  dev3->SetAddress (Ipv4Address (0x00000004));

  // Each device must be attached to the same channel
  Ptr<SingleModelSpectrumChannel> channel = CreateObject<SingleModelSpectrumChannel> ();
  Ptr<LogDistancePropagationLossModel> propModel = CreateObject<LogDistancePropagationLossModel> ();
  Ptr<ConstantSpeedPropagationDelayModel> delayModel = CreateObject<ConstantSpeedPropagationDelayModel> ();
  channel->AddPropagationLossModel (propModel);
  channel->SetPropagationDelayModel (delayModel);

  dev0->SetChannel (channel);
  dev1->SetChannel (channel);
  dev2->SetChannel (channel);
  dev3->SetChannel (channel);
  devgw->SetChannel (channel);

  // To complete configuration, a LoRaWANNetDevice must be added to a node
  n0->AddDevice (dev0);
  n1->AddDevice (dev1);
  n2->AddDevice (dev2);
  n3->AddDevice (dev3);
  gw0->AddDevice (devgw);

  // Trace state changes in the phy
  dev0->GetPhy ()->TraceConnect ("TrxState", std::string ("phy0"), MakeCallback (&StateChangeNotification));
  dev1->GetPhy ()->TraceConnect ("TrxState", std::string ("phy1"), MakeCallback (&StateChangeNotification));
  dev2->GetPhy ()->TraceConnect ("TrxState", std::string ("phy2"), MakeCallback (&StateChangeNotification));
  dev3->GetPhy ()->TraceConnect ("TrxState", std::string ("phy3"), MakeCallback (&StateChangeNotification));
  for (auto &i : devgw->GetPhys()) {
    i->TraceConnect ("TrxState", std::string ("phy-gw"), MakeCallback (&StateChangeNotification));
  }

  Ptr<ConstantPositionMobilityModel> sender0Mobility = CreateObject<ConstantPositionMobilityModel> ();
  sender0Mobility->SetPosition (Vector (0,5,0));
  dev0->GetPhy ()->SetMobility (sender0Mobility);
  Ptr<ConstantPositionMobilityModel> sender1Mobility = CreateObject<ConstantPositionMobilityModel> ();
  sender1Mobility->SetPosition (Vector (0,5,0));
  dev1->GetPhy ()->SetMobility (sender1Mobility);
  Ptr<ConstantPositionMobilityModel> sender2Mobility = CreateObject<ConstantPositionMobilityModel> ();
  sender2Mobility->SetPosition (Vector (0,5,0));
  dev2->GetPhy ()->SetMobility (sender2Mobility);
  Ptr<ConstantPositionMobilityModel> sender3Mobility = CreateObject<ConstantPositionMobilityModel> ();
  sender3Mobility->SetPosition (Vector (0,5,0));
  dev3->GetPhy ()->SetMobility (sender3Mobility);

  Ptr<ConstantPositionMobilityModel> gatewayMobility = CreateObject<ConstantPositionMobilityModel> ();
  gatewayMobility->SetPosition (Vector (0,0,0));
  for (auto &i : devgw->GetPhys()) {
    i->SetMobility (gatewayMobility);
  }

  DataConfirmCallback cb0 = MakeCallback (&DataConfirm);
  dev0->GetMac ()->SetDataConfirmCallback (cb0);
  DataIndicationCallback cb1 = MakeCallback (&DataIndication);
  dev0->GetMac ()->SetDataIndicationCallback (cb1);

  DataConfirmCallback cb2 = MakeCallback (&DataConfirm);
  dev1->GetMac ()->SetDataConfirmCallback (cb2);
  DataIndicationCallback cb3 = MakeCallback (&DataIndication);
  dev1->GetMac ()->SetDataIndicationCallback (cb3);

  DataConfirmCallback cb4 = MakeCallback (&DataConfirm);
  dev2->GetMac ()->SetDataConfirmCallback (cb4);
  DataIndicationCallback cb5 = MakeCallback (&DataIndication);
  dev2->GetMac ()->SetDataIndicationCallback (cb5);

  DataConfirmCallback cb6 = MakeCallback (&DataConfirm);
  dev3->GetMac ()->SetDataConfirmCallback (cb6);
  DataIndicationCallback cb7 = MakeCallback (&DataIndication);
  dev3->GetMac ()->SetDataIndicationCallback (cb7);

  DataConfirmCallback cb8 = MakeCallback (&DataConfirm);
  DataIndicationCallback cb9 = MakeCallback (&DataIndication);
  for (auto &it : devgw->GetMacs() ) {
    it->SetDataConfirmCallback (cb8);
    it->SetDataIndicationCallback (cb9);
  }

  // Send a packet from n0 to gw0
  LoRaWANDataRequestParams params0;
  params0.m_loraWANChannelIndex = 0;
  params0.m_loraWANDataRateIndex = 5;
  params0.m_loraWANCodeRate = 3;
  params0.m_msgType = LORAWAN_UNCONFIRMED_DATA_UP;
  params0.m_requestHandle = 0;
  params0.m_numberOfTransmissions = 0;
  Ptr<Packet> p0 = Create<Packet> (30);  // 30 bytes of dummy data
  Simulator::ScheduleWithContext (0, Seconds (0.0),
                                  &LoRaWANMac::sendMACPayloadRequest,
                                  dev0->GetMac (), params0, p0);

  // Send a packet from n1 to gw0
  LoRaWANDataRequestParams params1;
  params1.m_loraWANChannelIndex = 0;
  params1.m_loraWANDataRateIndex = 4;
  params1.m_loraWANCodeRate = 3;
  params1.m_msgType = LORAWAN_UNCONFIRMED_DATA_UP;
  params1.m_requestHandle = 1;
  params1.m_numberOfTransmissions = 0;
  Ptr<Packet> p1 = Create<Packet> (30);  // 30 bytes of dummy data
  Simulator::ScheduleWithContext (1, Seconds (0.010),
                                  &LoRaWANMac::sendMACPayloadRequest,
                                  dev1->GetMac (), params1, p1);

  // Send a packet from n2 to gw0
  LoRaWANDataRequestParams params2;
  params2.m_loraWANChannelIndex = 0;
  params2.m_loraWANDataRateIndex = 3;
  params2.m_loraWANCodeRate = 3;
  params2.m_msgType = LORAWAN_UNCONFIRMED_DATA_UP;
  params2.m_requestHandle = 2;
  params2.m_numberOfTransmissions = 0;
  Ptr<Packet> p2 = Create<Packet> (30);  // 30 bytes of dummy data
  Simulator::ScheduleWithContext (2, Seconds (0.020),
                                  &LoRaWANMac::sendMACPayloadRequest,
                                  dev2->GetMac (), params2, p2);

  // Send a packet from n3 to gw0
  LoRaWANDataRequestParams params3;
  params3.m_loraWANChannelIndex = 0;
  params3.m_loraWANDataRateIndex = 2;
  params3.m_loraWANCodeRate = 3;
  params3.m_msgType = LORAWAN_UNCONFIRMED_DATA_UP;
  params3.m_requestHandle = 3;
  params3.m_numberOfTransmissions = 0;
  Ptr<Packet> p3 = Create<Packet> (30);  // 30 bytes of dummy data
  Simulator::ScheduleWithContext (3, Seconds (0.030),
                                  &LoRaWANMac::sendMACPayloadRequest,
                                  dev3->GetMac (), params3, p3);

  Simulator::Stop (Seconds (10.0));

  Simulator::Run ();

  Simulator::Destroy ();
  return 0;
}
