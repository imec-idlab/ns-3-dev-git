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

// This program produces a gnuplot file that plots the packet success rate
// as a function of distance for the lorawan error model, assuming a default
// LogDistance propagation loss model, the MATLAB exp2 error model, a
// default transmit power of 0 dBm, and a default packet size of 20 bytes of
// MACPayload.
#include <ns3/test.h>
#include <ns3/log.h>
#include <ns3/callback.h>
#include <ns3/packet.h>
#include <ns3/simulator.h>
#include <ns3/propagation-loss-model.h>
#include <ns3/spectrum-value.h>
#include <ns3/node.h>
#include <ns3/net-device.h>
#include <ns3/single-model-spectrum-channel.h>
#include <ns3/multi-model-spectrum-channel.h>
#include <ns3/mac16-address.h>
#include <ns3/constant-position-mobility-model.h>
#include <ns3/uinteger.h>
#include <ns3/nstime.h>
#include <ns3/abort.h>
#include <ns3/command-line.h>
#include <ns3/gnuplot.h>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace ns3;
using namespace std;

static uint32_t g_received = 0;

NS_LOG_COMPONENT_DEFINE ("LoRaWANErrorDistancePlot");

static void
LoRaWANErrorDistanceCallback (LoRaWANDataIndicationParams params, Ptr<Packet> p)
{
  g_received++;
}

int main (int argc, char *argv[])
{
  std::ostringstream os;
  std::ofstream berfile ("lorawan-psr-distance.plt");

  int minDistance = 1;
  int maxDistance = 200;  // meters
  int increment = 1;
  int maxPackets = 1000;
  int packetSize = 20;
  double txPower = 0;
  uint32_t channelIndex = 0;

  CommandLine cmd;

  cmd.AddValue ("txPower", "transmit power (dBm)", txPower);
  cmd.AddValue ("packetSize", "packet (MSDU) size (bytes)", packetSize);
  cmd.AddValue ("channelIndex", "channel index", channelIndex);

  cmd.Parse (argc, argv);

  if (channelIndex >= LoRaWAN::m_supportedChannels.size())
    NS_LOG_ERROR("Invalid channel index, supported channels vector size = " << LoRaWAN::m_supportedChannels.size());

  LoRaWANChannel lorawanChannel = LoRaWAN::m_supportedChannels [channelIndex];
  os << "Packet (MSDU) size = " << packetSize << " bytes; tx power = " << txPower << " dBm; channelIndex = " << channelIndex;

  Gnuplot psrplot = Gnuplot ("lorawan-distance.eps");
  Gnuplot2dDataset psrdataset ("lorawan-psr-vs-distance");

  Ptr<Node> n0 = CreateObject <Node> ();
  Ptr<Node> n1 = CreateObject <Node> ();
  Ptr<LoRaWANNetDevice> dev0 = CreateObject<LoRaWANNetDevice> ();
  Ptr<LoRaWANNetDevice> dev1 = CreateObject<LoRaWANNetDevice> ();
  //dev0->SetAddress (Mac16Address ("00:01"));
  //dev1->SetAddress (Mac16Address ("00:02"));
  Ptr<MultiModelSpectrumChannel> channel = CreateObject<MultiModelSpectrumChannel> ();
  Ptr<LogDistancePropagationLossModel> model = CreateObject<LogDistancePropagationLossModel> ();
  channel->AddPropagationLossModel (model);
  dev0->SetChannel (channel);
  dev1->SetChannel (channel);
  n0->AddDevice (dev0);
  n1->AddDevice (dev1);
  Ptr<ConstantPositionMobilityModel> mob0 = CreateObject<ConstantPositionMobilityModel> ();
  dev0->GetPhy ()->SetMobility (mob0);
  Ptr<ConstantPositionMobilityModel> mob1 = CreateObject<ConstantPositionMobilityModel> ();
  dev1->GetPhy ()->SetMobility (mob1);

  LoRaWANSpectrumValueHelper svh;
  Ptr<SpectrumValue> psd = svh.CreateTxPowerSpectralDensity (txPower, lorawanChannel.m_fc);
  dev0->GetPhy ()->SetTxPowerSpectralDensity (psd);

  McpsDataIndicationCallback cb0;
  cb0 = MakeCallback (&LoRaWANErrorDistanceCallback);
  dev1->GetMac ()->SetDataIndicationCallback (cb0);

  McpsDataRequestParams params;
  params.m_srcAddrMode = SHORT_ADDR;
  params.m_dstAddrMode = SHORT_ADDR;
  params.m_dstPanId = 0;
  params.m_dstAddr = Mac16Address ("00:02");
  params.m_msduHandle = 0;
  params.m_txOptions = 0;

  Ptr<Packet> p;
  mob0->SetPosition (Vector (0,0,0));
  mob1->SetPosition (Vector (minDistance,0,0));
  for (int j = minDistance; j < maxDistance;  )
    {
      for (int i = 0; i < maxPackets; i++)
        {
          p = Create<Packet> (packetSize);
          Simulator::Schedule (Seconds (i),
                               &LoRaWANMac::McpsDataRequest,
                               dev0->GetMac (), params, p);
        }
      Simulator::Run ();
      NS_LOG_DEBUG ("Received " << g_received << " packets for distance " << j);
      psrdataset.Add (j, g_received / 1000.0);
      g_received = 0;
      j += increment;
      mob1->SetPosition (Vector (j,0,0));
    }

  psrplot.AddDataset (psrdataset);

  psrplot.SetTitle (os.str ());
  psrplot.SetTerminal ("postscript eps color enh \"Times-BoldItalic\"");
  psrplot.SetLegend ("distance (m)", "Packet Success Rate (PSR)");
  psrplot.SetExtra  ("set xrange [0:200]\n\
set yrange [0:1]\n\
set grid\n\
set style line 1 linewidth 5\n\
set style increment user");
  psrplot.GenerateOutput (berfile);
  berfile.close ();

  Simulator::Destroy ();
  return 0;
}

