#include "ns3/applications-module.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/core-module.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/gnuplot.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/point-to-point-module.h"

#include <cstdlib>
#include <fstream>
#include <string>
#include <unordered_map>

typedef uint32_t uint;
using namespace std;
using namespace ns3;
#define ERROR 0.00001

// global variables

uint32_t numLeaf = 3;
std::string rateRoutertoRouter = "10Mbps";
std::string latencyRouterToRouter = "50ms";
std::string rateHostToRouter = "100Mbps";
std::string latencyHostToRouter = "20ms";

uint packetSize = 1300; // 1.3*1000 (1.3 Kb)

PointToPointHelper p2p, p2pBottleneck;
Ptr<RateErrorModel> em;
NodeContainer routers, senders,
    receivers; // created to hold nodes for routers, senders, and receivers.
NetDeviceContainer routerDevices, leftRouterDevices, rightRouterDevices, senderDevices,
    receiverDevices;
Ipv4AddressHelper routerIP, senderIP, receiverIP;
Ipv4InterfaceContainer routerIFC, senderIFCs, receiverIFCs, leftRouterIFCs, rightRouterIFCs;

double simulationTime = 100; // this sets the time duration of the simulation.
double start = 0;            // start time when the simulation will start.
uint port = 37000;           // port number for comm between nodes.
uint numPackets = 1000000;   // number of packets to be sent.
std::string transferSpeed =
    "100Mbps"; // This sets the desired transfer speed for data, aiming for 100 megabits per second.
               // It's like the speed limit for the virtual network.

AsciiTraceHelper asciiTraceHelper;
std::string tcpVariant[3] = {"TcpHybla", "TcpWestwood", "TcpYeah"};
std::string nodeInd[3] = {"5", "6", "7"};

Ptr<FlowMonitor> flowmon;
FlowMonitorHelper flowmonHelper;

void
setNodeAndChannelAttributes()
{
    // in these lines we are calculating the size of our taildropdown queue and
    // setting up our network architecture.
    p2p.SetDeviceAttribute("DataRate", StringValue(rateHostToRouter));
    p2p.SetChannelAttribute("Delay", StringValue(latencyHostToRouter));
    p2p.SetQueue("ns3::DropTailQueue<Packet>", "MaxSize", StringValue("190p"));

    // QueueSizeValue (QueueSize ("190p"))); //b*d = 100*10^6 /8 B/s * 20 * 10^-3 s = 250000 B = 193
    // packets Here we are setting up the device and channel attributes along with the queue it will
    // use for congestion control for the nodes

    // methods are used to set data rates and delays for the access and bottleneck
    // links.(setDeviceAtt, channelAttri)
    p2pBottleneck.SetDeviceAttribute("DataRate", StringValue(rateRoutertoRouter));
    p2pBottleneck.SetChannelAttribute("Delay", StringValue(latencyRouterToRouter));
    p2pBottleneck.SetQueue("ns3::DropTailQueue<Packet>",
                           "MaxSize",
                           StringValue("49p")); // QueueSizeValue (QueueSize ("49p")));

    /**
        PointToPointHelper class is a powerful tool for creating point-to-point network connections between nodes.
        It simplifies the process by abstracting away many details and providing a structured way to configure various aspects of the connection.
        Toh agar ek node se dusre node ke beech conection banana hai to iske help se ham nodes ka attributes as well as channel ka attributes dono set kar sakte hain.
  */
}

void
createErrorModel()
{
    em = CreateObjectWithAttributes<RateErrorModel>("ErrorRate", DoubleValue(ERROR));

    //Essentially, this code creates a model that can introduce errors into packets based on a specified rate.
    //The RateErrorModel will use the value stored in the ERROR variable to determine how often it corrupts bits, bytes, or entire packets,
    //depending on how you've configured it.
}

void
createNodeContainers()
{
    std::cout << "-------------------Initialising node containers-----------------" << std::endl;
    // created to hold nodes for routers, senders, and receivers.

    // NodeContainer is a class in ns-3 representing a group of nodes.

    // creating 2nodes for routers, 3 for senders and 3 for receivers (hardcoded)
    routers.Create(2);
    senders.Create(numLeaf);
    receivers.Create(numLeaf);
    routerDevices = p2pBottleneck.Install(routers);
    //So in the above code we are initializing routerDevices by connecting the nodes in the routers using pointTopointHelper intance called
    //p2pBottleneck.This will hold the  pointers to our two routers leftRouter and RightRouter.
}

void
setUpAllContainer()
{
    uint i = 0;
    while (i < numLeaf)
    {
        // for left leaf nodes ( left of the dumbell)
        // install network devices on the link between the first router and the ith sender node
        //  leftContainer is a container holding the installed network devices for this connection.
        NetDeviceContainer cleft = p2p.Install(routers.Get(0), senders.Get(i));
        leftRouterDevices.Add(cleft.Get(0));
        senderDevices.Add(cleft.Get(1));
        cleft.Get(0)->SetAttribute("ReceiveErrorModel", PointerValue(em));
        i++;
    }
    //here as well we are creating container for holding pointers to the devices that are connected using pointToPointAccess.
    //cleft is the container and its first element holds the pointer of our leftRouter and 2nd holds the pointer for our sender i.
    //Also we kept adding senders to  our senderDevices containers.
    i = 0;
    while (i < numLeaf)
    {
        // this code is to install network devices on the right leaf node
        NetDeviceContainer cright = p2p.Install(routers.Get(1), receivers.Get(i));
        rightRouterDevices.Add(cright.Get(0));
        receiverDevices.Add(cright.Get(1));
        cright.Get(0)->SetAttribute("ReceiveErrorModel", PointerValue(em));
        i++;
    }
    //Similarly here as well we are doing it for the recievers and the right Routers as well.
}

void
installStackOnAllNodes()
{
    // The InternetStackHelper is a helper class in ns-3 used to install the internet protocol stack
    // on nodes.
    // Installing Internet Stack
    std::cout << "---------------Installing Internet Stack on the nodes----------------"
              << std::endl;
    InternetStackHelper stack;
    stack.Install(routers);
    stack.Install(senders);
    stack.Install(receivers);
}

void
createIPv4AddressPool()
{
    std::cout << "-----------Setting up IP addressess to the nodes------------------" << std::endl;
    
    routerIP = Ipv4AddressHelper("40.3.0.0", "255.255.255.0"); //(network, mask)
    senderIP = Ipv4AddressHelper("40.1.0.0", "255.255.255.0");
    receiverIP = Ipv4AddressHelper("40.2.0.0", "255.255.255.0");

    //This creates an Ipv4AddressHelper object named routerIP that generates IPv4 addresses within the subnet "10.3.0.0/24".
    //This means router addresses will range from 10.3.0.1 to 10.3.255.254.
    //Similar helpers are created for senderIP and receiverIP with different subnets.
}

void
assignIPToDevices()
{
    // assigning ip to router
    routerIFC = routerIP.Assign(routerDevices);
    //routerDevices is a container that holds the pointers to the router devices and using this we are assigning the routerIp that we have designed to our routers interfaces.
    //Later we will also assign the Ipv4 addresses to their IPv4 interfaces as well.
    //Each network device requires an interface to send and recieve packets over a specific network(subnet).
    //Interfaces have an IP address that identifies the device on that network and also involve settings like subnet mask and gateway.
    //By creating interface containers, you organize and manage interfaces for different groups of devices, similar to how NetDeviceContainer manages devices themselves.

    uint i = 0;
    while (i < numLeaf) // run 3 times
    {
        NetDeviceContainer senderDevice;
        senderDevice.Add(senderDevices.Get(i));
        senderDevice.Add(leftRouterDevices.Get(i));
        Ipv4InterfaceContainer senderIFC = senderIP.Assign(senderDevice);
        senderIFCs.Add(senderIFC.Get(0));
        leftRouterIFCs.Add(senderIFC.Get(1));
        senderIP.NewNetwork();


        NetDeviceContainer receiverDevice;
        receiverDevice.Add(receiverDevices.Get(i));
        receiverDevice.Add(rightRouterDevices.Get(i));
        Ipv4InterfaceContainer receiverIFC = receiverIP.Assign(receiverDevice);
        receiverIFCs.Add(receiverIFC.Get(0));
        rightRouterIFCs.Add(receiverIFC.Get(1));
        receiverIP.NewNetwork();

        // here we are doing it for our recievers in the same way.
        i++;
    }
    std::cout << "--------------------------Initialization of network finished-----------------------"
              << std::endl;
}

class ClientApp : public Application
{
  public:
    ClientApp()
        : m_socket(0),
          m_peer(),
          m_packetSize(0),
          m_nPackets(0),
          m_dataRate(0),
          m_sendEvent(),
          m_running(false),
          m_packetsSent(0)
    {
    }

    ~ClientApp()
    {
        m_socket = 0;
    }

    void Setup(Ptr<Socket> socket,
               Address address,
               uint32_t packetSize,
               uint32_t nPackets,
               DataRate dataRate)
    {
        m_socket = socket;
        m_peer = address;
        m_packetSize = packetSize;
        m_nPackets = nPackets;
        m_dataRate = dataRate;
    }

  private:
    Ptr<Socket> m_socket;
    Address m_peer;
    uint32_t m_packetSize;
    uint32_t m_nPackets;
    DataRate m_dataRate;
    EventId m_sendEvent;
    bool m_running;
    uint32_t m_packetsSent;

    void ScheduleTx()
    {
        if (m_running)
        {
            Time tNext(Seconds(m_packetSize * 8 / static_cast<double>(m_dataRate.GetBitRate())));
            m_sendEvent = Simulator::Schedule(tNext, &ClientApp::SendPacket, this);
        }
    }

    void SendPacket()
    {
        Ptr<Packet> packet = Create<Packet>(m_packetSize);
        m_socket->Send(packet);

        if (++m_packetsSent < m_nPackets)
            ScheduleTx();
    }

    void StartApplication()
    {
        m_running = true;
        m_packetsSent = 0;
        m_socket->Bind();
        m_socket->Connect(m_peer);
        SendPacket();
    }

    void StopApplication()
    {
        m_running = false;

        if (m_sendEvent.IsRunning())
            Simulator::Cancel(m_sendEvent);

        if (m_socket)
            m_socket->Close();
    }
};

std::map<Address, double> mapBytesReceivedAppLayer, mapMaxGoodput;
std::map<std::string, double> mapBytesReceivedIPV4, mapMaxThroughput;
std::unordered_map<uint, uint> c_loss;

static void
CwndChange(Ptr<OutputStreamWrapper> stream, double startTime, uint id, uint oldCwnd, uint newCwnd)
{
    if (newCwnd < oldCwnd)
        c_loss[id]++;
    *stream->GetStream() << Simulator::Now().GetSeconds() - startTime << "\t" << newCwnd
                         << std::endl;
}

void
PacketRcvdIPV4(Ptr<OutputStreamWrapper> stream,
               double startTime,
               std::string context,
               Ptr<const Packet> p,
               Ptr<Ipv4> ipv4,
               uint interface)
{
    double timeNow = Simulator::Now().GetSeconds();

    if (mapBytesReceivedIPV4.find(context) == mapBytesReceivedIPV4.end())
        mapBytesReceivedIPV4[context] = 0;
    if (mapMaxThroughput.find(context) == mapMaxThroughput.end())
        mapMaxThroughput[context] = 0;
    mapBytesReceivedIPV4[context] += p->GetSize();
    double kbps = (((mapBytesReceivedIPV4[context] * 8.0) / 1024) / (timeNow - startTime));
    *stream->GetStream() << timeNow - startTime << "\t" << kbps << std::endl;
    if (mapMaxThroughput[context] < kbps)
        mapMaxThroughput[context] = kbps;
}

void
PacketRcvdAppLayer(Ptr<OutputStreamWrapper> stream,
                   double startTime,
                   std::string context,
                   Ptr<const Packet> p,
                   const Address& addr)
{
    double timeNow = Simulator::Now().GetSeconds();

    if (mapBytesReceivedAppLayer.find(addr) == mapBytesReceivedAppLayer.end())
        mapBytesReceivedAppLayer[addr] = 0;
    mapBytesReceivedAppLayer[addr] += p->GetSize();
    double kbps = (((mapBytesReceivedAppLayer[addr] * 8.0) / 1024) / (timeNow - startTime));
    *stream->GetStream() << timeNow - startTime << "\t" << kbps << std::endl;
    if (mapMaxGoodput[addr] < kbps)
        mapMaxGoodput[addr] = kbps;
}

std::unordered_map<uint, uint> mapDrop;

Ptr<Socket>
singleFlow(Address sinkAddress,
           uint sinkPort,
           std::string tcpVariant,
           Ptr<Node> hostNode,
           Ptr<Node> sinkNode,
           double startTime,
           double stopTime,
           uint packetSize,
           uint numPackets,
           std::string dataRate,
           double appStartTime,
           double appStopTime)
{
    if (tcpVariant.compare("TcpHybla") == 0)
        Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpHybla::GetTypeId()));
    else if (tcpVariant.compare("TcpWestwood") == 0)
        Config::SetDefault("ns3::TcpL4Protocol::SocketType",
                           TypeIdValue(TcpWestwoodPlus::GetTypeId()));
    else if (tcpVariant.compare("TcpYeah") == 0)
        Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpYeah::GetTypeId()));
    else
    {
        std::cout << tcpVariant << std::endl;
        fprintf(stderr, "Invalid TCP version\n");
        exit(EXIT_FAILURE);
    }

    PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory",
                                      InetSocketAddress(Ipv4Address::GetAny(), sinkPort));
    ApplicationContainer sinkApps = packetSinkHelper.Install(sinkNode);
    sinkApps.Start(Seconds(startTime));
    sinkApps.Stop(Seconds(stopTime));

    // Creates a "packet sink" application using PacketSinkHelper and installs it on the sink node
    // using sinkApps = packetSinkHelper.Install. Starts the sink application at startTime and stops
    // it at stopTime using Start and Stop methods.

    Ptr<Socket> ns3TcpSocket = Socket::CreateSocket(hostNode, TcpSocketFactory::GetTypeId());

    Ptr<ClientApp> app = CreateObject<ClientApp>();
    app->Setup(ns3TcpSocket, sinkAddress, packetSize, numPackets, DataRate(dataRate));
    hostNode->AddApplication(app);
    app->SetStartTime(Seconds(appStartTime));
    app->SetStopTime(Seconds(appStopTime));

    return ns3TcpSocket;
    /**
        This function essentially creates a single TCP flow with the specified properties and starts it at a certain time in the simulation. 
        The chosen TCP variant determines the congestion control algorithm used, and the provided parameters control the data transfer characteristics. 
        By calling this function multiple times with different variants and settings, you can simulate and compare their performance under various network conditions.
    */
}

void
simulateSingleFlow()
{
    std::string trace[3] = {"Single_Flow_1to4", "Single_Flow_2to5", "Single_Flow_3to6"};
    for (uint i = 0; i < 3; ++i)
    {
        Ptr<OutputStreamWrapper> streamCWND = asciiTraceHelper.CreateFileStream(trace[i] + ".cwnd");
        Ptr<OutputStreamWrapper> streamTP = asciiTraceHelper.CreateFileStream(trace[i] + ".tp");
        Ptr<OutputStreamWrapper> streamGP = asciiTraceHelper.CreateFileStream(trace[i] + ".gp");

        // So in the above code we are making trace files to track the poackets dropping ,
        // throughput values, changes occurs in the congestion windows.
        Ptr<Socket> ns3TcpSocket1 = singleFlow(InetSocketAddress(receiverIFCs.GetAddress(i), port),
                                               port,
                                               tcpVariant[i],
                                               senders.Get(i),
                                               receivers.Get(i),
                                               start,
                                               start + simulationTime,
                                               packetSize,
                                               numPackets,
                                               transferSpeed,
                                               start,
                                               start + simulationTime);
        // In this above line we are calling a function singleFlow() which establish a TCP
        // connection  with given congestion control protocol between the given two nodes.

        ns3TcpSocket1->TraceConnectWithoutContext(
            "CongestionWindow",
            MakeBoundCallback(&CwndChange, streamCWND, start, i + 1));

        // This line monitors changes in the "Congestion Window" for the flow.
        // The congestion window determines how much data a sender can send at once. It adapts based
        // on network conditions to avoid overwhelming it. Whenever the window size changes,
        // information about the new size and timestamp is recorded in the streamCWND file, along
        // with the flow number.

        std::string sink = "/NodeList/" + nodeInd[i] + "/$ns3::Ipv4L3Protocol/Rx";
        std::string appSink = "/NodeList/" + nodeInd[i] + "/ApplicationList/0/$ns3::PacketSink/Rx";
        // the code lines provided defines paths to monitor received packets at different layers
        // of the network stack for a specific node The first line (sink) defines a path to monitor
        // packets that arrive at the IP layer of the first node in your simulation. The second line
        // (appSink) defines a path to monitor packets that are delivered to the first application
        // on the same node.

        Config::Connect(sink, MakeBoundCallback(&PacketRcvdIPV4, streamTP, start));
        Config::Connect(appSink, MakeBoundCallback(&PacketRcvdAppLayer, streamGP, start));

        // These two lines of code set up connections between specific points in your network
        // simulation and corresponding trace files, allowing you to capture data about received
        // packets.
        start += simulationTime;
    }
}

void
simulateMultipleFlow()
{
    std::string trace[3] = {"Multi_Flow_1to4", "Multi_Flow_2to5", "Multi_Flow_3to6"};

    Ptr<OutputStreamWrapper> streamCWND = asciiTraceHelper.CreateFileStream(trace[0] + ".cwnd");
    Ptr<OutputStreamWrapper> streamTP = asciiTraceHelper.CreateFileStream(trace[0] + ".tp");
    Ptr<OutputStreamWrapper> streamGP = asciiTraceHelper.CreateFileStream(trace[0] + ".gp");

    Ptr<Socket> ns3TcpSocket1 = singleFlow(InetSocketAddress(receiverIFCs.GetAddress(0), port),
                                           port,
                                           tcpVariant[0],
                                           senders.Get(0),
                                           receivers.Get(0),
                                           start,
                                           start + simulationTime,
                                           packetSize,
                                           numPackets,
                                           transferSpeed,
                                           start,
                                           start + simulationTime);
    ns3TcpSocket1->TraceConnectWithoutContext("CongestionWindow",
                                              MakeBoundCallback(&CwndChange, streamCWND, start, 1));

    std::string sink = "/NodeList/" + nodeInd[0] + "/$ns3::Ipv4L3Protocol/Rx";
    std::string appSink = "/NodeList/" + nodeInd[0] + "/ApplicationList/0/$ns3::PacketSink/Rx";
    Config::Connect(sink, MakeBoundCallback(&PacketRcvdIPV4, streamTP, start));
    Config::Connect(appSink, MakeBoundCallback(&PacketRcvdAppLayer, streamGP, start));
    start = 20;

    for (uint i = 1; i < 3; ++i)
    {
        Ptr<OutputStreamWrapper> streamCWND = asciiTraceHelper.CreateFileStream(trace[i] + ".cwnd");
        Ptr<OutputStreamWrapper> streamTP = asciiTraceHelper.CreateFileStream(trace[i] + ".tp");
        Ptr<OutputStreamWrapper> streamGP = asciiTraceHelper.CreateFileStream(trace[i] + ".gp");
        
        Ptr<Socket> ns3TcpSocket1 = singleFlow(InetSocketAddress(receiverIFCs.GetAddress(i), port),
                                               port,
                                               tcpVariant[i],
                                               senders.Get(i),
                                               receivers.Get(i),
                                               start,
                                               start + simulationTime,
                                               packetSize,
                                               numPackets,
                                               transferSpeed,
                                               start,
                                               start + simulationTime);

        ns3TcpSocket1->TraceConnectWithoutContext(
            "CongestionWindow",
            MakeBoundCallback(&CwndChange, streamCWND, start, i + 1));
        std::string sink = "/NodeList/" + nodeInd[i] + "/$ns3::Ipv4L3Protocol/Rx";
        std::string appSink = "/NodeList/" + nodeInd[i] + "/ApplicationList/0/$ns3::PacketSink/Rx";
        Config::Connect(sink, MakeBoundCallback(&PacketRcvdIPV4, streamTP, start));
        Config::Connect(appSink, MakeBoundCallback(&PacketRcvdAppLayer, streamGP, start));
    }
}

void
installFlowMonitorOnAllNodes()
{
    flowmon = flowmonHelper.InstallAll();
    //Installs the FlowMonitor on all nodes in the network, allowing you to later collect various statistics about data flow, such as throughput, latency, and packet loss.
    //FlowMonitor functionality revolves around collecting and recording statistics about traffic flows traversing the network during the simulation.
}

void
setUpNetworkSimulation()
{
    Ptr<ConstantPositionMobilityModel> s1 =
        routers.Get(0)->GetObject<ConstantPositionMobilityModel>();
    Ptr<ConstantPositionMobilityModel> s2 =
        routers.Get(1)->GetObject<ConstantPositionMobilityModel>();
    s1->SetPosition(Vector(33.0, 50.0, 0));
    s2->SetPosition(Vector(66.0, 50.0, 0));
    Ptr<ConstantPositionMobilityModel> s3 =
        senders.Get(0)->GetObject<ConstantPositionMobilityModel>();
    Ptr<ConstantPositionMobilityModel> s4 =
        senders.Get(1)->GetObject<ConstantPositionMobilityModel>();
    Ptr<ConstantPositionMobilityModel> s5 =
        senders.Get(2)->GetObject<ConstantPositionMobilityModel>();
    Ptr<ConstantPositionMobilityModel> s6 =
        receivers.Get(0)->GetObject<ConstantPositionMobilityModel>();
    Ptr<ConstantPositionMobilityModel> s7 =
        receivers.Get(1)->GetObject<ConstantPositionMobilityModel>();
    Ptr<ConstantPositionMobilityModel> s8 =
        receivers.Get(2)->GetObject<ConstantPositionMobilityModel>();
    s3->SetPosition(Vector(0, 25.0, 0));
    s4->SetPosition(Vector(0, 50.0, 0));
    s5->SetPosition(Vector(0, 75.0, 0));
    s6->SetPosition(Vector(100.0, 25.0, 0));
    s7->SetPosition(Vector(100.0, 50.0, 0));
    s8->SetPosition(Vector(100.0, 75.0, 0));
    Simulator::Stop(Seconds(350.0));
    Simulator::Run();
    flowmon->CheckForLostPackets();
    Simulator::Destroy();
    /**
        This code snippet sets up and runs a basic network simulation in NS3
        1. Node Positioning
            It retrieves pointers to ConstantPositionMobilityModel objects for multiple routers, senders, and receivers using GetObject<ConstantPositionMobilityModel>.
            It then sets the positions of these nodes at specific 3D coordinates using the SetPosition method, defining the network topology.
        2. Simulation Duration:
            It sets the simulation duration to 350 seconds using Simulator::Stop.
        3. Simulation Run:
            It starts the simulation with Simulator::Run(), allowing the simulated network to operate for the specified duration.
        4. Packet Loss Check:
            After the simulation, it checks for lost packets using flowmon->CheckForLostPackets(). This doesn't necessarily involve flow classification or TCP variant analysis.
        5. Simulation Cleanup:
            Finally, it destroys the simulation with Simulator::Destroy(), releasing resources and concluding the process.
  */
}

int
main()
{
    setNodeAndChannelAttributes();
    createErrorModel();
    createNodeContainers();
    setUpAllContainer();
    installStackOnAllNodes();
    createIPv4AddressPool();
    assignIPToDevices();

    //-------------------Select singleflow for (1) , multiflow for (2)--------------------------
    int choice ;
    cout<<"Enter 1 for singleFlow, 2 for multiFlow : ";
    cin>>choice;
    if(choice == 1)
    {
        cout<<"\n------------------------------Single Flow starting ----------------------------------"<<endl;
        simulateSingleFlow();
    }
    else
    {
        cout<<"\n ------------------------------Multi Flow starting ----------------------------------"<<endl;
        simulateMultipleFlow();
    }
    installFlowMonitorOnAllNodes();

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    // Sets the mobility model for all nodes to ConstantPositionMobilityModel, meaning they remain fixed in their positions throughout the simulation.
    mobility.Install(routers);
    mobility.Install(senders);
    mobility.Install(receivers);
    AnimationInterface anim("anim.xml");

    setUpNetworkSimulation();

    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(flowmonHelper.GetClassifier());
    //This line retrieves the classifier used by the FlowMonitor object and casts it to a pointer of type Ipv4FlowClassifier.
    //This classifier helps identify the source and destination addresses associated with each flow.
    std::map<FlowId, FlowMonitor::FlowStats> stats = flowmon->GetFlowStats();
    //This line retrieves a map containing the flow statistics collected by the FlowMonitor object.
    //The map uses FlowId (unique identifier for each flow) as keys and FlowMonitor::FlowStats objects as values
    //(containing various statistics like packets sent, received, etc.).
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin();
         i != stats.end();
         ++i)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);
        // std::cout << i->second.bytesDropped.size () << "size\n";
        if (t.sourceAddress == "40.1.0.1")
        {
            std::cout << "\n\n---------------------TCP Hybla----------------------" << endl;
            std::cout << "Flow of TCP Hybla " << i->first << " (" << t.sourceAddress << " -> "
                      << t.destinationAddress << ")\n";
            std::cout << "Sent Packets : " << i->second.txPackets << "\n";
            std::cout << "Received Packets : " << i->second.rxPackets << "\n";
            std::cout << "Congestion Loss : " << c_loss[1] << "\n";
            std::cout << "Maximum Throughput : "
                      << mapMaxThroughput["/NodeList/5/$ns3::Ipv4L3Protocol/Rx"] << std::endl
                      << std::endl;
        }
        else if (t.sourceAddress == "40.1.1.1")
        {
            std::cout << "\n-----------------------TCP WestWood+ ---------------------" << endl << endl;
            std::cout << "Flow of TcpWestwood+ " << i->first << " (" << t.sourceAddress << " -> "
                      << t.destinationAddress << ")\n";
            std::cout << "Sent Packets : " << i->second.txPackets << "\n";
            std::cout << "Received Packets : " << i->second.rxPackets << "\n";
            std::cout << "Congestion Loss : " << c_loss[2] << "\n";
            std::cout << "Maximum Throughput : "
                      << mapMaxThroughput["/NodeList/6/$ns3::Ipv4L3Protocol/Rx"] << std::endl
                      << std::endl;
        }
        else if (t.sourceAddress == "40.1.2.1")
        {
            std::cout << "\n-----------------------TCP YeAH---------------------" << endl << endl;
            std::cout << "Flow of TCP YeAH " << i->first << " (" << t.sourceAddress << " -> "
                      << t.destinationAddress << ")\n";
            std::cout << "Sent Packets : " << i->second.txPackets << "\n";
            std::cout << "Received Packets : " << i->second.rxPackets << "\n";
            std::cout << "Congestion Loss : "
                      << " " << c_loss[3] << "\n";
            std::cout << "Maximum Throughput : "
                      << mapMaxThroughput["/NodeList/7/$ns3::Ipv4L3Protocol/Rx"] << std::endl
                      << std::endl;
        }
    }
    
    //This code snippet iterates through the flow statistics, classifies flows based on source address, and prints details about each flow for specific
    //TCP variants (TcpReno, TcpWestwood, TcpYeah). It extracts information like packets sent/received, congestion loss, and maximum throughput for comparison between variants.
  
    return 0;
}