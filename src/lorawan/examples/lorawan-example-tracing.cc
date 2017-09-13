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
#include <ns3/network-module.h>
#include <ns3/ipv4-address.h>
#include <ns3/lorawan-module.h>
#include <ns3/propagation-loss-model.h>
#include <ns3/propagation-delay-model.h>
#include <ns3/mobility-module.h>
#include <ns3/applications-module.h>
#include <ns3/simulator.h>
#include <ns3/single-model-spectrum-channel.h>
#include <ns3/constant-position-mobility-model.h>
#include <ns3/node.h>
#include <ns3/packet.h>

#include <ns3/lorawan-spectrum-value-helper.h>
#include <ns3/spectrum-phy.h>
#include <ns3/spectrum-value.h>
#include <ns3/spectrum-model.h>

#include <ns3/netanim-module.h>

#include <iostream>
#include <iomanip>

using namespace ns3;

typedef enum
{
  LORAWAN_DR_CALC_METHOD_PER_INDEX = 0x00,
  LORAWAN_DR_CALC_METHOD_RANDOM_INDEX = 0x01,
  LORAWAN_DR_CALC_METHOD_FIXED_INDEX = 0x02,
} LoRaWANDataRateCalcMethodIndex;

class LoRaWANExampleTracing
{
public:
  LoRaWANExampleTracing ();
  void CaseRun (uint32_t nEndDevices,
               uint32_t nGateways,
               double discRadius,
               double totalTime,
               uint32_t usPacketSize,
               uint32_t usMaxBytes,
               double usDataPeriod,
               uint32_t usUnconfirmedDataNbRep,
               bool usConfirmedData,
               uint32_t dsPacketSize,
               bool dsDataGenerate,
               double dsDataExpMean,
               bool dsConfirmedData,
               bool verbose,
               bool stdcout,
               bool tracePhyTransmissions,
               bool tracePhyStates,
               bool traceMacPackets,
               bool traceMacStates,
               bool traceEdMsgs,
               bool traceNsDsMsgs,
               bool traceMisc,
               std::string phyTransmissionTraceCSVFileName,
               std::string phyStateTraceCSVFileName,
               std::string macPacketTraceCSVFileName,
               std::string macStateTraceCSVFileName,
               std::string edMsgTraceCSVFileName,
               std::string nsDSMsgTraceCSVFileName,
               std::string miscTraceCSVFileName,
               std::string nodesCSVFileName);

  void LogOutputLine (std::string output, std::string);

  std::ostringstream GeneratePhyTraceOutputLine (std::string traceSource, Ptr<LoRaWANNetDevice> device, Ptr<LoRaWANPhy> phy, Ptr<const Packet> packet, bool insertNewLine = true);
  static void PhyTxBegin (LoRaWANExampleTracing* example, Ptr<LoRaWANNetDevice> device, Ptr<LoRaWANPhy> phy, Ptr<const Packet> packet);
  static void PhyTxEnd (LoRaWANExampleTracing* example, Ptr<LoRaWANNetDevice> device, Ptr<LoRaWANPhy> phy, Ptr<const Packet> packet);
  static void PhyTxDrop (LoRaWANExampleTracing* example, Ptr<LoRaWANNetDevice> device, Ptr<LoRaWANPhy> phy, Ptr<const Packet> packet);
  static void PhyRxBegin (LoRaWANExampleTracing* example, Ptr<LoRaWANNetDevice> device, Ptr<LoRaWANPhy> phy, Ptr<const Packet> packet);
  static void PhyRxEnd (LoRaWANExampleTracing* example, Ptr<LoRaWANNetDevice> device, Ptr<LoRaWANPhy> phy, Ptr<const Packet> packet, double lqi);
  static void PhyRxDrop (LoRaWANExampleTracing* example, Ptr<LoRaWANNetDevice> device, Ptr<LoRaWANPhy> phy, Ptr<const Packet> packet, LoRaWANPhyDropRxReason dropReason);

  std::ostringstream GenerateMacTraceOutputLine (std::string traceSource, Ptr<LoRaWANNetDevice> device, Ptr<LoRaWANMac> mac, Ptr<const Packet> packet, bool insertNewLine = true);
  static void MacTx (LoRaWANExampleTracing* example, Ptr<LoRaWANNetDevice> device, Ptr<LoRaWANMac> mac, Ptr<const Packet> packet);
  static void MacTxOk(LoRaWANExampleTracing* example, Ptr<LoRaWANNetDevice> device, Ptr<LoRaWANMac> mac, Ptr<const Packet> packet);
  static void MacTxDrop(LoRaWANExampleTracing* example, Ptr<LoRaWANNetDevice> device, Ptr<LoRaWANMac> mac, Ptr<const Packet> packet);
  static void MacRx(LoRaWANExampleTracing* example, Ptr<LoRaWANNetDevice> device, Ptr<LoRaWANMac> mac, Ptr<const Packet> packet);
  static void MacRxDrop (LoRaWANExampleTracing* example, Ptr<LoRaWANNetDevice> device, Ptr<LoRaWANMac> mac, Ptr<const Packet> packet);
  static void MacSentPkt (LoRaWANExampleTracing* example, Ptr<LoRaWANNetDevice> device, Ptr<LoRaWANMac> mac, Ptr<const Packet> packet, uint8_t n_transmissions);

  static void PhyStateChangeNotification (LoRaWANExampleTracing* example, Ptr<LoRaWANNetDevice> device, Ptr<LoRaWANPhy> phy, LoRaWANPhyEnumeration oldState, LoRaWANPhyEnumeration newState);
  static void MacStateChangeNotification (LoRaWANExampleTracing* example, Ptr<LoRaWANNetDevice> device, Ptr<LoRaWANMac> mac, LoRaWANMacState oldState, LoRaWANMacState newState);

  static void nrRW1SentTrace (LoRaWANExampleTracing* example, uint32_t oldValue, uint32_t newValue);
  static void nrRW2SentTrace (LoRaWANExampleTracing* example, uint32_t oldValue, uint32_t newValue);
  static void nrRW1MissedTrace (LoRaWANExampleTracing* example, uint32_t oldValue, uint32_t newValue);
  static void nrRW2MissedTrace (LoRaWANExampleTracing* example, uint32_t oldValue, uint32_t newValue);

  std::ostringstream GenerateEDMsgTraceOutputLine (std::string traceSource, uint32_t deviceAddress, uint8_t msgType, Ptr<const Packet> packet, bool insertNewLine = true);
  static void USMsgTransmittedTrace (LoRaWANExampleTracing* example, uint32_t deviceAddress, uint8_t msgType, Ptr<const Packet> packet);
  static void DSMsgReceivedTrace (LoRaWANExampleTracing* example, uint32_t deviceAddress, uint8_t msgType, Ptr<const Packet> packet, uint8_t rw);

  std::ostringstream GenerateNSDSMsgTraceOutputLine (std::string traceSource, uint32_t deviceAddress, uint8_t transmissionsRemaning, uint8_t msgType, Ptr<const Packet> packet, bool insertNewLine = true);
  static void DSMsgGeneratedTrace (LoRaWANExampleTracing* example, uint32_t, uint8_t, uint8_t, Ptr<const Packet>);
  static void DSMsgTransmittedTrace (LoRaWANExampleTracing* example, uint32_t, uint8_t, uint8_t, Ptr<const Packet>, uint8_t rw);
  static void DSMsgAckdTrace (LoRaWANExampleTracing* example, uint32_t, uint8_t, uint8_t, Ptr<const Packet>);
  static void DSMsgDroppedTrace (LoRaWANExampleTracing* example, uint32_t, uint8_t, uint8_t, Ptr<const Packet>);
  static void USMsgReceivedTrace (LoRaWANExampleTracing* example, uint32_t deviceAddress, uint8_t msgType, Ptr<const Packet> packet);

  constexpr static const uint8_t m_perPacketSize = 1 + 8 + 8 + 4; // 1B MAC header, 8B frame header, 8 byte payload and 4B MIC
  uint8_t CalculateDataRateIndexPER (Ptr<Application> endDeviceApp);
  uint8_t CalculateRandomDataRateIndex (Ptr<Application> endDeviceApp);
  uint8_t CalculateFixedDataRateIndex (Ptr<Application> endDeviceApp);

  void SelectDRCalculationMethod (LoRaWANDataRateCalcMethodIndex loRaWANDataRateCalcMethodIndex);
  void SetDRCalcPerLimit (double drCalcPerLimit);
  void SetDrCalcFixedDrIndex (uint8_t fixedDataRateIndex);

  void WriteMiscStatsToFile ();
private:
  uint32_t m_nEndDevices;
  uint32_t m_nGateways;
  double m_discRadius;
  double m_totalTime;

  LoRaWANDataRateCalcMethodIndex m_drCalcMethodIndex;
  double m_drCalcPerLimit;
  uint8_t m_fixedDataRateIndex;

  uint32_t m_usPacketSize;
  uint32_t m_usMaxBytes;
  double m_usDataPeriod; // <! Period between subsequent data transmission of end devices
  uint32_t m_usUnconfirmedDataNbRep;
  bool m_usConfirmdData;

  uint32_t m_dsPacketSize;
  bool m_dsDataGenerate;
  double m_dsDataExpMean;
  bool m_dsConfirmedData;

  bool m_verbose;
  bool m_stdcout;
  std::string m_phyTransmissionTraceCSVFileName;
  std::string m_phyStateTraceCSVFileName;
  std::string m_macPacketTraceCSVFileName;
  std::string m_macStateTraceCSVFileName;
  std::string m_edMsgTraceCSVFileName;
  std::string m_nsDSMsgTraceCSVFileName;
  std::string m_miscTraceCSVFileName;
  std::map<std::string, std::ofstream> output_streams;

  std::string m_nodesCSVFileName;

  uint32_t m_nrRW1Sent;
  uint32_t m_nrRW2Sent;
  uint32_t m_nrRW1Missed;
  uint32_t m_nrRW2Missed;

  //std::string m_rate;
  //std::string m_phyMode;
  //uint32_t m_nodeSpeed;
  //uint32_t m_periodicUpdateInterval;
  //uint32_t m_settlingTime;
  //double m_dataStart;
  //uint32_t bytesTotal;
  //uint32_t packetsReceived;
  //bool m_printRoutes;

  NodeContainer m_endDeviceNodes;
  NodeContainer m_gatewayNodes;
  NodeContainer m_allNodes;
  NetDeviceContainer m_EDDevices;
  NetDeviceContainer m_GWDevices;

private:
  void CreateNodes ();
  void SetupMobility ();
  void CreateDevices ();
  void SetupTracing (bool tracePhyTransmissions, bool tracePhyStates, bool traceMacPackets, bool traceMacStates, bool traceEdMsgs, bool traceNsDsMsgs, bool traceMisc);
  void InstallApplications ();
  void OutputNodesToFile ();
};

int main (int argc, char *argv[])
{
  uint32_t randomSeed = 12345;
  uint32_t nEndDevices = 30;
  uint32_t nGateways = 1;
  double discRadius = 5000.0;
  double totalTime = 600.0;
  uint32_t nRuns = 1;
  uint32_t drCalcMethodIndex = 0;
  double drCalcPerLimit = 0.01;
  double drCalcFixedDRIndex = 0;
  uint32_t usPacketSize = 21;
  uint32_t usMaxBytes = 0;
  double usDataPeriod = 600.0;
  uint32_t usUnconfirmedDataNbRep = 1;
  bool usConfirmedData = false;
  uint32_t dsPacketSize = 21;
  bool dsDataGenerate = false;
  double dsDataExpMean = -1;
  bool dsConfirmedData = false;
  bool verbose = false;
  bool stdcout = false;
  bool tracePhyTransmissions = false;
  bool tracePhyStates = false;
  bool traceMacPackets = false;
  bool traceMacStates = false;
  bool traceEdMsgs = false;
  bool traceNsDsMsgs = false;
  bool traceMisc = false;
  std::string outputFileNamePrefix = "output/LoRaWAN-example-tracing";

  CommandLine cmd;
  cmd.AddValue ("randomSeed", "Random seed used in experiments[Default:12345]", randomSeed);
  cmd.AddValue ("nEndDevices", "Number of LoRaWAN class A end devices nodes[Default:30]", nEndDevices);
  cmd.AddValue ("nGateways", "Number of LoRaWAN gateways [Default:1]", nGateways);
  cmd.AddValue ("discRadius", "The radius of the disc (in meters) in which end devices and gateways are placed[Default:5000.0]", discRadius);
  cmd.AddValue ("totalTime", "Simulation time for one run in Seconds[Default:100]", totalTime);
  cmd.AddValue ("nRuns", "Number of simulation runs[Default:100]", nRuns);
  cmd.AddValue ("drCalcMethodIndex", "Method to use for end device Data Rate calculation[Default:0]", drCalcMethodIndex);
  cmd.AddValue ("drCalcPerLimit", "PER limit to use when using the PER Data Rate Calculation method (use index = 0)[Default:0.01]", drCalcPerLimit);
  cmd.AddValue ("drCalcFixedDRIndex", "Fixed Data Rate index to assign when using the Fixed Data Rate Calculation method (use index = 2)[Default:0]", drCalcFixedDRIndex);
  cmd.AddValue ("usPacketSize", "Packet size used for generating US packets[Default:21]", usPacketSize);
  cmd.AddValue ("usMaxBytes", "Maximum number of bytes to be queued for transmission in the upstream direction, note this does not take into account retransmissions. 0 means no limit. [Default:0]", usMaxBytes);
  cmd.AddValue ("usDataPeriod", "Period between subsequent Upstream data transmissions from an end device[Default:600]", usDataPeriod);
  cmd.AddValue ("usUnconfirmedDataNbRep", "Number of repetitions for unconfirmed US data messages[Default:1]", usUnconfirmedDataNbRep);
  cmd.AddValue ("usConfirmedData", "0 for Unconfirmed Upstream Data MAC packets, 1 for Confirmed Upstream Data MAC Packets[Default:0]", usConfirmedData);
  cmd.AddValue ("dsPacketSize", "Packet size used for generating DS packets[Default:21]", dsPacketSize);
  cmd.AddValue ("dsDataGenerate", "Should NS generate DS traffic for end devices, note that Acks are always sent.[Default:0]", dsDataGenerate);
  cmd.AddValue ("dsDataExpMean", "Mean for the Exponential random variable for inter packet time for DS transmission for an end device[Default:10*usDataPeriod]", dsDataExpMean);
  cmd.AddValue ("dsConfirmedData", "0 for Unconfirmed Downstream Data MAC packets, 1 for Confirmed Downstream Data MAC Packets[Default:0]", dsConfirmedData);
  cmd.AddValue ("verbose", "turn on all log components[Default:0]", verbose);
  cmd.AddValue ("stdcout", "Print output to std::cout[Default:0]", stdcout);
  cmd.AddValue ("tracePhyTransmissions", "Trace PHY transmissions[Default:0]", tracePhyTransmissions);
  cmd.AddValue ("tracePhyStates", "Trace PHY states[Default:0]", tracePhyStates);
  cmd.AddValue ("traceMacPackets", "Trace MAC packets[Default:0]", traceMacPackets);
  cmd.AddValue ("traceMacStates", "Trace MAC states[Default:0]", traceMacStates);
  cmd.AddValue ("traceEDMsgs", "Trace messages on end devices[Default:0]", traceEdMsgs);
  cmd.AddValue ("traceNSDSMsgs", "Trace NS downstream messages[Default:0]", traceNsDsMsgs);
  cmd.AddValue ("traceMisc", "Trace miscellanous stats[Default:0]", traceMisc);
  cmd.AddValue ("outputFileNamePrefix", "The prefix for the names of the output files[Default:output/LoRaWAN-example-tracing]", outputFileNamePrefix);
  //cmd.AddValue ("phyMode", "Wifi Phy mode[Default:DsssRate11Mbps]", phyMode);
  //cmd.AddValue ("rate", "CBR traffic rate[Default:8kbps]", rate);
  //cmd.AddValue ("nodeSpeed", "Node speed in RandomWayPoint model[Default:10]", nodeSpeed);
  //cmd.AddValue ("periodicUpdateInterval", "Periodic Interval Time[Default=15]", periodicUpdateInterval);
  //cmd.AddValue ("settlingTime", "Settling Time before sending out an update for changed metric[Default=6]", settlingTime);
  //cmd.AddValue ("dataStart", "Time at which nodes start to transmit data[Default=50.0]", dataStart);
  //cmd.AddValue ("printRoutingTable", "print routing table for nodes[Default:1]", printRoutingTable);
  cmd.Parse (argc, argv);

  if (dsDataExpMean == -1)
    dsDataExpMean = 10 * usDataPeriod; // set default value for dsDataExpMean

  LoRaWANDataRateCalcMethodIndex loRaWANDataRateCalcMethodIndex;
  if (drCalcMethodIndex == 0)
    loRaWANDataRateCalcMethodIndex = LORAWAN_DR_CALC_METHOD_PER_INDEX;
  else if (drCalcMethodIndex == 1)
    loRaWANDataRateCalcMethodIndex = LORAWAN_DR_CALC_METHOD_RANDOM_INDEX;
  else if (drCalcMethodIndex == 2)
    loRaWANDataRateCalcMethodIndex = LORAWAN_DR_CALC_METHOD_FIXED_INDEX;
  else {
    std::cout << " Invalid Data Rate Calculcation Method index provided: " << (uint32_t)drCalcMethodIndex << std::endl;
    return -1;
  }

  std::time_t unix_epoch = std::time(nullptr);
  for (uint32_t i = 0; i < nRuns; i++) {
    uint32_t seed = randomSeed + i;
    SeedManager::SetSeed (seed);

    std::ostringstream simRunFilesPrefix;
    simRunFilesPrefix << outputFileNamePrefix << "-" << unix_epoch << "-" << std::to_string(i);

    // prep csv files:
    std::ostringstream phyTransmissionTraceCSVFileName;
    phyTransmissionTraceCSVFileName << simRunFilesPrefix.str() << "-trace-phy-tx.csv";
    if (tracePhyTransmissions) {
      std::ofstream out (phyTransmissionTraceCSVFileName.str ().c_str ());
      out << "Time," <<
        "DeviceType," <<
        "NodeId," <<
        "IfIndex," <<
        "PhyIndex," <<
        "TraceSource," <<
        "PhyTraceIdTag," <<
        "PacketHex," <<
        "PacketLength," <<
        "Miscellaneous" <<
        std::endl;
      out.close ();
    }

    std::ostringstream phyStateTraceCSVFileName;
    phyStateTraceCSVFileName << simRunFilesPrefix.str() << "-trace-phy-state.csv";
    if (tracePhyStates) {
      std::ofstream out2 (phyStateTraceCSVFileName.str ().c_str ());
      out2 << "Time," <<
        "DeviceType," <<
        "NodeId," <<
        "IfIndex," <<
        "PhyIndex," <<
        "TraceSource," <<
        "OldState," <<
        "NewState" <<
        std::endl;
      out2.close ();
    }

    std::ostringstream macPacketTraceCSVFileName;
    macPacketTraceCSVFileName << simRunFilesPrefix.str() << "-trace-mac-packets.csv";
    if (traceMacPackets) {
      std::ofstream out (macPacketTraceCSVFileName.str ().c_str ());
      // TODO
      out << "Time," <<
        "DeviceType," <<
        "NodeId," <<
        "IfIndex," <<
        "PhyIndex," <<
        "TraceSource," <<
        "PhyTraceIdTag," <<
        "PacketHex," <<
        "PacketLength," <<
        "Miscellaneous" <<
        std::endl;
      out.close ();
    }

    std::ostringstream macStateTraceCSVFileName;
    macStateTraceCSVFileName << simRunFilesPrefix.str() << "-trace-mac-state.csv";
    if (traceMacStates) {
      std::ofstream out3 (macStateTraceCSVFileName.str ().c_str ());
      out3 << "Time," <<
        "DeviceType," <<
        "NodeId," <<
        "IfIndex," <<
        "MacIndex," <<
        "TraceSource," <<
        "OldState," <<
        "NewState" <<
        std::endl;
      out3.close ();
    }

    std::ostringstream edMsgTraceCSVFileName;
    edMsgTraceCSVFileName << simRunFilesPrefix.str() << "-trace-ed-msgs.csv";
    if (traceEdMsgs) {
      std::ofstream outed (edMsgTraceCSVFileName.str ().c_str ());
      outed << "Time," <<
        "TraceSource," <<
        "NodeId," <<
        "MsgType," <<
        "PacketHex," <<
        "PacketLength" <<
        std::endl;
      outed.close ();
    }

    std::ostringstream nsDSMsgTraceCSVFileName;
    nsDSMsgTraceCSVFileName << simRunFilesPrefix.str() << "-trace-ns-dsmsgs.csv";
    if (traceNsDsMsgs) {
      std::ofstream outns (nsDSMsgTraceCSVFileName.str ().c_str ());
      outns << "Time," <<
        "TraceSource," <<
        "NodeId," <<
        "MsgType," <<
        "TxRemaining," <<
        "PacketHex," <<
        "PacketLength" <<
        std::endl;
      outns.close ();
    }

    std::ostringstream miscTraceCSVFileName;
    miscTraceCSVFileName << simRunFilesPrefix.str() << "-trace-misc.csv";
    if (traceMisc) {
      std::ofstream out4 (miscTraceCSVFileName.str ().c_str ());
      out4 << "nrRW1Sent," <<
        "nrRW2Sent," <<
        "nrRW1Missed," <<
        "nrRW2Missed" <<
        std::endl;
      out4.close ();
    }

    std::ostringstream nodesCSVFileName;
    nodesCSVFileName << simRunFilesPrefix.str() << "-nodes.csv";
    std::ofstream out5 (nodesCSVFileName.str ().c_str ());
    out5 << "DeviceType," <<
      "NodeId," <<
      "Position x," <<
      "Position y," <<
      "DistanceToClosestGW," <<
      "DataRateIndex" <<
      std::endl;
    out5.close ();

    std::cout << std::endl;

    std::ostringstream simSettings;
    simSettings << "Simulation settings:" << std::endl;
    simSettings << "\trandomSeed = " << randomSeed << std::endl;
    simSettings << "\tnEndDevices = " << nEndDevices << std::endl;
    simSettings << "\tnGateways = " << nGateways << std::endl;
    simSettings << "\tdiscRadius = " << discRadius << std::endl;
    simSettings << "\ttotalTime = " << totalTime << std::endl;
    simSettings << "\tnRuns = " << nRuns << std::endl;
    simSettings << "\tusPacketSize = " << usPacketSize << std::endl;
    simSettings << "\tusMaxBytes = " << usMaxBytes << std::endl;
    simSettings << "\tusDataPeriod = " << usDataPeriod << std::endl;
    simSettings << "\tusUnconfirmedDataNbRep = " << usUnconfirmedDataNbRep << std::endl;
    simSettings << "\tusConfirmedData = " << usConfirmedData << std::endl;
    simSettings << "\tdsPacketSize = " << dsPacketSize << std::endl;
    simSettings << "\tdsDataGenerate = " << dsDataGenerate << std::endl;
    simSettings << "\tdsDataExpMean = " << dsDataExpMean << std::endl;
    simSettings << "\tdsConfirmedData = " << dsConfirmedData << std::endl;
    simSettings << "\tverbose = " << verbose << std::endl;
    simSettings << "\tstdcout = " << stdcout << std::endl;
    simSettings << "\ttracePhyTransmissions = " << tracePhyTransmissions << std::endl;
    simSettings << "\ttracePhyStates = " << tracePhyStates << std::endl;
    simSettings << "\ttraceMacPackets = " << traceMacPackets << std::endl;
    simSettings << "\ttraceMacStates = " << traceMacStates << std::endl;
    simSettings << "\ttraceEdMsgs = " << traceEdMsgs << std::endl;
    simSettings << "\ttraceNsDsMsgs = " << traceNsDsMsgs << std::endl;
    simSettings << "\ttraceMisc = " << traceMisc << std::endl;
    simSettings << "\toutputFileNamePrefix = " << outputFileNamePrefix << std::endl;
    simSettings << "\trun = " << i << std::endl;
    simSettings << "\tseed = " << seed << std::endl;
    simSettings << "\tphyTransmissionTraceCSVFileName = " << phyTransmissionTraceCSVFileName.str() << std::endl;
    simSettings << "\tphyStateTraceCSVFileName = " << phyStateTraceCSVFileName.str() << std::endl;
    simSettings << "\tmacPacketTraceCSVFileName = " << macPacketTraceCSVFileName.str() << std::endl;
    simSettings << "\tmacStateTraceCSVFileName = " << macStateTraceCSVFileName.str() << std::endl;
    simSettings << "\tedMsgTraceCSVFileName = " << edMsgTraceCSVFileName.str() << std::endl;
    simSettings << "\tnsDSMsgTraceCSVFileName = " << nsDSMsgTraceCSVFileName.str() << std::endl;
    simSettings << "\tmiscTraceCSVFileName = " << miscTraceCSVFileName.str() << std::endl;
    simSettings << "\tnodesCSVFileName = " << nodesCSVFileName.str() << std::endl;
    simSettings << "\tData rate assignment method index: " << loRaWANDataRateCalcMethodIndex;
    if (loRaWANDataRateCalcMethodIndex == LORAWAN_DR_CALC_METHOD_PER_INDEX)
      simSettings << ", PER limit = " << drCalcPerLimit << ", PER Packet size = " << (unsigned)LoRaWANExampleTracing::m_perPacketSize << " bytes";
    else if (loRaWANDataRateCalcMethodIndex == LORAWAN_DR_CALC_METHOD_FIXED_INDEX)
      simSettings << ", Fixed Data Rate Index = " << drCalcFixedDRIndex;
    simSettings  << std::endl;

    // print settings to std cout
    std::cout << simSettings.str();
    // write settings to file:
    std::ostringstream simSettingsFileName;
    simSettingsFileName << simRunFilesPrefix.str() << "-sim-settings.txt";
    std::ofstream out6 (simSettingsFileName.str ().c_str ());
    out6 << simSettings.str();
    out6.close ();

    // Start sim run:
    LoRaWANExampleTracing example = LoRaWANExampleTracing ();
    example.SelectDRCalculationMethod (loRaWANDataRateCalcMethodIndex);
    example.SetDRCalcPerLimit (drCalcPerLimit);
    example.SetDrCalcFixedDrIndex (drCalcFixedDRIndex);
    example.CaseRun (nEndDevices, nGateways, discRadius, totalTime,
        usPacketSize, usMaxBytes, usDataPeriod, usUnconfirmedDataNbRep, usConfirmedData,
        dsPacketSize, dsDataGenerate, dsDataExpMean, dsConfirmedData,
        verbose, stdcout, tracePhyTransmissions, tracePhyStates, traceMacPackets, traceMacStates, traceEdMsgs, traceNsDsMsgs, traceMisc,
        phyTransmissionTraceCSVFileName.str (), phyStateTraceCSVFileName.str (), macPacketTraceCSVFileName.str (), macStateTraceCSVFileName.str (), edMsgTraceCSVFileName.str(), nsDSMsgTraceCSVFileName.str(),  miscTraceCSVFileName.str(), nodesCSVFileName.str());
  }

  return 0;
}

LoRaWANExampleTracing::LoRaWANExampleTracing () : m_nrRW1Sent(0), m_nrRW2Sent(0), m_nrRW1Missed(0), m_nrRW2Missed(0) {}

void
LoRaWANExampleTracing::CaseRun (uint32_t nEndDevices, uint32_t nGateways, double discRadius, double totalTime,
    uint32_t usPacketSize, uint32_t usMaxBytes, double usDataPeriod, uint32_t usUnconfirmedDataNbRep, bool usConfirmedData,
    uint32_t dsPacketSize, bool dsDataGenerate, double dsDataExpMean, bool dsConfirmedData,
    bool verbose, bool stdcout, bool tracePhyTransmissions, bool tracePhyStates, bool traceMacPackets, bool traceMacStates, bool traceEdMsgs, bool traceNsDsMsgs, bool traceMisc,
    std::string phyTransmissionTraceCSVFileName, std::string phyStateTraceCSVFileName, std::string macPacketTraceCSVFileName, std::string macStateTraceCSVFileName, std::string edMsgTraceCSVFileName, std::string nsDSMsgTraceCSVFileName, std::string miscTraceCSVFileName, std::string nodesCSVFileName)
{
  m_nEndDevices = nEndDevices;
  m_nGateways = nGateways;
  m_discRadius = discRadius;
  m_totalTime = totalTime;

  m_usPacketSize = usPacketSize;
  m_usMaxBytes = usMaxBytes;
  m_usDataPeriod = usDataPeriod;
  m_usUnconfirmedDataNbRep = usUnconfirmedDataNbRep;
  m_usConfirmdData = usConfirmedData;

  m_dsDataGenerate = dsDataGenerate;
  m_dsPacketSize = dsPacketSize;
  m_dsDataExpMean = dsDataExpMean;
  m_dsConfirmedData = dsConfirmedData;

  m_verbose = verbose;
  m_stdcout = stdcout;

  m_phyTransmissionTraceCSVFileName = phyTransmissionTraceCSVFileName;
  m_phyStateTraceCSVFileName = phyStateTraceCSVFileName;
  m_macPacketTraceCSVFileName = macPacketTraceCSVFileName;
  m_macStateTraceCSVFileName = macStateTraceCSVFileName;
  m_edMsgTraceCSVFileName = edMsgTraceCSVFileName;
  m_nsDSMsgTraceCSVFileName = nsDSMsgTraceCSVFileName;

  m_miscTraceCSVFileName = miscTraceCSVFileName;
  m_nodesCSVFileName = nodesCSVFileName;

  CreateNodes ();
  SetupMobility (); // important: setup mobility before creating devices
  CreateDevices ();
  InstallApplications ();

  SetupTracing (tracePhyTransmissions, tracePhyStates, traceMacPackets, traceMacStates, traceEdMsgs, traceNsDsMsgs, traceMisc);
  OutputNodesToFile ();

  std::cout << "Starting simulation for " << m_totalTime << " s ...\n";

  Simulator::Stop (Seconds (m_totalTime));

  // netamin:
  AnimationInterface anim ("lorawan-netamin.xml");
  // stationary nodes: "You should assign the ConstantPositionMobilityModel. Constant Position is a kind of mobility."
  //for (NodeContainer::Iterator i = m_allNodes.Begin (); i != m_allNodes.End (); ++i)
  //{
  //  Ptr<Node> node = *i;
  //  //Ptr<LoRaWANNetDevice> netDevice = DynamicCast<LoRaWANNetDevice> (node->GetDevice (0));
  //  Ptr<MobilityModel> nodeMobility = node->GetObject<MobilityModel> ();
  //  NS_ASSERT (nodeMobility);
  //  Vector position = nodeMobility->GetPosition ();

  //  anim.SetConstantPosition (node, position.x, position.y);
  //}
  anim.EnablePacketMetadata (true);

  Simulator::Run ();

  if (traceMisc) // write after simulation has ended
    WriteMiscStatsToFile ();

  Simulator::Destroy ();
}

void
LoRaWANExampleTracing::CreateNodes ()
{
  std::cout << "Creating " << (unsigned) m_nGateways << " LoRaWAN gateway(s).\n";
  m_gatewayNodes.Create (m_nGateways);

  std::cout << "Creating " << (unsigned) m_nEndDevices << " LoRaWAN class A end devices.\n";
  m_endDeviceNodes.Create (m_nEndDevices);

  m_allNodes.Add (m_gatewayNodes);
  m_allNodes.Add (m_endDeviceNodes);
}

void
LoRaWANExampleTracing::SetupMobility ()
{
  MobilityHelper edMobility;
  edMobility.SetPositionAllocator ("ns3::UniformDiscPositionAllocator",
                                    "X", DoubleValue (0.0),
                                    "Y", DoubleValue (0.0),
                                    "rho", DoubleValue (m_discRadius));
  edMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  edMobility.Install (m_endDeviceNodes);

  MobilityHelper gwMobility;
  Ptr<ListPositionAllocator> nodePositionList = CreateObject<ListPositionAllocator> ();
  for (uint32_t i = 0; i < m_nGateways; i++) {
    Vector v;
    if (m_nGateways == 1) {
      v = Vector (0.0, 0.0, 0.0); // place gateway in center
    } else if (m_nGateways == 2) {
      v = Vector (-m_discRadius/2 + i*m_discRadius, 0.0, 0.0);  // place gateways on (-discRadius/2, 0) and (discRadius/2, 0)
    } else if (m_nGateways == 4) {
      // place gateways on (-discRadius/2, -discRadius/2) and (discRadius/2, -discRadius/2) and (-discRadius/2, discRadius/2) and (discRadius/2, discRadius/2)
      v = Vector (-m_discRadius/2 + (i/2)*m_discRadius, -m_discRadius/2 + (i % 2)*m_discRadius, 0.0);
    // } else if (m_nGateways == 9) {
    //   if (i < 4) {
    //     // place four gateways on (-discRadius/2, -discRadius/2) and (discRadius/2, -discRadius/2) and (-discRadius/2, discRadius/2) and (discRadius/2, discRadius/2)
    //     v = Vector (-m_discRadius/2 + (i/2)*m_discRadius, -m_discRadius/2 + (i % 2)*m_discRadius, 0.0);
    //   } else if (i == 4) {
    //     // place one gateways in center
    //     v = Vector (0.0, 0.0, 0.0); // place gateway in center
    //   } else {
    //     // place four gateways on (-discRadius, 0) and (discRadius, 0) and (0, discRadius) and (0, -discRadius)
    //     if (i == 5) {
    //       v = Vector (-m_discRadius, 0.0, 0.0);
    //     } else if (i == 6) {
    //       v = Vector (m_discRadius, 0.0, 0.0);
    //     } else if (i == 7) {
    //       v = Vector (0.0, m_discRadius, 0.0);
    //     } else if (i == 8) {
    //       v = Vector (0.0, -m_discRadius, 0.0);
    //     }
    //   }
    } else {
      NS_FATAL_ERROR (this << " " << m_nGateways << " number of gateways is not supported");
      exit (-1);
    }

    nodePositionList->Add (v);
  }

  gwMobility.SetPositionAllocator (nodePositionList);
  gwMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  gwMobility.Install (m_gatewayNodes);
}

void
LoRaWANExampleTracing::CreateDevices ()
{
  LoRaWANHelper lorawanHelper;
  if (m_verbose)
    lorawanHelper.EnableLogComponents (LOG_LEVEL_INFO);
  else
    lorawanHelper.EnableLogComponents (LOG_LEVEL_WARN);

  lorawanHelper.SetNbRep (m_usUnconfirmedDataNbRep);
  m_EDDevices = lorawanHelper.Install (m_endDeviceNodes);

  lorawanHelper.SetDeviceType (LORAWAN_DT_GATEWAY);
  m_GWDevices = lorawanHelper.Install (m_gatewayNodes);
}

void
LoRaWANExampleTracing::InstallApplications ()
{
  // PacketSocketHelper::Install adds a PacketSocketFactory object as an aggregate object to each of the nodes in the NodeContainer
  // This factory object is needed when applications create sockets in StartApplication
  PacketSocketHelper packetSocket;
  packetSocket.Install (m_endDeviceNodes); // TODO: replace two lines with m_allNodes?
  packetSocket.Install (m_gatewayNodes);

  // Gateways and Network Server:
  ApplicationContainer gatewayApps;
  ObjectFactory m_factory2; //!< Object factory.
  m_factory2.SetTypeId ("ns3::LoRaWANGatewayApplication");
  for (NodeContainer::Iterator i = m_gatewayNodes.Begin (); i != m_gatewayNodes.End (); ++i)
    {
      Ptr<Node> node = *i;
      Ptr<Application> app = m_factory2.Create<Application> ();
      node->AddApplication (app);
      gatewayApps.Add (app);
      //Ptr<LoRaWANGatewayApplication> gwApp = DynamicCast<LoRaWANGatewayApplication> (app);
    }

  gatewayApps.Start (Seconds (0.0));
  gatewayApps.Stop (Seconds (m_totalTime));

  // Set attributes on LoRaWANNetworkServer object:
  Ptr<LoRaWANNetworkServer> lorawanNSPtr = LoRaWANNetworkServer::getLoRaWANNetworkServerPointer ();
  NS_ASSERT (lorawanNSPtr);
  if (lorawanNSPtr) {
    lorawanNSPtr->SetAttribute ("PacketSize", UintegerValue (m_dsPacketSize));
    lorawanNSPtr->SetAttribute ("GenerateDataDown", BooleanValue (m_dsDataGenerate));
    lorawanNSPtr->SetAttribute ("ConfirmedDataDown", BooleanValue (m_dsConfirmedData));
    std::stringstream downstreamiatss;
    downstreamiatss << "ns3::ExponentialRandomVariable[Mean=" << m_dsDataExpMean << "]";
    lorawanNSPtr->SetAttribute ("DownstreamIAT", StringValue (downstreamiatss.str ()));
  }

  // End devices:
  ApplicationContainer endDeviceApp;
  ObjectFactory m_factory; //!< Object factory.
  m_factory.SetTypeId ("ns3::LoRaWANEndDeviceApplication");
  // Attributes shared by all end device apps:
  m_factory.Set ("PacketSize", UintegerValue (m_usPacketSize));
  m_factory.Set ("MaxBytes", UintegerValue (m_usMaxBytes));
  m_factory.Set ("ConfirmedDataUp", BooleanValue (m_usConfirmdData));
  std::stringstream upstreamiatss;
  upstreamiatss << "ns3::ConstantRandomVariable[Constant=" << m_usDataPeriod << "]";
  m_factory.Set ("UpstreamIAT", StringValue (upstreamiatss.str ()));
  // Limit the number of channels
  m_factory.Set ("ChannelRandomVariable", StringValue ("ns3::ConstantRandomVariable[Constant=0.0]"));

  Ptr<UniformRandomVariable> appStartRandomVariable = CreateObject<UniformRandomVariable> ();
  for (NodeContainer::Iterator i = m_endDeviceNodes.Begin (); i != m_endDeviceNodes.End (); ++i)
  {
      Ptr<Node> node = *i;
      Ptr<Application> app = m_factory.Create<Application> ();
      node->AddApplication (app);

      // Start the end device apps at different times:
      double appStartRandomValue = appStartRandomVariable->GetValue (0, m_usDataPeriod);
      app->SetStartTime (Seconds (appStartRandomValue));
      app->SetStopTime(Seconds (m_totalTime));

      endDeviceApp.Add (app);
  }

  // Assign data rate indexes to end device applications:
  for (ApplicationContainer::Iterator aci = endDeviceApp.Begin (); aci != endDeviceApp.End (); ++aci)
    {
      Ptr<Application> app = *aci;
      uint8_t dataRateIndex = 0;
      if (m_drCalcMethodIndex == LORAWAN_DR_CALC_METHOD_PER_INDEX)
        dataRateIndex = CalculateDataRateIndexPER (app); // assign data rates based on PER vs distance
      else if (m_drCalcMethodIndex == LORAWAN_DR_CALC_METHOD_RANDOM_INDEX)
        dataRateIndex = CalculateRandomDataRateIndex(app); // assign random data rates
      else if (m_drCalcMethodIndex == LORAWAN_DR_CALC_METHOD_FIXED_INDEX)
        dataRateIndex = CalculateFixedDataRateIndex (app); // use a fixed data rate

      app->SetAttribute ("DataRateIndex", UintegerValue(dataRateIndex));
    }
}

void
LoRaWANExampleTracing::OutputNodesToFile ()
{
  std::ofstream outNodes (m_nodesCSVFileName.c_str (), std::ios::app);
  // Output For Gateways: nodeid, device type, x, y
  for (NodeContainer::Iterator i = m_gatewayNodes.Begin (); i != m_gatewayNodes.End (); ++i)
  {
    Ptr<Node> node = *i;
    Ptr<LoRaWANNetDevice> netDevice = DynamicCast<LoRaWANNetDevice> (node->GetDevice (0));
    Ptr<MobilityModel> gatewayMobility = node->GetObject<MobilityModel> ();
    NS_ASSERT (gatewayMobility);
    Vector gatewayPos = gatewayMobility->GetPosition ();

    outNodes
      << node->GetId () << ","
      << netDevice->GetDeviceType () << ","
      << gatewayPos.x << ","
      << gatewayPos.y << std::endl;
  }

  // Output For End Devices: nodeid, device type, x, y, distancetoclosestgateway, datarateindex
  for (NodeContainer::Iterator i = m_endDeviceNodes.Begin (); i != m_endDeviceNodes.End (); ++i)
  {
    Ptr<Node> node = *i;
    Ptr<LoRaWANNetDevice> netDevice = DynamicCast<LoRaWANNetDevice> (node->GetDevice (0));

    Ptr<MobilityModel> endDeviceMobility = node->GetObject<MobilityModel> ();
    NS_ASSERT (endDeviceMobility);
    Vector endDevicePos = endDeviceMobility->GetPosition ();

    // find distance to the closest gateway:
    Vector closestGwPos; // to be determined
    double dmin = std::numeric_limits<double>::max ();
    {
    for (NodeContainer::Iterator it_gw = m_gatewayNodes.Begin (); it_gw != m_gatewayNodes.End (); ++it_gw)
    {
      Ptr<MobilityModel> gatewayMobility = (*it_gw)->GetObject<MobilityModel> ();
      NS_ASSERT (gatewayMobility);
      Vector gwPos = gatewayMobility->GetPosition ();

      double distance = CalculateDistance (endDevicePos, gwPos);
      if (distance < dmin) {
        dmin = distance;
        closestGwPos = gwPos;
      }
    }
    }

    Ptr<LoRaWANEndDeviceApplication> edApp = node->GetApplication (0)->GetObject<LoRaWANEndDeviceApplication> ();
    uint8_t dataRateIndex = edApp->GetDataRateIndex ();

    outNodes
      << node->GetId () << ","
      << netDevice->GetDeviceType () << ","
      << endDevicePos.x << ","
      << endDevicePos.y << ","
      << dmin << ","
      << (uint32_t)dataRateIndex << std::endl;
  }
  outNodes.close ();
}

void
LoRaWANExampleTracing::SelectDRCalculationMethod (LoRaWANDataRateCalcMethodIndex loRaWANDataRateCalcMethodIndex)
{
  if (loRaWANDataRateCalcMethodIndex == 0)
    this->m_drCalcMethodIndex = LORAWAN_DR_CALC_METHOD_PER_INDEX;
  else if (loRaWANDataRateCalcMethodIndex == 1)
    this->m_drCalcMethodIndex = LORAWAN_DR_CALC_METHOD_RANDOM_INDEX;
  else if (loRaWANDataRateCalcMethodIndex == 2)
    this->m_drCalcMethodIndex = LORAWAN_DR_CALC_METHOD_FIXED_INDEX;
  else
    std::cout << " Invalid Data Rate Calculcation Method index provided: " << (unsigned)loRaWANDataRateCalcMethodIndex << std::endl;
}

void
LoRaWANExampleTracing::SetDRCalcPerLimit (double drCalcPerLimit)
{
  NS_ASSERT (drCalcPerLimit >= 0.0 && drCalcPerLimit <= 1.0);
  if (drCalcPerLimit >= 0.0 && drCalcPerLimit <= 1.0)
    this->m_drCalcPerLimit = drCalcPerLimit;
  else
    std::cout << " Invalid Data Rate PER Calculcation PER limit provided: " << drCalcPerLimit << std::endl;
}

void
LoRaWANExampleTracing::SetDrCalcFixedDrIndex (uint8_t fixedDataRateIndex)
{
  if (fixedDataRateIndex < LoRaWAN::m_supportedDataRates.size() - 1)
    this->m_fixedDataRateIndex = fixedDataRateIndex;
  else {
    std::cout << "LoRaWANExampleTracing::SetDrCalcFixedDrIndex: Invalid DR index provided: " << (uint32_t)fixedDataRateIndex << std::endl;
    exit(-1);
  }
}

uint8_t
LoRaWANExampleTracing::CalculateDataRateIndexPER (Ptr<Application> endDeviceApp)
{
  // here we calculate the data rate index based on the distance between the
  // end device and the nearest gateway (the closer the node, the lower the
  // distance, the lower the path loss,  the higher the SINR, which allows for
  // a higher data rate)
  // we assign data rates so that the Packet Error Rate is below perLimit for a
  // perPacketSize packet sent at the position of the end device to the closest gateway
  const Ptr<Node> endDeviceNode = endDeviceApp->GetNode ();

  // !!!Important!!! we assume that the experiment is using only the LogDistancePropagationLossModel here
  Ptr<PropagationLossModel> lossModel = CreateObject<LogDistancePropagationLossModel> (); // TODO: can we get the loss model from the end device node object ?
  Ptr<MobilityModel> endDeviceMobility = endDeviceNode->GetObject<MobilityModel> ();
  NS_ASSERT (endDeviceMobility);

  // find lowest path loss (or highest rx power) for all gateways
  double maxRxPowerdBm = std::numeric_limits<double>::lowest (); // note: numeric_limits::min() returns minimum positive normalized value for floating point types
  Vector posGw;
  for (NodeContainer::Iterator it = m_gatewayNodes.Begin (); it != m_gatewayNodes.End (); ++it)
  {
    Ptr<MobilityModel> gatewayMobility = (*it)->GetObject<MobilityModel> ();
    NS_ASSERT (gatewayMobility);

    const double rxPowerdBm = lossModel->CalcRxPower (14.0, endDeviceMobility, gatewayMobility); // 14.0 dBm TX power as this is max power for most EU868 sub bands

    if (rxPowerdBm > maxRxPowerdBm) {
      maxRxPowerdBm = rxPowerdBm;
      posGw = gatewayMobility->GetPosition ();
    }
  }


  LoRaWANSpectrumValueHelper psdHelper;
  const uint32_t freq = LoRaWAN::m_supportedChannels [0].m_fc;
  Ptr<const SpectrumValue> noise = psdHelper.CreateNoisePowerSpectralDensity (freq);

  double maxRxPowerLinear = pow (10.0, maxRxPowerdBm / 10.0) / 1000.0; // in Watts
  const double noisePowerLinear = LoRaWANSpectrumValueHelper::TotalAvgPower (noise, freq); // in Watts
  double snrDb = 10.0 * log10 (maxRxPowerLinear/noisePowerLinear);

  /*
  std::cout << "noisePowerLinear equal to " << noisePowerLinear  << "W" << std::endl;
  std::cout << "maxRxPowerLinear equal to " << maxRxPowerLinear  << "W" << std::endl;
  std::cout << "noise power equal to " << 10.0 * log10 (noisePowerLinear) + 30 << "dBm" << std::endl;
  std::cout << "maxRxPowerDbm equal to " << maxRxPowerdBm  << "dBm" << std::endl;
  std::cout << "snrDb equal to " << snrDb  << "dB" << std::endl;
  */

  // For minimum path loss, search data rate so that PER is <= perLimit for a perPacketSize length packet
  // go from highest to lowest data rate, when PER drops below perLimit then we have found the data rate for the end device
  Ptr <LoRaWANErrorModel> errorModel = CreateObject<LoRaWANErrorModel> ();
  uint32_t nbits = 8*m_perPacketSize;
  uint8_t codeRate = 3;
  uint8_t calculatedDataRateIndex = LoRaWAN::m_supportedDataRates[0].dataRateIndex; // SF12 will be default SF

  bool hitPerLimit = false; // check whether we actually get a per below the perLimit
  double perForDR = 0.0;
  auto it = LoRaWAN::m_supportedDataRates.rbegin(); // iterate in reverse, so that we start at SF7 and move upto SF12
  it++; // skip the 250kHz data rate
  for (; it != LoRaWAN::m_supportedDataRates.rend (); it++)
  {
    uint32_t bandWidth = it->bandWith;
    LoRaSpreadingFactor sf = it->spreadingFactor;
    perForDR = 1.0 - errorModel->GetChunkSuccessRate (snrDb, nbits, bandWidth, sf, codeRate);
    NS_ASSERT (perForDR >= 0.0 && perForDR <= 1.0);

    if (perForDR <= m_drCalcPerLimit) {
      calculatedDataRateIndex = it->dataRateIndex;
      hitPerLimit = true;
      break;
    }
  }

  Vector endDevicePos = endDeviceMobility->GetPosition ();
  double distance = CalculateDistance (endDeviceMobility->GetPosition (), posGw);

  if (m_verbose) {
    std::cout << "CalculateDataRateIndexPER: node " << endDeviceNode->GetId () << "\tDRindex = " << (unsigned int)calculatedDataRateIndex
      << "\tSF=" << (unsigned int)LoRaWAN::m_supportedDataRates[calculatedDataRateIndex].spreadingFactor
      << "\tmaxRxPowerdBm = " << maxRxPowerdBm << "dBm\tsnrDb = " << snrDb << "dB" << "\tpos=(" << endDevicePos.x << "," << endDevicePos.y << ")\td=" << distance << "m\tPER=" << std::setprecision(5) << perForDR*100 << "%" << std::endl;

    if (!hitPerLimit) { // this is just for debugging purposes
      std::cout << "CalculateDataRateIndexPER: failed to find a data rate where the PER is lower than " << m_drCalcPerLimit << "."
        << "Last calculated PER was equal to " << std::setiosflags (std::ios::fixed) << std::setprecision (9) << perForDR << std::endl;
    }
  }

  return calculatedDataRateIndex;
}

uint8_t
LoRaWANExampleTracing::CalculateRandomDataRateIndex (Ptr<Application> endDeviceApp)
{
  const Ptr<Node> endDeviceNode = endDeviceApp->GetNode ();

  Ptr<UniformRandomVariable> dataRateRandomVariable = CreateObject<UniformRandomVariable> ();
  uint8_t calculatedDataRateIndex = dataRateRandomVariable->GetInteger (0, LoRaWAN::m_supportedDataRates.size() - 2);
  std::cout << "CalculateRandomDataRateIndex: node " << endDeviceNode->GetId () << "\tDRindex = " << (unsigned int)calculatedDataRateIndex
    << "\tSF=" << (unsigned int)LoRaWAN::m_supportedDataRates[calculatedDataRateIndex].spreadingFactor << std::endl;

  return calculatedDataRateIndex;
}

uint8_t
LoRaWANExampleTracing::CalculateFixedDataRateIndex (Ptr<Application> endDeviceApp)
{
  const Ptr<Node> endDeviceNode = endDeviceApp->GetNode ();

  uint8_t calculatedDataRateIndex = m_fixedDataRateIndex;
  if (m_verbose) {
    std::cout << "CalculateFixedDataRateIndex: node " << endDeviceNode->GetId () << "\tDRindex = " << (unsigned int)calculatedDataRateIndex
      << "\tSF=" << (unsigned int)LoRaWAN::m_supportedDataRates[calculatedDataRateIndex].spreadingFactor << std::endl;
  }

  return calculatedDataRateIndex;
}

void
LoRaWANExampleTracing::SetupTracing (bool tracePhyTransmissions, bool tracePhyStates, bool traceMacPackets, bool traceMacStates, bool traceEdMsgs, bool traceNsDsMsgs, bool traceMisc)
{
  // Connect trace sources.
  // Trace Phy transmissions:
  if (tracePhyTransmissions) {
    for (NodeContainer::Iterator i = m_endDeviceNodes.Begin (); i != m_endDeviceNodes.End (); ++i)
    {
      Ptr<Node> node = *i;
      Ptr<LoRaWANNetDevice> netDevice = DynamicCast<LoRaWANNetDevice> (node->GetDevice (0));
      Ptr<LoRaWANPhy> phy = netDevice->GetPhy ();
      phy->TraceConnectWithoutContext ("PhyTxBegin", MakeBoundCallback (&LoRaWANExampleTracing::PhyTxBegin, this, netDevice, phy));
      phy->TraceConnectWithoutContext ("PhyTxEnd", MakeBoundCallback (&LoRaWANExampleTracing::PhyTxEnd, this, netDevice, phy));
      phy->TraceConnectWithoutContext ("PhyTxDrop", MakeBoundCallback (&LoRaWANExampleTracing::PhyTxDrop, this, netDevice, phy));
      phy->TraceConnectWithoutContext ("PhyRxBegin", MakeBoundCallback (&LoRaWANExampleTracing::PhyRxBegin, this, netDevice, phy));
      phy->TraceConnectWithoutContext ("PhyRxEnd", MakeBoundCallback (&LoRaWANExampleTracing::PhyRxEnd, this, netDevice, phy));
      phy->TraceConnectWithoutContext ("PhyRxDrop", MakeBoundCallback (&LoRaWANExampleTracing::PhyRxDrop, this, netDevice, phy));
    }

    for (NodeContainer::Iterator i = m_gatewayNodes.Begin (); i != m_gatewayNodes.End (); ++i)
    {
      Ptr<Node> node = *i;
      Ptr<LoRaWANNetDevice> netDevice = DynamicCast<LoRaWANNetDevice> (node->GetDevice (0));
      for (auto &phy : netDevice->GetPhys() ) {
        phy->TraceConnectWithoutContext ("PhyTxBegin", MakeBoundCallback (&LoRaWANExampleTracing::PhyTxBegin, this, netDevice, phy));
        phy->TraceConnectWithoutContext ("PhyTxEnd", MakeBoundCallback (&LoRaWANExampleTracing::PhyTxEnd, this, netDevice, phy));
        phy->TraceConnectWithoutContext ("PhyTxDrop", MakeBoundCallback (&LoRaWANExampleTracing::PhyTxDrop, this, netDevice, phy));
        phy->TraceConnectWithoutContext ("PhyRxBegin", MakeBoundCallback (&LoRaWANExampleTracing::PhyRxBegin, this, netDevice, phy));
        phy->TraceConnectWithoutContext ("PhyRxEnd", MakeBoundCallback (&LoRaWANExampleTracing::PhyRxEnd, this, netDevice, phy));
        phy->TraceConnectWithoutContext ("PhyRxDrop", MakeBoundCallback (&LoRaWANExampleTracing::PhyRxDrop, this, netDevice, phy));
      }
    }
  }

  if (tracePhyStates) {
    // Trace state changes in the phy and mac
    for (NodeContainer::Iterator i = m_endDeviceNodes.Begin (); i != m_endDeviceNodes.End (); ++i)
    {
      Ptr<Node> node = *i;
      Ptr<LoRaWANNetDevice> netDevice = DynamicCast<LoRaWANNetDevice> (node->GetDevice (0));
      Ptr<LoRaWANPhy> phy = netDevice->GetPhy ();
      phy->TraceConnectWithoutContext ("TrxState", MakeBoundCallback (&LoRaWANExampleTracing::PhyStateChangeNotification, this, netDevice, phy));
    }

    for (NodeContainer::Iterator i = m_gatewayNodes.Begin (); i != m_gatewayNodes.End (); ++i)
    {
      Ptr<Node> node = *i;
      Ptr<LoRaWANNetDevice> netDevice = DynamicCast<LoRaWANNetDevice> (node->GetDevice (0));
      for (auto &phy : netDevice->GetPhys ()) {
        phy->TraceConnectWithoutContext ("TrxState", MakeBoundCallback (&LoRaWANExampleTracing::PhyStateChangeNotification, this, netDevice, phy));
      }
    }
  }

  if (traceMacPackets) {
    for (NodeContainer::Iterator i = m_endDeviceNodes.Begin (); i != m_endDeviceNodes.End (); ++i)
    {
      Ptr<Node> node = *i;
      Ptr<LoRaWANNetDevice> netDevice = DynamicCast<LoRaWANNetDevice> (node->GetDevice (0));
      Ptr<LoRaWANMac> mac = netDevice->GetMac ();

      mac->TraceConnectWithoutContext ("MacTx", MakeBoundCallback (&LoRaWANExampleTracing::MacTx, this, netDevice, mac));
      mac->TraceConnectWithoutContext ("MacTxOk", MakeBoundCallback (&LoRaWANExampleTracing::MacTxOk, this, netDevice, mac));
      mac->TraceConnectWithoutContext ("MacTxDrop", MakeBoundCallback (&LoRaWANExampleTracing::MacTxDrop, this, netDevice, mac));
      mac->TraceConnectWithoutContext ("MacRx", MakeBoundCallback (&LoRaWANExampleTracing::MacRx, this, netDevice, mac));
      mac->TraceConnectWithoutContext ("MacRxDrop", MakeBoundCallback (&LoRaWANExampleTracing::MacRxDrop, this, netDevice, mac));
      mac->TraceConnectWithoutContext ("MacSentPkt", MakeBoundCallback (&LoRaWANExampleTracing::MacSentPkt, this, netDevice, mac));
    }

    for (NodeContainer::Iterator i = m_gatewayNodes.Begin (); i != m_gatewayNodes.End (); ++i)
    {
      Ptr<Node> node = *i;
      Ptr<LoRaWANNetDevice> netDevice = DynamicCast<LoRaWANNetDevice> (node->GetDevice (0));
      for (auto &mac : netDevice->GetMacs ()) {
        mac->TraceConnectWithoutContext ("MacTx", MakeBoundCallback (&LoRaWANExampleTracing::MacTx, this, netDevice, mac));
        mac->TraceConnectWithoutContext ("MacTxOk", MakeBoundCallback (&LoRaWANExampleTracing::MacTxOk, this, netDevice, mac));
        mac->TraceConnectWithoutContext ("MacTxDrop", MakeBoundCallback (&LoRaWANExampleTracing::MacTxDrop, this, netDevice, mac));
        mac->TraceConnectWithoutContext ("MacRx", MakeBoundCallback (&LoRaWANExampleTracing::MacRx, this, netDevice, mac));
        mac->TraceConnectWithoutContext ("MacRxDrop", MakeBoundCallback (&LoRaWANExampleTracing::MacRxDrop, this, netDevice, mac));
        mac->TraceConnectWithoutContext ("MacSentPkt", MakeBoundCallback (&LoRaWANExampleTracing::MacSentPkt, this, netDevice, mac));
      }
    }

  }

  if (traceMacStates) {
    // Trace state changes in the phy and mac
    for (NodeContainer::Iterator i = m_endDeviceNodes.Begin (); i != m_endDeviceNodes.End (); ++i)
    {
      Ptr<Node> node = *i;
      Ptr<LoRaWANNetDevice> netDevice = DynamicCast<LoRaWANNetDevice> (node->GetDevice (0));
      Ptr<LoRaWANMac> mac = netDevice->GetMac ();
      mac->TraceConnectWithoutContext ("MacState", MakeBoundCallback (&LoRaWANExampleTracing::MacStateChangeNotification, this, netDevice, mac));
    }

    for (NodeContainer::Iterator i = m_gatewayNodes.Begin (); i != m_gatewayNodes.End (); ++i)
    {
      Ptr<Node> node = *i;
      Ptr<LoRaWANNetDevice> netDevice = DynamicCast<LoRaWANNetDevice> (node->GetDevice (0));
      for (auto &mac : netDevice->GetMacs ()) {
        mac->TraceConnectWithoutContext ("MacState", MakeBoundCallback (&LoRaWANExampleTracing::MacStateChangeNotification, this, netDevice, mac));
      }
    }

  }

  if (traceEdMsgs) {
    for (NodeContainer::Iterator i = m_endDeviceNodes.Begin (); i != m_endDeviceNodes.End (); ++i)
    {
      Ptr<Node> node = *i;
      NS_ASSERT (node != 0);
      Ptr<LoRaWANEndDeviceApplication> edApp = node->GetApplication (0)->GetObject<LoRaWANEndDeviceApplication> ();

      edApp->TraceConnectWithoutContext ("USMsgTransmitted", MakeBoundCallback (&LoRaWANExampleTracing::USMsgTransmittedTrace, this));
      edApp->TraceConnectWithoutContext ("DSMsgReceived", MakeBoundCallback (&LoRaWANExampleTracing::DSMsgReceivedTrace, this));
    }
  }

  if (traceNsDsMsgs) {
    Ptr<LoRaWANNetworkServer> lorawanNSPtr = LoRaWANNetworkServer::getLoRaWANNetworkServerPointer ();
    NS_ASSERT (lorawanNSPtr);
    if (lorawanNSPtr) {
      lorawanNSPtr->TraceConnectWithoutContext ("DSMsgGenerated", MakeBoundCallback (&LoRaWANExampleTracing::DSMsgGeneratedTrace, this));
      lorawanNSPtr->TraceConnectWithoutContext ("DSMsgTransmitted", MakeBoundCallback (&LoRaWANExampleTracing::DSMsgTransmittedTrace, this));
      lorawanNSPtr->TraceConnectWithoutContext ("DSMsgAckd", MakeBoundCallback (&LoRaWANExampleTracing::DSMsgAckdTrace, this));
      lorawanNSPtr->TraceConnectWithoutContext ("DSMsgDropped", MakeBoundCallback (&LoRaWANExampleTracing::DSMsgDroppedTrace, this));
      lorawanNSPtr->TraceConnectWithoutContext ("USMsgReceived", MakeBoundCallback (&LoRaWANExampleTracing::USMsgReceivedTrace, this));
    }
  }

  if (traceMisc) {
    Ptr<LoRaWANNetworkServer> lorawanNSPtr = LoRaWANNetworkServer::getLoRaWANNetworkServerPointer ();
    NS_ASSERT (lorawanNSPtr);
    if (lorawanNSPtr) {
      lorawanNSPtr->TraceConnectWithoutContext ("nrRW1Sent", MakeBoundCallback (&LoRaWANExampleTracing::nrRW1SentTrace, this));
      lorawanNSPtr->TraceConnectWithoutContext ("nrRW2Sent", MakeBoundCallback (&LoRaWANExampleTracing::nrRW2SentTrace, this));
      lorawanNSPtr->TraceConnectWithoutContext ("nrRW1Missed", MakeBoundCallback (&LoRaWANExampleTracing::nrRW1MissedTrace, this));
      lorawanNSPtr->TraceConnectWithoutContext ("nrRW2Missed", MakeBoundCallback (&LoRaWANExampleTracing::nrRW2MissedTrace, this));
    }
  }
}

void
LoRaWANExampleTracing::LogOutputLine (std::string output, std::string fileName)
{
  if (this->m_stdcout)
    std::cout << output;

  std::ofstream & outputstream = output_streams[fileName];   // creates the stream object and returns reference
  if (!outputstream.is_open()) {
    outputstream.open(fileName.c_str(), std::ios::app);
  }
  if (outputstream.is_open()) {
    outputstream << output;
  }
}

std::ostringstream
LoRaWANExampleTracing::GeneratePhyTraceOutputLine (std::string traceSource, Ptr<LoRaWANNetDevice> device, Ptr<LoRaWANPhy> phy, Ptr<const Packet> packet, bool insertNewLine)
{
  // Generate hex string of packet
  std::ostringstream packetHex;
  packetHex << std::hex << std::setfill('0');
  uint8_t bufferSize = packet->GetSize ();
  uint8_t buffer[bufferSize];
  uint32_t nCopied = packet->CopyData (buffer, bufferSize);
  for (uint32_t i=0; i<nCopied; i++) {
    packetHex << std::setw(2) << static_cast<unsigned>(buffer[i]);
    //if ((i+1) % 4 == 0)
    //  packetHex << " ";
  }

  // Get Trace Tag added by Phy
  LoRaWANPhyTraceIdTag traceTag;
  std::string traceTagOutput = "-1";
  if (packet->PeekPacketTag (traceTag)) {
    traceTagOutput = std::to_string (traceTag.GetFlowId ());
  }

  // Generate output line
  std::ostringstream output;
  output << std::setiosflags (std::ios::fixed) << std::setprecision (9) << Simulator::Now ().GetSeconds () << ","
    << device->GetDeviceType () << ","
    << device->GetNode ()->GetId () << ","
    << device->GetIfIndex () << ","
    << static_cast<uint32_t>(phy->GetIndex ()) << ","
    << traceSource << ","
    << traceTagOutput << ","
    << packetHex.str () << ","
    << packet->GetSize ();

  if (insertNewLine)
    output << std::endl;

  return output;
}

void
LoRaWANExampleTracing::PhyTxBegin (LoRaWANExampleTracing* example, Ptr<LoRaWANNetDevice> device, Ptr<LoRaWANPhy> phy, Ptr<const Packet> packet)
{
  std::ostringstream output = example->GeneratePhyTraceOutputLine ("PhyTxBegin", device, phy, packet, false);
  // add channel and data rate index of Phy to output
  uint8_t channelIndex = phy->GetCurrentChannelIndex ();
  uint8_t dataRateIndex = phy->GetCurrentDataRateIndex ();;
  output << "," << (uint32_t)channelIndex << "," << (uint32_t)dataRateIndex <<  std::endl;
  example->LogOutputLine (output.str (), example->m_phyTransmissionTraceCSVFileName);
}

void
LoRaWANExampleTracing::PhyTxEnd (LoRaWANExampleTracing* example, Ptr<LoRaWANNetDevice> device, Ptr<LoRaWANPhy> phy, Ptr<const Packet> packet)
{
  std::ostringstream output = example->GeneratePhyTraceOutputLine ("PhyTxEnd", device, phy, packet);
  example->LogOutputLine (output.str (), example->m_phyTransmissionTraceCSVFileName);
}

void
LoRaWANExampleTracing::PhyTxDrop (LoRaWANExampleTracing* example, Ptr<LoRaWANNetDevice> device, Ptr<LoRaWANPhy> phy, Ptr<const Packet> packet)
{
  std::ostringstream output = example->GeneratePhyTraceOutputLine ("PhyTxDrop", device, phy, packet);
  example->LogOutputLine (output.str (), example->m_phyTransmissionTraceCSVFileName);
}

void
LoRaWANExampleTracing::PhyRxBegin (LoRaWANExampleTracing* example, Ptr<LoRaWANNetDevice> device, Ptr<LoRaWANPhy> phy, Ptr<const Packet> packet)
{
  std::ostringstream output = example->GeneratePhyTraceOutputLine ("PhyRxBegin", device, phy, packet);
  example->LogOutputLine (output.str (), example->m_phyTransmissionTraceCSVFileName);
}

void
LoRaWANExampleTracing::PhyRxEnd (LoRaWANExampleTracing* example, Ptr<LoRaWANNetDevice> device, Ptr<LoRaWANPhy> phy, Ptr<const Packet> packet, double lqi)
{
  std::ostringstream output = example->GeneratePhyTraceOutputLine ("PhyRxEnd", device, phy, packet, false);
  output
    << "," << lqi << std::endl;
  example->LogOutputLine (output.str (), example->m_phyTransmissionTraceCSVFileName);
}

void
LoRaWANExampleTracing::PhyRxDrop (LoRaWANExampleTracing* example, Ptr<LoRaWANNetDevice> device, Ptr<LoRaWANPhy> phy, Ptr<const Packet> packet, LoRaWANPhyDropRxReason dropReason)
{
  std::ostringstream output = example->GeneratePhyTraceOutputLine ("PhyRxDrop", device, phy, packet, false);
  output
    << "," << dropReason << std::endl;
  example->LogOutputLine (output.str (), example->m_phyTransmissionTraceCSVFileName);
}


std::ostringstream
LoRaWANExampleTracing::GenerateMacTraceOutputLine (std::string traceSource, Ptr<LoRaWANNetDevice> device, Ptr<LoRaWANMac> mac, Ptr<const Packet> packet, bool insertNewLine)
{
  // Generate hex string of packet
  std::ostringstream packetHex;
  packetHex << std::hex << std::setfill('0');
  uint8_t bufferSize = packet->GetSize ();
  uint8_t buffer[bufferSize];
  uint32_t nCopied = packet->CopyData (buffer, bufferSize);
  for (uint32_t i=0; i<nCopied; i++) {
    packetHex << std::setw(2) << static_cast<unsigned>(buffer[i]);
  }

  // Generate output line
  std::ostringstream output;
  output << std::setiosflags (std::ios::fixed) << std::setprecision (9) << Simulator::Now ().GetSeconds () << ","
    << device->GetDeviceType () << ","
    << device->GetNode ()->GetId () << ","
    << device->GetIfIndex () << ","
    << static_cast<uint32_t>(mac->GetIndex ()) << ","
    << traceSource << ","
    << packetHex.str () << ","
    << packet->GetSize ();

  if (insertNewLine)
    output << std::endl;

  return output;
}

void
LoRaWANExampleTracing::MacTx (LoRaWANExampleTracing* example, Ptr<LoRaWANNetDevice> device, Ptr<LoRaWANMac> mac, Ptr<const Packet> packet)
{
  std::ostringstream output = example->GenerateMacTraceOutputLine ("MacTx", device, mac, packet);
  example->LogOutputLine (output.str (), example->m_macPacketTraceCSVFileName);
}

void
LoRaWANExampleTracing::MacTxOk (LoRaWANExampleTracing* example, Ptr<LoRaWANNetDevice> device, Ptr<LoRaWANMac> mac, Ptr<const Packet> packet)
{
  std::ostringstream output = example->GenerateMacTraceOutputLine ("MacTxOk", device, mac, packet);
  example->LogOutputLine (output.str (), example->m_macPacketTraceCSVFileName);
}

void
LoRaWANExampleTracing::MacTxDrop (LoRaWANExampleTracing* example, Ptr<LoRaWANNetDevice> device, Ptr<LoRaWANMac> mac, Ptr<const Packet> packet)
{
  std::ostringstream output = example->GenerateMacTraceOutputLine ("MacTxDrop", device, mac, packet);
  example->LogOutputLine (output.str (), example->m_macPacketTraceCSVFileName);
}

void
LoRaWANExampleTracing::MacRx (LoRaWANExampleTracing* example, Ptr<LoRaWANNetDevice> device, Ptr<LoRaWANMac> mac, Ptr<const Packet> packet)
{
  std::ostringstream output = example->GenerateMacTraceOutputLine ("MacRx", device, mac, packet);
  example->LogOutputLine (output.str (), example->m_macPacketTraceCSVFileName);
}

void
LoRaWANExampleTracing::MacRxDrop (LoRaWANExampleTracing* example, Ptr<LoRaWANNetDevice> device, Ptr<LoRaWANMac> mac, Ptr<const Packet> packet)
{
  std::ostringstream output = example->GenerateMacTraceOutputLine ("MacRxDrop", device, mac, packet);
  example->LogOutputLine (output.str (), example->m_macPacketTraceCSVFileName);
}

void
LoRaWANExampleTracing::MacSentPkt (LoRaWANExampleTracing* example, Ptr<LoRaWANNetDevice> device, Ptr<LoRaWANMac> mac, Ptr<const Packet> packet, uint8_t n_transmissions)
{
  std::ostringstream output = example->GenerateMacTraceOutputLine ("MacSentPkt", device, mac, packet, false);
  output
    << "," << (uint32_t)n_transmissions << std::endl;
  example->LogOutputLine (output.str (), example->m_macPacketTraceCSVFileName);
}

void
LoRaWANExampleTracing::PhyStateChangeNotification (LoRaWANExampleTracing* example, Ptr<LoRaWANNetDevice> device, Ptr<LoRaWANPhy> phy, LoRaWANPhyEnumeration oldState, LoRaWANPhyEnumeration newState)
{
  std::ostringstream output;
  output << std::setiosflags (std::ios::fixed) << std::setprecision (9) << Simulator::Now ().GetSeconds () << ","
    << device->GetDeviceType () << ","
    << device->GetNode ()->GetId () << ","
    << device->GetIfIndex () << ","
    << static_cast<uint32_t>(phy->GetIndex ()) << ","
    << "PhyState" << ","
    << oldState << ","
    << newState
    << std::endl;

  example->LogOutputLine (output.str (), example->m_phyStateTraceCSVFileName);
}

void
LoRaWANExampleTracing::MacStateChangeNotification (LoRaWANExampleTracing* example, Ptr<LoRaWANNetDevice> device, Ptr<LoRaWANMac> mac, LoRaWANMacState oldState, LoRaWANMacState newState)
{
  std::ostringstream output;
  output << std::setiosflags (std::ios::fixed) << std::setprecision (9) << Simulator::Now ().GetSeconds () << ","
    << device->GetDeviceType () << ","
    << device->GetNode ()->GetId () << ","
    << device->GetIfIndex () << ","
    << static_cast<uint32_t>(mac->GetIndex ()) << ","
    << "MacState" << ","
    << oldState << ","
    << newState
    << std::endl;

  example->LogOutputLine (output.str (), example->m_macStateTraceCSVFileName);
}

void
LoRaWANExampleTracing::nrRW1SentTrace (LoRaWANExampleTracing* example, uint32_t oldValue, uint32_t newValue)
{
  example->m_nrRW1Sent = newValue;
}

void
LoRaWANExampleTracing::nrRW2SentTrace (LoRaWANExampleTracing* example, uint32_t oldValue, uint32_t newValue)
{
  example->m_nrRW2Sent = newValue;
}
void
LoRaWANExampleTracing::nrRW1MissedTrace (LoRaWANExampleTracing* example, uint32_t oldValue, uint32_t newValue)
{
  example->m_nrRW1Missed = newValue;
}

void
LoRaWANExampleTracing::nrRW2MissedTrace (LoRaWANExampleTracing* example, uint32_t oldValue, uint32_t newValue)
{
  example->m_nrRW2Missed = newValue;
}

std::ostringstream
LoRaWANExampleTracing::GenerateEDMsgTraceOutputLine (std::string traceSource, uint32_t deviceAddress, uint8_t msgType, Ptr<const Packet> packet, bool insertNewLine)
{
  // Generate hex string of packet
  std::ostringstream packetHex;
  packetHex << std::hex << std::setfill('0');

  uint8_t bufferSize = packet->GetSize ();
  uint8_t buffer[bufferSize];
  uint32_t nCopied = packet->CopyData (buffer, bufferSize);
  for (uint32_t i=0; i<nCopied; i++) {
    packetHex << std::setw(2) << static_cast<unsigned>(buffer[i]);
  }

  // Generate output line
  std::ostringstream output;
  output << std::setiosflags (std::ios::fixed) << std::setprecision (9) << Simulator::Now ().GetSeconds () << ","
    << traceSource << ","
    << deviceAddress << ","
    << (uint32_t) msgType << ","
    << packetHex.str () << ","
    << packet->GetSize ();

  if (insertNewLine)
    output << std::endl;

  return output;
}

void
LoRaWANExampleTracing::USMsgTransmittedTrace (LoRaWANExampleTracing* example, uint32_t deviceAddress, uint8_t msgType, Ptr<const Packet> packet)
{
  std::ostringstream output = example->GenerateEDMsgTraceOutputLine ("USMsgTx", deviceAddress, msgType, packet);
  example->LogOutputLine (output.str (), example->m_edMsgTraceCSVFileName);
}

void
LoRaWANExampleTracing::DSMsgReceivedTrace (LoRaWANExampleTracing* example, uint32_t deviceAddress, uint8_t msgType, Ptr<const Packet> packet, uint8_t rw)
{
  std::ostringstream output = example->GenerateEDMsgTraceOutputLine ("DSMsgRx", deviceAddress, msgType, packet, false);
  // add receive window to output
  output << "," << (unsigned)rw << std::endl;
  example->LogOutputLine (output.str (), example->m_edMsgTraceCSVFileName);
}

std::ostringstream
LoRaWANExampleTracing::GenerateNSDSMsgTraceOutputLine (std::string traceSource, uint32_t deviceAddress, uint8_t transmissionsRemaning, uint8_t msgType, Ptr<const Packet> packet, bool insertNewLine)
{
  // Generate hex string of packet
  std::ostringstream packetHex;
  packetHex << std::hex << std::setfill('0');

  uint8_t bufferSize = packet->GetSize ();
  uint8_t buffer[bufferSize];
  uint32_t nCopied = packet->CopyData (buffer, bufferSize);
  for (uint32_t i=0; i<nCopied; i++) {
    packetHex << std::setw(2) << static_cast<unsigned>(buffer[i]);
  }

  // Generate output line
  std::ostringstream output;
  output << std::setiosflags (std::ios::fixed) << std::setprecision (9) << Simulator::Now ().GetSeconds () << ","
    << traceSource << ","
    << deviceAddress << ","
    << (uint32_t) msgType << ","
    << (uint32_t) transmissionsRemaning << ","
    << packetHex.str () << ","
    << packet->GetSize ();

  if (insertNewLine)
    output << std::endl;

  return output;
}

void
LoRaWANExampleTracing::DSMsgTransmittedTrace (LoRaWANExampleTracing* example, uint32_t deviceAddress, uint8_t transmissionsRemaning, uint8_t msgType, Ptr<const Packet> packet, uint8_t rw)
{
  std::ostringstream output = example->GenerateNSDSMsgTraceOutputLine ("DSMsgTx", deviceAddress, transmissionsRemaning, msgType, packet, false);
  // add receive window to output
  output << "," << (unsigned)rw << std::endl;
  example->LogOutputLine (output.str (), example->m_nsDSMsgTraceCSVFileName);
}

void
LoRaWANExampleTracing::DSMsgGeneratedTrace (LoRaWANExampleTracing* example, uint32_t deviceAddress, uint8_t transmissionsRemaning, uint8_t msgType, Ptr<const Packet> packet)
{
  std::ostringstream output = example->GenerateNSDSMsgTraceOutputLine ("DSMsgGenerated", deviceAddress, transmissionsRemaning, msgType, packet);
  example->LogOutputLine (output.str (), example->m_nsDSMsgTraceCSVFileName);
}

void
LoRaWANExampleTracing::DSMsgAckdTrace (LoRaWANExampleTracing* example, uint32_t deviceAddress, uint8_t transmissionsRemaning, uint8_t msgType, Ptr<const Packet> packet)
{
  std::ostringstream output = example->GenerateNSDSMsgTraceOutputLine ("DSMsgAckd", deviceAddress, transmissionsRemaning, msgType, packet);
  example->LogOutputLine (output.str (), example->m_nsDSMsgTraceCSVFileName);
}

void
LoRaWANExampleTracing::DSMsgDroppedTrace (LoRaWANExampleTracing* example, uint32_t deviceAddress, uint8_t transmissionsRemaning, uint8_t msgType, Ptr<const Packet> packet)
{
  std::ostringstream output = example->GenerateNSDSMsgTraceOutputLine ("DSMsgDrop", deviceAddress, transmissionsRemaning, msgType, packet);
  example->LogOutputLine (output.str (), example->m_nsDSMsgTraceCSVFileName);
}

void
LoRaWANExampleTracing::USMsgReceivedTrace (LoRaWANExampleTracing* example, uint32_t deviceAddress, uint8_t msgType, Ptr<const Packet> packet)
{
  std::ostringstream output = example->GenerateNSDSMsgTraceOutputLine ("USMsgRx", deviceAddress, 0, msgType, packet);
  example->LogOutputLine (output.str (), example->m_nsDSMsgTraceCSVFileName);
}

void
LoRaWANExampleTracing::WriteMiscStatsToFile ()
{
  std::ofstream out (m_miscTraceCSVFileName.c_str (), std::ios::app);
  out << m_nrRW1Sent << ","
    << m_nrRW2Sent << ","
    << m_nrRW1Missed << ","
    << m_nrRW2Missed
    << std::endl;
  out.close ();
}
