/*
opyright (c) 2013 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
=======
/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
>>>>>>> 6523bb3dccd0f9140ca6da3947a330f71fbb27e7
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

/* add HeNB code. */
 #include <ns3/buildings-module.h>
 #include <ns3/point-to-point-helper.h>
 #include <ns3/log.h>
 #include <iomanip>
 #include <ios>
 #include <string>
 #include <vector>

 /* include flowmonitor. */
 #include "ns3/lte-helper.h"
 #include "ns3/epc-helper.h"
 #include "ns3/ipv4-global-routing-helper.h"
 #include "ns3/config-store.h"
 #include "ns3/flow-monitor-module.h"
 #include "ns3/lte-global-pathloss-database.h"

int numberOfHandover = 0;
int numberOfSuccessHandover = 0;
int numberOfConnectionEstablishedEnb = 0;
int numberOfConnectionEstablishedUe = 0;
int EstablishedUe_time = 0;
int HandoverStartUe_time = 0;
int HandoverEndOkUe_time = 0;
int handoverTimeSum = 0;

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("LTE-NeNB-coexistence");

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

/* add HeNB code. */
bool AreOverlapping (Box a, Box b)
{
  return !((a.xMin > b.xMax) || (b.xMin > a.xMax) || (a.yMin > b.yMax) || (b.yMin > a.yMax));
}

// Femtocell class.
class FemtocellBlockAllocator
{
public:
  FemtocellBlockAllocator (Box area, uint32_t nApartmentsX, uint32_t nFloors);
  void Create (uint32_t n);
  void Create ();

private:
  bool OverlapsWithAnyPrevious (Box);
  Box m_area;
  uint32_t m_nApartmentsX;
  uint32_t m_nFloors;
  std::list<Box> m_previousBlocks;
  double m_xSize;
  double m_ySize;
  Ptr<UniformRandomVariable> m_xMinVar;
  Ptr<UniformRandomVariable> m_yMinVar;

};

FemtocellBlockAllocator::FemtocellBlockAllocator (Box area, uint32_t nApartmentsX, uint32_t nFloors)
  : m_area (area),
    m_nApartmentsX (nApartmentsX),
    m_nFloors (nFloors),
    m_xSize (nApartmentsX*10 + 20),
    m_ySize (70)
{
  m_xMinVar = CreateObject<UniformRandomVariable> ();
  m_xMinVar->SetAttribute ("Min", DoubleValue (area.xMin));
  m_xMinVar->SetAttribute ("Max", DoubleValue (area.xMax - m_xSize));
  m_yMinVar = CreateObject<UniformRandomVariable> ();
  m_yMinVar->SetAttribute ("Min", DoubleValue (area.yMin));
  m_yMinVar->SetAttribute ("Max", DoubleValue (area.yMax - m_ySize));
}

void 
FemtocellBlockAllocator::Create (uint32_t n)
{
  for (uint32_t i = 0; i < n; ++i)
    {
      Create ();
    }
}

void
FemtocellBlockAllocator::Create ()
{
  Box box;
  uint32_t attempt = 0;
  do 
    {
      NS_ASSERT_MSG (attempt < 100, "Too many failed attemtps to position apartment block. Too many blocks? Too small area?");
      box.xMin = m_xMinVar->GetValue ();
      box.xMax = box.xMin + m_xSize;
      box.yMin = m_yMinVar->GetValue ();
      box.yMax = box.yMin + m_ySize;
      ++attempt;
    }
  while (OverlapsWithAnyPrevious (box));

  NS_LOG_LOGIC ("allocated non overlapping block " << box);
  m_previousBlocks.push_back (box);
  Ptr<GridBuildingAllocator>  gridBuildingAllocator;
  gridBuildingAllocator = CreateObject<GridBuildingAllocator> ();
  gridBuildingAllocator->SetAttribute ("GridWidth", UintegerValue (1));
  gridBuildingAllocator->SetAttribute ("LengthX", DoubleValue (10*m_nApartmentsX)); 
  gridBuildingAllocator->SetAttribute ("LengthY", DoubleValue (10*2));
  gridBuildingAllocator->SetAttribute ("DeltaX", DoubleValue (10));
  gridBuildingAllocator->SetAttribute ("DeltaY", DoubleValue (10));
  gridBuildingAllocator->SetAttribute ("Height", DoubleValue (3*m_nFloors));
  gridBuildingAllocator->SetBuildingAttribute ("NRoomsX", UintegerValue (m_nApartmentsX));
  gridBuildingAllocator->SetBuildingAttribute ("NRoomsY", UintegerValue (2));
  gridBuildingAllocator->SetBuildingAttribute ("NFloors", UintegerValue (m_nFloors));
  gridBuildingAllocator->SetAttribute ("MinX", DoubleValue (box.xMin + 10));
  gridBuildingAllocator->SetAttribute ("MinY", DoubleValue (box.yMin + 10));
  gridBuildingAllocator->Create (2);
}

bool 
FemtocellBlockAllocator::OverlapsWithAnyPrevious (Box box)
{
  for (std::list<Box>::iterator it = m_previousBlocks.begin (); it != m_previousBlocks.end (); ++it)
    {
      if (AreOverlapping (*it, box))
        {
          return true;
        }
    }
  return false;
}

void 
PrintGnuplottableBuildingListToFile (std::string filename)
{
  std::ofstream outFile;
  outFile.open (filename.c_str (), std::ios_base::out | std::ios_base::trunc);
  if (!outFile.is_open ())
    {
      NS_LOG_ERROR ("Can't open file " << filename);
      return;
    }
  uint32_t index = 0;
  for (BuildingList::Iterator it = BuildingList::Begin (); it != BuildingList::End (); ++it)
    {
      ++index;
      Box box = (*it)->GetBoundaries ();
      outFile << "set object " << index
              << " rect from " << box.xMin  << "," << box.yMin
              << " to "   << box.xMax  << "," << box.yMax
              << " front fs empty "
              << std::endl;
    }
}

void 
PrintGnuplottableUeListToFile (std::string filename)
{
  std::ofstream outFile;
  outFile.open (filename.c_str (), std::ios_base::out | std::ios_base::trunc);
  if (!outFile.is_open ())
    {
      NS_LOG_ERROR ("Can't open file " << filename);
      return;
    }
  for (NodeList::Iterator it = NodeList::Begin (); it != NodeList::End (); ++it)
    {
      Ptr<Node> node = *it;
      int nDevs = node->GetNDevices ();
      for (int j = 0; j < nDevs; j++)
        {
          Ptr<LteUeNetDevice> uedev = node->GetDevice (j)->GetObject <LteUeNetDevice> ();
          if (uedev)
            {
              Vector pos = node->GetObject<MobilityModel> ()->GetPosition ();
              outFile << "set label \"" << uedev->GetImsi ()
                      << "\" at "<< pos.x << "," << pos.y << " left font \"Helvetica,4\" textcolor rgb \"grey\" front point pt 1 ps 0.3 lc rgb \"grey\" offset 0,0"
                      << std::endl;
            }
        }
    }
}

void 
PrintGnuplottableEnbListToFile (std::string filename)
{
  std::ofstream outFile;
  outFile.open (filename.c_str (), std::ios_base::out | std::ios_base::trunc);
  if (!outFile.is_open ())
    {
      NS_LOG_ERROR ("Can't open file " << filename);
      return;
    }
  for (NodeList::Iterator it = NodeList::Begin (); it != NodeList::End (); ++it)
    {
      Ptr<Node> node = *it;
      int nDevs = node->GetNDevices ();
      for (int j = 0; j < nDevs; j++)
        {
          Ptr<LteEnbNetDevice> enbdev = node->GetDevice (j)->GetObject <LteEnbNetDevice> ();
          if (enbdev)
            {
              Vector pos = node->GetObject<MobilityModel> ()->GetPosition ();
              outFile << "set label \"" << enbdev->GetCellId ()
                      << "\" at "<< pos.x << "," << pos.y
                      << " left font \"Helvetica,4\" textcolor rgb \"white\" front  point pt 2 ps 0.3 lc rgb \"white\" offset 0,0"
                      << std::endl;
            }
        }
    }
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
  // LogLevel logLevel = (LogLevel)(LOG_PREFIX_ALL | LOG_LEVEL_ALL);

  // LogComponentEnable ("LteHelper", logLevel);
  // LogComponentEnable ("EpcHelper", logLevel);
  // LogComponentEnable ("EpcEnbApplication", logLevel);
  // LogComponentEnable ("EpcX2", logLevel);
  // LogComponentEnable ("EpcSgwPgwApplication", logLevel);

  // LogComponentEnable ("LteEnbRrc", logLevel);
  // LogComponentEnable ("LteEnbNetDevice", logLevel);
  // LogComponentEnable ("LteUeRrc", logLevel);
  // LogComponentEnable ("LteUeNetDevice", logLevel);
  // LogComponentEnable ("A2A4RsrqHandoverAlgorithm", logLevel);
  // LogComponentEnable ("A3RsrpHandoverAlgorithm", logLevel);

  uint16_t numberOfUes = 1;
  uint16_t numberOfEnbs = 2;
  uint16_t numBearersPerUe = 1;
  double distance = 500.0; // m
  double yForUe = 500.0;   // m
  double speed = 0;       // m/s
  // double simTime = (double)(numberOfEnbs + 1) * distance / speed; // 1500 m / 20 m/s = 75 secs
  double simTime = 20;
  double enbTxPowerDbm = 46.0;
  double TTT = 40.0; // 0, 256, 5120, 
  double Hyst = 4.0;
  double ber = 0.00005;

  // change some default attributes so that they are reasonable for
  // this scenario, but do this before processing command line
  // arguments, so that the user is allowed to override these settings
  Config::SetDefault("ns3::LteAmc::AmcModel", EnumValue(LteAmc::PiroEW2010));
  Config::SetDefault("ns3::LteAmc::Ber", DoubleValue(ber));
  Config::SetDefault("ns3::LteSpectrumPhy::CtrlErrorModelEnabled", BooleanValue(false));
  Config::SetDefault("ns3::LteSpectrumPhy::DataErrorModelEnabled", BooleanValue(false));
  Config::SetDefault("ns3::LteEnbRrc::EpsBearerToRlcMapping", EnumValue(LteHelper::RLC_UM_ALWAYS));
  Config::SetDefault("ns3::LteEnbRrc::SrsPeriodicity", UintegerValue(320)); // 160.
  Config::SetDefault("ns3::LteEnbMac::NumberOfRaPreambles", UintegerValue(10));

  Config::SetDefault ("ns3::UdpClient::Interval", TimeValue (MilliSeconds (10)));
  Config::SetDefault ("ns3::UdpClient::MaxPackets", UintegerValue (1000000));
  Config::SetDefault ("ns3::LteHelper::UseIdealRrc", BooleanValue (true));

  // Command line arguments
  CommandLine cmd;
  cmd.AddValue ("simTime", "Total duration of the simulation (in seconds)", simTime);
  cmd.AddValue ("speed", "Speed of the UE (default = 20 m/s)", speed);
  cmd.AddValue ("enbTxPowerDbm", "TX power [dBm] used by HeNBs (defalut = 46.0)", enbTxPowerDbm);
  cmd.AddValue("TTT", "Time-to-Trigger (default = 246", TTT);
  cmd.AddValue("Hyst", "Hysteresis (default = 4.0", Hyst);

  cmd.Parse (argc, argv);


  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);
  lteHelper->SetSchedulerType ("ns3::RrFfMacScheduler");

  // lteHelper->SetHandoverAlgorithmType ("ns3::A2A4RsrqHandoverAlgorithm");
  // lteHelper->SetHandoverAlgorithmAttribute ("ServingCellThreshold",
  //                                           UintegerValue (30));
  // lteHelper->SetHandoverAlgorithmAttribute ("NeighbourCellOffset",
  //                                           UintegerValue (1));

   lteHelper->SetHandoverAlgorithmType ("ns3::A3RsrpHandoverAlgorithm");
   lteHelper->SetHandoverAlgorithmAttribute ("Hysteresis",
                                             DoubleValue (3.0));
   lteHelper->SetHandoverAlgorithmAttribute ("TimeToTrigger",
                                             TimeValue (MilliSeconds (256)));

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
  // Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1); // Need to be used if you want to UL flow.


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
  enbNodes.Create (numberOfEnbs);
  ueNodes.Create (numberOfUes);

  /* add HeNB code. */
  Box macroUeBox; 
  macroUeBox = Box (0, 150, 0, 150, 1.5, 1.5);

  FemtocellBlockAllocator blockAllocator (macroUeBox, 10, 1); // nApartmentsX = 1; nFloors = 1.
  blockAllocator.Create (1);

  // uint32_t nHomeEnbs = round (4 * 1 * 10 * 1 * 0.2 * 0.5);
  uint32_t nHomeEnbs = 2;

  NodeContainer homeEnbs;
  homeEnbs.Create (nHomeEnbs);
  // Install Mobility Model in homeEnbs.
  Ptr<ListPositionAllocator> homePositionAlloc = CreateObject<ListPositionAllocator> ();
  for (uint16_t i = 0; i < nHomeEnbs; i++) {
    Vector homeEnbsPosition (distance * (i+1) - 100, distance+1000, 0);
    homePositionAlloc->Add (homeEnbsPosition);
  }
  // for (uint16_t i = 0; i < 6; i++) {
  //   Vector homeEnbsPosition (distance * (i+1) + 100, distance +200, 0);
  //   homePositionAlloc->Add(homeEnbsPosition);
  // }
  MobilityHelper homeEnbsMobility;
  homeEnbsMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  homeEnbsMobility.SetPositionAllocator (homePositionAlloc);
  homeEnbsMobility.Install (homeEnbs);
  // Install homeEnbs' devie interface. 
  NetDeviceContainer homeEnbDevs  = lteHelper->InstallEnbDevice (homeEnbs);

  // Install Mobility Model in eNB
  Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator> ();
  for (uint16_t i = 0; i < numberOfEnbs; i++)
    {
      Vector enbPosition (distance * (i + 1), distance, 0);
      enbPositionAlloc->Add (enbPosition);
    }
  MobilityHelper enbMobility;
  enbMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  enbMobility.SetPositionAllocator (enbPositionAlloc);
  enbMobility.Install (enbNodes);

  // Install Mobility Model in UE
  MobilityHelper ueMobility;
  ueMobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
  ueMobility.Install (ueNodes);
  ueNodes.Get (0)->GetObject<MobilityModel> ()->SetPosition (Vector (2000, yForUe+100, 0));
  ueNodes.Get (0)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (speed, 0, 0));
  // for (uint32_t u = 0; u < ueNodes.GetN(); u++) {
  //   ueNodes.Get(u)->GetObject<MobilityModel> ()->SetPosition (Vector (3000+u*10, yForUe, 0));
  //   ueNodes.Get (u)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (speed, 0, 0));
  // }

  // Install LTE Devices in eNB and UEs
  Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (enbTxPowerDbm));
  NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);

  // Install the IP stack on the UEs
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIfaces;
  ueIpIfaces = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));
  // Assign IP address to UEs, and install applications
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<Node> ueNode = ueNodes.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }


  // Attach all UEs to the first eNodeB
  for (uint16_t i = 0; i < numberOfUes; i++)
    {
      lteHelper->Attach (ueLteDevs.Get (i), enbLteDevs.Get (0));
      // lteHelper->Attach (ueLteDevs.Get (i), homeEnbDevs.Get(0));
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
          
          // setup the dlClientHelper.
          dlClientHelper.SetAttribute("Interval", TimeValue(MilliSeconds(1))); // default = 1.
          dlClientHelper.SetAttribute("MaxPackets", UintegerValue(100000));  // default = 100000, 1000000
          dlClientHelper.SetAttribute("PacketSize", UintegerValue(1500));

          clientApps.Add (dlClientHelper.Install (remoteHost));
          PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory",
                                               InetSocketAddress (Ipv4Address::GetAny (), dlPort));
          serverApps.Add (dlPacketSinkHelper.Install (ue));

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
  /* Add HeNB code. */
  lteHelper->AddX2Interface (homeEnbs);
  for (uint32_t i = 0; i < numberOfEnbs; i++) {
  //   lteHelper->AddX2Interface(enbNodes.Get(i), enbNodes.Get(i+1));
    for (uint32_t j = 0; j < nHomeEnbs; j++) {
      lteHelper->AddX2Interface(enbNodes.Get(i), homeEnbs.Get(j));
  //     lteHelper->AddX2Interface(homeEnbs.Get(j), homeEnbs.Get(j+1));
    }
  }
  // lteHelper->AddX2Interface(enbNodes.Get(0), homeEnbs.Get(0));
  // lteHelper->AddX2Interface(enbNodes.Get(1), homeEnbs.Get(0));
  /* We need to figure out the issue of this class, "AddX2Interface". */
  // lteHelper->AddX2Interface (enbLteDevs, homeEnbDevs);

  // X2-based Handover
  //lteHelper->HandoverRequest (Seconds (0.100), ueLteDevs.Get (0), enbLteDevs.Get (0), enbLteDevs.Get (1));

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

  /* flow-monitor. */
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.Install (ueNodes); 

  flowmon.Install (remoteHost);
  // ueNodes.
  flowmon.Install (ueNodes.Get(0));
  // flowmon.Install (ueNodes.Get(1));
  // for (uint32_t u = 0; u < ueNodes.GetN(); u++) {
  //   flowmon.Install (ueNodes.Get(u));
  // }
  // enbNodes.
  flowmon.Install (enbNodes.Get(0));
  flowmon.Install (enbNodes.Get(1));
  // homeEnbs
  // flowmon.Install (homeEnbs.Get(0));
  // flowmon.Install (homeEnbs.Get(1));

  monitor->SetAttribute("JitterBinWidth", ns3::DoubleValue(0.001));
  monitor->SetAttribute("DelayBinWidth", ns3::DoubleValue(0.001));
  monitor->SetAttribute("PacketSizeBinWidth", ns3::DoubleValue(20));

  Simulator::Stop (Seconds (simTime));
  Simulator::Run ();

  // recode the eperiment results. 
  // monitor->SerializeToXmlFile ("LTE-NeNB-coexistence.flowmon", true, true);

  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier());
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

  double throughput, av_throughput = 0.0;
  double numberOfPacketLoss = 0.0;
  double packetLossRate, minPacketLossRate, maxPacketLossRate, av_TotalPacketLossRate = 0.0;
  double totalReceivedPacket, totalSendPacket = 0.0;
  double total_throughput = 0.0;
  double delay_down = 0.0;
  double delay_up = 0.0;
  double avgHandovers = 0.0;

  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin(); i != stats.end(); ++i) 
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);

        NS_LOG_UNCOND("Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << " )");
        NS_LOG_UNCOND(" Tx Packets: " << i->second.txPackets);
        NS_LOG_UNCOND(" Rx Packets: " << i->second.rxPackets);
        throughput = ((i->second.rxBytes * 8.0) / ((i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds()))) / 1024;
        NS_LOG_UNCOND(" Throughputs: " << throughput << " kbps"); // Throuhgput.
        numberOfPacketLoss = i->second.txPackets - i->second.rxPackets;
        packetLossRate = (numberOfPacketLoss / i->second.txPackets)*100;
        NS_LOG_UNCOND(" Packet Loss ratio: " << packetLossRate << "(%)"); // Packet Loss Ratio.
        
        // NEW throughput to calculate the correct of throughput. 
        if (t.sourceAddress == Ipv4Address("1.0.0.2") && t.destinationAddress == Ipv4Address("7.0.0.2")) 
        {
          NS_LOG_UNCOND ("(NEW Flow ID: " << i->first << "Scource ADDr: " << t.sourceAddress << "; Dest ADDr: " << t.destinationAddress);
            NS_LOG_UNCOND ("Tx Packet = " << i->second.txPackets);
            NS_LOG_UNCOND ("Rx Packets = " << i->second.rxPackets);
            NS_LOG_UNCOND ("Throuhgput: " << i->second.rxBytes * 8.0 / i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds() /1024 << "kbps");
        }
        
        // // show PLR vs. time.
        // if (i->first == 1)
        // {
        //     FILE *plrFile;
        //     plrFile = fopen("PLR_vs_time.txt", "a");
        //     if (plrFile == NULL || plrFile != NULL) 
        //     {
        //         std::cout << "Time = " << i->first << "; PLR = " << i->second.rxPackets << std::endl;
        //         fprintf(plrFile, "Time = %d; PLR = %d\n", i->first, (int)i->second.rxPackets);
        //         fclose(plrFile);
        //     }
        // }
        // the min of PLR & the max of PLR.
        if (minPacketLossRate > packetLossRate && i->first <= numberOfUes)
        {
            minPacketLossRate = packetLossRate;
        }
        if (maxPacketLossRate < packetLossRate && i->first <= numberOfUes)
        {
            maxPacketLossRate = packetLossRate;
        }
        // average packet loss ratio.
        totalReceivedPacket += i->second.rxPackets;
        totalSendPacket += i->second.txPackets;
        av_TotalPacketLossRate = 100 * ((totalSendPacket - totalReceivedPacket) / totalSendPacket);
        // std::cout << "totalReceivedPacket = " << totalReceivedPacket << "; totalSendPacket = " << totalSendPacket << std::endl;
        // std::cout << "av_TotalPacketLossRate = " << av_TotalPacketLossRate << std::endl;
        // number of packet loss. (Total)
        numberOfPacketLoss += numberOfPacketLoss;
        // average throughput.
        throughput = ((i->second.rxBytes * 8.0) / ((i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds()))) / 1024;
        if (i->first != 1) {
            total_throughput += throughput;
            // std::cout << "total_throughput = " << total_throughput << std::endl;
            if (i->first == numberOfUes)
            {
                av_throughput = total_throughput / i->first;
                // std::cout << "av_throughput = " << av_throughput << std::endl;
            }
        } else {
            total_throughput = throughput;
            // std::cout << "throughput = " << throughput << std::endl;
        }
        // average delay.
        if (i->second.rxPackets > 0)
        {
            delay_down = i->second.delaySum.GetSeconds() / i->second.rxPackets;
            // std::cout << "Delay Down = " << i->second.delaySum.GetSeconds() / i->second.rxPackets << "\n";
        }   
        if (i->second.rxPackets > 0)
        {
            delay_up = i->second.delaySum.GetSeconds() / i->second.rxPackets;
            // std::cout << "Delay Up = " << i->second.delaySum.GetSeconds() / i->second.rxPackets << "\n";
        }
        // average handover time. 
        avgHandovers = numberOfSuccessHandover / simTime;
        // handover failure ratio.
    }

    // recod all of the performance evaluation.
    FILE *pFile;
    pFile = fopen("throughput.txt", "a");
    if (pFile == NULL || pFile != NULL) {
         fprintf(pFile, "Speed=%f; TTT=%f; Hyst=%f; av_Throughputs=%f (kbits/sec); av_PLR=%f (minPLR = %f, maxPLR = %f); numberOfPacketLoss=%f ; Delay Down=%f; Delay Up=%f; avgHandovers = %f (numberOfSuccessHandover=%d)\n", speed, TTT, Hyst, av_throughput, av_TotalPacketLossRate, minPacketLossRate, maxPacketLossRate, numberOfPacketLoss, delay_down, delay_up, avgHandovers, numberOfSuccessHandover);
//        fprintf(pFile, "Speed=%f; TTT=%f; Hyst=%f; av_Throughputs=%f (throughput = %f  kbits/sec); av_PLR=%f (minPLR = %f, maxPLR = %f); numberOfPacketLoss=%f (numberOfSuccessHandover=%f)\n", speed, TTT, Hyst, av_throughput, throughput, av_TotalPacketLossRate, minPacketLossRate, maxPacketLossRate, numberOfPacketLoss, numberOfSuccessHandover);
        fclose(pFile);
    }

  // GtkConfigStore config;
  // config.ConfigureAttributes ();

  Simulator::Destroy ();
  return 0;

}




