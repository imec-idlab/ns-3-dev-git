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
#include <ns3/test.h>
#include <ns3/command-line.h>
#include <ns3/packet.h>
#include <ns3/lorawan-phy.h>
#include <ns3/lorawan-mac.h>
#include <ns3/simulator.h>
#include <ns3/single-model-spectrum-channel.h>
#include <ns3/constant-position-mobility-model.h>

using namespace ns3;

void GetSetTRXStateConfirm (LoRaWANPhyEnumeration status)
{
  NS_LOG_UNCOND ("At: " << Simulator::Now ()
                        << " Received Set TRX Confirm: " << status);
}

void
ReceivePdDataIndication (uint32_t psduLength,
                         Ptr<Packet> p,
                         uint8_t lqi,
                         uint8_t channelIndex,
                         uint8_t dataRateIndex,
                         uint8_t codeRate)
{
  NS_LOG_UNCOND ("At: " << Simulator::Now ()
                        << " Received frame size: " << psduLength
                        << " LQI: " << (uint16_t) lqi
                        << " channelIndex: " << (uint16_t) channelIndex
                        << " dataRateIndex: " << (uint16_t) dataRateIndex
                        << " codeRate: " << (uint16_t) codeRate);
}

void SendOnePacket (Ptr<LoRaWANPhy> sender, Ptr<LoRaWANPhy> receiver)
{
  NS_LOG_UNCOND ("At: " << Simulator::Now ()
                        << " Sending one frame");

  uint32_t n = 10;
  Ptr<Packet> p = Create<Packet> (n);
  sender->PdDataRequest (p->GetSize (), p);
}


int main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);

  LogComponentEnableAll (LOG_PREFIX_FUNC);
  LogComponentEnable ("LoRaWANPhy", LOG_LEVEL_ALL);
  LogComponentEnable ("SingleModelSpectrumChannel", LOG_LEVEL_ALL);

  Ptr<LoRaWANPhy> sender = CreateObject<LoRaWANPhy> ();
  Ptr<LoRaWANPhy> receiver = CreateObject<LoRaWANPhy> ();

  Ptr<SingleModelSpectrumChannel> channel = CreateObject<SingleModelSpectrumChannel> ();
  sender->SetChannel (channel);
  receiver->SetChannel (channel);
  channel->AddRx (sender);
  channel->AddRx (receiver);

  // CONFIGURE MOBILITY
  Ptr<ConstantPositionMobilityModel> senderMobility = CreateObject<ConstantPositionMobilityModel> ();
  sender->SetMobility (senderMobility);
  Ptr<ConstantPositionMobilityModel> receiverMobility = CreateObject<ConstantPositionMobilityModel> ();
  receiver->SetMobility (receiverMobility);


  sender->SetSetTRXStateConfirmCallback (MakeCallback (&GetSetTRXStateConfirm));
  receiver->SetSetTRXStateConfirmCallback (MakeCallback (&GetSetTRXStateConfirm));

  sender->SetTRXStateRequest (LORAWAN_PHY_TX_ON);
  receiver->SetTRXStateRequest (LORAWAN_PHY_RX_ON);

  receiver->SetPdDataIndicationCallback (MakeCallback (&ReceivePdDataIndication));

  Simulator::Schedule (Seconds (1.0), &SendOnePacket, sender, receiver);

  Simulator::Stop (Seconds (10.0));

  Simulator::Run ();

  Simulator::Destroy ();

  return 0;
}
