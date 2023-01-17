#include "ns3/command-line.h"
#include "ns3/node-container.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/udp-server.h"
#include "ns3/udp-client.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/ptr.h"
#include "ns3/data-rate.h"
#include "ns3/nstime.h"
#include "ns3/uinteger.h"
#include "ns3/integer.h"
#include "ns3/csma-helper.h"
#include "ns3/onoff-application.h"
#include "ns3/on-off-helper.h"
#include "ns3/inet-socket-address.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/application-container.h"
#include "ns3/bridge-helper.h"
#include "ns3/random-variable-stream.h"
#include "ns3/attribute.h"
#include "ns3/pointer.h"
#include "ns3/double.h"
#include "ns3/csma-net-device.h"
#include "ns3/packet.h"
#include "ns3/queue.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/queue-size.h"
#include "ns3/csma-channel.h"
#include "ns3/gnuplot.h"
#include <cmath>
#include "ns3/traffic-control-layer.h"
#include "ns3/queue-disc.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "ns3/object-factory.h"
#include "ns3/network-module.h"
#include "ns3/object-ptr-container.h"
#include <typeinfo>
#include "ns3/packet-sink-helper.h"
#include "ns3/address.h"
#include "ns3/ethernet-header.h"
#include "ns3/udp-header.h"
#include "ns3/ppp-header.h"
#include "ns3/wimax-mac-header.h"
#include "ns3/packet-sink.h"
#include "retardo.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Trabajo");

void PacketReceivedWithAddress (const Ptr<const Packet> packet, const Address &srcAddress,
                                const Address &destAddress);
void PacketReceivedWithoutAddress (Ptr<const Packet> packet);

Ptr<Node> PuenteHelper (NodeContainer nodosLan, NetDeviceContainer &d_nodosLan, DataRate tasa);

double escenario (int nodos_lan1, int nodos_lan2, DataRate capacidad, int tam_paq, Time stop_time,
                  Time t_sim, double intervalo, std::string t_cola);

ApplicationContainer c_app_onoff_all_in_one_node;
//ApplicationContainer c_app_lan2_udp;
/**
 *  Función [main]
 * Es la esencial para el correcto funcionamiento del programa. Será la encargada de ejecutar el escenario que corresponda
 * en cada momento.
*/
int
main (int argc, char *argv[])
{

  Time::SetResolution (Time::NS);
  //Packet::EnablePrinting ();

  CommandLine cmd;
  int nFuentes = 6; // Número de Fuentes
  int tam_pkt = 118; // Tamaño del Paquete
  Time stop_time ("120s"); // Tiempo de parada para las fuentes
  DataRate cap_tx ("100000kb/s"); // Capacidad de transmisión (100Mb/s)
  Time tSim ("120s"); // Tiempo de Simulación
  double intervalo = 0.060; // Intervalo entre paquetes

  std::string tam_cola = "1p"; // Tamaño de  la cola

  //DataRate tasa_codec ("16kbps"); // Tasa de envío de la fuente en el estado on

  cmd.AddValue ("nFuentes", "Número total de fuentes", nFuentes);
  cmd.AddValue ("tam_pkt", "Tamaño del paquete (Bytes)", tam_pkt);
  cmd.AddValue ("stopTime", "Tiempo de parada para las fuentes", stop_time);
  cmd.AddValue ("cap_tx", "Capacidad de transmision de los enlaces", cap_tx);
  cmd.AddValue ("tSim", "Tiempo de simulación", tSim);
  cmd.AddValue ("intervalo", "Intervalo entre paquetes de cada fuente", intervalo);
  cmd.AddValue ("tam_cola", "Tamaño de las colas", tam_cola);
  //cmd.AddValue ("tasa_codec", "Tasa de envío de la fuente en el estado on", tasa_codec);
  cmd.Parse (argc, argv);

  escenario (nFuentes, nFuentes, cap_tx, tam_pkt, stop_time, tSim, intervalo, tam_cola);
  //grafica(nFuentes, cap_tx, tam_pkt, stop_time, tSim, tam_cola);
}

/**
 *  Función [PuenteHelper]
 * Sirve para construir un puente. Toma como parámetros un contenedor de nodos, un contenedor de 
 * dispositivos web y un Datarate para devolver un puntero a Node.
*/
Ptr<Node>
PuenteHelper (NodeContainer nodosLan, NetDeviceContainer &d_nodosLan, DataRate tasa)
{
  AsciiTraceHelper ascii;

  NetDeviceContainer d_puertosBridge;
  CsmaHelper h_csma;
  BridgeHelper h_bridge;
  Ptr<Node> puente = CreateObject<Node> ();
  h_csma.SetChannelAttribute ("DataRate", DataRateValue (tasa));

  // h_csma.EnableAsciiAll (ascii.CreateFileStream ("myfirst.tr"));
  // h_csma.EnablePcapAll ("myfirst.tr");

  for (NodeContainer::Iterator indice = nodosLan.Begin (); indice != nodosLan.End (); indice++)
    {

      NetDeviceContainer enlace = h_csma.Install (NodeContainer (*indice, puente));
      d_nodosLan.Add (enlace.Get (0));
      d_puertosBridge.Add (enlace.Get (1));
    }
  h_bridge.Install (puente, d_puertosBridge);
  return puente;
}

/**
 *  Función [escenario1]
 * Al ejecutar esta función se realizarán las simulaciones correspondientes
 * al apartado 1 de la práctica en función de los parámetros y condiciones que se requieran.
*/
double
escenario (int nodos_lan1, int nodos_lan2, DataRate capacidad, int tam_paq, Time stop_time,
           Time t_sim, double intervalo, std::string t_cola)
{

  // Network topology
  /*

      L2 - R1 ----- R0 -  L1
             \     /
              \   /             
               \ /
                R2
                |
                L3


*/

  // Ptr<Node> n_servidor = CreateObject<Node> ();
  // NodeContainer c_todos (n_servidor);
  // c_todos.Add (c_fuentes);

  ////////////////////////////////////////////////////
  //Implementación del router
  ////////////////////////////////////////////////////
  NodeContainer c_routers;
  c_routers.Create (3);

  NodeContainer r0r1 = NodeContainer (c_routers.Get (0), c_routers.Get (1));
  NodeContainer r0r2 = NodeContainer (c_routers.Get (0), c_routers.Get (2));
  NodeContainer r1r2 = NodeContainer (c_routers.Get (1), c_routers.Get (2));

  PointToPointHelper p2p;
  //Para que no afecte al retardo
  p2p.SetDeviceAttribute ("DataRate", StringValue ("999Gbps"));

  NetDeviceContainer d0d1 = p2p.Install (r0r1);
  NetDeviceContainer d0d2 = p2p.Install (r0r2);
  NetDeviceContainer d1d2 = p2p.Install (r1r2);
  ////////////////////////////////////////////////////
  //Fin implementación router
  ////////////////////////////////////////////////////

  ////////////////////////////////////////////////////
  //CREACIÓN DE LA LAN 1
  ////////////////////////////////////////////////////

  NodeContainer c_lan1_fuentes;
  c_lan1_fuentes.Create (nodos_lan1);

  NodeContainer c_lan1;
  c_lan1.Add (r0r1.Get (0));
  c_lan1.Add (c_lan1_fuentes); //Hacemos esto para que el nodo 0 sea el router

  NS_LOG_INFO ("Hay [" << c_lan1.GetN () << "] nodos en la LAN 1");

  NetDeviceContainer c_lan1_router_fuentes;

  NS_LOG_DEBUG ("Creando puente de la LAN 1...");
  Ptr<Node> bridge_lan1 = PuenteHelper (c_lan1, c_lan1_router_fuentes, capacidad);

  //Cambiamos MTU para no tener que fragmentar
  for (uint32_t i = 0; i < c_lan1_router_fuentes.GetN (); i++)
    {
      c_lan1_router_fuentes.Get (i)->GetObject<CsmaNetDevice> ()->SetMtu (10000);
    }

  NS_LOG_DEBUG ("Creando los UdpServer en las fuentes de la LAN 1...");

  for (uint32_t i = 1; i < c_lan1.GetN (); i++)
    {
      UdpServerHelper udpServerHelper (20000);
      ApplicationContainer c_server = udpServerHelper.Install (c_lan1.Get (i));
    }
  NS_LOG_DEBUG ("UdpServers de la LAN 1 creados...");

  ////////////////////////////////////////////////////
  //FIN CREACIÓN DE LA LAN 1
  ////////////////////////////////////////////////////

  ////////////////////////////////////////////////////
  //CREACIÓN DE LA LAN 2
  ////////////////////////////////////////////////////

  NodeContainer c_lan2_fuentes;
  c_lan2_fuentes.Create (nodos_lan2);

  NodeContainer c_lan2;
  c_lan2.Add (r0r1.Get (1));
  c_lan2.Add (c_lan2_fuentes); //Hacemos esto para que el nodo 0 sea el router

  NS_LOG_INFO ("Hay [" << c_lan2.GetN () << "] nodos en la LAN 2");

  NetDeviceContainer c_lan2_router_fuentes;

  NS_LOG_DEBUG ("Creando puente de la LAN 2...");
  Ptr<Node> bridge_lan2 = PuenteHelper (c_lan2, c_lan2_router_fuentes, capacidad);

  //Cambiamos MTU para no tener que fragmentar
  for (uint32_t i = 0; i < c_lan2_router_fuentes.GetN (); i++)
    {
      c_lan2_router_fuentes.Get (i)->GetObject<CsmaNetDevice> ()->SetMtu (10000);
    }

  NS_LOG_DEBUG ("Creando los UdpServer en las fuentes de la LAN 2...");

  for (uint32_t i = 1; i < c_lan2.GetN (); i++)
    {
      UdpServerHelper udpServerHelper (20000);
      ApplicationContainer c_server = udpServerHelper.Install (c_lan2.Get (i));
    }
  NS_LOG_DEBUG ("Aplicaciones en el nodo 3 de la lan2: " << c_lan2.Get (3)->GetNApplications ());

  NS_LOG_DEBUG ("UdpServers de la LAN 2 creados...");

  ////////////////////////////////////////////////////
  //FIN CREACIÓN DE LA LAN 2
  ////////////////////////////////////////////////////

  ////////////////////////////////////////////////////
  //CREACIÓN DE LA LAN 3
  ////////////////////////////////////////////////////

  NodeContainer c_lan3_temp;
  c_lan3_temp.Create (3);

  NodeContainer c_lan3;
  c_lan3.Add (r1r2.Get (1));
  c_lan3.Add (c_lan3_temp); //Hacemos esto para que el nodo 0 sea el router

  NS_LOG_INFO ("Hay [" << c_lan3.GetN () << "] nodos en la LAN 3");

  NetDeviceContainer c_lan3_router_otros;

  NS_LOG_DEBUG ("Creando puente de la LAN 3...");
  Ptr<Node> bridge_lan3 = PuenteHelper (c_lan3, c_lan3_router_otros, capacidad);

  //Cambiamos MTU para no tener que fragmentar
  for (uint32_t i = 0; i < c_lan3_router_otros.GetN (); i++)
    {
      c_lan3_router_otros.Get (i)->GetObject<CsmaNetDevice> ()->SetMtu (10000);
    }

  // 0 -> router
  // 1 -> admin
  // 2 -> server UDP
  // 3 -> DB

  //Creamos y añadimos el servidor UDP al nodo 2
  NS_LOG_DEBUG ("Creando UdpServer...");
  Ptr<UdpServer> udpServer = CreateObject<UdpServer> ();
  c_lan3.Get (2)->AddApplication (udpServer);

  UintegerValue puerto_udp;
  udpServer->GetAttribute ("Port", puerto_udp);

  //Creamos el server TCP (Es la base de datos)
  uint16_t puerto_tcp = 50000;
  Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), puerto_tcp));
  PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
  ApplicationContainer sinkApp = sinkHelper.Install (c_lan3.Get (3)); //El 3 es la base de datos
  sinkApp.Start (Seconds (1.0));
  sinkApp.Stop (stop_time);

  ////////////////////////////////////////////////////
  //FIN CREACIÓN DE LA LAN 3
  ////////////////////////////////////////////////////

  ////////////////////////////////////////////////////
  //ASIGNACIÓN DE DIRECCIONES IP
  ////////////////////////////////////////////////////

  InternetStackHelper h_pila;
  h_pila.SetIpv6StackInstall (false);
  h_pila.Install (c_lan1);
  h_pila.Install (c_lan2);
  h_pila.Install (c_lan3);

  NS_LOG_DEBUG ("Creando Ipv4AddressHelper e Ipv4InterfaceContainer...");

  Ipv4AddressHelper address;

  address.SetBase ("10.20.30.0", "255.255.255.0"); //IP de la LAN 1
  Ipv4InterfaceContainer interfaces_lan1;
  interfaces_lan1 = address.Assign (c_lan1_router_fuentes);

  address.SetBase ("40.50.60.0", "255.255.255.0"); //IP de la LAN 2
  Ipv4InterfaceContainer interfaces_lan2;
  interfaces_lan2 = address.Assign (c_lan2_router_fuentes);

  address.SetBase ("70.80.90.0", "255.255.255.0"); //IP de la LAN 3
  Ipv4InterfaceContainer interfaces_lan3;
  interfaces_lan3 = address.Assign (c_lan3_router_otros);

  address.SetBase ("100.110.120.0", "255.255.255.0"); //IP link r0-r1
  Ipv4InterfaceContainer interfaces_d0d1;
  interfaces_d0d1 = address.Assign (d0d1);

  address.SetBase ("130.140.150.0", "255.255.255.0"); //IP link r0-r2
  Ipv4InterfaceContainer interfaces_d0d2;
  interfaces_d0d2 = address.Assign (d0d2);

  address.SetBase ("160.170.180.0", "255.255.255.0"); //IP link r1-r2
  Ipv4InterfaceContainer interfaces_d1d2;
  interfaces_d1d2 = address.Assign (d1d2);
  NS_LOG_DEBUG ("Direcciones IP asignadas");

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  ////////////////////////////////////////////////////
  //FIN ASIGNACIÓN DE DIRECCIONES IP
  ////////////////////////////////////////////////////

  ////////////////////////////////////////////////////
  //CREACIÓN DE LAS FUENTES ONOFF UDP
  ////////////////////////////////////////////////////
  OnOffHelper h_onoff ("ns3::UdpSocketFactory",
                       InetSocketAddress (interfaces_lan3.GetAddress (2),
                                          puerto_udp.Get ())); //El 2 es el Servidor UDP
  h_onoff.SetAttribute ("PacketSize", UintegerValue (tam_paq));
  h_onoff.SetAttribute (
      "OnTime",
      StringValue (
          "ns3::ExponentialRandomVariable[Mean=0.35]")); //Se establece el atributo OnTime a 0.35
  h_onoff.SetAttribute (
      "OffTime",
      StringValue (
          "ns3::ExponentialRandomVariable[Mean=0.65]")); //Se establece el atributo Offtime a 0.65
  //No se si aquí hay que poner 8kbps o 2kbps -> PPS = 16,67pps -> 16,67pps*118B = 2Kbps
  h_onoff.SetAttribute ("DataRate",
                        StringValue ("8kbps")); //Se establece el regimen binario a 8kbps
  h_onoff.SetAttribute ("StopTime", TimeValue (stop_time)); //Se establece el tiempo de parada
  NS_LOG_DEBUG ("Atributos del objeto h_onoff modificados");

  DataRate tasa_codec ("8kbps");
  // h_onoff.SetConstantRate(tasa_codec, tam_paq); //Esto creo que tiene que ser así

  ApplicationContainer c_app_lan1 = h_onoff.Install (c_lan1_fuentes);

  for (uint32_t i = 1; i < c_lan1_fuentes.GetN (); i++) // Se empieza en 1, ya que el 0 es el router
    {
      c_app_lan1.Get (i)->SetStopTime (stop_time);
    }

  ApplicationContainer c_app_lan2 = h_onoff.Install (c_lan2_fuentes);

  for (uint32_t i = 1; i < c_lan2_fuentes.GetN (); i++) // Se empieza en 1, ya que el 0 es el router
    {
      c_app_lan2.Get (i)->SetStopTime (stop_time);
    }

  //IntegerValue reg_binFuente = tam_paq * 8 / intervalo;
  //NS_LOG_INFO ("Tiempo entre paquetes: " << intervalo << "s");
  //NS_LOG_INFO ("Régimen binario de las fuentes: " << reg_binFuente.Get () << " bps");

  ////////////////////////////////////////////////////
  //FIN CREACIÓN DE LAS FUENTES ONOFF UDP
  ////////////////////////////////////////////////////

  ////////////////////////////////////////////////////
  //CREACIÓN DE LAS FUENTES ONOFF UDP EN EL MISMO NODO
  ////////////////////////////////////////////////////

  //uint32_t nodos_total = nodos_lan1 + nodos_lan2;

  // ApplicationContainer c_app_lan1_udp;
  //Añadimos al servidor UDP tantas fuentes OnOff como nodos en cada LAN
  for (int i = 1; i < nodos_lan1 + 1;
       i++) //Hay que sumar 1, ya que interfaces_lan1.GetAddress (0) es la IP del router
    {
      OnOffHelper h_onoff ("ns3::UdpSocketFactory",
                           InetSocketAddress (interfaces_lan1.GetAddress (i), 20000));

      h_onoff.SetAttribute ("PacketSize", UintegerValue (tam_paq));
      h_onoff.SetAttribute (
          "OnTime",
          StringValue (
              "ns3::ExponentialRandomVariable[Mean=0.01]")); //Se establece el atributo OnTime a 0.35
      h_onoff.SetAttribute (
          "OffTime",
          StringValue (
              "ns3::ExponentialRandomVariable[Mean=1]")); //Se establece el atributo Offtime a 0.65
      //No se si aquí hay que poner 8kbps o 2kbps -> PPS = 16,67pps -> 16,67pps*118B = 2Kbps
      h_onoff.SetAttribute ("DataRate",
                            StringValue ("8kbps")); //Se establece el regimen binario a 8kbps
      h_onoff.SetAttribute ("StopTime",
                            TimeValue (Simulator::Now ())); //Se establece el tiempo de parada

      ApplicationContainer OnOffAppTemp = h_onoff.Install (c_lan3.Get (2));
      c_app_onoff_all_in_one_node.Add (OnOffAppTemp);
    }

  //ApplicationContainer c_app_lan2_udp;
  //Añadimos al servidor UDP tantas fuentes OnOff como nodos en cada LAN
  for (int i = 1; i < nodos_lan2 + 1; i++)
    {
      OnOffHelper h_onoff ("ns3::UdpSocketFactory",
                           InetSocketAddress (interfaces_lan2.GetAddress (i), 20000));

      h_onoff.SetAttribute ("PacketSize", UintegerValue (tam_paq));
      h_onoff.SetAttribute (
          "OnTime",
          StringValue (
              "ns3::ExponentialRandomVariable[Mean=0.01]")); //Se establece el atributo OnTime a 0.35
      h_onoff.SetAttribute (
          "OffTime",
          StringValue (
              "ns3::ExponentialRandomVariable[Mean=1]")); //Se establece el atributo Offtime a 0.65
      //No se si aquí hay que poner 8kbps o 2kbps -> PPS = 16,67pps -> 16,67pps*118B = 2Kbps
      h_onoff.SetAttribute ("DataRate",
                            StringValue ("8kbps")); //Se establece el regimen binario a 8kbps
      h_onoff.SetAttribute ("StopTime",
                            TimeValue (Simulator::Now ())); //Se establece el tiempo de parada

      ApplicationContainer OnOffAppTemp = h_onoff.Install (c_lan3.Get (2));
      c_app_onoff_all_in_one_node.Add (OnOffAppTemp);
    }

  // for (uint32_t i = 0; i < c_app_onoff_all_in_one_node.GetN (); i++)
  //   {
  //     Ptr<Application> app = c_app_onoff_all_in_one_node.Get (i);
  //     app->SetStopTime (Simulator::Now ());
  //   }

  // for (uint32_t i = 0; i < c_app_onoff_all_in_one_node.GetN (); i++)
  //   {
  //     // Ptr<OnOffApplication> onoff_app = c_app_onoff_all_in_one_node.Get (i)->GetObject<OnOffApplication>();
  //     // onoff_app->SendPacket ();
  //     //c_app_onoff_all_in_one_node.Start(Simulator::Now());
  //     c_app_onoff_all_in_one_node.Stop (Simulator::Now ());
  //   }

  ////////////////////////////////////////////////////
  //FIN CREACIÓN DE LAS FUENTES ONOFF UDP EN EL MISMO NODO
  ////////////////////////////////////////////////////

  ////////////////////////////////////////////////////
  //CREACIÓN DE LA FUENTE ONOFF TCP
  ////////////////////////////////////////////////////

  OnOffHelper clientHelperTcp ("ns3::TcpSocketFactory", Address ());
  clientHelperTcp.SetAttribute ("PacketSize", UintegerValue (tam_paq));
  clientHelperTcp.SetAttribute ("OnTime",
                                StringValue ("ns3::ExponentialRandomVariable[Mean=0.35]"));
  clientHelperTcp.SetAttribute ("OffTime",
                                StringValue ("ns3::ExponentialRandomVariable[Mean=0.85]"));
  clientHelperTcp.SetAttribute ("DataRate",
                                StringValue ("64kbps")); //Se establece el regimen binario a 64kbps

  ApplicationContainer clientApp; // Nodo admin

  AddressValue remoteAddress (InetSocketAddress (interfaces_lan3.GetAddress (3), puerto_tcp));
  clientHelperTcp.SetAttribute ("Remote", remoteAddress);
  clientApp.Add (
      clientHelperTcp.Install (c_lan3.Get (1))); // Instala la fuente TCP en el nodo admin

  //clientApp.Start (Seconds (1.0));
  clientApp.Stop (stop_time);

  ////////////////////////////////////////////////////
  //FIN CREACIÓN DE LA FUENTE ONOFF TCP
  ////////////////////////////////////////////////////

  // Ptr<DropTailQueue<Packet>> cola_DP_lan1 = bridge_lan1->GetDevice (0)
  //                                               ->GetObject<CsmaNetDevice> ()
  //                                               ->GetQueue ()
  //                                               ->GetObject<DropTailQueue<Packet>> ();
  // QueueSizeValue tam_cola;
  // cola_DP_lan1->SetAttribute ("MaxSize", QueueSizeValue (QueueSize (t_cola)));
  // cola_DP_lan1->GetAttribute ("MaxSize", tam_cola);
  // Ptr<DropTailQueue<Packet>> cola_DP_lan2 = bridge_lan2->GetDevice (0)
  //                                               ->GetObject<CsmaNetDevice> ()
  //                                               ->GetQueue ()
  //                                               ->GetObject<DropTailQueue<Packet>> ();
  // QueueSizeValue tam_cola;
  // cola_DP_lan2->SetAttribute ("MaxSize", QueueSizeValue (QueueSize (t_cola)));
  // cola_DP_lan2->GetAttribute ("MaxSize", tam_cola);
  // Ptr<DropTailQueue<Packet>> cola_DP_lan3 = bridge_lan3->GetDevice (0)
  //                                               ->GetObject<CsmaNetDevice> ()
  //                                               ->GetQueue ()
  //                                               ->GetObject<DropTailQueue<Packet>> ();
  // QueueSizeValue tam_cola;
  // cola_DP_lan3->SetAttribute ("MaxSize", QueueSizeValue (QueueSize (t_cola)));
  // cola_DP_lan3->GetAttribute ("MaxSize", tam_cola);

  // NS_LOG_INFO (
  //     "Tamaño de la cola del puerto del switch conectado al servidor: " << tam_cola.Get ());
  // NS_LOG_INFO ("[Arranca la simulación] tiempo de simulación: " << t_sim.GetSeconds () << " s");

  NS_LOG_DEBUG ("Aplicaciones en el nodo 3 de la lan2: " << c_lan2.Get (3)->GetNApplications ());
  NS_LOG_DEBUG ("Aplicaciones en el nodo 2 de la lan1: " << c_lan1.Get (2)->GetNApplications ());
  NS_LOG_DEBUG ("Aplicaciones en el nodo servidor: " << c_lan3.Get (2)->GetNApplications ());
  NS_LOG_DEBUG ("Aplicaciones en el contenedor de aplicaciones del nodo servidor: "
                << c_app_onoff_all_in_one_node.GetN ());

  NS_LOG_DEBUG ("Creando objeto_retardo...");
  Ptr<NetDevice> nodo_transmisor =
      c_lan1.Get (3)->GetDevice (0); //El emisor será el nodo 3 de la LAN 1 por ejemplo
  Ptr<NetDevice> nodo_receptor =
      c_lan3.Get (2)->GetDevice (0); //El receptor será el servidor UDP de la LAN 3

  Retardo objeto_retardo = Retardo (nodo_transmisor, nodo_receptor);

  //Nos suscribimos a la traza para saber la dirección del emisor cuando recibimos algún paquete
  //udpserver->TraceConnectWithoutContext ("RxWithAddresses", MakeCallback (&PacketReceivedWithAddress)); //LAN fuentes
  udpServer->TraceConnectWithoutContext (
      "Rx", MakeCallback (&PacketReceivedWithoutAddress)); //LAN fuentes
  //udpserver_admin->TraceConnectWithoutContext ("RxWithAddresses", MakeCallback (&PacketReceivedWithAddress)); //LAN admin

  Simulator::Stop (stop_time);
  Simulator::Run ();
  NS_LOG_INFO ("--[Simulación completada]--");

  //  int paquetesPerdidos = cola_DP->GetTotalDroppedPackets ();
  //NS_LOG_INFO ("Paquetes perdidos en la cola: " << paquetesPerdidos);
  NS_LOG_INFO ("LAN 1: " << c_lan1.GetN () << ", LAN 2: " << c_lan2.GetN ()
                         << ", LAN 3: " << c_lan3.GetN ());

  for (uint32_t i = 0; i < c_lan1.GetN (); i++)
    {
      Ptr<Queue<Packet>> cola_aux = bridge_lan1->GetDevice (i)
                                        ->GetObject<CsmaNetDevice> ()
                                        ->GetQueue ()
                                        ->GetObject<DropTailQueue<Packet>> ();

      NS_LOG_INFO ("LAN 1: Paquetes RECIBIDOS en la cola del puerto ["
                   << i << "]: " << cola_aux->GetTotalReceivedPackets ());
    }

  for (uint32_t i = 0; i < c_lan2.GetN (); i++)
    {
      Ptr<Queue<Packet>> cola_aux = bridge_lan2->GetDevice (i)
                                        ->GetObject<CsmaNetDevice> ()
                                        ->GetQueue ()
                                        ->GetObject<DropTailQueue<Packet>> ();

      NS_LOG_INFO ("LAN 2: Paquetes RECIBIDOS en la cola del puerto ["
                   << i << "]: " << cola_aux->GetTotalReceivedPackets ());
    }

  for (uint32_t i = 0; i < c_lan3.GetN (); i++)
    {
      Ptr<Queue<Packet>> cola_aux = bridge_lan3->GetDevice (i)
                                        ->GetObject<CsmaNetDevice> ()
                                        ->GetQueue ()
                                        ->GetObject<DropTailQueue<Packet>> ();
      NS_LOG_INFO ("LAN 3: Paquetes RECIBIDOS en la cola del puerto ["
                   << i << "]: " << cola_aux->GetTotalReceivedPackets ());
    }

  NS_LOG_INFO (
      "Llamando al método GetReceived() del nodo servidor UDP: " << udpServer->GetReceived ());

  NS_LOG_INFO ("Retardo medio: " << objeto_retardo.GetRetardoMedio () << " ms");

  Simulator::Destroy ();

  return objeto_retardo.GetRetardoMedio ();
}

void
PacketReceivedWithAddress (const Ptr<const Packet> packet, const Address &srcAddress,
                           const Address &destAddress)
{
  NS_LOG_INFO ("Paquete recibido: " << packet << ", dirección origen: " << srcAddress
                                    << ", dirección destino: " << destAddress);
  Ptr<Packet> copy = packet->Copy ();

  // Headers must be removed in the order they're present.
  // EthernetHeader ethernetHeader;
  //copy->RemoveHeader(ethernetHeader);
  Ipv4Header ipHeader;
  NS_LOG_INFO ("IP peekHeader: " << copy->PeekHeader (ipHeader));

  copy->RemoveHeader (ipHeader);

  NS_LOG_INFO ("IP origen: " << ipHeader.GetSource ());
  NS_LOG_INFO ("IP destino: " << ipHeader.GetDestination ());
}
void
PacketReceivedWithoutAddress (Ptr<const Packet> packet)
{
  //NS_LOG_INFO ("Paquete recibido, contenido: " << *packet);

  for (uint32_t i = 0; i < c_app_onoff_all_in_one_node.GetN (); i++)
    {
      Ptr<OnOffApplication> onoff_app =
          c_app_onoff_all_in_one_node.Get (i)->GetObject<OnOffApplication> ();
      onoff_app->SetStartTime (Simulator::Now ());
      onoff_app->SetStopTime (Time ("1ms"));

      // c_app_onoff_all_in_one_node.Start(Simulator::Now());
      // c_app_onoff_all_in_one_node.Stop(Time("1ms"));
    }
  NS_LOG_INFO ("Paquete de respuesta enviado");

  // Ptr<Packet> copy = packet->Copy ();

  // // Headers must be removed in the order they're present.
  // PppHeader pppHeader;

  // EthernetHeader ethernetHeader;
  // //copy->RemoveHeader(ethernetHeader);
  // GenericMacHeader macHeader;
  // Ipv4Header ipHeader;
  // // copy->RemoveHeader(ipHeader);
  // UdpHeader udpHeader;

  // PacketMetadata::ItemIterator metadataIterator = copy->BeginItem ();
  // PacketMetadata::Item item;
  // NS_LOG_INFO ("Fuera");

  // while (metadataIterator.HasNext ())
  //   {
  //     NS_LOG_INFO ("Dentro");

  //     item = metadataIterator.Next ();
  //     // item.RemoveHeader (ipHeader, 20);
  //     // NS_LOG_INFO ("HECHO");
  //     NS_LOG_INFO ("item name: " << item.tid.GetName ());

  //     if (item.tid.GetName () == "ns3::Ipv4Header")
  //       {
  //         NS_LOG_INFO ("ENCONTRADA");
  //       }
  //     break;
  //   }

  // NS_LOG_INFO ("Tamaño del paquete: " << copy->GetSize ());

  // copy->PeekHeader (ethernetHeader);
  // NS_LOG_INFO ("ip header type: " << ipHeader.GetTypeId ());

  // // if (llc.GetType () == 0x0806)
  // //   {
  // //     // found an ARP packet
  // //   }

  // //NS_LOG_INFO ("IP peekHeader: " << copy->PeekHeader (ipHeader));

  // NS_LOG_INFO ("ppp removeHeader: " << copy->RemoveHeader (pppHeader));

  // // NS_LOG_INFO ("generic mac removeHeader: " << copy->RemoveHeader (macHeader));

  // // NS_LOG_INFO ("Ethernet removeHeader: " << copy->RemoveHeader (ethernetHeader));

  // NS_LOG_INFO ("IP removeHeader: " << copy->RemoveHeader (ipHeader));
  // // NS_LOG_INFO ("UDP removeHeader: " << copy->RemoveHeader (udpHeader));

  // // NS_LOG_INFO ("header size ethernet: " << ethernetHeader.GetHeaderSize ());
  // // // NS_LOG_INFO ("IP size ethernet: " << ipHeader.GetHeaderSize());

  // NS_LOG_INFO ("Payload size: " << ipHeader.GetPayloadSize ());

  // NS_LOG_INFO ("IP origen: " << ipHeader.GetSource ());
  // NS_LOG_INFO ("IP destino: " << ipHeader.GetDestination ());
}