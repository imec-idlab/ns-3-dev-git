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
// Include a header file from your module to test.
#include "ns3/lorawan.h"
#include "ns3/lorawan-frame-header.h"
#include "ns3/lorawan-mac-header.h"

// An essential include is test.h
#include "ns3/test.h"
#include "ns3/packet.h"
#include "ns3/ipv4-address.h"

// Do not put your test classes in namespace ns3.  You may find it useful
// to use the using directive to access the ns3 namespace directly
using namespace ns3;

// This is an example TestCase.
class LorawanPacketTestCase : public TestCase
{
public:
  LorawanPacketTestCase ();
  virtual ~LorawanPacketTestCase ();

private:
  virtual void DoRun (void);
};

// Add some help text to this case to describe what it is intended to test
LorawanPacketTestCase::LorawanPacketTestCase ()
  : TestCase ("Test the LoRaWAN MAC and frame headers classes")
{
}

// This destructor does nothing but we include it as a reminder that
// the test case should clean up after itself
LorawanPacketTestCase::~LorawanPacketTestCase ()
{
}

//
// This method is the pure virtual method from class TestCase that every
// TestCase must implement
//
void
LorawanPacketTestCase::DoRun (void)
{
  // Uplink messages: Preamble | PHDR | PHDR_CRC | PHYPayload | CRC
  // PHYPayload: MHDR | MACPayload | MIC
  // MACPayload: FHDR | FPort | FRMPayload
  // FHDR: DevAddr | FCtrl | FCnt | FOpts

  LoRaWANMacHeader mhdr;
  mhdr.setLoRaWANMsgType (LoRaWANMsgType::LORAWAN_UNCONFIRMED_DATA_UP);
  mhdr.setMajor (0);

  LoRaWANFrameHeader fhdr;
  Ipv4Address devAddr (0x00000001);
  fhdr.setDevAddr (devAddr);
  fhdr.setAck (true);
  fhdr.setFramePending (false);
  fhdr.setFrameCounter (10);
  // FPort: we will send FRMPayload so set FPort to 1
  fhdr.setFramePort (1);

  // Allocate 4 extra bytes which represent the MAC MIC
  Ptr<Packet> p = Create<Packet> (20+4);  // 20 bytes of dummy data
  NS_TEST_ASSERT_MSG_EQ (p->GetSize (), 24, "Packet created with unexpected size");

  // Frame header should add 8 bytes: DevAddr (4), FCtrl (1), FCnt (2), FOpts (0), FPort (1)
  p->AddHeader (fhdr);
  std::cout << " <--Frame Header added " << std::endl;
  NS_TEST_ASSERT_MSG_EQ (p->GetSize (), 32, "Packet wrong size after adding frame header");

  // Mac header should add 1 bytes
  p->AddHeader (mhdr);
  std::cout << " <--MAC Header added " << std::endl;
  NS_TEST_ASSERT_MSG_EQ (p->GetSize (), 33, "Packet wrong size after adding MAC header");

  p->Print (std::cout);
  std::cout << " <--Packet P1 " << std::endl;

  // Test serialization and deserialization
  uint32_t size = p->GetSerializedSize ();
  uint8_t buffer[size];
  p->Serialize (buffer, size);
  Ptr<Packet> p2 = Create<Packet> (buffer, size, true);
  p2->Print (std::cout);
  std::cout << " <--Packet P2 " << std::endl;
  NS_TEST_ASSERT_MSG_EQ (p2->GetSize (), 33, "Packet wrong size after deserialization");

  LoRaWANMacHeader receivedMHdr;
  p2->RemoveHeader (receivedMHdr);
  receivedMHdr.Print (std::cout);
  std::cout << " <--P2 Mac Header " << std::endl;
  NS_TEST_ASSERT_MSG_EQ (p2->GetSize (), 32, "Packet wrong size after removing mac hdr");

  LoRaWANFrameHeader receiverFHdr;
  receiverFHdr.setSerializeFramePort (true); // Frame Header contains Frame Port so set this to true (note Serialize en Deserialize are the same in this case)
  p2->RemoveHeader (receiverFHdr);
  receiverFHdr.Print (std::cout);
  std::cout << " <--P2 Frame Header " << std::endl;
  NS_TEST_ASSERT_MSG_EQ (p2->GetSize (), 24, "Packet wrong size after removing frame hdr");

  NS_TEST_ASSERT_MSG_EQ (receiverFHdr.getDevAddr().Get(), devAddr.Get(), "Addresses do not match");
  NS_TEST_ASSERT_MSG_EQ (receiverFHdr.getAck(), true, "Acks do not match");
  NS_TEST_ASSERT_MSG_EQ (receiverFHdr.getFramePending(), false, "Frame pendings do not match");
  NS_TEST_ASSERT_MSG_EQ (receiverFHdr.getFrameCounter(), 10, "Frame counters do not match");
  NS_TEST_ASSERT_MSG_EQ (receiverFHdr.getFramePort (), 1, "Frame ports do not match");
}

// The TestSuite class names the TestSuite, identifies what type of TestSuite,
// and enables the TestCases to be run.  Typically, only the constructor for
// this class must be defined
//
class LoRaWANPacketTestSuite : public TestSuite
{
public:
  LoRaWANPacketTestSuite ();
};

LoRaWANPacketTestSuite::LoRaWANPacketTestSuite ()
  : TestSuite ("lorawan-packet", UNIT)
{
  // TestDuration for TestCase can be QUICK, EXTENSIVE or TAKES_FOREVER
  AddTestCase (new LorawanPacketTestCase, TestCase::QUICK);
}

// Do not forget to allocate an instance of this TestSuite
static LoRaWANPacketTestSuite lorawanPacketTestSuite;

