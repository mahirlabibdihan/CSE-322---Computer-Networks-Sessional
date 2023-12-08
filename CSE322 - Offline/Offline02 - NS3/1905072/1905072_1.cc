// #include "tutorial-app.h"

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv6-routing-table-entry.h"
#include "ns3/ipv6-static-routing-helper.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ssid.h"
#include "ns3/stats-module.h"
#include "ns3/tcp-header.h"
#include "ns3/traffic-control-module.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/yans-wifi-helper.h"

#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>
using namespace std;
using namespace ns3;

NS_LOG_COMPONENT_DEFINE("wifi-practice");

map<pair<Ipv4Address, uint16_t>, uint32_t> flowMonitor;
map<pair<Ipv4Address, uint16_t>, uint32_t> deliveredFlows, receivedFlows;
NodeContainer senders, receivers, gateway;
NetDeviceContainer senderStaDevices, senderApDevice, receiverStaDevices, receiverApDevice;
Ipv4InterfaceContainer senderStaInterfaces, receiverStaInterfaces;
Ipv4InterfaceContainer senderApInterface, receiverApInterface;
InternetStackHelper stack;
Ipv4AddressHelper address;
MobilityHelper mobility;
YansWifiChannelHelper channel;
map<pair<Ipv4Address, uint16_t>, double> start, stop;
double gstart = -1, gstop = -1;

uint32_t packetSize = 1024; /* Transport layer payload size in bytes. */
uint32_t packetsPerSecond = 100;
uint32_t numNodes = 20; // max = 254*2 -> Ip Address limitation
uint32_t numFlows = numNodes / 2;
double coverageMultiplier = 5.0; // Set the coverage area multiplier
double coverage;
double nodeSpeed = 5.0;
uint32_t numSenders, numReceivers;
double dataRate;
int verbose = 0;
bool tracing = false;
bool firstRun = true;
double delieveryRatio, throughput;
double bottleneckSpeed = 1;
double simulationTime = 10.0;
bool animation = false;
bool isCircular = false;
uint32_t sinkPort = 9;
ApplicationContainer senderApps;
bool initialized = false;
// Different Code
void SetupMobility(NodeContainer wifiStaNodes);
void SetupCoverage();

#define Animate(x)                                                                                 \
    AnimationInterface anim("scratch/wifi-offline/" + string(x) + ".xml");                         \
    for (uint32_t i = 0; i < numReceivers; i++)                                                    \
    {                                                                                              \
        anim.UpdateNodeColor(receivers.Get(i), 0, 0, 255);                                         \
    }                                                                                              \
    anim.UpdateNodeColor(gateway.Get(0), 0, 255, 0);                                               \
    anim.UpdateNodeColor(gateway.Get(1), 0, 255, 0);                                               \
    anim.EnableIpv4L3ProtocolCounters(Seconds(0), Seconds(10));                                    \
    anim.SetMaxPktsPerTraceFile(5000000)

void
CalculateMetrics()
{
    uint32_t c = 0, d = 0;

    delieveryRatio = throughput = 0;
    for (auto m : deliveredFlows)
    {
        Ipv4Address ip = m.first.first;
        uint16_t port = m.first.second;

        uint32_t received = receivedFlows[{ip, port}];
        uint32_t delivered = deliveredFlows[{ip, port}];
        double time_diff = -1;

        if (start.find({ip, port}) != start.end())
        {
            time_diff = stop[{ip, port}] - start[{ip, port}] + 1;
            d++;
            throughput += received;
        }
        // else No packet delivered
        if (verbose == 1)
        {
            cout << "From " << ip << " port " << port << " => Delivered " << delivered
                 << " Bytes, Received " << received << " Bytes in " << time_diff << " Seconds"
                 << " (Start: " << start[{ip, port}] << " - Stop: " << stop[{ip, port}] << ")"
                 << endl;
        }
        if (delivered)
        {
            c++;
            delieveryRatio += 100.0 * received / delivered;
        }
    }

    // throughput = (d ? (throughput * 8) / (1e3 * d) : 0);
    throughput = (throughput * 8) / (1e3 * (gstop - gstart));
    delieveryRatio = (c ? delieveryRatio / c : 0);
}

void
PrintStats()
{
    if (verbose == 1)
    {
        NS_LOG_UNCOND("Delivery Ratio: " << delieveryRatio << "%\nThroughput: " << throughput
                                         << "Kbps");
    }
    NS_LOG_UNCOND(numNodes << "\t" << numFlows << "\t" << packetsPerSecond << "\t" << nodeSpeed
                           << "\t" << coverage << "\t" << delieveryRatio << "\t" << throughput
                           << "\t" << gstop);
}

// Stop simulator after all packets are received
void
CheckPacketSinkStatus()
{
    if (gstop != -1)
    {
        if (Simulator::Now().GetSeconds() - gstop > 2)
        {
            CalculateMetrics();
            // PrintStats();
            if (delieveryRatio == 100)
            {
                Simulator::Stop();
            }
        }
    }
    Simulator::Schedule(Seconds(1.0), &CheckPacketSinkStatus);
}

template <typename T>
vector<string>
Tokenize(T s)
{
    std::stringstream ss;
    ss << s;
    std::string token;
    vector<string> tokens;
    while (std::getline(ss, token, '.'))
    {
        tokens.push_back(token);
    }
    return tokens;
}

uint16_t
GetSourcePort(Ptr<Application> app)
{
    Ptr<Socket> socket = DynamicCast<OnOffApplication>(app)->GetSocket();
    Address addr;
    socket->GetSockName(addr);
    InetSocketAddress localInetAddr = InetSocketAddress::ConvertFrom(addr);
    return localInetAddr.GetPort();
}

Ipv4Address
GetSourceIp(Ptr<Application> app)
{
    Ptr<Socket> socket = DynamicCast<OnOffApplication>(app)->GetSocket();
    Address addr;
    socket->GetSockName(addr);
    InetSocketAddress localInetAddr = InetSocketAddress::ConvertFrom(addr);
    return localInetAddr.GetIpv4();
}

uint16_t
GetDestPort(Ptr<Application> app)
{
    Ptr<Socket> socket = DynamicCast<OnOffApplication>(app)->GetSocket();
    Address addr;
    socket->GetPeerName(addr);
    InetSocketAddress localInetAddr = InetSocketAddress::ConvertFrom(addr);
    return localInetAddr.GetPort();
}

Ipv4Address
GetDestIp(Ptr<Application> app)
{
    Ptr<Socket> socket = DynamicCast<OnOffApplication>(app)->GetSocket();
    Address addr;
    socket->GetPeerName(addr);
    InetSocketAddress localInetAddr = InetSocketAddress::ConvertFrom(addr);
    return localInetAddr.GetIpv4();
}

void
TxCallback(Ptr<Application> app, Ptr<const Packet> packet)
{
    Ipv4Address ip = GetSourceIp(app);
    uint16_t port = GetSourcePort(app);

    if (start.find({ip, port}) == start.end())
    {
        start[{ip, port}] = Simulator::Now().GetSeconds();
    }
    if (gstart == -1)
    {
        gstart = Simulator::Now().GetSeconds();
    }
    deliveredFlows[{GetSourceIp(app), GetSourcePort(app)}] += packet->GetSize();
}

void
RxCallback(Ptr<const Packet> packet, const Address& senderAddr)
{
    InetSocketAddress address = InetSocketAddress::ConvertFrom(senderAddr);
    Ipv4Address ip = address.GetIpv4();
    uint16_t port = address.GetPort();

    receivedFlows[{ip, port}] += packet->GetSize();

    gstop = Simulator::Now().GetSeconds();

    stop[{ip, port}] = Simulator::Now().GetSeconds();
}

void
SetupCircularPosition(NodeContainer wifiStaNodes,
                      NodeContainer wifiApNode,
                      double xOffset,
                      double yOffset)
{
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    uint32_t c = 0;
    for (uint32_t i = 1; i <= numNodes / 2; i++)
    {
        for (uint32_t j = c * 20 + 1; j <= (c + 1) * (20) && j <= numNodes / 2; j++, i++)
        {
            double angle = 2.0 * M_PI * j / min(20U, numNodes / 2 - c * 20);
            double radius = 3.0 * (c + 1);
            double x = radius * cos(angle);
            double y = radius * sin(angle);
            positionAlloc->Add(Vector(xOffset + x, yOffset + y, 0.0));
        }
        c++;
    }

    mobility.SetPositionAllocator(positionAlloc);
    SetupMobility(wifiStaNodes);
}

void
SetupDiskPosition(NodeContainer wifiStaNodes,
                  NodeContainer wifiApNode,
                  double xOffset,
                  double yOffset)
{
    mobility.SetPositionAllocator("ns3::RandomDiscPositionAllocator",
                                  "X",
                                  StringValue(to_string(xOffset)),
                                  "Y",
                                  StringValue(to_string(yOffset)),
                                  "Rho",
                                  ns3::StringValue("ns3::UniformRandomVariable[Min=1.0|Max=" +
                                                   std::to_string(0.9 * coverage) + "]"));
    SetupMobility(wifiStaNodes);
}

void
SetupLinearPosition(NodeContainer wifiStaNodes, NodeContainer wifiApNode, double xSta)
{
    double gap = 40.0 / numNodes;

    // !m_wifiPhy->m_currentEvent
    // Install mobility for the nodes
    // mobility.SetPositionAllocator(
    //     "ns3::GridPositionAllocator",
    //     "MinX",
    //     DoubleValue(xSta),
    //     "MinY",
    //     DoubleValue(gap / 2),
    //     "DeltaY",
    //     DoubleValue(gap), // Change this value to set the spacing between rows
    //     "DeltaX",
    //     DoubleValue(8), // Change this value to set the spacing between rows
    //     "GridWidth",
    //     UintegerValue(2),
    //     "LayoutType",
    //     StringValue("RowFirst"));
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    for (uint32_t i = 0; i < numNodes / 2; i++)
    {
        positionAlloc->Add(Vector(xSta, gap / 2 + i * gap, 0.0));
    }
    mobility.SetPositionAllocator(positionAlloc);
    SetupMobility(wifiStaNodes);
}

YansWifiPhyHelper
SetupPhysicalLayer()
{
    SetupCoverage();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());
    return phy;
}

void
SetupMacLayer(NodeContainer wifiStaNodes,
              NodeContainer wifiApNode,
              NetDeviceContainer& wifiStaDevices,
              NetDeviceContainer& wifiApDevice,
              YansWifiPhyHelper phy,
              string ssid_name)
{
    WifiHelper wifi;
    Ssid ssid = Ssid(ssid_name);

    WifiMacHelper mac;
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing", BooleanValue(false));
    wifiStaDevices = wifi.Install(phy, mac, wifiStaNodes);
    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    wifiApDevice = wifi.Install(phy, mac, wifiApNode);
}

Ipv4InterfaceContainer
SetupNetworkLayer(NetDeviceContainer wifiStaDevices,
                  NetDeviceContainer wifiApDevice,
                  Ipv4Address ipv4)
{
    address.SetBase(ipv4, "255.255.255.0");
    Ipv4InterfaceContainer wifiStaInterfaces = address.Assign(wifiStaDevices);
    address.Assign(wifiApDevice);
    return wifiStaInterfaces;
}

void
SetupSenderApplications()
{
    // When an application is started, the first packet transmission occurs after a delay equal to
    // (packet size/bit rate) = 1/packetsPerSecond*8
    OnOffHelper senderHelper("ns3::TcpSocketFactory", Ipv4Address::GetAny());
    senderHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    senderHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    senderHelper.SetAttribute("PacketSize", UintegerValue(packetSize));
    senderHelper.SetAttribute("DataRate", DataRateValue(DataRate(dataRate)));
    // senderHelper.SetAttribute("MaxBytes", UintegerValue(packetSize * 10));

    for (uint32_t i = 0; i < numFlows; i++)
    {
        uint32_t si = i % numSenders;
        uint32_t ri = (i + i / numReceivers) % (numReceivers);

        AddressValue remoteAddress(
            InetSocketAddress(receiverStaInterfaces.GetAddress(ri), sinkPort));
        senderHelper.SetAttribute("Remote", remoteAddress);
        senderApps.Add(senderHelper.Install(senders.Get(si)));

        senderApps.Get(i)->TraceConnectWithoutContext(
            "Tx",
            MakeBoundCallback(&TxCallback, senderApps.Get(i)));
    }
    senderApps.Start(Seconds(1.0));
    senderApps.Stop(Seconds(simulationTime));
}

void
SetupReceiverApplications()
{
    // packet sink does not send acknowledgments.
    PacketSinkHelper receiverHelper(
        "ns3::TcpSocketFactory",
        Address(InetSocketAddress(Ipv4Address::GetAny(),
                                  sinkPort))); // Capture packet from any ip source

    ApplicationContainer receiverApps;
    for (uint32_t i = 0; i < numReceivers; i++)
    {
        receiverApps.Add(receiverHelper.Install(receivers.Get(i)));
        receiverApps.Get(i)->TraceConnectWithoutContext("Rx", MakeCallback(&RxCallback));
    }
    receiverApps.Start(Seconds(0.0));
}

void
SetupReceivers()
{
    receivers.Create(numReceivers);
    YansWifiPhyHelper phy = SetupPhysicalLayer();
    SetupMacLayer(receivers,
                  gateway.Get(1),
                  receiverStaDevices,
                  receiverApDevice,
                  phy,
                  "receiver-ssid");

    if (isCircular)
    {
        uint32_t layers = ceil((numNodes / 2) / 20.0);
        SetupCircularPosition(receivers, gateway.Get(1), 3 * layers * 4, layers * 4);
    }

    else
    {
        SetupLinearPosition(receivers, gateway.Get(1), 20.0);
        // SetupDiskPosition(receivers, gateway.Get(1), 3.1 * coverage, coverage);
    }

    stack.Install(receivers); // Installs ns3::LoopbackNetDevice
    receiverStaInterfaces = SetupNetworkLayer(receiverStaDevices, receiverApDevice, "10.1.3.0");
    SetupReceiverApplications();

    if (tracing)
    {
        phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
        phy.EnablePcapAll("scratch/wifi-offline/1905072");
    }
}

void
SetupSenders()
{
    senders.Create(numSenders);
    YansWifiPhyHelper phy = SetupPhysicalLayer();
    SetupMacLayer(senders, gateway.Get(0), senderStaDevices, senderApDevice, phy, "sender-ssid");

    if (isCircular)
    {
        uint32_t layers = ceil((numNodes / 2) / 20.0);
        SetupCircularPosition(senders, gateway.Get(0), layers * 4, layers * 4);
    }

    else
    {
        // SetupDiskPosition(senders, gateway.Get(0), coverage, coverage);
        SetupLinearPosition(senders, gateway.Get(0), 0);
    }

    stack.Install(senders); // Installs ns3::LoopbackNetDevice
    senderStaInterfaces = SetupNetworkLayer(senderStaDevices, senderApDevice, "10.1.1.0");

    SetupSenderApplications();

    if (tracing)
    {
        phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
        phy.EnablePcapAll("scratch/wifi-offline/1905072");
    }
}

void
SetupGateway()
{
    gateway.Create(2);
    PointToPointHelper bottleneck;
    bottleneck.SetDeviceAttribute("DataRate", StringValue(to_string(bottleneckSpeed) + "Mbps"));
    bottleneck.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer gatewayDevices = bottleneck.Install(gateway);

    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    // Circular -> Dynamic
    if (isCircular)
    {
        uint32_t layers = ceil((numNodes / 2) / 20.0);
        positionAlloc->Add(Vector(layers * 4, layers * 4, 0.0));
    }
    else // Linear -> Static
    {
        positionAlloc->Add(Vector(4.0, 10.0, 0.0));
        // positionAlloc->Add(Vector(coverage, coverage, 0.0));
    }

    if (isCircular)
    {
        uint32_t layers = ceil((numNodes / 2) / 20.0);
        positionAlloc->Add(Vector(3 * layers * 4, layers * 4, 0.0));
    }
    else // Linear -> Static
    {
        positionAlloc->Add(Vector(16.0, 10.0, 0.0));
        // positionAlloc->Add(Vector(3.1 * coverage, coverage, 0.0));
    }

    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(gateway);

    stack.Install(gateway); // Installs ns3::LoopbackNetDevice
    address.SetBase("10.1.2.0", "255.255.255.0");
    address.Assign(gatewayDevices);

    if (tracing)
    {
        bottleneck.EnablePcapAll("scratch/wifi-offline/1905072");
    }
}

void
Setup()
{
    SetupGateway();
    SetupReceivers();
    SetupSenders();
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
}

void
ParseCMD(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);
    cmd.AddValue("numNodes", "Number of nodes", numNodes);
    cmd.AddValue("numFlows", "Number of flows", numFlows);
    cmd.AddValue("packetsPerSecond", "Number of packets per second", packetsPerSecond);
    cmd.AddValue("nodeSpeed", "Speed of nodes", nodeSpeed);
    cmd.AddValue("coverageMultiplier", "Coverage aread", coverageMultiplier);
    cmd.AddValue("bottleneckSpeed", "Mbps: Controls data rate of bottleneck", bottleneckSpeed);
    cmd.AddValue("verbose", "Turn on all device log components", verbose);
    cmd.AddValue("tracing", "Flag to enable/disable tracing", tracing);
    cmd.AddValue("simulationTime", "Simulation Time", simulationTime);
    cmd.AddValue("isCircular", "Position nodes in a circle", isCircular);
    cmd.Parse(argc, argv);
}

void
Init()
{
    numReceivers = numSenders = numNodes / 2;
    dataRate = packetsPerSecond * packetSize * 8.0;
    coverage = coverageMultiplier * 5.0;
    numFlows = max(numNodes / 2, numFlows);
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(packetSize));
    channel = YansWifiChannelHelper::Default();
}

//      10.1.1.0                      10.1.3.0
//  s0 ..........    10.1.2.0      ............ r0
//  s1 ........ n0 -------------- n1 .......... r1
//  s2 ..........  point-to-point  ............ r2

int
main(int argc, char* argv[])
{
    ParseCMD(argc, argv);

    if (verbose == 2)
    {
        LogComponentEnable("OnOffApplication", LOG_LEVEL_INFO);
        LogComponentEnable("PacketSink", LOG_LEVEL_INFO);
    }

    Init();
    Setup();
    // Animate("dumbbell-animation");
    // Simulator::Schedule(Seconds(1.0), &CheckPacketSinkStatus);
    Simulator::Stop(Seconds(simulationTime + 3));
    Simulator::Run();
    Simulator::Destroy();

    CalculateMetrics();
    PrintStats();
    return 0;
}

/*==================================================================*/
/*====== S T A T I C ===== W I R E L E S S ===== N E T W O R K =====*/
/*==================================================================*/

void
SetupCoverage()
{
    channel.AddPropagationLoss("ns3::RangePropagationLossModel", "MaxRange", DoubleValue(coverage));
    channel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
}

void
SetupMobility(NodeContainer wifiStaNodes)
{
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiStaNodes);
}

/*==================================================================*/
/*====== S T A T I C ===== W I R E L E S S ===== N E T W O R K =====*/
/*==================================================================*/