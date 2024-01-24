#include "ns3/antenna-module.h"
#include "ns3/applications-module.h"
#include "ns3/buildings-module.h"
#include "ns3/config-store-module.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/nr-module.h"
#include "ns3/point-to-point-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("CttcNrSimpleQosSchedProof");

int
main(int argc, char* argv[])
{
    // LogComponentEnable("NrMacSchedulerNs3", LOG_LEVEL_DEBUG);
    LogComponentEnable("NrMacSchedulerOfdmaDPP", LOG_LEVEL_ALL);
    LogComponentEnable("NrMacSchedulerUeInfoDPP", LOG_LEVEL_ALL);
    
    uint16_t gNbNum = 1;
    uint16_t ueNumPergNb = 2;

    Time simTime = MilliSeconds(402);
    Time udpAppStartTime = MilliSeconds(400);

    uint16_t numerology = 0;
    double centralFrequency = 4e9;
    double bandwidth = 10e6;
    double totalTxPower = 43;

    bool enableOfdma = false;
    std::string schedulerType = "DPP";
    bool enableQoSLcScheduler = false;

    uint8_t priorityTrafficScenario = 0; // default is saturation

    uint16_t mcsTable = 2;

    bool enablePdcpDiscarding = false;
    uint32_t discardTimerMs = 0;

    CommandLine cmd;

    cmd.AddValue("priorityTrafficScenario",
                 "The traffic scenario for the case of priority. Can be 0: saturation"
                 "or 1: medium-load",
                 priorityTrafficScenario);
    cmd.AddValue("simTime", "Simulation time", simTime);
    cmd.AddValue("numerology", "The numerology to be used", numerology);
    cmd.AddValue("centralFrequency", "The system frequency to be used", centralFrequency);
    cmd.AddValue("bandwidth", "The system bandwidth to be used", bandwidth);
    cmd.AddValue("totalTxPower",
                 "total tx power that will be proportionally assigned to"
                 " bands, CCs and bandwidth parts depending on each BWP bandwidth ",
                 totalTxPower);
    cmd.AddValue("enableOfdma",
                 "If set to true it enables Ofdma scheduler. Default value is false (Tdma)",
                 enableOfdma),
        cmd.AddValue("schedulerType",
                     "PF: Proportional Fair (default), RR: Round-Robin, Qos",
                     schedulerType),
        cmd.AddValue("enableQoSLcScheduler",
                     "If set to true, it enables the QoS LC scheduler. Default is RR (false)",
                     enableQoSLcScheduler),
        cmd.AddValue("enablePdcpDiscarding",
                     "Whether to enable PDCP TX discarding",
                     enablePdcpDiscarding),
        cmd.AddValue("discardTimerMs",
                     "Discard timer value in milliseconds to use for all the flows",
                     discardTimerMs);

    cmd.Parse(argc, argv);

    Config::SetDefault("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue(999999999));
    Config::SetDefault("ns3::LteRlcUm::EnablePdcpDiscarding", BooleanValue(enablePdcpDiscarding));
    Config::SetDefault("ns3::LteRlcUm::DiscardTimerMs", UintegerValue(discardTimerMs));

    /*
     * Create the scenario. In our examples, we heavily use helpers that setup
     * the gnbs and ue following a pre-defined pattern. Please have a look at the
     * GridScenarioHelper documentation to see how the nodes will be distributed.
     */
    int64_t randomStream = 1;

    GridScenarioHelper gridScenario;
    gridScenario.SetRows(1);
    gridScenario.SetColumns(gNbNum);
    gridScenario.SetHorizontalBsDistance(5.0);
    gridScenario.SetVerticalBsDistance(5.0);
    gridScenario.SetBsHeight(1.5);
    gridScenario.SetUtHeight(1.5);
    // must be set before BS number
    gridScenario.SetSectorization(GridScenarioHelper::SINGLE);
    gridScenario.SetBsNumber(gNbNum);
    gridScenario.SetUtNumber(ueNumPergNb * gNbNum);
    gridScenario.SetScenarioHeight(3); // Create a 3x3 scenario where the UE will
    gridScenario.SetScenarioLength(3); // be distribuited.
    randomStream += gridScenario.AssignStreams(randomStream);
    gridScenario.CreateScenario();

    uint32_t udpPacketSize1;
    uint32_t udpPacketSize2;
    uint32_t udpPacketSize3;
    uint32_t lambda1 = 1000;
    uint32_t lambda2 = 1000;
    uint32_t lambda3 = 1000;

    if (priorityTrafficScenario == 0) // saturation
    {
        udpPacketSize1 = 3000;
        udpPacketSize2 = 3000;
        udpPacketSize3 = 3000;
    }
    else if (priorityTrafficScenario == 1) // medium-load
    {
        udpPacketSize1 = 3000;
        udpPacketSize2 = 1252;
        udpPacketSize3 = 1252;
    }
    else
    {
        NS_ABORT_MSG("The priorityTrafficScenario chosen is not correct. "
                     "Please choose among 0: saturation and 1: medium-load");
    }

    /*
     * Create two different NodeContainer for the different traffic type.
     * In ueLowLat we will put the UEs that will receive low-latency traffic,
     * while in ueVoice we will put the UEs that will receive the voice traffic.
     *
     * Lo cambio para poder poner 3 UEs configurables!
     */
    NodeContainer ue1flowContainer;
    NodeContainer ue2flowContainer;
    NodeContainer ue3flowContainer;

    for (uint32_t j = 0; j < gridScenario.GetUserTerminals().GetN(); ++j)
    {
        Ptr<Node> ue = gridScenario.GetUserTerminals().Get(j);

        if (j == 0)
        {
            ue1flowContainer.Add(ue);
        }
        else
        {
            j % 2 == 0 ? ue3flowContainer.Add(ue) : ue2flowContainer.Add(ue);
        }
    }

    if (priorityTrafficScenario == 1)
    {
        lambda1 = 1000 / ue1flowContainer.GetN();
        lambda2 = 1000 / ue2flowContainer.GetN();
        lambda3 = 1000 / ue2flowContainer.GetN();
    }

    // setup the nr simulation
    Ptr<NrPointToPointEpcHelper> epcHelper = CreateObject<NrPointToPointEpcHelper>();
    Ptr<IdealBeamformingHelper> idealBeamformingHelper = CreateObject<IdealBeamformingHelper>();
    Ptr<NrHelper> nrHelper = CreateObject<NrHelper>();

    // Put the pointers inside nrHelper
    nrHelper->SetBeamformingHelper(idealBeamformingHelper);
    nrHelper->SetEpcHelper(epcHelper);

    nrHelper->SetPathlossAttribute("ShadowingEnabled", BooleanValue(false));
    epcHelper->SetAttribute("S1uLinkDelay", TimeValue(MilliSeconds(0)));
    Config::SetDefault("ns3::ThreeGppChannelModel::UpdatePeriod", TimeValue(MilliSeconds(0)));
    nrHelper->SetChannelConditionModelAttribute("UpdatePeriod", TimeValue(MilliSeconds(0)));

    std::stringstream scheduler;
    std::string subType;

    subType = enableOfdma == false ? "Tdma" : "Ofdma";
    scheduler << "ns3::NrMacScheduler" << subType << schedulerType;
    std::cout << "Scheduler: " << scheduler.str() << std::endl;
    nrHelper->SetSchedulerTypeId(TypeId::LookupByName(scheduler.str()));

    if (enableQoSLcScheduler)
    {
        nrHelper->SetSchedulerAttribute("SchedLcAlgorithmType",
                                        TypeIdValue(NrMacSchedulerLcQos::GetTypeId()));
    }

    // Error Model: gNB and UE with same spectrum error model.
    std::string errorModel = "ns3::NrEesmIrT" + std::to_string(mcsTable);
    nrHelper->SetDlErrorModel(errorModel);
    nrHelper->SetUlErrorModel(errorModel);

    // Both DL and UL AMC will have the same model behind.
    nrHelper->SetGnbDlAmcAttribute("AmcModel", EnumValue(NrAmc::ErrorModel));
    nrHelper->SetGnbUlAmcAttribute("AmcModel", EnumValue(NrAmc::ErrorModel));

    // Beamforming method
    idealBeamformingHelper->SetAttribute("BeamformingMethod",
                                         TypeIdValue(DirectPathBeamforming::GetTypeId()));

    // Antennas for all the UEs
    nrHelper->SetUeAntennaAttribute("NumRows", UintegerValue(1));
    nrHelper->SetUeAntennaAttribute("NumColumns", UintegerValue(1));
    nrHelper->SetUeAntennaAttribute("AntennaElement",
                                    PointerValue(CreateObject<IsotropicAntennaModel>()));

    // Antennas for all the gNbs
    nrHelper->SetGnbAntennaAttribute("NumRows", UintegerValue(1));
    nrHelper->SetGnbAntennaAttribute("NumColumns", UintegerValue(1));
    nrHelper->SetGnbAntennaAttribute("AntennaElement",
                                     PointerValue(CreateObject<IsotropicAntennaModel>()));

    /*
     * Setup the configuration of the spectrum. One operation band is deployed
     * with 1 component carrier (CC), automatically generated by the ccBwpManager
     */
    BandwidthPartInfoPtrVector allBwps;
    CcBwpCreator ccBwpCreator;
    OperationBandInfo band;
    const uint8_t numOfCcs = 1;

    auto bandMask = NrHelper::INIT_PROPAGATION | NrHelper::INIT_CHANNEL;

    // Create the configuration for the CcBwpHelper. SimpleOperationBandConf creates
    // a single BWP per CC
    CcBwpCreator::SimpleOperationBandConf bandConf(centralFrequency,
                                                   bandwidth,
                                                   numOfCcs,
                                                   BandwidthPartInfo::UMi_StreetCanyon_LoS);

    bandConf.m_numBwp = 1;
    // By using the configuration created, it is time to make the operation band
    band = ccBwpCreator.CreateOperationBandContiguousCc(bandConf);

    nrHelper->InitializeOperationBand(&band, bandMask);
    // nrHelper->InitializeOperationBand(&band);
    allBwps = CcBwpCreator::GetAllBwps({band});

    double x = pow(10, totalTxPower / 10);

    Packet::EnableChecking();
    Packet::EnablePrinting();

    uint32_t bwpIdUe1 = 0;
    uint32_t bwpIdUe2 = 0;
    uint32_t bwpIdUe3 = 0;

    // gNb routing between Bearer and bandwidh part
    nrHelper->SetGnbBwpManagerAlgorithmAttribute("GBR_LIVE_UL_76", UintegerValue(bwpIdUe1));
    nrHelper->SetGnbBwpManagerAlgorithmAttribute("GBR_LIVE_UL_76", UintegerValue(bwpIdUe2));
    nrHelper->SetGnbBwpManagerAlgorithmAttribute("GBR_LIVE_UL_76", UintegerValue(bwpIdUe3));

    // Ue routing between Bearer and bandwidth part
    nrHelper->SetUeBwpManagerAlgorithmAttribute("GBR_LIVE_UL_76", UintegerValue(bwpIdUe1));
    nrHelper->SetUeBwpManagerAlgorithmAttribute("GBR_LIVE_UL_76", UintegerValue(bwpIdUe2));
    nrHelper->SetUeBwpManagerAlgorithmAttribute("GBR_LIVE_UL_76", UintegerValue(bwpIdUe3));

    /*
     * We have configured the attributes we needed. Now, install and get the pointers
     * to the NetDevices, which contains all the NR stack:
     */
    NetDeviceContainer enbNetDev =
        nrHelper->InstallGnbDevice(gridScenario.GetBaseStations(), allBwps);
    NetDeviceContainer ue1flowNetDev = nrHelper->InstallUeDevice(ue1flowContainer, allBwps);
    NetDeviceContainer ue2flowNetDev = nrHelper->InstallUeDevice(ue2flowContainer, allBwps);
    NetDeviceContainer ue3flowNetDev = nrHelper->InstallUeDevice(ue3flowContainer, allBwps);

    randomStream += nrHelper->AssignStreams(enbNetDev, randomStream);
    randomStream += nrHelper->AssignStreams(ue1flowNetDev, randomStream);
    randomStream += nrHelper->AssignStreams(ue2flowNetDev, randomStream);

    nrHelper->GetGnbPhy(enbNetDev.Get(0), 0)->SetAttribute("Numerology", UintegerValue(numerology));
    nrHelper->GetGnbPhy(enbNetDev.Get(0), 0)->SetAttribute("TxPower", DoubleValue(10 * log10(x)));

    // When all the configuration is done, explicitly call UpdateConfig ()
    for (auto it = enbNetDev.Begin(); it != enbNetDev.End(); ++it)
    {
        DynamicCast<NrGnbNetDevice>(*it)->UpdateConfig();
    }

    for (auto it = ue1flowNetDev.Begin(); it != ue1flowNetDev.End(); ++it)
    {
        DynamicCast<NrUeNetDevice>(*it)->UpdateConfig();
    }
    for (auto it = ue2flowNetDev.Begin(); it != ue2flowNetDev.End(); ++it)
    {
        DynamicCast<NrUeNetDevice>(*it)->UpdateConfig();
    }
    for (auto it = ue3flowNetDev.Begin(); it != ue3flowNetDev.End(); ++it)
    {
        DynamicCast<NrUeNetDevice>(*it)->UpdateConfig();
    }

    // create the internet and install the IP stack on the UEs
    // get SGW/PGW and create a single RemoteHost
    Ptr<Node> pgw = epcHelper->GetPgwNode();
    NodeContainer remoteHostContainer;
    remoteHostContainer.Create(1);
    Ptr<Node> remoteHost = remoteHostContainer.Get(0);
    InternetStackHelper internet;
    internet.Install(remoteHostContainer);

    // connect a remoteHost to pgw. Setup routing too
    PointToPointHelper p2ph;
    p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Gb/s")));
    p2ph.SetDeviceAttribute("Mtu", UintegerValue(2500));
    p2ph.SetChannelAttribute("Delay", TimeValue(Seconds(0.000)));
    NetDeviceContainer internetDevices = p2ph.Install(pgw, remoteHost);
    Ipv4AddressHelper ipv4h;
    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    ipv4h.SetBase("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign(internetDevices);
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting =
        ipv4RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv4>());
    remoteHostStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), 1);
    internet.Install(gridScenario.GetUserTerminals());

    Ipv4InterfaceContainer ue1FlowIpIface;
    Ipv4InterfaceContainer ue2FlowIpIface;
    Ipv4InterfaceContainer ue3FlowIpIface;
    ue1FlowIpIface = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ue1flowNetDev));
    ue2FlowIpIface = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ue2flowNetDev));
    ue3FlowIpIface = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ue3flowNetDev));

    // Set the default gateway for the UEs
    for (uint32_t j = 0; j < gridScenario.GetUserTerminals().GetN(); ++j)
    {
        Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting(
            gridScenario.GetUserTerminals().Get(j)->GetObject<Ipv4>());
        ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);
    }

    // attach UEs to the closest gNB
    nrHelper->AttachToClosestEnb(ue1flowNetDev, enbNetDev);
    nrHelper->AttachToClosestEnb(ue2flowNetDev, enbNetDev);
    nrHelper->AttachToClosestEnb(ue3flowNetDev, enbNetDev);

    /*
     * Traffic part. Install two kind of traffic: low-latency and voice, each
     * identified by a particular source port.
     */
    uint16_t dlPortUe1flow = 1234;
    uint16_t dlPortUe2flow = 1235;
    uint16_t dlPortUe3flow = 1236;

    ApplicationContainer serverApps;

    // The sink will always listen to the specified ports
    UdpServerHelper dlPacketSinkUe1flow(dlPortUe1flow);
    UdpServerHelper dlPacketSinkUe2flow(dlPortUe2flow);
    UdpServerHelper dlPacketSinkUe3flow(dlPortUe3flow);

    // The server, that is the application which is listening, is installed in the UE
    serverApps.Add(dlPacketSinkUe1flow.Install(ue1flowContainer));
    serverApps.Add(dlPacketSinkUe2flow.Install(ue2flowContainer));
    serverApps.Add(dlPacketSinkUe3flow.Install(ue3flowContainer));

    /*
     * Configure attributes for the different generators, using user-provided
     * parameters for generating a CBR traffic
     *
     * UE with 1 flow configuration and object creation:
     */
    /******************************************************************************/
    UdpClientHelper dlClientUe1flow;
    dlClientUe1flow.SetAttribute("RemotePort", UintegerValue(dlPortUe1flow));
    dlClientUe1flow.SetAttribute("MaxPackets", UintegerValue(0xFFFFFFFF));
    dlClientUe1flow.SetAttribute("PacketSize", UintegerValue(udpPacketSize1));
    dlClientUe1flow.SetAttribute("Interval", TimeValue(Seconds(1.0 / lambda1)));

    GbrQosInformation qosUe1flow;
    qosUe1flow.gbrDl = 15e6; // Downlink GBR

    // The bearer that will carry Ue 2 Flows DC GBR traffic
    EpsBearer ue1flowDcGbrBearer(EpsBearer::GBR_LIVE_UL_76, qosUe1flow);

    // The filter for the 2 Flows DC GBR traffic
    Ptr<EpcTft> ue1FlowTft = Create<EpcTft>();
    EpcTft::PacketFilter dlpfUe1flow;
    dlpfUe1flow.localPortStart = dlPortUe1flow;
    dlpfUe1flow.localPortEnd = dlPortUe1flow;
    ue1FlowTft->Add(dlpfUe1flow);
    /******************************************************************************/

    /******************************************************************************/
    UdpClientHelper dlClientUe2flow;
    dlClientUe2flow.SetAttribute("RemotePort", UintegerValue(dlPortUe2flow));
    dlClientUe2flow.SetAttribute("MaxPackets", UintegerValue(0xFFFFFFFF));
    dlClientUe2flow.SetAttribute("PacketSize", UintegerValue(udpPacketSize2));
    dlClientUe2flow.SetAttribute("Interval", TimeValue(Seconds(1.0 / lambda2)));

    GbrQosInformation qosUe2flow;
    qosUe2flow.gbrDl = 15e6; // Downlink GBR

    // The bearer that will carry Ue 2 Flows DC GBR traffic
    EpsBearer ue2flowDcGbrBearer(EpsBearer::GBR_LIVE_UL_76, qosUe2flow);

    // The filter for the 2 Flows DC GBR traffic
    Ptr<EpcTft> ue2FlowTft = Create<EpcTft>();
    EpcTft::PacketFilter dlpfUe2flow;
    dlpfUe2flow.localPortStart = dlPortUe2flow;
    dlpfUe2flow.localPortEnd = dlPortUe2flow;
    ue2FlowTft->Add(dlpfUe2flow);
    /******************************************************************************/

    /******************************************************************************/
    UdpClientHelper dlClientUe3flow;
    dlClientUe3flow.SetAttribute("RemotePort", UintegerValue(dlPortUe3flow));
    dlClientUe3flow.SetAttribute("MaxPackets", UintegerValue(0xFFFFFFFF));
    dlClientUe3flow.SetAttribute("PacketSize", UintegerValue(udpPacketSize3));
    dlClientUe3flow.SetAttribute("Interval", TimeValue(Seconds(1.0 / lambda3)));

    GbrQosInformation qosUe3flow;
    qosUe3flow.gbrDl = 15e6; // Downlink GBR

    // The bearer that will carry Ue 2 Flows DC GBR traffic
    EpsBearer ue3flowDcGbrBearer(EpsBearer::GBR_LIVE_UL_76, qosUe3flow);

    // The filter for the 2 Flows DC GBR traffic
    Ptr<EpcTft> ue3FlowTft = Create<EpcTft>();
    EpcTft::PacketFilter dlpfUe3flow;
    dlpfUe3flow.localPortStart = dlPortUe3flow;
    dlpfUe3flow.localPortEnd = dlPortUe3flow;
    ue3FlowTft->Add(dlpfUe3flow);
    /******************************************************************************/

    //  Install the applications
    ApplicationContainer clientApps;

    for (uint32_t i = 0; i < ue1flowContainer.GetN(); ++i)
    {
        Ptr<NetDevice> ueDevice = ue1flowNetDev.Get(i);
        Address ueAddress = ue1FlowIpIface.GetAddress(i);

        // The client, who is transmitting, is installed in the remote host,
        // with destination address set to the address of the UE
        dlClientUe1flow.SetAttribute("RemoteAddress", AddressValue(ueAddress));
        clientApps.Add(dlClientUe1flow.Install(remoteHost));

        // Activate a dedicated bearer for the traffic type
        nrHelper->ActivateDedicatedEpsBearer(ueDevice, ue1flowDcGbrBearer, ue1FlowTft);
    }

    for (uint32_t i = 0; i < ue2flowContainer.GetN(); ++i)
    {
        Ptr<NetDevice> ueDevice = ue2flowNetDev.Get(i);
        Address ueAddress = ue2FlowIpIface.GetAddress(i);

        // The client, who is transmitting, is installed in the remote host,
        // with destination address set to the address of the UE
        dlClientUe2flow.SetAttribute("RemoteAddress", AddressValue(ueAddress));
        clientApps.Add(dlClientUe2flow.Install(remoteHost));

        // Activate a dedicated bearer for the traffic type
        nrHelper->ActivateDedicatedEpsBearer(ueDevice, ue2flowDcGbrBearer, ue2FlowTft);
    }

    // for (uint32_t i = 0; i < ue3flowContainer.GetN(); ++i)
    // {
    //     Ptr<NetDevice> ueDevice = ue3flowNetDev.Get(i);
    //     Address ueAddress = ue3FlowIpIface.GetAddress(i);

    //     // The client, who is transmitting, is installed in the remote host,
    //     // with destination address set to the address of the UE
    //     dlClientUe3flow.SetAttribute("RemoteAddress", AddressValue(ueAddress));
    //     clientApps.Add(dlClientUe3flow.Install(remoteHost));

    //     // Activate a dedicated bearer for the traffic type
    //     nrHelper->ActivateDedicatedEpsBearer(ueDevice, ue3flowDcGbrBearer, ue3FlowTft);
    // }

    // start UDP server and client apps
    serverApps.Start(udpAppStartTime);
    clientApps.Start(udpAppStartTime);
    serverApps.Stop(simTime);
    clientApps.Stop(simTime);

    Simulator::Stop(simTime);
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
