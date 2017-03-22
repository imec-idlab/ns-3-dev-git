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
#include "ns3/lorawan-phy.h"

// An essential include is test.h
#include "ns3/test.h"
#include "ns3/nstime.h"
#include <ns3/log.h>

// Do not put your test classes in namespace ns3.  You may find it useful
// to use the using directive to access the ns3 namespace directly
using namespace ns3;

// This is an example TestCase.
class LoRaWANPhyTxTimeTestCase : public TestCase
{
public:
  LoRaWANPhyTxTimeTestCase ();
  virtual ~LoRaWANPhyTxTimeTestCase ();

private:
  virtual void DoRun (void);
};

// Add some help text to this case to describe what it is intended to test
LoRaWANPhyTxTimeTestCase::LoRaWANPhyTxTimeTestCase ()
  : TestCase ("Test calculation of PHY preamble duration and PHY frame duration and compare to the LoRa Modem Calculator Tool")
{
}

// This destructor does nothing but we include it as a reminder that
// the test case should clean up after itself
LoRaWANPhyTxTimeTestCase::~LoRaWANPhyTxTimeTestCase ()
{
}

//
// This method is the pure virtual method from class TestCase that every
// TestCase must implement
//
void
LoRaWANPhyTxTimeTestCase::DoRun (void)
{
  Ptr<LoRaWANPhy> phy = CreateObject<LoRaWANPhy> ();

  int8_t txPower = 2;
  uint32_t channelIndex = 0;
  uint32_t dataRateIndex = 5; // SF7
  uint32_t spreadingFactor = LoRaWAN::m_supportedDataRates [dataRateIndex].spreadingFactor;

  uint32_t codeRate = 1;
  uint16_t preambleLength = 8;
  bool implicitHeader = false;
  bool crcOn = true;

  // Configure Tx
  bool txConfSucces = phy->SetTxConf (txPower, channelIndex, dataRateIndex, codeRate, preambleLength, implicitHeader, crcOn);
  NS_TEST_ASSERT_MSG_EQ (txConfSucces, true, "Failed to configure LoRa PHY");

  // Preamble SF7
  Time preambleDuration = phy->CalculatePreambleTime ();
  Time expected = MicroSeconds(12.54e3);
  NS_TEST_ASSERT_MSG_EQ_TOL (preambleDuration, expected, expected/100, "LoRaWANPhy::CalculatePreambleTime() returned " << preambleDuration << ". Expected " << expected);

  // SF7, CR1
  Time timeOnAir = phy->CalculateTxTime (8);
  expected = MicroSeconds(36.1e3);
  NS_TEST_ASSERT_MSG_EQ_TOL (timeOnAir, expected, expected/100, "LoRaWANPhy::CalculateTxTime() returned " << timeOnAir << ". Expected " << expected << ". " << spreadingFactor << "/" << codeRate);

  timeOnAir = phy->CalculateTxTime (41);
  expected = MicroSeconds(87.3e3);
  NS_TEST_ASSERT_MSG_EQ_TOL (timeOnAir, expected, expected/100, "LoRaWANPhy::CalculateTxTime() returned " << timeOnAir << ". Expected " << expected << ". " << spreadingFactor << "/" << codeRate);

  // SF7, CR3
  codeRate = 3;
  txConfSucces = phy->SetTxConf (txPower, channelIndex, dataRateIndex, codeRate, preambleLength, implicitHeader, crcOn);
  NS_TEST_ASSERT_MSG_EQ (txConfSucces, true, "Failed to configure LoRa PHY");

  timeOnAir = phy->CalculateTxTime (8);
  expected = MicroSeconds(42.24e3);
  NS_TEST_ASSERT_MSG_EQ_TOL (timeOnAir, expected, expected/100, "LoRaWANPhy::CalculateTxTime() returned " << timeOnAir << ". Expected " << expected << ". " << spreadingFactor << "/" << codeRate);

  timeOnAir = phy->CalculateTxTime (10);
  expected = MicroSeconds(49.41e3);
  NS_TEST_ASSERT_MSG_EQ_TOL (timeOnAir, expected, expected/100, "LoRaWANPhy::CalculateTxTime() returned " << timeOnAir << ". Expected " << expected << ". " << spreadingFactor << "/" << codeRate);

  timeOnAir = phy->CalculateTxTime (41);
  expected = MicroSeconds(113.92e3);
  NS_TEST_ASSERT_MSG_EQ_TOL (timeOnAir, expected, expected/100, "LoRaWANPhy::CalculateTxTime() returned " << timeOnAir << ". Expected " << expected << ". " << spreadingFactor << "/" << codeRate);

  // Preamble SF8
  dataRateIndex = 4; // SF8
  spreadingFactor = LoRaWAN::m_supportedDataRates [dataRateIndex].spreadingFactor;
  txConfSucces = phy->SetTxConf (txPower, channelIndex, dataRateIndex, codeRate, preambleLength, implicitHeader, crcOn);
  NS_TEST_ASSERT_MSG_EQ (txConfSucces, true, "Failed to configure LoRa PHY");

  preambleDuration = phy->CalculatePreambleTime ();
  expected = MicroSeconds(25.09e3);
  NS_TEST_ASSERT_MSG_EQ_TOL (preambleDuration, expected, expected/100, "LoRaWANPhy::CalculatePreambleTime() returned " << preambleDuration << ". Expected " << expected);

  // SF8, CR1
  codeRate = 1;
  txConfSucces = phy->SetTxConf (txPower, channelIndex, dataRateIndex, codeRate, preambleLength, implicitHeader, crcOn);
  NS_TEST_ASSERT_MSG_EQ (txConfSucces, true, "Failed to configure LoRa PHY");

  timeOnAir = phy->CalculateTxTime (8);
  expected = MicroSeconds(72.19e3);
  NS_TEST_ASSERT_MSG_EQ_TOL (timeOnAir, expected, expected/100, "LoRaWANPhy::CalculateTxTime() returned " << timeOnAir << ". Expected " << expected << ". " << spreadingFactor << "/" << codeRate);

  timeOnAir = phy->CalculateTxTime (39);
  expected = MicroSeconds(154.11e3);
  NS_TEST_ASSERT_MSG_EQ_TOL (timeOnAir, expected, expected/100, "LoRaWANPhy::CalculateTxTime() returned " << timeOnAir << ". Expected " << expected << ". " << spreadingFactor << "/" << codeRate);

  // SF8, CR3
  codeRate = 3;
  txConfSucces = phy->SetTxConf (txPower, channelIndex, dataRateIndex, codeRate, preambleLength, implicitHeader, crcOn);
  NS_TEST_ASSERT_MSG_EQ (txConfSucces, true, "Failed to configure LoRa PHY");

  timeOnAir = phy->CalculateTxTime (8);
  expected = MicroSeconds(84.48e3);
  NS_TEST_ASSERT_MSG_EQ_TOL (timeOnAir, expected, expected/100, "LoRaWANPhy::CalculateTxTime() returned " << timeOnAir << ". Expected " << expected << ". " << spreadingFactor << "/" << codeRate);

  timeOnAir = phy->CalculateTxTime (39);
  expected = MicroSeconds(199.17e3);
  NS_TEST_ASSERT_MSG_EQ_TOL (timeOnAir, expected, expected/100, "LoRaWANPhy::CalculateTxTime() returned " << timeOnAir << ". Expected " << expected << ". " << spreadingFactor << "/" << codeRate);

  // Preamble SF9
  dataRateIndex = 3; // SF9
  spreadingFactor = LoRaWAN::m_supportedDataRates [dataRateIndex].spreadingFactor;
  txConfSucces = phy->SetTxConf (txPower, channelIndex, dataRateIndex, codeRate, preambleLength, implicitHeader, crcOn);
  NS_TEST_ASSERT_MSG_EQ (txConfSucces, true, "Failed to configure LoRa PHY");

  preambleDuration = phy->CalculatePreambleTime ();
  expected = MicroSeconds(50.18e3);
  NS_TEST_ASSERT_MSG_EQ_TOL (preambleDuration, expected, expected/100, "LoRaWANPhy::CalculatePreambleTime() returned " << preambleDuration << ". Expected " << expected);

  // SF9, CR1
  codeRate = 1;
  txConfSucces = phy->SetTxConf (txPower, channelIndex, dataRateIndex, codeRate, preambleLength, implicitHeader, crcOn);
  NS_TEST_ASSERT_MSG_EQ (txConfSucces, true, "Failed to configure LoRa PHY");

  timeOnAir = phy->CalculateTxTime (8);
  expected = MicroSeconds(123.9e3);
  NS_TEST_ASSERT_MSG_EQ_TOL (timeOnAir, expected, expected/100, "LoRaWANPhy::CalculateTxTime() returned " << timeOnAir << ". Expected " << expected << ". " << spreadingFactor << "/" << codeRate);

  timeOnAir = phy->CalculateTxTime (38);
  expected = MicroSeconds(267.26e3);
  NS_TEST_ASSERT_MSG_EQ_TOL (timeOnAir, expected, expected/100, "LoRaWANPhy::CalculateTxTime() returned " << timeOnAir << ". Expected " << expected << ". " << spreadingFactor << "/" << codeRate);

  // SF9, CR3
  codeRate = 3;
  txConfSucces = phy->SetTxConf (txPower, channelIndex, dataRateIndex, codeRate, preambleLength, implicitHeader, crcOn);
  NS_TEST_ASSERT_MSG_EQ (txConfSucces, true, "Failed to configure LoRa PHY");

  timeOnAir = phy->CalculateTxTime (8);
  expected = MicroSeconds(140.29e3);
  NS_TEST_ASSERT_MSG_EQ_TOL (timeOnAir, expected, expected/100, "LoRaWANPhy::CalculateTxTime() returned " << timeOnAir << ". Expected " << expected << ". " << spreadingFactor << "/" << codeRate);

  timeOnAir = phy->CalculateTxTime (38);
  expected = MicroSeconds(340.99e3);
  NS_TEST_ASSERT_MSG_EQ_TOL (timeOnAir, expected, expected/100, "LoRaWANPhy::CalculateTxTime() returned " << timeOnAir << ". Expected " << expected << ". " << spreadingFactor << "/" << codeRate);

  // Preamble SF10
  dataRateIndex = 2; // SF10
  spreadingFactor = LoRaWAN::m_supportedDataRates [dataRateIndex].spreadingFactor;
  txConfSucces = phy->SetTxConf (txPower, channelIndex, dataRateIndex, codeRate, preambleLength, implicitHeader, crcOn);
  NS_TEST_ASSERT_MSG_EQ (txConfSucces, true, "Failed to configure LoRa PHY");

  preambleDuration = phy->CalculatePreambleTime ();
  expected = MicroSeconds(100.35e3);
  NS_TEST_ASSERT_MSG_EQ_TOL (preambleDuration, expected, expected/100, "LoRaWANPhy::CalculatePreambleTime() returned " << preambleDuration << ". Expected " << expected);

  // SF10, CR1
  codeRate = 1;
  txConfSucces = phy->SetTxConf (txPower, channelIndex, dataRateIndex, codeRate, preambleLength, implicitHeader, crcOn);
  NS_TEST_ASSERT_MSG_EQ (txConfSucces, true, "Failed to configure LoRa PHY");

  timeOnAir = phy->CalculateTxTime (8);
  expected = MicroSeconds(247.81e3);
  NS_TEST_ASSERT_MSG_EQ_TOL (timeOnAir, expected, expected/100, "LoRaWANPhy::CalculateTxTime() returned " << timeOnAir << ". Expected " << expected << ". " << spreadingFactor << "/" << codeRate);

  timeOnAir = phy->CalculateTxTime (38);
  expected = MicroSeconds(493.47e3);
  NS_TEST_ASSERT_MSG_EQ_TOL (timeOnAir, expected, expected/100, "LoRaWANPhy::CalculateTxTime() returned " << timeOnAir << ". Expected " << expected << ". " << spreadingFactor << "/" << codeRate);

  // SF10, CR3
  codeRate = 3;
  txConfSucces = phy->SetTxConf (txPower, channelIndex, dataRateIndex, codeRate, preambleLength, implicitHeader, crcOn);
  NS_TEST_ASSERT_MSG_EQ (txConfSucces, true, "Failed to configure LoRa PHY");

  timeOnAir = phy->CalculateTxTime (8);
  expected = MicroSeconds(280.58e3);
  NS_TEST_ASSERT_MSG_EQ_TOL (timeOnAir, expected, expected/100, "LoRaWANPhy::CalculateTxTime() returned " << timeOnAir << ". Expected " << expected << ". " << spreadingFactor << "/" << codeRate);

  timeOnAir = phy->CalculateTxTime (38);
  expected = MicroSeconds(624.64e3);
  NS_TEST_ASSERT_MSG_EQ_TOL (timeOnAir, expected, expected/100, "LoRaWANPhy::CalculateTxTime() returned " << timeOnAir << ". Expected " << expected << ". " << spreadingFactor << "/" << codeRate);

  // Preamble SF11
  dataRateIndex = 1; // SF11
  spreadingFactor = LoRaWAN::m_supportedDataRates [dataRateIndex].spreadingFactor;
  txConfSucces = phy->SetTxConf (txPower, channelIndex, dataRateIndex, codeRate, preambleLength, implicitHeader, crcOn);
  NS_TEST_ASSERT_MSG_EQ (txConfSucces, true, "Failed to configure LoRa PHY");

  preambleDuration = phy->CalculatePreambleTime ();
  expected = MicroSeconds(200.70e3);
  NS_TEST_ASSERT_MSG_EQ_TOL (preambleDuration, expected, expected/100, "LoRaWANPhy::CalculatePreambleTime() returned " << preambleDuration << ". Expected " << expected);

  // SF11, CR1
  codeRate = 1;
  txConfSucces = phy->SetTxConf (txPower, channelIndex, dataRateIndex, codeRate, preambleLength, implicitHeader, crcOn);
  NS_TEST_ASSERT_MSG_EQ (txConfSucces, true, "Failed to configure LoRa PHY");

  timeOnAir = phy->CalculateTxTime (8);
  expected = MicroSeconds(495.62e3);
  NS_TEST_ASSERT_MSG_EQ_TOL (timeOnAir, expected, expected/100, "LoRaWANPhy::CalculateTxTime() returned " << timeOnAir << ". Expected " << expected << ". " << spreadingFactor << "/" << codeRate);

  timeOnAir = phy->CalculateTxTime (36);
  expected = MicroSeconds(905.22e3);
  NS_TEST_ASSERT_MSG_EQ_TOL (timeOnAir, expected, expected/100, "LoRaWANPhy::CalculateTxTime() returned " << timeOnAir << ". Expected " << expected << ". " << spreadingFactor << "/" << codeRate);

  // SF11, CR3
  codeRate = 3;
  txConfSucces = phy->SetTxConf (txPower, channelIndex, dataRateIndex, codeRate, preambleLength, implicitHeader, crcOn);
  NS_TEST_ASSERT_MSG_EQ (txConfSucces, true, "Failed to configure LoRa PHY");

  timeOnAir = phy->CalculateTxTime (8);
  expected = MicroSeconds(561.15e3);
  NS_TEST_ASSERT_MSG_EQ_TOL (timeOnAir, expected, expected/100, "LoRaWANPhy::CalculateTxTime() returned " << timeOnAir << ". Expected " << expected << ". " << spreadingFactor << "/" << codeRate);

  timeOnAir = phy->CalculateTxTime (36);
  expected = MicroSeconds(1134.59e3);
  NS_TEST_ASSERT_MSG_EQ_TOL (timeOnAir, expected, expected/100, "LoRaWANPhy::CalculateTxTime() returned " << timeOnAir << ". Expected " << expected << ". " << spreadingFactor << "/" << codeRate);

  // Preamble SF12
  dataRateIndex = 0; // SF12
  spreadingFactor = LoRaWAN::m_supportedDataRates [dataRateIndex].spreadingFactor;
  txConfSucces = phy->SetTxConf (txPower, channelIndex, dataRateIndex, codeRate, preambleLength, implicitHeader, crcOn);
  NS_TEST_ASSERT_MSG_EQ (txConfSucces, true, "Failed to configure LoRa PHY");

  preambleDuration = phy->CalculatePreambleTime ();
  expected = MicroSeconds(401.41e3);
  NS_TEST_ASSERT_MSG_EQ_TOL (preambleDuration, expected, expected/100, "LoRaWANPhy::CalculatePreambleTime() returned " << preambleDuration << ". Expected " << expected);

  // SF12, CR1
  codeRate = 1;
  txConfSucces = phy->SetTxConf (txPower, channelIndex, dataRateIndex, codeRate, preambleLength, implicitHeader, crcOn);
  NS_TEST_ASSERT_MSG_EQ (txConfSucces, true, "Failed to configure LoRa PHY");

  timeOnAir = phy->CalculateTxTime (8);
  expected = MicroSeconds(991.23e3);
  NS_TEST_ASSERT_MSG_EQ_TOL (timeOnAir, expected, expected/100, "LoRaWANPhy::CalculateTxTime() returned " << timeOnAir << ". Expected " << expected << ". " << spreadingFactor << "/" << codeRate);

  timeOnAir = phy->CalculateTxTime (40);
  expected = MicroSeconds(1810.43e3);
  NS_TEST_ASSERT_MSG_EQ_TOL (timeOnAir, expected, expected/100, "LoRaWANPhy::CalculateTxTime() returned " << timeOnAir << ". Expected " << expected << ". " << spreadingFactor << "/" << codeRate);

  // SF12, CR3
  codeRate = 3;
  txConfSucces = phy->SetTxConf (txPower, channelIndex, dataRateIndex, codeRate, preambleLength, implicitHeader, crcOn);
  NS_TEST_ASSERT_MSG_EQ (txConfSucces, true, "Failed to configure LoRa PHY");

  timeOnAir = phy->CalculateTxTime (8);
  expected = MicroSeconds(1122.3e3);
  NS_TEST_ASSERT_MSG_EQ_TOL (timeOnAir, expected, expected/100, "LoRaWANPhy::CalculateTxTime() returned " << timeOnAir << ". Expected " << expected << ". " << spreadingFactor << "/" << codeRate);

  timeOnAir = phy->CalculateTxTime (40);
  expected = MicroSeconds(2269.18e3);
  NS_TEST_ASSERT_MSG_EQ_TOL (timeOnAir, expected, expected/100, "LoRaWANPhy::CalculateTxTime() returned " << timeOnAir << ". Expected " << expected << ". " << spreadingFactor << "/" << codeRate);



  // A wide variety of test macros are available in src/core/test.h
  NS_TEST_ASSERT_MSG_EQ (true, true, "true doesn't equal true for some reason");

  // Use this one for floating point comparisons
  NS_TEST_ASSERT_MSG_EQ_TOL (0.01, 0.01, 0.001, "Numbers are not equal within tolerance");
}

// The TestSuite class names the TestSuite, identifies what type of TestSuite,
// and enables the TestCases to be run.  Typically, only the constructor for
// this class must be defined
//
class LoRaWANPhyTestSuite : public TestSuite
{
public:
  LoRaWANPhyTestSuite ();
};

LoRaWANPhyTestSuite::LoRaWANPhyTestSuite()
  : TestSuite ("lorawan-phy", UNIT)
{
  // TestDuration for TestCase can be QUICK, EXTENSIVE or TAKES_FOREVER
  AddTestCase (new LoRaWANPhyTxTimeTestCase, TestCase::QUICK);
}

// Do not forget to allocate an instance of this TestSuite
static LoRaWANPhyTestSuite lorawanPhyTestSuite;
