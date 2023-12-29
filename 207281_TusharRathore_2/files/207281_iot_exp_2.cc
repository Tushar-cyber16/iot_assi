/*
 * Copyright (c) 2014 Universita' di Firenze
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
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

// Network topology
//
//       n0    n1
//       |     |
//       =================
//        WSN (802.15.4)
//
// - ICMPv6 echo request flows from n0 to n1 and back with ICMPv6 echo reply
// - DropTail queues
// - Tracing of queues and packet receptions to file "wsn-ping6.tr"
//
// This example is based on the "ping6.cc" example.

#include "ns3/core-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/internet-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/mobility-module.h"
#include "ns3/sixlowpan-module.h"
#include "ns3/netanim-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"

#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("207281_TusharRathore-IOT-Lab-Exp-2");

int
main(int argc, char** argv)
{
    bool verbose = false;
    Address servAddr;

    CommandLine cmd(__FILE__);
    cmd.AddValue("verbose", "turn on log components", verbose);
    cmd.Parse(argc, argv);

    if (verbose)
    {
        LogComponentEnable("Ping6WsnExample", LOG_LEVEL_INFO);
        LogComponentEnable("Ipv6EndPointDemux", LOG_LEVEL_ALL);
        LogComponentEnable("Ipv6L3Protocol", LOG_LEVEL_ALL);
        LogComponentEnable("Ipv6StaticRouting", LOG_LEVEL_ALL);
        LogComponentEnable("Ipv6ListRouting", LOG_LEVEL_ALL);
        LogComponentEnable("Ipv6Interface", LOG_LEVEL_ALL);
        LogComponentEnable("Icmpv6L4Protocol", LOG_LEVEL_ALL);
        LogComponentEnable("Ping", LOG_LEVEL_ALL);
        LogComponentEnable("NdiscCache", LOG_LEVEL_ALL);
        LogComponentEnable("SixLowPanNetDevice", LOG_LEVEL_ALL);
    }

    NS_LOG_INFO("Create nodes.");
    NodeContainer nodes;
    nodes.Create(10); // we required 10 nodes in this experiment

    // Set seed for random numbers
    SeedManager::SetSeed(167);

    // Install mobility
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    Ptr<ListPositionAllocator> nodesPositionAlloc = CreateObject<ListPositionAllocator>();
    
    for(float i = 0.0; i < 100.0; i += 10.0){
        nodesPositionAlloc->Add(Vector(i, 0.0, 0.0));    
    }
    mobility.SetPositionAllocator(nodesPositionAlloc);
    mobility.Install(nodes);

    NS_LOG_INFO("Create channels.");
    LrWpanHelper lrWpanHelper;
    // Add and install the LrWpanNetDevice for each node
    // lrWpanHelper.EnableLogComponents();
    NetDeviceContainer devContainer = lrWpanHelper.Install(nodes);
    lrWpanHelper.CreateAssociatedPan(devContainer, 10);

    std::cout << "Created " << devContainer.GetN() << " devices" << std::endl;
    std::cout << "There are " << nodes.GetN() << " nodes" << std::endl;

    /* Install IPv4/IPv6 stack */
    NS_LOG_INFO("Install Internet stack.");
    InternetStackHelper internetv6;
    internetv6.SetIpv4StackInstall(false);
    internetv6.Install(nodes);

    // Install 6LowPan layer
    NS_LOG_INFO("Install 6LoWPAN.");
    SixLowPanHelper sixlowpan;
    NetDeviceContainer six1 = sixlowpan.Install(devContainer);

    NS_LOG_INFO("Assign addresses.");
    Ipv6AddressHelper ipv6;
    ipv6.SetBase(Ipv6Address("2001:1::"), Ipv6Prefix(64));
    Ipv6InterfaceContainer i = ipv6.Assign(six1);
    servAddr =  Address(i.GetAddress(1,1));

    NS_LOG_INFO("Create Applications.");

    //
    // Create a UdpEchoServer application on node one.
    //
    uint16_t port = 9; // well-known echo port number
    UdpEchoServerHelper server(port);
    ApplicationContainer apps = server.Install(nodes.Get(1));
    apps.Start(Seconds(1.0));
    apps.Stop(Seconds(10.0));

    //
    // Create a UdpEchoClient application to send UDP datagrams from node zero to
    // node one.
    //
    uint32_t packetSize = 1024;
    uint32_t maxPacketCount = 1;
    Time interPacketInterval = Seconds(1.);
    UdpEchoClientHelper client(servAddr, port);
    client.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
    client.SetAttribute("Interval", TimeValue(interPacketInterval));
    client.SetAttribute("PacketSize", UintegerValue(packetSize));
    apps = client.Install(nodes.Get(0));
    apps.Start(Seconds(2.0));
    apps.Stop(Seconds(10.0));
    client.SetFill(apps.Get(0), "Hello");

    AsciiTraceHelper ascii;
    lrWpanHelper.EnableAsciiAll(ascii.CreateFileStream("experiments/E2/udpwsn.tr")); // To store .tr file in experiments/E2/ folder
    lrWpanHelper.EnablePcapAll(std::string("experiments/E2/udpwsn"), true); // Same for PCAP files 

    AnimationInterface anim("experiments/E2/iot_lab_2.xml");

    float j = 10.0;
    for(int i = 0; i < 5; ++i){
        anim.SetConstantPosition(nodes.Get(i), 0.0, j);
        anim.SetConstantPosition(nodes.Get(i+5), 50.0, j);
        j += 10.0;
    }

    NS_LOG_INFO("Run Simulation.");
    Simulator::Run();
    Simulator::Destroy();
    NS_LOG_INFO("Done.");

    return 0;
}
