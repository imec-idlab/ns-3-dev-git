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
#include <ns3/log.h>
#include <ns3/core-module.h>
#include <ns3/lorawan-module.h>
#include <ns3/propagation-loss-model.h>
#include <ns3/propagation-delay-model.h>
#include <ns3/simulator.h>
#include <ns3/single-model-spectrum-channel.h>
#include <ns3/constant-position-mobility-model.h>
#include <ns3/node.h>
#include <ns3/packet.h>
#include "ns3/rng-seed-manager.h"

#include <iostream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("lorawan-retransmit-timeout-test");

class LoRaWANRetransmitTimeoutTestCase : public TestCase
{
public:
  LoRaWANRetransmitTimeoutTestCase ();

  static void DataConfirm (LoRaWANRetransmitTimeoutTestCase *testCase, Ptr<LoRaWANNetDevice> dev, LoRaWANDataConfirmParams params);

private:
  virtual void DoRun (void);
  LoRaWANMcpsDataConfirmStatus confirmParamsStatus;
};

LoRaWANRetransmitTimeoutTestCase::LoRaWANRetransmitTimeoutTestCase ()
  : TestCase ("Test the LoRaWAN retransmission handling (including RDC)")
{
  confirmParamsStatus = LORAWAN_SUCCESS;
}

void
LoRaWANRetransmitTimeoutTestCase::DataConfirm (LoRaWANRetransmitTimeoutTestCase *testCase, Ptr<LoRaWANNetDevice> dev, LoRaWANDataConfirmParams params)
{
  testCase->confirmParamsStatus = params.m_status;
}

void
LoRaWANRetransmitTimeoutTestCase::DoRun (void)
{
  // Set the random seed and run number for this test
  RngSeedManager::SetSeed (1);
  RngSeedManager::SetRun (6);

  // Create 1 node, and a NetDevice
  Ptr<Node> n0 = CreateObject <Node> ();

  Ptr<LoRaWANNetDevice> dev0 = CreateObject<LoRaWANNetDevice> (LORAWAN_DT_END_DEVICE_CLASS_A);

  // Make random variable stream assignment deterministic
  //dev0->AssignStreams (0);

  dev0->SetAddress (Ipv4Address (0x00000001));

  // Each device must be attached to the same channel
  Ptr<SingleModelSpectrumChannel> channel = CreateObject<SingleModelSpectrumChannel> ();
  Ptr<LogDistancePropagationLossModel> propModel = CreateObject<LogDistancePropagationLossModel> ();
  Ptr<ConstantSpeedPropagationDelayModel> delayModel = CreateObject<ConstantSpeedPropagationDelayModel> ();
  channel->AddPropagationLossModel (propModel);
  channel->SetPropagationDelayModel (delayModel);

  dev0->SetChannel (channel);

  // To complete configuration, a LoRaWANNetDevice must be added to a node
  n0->AddDevice (dev0);

  Ptr<ConstantPositionMobilityModel> sender0Mobility = CreateObject<ConstantPositionMobilityModel> ();
  sender0Mobility->SetPosition (Vector (0,0,0));
  dev0->GetPhy ()->SetMobility (sender0Mobility);

  DataConfirmCallback cb0;
  cb0 = MakeBoundCallback (&LoRaWANRetransmitTimeoutTestCase::DataConfirm, this, dev0);
  dev0->GetMac ()->SetDataConfirmCallback (cb0);

  Ptr<Packet> p0 = Create<Packet> (20);  // 20 bytes of dummy data

  LoRaWANDataRequestParams params;
  params.m_loraWANChannelIndex = 0;
  params.m_loraWANDataRateIndex = 5;
  params.m_loraWANCodeRate = 3;
  params.m_msgType = LORAWAN_CONFIRMED_DATA_UP;
  params.m_requestHandle = 1;
  params.m_numberOfTransmissions = 3;

  Simulator::ScheduleNow (&LoRaWANMac::sendMACPayloadRequest, dev0->GetMac(), params, p0);

  Simulator::Run ();

  NS_TEST_ASSERT_MSG_EQ ((confirmParamsStatus == LORAWAN_NO_ACK), true, "ConfirmData status is not set to LORAWAN_NO_ACK. status =  " << confirmParamsStatus);

  Simulator::Destroy ();
}

class LoRaWANRetransmitTimeoutTestSuite  : public TestSuite
{
public:
  LoRaWANRetransmitTimeoutTestSuite ();
};

LoRaWANRetransmitTimeoutTestSuite::LoRaWANRetransmitTimeoutTestSuite ()
  : TestSuite ("lorawan-retransmit-timeout", UNIT)
{
  AddTestCase (new LoRaWANRetransmitTimeoutTestCase, TestCase::QUICK);
}

static LoRaWANRetransmitTimeoutTestSuite g_loraWANAckTimeoutTestSuite;
