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
#include <ns3/test.h>
#include <ns3/log.h>
#include <ns3/core-module.h>
#include <ns3/callback.h>
#include <ns3/packet.h>
#include <ns3/simulator.h>
#include <ns3/propagation-loss-model.h>
#include <ns3/node.h>
#include <ns3/net-device.h>
#include <ns3/single-model-spectrum-channel.h>
#include <ns3/constant-position-mobility-model.h>
#include "ns3/rng-seed-manager.h"

//#include "ns3/lorawan.h"
//#include "ns3/lorawan-error-model.h"
#include <ns3/lorawan-module.h>
//#include <ns3/lorawan-error-model.h>

#include <iostream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("lorawan-error-model-test");

class LoRaWANErrorDistanceTestCase : public TestCase
{
public:
  LoRaWANErrorDistanceTestCase ();
  virtual ~LoRaWANErrorDistanceTestCase ();
  uint32_t GetReceived (void) const
  {
    return m_received;
  }

private:
  virtual void DoRun (void);
  void IndicationCallback (LoRaWANDataIndicationParams params, Ptr<Packet> p);
  uint32_t m_received;
};

LoRaWANErrorDistanceTestCase::LoRaWANErrorDistanceTestCase ()
  : TestCase ("Test the lora error model vs distance"),
    m_received (0)
{
}

LoRaWANErrorDistanceTestCase::~LoRaWANErrorDistanceTestCase ()
{
}

void
LoRaWANErrorDistanceTestCase::IndicationCallback (LoRaWANDataIndicationParams params, Ptr<Packet> p)
{
  m_received++;
}

void
LoRaWANErrorDistanceTestCase::DoRun (void)
{
  // Set the random seed and run number for this test
  RngSeedManager::SetSeed (1);
  RngSeedManager::SetRun (6);

  Ptr<Node> n0 = CreateObject <Node> ();
  Ptr<Node> n1 = CreateObject <Node> ();
  Ptr<LoRaWANNetDevice> dev0 = CreateObject<LoRaWANNetDevice> (LORAWAN_DT_END_DEVICE_CLASS_A);
  Ptr<LoRaWANNetDevice> devgw = CreateObject<LoRaWANNetDevice> (LORAWAN_DT_GATEWAY);

  // Make random variable stream assignment deterministic
  dev0->AssignStreams (0);
  devgw->AssignStreams (10);

  Ipv4Address nodeAddr (0x00000001);
  dev0->SetAddress (nodeAddr);

  Ptr<SingleModelSpectrumChannel> channel = CreateObject<SingleModelSpectrumChannel> ();
  Ptr<LogDistancePropagationLossModel> model = CreateObject<LogDistancePropagationLossModel> ();
  channel->AddPropagationLossModel (model);

  dev0->SetChannel (channel);
  devgw->SetChannel (channel);

  // To complete configuration, a LoraWANNetDevice must be added to a node
  n0->AddDevice (dev0);
  n1->AddDevice (devgw);

  Ptr<ConstantPositionMobilityModel> mob0 = CreateObject<ConstantPositionMobilityModel> ();
  dev0->GetPhy ()->SetMobility (mob0);
  Ptr<ConstantPositionMobilityModel> mob1 = CreateObject<ConstantPositionMobilityModel> ();
  //sender1Mobility->SetPosition (Vector (0,0,0));
  for (auto &it : devgw->GetPhys() ) {
    it->SetMobility (mob1);
  }

  DataIndicationCallback cb0;
  cb0 = MakeCallback (&LoRaWANErrorDistanceTestCase::IndicationCallback, this);
  for (auto &it : devgw->GetMacs() ) {
    it->SetDataIndicationCallback (cb0);
  }

  LoRaWANDataRequestParams params;
  params.m_loraWANChannelIndex = 0;
  params.m_loraWANDataRateIndex = 5;
  params.m_loraWANCodeRate = 3;
  params.m_msgType = LORAWAN_UNCONFIRMED_DATA_UP;
  params.m_requestHandle = 1;
  params.m_numberOfTransmissions = 1;

  Ptr<Packet> p;
  mob0->SetPosition (Vector (0,0,0));
  mob1->SetPosition (Vector (2000,0,0));
  for (int i = 0; i < 1000; i++)
    {
      p = Create<Packet> (20);
      Simulator::Schedule(Seconds (4*i),
          &LoRaWANMac::sendMACPayloadRequest,
          dev0->GetMac (), params, p);
    }


  Simulator::Run ();

  NS_LOG_UNCOND ("Received " << GetReceived () << " packets");
  // Test that we received 977 packets out of 1000, at distance of 100 m
  // with default power of 0
  NS_TEST_ASSERT_MSG_EQ (GetReceived (), 954, "Model fails");

  Simulator::Destroy ();
}

// ==============================================================================
class LoRaWANErrorModelTestCase : public TestCase
{
public:
  LoRaWANErrorModelTestCase ();
  virtual ~LoRaWANErrorModelTestCase ();

private:
  virtual void DoRun (void);
};

LoRaWANErrorModelTestCase::LoRaWANErrorModelTestCase ()
  : TestCase ("Test the LoRaWAN error model")
{
}

LoRaWANErrorModelTestCase::~LoRaWANErrorModelTestCase ()
{
}

void
LoRaWANErrorModelTestCase::DoRun (void)
{
  //std::cout << "Starting BER calculation tests " << std::endl;

  Ptr<LoRaWANErrorModel> model = CreateObject<LoRaWANErrorModel> ();
  uint32_t bandwidth = 125e3;

  // TODO: extend test cases

  // SF7, CR3
  LoRaSpreadingFactor spreadingFactor = static_cast <LoRaSpreadingFactor> (7);
  uint8_t codeRate = 3;

  double snr = -5.0;
  double ber = model->getBER (snr, bandwidth, spreadingFactor, codeRate);
  ber = model->getBER (snr, bandwidth, spreadingFactor, codeRate);
  NS_TEST_ASSERT_MSG_EQ_TOL (ber, 6.7884e-17, 1.0e-19, "Error Model fails for SNR = " << snr << " BER = " << ber);

  snr = -8.0;
  ber = model->getBER (snr, bandwidth, spreadingFactor, codeRate);
  NS_TEST_ASSERT_MSG_EQ_TOL (ber, 5.54569e-06, 0.00001, "Error Model fails for SNR = " << snr << " BER = " << ber);

  snr = -10.0;
  ber = model->getBER (snr, bandwidth, spreadingFactor, codeRate);
  NS_TEST_ASSERT_MSG_EQ_TOL (ber, 0.00327354, 0.00001, "Error Model fails for SNR = " << snr << " BER = " << ber);

}

// ==============================================================================
class LoRaWANErrorModelTestSuite : public TestSuite
{
public:
  LoRaWANErrorModelTestSuite ();
};

LoRaWANErrorModelTestSuite::LoRaWANErrorModelTestSuite ()
  : TestSuite ("lorawan-error-model", UNIT)
{
  AddTestCase (new LoRaWANErrorModelTestCase, TestCase::QUICK);
  AddTestCase (new LoRaWANErrorDistanceTestCase, TestCase::QUICK);
}

static LoRaWANErrorModelTestSuite lorawanErrorModelTestSuite;
