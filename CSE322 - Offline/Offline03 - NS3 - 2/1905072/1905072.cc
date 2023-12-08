#include "tutorial-app.h"

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv6-routing-table-entry.h"
#include "ns3/ipv6-static-routing-helper.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ssid.h"
#include "ns3/stats-module.h"
#include "ns3/tcp-header.h"
#include "ns3/traffic-control-module.h"
#include "ns3/yans-wifi-helper.h"

#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>
using namespace std;
using namespace ns3;

NS_LOG_COMPONENT_DEFINE("1905072_log");
double bttlnkRate = 50, bttlnkDelay = 100;
uint32_t simulationTime = 10;
int32_t lossRateExp = 6;
double p2pRate = 1024, appRate = 1024, p2pDelay = 1;
uint32_t payload = 1024;
uint16_t sinkPort = 8080;
uint32_t nSenders = 2;
uint32_t nReceivers = 2;
uint32_t n_flows = 2;

string congestionAlgorithm[2] = {"TcpNewReno", "TcpAdaptiveReno"};
std::string dir = "scratch/plots";
#define Animate(x)                                                                                 \
    AnimationInterface anim("scratch/plots/" + string(x) + ".xml");                                \
    anim.EnableIpv4L3ProtocolCounters(Seconds(0), Seconds(10));                                    \
    anim.SetMaxPktsPerTraceFile(5000000)

vector<std::ofstream> cwndStream;
int traceCwnd = 0;

/**
 * Congestion window change callback
 *
 * \param oldCwnd Old congestion window.
 * \param newCwnd New congestion window.
 */
static void
CwndChange(int idx, uint32_t oldCwnd, uint32_t newCwnd)
{
    // NS_LOG_UNCOND(Simulator::Now().GetSeconds() << "\t" << newCwnd << "\t" << idx);
    cwndStream[idx] << Simulator::Now().GetSeconds() << "\t" << newCwnd / 1024 << std::endl;
}

void
ParseCMD(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);
    cmd.AddValue("bttlnkRate", "Data rate of bottleneck in 1 to 300 Mbps", bttlnkRate);
    cmd.AddValue("lossRateExp",
                 "Packet loss rate (Exp) for the error model in 2 to 6",
                 lossRateExp);
    cmd.AddValue("traceCwnd", "Trace Congestion window", traceCwnd);
    cmd.AddValue("simulTime", "Simulation Time in Seconds", simulationTime);
    cmd.AddValue("algo", "Congestion Algorithm to compare with TcpNewReno", congestionAlgorithm[1]);
    cmd.AddValue("appRate", "Sender Application Data Rate", appRate);
    cmd.Parse(argc, argv);
}

void
PrintStats(FlowMonitorHelper& flowHelper, Ptr<FlowMonitor> flowmon)
{
    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = flowmon->GetFlowStats();
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin();
         i != stats.end();
         ++i)
    {
        if (true)
        {
            Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);
            NS_LOG_INFO("Flow " << i->first << " (" << t.sourceAddress << " -> "
                                << t.destinationAddress << ")"
                                << "  Throughput: "
                                << (8 * i->second.rxBytes) / (1e3 * simulationTime) << " Kbps");
        }
    }
}

void
Init()
{
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(payload));
    Config::SetDefault("ns3::TcpSocket::InitialCwnd", UintegerValue(1));
}

void
BuildTopology()
{
    /**
     * The DataRate and Delay of the senders and receivers will be 1Gbps and 1ms respectively. Delay
     * of bottleneck link will be 100ms.
     */
    PointToPointHelper pointToPointRouter;
    pointToPointRouter.SetDeviceAttribute("DataRate", StringValue(to_string(bttlnkRate) + "Mbps"));
    pointToPointRouter.SetChannelAttribute("Delay", StringValue(to_string(bttlnkDelay) + "ms"));
    PointToPointHelper pointToPointLeaf;
    pointToPointLeaf.SetDeviceAttribute("DataRate", StringValue(to_string(p2pRate) + "Mbps"));
    pointToPointLeaf.SetChannelAttribute("Delay", StringValue(to_string(p2pDelay) + "ms"));

    /**
     * To set the router buffer capacity to the bandwidth delay product and to get the buffer employ
     * drop-tail discarding as mentioned in the reference paper.
     */
    int64_t bufferSize =
        round(bttlnkDelay * bttlnkRate * 1024 / (1000 * 8)); // KB, Here 1 KB = 1 Packet
    // ms * Mbps / 1000 = Mb
    // Mb * 1024 = Kb
    // Kb * 8 = KB
    pointToPointRouter.SetQueue("ns3::DropTailQueue",
                                "MaxSize",
                                StringValue(std::to_string(bufferSize) + "p"));

    // Setup Physical and Data link layer
    PointToPointDumbbellHelper d(nSenders,
                                 pointToPointLeaf,
                                 nReceivers,
                                 pointToPointLeaf,
                                 pointToPointRouter);

    /**
     * There will be only two flows in the network, each consisting of a single sender and a
     * receiver. Each flow must use different congestion control algorithms. For example, if the
     * flow 1 uses two nodes with TCP NewReno, then flow 2 will use the other two nodes with TCP
     * WestwoodPlus. You can achieve this by installing different internet stacks in the nodes.
     */
    InternetStackHelper stack;
    for (uint32_t i = 0; i < n_flows; i++)
    {
        Config::SetDefault("ns3::TcpL4Protocol::SocketType",
                           StringValue("ns3::" + congestionAlgorithm[i % 2]));
        stack.Install(d.GetLeft(i % 2));
        stack.Install(d.GetRight(i % 2));

        if (i == 0)
        {
            /**
             * For the bottleneck devices, you can install the corresponding internet stack with TCP
             * NewReno.
             */
            stack.Install(d.GetLeft());
            stack.Install(d.GetRight());
        }
    }

    // Assign IP Addresses
    d.AssignIpv4Addresses(Ipv4AddressHelper("10.1.1.0", "255.255.255.0"),
                          Ipv4AddressHelper("10.2.1.0", "255.255.255.0"),
                          Ipv4AddressHelper("10.3.1.0", "255.255.255.0"));

    d.BoundingBox(1, 1, 100, 100);
    NetDeviceContainer routerDevices = d.m_routerDevices;

    // Add the RateErrorModel to the devices of the bottleneck link.
    double errorRate = (1.0 / std::pow(10, lossRateExp));
    Ptr<RateErrorModel> em = CreateObject<RateErrorModel>();
    em->SetAttribute("ErrorRate", DoubleValue(errorRate));
    routerDevices.Get(0)->SetAttribute("ReceiveErrorModel", PointerValue(em));
    routerDevices.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(em));

    Address sinkLocalAddress(InetSocketAddress(Ipv4Address::GetAny(), sinkPort));
    PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory", sinkLocalAddress);

    ApplicationContainer sinkApps;
    // Server Apps
    for (uint32_t i = 0; i < nReceivers; ++i)
    {
        sinkApps.Add(packetSinkHelper.Install(d.GetRight(i)));
    }

    sinkApps.Start(Seconds(0.));
    sinkApps.Stop(Seconds(simulationTime));

    // Client Apps
    for (uint32_t i = 0; i < nSenders; i++)
    {
        Ptr<Socket> ns3TcpSocket =
            Socket::CreateSocket(d.GetLeft(i), TcpSocketFactory::GetTypeId());

        /**
         * You will need to use the trace source CongestionWindow for plotting.
         */

        if (traceCwnd)
        {
            cwndStream.push_back(
                std::ofstream(dir + "/cwnd_time/data/" + congestionAlgorithm[i] + ".dat",
                              std::ios::out));
            ns3TcpSocket->TraceConnectWithoutContext("CongestionWindow",
                                                     MakeBoundCallback(&CwndChange, i));
        }

        Ptr<TutorialApp> app = CreateObject<TutorialApp>();
        Address sinkAddress(InetSocketAddress(d.GetRightIpv4Address(i), sinkPort));
        app->Setup(ns3TcpSocket,
                   sinkAddress,
                   payload,
                   DataRate(to_string(appRate) + "Mbps")); // What should be the rate?

        d.GetLeft(i)->AddApplication(app);
        app->SetStartTime(Seconds(1.));
        app->SetStopTime(Seconds(simulationTime));
    }

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
}

void
CalculateMetrics(Ptr<FlowMonitor> flowmon)
{
    double jain_index_numerator = 0;
    double jain_index_denominator = 0;

    FlowMonitor::FlowStatsContainer stats = flowmon->GetFlowStats();
    double total_received[2] = {0};
    for (auto itr : stats)
    {
        total_received[itr.first % 2] += itr.second.rxBytes;
        double thr = itr.second.rxBytes * 8.0 / (simulationTime * 1024);
        jain_index_numerator += thr;
        jain_index_denominator += (thr * thr);
    }

    double jain_index = jain_index_denominator ? (jain_index_numerator * jain_index_numerator) /
                                                     (4 * jain_index_denominator)
                                               : 0;
    NS_LOG_UNCOND(bttlnkRate << " " << -lossRateExp << " "
                             << (8 * total_received[1]) / (1024 * simulationTime) << " "
                             << (8 * total_received[0]) / (1024 * simulationTime) << " "
                             << jain_index);
}

int
main(int argc, char* argv[])
{
    ParseCMD(argc, argv);
    Init();
    BuildTopology();

    FlowMonitorHelper flowHelper;
    Ptr<FlowMonitor> flowmon;
    flowmon = flowHelper.InstallAll();

    // Animate("dumbbell-animation");

    Simulator::Stop(Seconds(simulationTime + 1));
    Simulator::Run();
    Simulator::Destroy();

    PrintStats(flowHelper, flowmon);
    CalculateMetrics(flowmon);

    return EXIT_SUCCESS;
}