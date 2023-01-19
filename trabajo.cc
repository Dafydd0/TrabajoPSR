#include "retardo.h"
//#include "etiquetaTiempo.h"
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

#include <iostream>
#include <random>

#define MIN_AUDIO_CLIENTE 80
#define MAX_AUDIO_CLIENTE 160

#define MIN_AUDIO_SERVER 160
#define MAX_AUDIO_SERVER 320

#define MIN_VIDEO_CLIENTE 640
#define MAX_VIDEO_CLIENTE 1280

#define MIN_VIDEO_SERVER 640
#define MAX_VIDEO_SERVER 1280

#define NUM_CURVAS 5
#define NUM_PUNTOS 8
#define ITERACIONES 10
#define T_STUDENT_16_95 1.7459
#define T_STUDENT_8_95 1.8595

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Trabajo");

int random (int low, int high);

void PacketReceivedWithAddress (const Ptr<const Packet> packet, const Address &srcAddress,
                                const Address &destAddress);
void PacketReceivedWithoutAddress (Ptr<const Packet> packet);

Ptr<Node> PuenteHelper (NodeContainer nodosLan, NetDeviceContainer &d_nodosLan, DataRate tasa);

double escenario (int nodos_lan1, int nodos_lan2, DataRate capacidad, Time stop_time, Time t_sim,
                  double intervalo, std::string t_cola, bool trafico);

double grafica (int nodos_lan1, int nodos_lan2, DataRate capacidad, Time stop_time, Time t_sim,
                double intervalo, std::string t_cola, bool trafico);

//Para la generación de numeros aleatorios
std::random_device rd;
std::mt19937 gen (rd ());

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
  // int tam_pkt = 118; // Tamaño del Paquete
  Time stop_time ("120s"); // Tiempo de parada para las fuentes
  DataRate cap_tx ("1000000kb/s"); // Capacidad de transmisión (100Mb/s)
  Time tSim ("120s"); // Tiempo de Simulación
  double intervalo = 0.060; // Intervalo entre paquetes

  std::string tam_cola = "1p"; // Tamaño de  la cola

  bool trafico = false;

  //DataRate tasa_codec ("16kbps"); // Tasa de envío de la fuente en el estado on

  cmd.AddValue ("nFuentes", "Número total de fuentes", nFuentes);
  // cmd.AddValue ("tam_pkt", "Tamaño del paquete (Bytes)", tam_pkt);
  cmd.AddValue ("stopTime", "Tiempo de parada para las fuentes", stop_time);
  cmd.AddValue ("cap_tx", "Capacidad de transmision de los enlaces", cap_tx);
  cmd.AddValue ("tSim", "Tiempo de simulación", tSim);
  cmd.AddValue ("intervalo", "Intervalo entre paquetes de cada fuente", intervalo);
  cmd.AddValue ("tam_cola", "Tamaño de las colas", tam_cola);
  cmd.AddValue ("trafico", "Selecciona tipo de tráfico FALSE -> Audio | TRUE -> Audio + Video",
                trafico);
  //cmd.AddValue ("tasa_codec", "Tasa de envío de la fuente en el estado on", tasa_codec);
  cmd.Parse (argc, argv);

  grafica (nFuentes, nFuentes, cap_tx, stop_time, tSim, intervalo, tam_cola, trafico);
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
escenario (int nodos_lan1, int nodos_lan2, DataRate capacidad, Time stop_time, Time t_sim,
           double intervalo, std::string t_cola, bool trafico)
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

  // GENERACIÓN DE TASA ALEATORIA
  int tam_paq_cliente;
  int tam_paq_servidor;
  DataRate tasa_cliente;
  DataRate tasa_server;
  //uint64_t tasa_total = 0;
  // if (trafico)
  //   { //TRUE -> Audio + video
  //     tam_paq_cliente = random (MIN_VIDEO_CLIENTE, MAX_VIDEO_CLIENTE);
  //     tam_paq_servidor = random (MIN_VIDEO_SERVER, MAX_VIDEO_SERVER);
  //     tasa_cliente = 18600 * tam_paq_cliente; //18600 por medidas reales de wireshark
  //     tasa_server = 5500 * tam_paq_servidor; //5500 por medidas reales de wireshark
  //   }

  // else
  //   { //FALSE -> Audio
  //     tam_paq_cliente = random (MIN_AUDIO_CLIENTE, MAX_AUDIO_CLIENTE);
  //     tam_paq_servidor = random (MIN_AUDIO_SERVER, MAX_AUDIO_SERVER);
  //     tasa_cliente = 4300 * tam_paq_cliente; //4300 por medidas reales de wireshark
  //     tasa_server = 1260 * tam_paq_servidor; //1260 por medidas reales de wireshark
  //   }

  NS_LOG_DEBUG ("Atributos del objeto h_onoff modificados");

  ApplicationContainer c_app_lan1;
  ApplicationContainer c_app_lan2;

  OnOffHelper h_onoff ("ns3::UdpSocketFactory",
                       InetSocketAddress (interfaces_lan3.GetAddress (2),
                                          puerto_udp.Get ())); //El 2 es el Servidor UDP

  h_onoff.SetAttribute ("StartTime",
                        TimeValue (Simulator::Now ())); //Se establece el tiempo de parada
  h_onoff.SetAttribute ("StopTime",
                        TimeValue (stop_time)); //Se establece el tiempo de parada

  for (uint32_t i = 1; i < c_lan1_fuentes.GetN (); i++) // Se empieza en 1, ya que el 0 es el router
    {
      if (trafico)
        { //TRUE -> Audio + video
          tam_paq_cliente = random (MIN_VIDEO_CLIENTE, MAX_VIDEO_CLIENTE);
          tasa_cliente = 18600 * tam_paq_cliente; //18600 por medidas reales de wireshark
          h_onoff.SetAttribute ("OffTime",
                                StringValue ("ns3::ExponentialRandomVariable[Mean=0.004]")); //4ms
        }

      else
        { //FALSE -> Audio
          tam_paq_cliente = random (MIN_AUDIO_CLIENTE, MAX_AUDIO_CLIENTE);
          tasa_cliente = 4300 * tam_paq_cliente; //4300 por medidas reales de wireshark
          h_onoff.SetAttribute ("OffTime",
                                StringValue ("ns3::ExponentialRandomVariable[Mean=0.015]")); //15ms
        }
      h_onoff.SetAttribute ("OnTime",
                            StringValue ("ns3::ExponentialRandomVariable[Mean=0.003]")); //3ms
      h_onoff.SetAttribute ("PacketSize", UintegerValue (tam_paq_cliente));

      h_onoff.SetAttribute ("DataRate",
                            DataRateValue (tasa_cliente)); //Se establece el regimen binario a 8kbps

      ApplicationContainer c_app_temp = h_onoff.Install (c_lan1_fuentes.Get (i));
      c_app_lan1.Add (c_app_temp);
    }

  for (uint32_t i = 1; i < c_lan2_fuentes.GetN (); i++) // Se empieza en 1, ya que el 0 es el router
    {

      if (trafico)
        { //TRUE -> Audio + video
          tam_paq_cliente = random (MIN_VIDEO_CLIENTE, MAX_VIDEO_CLIENTE);
          tasa_cliente = 18600 * tam_paq_cliente; //18600 por medidas reales de wireshark
          h_onoff.SetAttribute ("OffTime",
                                StringValue ("ns3::ExponentialRandomVariable[Mean=0.004]")); //4ms
        }

      else
        { //FALSE -> Audio
          tam_paq_cliente = random (MIN_AUDIO_CLIENTE, MAX_AUDIO_CLIENTE);
          tasa_cliente = 4300 * tam_paq_cliente; //4300 por medidas reales de wireshark
          h_onoff.SetAttribute ("OffTime",
                                StringValue ("ns3::ExponentialRandomVariable[Mean=0.015]")); //15ms
        }
      h_onoff.SetAttribute ("OnTime",
                            StringValue ("ns3::ExponentialRandomVariable[Mean=0.003]")); //3ms

      h_onoff.SetAttribute ("PacketSize", UintegerValue (tam_paq_cliente));

      h_onoff.SetAttribute ("DataRate",
                            DataRateValue (tasa_cliente)); //Se establece el regimen binario a 8kbps

      ApplicationContainer c_app_temp = h_onoff.Install (c_lan2_fuentes.Get (i));
      c_app_lan2.Add (c_app_temp);
    }
  NS_LOG_DEBUG ("c_app_lan2 size: " << c_app_lan2.GetN ());

  ////////////////////////////////////////////////////
  //FIN CREACIÓN DE LAS FUENTES ONOFF UDP
  ////////////////////////////////////////////////////

  ////////////////////////////////////////////////////
  //CREACIÓN DE LAS FUENTES ONOFF UDP EN EL MISMO NODO
  ////////////////////////////////////////////////////

  //Añadimos al servidor UDP tantas fuentes OnOff como nodos en cada LAN

  ApplicationContainer c_app_onoff_all_in_one_node;

  for (int i = 1; i < nodos_lan1 + 1;
       i++) //Hay que sumar 1, ya que interfaces_lan1.GetAddress (0) es la IP del router
    {

      OnOffHelper h_onoff ("ns3::UdpSocketFactory",
                           InetSocketAddress (interfaces_lan1.GetAddress (i), 20000));

      if (trafico)
        { //TRUE -> Audio + video
          tam_paq_servidor = random (MIN_VIDEO_SERVER, MAX_VIDEO_SERVER);
          tasa_server = 5500 * tam_paq_servidor; //5500 por medidas reales de wireshark

          h_onoff.SetAttribute ("OffTime",
                                StringValue ("ns3::ExponentialRandomVariable[Mean=0.006]")); //6ms
        }

      else
        { //FALSE -> Audio
          tam_paq_servidor = random (MIN_AUDIO_SERVER, MAX_AUDIO_SERVER);
          tasa_server = 1260 * tam_paq_servidor; //1260 por medidas reales de wireshark
          h_onoff.SetAttribute ("OffTime",
                                StringValue ("ns3::ExponentialRandomVariable[Mean=0.02]")); //20ms
        }
      //Según wireshark el ontime es de ~0.3ms y el offtime es de ~3ms

      h_onoff.SetAttribute ("OnTime",
                            StringValue ("ns3::ExponentialRandomVariable[Mean=0.003]")); //3ms

      h_onoff.SetAttribute ("PacketSize", UintegerValue (tam_paq_servidor));

      h_onoff.SetAttribute ("DataRate",
                            DataRateValue (tasa_server)); //Se establece el regimen binario a 8kbps
      h_onoff.SetAttribute ("StartTime",
                            TimeValue (Time ("10s"))); //Se establece el tiempo de parada
      h_onoff.SetAttribute ("StopTime",
                            TimeValue (stop_time)); //Se establece el tiempo de parada

      ApplicationContainer OnOffAppTemp = h_onoff.Install (c_lan3.Get (2));
      c_app_onoff_all_in_one_node.Add (OnOffAppTemp);
    }

  //ApplicationContainer c_app_lan2_udp;
  //Añadimos al servidor UDP tantas fuentes OnOff como nodos en cada LAN
  for (int i = 1; i < nodos_lan2 + 1; i++)
    {
      OnOffHelper h_onoff ("ns3::UdpSocketFactory",
                           InetSocketAddress (interfaces_lan2.GetAddress (i), 20000));

      if (trafico)
        { //TRUE -> Audio + video
          tam_paq_servidor = random (MIN_VIDEO_SERVER, MAX_VIDEO_SERVER);
          tasa_server = 5500 * tam_paq_servidor; //5500 por medidas reales de wireshark

          h_onoff.SetAttribute ("OffTime",
                                StringValue ("ns3::ExponentialRandomVariable[Mean=0.006]")); //6ms
        }

      else
        { //FALSE -> Audio
          tam_paq_servidor = random (MIN_AUDIO_SERVER, MAX_AUDIO_SERVER);
          tasa_server = 1260 * tam_paq_servidor; //1260 por medidas reales de wireshark
          h_onoff.SetAttribute ("OffTime",
                                StringValue ("ns3::ExponentialRandomVariable[Mean=0.02]")); //20ms
        }

      //Según wireshark el ontime es de ~0.3ms y el offtime es de ~3ms

      h_onoff.SetAttribute ("OnTime",
                            StringValue ("ns3::ExponentialRandomVariable[Mean=0.003]")); //3ms

      h_onoff.SetAttribute ("PacketSize", UintegerValue (tam_paq_servidor));

      h_onoff.SetAttribute ("DataRate",
                            DataRateValue (tasa_server)); //Se establece el regimen binario a 8kbps
      h_onoff.SetAttribute ("StartTime",
                            TimeValue (Time ("10s"))); //Se establece el tiempo de parada
      h_onoff.SetAttribute ("StopTime",
                            TimeValue (stop_time)); //Se establece el tiempo de parada

      ApplicationContainer OnOffAppTemp = h_onoff.Install (c_lan3.Get (2));
      c_app_onoff_all_in_one_node.Add (OnOffAppTemp);
    }

  NS_LOG_DEBUG ("tam_paq_cliente aleatorio: " << tam_paq_cliente);
  NS_LOG_DEBUG ("tam_paq_server aleatorio: " << tam_paq_servidor);
  NS_LOG_DEBUG ("tasa_cliente aleatorio: " << tasa_cliente);
  NS_LOG_DEBUG ("tasa_servidor aleatorio: " << tasa_server);

  ////////////////////////////////////////////////////
  //FIN CREACIÓN DE LAS FUENTES ONOFF UDP EN EL MISMO NODO
  ////////////////////////////////////////////////////

  ////////////////////////////////////////////////////
  //CREACIÓN DE LA FUENTE ONOFF TCP
  ////////////////////////////////////////////////////

  OnOffHelper clientHelperTcp ("ns3::TcpSocketFactory", Address ());
  clientHelperTcp.SetAttribute ("PacketSize", UintegerValue (1500)); //1500 por ejemplo
  clientHelperTcp.SetAttribute ("OnTime",
                                StringValue ("ns3::ExponentialRandomVariable[Mean=0.03]"));
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

  Ptr<DropTailQueue<Packet>> cola_DP_lan1 = bridge_lan1->GetDevice (0)
                                                ->GetObject<CsmaNetDevice> ()
                                                ->GetQueue ()
                                                ->GetObject<DropTailQueue<Packet>> ();
  QueueSizeValue tam_cola1;
  cola_DP_lan1->SetAttribute ("MaxSize", QueueSizeValue (QueueSize (t_cola)));
  cola_DP_lan1->GetAttribute ("MaxSize", tam_cola1);
  Ptr<DropTailQueue<Packet>> cola_DP_lan2 = bridge_lan2->GetDevice (0)
                                                ->GetObject<CsmaNetDevice> ()
                                                ->GetQueue ()
                                                ->GetObject<DropTailQueue<Packet>> ();
  QueueSizeValue tam_cola2;
  cola_DP_lan2->SetAttribute ("MaxSize", QueueSizeValue (QueueSize (t_cola)));
  cola_DP_lan2->GetAttribute ("MaxSize", tam_cola2);
  Ptr<DropTailQueue<Packet>> cola_DP_lan3 = bridge_lan3->GetDevice (0)
                                                ->GetObject<CsmaNetDevice> ()
                                                ->GetQueue ()
                                                ->GetObject<DropTailQueue<Packet>> ();
  QueueSizeValue tam_cola3;
  cola_DP_lan3->SetAttribute ("MaxSize", QueueSizeValue (QueueSize (t_cola)));
  cola_DP_lan3->GetAttribute ("MaxSize", tam_cola3);

  NS_LOG_INFO ("Tamaño de la cola del puerto del switch L1: " << tam_cola1.Get ());
  NS_LOG_INFO ("Tamaño de la cola del puerto del switch L2: " << tam_cola2.Get ());
  NS_LOG_INFO ("Tamaño de la cola del puerto del switch L3: " << tam_cola3.Get ());

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

  Ptr<UdpServer> ptr_server_udp_lan1 = c_lan1.Get (3)->GetObject<UdpServer> ();
  //Retardo objeto_retardo (ptr_server_udp_lan1, c_app_onoff_all_in_one_node);

  //Nos suscribimos a la traza para saber la dirección del emisor cuando recibimos algún paquete
  //udpserver->TraceConnectWithoutContext ("RxWithAddresses", MakeCallback (&PacketReceivedWithAddress)); //LAN fuentes
  // udpServer->TraceConnectWithoutContext (
  //     "Rx", MakeCallback (&PacketReceivedWithoutAddress)); //LAN fuentes
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

  NS_LOG_INFO ("Retardo medio: " << objeto_retardo.GetRetardoMedio () * 2 << " ms");

  Simulator::Destroy ();

  return 2 * objeto_retardo.GetRetardoMedio ();
}

int
random (int low, int high)
{
  std::uniform_int_distribution<> dist (low, high);
  return dist (gen);
}

double
grafica (int nodos_lan1, int nodos_lan2, DataRate capacidad, Time stop_time, Time t_sim,
         double intervalo, std::string t_cola, bool trafico)
{
  int nodos_lan1_in = nodos_lan1;
  int nodos_lan2_in = nodos_lan2;

  int fuentes = nodos_lan1 + nodos_lan2;
  // int fuentes_iniciales = fuentes;
  double t_cola_aux = std::stod (t_cola);
  double t_cola_inicial = t_cola_aux;

  Gnuplot grafica;
  grafica.SetTitle ("GRAFICA TRABAJO");
  grafica.SetLegend ("Tamaño de la cola de control (paquetes)", "Retardo medio (ms)");

  for (int i = 1; i < NUM_CURVAS; i++)
    {
      Average<double> puntos;
      double IC = 0.0;
      Gnuplot2dDataset curva ("Fuentes: " + std::to_string (fuentes));
      curva.SetStyle (Gnuplot2dDataset::LINES_POINTS);
      curva.SetErrorBars (Gnuplot2dDataset::Y);

      for (uint32_t fac = 0; fac < NUM_PUNTOS; fac += 1)
        {
          // ABSCISAS
          for (uint32_t iteracion = 0; iteracion < ITERACIONES; iteracion++)
            {
              NS_LOG_DEBUG ("Generando el punto [" << iteracion << "] ...");
              std::string t_c_aux = std::to_string (t_cola_aux);
              // Procedemos a la simulación
              if (t_cola_aux < 1.0)
                {
                  t_c_aux = "1p";
                  NS_LOG_DEBUG ("ADVERTENCIA: Se ha alcanzado el tamaño mínimo de la cola, se "
                                "redondeará a 1p.");
                }
              else
                {
                  t_c_aux = t_c_aux + "p";
                  NS_LOG_DEBUG ("Punto [" << i << "]\tvalor de 'cola': " << t_c_aux);
                }

              // Actualizamos el punto actual con los datos obtenidos de la simulación

              double dato = escenario (nodos_lan1_in, nodos_lan2_in, capacidad, stop_time, t_sim,
                                       intervalo, t_c_aux, trafico);
              NS_LOG_DEBUG ("\n\tvalor [" << i << "]\t-> " << dato << "\n");
              puntos.Update (dato);
            }

          NS_LOG_DEBUG ("Generación de puntos finalizada");

          // Cálculo del intervalo de confianza
          IC = T_STUDENT_8_95 * sqrt (puntos.Var () / puntos.Count ());

          curva.Add (t_cola_aux, puntos.Avg (), IC);
          NS_LOG_DEBUG ("\n\n[======] PUNTO AÑADIDO A LA CURVA [======]\n\n");
          t_cola_aux = t_cola_aux + 5;
        }
      grafica.AddDataset (curva);

      // Actualización de los valores
      nodos_lan1_in = nodos_lan1_in + 5;
      nodos_lan2_in = nodos_lan2_in + 5;

      t_cola_aux = t_cola_inicial;
    }

  // Generación de ficheros
  NS_LOG_DEBUG ("Generando archivo 'grafica4.plt'...");
  std::ofstream fichero ("grafica4.plt");
  grafica.GenerateOutput (fichero);
  fichero << "pause -1" << std::endl;
  fichero.close ();
}

// double
// grafica (int nodos_lan1, int nodos_lan2, DataRate capacidad, Time stop_time, Time t_sim,
//          double intervalo, std::string t_cola, bool trafico)
// {

//   // NS_LOG_FUNCTION (n_fuentes << capacidad_tx << tamanio_paquetes << timeBtwPkts << n_paq << tamcola
//   //                            << simtime);

//   double colaAux = std::stod (t_cola);
//   //NS_LOG_DEBUG("colaAux:" << colaAux);

//   Gnuplot grafica;
//   grafica.SetTitle ("GRAFICA TRABAJO");
//   grafica.SetLegend ("Tamaño de la cola [paquetes]", "Perdida de paquetes [%]");

//   for (uint32_t i = 1; i < N_CURVAS; i++)
//     {

//       //Actualizamos los valores de los parámetros de las curvas

//       Average<double> puntos;
//       Gnuplot2dDataset curva ("Fuentes: " + std::to_string (n_fuentes));
//       curva.SetStyle (Gnuplot2dDataset::LINES_POINTS);
//       curva.SetErrorBars (Gnuplot2dDataset::Y);

//       for (uint32_t x = 0; x < N_PUNTOS; x++)
//         {

//           for (uint32_t iteracion = 0; iteracion < N_ITERACIONES; iteracion++)
//             {

//               NS_LOG_DEBUG ("Generando punto: " << iteracion);
//               // Realizamos la simulación

//               std::string colaAuxString = std::to_string (colaAux); //Vamos a manejar la cola
//               //NS_LOG_DEBUG("colaAuxString: " << colaAuxString);

//               if (colaAux < 1.0)
//                 { //Si la cola es menor que la mínima le cambiamos el valor a la minima

//                   colaAuxString = "1p";
//                   NS_LOG_DEBUG ("Se ha alcanzado el tamaño mínimo de la cola");
//                 }

//               Time stopTime = Time (std::to_string (n_paq * (timeBtwPkts.GetSeconds ())));
//               Time simTime = Time (std::to_string (n_paq * (timeBtwPkts.GetSeconds ()) +
//                                                    (timeBtwPkts.GetSeconds ())));

//               double colaAux_toDouble = std::stod (colaAuxString);
//               //NS_LOG_DEBUG("colaAux_toDouble: " << colaAux_toDouble);
//               std::stringstream temp;
//               temp << colaAux_toDouble << "p";
//               std::string colaAux_toDouble_toString = temp.str ();
//               //NS_LOG_DEBUG("colaAux_toDouble_toString: " << colaAux_toDouble_toString);

//               double dato = escenario (n_fuentes, capacidad_tx, tamanio_paquetes, timeBtwPkts,
//                                        n_paq, colaAux_toDouble_toString, simTime, stopTime, false);

//               double porcentaje = (dato / (n_paq * n_fuentes)) * 100;
//               puntos.Update (porcentaje);
//             }
//           //Añadimos el punto a la curva con IC=0

//           curva.Add (colaAux, puntos.Avg (), 0);

//           //Cambiamos el tamaño de la cola según el régimen exponencial binario
//           colaAux = pow (colaAux * 0.5, 1);

//           NS_LOG_DEBUG ("Punto añadido a la curva");
//         }

//       //Añadimos la curva a la gráfica
//       grafica.AddDataset (curva);

//       //Actualizamos valores
//       n_fuentes = n_fuentes + 8;

//       colaAux = std::stod (tamcola);
//     }

//   //Generamos el ficheros
//   std::ofstream fichero ("grafica4.plt");
//   grafica.GenerateOutput (fichero);
//   fichero << "pause -1" << std::endl;
//   fichero.close ();
// }

// void
// PacketReceivedWithAddress (const Ptr<const Packet> packet, const Address &srcAddress,
//                            const Address &destAddress)
// {
//   NS_LOG_INFO ("Paquete recibido: " << packet << ", dirección origen: " << srcAddress
//                                     << ", dirección destino: " << destAddress);
//   Ptr<Packet> copy = packet->Copy ();

//   // Headers must be removed in the order they're present.
//   // EthernetHeader ethernetHeader;
//   //copy->RemoveHeader(ethernetHeader);
//   Ipv4Header ipHeader;
//   NS_LOG_INFO ("IP peekHeader: " << copy->PeekHeader (ipHeader));

//   copy->RemoveHeader (ipHeader);

//   NS_LOG_INFO ("IP origen: " << ipHeader.GetSource ());
//   NS_LOG_INFO ("IP destino: " << ipHeader.GetDestination ());
// }
// void
// PacketReceivedWithoutAddress (Ptr<const Packet> packet)
// {
//   NS_LOG_INFO ("Paquete recibido, contenido: " << *packet);

//   for (uint32_t i = 0; i < c_app_onoff_all_in_one_node.GetN (); i++)
//     {
//       Ptr<OnOffApplication> onoff_app =
//           c_app_onoff_all_in_one_node.Get (i)->GetObject<OnOffApplication> ();
//       onoff_app->SetStartTime (Simulator::Now ());
//       onoff_app->SetStopTime (Time ("1ms"));

//       // c_app_onoff_all_in_one_node.Start(Simulator::Now());
//       // c_app_onoff_all_in_one_node.Stop(Time("1ms"));
//     }
//   //NS_LOG_INFO ("Paquete de respuesta enviado");

//   // Ptr<Packet> copy = packet->Copy ();

//   // // Headers must be removed in the order they're present.
//   // PppHeader pppHeader;

//   // EthernetHeader ethernetHeader;
//   // //copy->RemoveHeader(ethernetHeader);
//   // GenericMacHeader macHeader;
//   // Ipv4Header ipHeader;
//   // // copy->RemoveHeader(ipHeader);
//   // UdpHeader udpHeader;

//   // PacketMetadata::ItemIterator metadataIterator = copy->BeginItem ();
//   // PacketMetadata::Item item;
//   // NS_LOG_INFO ("Fuera");

//   // while (metadataIterator.HasNext ())
//   //   {
//   //     NS_LOG_INFO ("Dentro");

//   //     item = metadataIterator.Next ();
//   //     // item.RemoveHeader (ipHeader, 20);
//   //     // NS_LOG_INFO ("HECHO");
//   //     NS_LOG_INFO ("item name: " << item.tid.GetName ());

//   //     if (item.tid.GetName () == "ns3::Ipv4Header")
//   //       {
//   //         NS_LOG_INFO ("ENCONTRADA");
//   //       }
//   //     break;
//   //   }

//   // NS_LOG_INFO ("Tamaño del paquete: " << copy->GetSize ());

//   // copy->PeekHeader (ethernetHeader);
//   // NS_LOG_INFO ("ip header type: " << ipHeader.GetTypeId ());

//   // // if (llc.GetType () == 0x0806)
//   // //   {
//   // //     // found an ARP packet
//   // //   }

//   // //NS_LOG_INFO ("IP peekHeader: " << copy->PeekHeader (ipHeader));

//   // NS_LOG_INFO ("ppp removeHeader: " << copy->RemoveHeader (pppHeader));

//   // // NS_LOG_INFO ("generic mac removeHeader: " << copy->RemoveHeader (macHeader));

//   // // NS_LOG_INFO ("Ethernet removeHeader: " << copy->RemoveHeader (ethernetHeader));

//   // NS_LOG_INFO ("IP removeHeader: " << copy->RemoveHeader (ipHeader));
//   // // NS_LOG_INFO ("UDP removeHeader: " << copy->RemoveHeader (udpHeader));

//   // // NS_LOG_INFO ("header size ethernet: " << ethernetHeader.GetHeaderSize ());
//   // // // NS_LOG_INFO ("IP size ethernet: " << ipHeader.GetHeaderSize());

//   // NS_LOG_INFO ("Payload size: " << ipHeader.GetPayloadSize ());

//   // NS_LOG_INFO ("IP origen: " << ipHeader.GetSource ());
//   // NS_LOG_INFO ("IP destino: " << ipHeader.GetDestination ());
// }
