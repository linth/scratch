/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 * Author: Manuel Requena <manuel.requena@cttc.es>
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/config-store-module.h"
#include "src/internet/model/ipv4-interface.h"
#include "src/internet/helper/ipv4-interface-container.h"
#include "src/flow-monitor/helper/flow-monitor-helper.h"
#include "src/flow-monitor/model/ipv4-flow-classifier.h"
#include "ns3/ipv4-flow-classifier.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("PacektLossCounter");

void
NotifyConnectionEstablishedUe (std::string context,
                               uint64_t imsi,
                               uint16_t cellid,
                               uint16_t rnti)
{
  std::cout << context
            << " UE IMSI " << imsi
            << ": connected to CellId " << cellid
            << " with RNTI " << rnti
            << std::endl;
}

void
NotifyHandoverStartUe (std::string context,
                       uint64_t imsi,
                       uint16_t cellid,
                       uint16_t rnti,
                       uint16_t targetCellId)
{
  std::cout << context
            << " UE IMSI " << imsi
            << ": previously connected to CellId " << cellid
            << " with RNTI " << rnti
            << ", doing handover to CellId " << targetCellId
            << std::endl;
}

void
NotifyHandoverEndOkUe (std::string context,
                       uint64_t imsi,
                       uint16_t cellid,
                       uint16_t rnti)
{
  std::cout << context
            << " UE IMSI " << imsi
            << ": successful handover to CellId " << cellid
            << " with RNTI " << rnti
            << std::endl;
}

void
NotifyConnectionEstablishedEnb (std::string context,
                                uint64_t imsi,
                                uint16_t cellid,
                                uint16_t rnti)
{
  std::cout << context
            << " eNB CellId " << cellid
            << ": successful connection of UE with IMSI " << imsi
            << " RNTI " << rnti
            << std::endl;
}

void
NotifyHandoverStartEnb (std::string context,
                        uint64_t imsi,
                        uint16_t cellid,
                        uint16_t rnti,
                        uint16_t targetCellId)
{
  std::cout << context
            << " eNB CellId " << cellid
            << ": start handover of UE with IMSI " << imsi
            << " RNTI " << rnti
            << " to CellId " << targetCellId
            << std::endl;
}

void
NotifyHandoverEndOkEnb (std::string context,
                        uint64_t imsi,
                        uint16_t cellid,
                        uint16_t rnti)
{
  std::cout << context
            << " eNB CellId " << cellid
            << ": completed handover of UE with IMSI " << imsi
            << " RNTI " << rnti
            << std::endl;
}

double firstRxTime = -1.0;
double lastRxTime;
uint32_t bytesTotal = 0; 

void DevTxTrace (std::string context, Ptr<const Packet> p, Mac48Address address)
{
  std::cout << "TX to= " << address << " p: " << *p << std::endl;
}

void SinkRxTrace (Ptr<const Packet> pkt, const Address &addr)
{
  if (firstRxTime < 0)
  {
    firstRxTime = Simulator::Now().GetSeconds();
  }
  lastRxTime = Simulator::Now().GetSeconds();
  bytesTotal += pkt->GetSize();
}

static void CourseChange (std::string foo, Ptr<const MobilityModel> mobility)
{
  Vector pos = mobility->GetPosition ();
  Vector vel = mobility->GetVelocity ();
  std::cout << Simulator::Now () << ", model=" << mobility << ", POS: x=" << pos.x << ", y=" << pos.y
            << ", z=" << pos.z << "; VEL:" << vel.x << ", y=" << vel.y
            << ", z=" << vel.z << std::endl;
}

/**
 * Sample simulation script for an automatic X2-based handover based on the RSRQ measures.
 * It instantiates two eNodeB, attaches one UE to the 'source' eNB.
 * The UE moves between both eNBs, it reports measures to the serving eNB and
 * the 'source' (serving) eNB triggers the handover of the UE towards
 * the 'target' eNB when it considers it is a better eNB.
 */
int
main (int argc, char *argv[])
{
//   LogLevel logLevel = (LogLevel)(LOG_PREFIX_ALL | LOG_LEVEL_ALL);
//
//   LogComponentEnable ("LteHelper", logLevel);
//   LogComponentEnable ("EpcHelper", logLevel);
//   LogComponentEnable ("EpcEnbApplication", logLevel);
//   LogComponentEnable ("EpcX2", logLevel);
//   LogComponentEnable ("EpcSgwPgwApplication", logLevel);
//
//   LogComponentEnable ("LteEnbRrc", logLevel);
//   LogComponentEnable ("LteEnbNetDevice", logLevel);
//   LogComponentEnable ("LteUeRrc", logLevel);
//   LogComponentEnable ("LteUeNetDevice", logLevel);
//   LogComponentEnable ("A2A4RsrqHandoverAlgorithm", logLevel);
//   LogComponentEnable ("A3RsrpHandoverAlgorithm", logLevel);

  uint16_t numberOfUes = 1;
  uint16_t numberOfEnbs = 2;
  uint16_t numberOfFixUes = 20;
  uint16_t numBearersPerUe = 1;
  double distance = 500.0; // m
//  double yForUe = 500.0;   // m
  double speed = 33.33;    // m/s
//  double simTime = (double)(numberOfEnbs + 1) * distance / speed; // 1500 m / 20 m/s = 75 secs
  double simTime = 75.0; // 180 secs.
  double enbTxPowerDbm = 46.0;
  double TTT = 256.0;
  double Hyst = 2.0;
  double throughput = 0.0;
  double numberOfPacketLoss = 0.0;
  double packetLossRate = 0.0;
  
//  double randomDirection; // for direction.
//  srand (time(NULL));
//  if (rand() % 2 == 0)
//  {
//      randomDirection = 0;
//  }
//  else
//  {
//      randomDirection = 4.71238;
//  }
  
  // change some default attributes so that they are reasonable for
  // this scenario, but do this before processing command line
  // arguments, so that the user is allowed to override these settings
  Config::SetDefault ("ns3::UdpClient::Interval", TimeValue (MilliSeconds (10)));
  Config::SetDefault ("ns3::UdpClient::MaxPackets", UintegerValue (1000000));
  Config::SetDefault ("ns3::LteHelper::UseIdealRrc", BooleanValue (true));

  // Command line arguments
  CommandLine cmd;
  cmd.AddValue ("simTime", "Total duration of the simulation (in seconds)", simTime);
  cmd.AddValue ("speed", "Speed of the UE (default = 20 m/s)", speed);
  cmd.AddValue ("enbTxPowerDbm", "TX power [dBm] used by HeNBs (defalut = 46.0)", enbTxPowerDbm);
  cmd.AddValue ("TTT", "Time-To-Trigger (default = 256", TTT);
  cmd.AddValue ("Hyst", "Hysteresis (default = 2.0", Hyst);

  cmd.Parse (argc, argv);


  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);
  lteHelper->SetSchedulerType ("ns3::RrFfMacScheduler");

//  lteHelper->SetHandoverAlgorithmType ("ns3::A2A4RsrqHandoverAlgorithm");
//  lteHelper->SetHandoverAlgorithmAttribute ("ServingCellThreshold",
//                                            UintegerValue (30));
//  lteHelper->SetHandoverAlgorithmAttribute ("NeighbourCellOffset",
//                                            UintegerValue (1));

    lteHelper->SetHandoverAlgorithmType ("ns3::A3RsrpHandoverAlgorithm");
    lteHelper->SetHandoverAlgorithmAttribute ("Hysteresis",
                                              DoubleValue (Hyst));
    lteHelper->SetHandoverAlgorithmAttribute ("TimeToTrigger",
                                              TimeValue (MilliSeconds (TTT)));

  Ptr<Node> pgw = epcHelper->GetPgwNode ();

  // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);


  // Routing of the Internet Host (towards the LTE network)
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  // interface 0 is localhost, 1 is the p2p device
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  /*
   * Network topology:
   *
   *      |     + --------------------------------------------------------->
   *      |     UE
   *      |
   *      |               d                   d                   d
   *    y |     |-------------------x-------------------x-------------------
   *      |     |                 eNodeB              eNodeB
   *      |   d |
   *      |     |
   *      |     |                                             d = distance
   *            o (0, 0, 0)                                   y = yForUe
   */

  NodeContainer ueNodes;
  NodeContainer enbNodes;
  NodeContainer uefixNodes; // fix UE node container.
  
  enbNodes.Create (numberOfEnbs);
  ueNodes.Create (numberOfUes);
  uefixNodes.Create (numberOfFixUes); // create fix UE node.

  Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator> (); // Install the location of eNB.
  for (uint16_t i = 0; i < numberOfEnbs; i++)
    {
      Vector enbPosition (distance * (i + 1), distance, 0);
      enbPositionAlloc->Add (enbPosition);
    }
  MobilityHelper enbMobility; // Install Mobility Model in eNB
  enbMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  enbMobility.SetPositionAllocator (enbPositionAlloc);
  enbMobility.Install (enbNodes);

  MobilityHelper ueMobility; // Install Mobility Model in UE
//  ueMobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
//  ueMobility.Install (ueNodes);
//  ueNodes.Get (0)->GetObject<MobilityModel> ()->SetPosition (Vector (500, yForUe, 0));
//  ueNodes.Get (0)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (speed, 0, 0));
  
  
  
  ueMobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (750.0),
                                 "MinY", DoubleValue (500.0),
                                 "DeltaX", DoubleValue (0.0),
                                 "DeltaY", DoubleValue (0.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst")); // set up the location of station nodes.

  ueMobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (600, 900, 490, 510)),
                             "Time", StringValue ("2s"),
//                             "Direction", DoubleValue (randomDirection), // 2 direction.
                             "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=33.33]")); // set up the mobility model of station nodes.
                             // Min=2.0|Max=4.0
  ueMobility.Install (ueNodes);
  

  Ptr<ListPositionAllocator> fixuePoistionAlloc = CreateObject<ListPositionAllocator> (); // Install the location of fixed UE.
  for (uint16_t i = 0;  i < numberOfFixUes/2; i++)
  {
      for (uint16_t j = 0; j < numberOfFixUes/2; j++)
      {
          Vector fixuePosition (1100 + i*10, 250 + j*50, 0);
          fixuePoistionAlloc->Add (fixuePosition);
      }
  }
  MobilityHelper fixueMobility; // Install Mobility Model in fixed UE. 
  fixueMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  fixueMobility.SetPositionAllocator (fixuePoistionAlloc);
  fixueMobility.Install (uefixNodes);
  
  // Install LTE Devices in eNB and UEs
  Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (enbTxPowerDbm));
  NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);
  NetDeviceContainer fixueLteDevs = lteHelper->InstallUeDevice (uefixNodes); // fixed UE nodes' device.

  // Install the IP stack on the UEs
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIfaces;
  ueIpIfaces = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));
  internet.Install (uefixNodes);
  Ipv4InterfaceContainer fixueIpIfaces;
  fixueIpIfaces = epcHelper->AssignUeIpv4Address (NetDeviceContainer (fixueLteDevs));
  // Assign IP address to UEs, and install applications
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<Node> ueNode = ueNodes.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }
  for (uint32_t u = 0; u < uefixNodes.GetN(); ++u)
  {
      Ptr<Node> uefixNode = uefixNodes.Get (u);
      Ptr<Ipv4StaticRouting> fixueStaticRouting = ipv4RoutingHelper.GetStaticRouting(uefixNode->GetObject<Ipv4> ());
      fixueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress (), 1);
  }

  // Attach all UEs to the first eNodeB
  for (uint16_t i = 0; i < numberOfUes; i++)
    {
      lteHelper->Attach (ueLteDevs.Get (i), enbLteDevs.Get (0));
    }
  for (uint16_t i = 0; i < numberOfFixUes; i++)
  {
      lteHelper->Attach (fixueLteDevs.Get (i), enbLteDevs.Get (1));
  }


  NS_LOG_LOGIC ("setting up applications");

  // Install and start applications on UEs and remote host
  uint16_t dlPort = 10000;
  uint16_t ulPort = 20000;

  // randomize a bit start times to avoid simulation artifacts
  // (e.g., buffer overflows due to packet transmissions happening
  // exactly at the same time)
  Ptr<UniformRandomVariable> startTimeSeconds = CreateObject<UniformRandomVariable> ();
  startTimeSeconds->SetAttribute ("Min", DoubleValue (0));
  startTimeSeconds->SetAttribute ("Max", DoubleValue (0.010));

  for (uint32_t u = 0; u < numberOfUes; ++u)
    {
      Ptr<Node> ue = ueNodes.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ue->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);

      for (uint32_t b = 0; b < numBearersPerUe; ++b)
        {
          ++dlPort;
          ++ulPort;

          ApplicationContainer clientApps;
          ApplicationContainer serverApps;

          NS_LOG_LOGIC ("installing UDP DL app for UE " << u);
          UdpClientHelper dlClientHelper (ueIpIfaces.GetAddress (u), dlPort);
          clientApps.Add (dlClientHelper.Install (remoteHost));
          PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory",
                                               InetSocketAddress (Ipv4Address::GetAny (), dlPort));
          serverApps.Add (dlPacketSinkHelper.Install (ue));

          NS_LOG_LOGIC ("installing UDP UL app for UE " << u);
          UdpClientHelper ulClientHelper (remoteHostAddr, ulPort);
          clientApps.Add (ulClientHelper.Install (ue));
          PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory",
                                               InetSocketAddress (Ipv4Address::GetAny (), ulPort));
          serverApps.Add (ulPacketSinkHelper.Install (remoteHost));

          Ptr<EpcTft> tft = Create<EpcTft> ();
          EpcTft::PacketFilter dlpf;
          dlpf.localPortStart = dlPort;
          dlpf.localPortEnd = dlPort;
          tft->Add (dlpf);
          EpcTft::PacketFilter ulpf;
          ulpf.remotePortStart = ulPort;
          ulpf.remotePortEnd = ulPort;
          tft->Add (ulpf);
          EpsBearer bearer (EpsBearer::NGBR_VIDEO_TCP_DEFAULT);
          lteHelper->ActivateDedicatedEpsBearer (ueLteDevs.Get (u), bearer, tft);

          Time startTime = Seconds (startTimeSeconds->GetValue ());
          serverApps.Start (startTime);
          clientApps.Start (startTime);

        } // end for b
    }


  // Add X2 inteface
  lteHelper->AddX2Interface (enbNodes);

  // X2-based Handover
  lteHelper->HandoverRequest (Seconds (0.100), ueLteDevs.Get (0), enbLteDevs.Get (0), enbLteDevs.Get (1));

  // Uncomment to enable PCAP tracing
  // p2ph.EnablePcapAll("lena-x2-handover-measures");

  lteHelper->EnablePhyTraces ();
  lteHelper->EnableMacTraces ();
  lteHelper->EnableRlcTraces ();
  lteHelper->EnablePdcpTraces ();
  Ptr<RadioBearerStatsCalculator> rlcStats = lteHelper->GetRlcStats ();
  rlcStats->SetAttribute ("EpochDuration", TimeValue (Seconds (1.0)));
  Ptr<RadioBearerStatsCalculator> pdcpStats = lteHelper->GetPdcpStats ();
  pdcpStats->SetAttribute ("EpochDuration", TimeValue (Seconds (1.0)));

  // connect custom trace sinks for RRC connection establishment and handover notification
  Config::Connect ("/NodeList/*/DeviceList/*/LteEnbRrc/ConnectionEstablished",
                   MakeCallback (&NotifyConnectionEstablishedEnb));
  Config::Connect ("/NodeList/*/DeviceList/*/LteUeRrc/ConnectionEstablished",
                   MakeCallback (&NotifyConnectionEstablishedUe));
  Config::Connect ("/NodeList/*/DeviceList/*/LteEnbRrc/HandoverStart",
                   MakeCallback (&NotifyHandoverStartEnb));
  Config::Connect ("/NodeList/*/DeviceList/*/LteUeRrc/HandoverStart",
                   MakeCallback (&NotifyHandoverStartUe));
  Config::Connect ("/NodeList/*/DeviceList/*/LteEnbRrc/HandoverEndOk",
                   MakeCallback (&NotifyHandoverEndOkEnb));
  Config::Connect ("/NodeList/*/DeviceList/*/LteUeRrc/HandoverEndOk",
                   MakeCallback (&NotifyHandoverEndOkUe));
  
  Config::Connect ("/NodeList/*/$ns3::MobilityModel/CourseChange", 
  				   MakeCallback (&CourseChange));
  Config::ConnectWithoutContext ("/NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx", 
  				   MakeCallback(&SinkRxTrace));
  

  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor;
  monitor = flowmon.Install (ueNodes);
  monitor = flowmon.Install (enbNodes);
  monitor = flowmon.Install (remoteHost);
  
  monitor->SetAttribute("JitterBinWidth", ns3::DoubleValue (0.001));
  monitor->SetAttribute("DelayBinWidth", ns3::DoubleValue (0.001));
  monitor->SetAttribute("PacketSizeBinWidth", ns3::DoubleValue(20));
  
  Simulator::Stop (Seconds (simTime));
  Simulator::Run ();

  // Performance evaluation.
  monitor->CheckForLostPackets();
  monitor->SerializeToXmlFile("S2V-L2H.flowmon", true, true);
  
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier());
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin(); i != stats.end (); ++i)
  {    
      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
      
      // Show all information. 
//      NS_LOG_UNCOND ("Flow " << i->first << " (" << t.destinationAddress << " -> " << t.destinationAddress << " )");
//      NS_LOG_UNCOND (" Tx Packets: " << i->second.txPackets);
//      NS_LOG_UNCOND (" Rx Packets: " << i->second.rxPackets);
//      double throuhgput = ((i->second.rxBytes*8.0) / (i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds())) / 1024;
//      double numberOfPacketLoss = i->second.txPackets - i->second.rxPackets;
//      double packetLossRate = (numberOfPacketLoss / i->second.txPackets) * 100;
//      NS_LOG_UNCOND (" Throughput: " << throuhgput << "kbit/sec");
//      NS_LOG_UNCOND (" Packet Loss Ratio: " << packetLossRate << "(%)");
      
      // Show partial information.
      if (t.sourceAddress == "1.0.0.2" && t.destinationAddress == "7.0.0.2")
      {
          NS_LOG_UNCOND ("Flow " << i->first << " (" << t.destinationAddress << " -> " << t.destinationAddress << " )");
          NS_LOG_UNCOND (" Tx Packets: " << i->second.txPackets);
          NS_LOG_UNCOND (" Rx Packets: " << i->second.rxPackets);
          throughput = ((i->second.rxBytes*8.0) / (i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds())) / 1024;
          numberOfPacketLoss = i->second.txPackets - i->second.rxPackets;
          packetLossRate = (numberOfPacketLoss / i->second.txPackets) * 100;
          NS_LOG_UNCOND (" Throughput: " << throughput << "kbit/sec");
          NS_LOG_UNCOND (" Packet Loss Ratio: " << packetLossRate << "(%)");
      }
      
      // Show time vs. throughput.
//      if (t.sourceAddress == "1.0.0.2" && t.destinationAddress == "7.0.0.2")
//      {
//          for (int j = i->second.timeFirstRxPacket.GetSeconds(); j < i->second.timeLastRxPacket.GetSeconds(); j++)
//          {
//              double averageBytes = (i->second.rxBytes);
//              NS_LOG_UNCOND ("averageBytes: " << averageBytes);
//          }
//      }
      
      // Calculate Delay.
      if (t.sourceAddress == "1.0.0.2" && t.destinationAddress == "7.0.0.2")
      {
          NS_LOG_DEBUG ("Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")");
          if (i->second.rxPackets > 0)
          {
              std::cout << "Delay Down = " << i->second.delaySum.GetSeconds() / i->second.rxPackets << "\n"; 
          }
      }
      
      if (t.sourceAddress == "7.0.0.2" && t.destinationAddress == "1.0.0.2")
      {
          NS_LOG_DEBUG ("Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")");
          if (i->second.rxPackets > 0)
          {
              std::cout << "Delay Up = " << i->second.delaySum.GetSeconds() / i->second.rxPackets << "\n";
          }
      }
  }
  
  
  
  Simulator::Destroy ();
  return 0;

}
