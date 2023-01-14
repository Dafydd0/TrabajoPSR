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

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Trabajo");

void PacketReceivedWithAddress (const Ptr<const Packet> packet, const Address &srcAddress,
                                const Address &destAddress);
void PacketReceivedWithoutAddress (Ptr<const Packet> packet);

Ptr<Node> PuenteHelper (NodeContainer nodosLan, NetDeviceContainer &d_nodosLan, DataRate tasa);
double escenario (int nodos, DataRate capacidad, int tam_paq, Time stop_time, Time t_sim,
                  double intervalo, std::string t_cola);

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

  escenario (nFuentes, cap_tx, tam_pkt, stop_time, tSim, intervalo, tam_cola);
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
escenario (int nodos, DataRate capacidad, int tam_paq, Time stop_time, Time t_sim, double intervalo,
           std::string t_cola)
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

  NS_LOG_INFO ("Hay [" << nodos << "] Fuentes");
  NodeContainer c_lan1;
  c_lan1.Create (nodos);
  c_lan1.Add (r0r1.Get (0));

  NetDeviceContainer c_lan1_router_fuentes;

  NS_LOG_DEBUG ("Creando puente de la LAN 1...");
  Ptr<Node> bridge_lan1 = PuenteHelper (c_lan1, c_lan1_router_fuentes, capacidad);

  //Cambiamos MTU para no tener que fragmentar
  for (uint32_t i = 0; i < c_lan1_router_fuentes.GetN (); i++)
    {
      c_lan1_router_fuentes.Get (i)->GetObject<CsmaNetDevice> ()->SetMtu (10000);
    }

  ////////////////////////////////////////////////////
  //FIN CREACIÓN DE LA LAN 1
  ////////////////////////////////////////////////////

  ////////////////////////////////////////////////////
  //CREACIÓN DE LA LAN 2
  ////////////////////////////////////////////////////

  NS_LOG_INFO ("Hay [" << nodos << "] Fuentes");
  NodeContainer c_lan2;
  c_lan2.Create (nodos);
  c_lan2.Add (r0r1.Get (1));

  NetDeviceContainer c_lan2_router_fuentes;

  NS_LOG_DEBUG ("Creando puente de la LAN 2...");
  Ptr<Node> bridge_lan2 = PuenteHelper (c_lan2, c_lan2_router_fuentes, capacidad);

  //Cambiamos MTU para no tener que fragmentar
  for (uint32_t i = 0; i < c_lan2_router_fuentes.GetN (); i++)
    {
      c_lan2_router_fuentes.Get (i)->GetObject<CsmaNetDevice> ()->SetMtu (10000);
    }

  ////////////////////////////////////////////////////
  //FIN CREACIÓN DE LA LAN 2
  ////////////////////////////////////////////////////

  ////////////////////////////////////////////////////
  //CREACIÓN DE LA LAN 3
  ////////////////////////////////////////////////////

  NodeContainer c_lan3;
  c_lan3.Create (3);
  c_lan3.Add (r1r2.Get (1)); //4 nodos
  NS_LOG_INFO ("Hay [" << c_lan3.GetN () << "] nodos en la LAN 3");

  NetDeviceContainer c_lan3_router_otros;

  NS_LOG_DEBUG ("Creando puente de la LAN 2...");
  Ptr<Node> bridge_lan3 = PuenteHelper (c_lan3, c_lan3_router_otros, capacidad);

  //Cambiamos MTU para no tener que fragmentar
  for (uint32_t i = 0; i < c_lan3_router_otros.GetN (); i++)
    {
      c_lan3_router_otros.Get (i)->GetObject<CsmaNetDevice> ()->SetMtu (10000);
    }

  NS_LOG_DEBUG ("Creando UdpServer...");
  Ptr<UdpServer> udpServer = CreateObject<UdpServer> ();
  c_lan3.Get (2)->AddApplication (udpServer); //0 - 1 - Servidor UDP -> 2 - 3

  UintegerValue puerto;
  udpServer->GetAttribute ("Port", puerto);

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
  //CREACIÓN DE LAS FUENTES ONOFF
  ////////////////////////////////////////////////////
  OnOffHelper h_onoff (
      "ns3::UdpSocketFactory",
      InetSocketAddress (interfaces_lan3.GetAddress (2), puerto.Get ())); //El 2 es el Servidor UDP
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

  ApplicationContainer c_app_lan1 = h_onoff.Install (
      c_lan1); //Tambien le estoy instalando la fuente al router, tengo que cambiarlo

  for (uint32_t i = 0; i < c_lan1_router_fuentes.GetN (); i++)
    {
      c_app_lan1.Get (i)->SetStopTime (stop_time);
    }

  ApplicationContainer c_app_lan2 = h_onoff.Install (
      c_lan2); //Tambien le estoy instalando la fuente al router, tengo que cambiarlo

  for (uint32_t i = 0; i < c_lan2_router_fuentes.GetN (); i++)
    {
      c_app_lan2.Get (i)->SetStopTime (stop_time);
    }

  //IntegerValue reg_binFuente = tam_paq * 8 / intervalo;
  //NS_LOG_INFO ("Tiempo entre paquetes: " << intervalo << "s");
  //NS_LOG_INFO ("Régimen binario de las fuentes: " << reg_binFuente.Get () << " bps");

  ////////////////////////////////////////////////////
  //FIN CREACIÓN DE LAS FUENTES ONOFF
  ////////////////////////////////////////////////////

  //NS_LOG_INFO ("Hay [" << bridge->GetNDevices () << "] dispositivos");

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

  // NS_LOG_DEBUG ("Creando objeto_retardo...");
  //Retardo objeto_retardo = Retardo (c_dispositivos.Get (0), c_dispositivos.Get (1));

  ///////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////
  //CREACION LAN ADMIN-DDBB
  ///////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////

  // NodeContainer nodos_lan_admin;
  // NodeContainer db_node;
  // nodos_lan_admin.Create (2);
  // db_node.Create (1);
  // NodeContainer allNodesTemp =
  //     NodeContainer (nodos_lan_admin, db_node); // 0 -> admin   1 -> server UDP   2 -> DB
  // Ptr<Node> n_router = CreateObject<Node> ();
  // NodeContainer c_todos_lan_admin (n_router);
  // c_todos_lan_admin.Add (allNodesTemp);

  // NS_LOG_DEBUG ("Creando UdpServer...");
  // Ptr<UdpServer> udpserver_admin = CreateObject<UdpServer> ();
  // c_todos_lan_admin.Get (2)->AddApplication (udpserver_admin);

  // UintegerValue puerto_lan_admin;
  // udpserver_admin->GetAttribute ("Port", puerto_lan_admin);

  // // 0 -> DB
  // // 1 -> admin
  // // 2 -> server UDP
  // // 3 -> router

  // h_pila.Install (c_todos_lan_admin);

  // NetDeviceContainer c_dispositivos_lan_admin;
  // NS_LOG_DEBUG ("Creando puente LAN ADMIN...");
  // Ptr<Node> bridge_lan_admin =
  //     PuenteHelper (c_todos_lan_admin, c_dispositivos_lan_admin, capacidad);

  // //Cambiamos MTU para no tener que fragmentar
  // for (uint32_t i = 0; i < c_dispositivos_lan_admin.GetN (); i++)
  //   {
  //     c_dispositivos_lan_admin.Get (i)->GetObject<CsmaNetDevice> ()->SetMtu (10000);
  //   }

  // Ipv4AddressHelper h_direcciones_admin ("10.20.20.0", "255.255.255.0");
  // Ipv4InterfaceContainer c_interfaces_admin = h_direcciones_admin.Assign (c_dispositivos_lan_admin);
  // NS_LOG_DEBUG ("Asignando direcciones IP...");

  // //Creamos el server TCP (Es la base de datos)
  // uint16_t port = 50000;
  // Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
  // PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
  // ApplicationContainer sinkApp = sinkHelper.Install (db_node);
  // sinkApp.Start (Seconds (1.0));
  // sinkApp.Stop (stop_time);

  // // Create the OnOff applications to send TCP to the server
  // OnOffHelper clientHelperTcp ("ns3::TcpSocketFactory", Address ());
  // clientHelperTcp.SetAttribute ("PacketSize", UintegerValue (tam_paq));
  // clientHelperTcp.SetAttribute ("OnTime",
  //                               StringValue ("ns3::ExponentialRandomVariable[Mean=0.35]"));
  // clientHelperTcp.SetAttribute ("OffTime",
  //                               StringValue ("ns3::ExponentialRandomVariable[Mean=0.85]"));
  // clientHelperTcp.SetAttribute ("DataRate",
  //                               StringValue ("64kbps")); //Se establece el regimen binario a 64kbps

  // ApplicationContainer clientApp; // Nodo admin

  // AddressValue remoteAddress (InetSocketAddress (c_interfaces_admin.GetAddress (3), port));
  // clientHelperTcp.SetAttribute ("Remote", remoteAddress);
  // //clientApp.Add (clientHelperTcp.Install (c_todos_lan_admin.Get (1))); // Instala la fuente TCP en el nodo admin

  // //clientApp.Start (Seconds (1.0));
  // //clientApp.Stop (stop_time);

  //Nos suscribimos a la traza para saber la dirección del emisor cuando recibimos algún paquete
  //udpserver->TraceConnectWithoutContext ("RxWithAddresses", MakeCallback (&PacketReceivedWithAddress)); //LAN fuentes
  udpServer->TraceConnectWithoutContext (
      "Rx", MakeCallback (&PacketReceivedWithoutAddress)); //LAN fuentes
  //udpserver_admin->TraceConnectWithoutContext ("RxWithAddresses", MakeCallback (&PacketReceivedWithAddress)); //LAN admin

  Simulator::Stop (stop_time); //Falta un tiempo, si no la simulación no termina
  Simulator::Run ();
  NS_LOG_INFO ("--[Simulación completada]--");

  //  int paquetesPerdidos = cola_DP->GetTotalDroppedPackets ();
  //NS_LOG_INFO ("Paquetes perdidos en la cola: " << paquetesPerdidos);
  NS_LOG_INFO ("LAN 1: " <<  c_lan1.GetN () << ", LAN 2: " <<  c_lan2.GetN () << ", LAN 3: " << c_lan3.GetN ());

  for (uint32_t i = 0; i < c_lan1.GetN (); i++)
    {
      Ptr<Queue<Packet>> cola_aux = bridge_lan1->GetDevice (i)
                                        ->GetObject<CsmaNetDevice> ()
                                        ->GetQueue ()
                                        ->GetObject<DropTailQueue<Packet>> ();

      NS_LOG_INFO ("LAN 1: Paquetes RECIBIDOS en la cola del puerto ["
                   << i << "]: " << cola_aux->GetTotalReceivedPackets ());
    }

  NS_LOG_INFO ("AAAAAAAAAAAAAAAAAAAAA");

  for (uint32_t i = 0; i < c_lan2.GetN (); i++)
    {
      Ptr<Queue<Packet>> cola_aux = bridge_lan2->GetDevice (i)
                                        ->GetObject<CsmaNetDevice> ()
                                        ->GetQueue ()
                                        ->GetObject<DropTailQueue<Packet>> ();

      NS_LOG_INFO ("LAN 2: Paquetes RECIBIDOS en la cola del puerto ["
                   << i << "]: " << cola_aux->GetTotalReceivedPackets ());
    }
  NS_LOG_INFO ("BBBBBBBBBBBBBBBBBB");

  for (uint32_t i = 0; i < c_lan3.GetN (); i++)
    {
      Ptr<Queue<Packet>> cola_aux = bridge_lan3->GetDevice (i)
                                        ->GetObject<CsmaNetDevice> ()
                                        ->GetQueue ()
                                        ->GetObject<DropTailQueue<Packet>> ();

      NS_LOG_INFO ("LAN 3: Paquetes RECIBIDOS en la cola del puerto ["
                   << i << "]: " << cola_aux->GetTotalReceivedPackets ());
    }
  NS_LOG_INFO ("CCCCCCCCCCCCCCCCCCCCC");

  NS_LOG_INFO (
      "Llamando al método GetReceived() del nodo servidor UDP: " << udpServer->GetReceived ());

  Simulator::Destroy ();

  //return objeto_retardo.GetRetardoMedio ();
  return 0;
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
  NS_LOG_INFO ("Paquete recibido, contenido: " << *packet);

  Ptr<Packet> copy = packet->Copy ();

  // Headers must be removed in the order they're present.
  PppHeader pppHeader;

  EthernetHeader ethernetHeader;
  //copy->RemoveHeader(ethernetHeader);
  GenericMacHeader macHeader;
  Ipv4Header ipHeader;
  // copy->RemoveHeader(ipHeader);
  UdpHeader udpHeader;

  PacketMetadata::ItemIterator metadataIterator = copy->BeginItem ();
  PacketMetadata::Item item;
  NS_LOG_INFO ("Fuera");

  while (metadataIterator.HasNext ())
    {
      NS_LOG_INFO ("Dentro");

      item = metadataIterator.Next ();
      // item.RemoveHeader (ipHeader, 20);
      // NS_LOG_INFO ("HECHO");
      NS_LOG_INFO ("item name: " << item.tid.GetName ());

      if (item.tid.GetName () == "ns3::Ipv4Header")
        {
          NS_LOG_INFO ("ENCONTRADA");
        }
      break;
    }

  NS_LOG_INFO ("Tamaño del paquete: " << copy->GetSize ());

  copy->PeekHeader (ethernetHeader);
  NS_LOG_INFO ("ip header type: " << ipHeader.GetTypeId ());

  // if (llc.GetType () == 0x0806)
  //   {
  //     // found an ARP packet
  //   }

  //NS_LOG_INFO ("IP peekHeader: " << copy->PeekHeader (ipHeader));

  NS_LOG_INFO ("ppp removeHeader: " << copy->RemoveHeader (pppHeader));

  // NS_LOG_INFO ("generic mac removeHeader: " << copy->RemoveHeader (macHeader));

  // NS_LOG_INFO ("Ethernet removeHeader: " << copy->RemoveHeader (ethernetHeader));

  NS_LOG_INFO ("IP removeHeader: " << copy->RemoveHeader (ipHeader));
  // NS_LOG_INFO ("UDP removeHeader: " << copy->RemoveHeader (udpHeader));

  // NS_LOG_INFO ("header size ethernet: " << ethernetHeader.GetHeaderSize ());
  // // NS_LOG_INFO ("IP size ethernet: " << ipHeader.GetHeaderSize());

  NS_LOG_INFO ("Payload size: " << ipHeader.GetPayloadSize ());

  NS_LOG_INFO ("IP origen: " << ipHeader.GetSource ());
  NS_LOG_INFO ("IP destino: " << ipHeader.GetDestination ());
}