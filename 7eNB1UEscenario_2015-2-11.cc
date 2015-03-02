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

#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"
//#include "ns3/gtk-config-store.h"
#include "ns3/flow-monitor-module.h"

#include <iomanip>
#include <cstdlib>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("PacektLossCounter"); // please type the command of "NS_LOG=PacketLossCounter".

int numberOfHandover = 0;
int numberOfSuccessHandover = 0;
int numberOfConnectionEstablishedEnb = 0;
int numberOfConnectionEstablishedUe = 0;

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
  FILE *hFile;
  hFile = fopen ("numberofhandover.txt", "a");
  numberOfHandover = numberOfHandover + 1;
  if (hFile == NULL || hFile != NULL) {
    fprintf(hFile, "Number of Handover = %d\n", numberOfHandover);
    fclose (hFile);
  }
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

  FILE *sFile;
  sFile = fopen ("numberOfSuccessHandover.txt", "a");
  numberOfSuccessHandover = numberOfSuccessHandover + 1;
  if (sFile == NULL || sFile != NULL) {
    // fprintf(sFile, "Number of Successful Handover = %d\n", numberOfSuccessHandover);
    fclose (sFile);
  }
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
  numberOfConnectionEstablishedEnb = numberOfConnectionEstablishedEnb + 1;
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

/**
  ***************************************************************
  Calculate the performance of throughput, delay, etc necessarily. 
  ***************************************************************
 */

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
  std::cout << Simulator::Now () << ", model=" << mobility << ", POS: x=" << ", y=" << pos.y
            << ", z=" << pos.z << "; VEL:" << vel.x << ", y=" << vel.y
            << ", z=" << vel.z << std::endl;
}


/** *************************************************************************
   Mobility node Install.
   **************************************************************************
 */

void Mobility_R2L (uint16_t numberOfUes_R2L, ns3::NodeContainer ueNodes_R2L, double yForUe)
{
  MobilityHelper ueMobility_R2L; // Right-side to Left-side.
  ueMobility_R2L.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");

  ueMobility_R2L.Install (ueNodes_R2L);
  for (uint16_t i = 0; i < numberOfUes_R2L; i++)
  {
    ueNodes_R2L.Get(i)->GetObject<MobilityModel>()->SetPosition (Vector (1500, yForUe-10*i, 0));
    ueNodes_R2L.Get(i)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (-rand()%50, 0, 0));
  }
}

void Mobility_L2R (uint16_t numberOfUes_L2R, ns3::NodeContainer ueNodes_L2R, double yForUe)
{
  MobilityHelper ueMobility_L2R; // Left-side to Right-side.
  ueMobility_L2R.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");

  ueMobility_L2R.Install (ueNodes_L2R);
  for (uint16_t i = 0; i < numberOfUes_L2R; i++)
  {
    ueNodes_L2R.Get(i)->GetObject<MobilityModel>()->SetPosition (Vector (0, yForUe-10*i, 0));
    ueNodes_L2R.Get(i)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (rand()%50, 0, 0));
  }
}

void Mobility_RandomR2L (uint16_t numberOfUes_RandomR2L, ns3::NodeContainer ueNodes_randomR2L)
{
  MobilityHelper ueMobility_randomR2L; // Right-side to Left-side randomly.
  ueMobility_randomR2L.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");

  ueMobility_randomR2L.Install (ueNodes_randomR2L);
  for (uint16_t i = 0; i < numberOfUes_RandomR2L; i++)
  {
    ueNodes_randomR2L.Get(i)->GetObject<MobilityModel>()->SetPosition (Vector (1000+i*100, 400, 0));
    ueNodes_randomR2L.Get(i)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (0, 0, 0));
  }
}

void Mobility_RandomL2R (uint16_t numberOfUes_RandomL2R, ns3::NodeContainer ueNodes_randomL2R, double yForUe)
{
  MobilityHelper ueMobility_randomL2R; // Left-side to Right-side randomly.
  ueMobility_randomL2R.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");

  ueMobility_randomL2R.Install (ueNodes_randomL2R);
  for (uint16_t i = 0; i < numberOfUes_RandomL2R; i++)
  {
    ueNodes_randomL2R.Get(i)->GetObject<MobilityModel>()->SetPosition (Vector ((100*i), yForUe, 0));
    ueNodes_randomL2R.Get(i)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector ((-1)^(i+1)*(rand()%50), 0, 0));
  }
}

/** *************************************
   Main processing.
  ***************************************
 */
int 
main (int argc, char *argv[])
{
  /* Parameters. */
  uint16_t numberOfUes = 2;
  uint16_t numberOfEnbs = 7;
  uint16_t numBearersPerUe = 1;
  double distance = 1000.0; // m
//  double yForUe = 500.0;   // m
  double speed = 33.33;       // m/s
//  double simTime = (double)(numberOfEnbs + 1) * distance / speed; // 1500 m / 20 m/s = 75 secs
  double simTime = 100; // 1500 m / 20 m/s = 75 secs
  double enbTxPowerDbm = 46.0; // 46.0
  double TTT = 256.0; // 0, 256, 5120; 40-60ms, 60-100ms, 100-160ms
  double Hyst = 2.0; 

  double ber = 0.00005;
  // uint16_t numberOfUes_R2L = 10; /* Nodes of mobility model. */
  // uint16_t numberOfUes_L2R = 10;
  // uint16_t numberOfUes_RandomR2L = 10;
  // uint16_t numberOfUes_RandomL2R = 10;

  /* Config default. */
  Config::SetDefault ("ns3::LteAmc::AmcModel", EnumValue (LteAmc::PiroEW2010));
  Config::SetDefault ("ns3::LteAmc::Ber", DoubleValue (ber));
  Config::SetDefault ("ns3::LteSpectrumPhy::CtrlErrorModelEnabled", BooleanValue (false));
  Config::SetDefault ("ns3::LteSpectrumPhy::DataErrorModelEnabled", BooleanValue (false));
  Config::SetDefault ("ns3::LteEnbRrc::EpsBearerToRlcMapping",EnumValue(LteHelper::RLC_UM_ALWAYS));

  Config::SetDefault ("ns3::UdpClient::Interval", TimeValue (MilliSeconds (10)));
  Config::SetDefault ("ns3::UdpClient::MaxPackets", UintegerValue (1000000));
  Config::SetDefault ("ns3::LteHelper::UseIdealRrc", BooleanValue (true));

  /* Command line arguments */
  CommandLine cmd;
  cmd.AddValue ("speed", "Speed of the UE (default = 20 m/s)", speed);
  cmd.AddValue ("TTT", "Time-to-Trigger (default = 246", TTT);
  cmd.AddValue ("Hyst", "Hysteresis (default = 2.0", Hyst);
  // cmd.AddValue ("simTime", "Total duration of the simulation (in seconds)", simTime);
  // cmd.AddValue ("enbTxPowerDbm", "TX power [dBm] used by HeNBs (defalut = 46.0)", enbTxPowerDbm); // check TX power.
  cmd.Parse (argc, argv);

  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);
  /* MAC Layer Install. */
  lteHelper->SetSchedulerType ("ns3::RrFfMacScheduler"); // define scheduler type.
  // lteHelper->SetSchedulerType ("ns3::FdMtFfMacScheduler"); // FD-MT scheduler.
  // lteHelper->SetSchedulerType ("ns3::TdMtFfMacScheduler"); // TD-MT scheduler.
  // lteHelper->SetSchedulerType ("ns3::TtaFfMacScheduler"); // TTA scheduler.
  // lteHelper->SetSchedulerType ("ns3::FdBetFfMacScheduler"); // FD-BET scheduler.
  // lteHelper->SetSchedulerType ("ns3::TdBetFfMacScheduler"); // TD-BET scheduler.
  // lteHelper->SetSchedulerType ("ns3::FdTbfqFfMacScheduler"); // FD-TBFQ scheduler.
  // lteHelper->SetSchedulerType ("ns3::TdTbfqFfMacScheduler"); // TD-TBFQ scheduler.
  // lteHelper->SetSchedulerType ("ns3::PssFfMacScheduler"); // PSS scheduler.

  /* Handover algorithm Install. */  
  lteHelper->SetHandoverAlgorithmType ("ns3::A3RsrpHandoverAlgorithm"); // Use A3 RSRP Handover Algorithm.
  lteHelper->SetHandoverAlgorithmAttribute ("Hysteresis",
                                             DoubleValue (Hyst)); // default = 1.0
  lteHelper->SetHandoverAlgorithmAttribute ("TimeToTrigger",
                                             TimeValue (MilliSeconds (TTT))); // 0, 256, 5120; 40-60ms, 60-100ms, 100-160ms
  // lteHelper->SetHandoverAlgorithmType ("ns3::A2A4RsrqHandoverAlgorithm"); // Use A2A4 RSRQ Handover algorithm.
  // lteHelper->SetHandoverAlgorithmAttribute ("ServingCellThreshold",
  //                                           UintegerValue (30));
  // lteHelper->SetHandoverAlgorithmAttribute ("NeighbourCellOffset",
  //                                           UintegerValue (1));
  // lteHelper->SetHandoverAlgorithmType ("ns3::NoOpHandoverAlgorithm"); // Don't use handover algorithm.

  Ptr<Node> pgw = epcHelper->GetPgwNode ();

  // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s"))); // 100Gb/s
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  // Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1); // Need to be used if you want to UL flow.

  /* Routing of the Internet Host (towards the LTE network) */
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1); // interface 0 is localhost, 1 is the p2p device

  /*
   * Network topology:
   *
   *
   *      |                         d                   d
   *      |     ----------x-------------------x-------------------x---------
   *      |             eNodeB              eNodeB              eNodeB
   *      |
   *      |
   *      |
   *      |     (0, yForUe, 0)
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
  // NodeContainer ueNodes_R2L;
  // NodeContainer ueNodes_L2R;
  // NodeContainer ueNodes_randomR2L;
  // NodeContainer ueNodes_randomL2R;
  NodeContainer enbNodes;

  enbNodes.Create (numberOfEnbs);
  ueNodes.Create (numberOfUes);
  // ueNodes_R2L.Create (numberOfUes_R2L);
  // ueNodes_L2R.Create (numberOfUes_L2R);
  // ueNodes_randomR2L.Create (numberOfUes_RandomR2L);
  // ueNodes_randomL2R.Create (numberOfUes_RandomL2R);

  // Install Mobility Model in eNB
  Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator> (); // declare enbPositionAlloc pointer.
  for (uint16_t i = 0; i < numberOfEnbs; i++)
    {
      Vector enbPosition (distance * (i + 1), distance, 0); // orgin.

      if (i == 0 || i%2 != 0)
      {
        Vector enbPosition (250 + distance*i, 1000, 0);
        enbPositionAlloc->Add (enbPosition);
      }
      else if (i%2 == 0)
      {
        Vector enbPosition (distance*i, 0, 0);
        enbPositionAlloc->Add (enbPosition);
      }
      // enbPositionAlloc->Add (enbPosition); // orgin
    }

  MobilityHelper enbMobility; // enb mobility helper.
  enbMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  enbMobility.SetPositionAllocator (enbPositionAlloc);
  enbMobility.Install (enbNodes);

  /* Install Mobility Model in UE. */
  MobilityHelper ueMobility;
  ueMobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
  ueMobility.Install (ueNodes);
  // for 1 UE node.
  // ueNodes.Get (0)->GetObject<MobilityModel> ()->SetPosition (Vector (0, yForUe, 0));
  // ueNodes.Get (0)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (speed, 0, 0));
  
  // for several UE node.
  for (uint16_t i=0; i<numberOfUes; i++)
  {
    ueNodes.Get(i)->GetObject<MobilityModel>()->SetPosition (Vector (0, 400+i, 0));
  	ueNodes.Get(i)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (speed, 0, 0)); 
  }

  /* Mobility Model (Random) */
  // Mobility_R2L (numberOfUes_R2L, ueNodes_R2L, yForUe);
  // Mobility_L2R (numberOfUes_L2R, ueNodes_L2R, yForUe);
  // Mobility_RandomR2L(numberOfUes_RandomR2L, ueNodes_randomR2L);
  // Mobility_RandomL2R (numberOfUes_RandomL2R, ueNodes_randomL2R, yForUe);



  // Install LTE Devices in eNB and UEs
  Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (enbTxPowerDbm));
  NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);
  // NetDeviceContainer ueLteDevs_R2L = lteHelper->InstallUeDevice (ueNodes_R2L); 

  // Install the IP stack on the UEs
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIfaces;
  ueIpIfaces = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));
  // internet.Install (ueNodes_R2L);
  // Ipv4InterfaceContainer ueIpIfaces_R2L;
  // ueIpIfaces = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs_R2L));

  // Assign IP address to UEs, and install applications
  // unicast, multicast application. 
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<Node> ueNode = ueNodes.Get (u);
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }


  // Attach all UEs to the first eNodeB
  for (uint16_t i = 0; i < numberOfUes; i++)
    {
      lteHelper->Attach (ueLteDevs.Get (i), enbLteDevs.Get (0)); // attach one or more UEs to a single eNodeB.
      // lteHelper->Attach (ueLteDevs.Get(i)); // attach one or more UEs to a strongest cell.
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
  // startTimeSeconds->SetAttribute ("Max", DoubleValue (100.0));

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

          // setup the dlClientHelper.
          dlClientHelper.SetAttribute ("Interval", TimeValue(MilliSeconds(1)));
          dlClientHelper.SetAttribute ("MaxPackets", UintegerValue(100000));
          dlClientHelper.SetAttribute ("PacketSize", UintegerValue(1500));

          clientApps.Add (dlClientHelper.Install (remoteHost));
          PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory",
                                               InetSocketAddress (Ipv4Address::GetAny (), dlPort));
          serverApps.Add (dlPacketSinkHelper.Install (ue));

          // uplink
          // NS_LOG_LOGIC ("installing UDP UL app for UE " << u);
          // UdpClientHelper ulClientHelper (remoteHostAddr, ulPort);
          // clientApps.Add (ulClientHelper.Install (ue));
          // PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory",
          //                                      InetSocketAddress (Ipv4Address::GetAny (), ulPort));
          // serverApps.Add (ulPacketSinkHelper.Install (remoteHost));

          Ptr<EpcTft> tft = Create<EpcTft> ();
          EpcTft::PacketFilter dlpf;
          dlpf.localPortStart = dlPort;
          dlpf.localPortEnd = dlPort;
          tft->Add (dlpf);
          // uplink
          // EpcTft::PacketFilter ulpf;
          // ulpf.remotePortStart = ulPort;
          // ulpf.remotePortEnd = ulPort;
          // tft->Add (ulpf);

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
  // lteHelper->HandoverRequest (Seconds (0.100), ueLteDevs.Get (0), enbLteDevs.Get (1), enbLteDevs.Get (2));
  // lteHelper->HandoverRequest (Seconds (0.100), ueLteDevs.Get (0), enbLteDevs.Get (2), enbLteDevs.Get (3));
  // std::cout << "enbLteDevs = " << enbLteDevs.Get(0) << std::endl;

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
  // Config::Connect ("/NodeList/*/DeviceList/*/LteUeRrc/testPrint", MakeCallback (&testPrint));

  FlowMonitorHelper flowmon; // flow-monitor.
  // Ptr<FlowMonitor> monitor = flowmon.InstallAll();
  Ptr<FlowMonitor> monitor = flowmon.Install(ueNodes); // UE flow monitor.
  flowmon.Install(remoteHost);
  flowmon.Install(ueNodes.Get(0));
  flowmon.Install(ueNodes.Get(1));
  flowmon.Install(enbNodes.Get(0));
  flowmon.Install(enbNodes.Get(1));

  monitor->SetAttribute("JitterBinWidth", ns3::DoubleValue(0.001)); // 0.001
  monitor->SetAttribute("DelayBinWidth", ns3::DoubleValue(0.001)); // 0.001
  monitor->SetAttribute("PacketSizeBinWidth", ns3::DoubleValue(20)); // 20

  Simulator::Stop (Seconds (simTime));
  Simulator::Run ();

  // GtkConfigStore config;
  // config.ConfigureAttributes ();

  // calculate packet loss.
  monitor->CheckForLostPackets();
  
  // recode the experiment results.
  monitor->SerializeToXmlFile("2eNB1UEscenario.flowmon", true, true);

  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier()); /* UE flow. */
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();

  double throughput = 0.0;
  double tempRx = 0;
  double tempTx = 0;
  double numberOfPacketLoss = 0.0;
  double packetLossRate = 0.0;

  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin(); i != stats.end(); ++i)
  {
 	
  	Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
  	
  	NS_LOG_UNCOND ("Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << " )");
  	NS_LOG_UNCOND (" Tx Packets: " << i->second.txPackets);
  	NS_LOG_UNCOND (" Rx Packets: " << i->second.rxPackets);
  	throughput = ((i->second.rxBytes*8.0) / ((i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds())))/1024;
  	NS_LOG_UNCOND (" Throughputs: " << throughput << " kbps"); // Throuhgput.
  	numberOfPacketLoss = i->second.txPackets - i->second.rxPackets;
  	packetLossRate = numberOfPacketLoss / i->second.txPackets;
  	NS_LOG_UNCOND (" Packet Loss ratio: " << packetLossRate << "(%)"); // Packet Loss Ratio.


  	if (t.destinationPort == dlPort)
  	{
  		NS_LOG_UNCOND ("******************************************************");
  		NS_LOG_UNCOND ("Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << " )");
  		NS_LOG_UNCOND (" Tx Packets: " << i->second.txPackets);
  		tempTx += i->second.txBytes;
  		NS_LOG_UNCOND (" Rx Packets: " << i->second.rxPackets);
  		tempRx += i->second.rxBytes;
  		throughput = ((i->second.rxBytes*8.0) / ((i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds())))/1024;
  		// NS_LOG_UNCOND ( "timeLastRxPacket = " << i->second.timeLastRxPacket.GetSeconds());
  		// NS_LOG_UNCOND ( "timeFirstRxPacket = " << i->second.timeFirstRxPacket.GetSeconds());
  		NS_LOG_UNCOND (" Throughputs: " << throughput << " kbps"); // Throuhgput.
  		numberOfPacketLoss = i->second.txPackets - i->second.rxPackets;
  		packetLossRate = numberOfPacketLoss / i->second.txPackets;
  		NS_LOG_UNCOND (" Packet Loss ratio: " << packetLossRate << "(%)"); // Packet Loss Ratio.
  		NS_LOG_UNCOND ("******************************************************");
      
                FILE *pFile;
                pFile = fopen ("throughput.txt", "a");
                if (pFile == NULL || pFile != NULL) 
                {
                    fprintf(pFile, "Speed=%f; TTT=%f; Hyst=%f; Throughputs=%f (kbits/sec); PLR=%f; numberOfPacketLoss=%f \n", speed, TTT, Hyst, throughput, packetLossRate, numberOfPacketLoss);
                    fclose (pFile);
                } 
        }
  }   

  Simulator::Destroy ();
  return 0;

}