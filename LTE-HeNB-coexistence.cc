
opyright (c) 2013 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
#include "ns3/lte-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"
//#include "ns3/gtk-config-store.h"
#include "ns3/flow-monitor-module.h"

#include <iomanip>
#include <cstdlib>
#include <map>
#include <iostream>

#include <fstream>
#include "ns3/stats-module.h"
#include "ns3/lte-rrc-sap.h"
//#include <ns3/buildings-mobility-model.h>
//#include <ns3/buildings-propagation-loss-model.h>
//#include <ns3/building.h>
#include "ns3/lte-global-pathloss-database.h"

using namespace ns3;

// please type the command of "NS_LOG=PacketLossCounter".
//NS_LOG_COMPONENT_DEFINE ("2eNB1UEscenario");
NS_LOG_COMPONENT_DEFINE("PacektLossCounter");

int numberOfHandover = 0;
int numberOfSuccessHandover = 0;
int numberOfConnectionEstablishedEnb = 0;
int numberOfConnectionEstablishedUe = 0;
int EstablishedUe_time = 0;
int HandoverStartUe_time = 0;
int HandoverEndOkUe_time = 0;
int handoverTimeSum = 0;

using namespace ns3;

void
NotifyConnectionEstablishedUe(std::string context,
        uint64_t imsi,
        uint16_t cellid,
        uint16_t rnti) {
    std::cout << context
            << " UE IMSI " << imsi
            << ": connected to CellId " << cellid
            << " with RNTI " << rnti
            << std::endl;

    EstablishedUe_time = Now().GetSeconds();
//    std::cout << "EstablishedUe time = " << Now().GetSeconds() << std::endl;
}

void
NotifyHandoverStartUe(std::string context,
        uint64_t imsi,
        uint16_t cellid,
        uint16_t rnti,
        uint16_t targetCellId) {
    std::cout << context
            << " UE IMSI " << imsi
            << ": previously connected to CellId " << cellid
            << " with RNTI " << rnti
            << ", doing handover to CellId " << targetCellId
            << std::endl;

    HandoverStartUe_time = Now().GetSeconds();
//    std::cout << "HandoverStartUe time = " << Now().GetSeconds() << std::endl;

    // total handover time.
    numberOfHandover++;
//    std::cout << "totoal numberOfHandover = " << numberOfHandover << std::endl;
}

void
NotifyHandoverEndOkUe(std::string context,
        uint64_t imsi,
        uint16_t cellid,
        uint16_t rnti) {
    std::cout << context
            << " UE IMSI " << imsi
            << ": successful handover to CellId " << cellid
            << " with RNTI " << rnti
            << std::endl;

    HandoverEndOkUe_time = Now().GetSeconds();
//    std::cout << "HandoverEndOkUe time = " << Now().GetSeconds() << "; HandoverStartUe time = " << HandoverStartUe_time << std::endl;
//    std::cout << "Total the time of handover = " << (HandoverEndOkUe_time - HandoverStartUe_time) << std::endl;

    // average time of handover.
    handoverTimeSum += (HandoverEndOkUe_time - HandoverStartUe_time);
//    std::cout << "handoverTimeSum = " << handoverTimeSum << std::endl;

    // total number of successful handover.
    numberOfSuccessHandover++;
//    std::cout << "numberOfSuccessHandover = " << numberOfSuccessHandover << std::endl;

    FILE *sFile;
    sFile = fopen("numberOfSuccessHandover.txt", "a");
    if (sFile == NULL || sFile != NULL) {
        fprintf(sFile, "Number of Successful Handover = %d\n", numberOfSuccessHandover);
        fclose(sFile);
    }
}

void
NotifyConnectionEstablishedEnb(std::string context,
        uint64_t imsi,
        uint16_t cellid,
        uint16_t rnti) {
    std::cout << context
            << " eNB CellId " << cellid
            << ": successful connection of UE with IMSI " << imsi
            << " RNTI " << rnti
            << std::endl;
    numberOfConnectionEstablishedEnb++;
}

void
NotifyHandoverStartEnb(std::string context,
        uint64_t imsi,
        uint16_t cellid,
        uint16_t rnti,
        uint16_t targetCellId) {
    std::cout << context
            << " eNB CellId " << cellid
            << ": start handover of UE with IMSI " << imsi
            << " RNTI " << rnti
            << " to CellId " << targetCellId
            << std::endl;
}

void
NotifyHandoverEndOkEnb(std::string context,
        uint64_t imsi,
        uint16_t cellid,
        uint16_t rnti) {
    std::cout << context
            << " eNB CellId " << cellid
            << ": completed handover of UE with IMSI " << imsi
            << " RNTI " << rnti
            << std::endl;
}

// calculate the performance of throughput, delay, etc necessarily. 
double firstRxTime = -1.0;
double lastRxTime;
uint32_t bytesTotal = 0;

void DevTxTrace(std::string context, Ptr<const Packet> p, Mac48Address address) {
    std::cout << "TX to= " << address << " p: " << *p << std::endl;
}

void SinkRxTrace(Ptr<const Packet> pkt, const Address &addr) {
    if (firstRxTime < 0) {
        firstRxTime = Simulator::Now().GetSeconds();
    }
    lastRxTime = Simulator::Now().GetSeconds();
    bytesTotal += pkt->GetSize();
}

static void CourseChange(std::string foo, Ptr<const MobilityModel> mobility) {
    Vector pos = mobility->GetPosition();
    Vector vel = mobility->GetVelocity();
    std::cout << Simulator::Now() << ", model=" << mobility << ", POS: x=" << ", y=" << pos.y
            << ", z=" << pos.z << "; VEL:" << vel.x << ", y=" << vel.y
            << ", z=" << vel.z << std::endl;
}

void handler (int arg0, int arg1)
{
    std::cout << "handler called with argument arg0=" << arg0 << " and"
            "arg1=" << arg1 << std::endl;
}

int
main(int argc, char *argv[]) {
    uint16_t numberOfUes_fix = 0; //30, 60, 0, 
    uint16_t numberOfUes = 40 + numberOfUes_fix; // including numberOfUes_fix.  // 20.
    uint16_t numberOfEnbs = 7;
    uint16_t numBearersPerUe = 1;
    // uint16_t numBearersPerUe_fix = 1;
    double distance = 1000.0; // m
    double yForUe = 500.0; // m
    double speed = 20; // m/s, 33.33
    //  double simTime = (double)(numberOfEnbs + 1) * distance / speed; // 1500 m / 20 m/s = 75 secs
    double simTime = 30; // 1500 m / 20 m/s = 75 secs
    double enbTxPowerDbm = 46.0; // 46.0
    double TTT = 40.0; // 0, 256, 5120; 40-60ms, 60-100ms, 100-160ms
    double Hyst = 4.0;
    double ber = 0.00005;

    Config::SetDefault("ns3::LteAmc::AmcModel", EnumValue(LteAmc::PiroEW2010));
    Config::SetDefault("ns3::LteAmc::Ber", DoubleValue(ber));
    Config::SetDefault("ns3::LteSpectrumPhy::CtrlErrorModelEnabled", BooleanValue(false));
    Config::SetDefault("ns3::LteSpectrumPhy::DataErrorModelEnabled", BooleanValue(false));
    Config::SetDefault("ns3::LteEnbRrc::EpsBearerToRlcMapping", EnumValue(LteHelper::RLC_UM_ALWAYS));
    Config::SetDefault("ns3::LteEnbRrc::SrsPeriodicity", UintegerValue(320)); // 160.
    Config::SetDefault("ns3::LteEnbMac::NumberOfRaPreambles", UintegerValue(10));

    Config::SetDefault("ns3::UdpClient::Interval", TimeValue(MilliSeconds(10)));
    Config::SetDefault("ns3::UdpClient::MaxPackets", UintegerValue(1000000)); // default = 1000000, 100000
    Config::SetDefault("ns3::LteHelper::UseIdealRrc", BooleanValue(true));

    // Command line arguments
    CommandLine cmd;
    cmd.AddValue("simTime", "Total duration of the simulation (in seconds)", simTime);
    cmd.AddValue("speed", "Speed of the UE (default = 20 m/s)", speed);
    cmd.AddValue("TTT", "Time-to-Trigger (default = 246", TTT);
    cmd.AddValue("Hyst", "Hysteresis (default = 4.0", Hyst);
    // cmd.AddValue ("enbTxPowerDbm", "TX power [dBm] used by HeNBs (defalut = 46.0)", enbTxPowerDbm); // check TX power.

    cmd.Parse(argc, argv);

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
    Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();
    lteHelper->SetEpcHelper(epcHelper);
    lteHelper->SetSchedulerType("ns3::RrFfMacScheduler");
    // setup fading model.
    lteHelper->SetAttribute ("FadingModel", StringValue ("ns3::TraceFadingLossModel"));
    std::ifstream ifTraceFile;
    ifTraceFile.open ("../../src/lte/model/fading-traces/fading_trace_EPA_3kmph.fad", std::ifstream::in);
    if (ifTraceFile.good ())
    {
        lteHelper->SetFadingModelAttribute ("TraceFilename", StringValue ("../../src/lte/model/fading-traces/fading_trace_EPA_3kmph.fad"));
    }
    else 
    {
        lteHelper->SetFadingModelAttribute ("TraceFilename", StringValue ("src/lte/model/fading-traces/fading_trace_EPA_3kmph.fad"));
    }
    lteHelper->SetFadingModelAttribute("TraceLength", TimeValue(Seconds(10.0)));
    lteHelper->SetFadingModelAttribute("SamplesNum", UintegerValue(10000));
    lteHelper->SetFadingModelAttribute("WindowSize", TimeValue(Seconds(0.5)));
    lteHelper->SetFadingModelAttribute("RbNum", UintegerValue(100));
    // setup pathloss model.
//    lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::BuildingsPropagationLossModel"));
    lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::Cost231PropagationLossModel"));

    // A3-RSRP Handover Algorithm.
//    lteHelper->SetHandoverAlgorithmType("ns3::A3RsrpHandoverAlgorithm");
//    lteHelper->SetHandoverAlgorithmAttribute("Hysteresis",
//            DoubleValue(Hyst));
//    lteHelper->SetHandoverAlgorithmAttribute("TimeToTrigger",
//            TimeValue(MilliSeconds(TTT))); // 0, 256, 5120; 40-60ms, 60-100ms, 100-160ms

    // A2A4-RSRQ Handover Algorithm.
    lteHelper->SetHandoverAlgorithmType ("ns3::A2A4RsrqHandoverAlgorithm"); 
    lteHelper->SetHandoverAlgorithmAttribute ("ServingCellThreshold",
                                                    UintegerValue (30));
    lteHelper->SetHandoverAlgorithmAttribute ("NeighbourCellOffset",
                                                   UintegerValue (1));

//     lteHelper->SetHandoverAlgorithmType ("ns3::NoOpHandoverAlgorithm"); 

    Ptr<Node> pgw = epcHelper->GetPgwNode();

    // Create a single RemoteHost
    NodeContainer remoteHostContainer;
    remoteHostContainer.Create(1);

    Ptr<Node> remoteHost = remoteHostContainer.Get(0);
    InternetStackHelper internet;
    internet.Install(remoteHostContainer); // add internet to UE_fix and eNB as follows.

    // Create the Internet
    PointToPointHelper p2ph; // pointToPoint is in epc network; 
    p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate("10Gb/s"))); // default value = 100Gb/s
    p2ph.SetDeviceAttribute("Mtu", UintegerValue(1500));
    p2ph.SetChannelAttribute("Delay", TimeValue(Seconds(0.010)));
    NetDeviceContainer internetDevices = p2ph.Install(pgw, remoteHost);
    Ipv4AddressHelper ipv4h; // install ipv4 helper for pgw.
    ipv4h.SetBase("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign(internetDevices);
    // Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1); // Need to be used if you want to UL flow.

    // Routing of the Internet Host (towards the LTE network)
    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv4> ());
    // interface 0 is localhost, 1 is the p2p device
    remoteHostStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), 1);

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
    NodeContainer enbNodes;
    // NodeContainer ueNodes_fix;
    enbNodes.Create(numberOfEnbs);
    ueNodes.Create(numberOfUes);
    // ueNodes_fix.Create (numberOfUes_fix);

    // Install Mobility Model in eNB
    Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator> (); // declare enbPositionAlloc pointer.
    for (uint16_t i = 0; i < numberOfEnbs; i++) {
        if (i == 0) // 0
        {
            Vector enbPosition(-distance - 500, 1000, 0);
            enbPositionAlloc->Add(enbPosition);
        } else if (i % 2 != 0) // 1, 3, 5
        {
            // Vector enbPosition (-500+(250*(i-1)), distance, 0);
            Vector enbPosition(-500 + (500 * (i - 1)), distance, 0);
            enbPositionAlloc->Add(enbPosition);
        } else if (i % 2 == 0) // 2, 4, 6
        {
            // Vector enbPosition (-750+500*(i/2-1), 0, 0);
            Vector enbPosition(-distance * 2 + (1000 * (i / 2)), 0, 0);
            enbPositionAlloc->Add(enbPosition);
        }
    }
    MobilityHelper enbMobility; // enb mobility helper.
    enbMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    enbMobility.SetPositionAllocator(enbPositionAlloc);
    enbMobility.Install(enbNodes);

    // Install Mobility Model in UE
    MobilityHelper ueMobility; // ue mobility helper.
    ueMobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
    ueMobility.Install(ueNodes);
    // ueNodes.Get (0)->GetObject<MobilityModel> ()->SetPosition (Vector (0, yForUe, 0)); // for 1 UE node.
    // ueNodes.Get (0)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (speed, 0, 0));
    for (uint16_t i = 0; i < numberOfUes; i++) // for several UE node.
    {
        // 20, 
        if (i < 21) { 
            //    	ueMobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
            ueNodes.Get(i)->GetObject<MobilityModel>()->SetPosition(Vector(-1500 + i*speed/2, yForUe, 0)); // -1500
            ueNodes.Get(i)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity(Vector(speed, 0, 0));
        } else if (i > 21 && i < 41) { //19, 40.
//            ueNodes.Get(i)->GetObject<MobilityModel>()->SetPosition(Vector(1300 - i*speed/2, yForUe, 0)); // -1500
//            ueNodes.Get(i)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity(Vector(-speed, 0, 0));
            
            ueNodes.Get(i)->GetObject<MobilityModel>()->SetPosition(Vector(-1500 + i*speed/2, yForUe+20, 0));
            ueNodes.Get(i)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity(Vector(speed, 0, 0));
        }else {
            //    	ueMobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
            ueNodes.Get(i)->GetObject<MobilityModel>()->SetPosition(Vector(1000 - i*speed/2, yForUe, 0)); // 1000
            ueNodes.Get(i)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity(Vector(0, 0, 0));
        }
    }

    // Install LTE Net Devices in eNB and UEs
    Config::SetDefault("ns3::LteEnbPhy::TxPower", DoubleValue(enbTxPowerDbm));
    NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice(enbNodes);
    NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice(ueNodes); // ueNodes' network device 
    // NetDeviceContainer ueLteDevs_ = lteHelper->InstallUeDevice (ueNodes_);
    // NetDeviceContainer ueLteDevs_fix = lteHelper->InstallUeDevice (ueNodes_fix); // ueNodes_fix network device

    // Install the IP stack on the UEs
    internet.Install(ueNodes);
    Ipv4InterfaceContainer ueIpIfaces;
    ueIpIfaces = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueLteDevs)); // ueNodes' network interface

    // internet.Install (ueNodes_fix);
    // Ipv4InterfaceContainer ueIpIfaces_fix;
    // ueIpIfaces_fix = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs_fix)); // ueNodes_fix's network interface

    // Assign IP address to UEs, and install applications
    // unicast, multicast application. 
    for (uint32_t u = 0; u < ueNodes.GetN(); ++u) // ueNodes
    {
        Ptr<Node> ueNode = ueNodes.Get(u);
        Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting(ueNode->GetObject<Ipv4> ());
        ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1); //linth
    }
    // for (uint32_t u = 0; u < ueNodes_fix.GetN(); ++u) // ueNodes_fix
    //   {
    //     Ptr<Node> ueNode_fix = ueNodes_fix.Get (u);
    //     Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode_fix->GetObject<Ipv4> ());
    //     ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 0);
    //   }

    // Attach all UEs to the first eNodeB
    for (uint16_t i = 0; i < numberOfUes; i++) // ueNodes
    {
        if (i == 0) {
            lteHelper->Attach(ueLteDevs.Get(i), enbLteDevs.Get(0));
        } else {
//            lteHelper->Attach(ueLteDevs.Get(i), enbLteDevs.Get(3)); // 5
                        lteHelper->Attach(ueLteDevs.Get(i));
        }
        // lteHelper->Attach (ueLteDevs.Get (i), enbLteDevs.Get (0)); // attach one or more UEs to a single eNodeB.
        // lteHelper->Attach (ueLteDevs.Get (i)); // attach one or more UEs to a strongest cell.
    }
    // for (uint16_t i = 0; i < numberOfUes_fix; i++) // ueNodes_fix
    //   {
    //     lteHelper->Attach (ueLteDevs_fix.Get (i), enbLteDevs.Get (5)); // not sure. 
    //   }

    Ptr<LteEnbNetDevice> lteEnbDev = enbLteDevs.Get(0)->GetObject<LteEnbNetDevice> ();
//    std::cout << "Enb " << 0 << "-> DL Bandwidth: " << (int)lteEnbDev->GetUlBandwidth() << std::endl;

    // event & schedule. 
//    Simulator::Schedule(Seconds(10), &handler, 10, 5);
    
    NS_LOG_LOGIC("setting up applications");
    // Install and start applications on UEs and remote host
    uint16_t dlPort = 10000; // for ueNodes
    uint16_t ulPort = 20000;
    // uint16_t dlPort_fix = 50000; // for ueNodes_fix
    // uint16_t ulPort_fix = 60000;

    // randomize a bit start times to avoid simulation artifacts
    // (e.g., buffer overflows due to packet transmissions happening
    // exactly at the same time)
    Ptr<UniformRandomVariable> startTimeSeconds = CreateObject<UniformRandomVariable> ();
    startTimeSeconds->SetAttribute("Min", DoubleValue(0)); // udp
    startTimeSeconds->SetAttribute("Max", DoubleValue(0.010)); // udp
    //  startTimeSeconds->SetAttribute ("Min", DoubleValue (0.100)); // tcp
    //  startTimeSeconds->SetAttribute ("Max", DoubleValue (0.110)); // tcp

    for (uint32_t u = 0; u < numberOfUes; ++u) // ueNodes
    {
        Ptr<Node> ue = ueNodes.Get(u);

        // Set the default gateway for the UE
        Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting(ue->GetObject<Ipv4> ());
        ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);

        for (uint32_t b = 0; b < numBearersPerUe; ++b) {
            ++dlPort;
            ++ulPort;

            ApplicationContainer clientApps;
            ApplicationContainer serverApps;

            NS_LOG_LOGIC("installing UDP DL app for UE " << u);
            UdpClientHelper dlClientHelper(ueIpIfaces.GetAddress(u), dlPort);

            // setup the dlClientHelper.
            dlClientHelper.SetAttribute("Interval", TimeValue(MilliSeconds(1))); // default = 1.
            dlClientHelper.SetAttribute("MaxPackets", UintegerValue(100000));  // default = 100000, 1000000
            dlClientHelper.SetAttribute("PacketSize", UintegerValue(1500));

            clientApps.Add(dlClientHelper.Install(remoteHost));
            PacketSinkHelper dlPacketSinkHelper("ns3::UdpSocketFactory",
                    InetSocketAddress(Ipv4Address::GetAny(), dlPort));
            serverApps.Add(dlPacketSinkHelper.Install(ue));

            // 	NS_LOG_LOGIC ("Part 2: installing UDP DL app for UE " << u);
            //             UdpClientHelper dlClientHelper (ueIpIfaces.GetAddress (u), dlPort);;
            //             dlClientHelper.SetAttribute ("Interval", TimeValue(MilliSeconds(0.8)));
            //             dlClientHelper.SetAttribute ("MaxPackets", UintegerValue(100000));
            //             dlClientHelper.SetAttribute ("PacketSize", UintegerValue(1500));

            // clientApps.Add (dlClientHelper.Install (remoteHost));
            // PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory",
            //                                        InetSocketAddress (Ipv4Address::GetAny (), dlPort));
            // serverApps.Add (dlPacketSinkHelper.Install (ue));

            /*
            else if (u != 0) {
              
               NS_LOG_LOGIC ("installing TCP DL app for UE" << u);
            BulkSendHelper dlClientHelper ("ns3::TcpSocketFactory",
                                                                                  InetSocketAddress (ueIpIfaces.GetAddress (u), dlPort));
            dlClientHelper.SetAttribute ("MaxBytes", UintegerValue (100000));
            clientApps.Add (dlClientHelper.Install(remoteHost));
            PacketSinkHelper dlPacketSinkHelper ("ns3::TcpSocketFactory",
                                                                                            InetSocketAddress (Ipv4Address::GetAny(), dlPort));
            serverApps.Add (dlPacketSinkHelper.Install (ue));
            }     
             */

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
            tft->Add(dlpf);
            // uplinkp2
            // EpcTft::PacketFilter ulpf;
            // ulpf.remotePortStart = ulPort;
            // ulpf.remotePortEnd = ulPort;
            // tft->Add (ulpf);

            EpsBearer bearer(EpsBearer::NGBR_VIDEO_TCP_DEFAULT);
            lteHelper->ActivateDedicatedEpsBearer(ueLteDevs.Get(u), bearer, tft);

            Time startTime = Seconds(startTimeSeconds->GetValue());
            serverApps.Start(startTime);
            clientApps.Start(startTime);
        } // end for b
    }

    // Add X2 inteface
    lteHelper->AddX2Interface(enbNodes);

    // X2-based Handover
    //  lteHelper->HandoverRequest (Seconds (0.100), ueLteDevs.Get (0), enbLteDevs.Get (0), enbLteDevs.Get (1));

    // Uncomment to enable PCAP tracing
//     p2ph.EnablePcapAll("lena-x2-handover-measures");     

    lteHelper->EnablePhyTraces();
    lteHelper->EnableMacTraces();
    lteHelper->EnableRlcTraces();
    lteHelper->EnablePdcpTraces();
    Ptr<RadioBearerStatsCalculator> rlcStats = lteHelper->GetRlcStats();
    rlcStats->SetAttribute("EpochDuration", TimeValue(Seconds(1.0)));
    Ptr<RadioBearerStatsCalculator> pdcpStats = lteHelper->GetPdcpStats();
    pdcpStats->SetAttribute("EpochDuration", TimeValue(Seconds(1.0)));

    // connect custom trace sinks for RRC connection establishment and handover notification
    Config::Connect("/NodeList/*/DeviceList/*/LteEnbRrc/ConnectionEstablished",
            MakeCallback(&NotifyConnectionEstablishedEnb));
    Config::Connect("/NodeList/*/DeviceList/*/LteUeRrc/ConnectionEstablished",
            MakeCallback(&NotifyConnectionEstablishedUe));
    Config::Connect("/NodeList/*/DeviceList/*/LteEnbRrc/HandoverStart",
            MakeCallback(&NotifyHandoverStartEnb));
    Config::Connect("/NodeList/*/DeviceList/*/LteUeRrc/HandoverStart",
            MakeCallback(&NotifyHandoverStartUe));
    Config::Connect("/NodeList/*/DeviceList/*/LteEnbRrc/HandoverEndOk",
            MakeCallback(&NotifyHandoverEndOkEnb));
    Config::Connect("/NodeList/*/DeviceList/*/LteUeRrc/HandoverEndOk",
            MakeCallback(&NotifyHandoverEndOkUe));

    Config::Connect("/NodeList/*/$ns3::MobilityModel/CourseChange",
            MakeCallback(&CourseChange));
    Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx",
            MakeCallback(&SinkRxTrace));

    // Config::Connect ("/NodeList/*/DeviceList/*/LteUeRrc/testPrint", MakeCallback (&testPrint));

    // flow-monitor.
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.Install(ueNodes); // flow monitor. ; ueNodes  

    flowmon.Install(remoteHost);
    flowmon.Install(ueNodes.Get(0));
    flowmon.Install(ueNodes.Get(1));
    //  flowmon.Install(ueNodes_fix.Get(0));   

    flowmon.Install(enbNodes.Get(0)); // option 1: 0, 1, 3, 5; option 2: 
    flowmon.Install(enbNodes.Get(1));
    flowmon.Install(enbNodes.Get(2));
    flowmon.Install(enbNodes.Get(3));
    flowmon.Install(enbNodes.Get(4)); 
    flowmon.Install(enbNodes.Get(5));
//    flowmon.Install(enbNodes.Get(6));

    monitor->SetAttribute("JitterBinWidth", ns3::DoubleValue(0.001)); // 0.001
    monitor->SetAttribute("DelayBinWidth", ns3::DoubleValue(0.001)); // 0.001
    monitor->SetAttribute("PacketSizeBinWidth", ns3::DoubleValue(20)); // 20

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();

    // GtkConfigStore config;
    // config.ConfigureAttributes ();

    // calculate packet loss.
    monitor->CheckForLostPackets();

    // recode the experiment results.
    monitor->SerializeToXmlFile("2eNB1UEscenario.flowmon", true, true);

    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

    double throughput, av_throughput = 0.0;
    //    double tempRx = 0;
    //    double tempTx = 0;
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
        // show load balancing. 
        
        // show PLR vs. time.
        if (i->first == 1)
        {
            FILE *plrFile;
            plrFile = fopen("PLR_vs_time.txt", "a");
            if (plrFile == NULL || plrFile != NULL) 
            {
                std::cout << "Time = " << i->first << "; PLR = " << i->second.rxPackets << std::endl;
                fprintf(plrFile, "Time = %d; PLR = %d\n", i->first, (int)i->second.rxPackets);
                fclose(plrFile);
            }
        }
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
        //        std::cout << "totalReceivedPacket = " << totalReceivedPacket << "; totalSendPacket = " << totalSendPacket << std::endl;
//        std::cout << "av_TotalPacketLossRate = " << av_TotalPacketLossRate << std::endl;
        // number of packet loss. (Total)
        numberOfPacketLoss += numberOfPacketLoss;
        // average throughput.
        throughput = ((i->second.rxBytes * 8.0) / ((i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds()))) / 1024;
        if (i->first != 1) {
            total_throughput += throughput;
//            std::cout << "total_throughput = " << total_throughput << std::endl;
            if (i->first == numberOfUes)
            {
                av_throughput = total_throughput / i->first;
//                std::cout << "av_throughput = " << av_throughput << std::endl;
            }
        } else {
            total_throughput = throughput;
//            std::cout << "throughput = " << throughput << std::endl;
        }
        // average delay.
        if (i->second.rxPackets > 0)
        {
            delay_down = i->second.delaySum.GetSeconds() / i->second.rxPackets;
//            std::cout << "Delay Down = " << i->second.delaySum.GetSeconds() / i->second.rxPackets << "\n";
        }   
        if (i->second.rxPackets > 0)
        {
            delay_up = i->second.delaySum.GetSeconds() / i->second.rxPackets;
//            std::cout << "Delay Up = " << i->second.delaySum.GetSeconds() / i->second.rxPackets << "\n";
        }
        // average handover time. 
        avgHandovers = numberOfSuccessHandover / simTime;
        // handover failure ratio.

        // recode the number of handover.
//        FILE *hFile;
//        hFile = fopen("numberofhandover.txt", "a");
//        if (hFile == NULL || hFile != NULL) {
//            fprintf(hFile, "Number of Handover (total) = %d\n", numberOfHandover);
//            fclose(hFile);
//        }

        //        if (t.destinationPort == dlPort && t.destinationAddress == ("7.0.0.2")) // 7.0.0.3
        //        {
        //            NS_LOG_UNCOND("******************************************************");
        //            NS_LOG_UNCOND("Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << " )");
        //            NS_LOG_UNCOND(" Tx Packets: " << i->second.txPackets);
        //            tempTx += i->second.txBytes;
        //            NS_LOG_UNCOND(" Rx Packets: " << i->second.rxPackets);
        //            tempRx += i->second.rxBytes;
        //            throughput = ((i->second.rxBytes * 8.0) / ((i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds()))) / 1024;
        //            // NS_LOG_UNCOND ( "timeLastRxPacket = " << i->second.timeLastRxPacket.GetSeconds());
        //            // NS_LOG_UNCOND ( "timeFirstRxPacket = " << i->second.timeFirstRxPacket.GetSeconds());
        //            NS_LOG_UNCOND(" Throughputs: " << throughput << " kbps"); // Throuhgput.
        //            numberOfPacketLoss = i->second.txPackets - i->second.rxPackets;
        //            packetLossRate = (numberOfPacketLoss / i->second.txPackets)*100;
        //            NS_LOG_UNCOND(" Packet Loss ratio: " << packetLossRate << "(%)"); // Packet Loss Ratio.
        //
        //            // Delay.
        //            if (i->second.rxPackets > 0)
        //                std::cout << "Delay Down = " << i->second.delaySum.GetSeconds() / i->second.rxPackets << "\n";
        //            if (i->second.rxPackets > 0)
        //                std::cout << "Delay Up = " << i->second.delaySum.GetSeconds() / i->second.rxPackets << "\n";
        //            NS_LOG_UNCOND("******************************************************");
        //
        //            FILE *pFile;
        //            pFile = fopen("throughput.txt", "a");
        //            if (pFile == NULL || pFile != NULL) {
        //                fprintf(pFile, "7.0.0.2 -> Speed=%f; TTT=%f; Hyst=%f; Throughputs=%f (kbits/sec); PLR=%f; numberOfPacketLoss=%f \n", speed, TTT, Hyst, throughput, packetLossRate, numberOfPacketLoss);
        //                fclose(pFile);
        //            }
        //        }
        //
        //        if (t.destinationPort == dlPort) // && t.destinationAddress == Ipv4Address ("7.0.0.4"))// && t.destinationAddress == "7.0.0.5")  // ueNodes_fix.
        //        {
        //            NS_LOG_UNCOND("******************************************************");
        //            NS_LOG_UNCOND("Flow_fix " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << " )");
        //            NS_LOG_UNCOND(" Tx Packets: " << i->second.txPackets);
        //            tempTx += i->second.txBytes;
        //            NS_LOG_UNCOND(" Rx Packets: " << i->second.rxPackets);
        //            tempRx += i->second.rxBytes;
        //            throughput = ((i->second.rxBytes * 8.0) / ((i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds()))) / 1024;
        //            // NS_LOG_UNCOND ( "timeLastRxPacket = " << i->second.timeLastRxPacket.GetSeconds());
        //            // NS_LOG_UNCOND ( "timeFirstRxPacket = " << i->second.timeFirstRxPacket.GetSeconds());
        //            NS_LOG_UNCOND(" Throughputs: " << throughput << " kbps"); // Throuhgput.
        //            numberOfPacketLoss = i->second.txPackets - i->second.rxPackets;
        //            packetLossRate = (numberOfPacketLoss / i->second.txPackets)*100;
        //            NS_LOG_UNCOND(" Packet Loss ratio: " << packetLossRate << "(%)"); // Packet Loss Ratio.
        //            NS_LOG_UNCOND("******************************************************");
        //
        //            FILE *pFile;
        //            pFile = fopen("throughput.txt", "a");
        //            if (pFile == NULL || pFile != NULL) {
        //                fprintf(pFile, "7.0.0.3 -> Speed=%f; TTT=%f; Hyst=%f; Throughputs=%f (kbits/sec); PLR=%f; numberOfPacketLoss=%f \n", speed, TTT, Hyst, throughput, packetLossRate, numberOfPacketLoss);
        //                fclose(pFile);
        //            }
        //        }
    }

    // recod all of the performance evaluation.
    FILE *pFile;
    pFile = fopen("throughput.txt", "a");
    if (pFile == NULL || pFile != NULL) {
         fprintf(pFile, "Speed=%f; TTT=%f; Hyst=%f; av_Throughputs=%f (kbits/sec); av_PLR=%f (minPLR = %f, maxPLR = %f); numberOfPacketLoss=%f ; Delay Down=%f; Delay Up=%f; avgHandovers = %f (numberOfSuccessHandover=%d)\n", speed, TTT, Hyst, av_throughput, av_TotalPacketLossRate, minPacketLossRate, maxPacketLossRate, numberOfPacketLoss, delay_down, delay_up, avgHandovers, numberOfSuccessHandover);
//        fprintf(pFile, "Speed=%f; TTT=%f; Hyst=%f; av_Throughputs=%f (throughput = %f  kbits/sec); av_PLR=%f (minPLR = %f, maxPLR = %f); numberOfPacketLoss=%f (numberOfSuccessHandover=%f)\n", speed, TTT, Hyst, av_throughput, throughput, av_TotalPacketLossRate, minPacketLossRate, maxPacketLossRate, numberOfPacketLoss, numberOfSuccessHandover);
        fclose(pFile);
    }

    Simulator::Destroy();
    return 0;
}


