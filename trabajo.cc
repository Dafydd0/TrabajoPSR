#include "retardo.h"
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

#define NUM_CURVAS 2
#define NUM_PUNTOS 8
#define ITERACIONES 10
#define T_STUDENT_16_95 1.7459
#define T_STUDENT_8_95 1.8595

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Trabajo");

int random (int low, int high);

Ptr<Node> PuenteHelper (NodeContainer nodosLan, NetDeviceContainer &d_nodosLan, DataRate tasa);

double escenario (int nodos_lan1, int nodos_lan2, DataRate capacidad_lan1, DataRate capacidad_lan2,
                  DataRate capacidad_lan3, Time stop_time, Time t_sim, std::string t_cola,
                  bool trafico);

void grafica (int nodos_lan1, int nodos_lan2, DataRate capacidad_lan1, DataRate capacidad_lan2,
              DataRate capacidad_lan3, Time stop_time, Time t_sim, std::string t_cola,
              bool trafico);

void grafica_perd (int nodos_lan1, int nodos_lan2, DataRate capacidad_lan1, DataRate capacidad_lan2,
                   DataRate capacidad_lan3, Time stop_time, Time t_sim, std::string t_cola,
                   bool trafico);

//Para la generación de numeros aleatorios
std::random_device rd;
std::mt19937 gen (rd ());

/**
 *  Función [main]
 * Es la esencial para el correcto funcionamiento del programa. Será la encargada de ejecutar el escenario que corresponda
 * en cada momento.
*/
int
main (int argc, char *argv[])
{

  Time::SetResolution (Time::NS);

  CommandLine cmd;
  int nFuentes1 = 3; // Número de Fuentes LAN 1
  int nFuentes2 = 3; // Número de Fuentes LAN 2

  // int tam_pkt = 118; // Tamaño del Paquete
  Time stop_time ("120s"); // Tiempo de parada para las fuentes
  DataRate cap_l1 ("100Mbps"); // Capacidad de transmisión (100Mb/s)
  DataRate cap_l2 ("100Mbps"); // Capacidad de transmisión (100Mb/s)
  DataRate cap_l3 ("100Mbps"); // Capacidad de transmisión (100Mb/s)

  Time tSim ("120s"); // Tiempo de Simulación

  std::string tam_cola = "100p"; // Tamaño de  la cola

  bool trafico = true;

  cmd.AddValue ("nFuentes1", "Número de fuentes en la LAN 1", nFuentes1);
  cmd.AddValue ("nFuentes2", "Número de fuentes en la LAN 2", nFuentes2);

  cmd.AddValue ("stopTime", "Tiempo de parada para las fuentes", stop_time);
  cmd.AddValue ("cap_l1", "Capacidad de transmision LAN 1", cap_l1);
  cmd.AddValue ("cap_l2", "Capacidad de transmision LAN 2", cap_l2);
  cmd.AddValue ("cap_l3", "Capacidad de transmision LAN 3", cap_l3);

  cmd.AddValue ("tSim", "Tiempo de simulación", tSim);
  cmd.AddValue ("tam_cola", "Tamaño de las colas", tam_cola);
  cmd.AddValue ("trafico", "Selecciona tipo de tráfico FALSE -> Audio | TRUE -> Audio + Video",
                trafico);
  cmd.Parse (argc, argv);

  grafica (nFuentes1, nFuentes2, cap_l1, cap_l2, cap_l3, stop_time, tSim, tam_cola, trafico);
  //grafica_perd (nFuentes1, nFuentes2, cap_l1, cap_l2, cap_l3, stop_time, tSim, tam_cola, trafico);
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
 *  Función [escenario]
 * Al ejecutar esta función se realizarán las simulaciones en
 * función de los parámetros y condiciones que se requieran.
*/
double
escenario (int nodos_lan1, int nodos_lan2, DataRate capacidad_lan1, DataRate capacidad_lan2,
           DataRate capacidad_lan3, Time stop_time, Time t_sim, std::string t_cola, bool trafico)
{
  ////////////////////////////////////////////////////
  //Implementación del router
  ////////////////////////////////////////////////////
  NodeContainer c_routers;
  c_routers.Create (3);

  NodeContainer r0r1 = NodeContainer (c_routers.Get (0), c_routers.Get (1));
  NodeContainer r0r2 = NodeContainer (c_routers.Get (0), c_routers.Get (2));
  NodeContainer r1r2 = NodeContainer (c_routers.Get (1), c_routers.Get (2));

  PointToPointHelper p2p;
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
  Ptr<Node> bridge_lan1 = PuenteHelper (c_lan1, c_lan1_router_fuentes, capacidad_lan1);

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
  Ptr<Node> bridge_lan2 = PuenteHelper (c_lan2, c_lan2_router_fuentes, capacidad_lan2);

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
  Ptr<Node> bridge_lan3 = PuenteHelper (c_lan3, c_lan3_router_otros, capacidad_lan3);

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
  int tam_paq_cliente = 0;
  int tam_paq_servidor = 0;
  DataRate tasa_cliente;
  DataRate tasa_server;

  ApplicationContainer c_app_lan1;
  ApplicationContainer c_app_lan2;

  /*
  APLICACIONES ONOFF TCP
  */
  OnOffHelper h_onoff ("ns3::UdpSocketFactory",
                       InetSocketAddress (interfaces_lan3.GetAddress (2),
                                          puerto_udp.Get ())); //El 2 es el Servidor UDP

  OnOffHelper clientHelperTcpL1 ("ns3::TcpSocketFactory", Address ());
  clientHelperTcpL1.SetAttribute ("PacketSize", UintegerValue (4096));
  clientHelperTcpL1.SetAttribute ("OnTime",
                                  StringValue ("ns3::ExponentialRandomVariable[Mean=0.003]"));
  clientHelperTcpL1.SetAttribute ("OffTime",
                                  StringValue ("ns3::ExponentialRandomVariable[Mean=0.01]"));
  clientHelperTcpL1.SetAttribute ("DataRate", StringValue ("1Mbps"));

  ApplicationContainer clientAppTcp; // Nodo admin

  AddressValue remoteAddressL1 (InetSocketAddress (interfaces_lan3.GetAddress (3), puerto_tcp));

  clientHelperTcpL1.SetAttribute ("Remote", remoteAddressL1);

  for (uint32_t i = 1; i < c_lan1_fuentes.GetN (); i++) // Se empieza en 1, ya que el 0 es el router
    {
      if (trafico)
        { //TRUE -> Audio + video
          tam_paq_cliente = random (MIN_VIDEO_CLIENTE, MAX_VIDEO_CLIENTE);
          tasa_cliente = ((18600 / 60) * tam_paq_cliente) /
                         (c_lan1_fuentes.GetN ()); //18600 por medidas reales de wireshark
          h_onoff.SetAttribute ("OffTime",
                                StringValue ("ns3::ExponentialRandomVariable[Mean=0.0004]")); //4ms
        }

      else
        { //FALSE -> Audio
          tam_paq_cliente = random (MIN_AUDIO_CLIENTE, MAX_AUDIO_CLIENTE);
          tasa_cliente = ((4300 / 60) * tam_paq_cliente) /
                         (c_lan1_fuentes.GetN ()); //4300 por medidas reales de wireshark
          h_onoff.SetAttribute ("OffTime",
                                StringValue ("ns3::ExponentialRandomVariable[Mean=0.0015]")); //15ms
        }
      h_onoff.SetAttribute ("OnTime",
                            StringValue ("ns3::ExponentialRandomVariable[Mean=0.003]")); //3ms
      h_onoff.SetAttribute ("PacketSize", UintegerValue (tam_paq_cliente));

      h_onoff.SetAttribute ("DataRate", DataRateValue (tasa_cliente));

      h_onoff.SetAttribute ("StartTime", TimeValue (Time ("2s")));
      h_onoff.SetAttribute ("StopTime",
                            TimeValue (stop_time)); //Se establece el tiempo de parada

      ApplicationContainer c_app_temp = h_onoff.Install (c_lan1_fuentes.Get (i));
      c_app_lan1.Add (c_app_temp);

      //Cliente TCP
      clientAppTcp.Add (clientHelperTcpL1.Install (c_lan1.Get (i)));
    }

  for (uint32_t i = 1; i < c_lan2_fuentes.GetN (); i++) // Se empieza en 1, ya que el 0 es el router
    {

      if (trafico)
        { //TRUE -> Audio + video
          tam_paq_cliente = random (MIN_VIDEO_CLIENTE, MAX_VIDEO_CLIENTE);
          tasa_cliente = ((18600 / 60) * tam_paq_cliente) /
                         (c_lan2_fuentes.GetN ()); //18600 por medidas reales de wireshark
          h_onoff.SetAttribute ("OffTime",
                                StringValue ("ns3::ExponentialRandomVariable[Mean=0.0004]")); //4ms
        }

      else
        { //FALSE -> Audio
          tam_paq_cliente = random (MIN_AUDIO_CLIENTE, MAX_AUDIO_CLIENTE);
          tasa_cliente = ((4300 / 60) * tam_paq_cliente) /
                         (c_lan2_fuentes.GetN ()); //4300 por medidas reales de wireshark
          h_onoff.SetAttribute ("OffTime",
                                StringValue ("ns3::ExponentialRandomVariable[Mean=0.0015]")); //15ms
        }
      h_onoff.SetAttribute ("OnTime",
                            StringValue ("ns3::ExponentialRandomVariable[Mean=0.003]")); //3ms

      h_onoff.SetAttribute ("PacketSize", UintegerValue (tam_paq_cliente));

      h_onoff.SetAttribute ("DataRate", DataRateValue (tasa_cliente));

      h_onoff.SetAttribute ("StartTime", TimeValue (Time ("2s")));
      h_onoff.SetAttribute ("StopTime",
                            TimeValue (stop_time)); //Se establece el tiempo de parada

      ApplicationContainer c_app_temp = h_onoff.Install (c_lan2_fuentes.Get (i));
      c_app_lan2.Add (c_app_temp);

      clientAppTcp.Add (clientHelperTcpL1.Install (c_lan2.Get (i)));
    }

  clientAppTcp.Start (Seconds (2.0));
  clientAppTcp.Stop (stop_time);

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
          tasa_server = ((5500 / 60) * tam_paq_servidor) /
                        (nodos_lan1); //5500 por medidas reales de wireshark

          h_onoff.SetAttribute ("OffTime",
                                StringValue ("ns3::ExponentialRandomVariable[Mean=0.0006]")); //6ms
        }

      else
        { //FALSE -> Audio
          tam_paq_servidor = random (MIN_AUDIO_SERVER, MAX_AUDIO_SERVER);
          tasa_server = ((1260 / 60) * tam_paq_servidor) /
                        (nodos_lan1); //1260 por medidas reales de wireshark
          h_onoff.SetAttribute ("OffTime",
                                StringValue ("ns3::ExponentialRandomVariable[Mean=0.002]")); //20ms
        }

      h_onoff.SetAttribute ("OnTime",
                            StringValue ("ns3::ExponentialRandomVariable[Mean=0.003]")); //3ms

      h_onoff.SetAttribute ("PacketSize", UintegerValue (tam_paq_servidor));

      h_onoff.SetAttribute ("DataRate", DataRateValue (tasa_server));

      h_onoff.SetAttribute ("StartTime", TimeValue (Time ("10s")));
      h_onoff.SetAttribute ("StopTime",
                            TimeValue (stop_time)); //Se establece el tiempo de parada

      ApplicationContainer OnOffAppTemp = h_onoff.Install (c_lan3.Get (2));
      c_app_onoff_all_in_one_node.Add (OnOffAppTemp);
    }

  //Añadimos al servidor UDP tantas fuentes OnOff como nodos en cada LAN
  for (int i = 1; i < nodos_lan2 + 1; i++)
    {
      OnOffHelper h_onoff ("ns3::UdpSocketFactory",
                           InetSocketAddress (interfaces_lan2.GetAddress (i), 20000));

      if (trafico)
        { //TRUE -> Audio + video
          tam_paq_servidor = random (MIN_VIDEO_SERVER, MAX_VIDEO_SERVER);
          tasa_server = ((5500 / 60) * tam_paq_servidor) /
                        (nodos_lan2); //5500 por medidas reales de wireshark

          h_onoff.SetAttribute ("OffTime",
                                StringValue ("ns3::ExponentialRandomVariable[Mean=0.0006]")); //6ms
        }

      else
        { //FALSE -> Audio
          tam_paq_servidor = random (MIN_AUDIO_SERVER, MAX_AUDIO_SERVER);
          tasa_server = ((1260 / 60) * tam_paq_servidor) /
                        (nodos_lan2); //1260 por medidas reales de wireshark
          h_onoff.SetAttribute ("OffTime",
                                StringValue ("ns3::ExponentialRandomVariable[Mean=0.002]")); //20ms
        }

      h_onoff.SetAttribute ("OnTime",
                            StringValue ("ns3::ExponentialRandomVariable[Mean=0.003]")); //3ms

      h_onoff.SetAttribute ("PacketSize", UintegerValue (tam_paq_servidor));

      h_onoff.SetAttribute ("DataRate", DataRateValue (tasa_server));

      h_onoff.SetAttribute ("StartTime", TimeValue (Time ("10s")));
      h_onoff.SetAttribute ("StopTime",
                            TimeValue (stop_time)); //Se establece el tiempo de parada

      ApplicationContainer OnOffAppTemp = h_onoff.Install (c_lan3.Get (2));
      c_app_onoff_all_in_one_node.Add (OnOffAppTemp);
    }

  NS_LOG_DEBUG (
      "Numero de aplicaciones OnOff en el nodo servidor: " << c_app_onoff_all_in_one_node.GetN ());

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
                                StringValue ("ns3::ExponentialRandomVariable[Mean=0.085]"));
  clientHelperTcp.SetAttribute ("DataRate",
                                StringValue ("64kbps")); //Se establece el regimen binario a 64kbps

  ApplicationContainer clientApp; // Nodo admin

  AddressValue remoteAddress (InetSocketAddress (interfaces_lan3.GetAddress (3), puerto_tcp));
  clientHelperTcp.SetAttribute ("Remote", remoteAddress);
  clientApp.Add (
      clientHelperTcp.Install (c_lan3.Get (1))); // Instala el servidor TCP en el nodo admin

  clientApp.Start (Seconds (5.0));
  clientApp.Stop (stop_time);

  ////////////////////////////////////////////////////
  //FIN CREACIÓN DE LA FUENTE ONOFF TCP
  ////////////////////////////////////////////////////

  //Cambiamos el tamaño de las colas (por defecto a 100)
  QueueSizeValue tam_cola1;
  QueueSizeValue tam_cola2;
  QueueSizeValue tam_cola3;

  for (uint32_t i = 0; i < c_lan1.GetN (); i++)
    {
      Ptr<DropTailQueue<Packet>> cola_DP_lan1 = bridge_lan1->GetDevice (i)
                                                    ->GetObject<CsmaNetDevice> ()
                                                    ->GetQueue ()
                                                    ->GetObject<DropTailQueue<Packet>> ();
      cola_DP_lan1->SetAttribute ("MaxSize", QueueSizeValue (QueueSize (t_cola)));
      cola_DP_lan1->GetAttribute ("MaxSize", tam_cola1);
    }
  for (uint32_t i = 0; i < c_lan2.GetN (); i++)
    {
      Ptr<DropTailQueue<Packet>> cola_DP_lan2 = bridge_lan2->GetDevice (i)
                                                    ->GetObject<CsmaNetDevice> ()
                                                    ->GetQueue ()
                                                    ->GetObject<DropTailQueue<Packet>> ();
      cola_DP_lan2->SetAttribute ("MaxSize", QueueSizeValue (QueueSize (t_cola)));
      cola_DP_lan2->GetAttribute ("MaxSize", tam_cola2);
    }
  for (uint32_t i = 0; i < c_lan3.GetN (); i++)
    {
      Ptr<DropTailQueue<Packet>> cola_DP_lan3 = bridge_lan3->GetDevice (i)
                                                    ->GetObject<CsmaNetDevice> ()
                                                    ->GetQueue ()
                                                    ->GetObject<DropTailQueue<Packet>> ();
      cola_DP_lan3->SetAttribute ("MaxSize", QueueSizeValue (QueueSize (t_cola)));
      cola_DP_lan3->GetAttribute ("MaxSize", tam_cola3);
    }
  NS_LOG_INFO ("Tamaño de la cola del puerto del switch L1: " << tam_cola1.Get ());
  NS_LOG_INFO ("Tamaño de la cola del puerto del switch L2: " << tam_cola2.Get ());
  NS_LOG_INFO ("Tamaño de la cola del puerto del switch L3: " << tam_cola3.Get ());

  NS_LOG_DEBUG ("Aplicaciones en el contenedor de aplicaciones del nodo servidor: "
                << c_app_onoff_all_in_one_node.GetN ());

  NS_LOG_DEBUG ("Aplicaciones en el contenedor de aplicaciones TCP: " << clientAppTcp.GetN ());

  NS_LOG_DEBUG ("Creando objeto_retardo...");
  Ptr<NetDevice> nodo_transmisor =
      c_lan1.Get ((c_lan1.GetN () - 1))->GetDevice (0); //El emisor será el último nodo de la LAN 1
  Ptr<NetDevice> nodo_receptor =
      c_lan3.Get (2)->GetDevice (0); //El receptor será el servidor UDP de la LAN 3

  Retardo objeto_retardo = Retardo (nodo_transmisor, nodo_receptor);

  Simulator::Stop (stop_time);
  Simulator::Run ();
  NS_LOG_INFO ("--[Simulación completada]--");

  Ptr<Queue<Packet>> colaL1 = bridge_lan1->GetDevice (0)->GetObject<CsmaNetDevice> ()->GetQueue ();
  Ptr<Queue<Packet>> colaL2 = bridge_lan2->GetDevice (0)->GetObject<CsmaNetDevice> ()->GetQueue ();
  Ptr<Queue<Packet>> colaL3 = bridge_lan3->GetDevice (0)->GetObject<CsmaNetDevice> ()->GetQueue ();

  NS_LOG_INFO ("Paquetes perdidos en la cola L1: " << colaL1->GetTotalDroppedPackets ());
  NS_LOG_INFO ("Paquetes perdidos en la cola L2: " << colaL2->GetTotalDroppedPackets ());
  NS_LOG_INFO ("Paquetes perdidos en la cola L3: " << colaL3->GetTotalDroppedPackets ());

  NS_LOG_INFO ("LAN 1: " << c_lan1.GetN () << ", LAN 2: " << c_lan2.GetN ()
                         << ", LAN 3: " << c_lan3.GetN ());

  //Imprimimos el número de paquetes recibidos en cada puerto
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

  double retardo_final = (objeto_retardo.GetRetardoMedio () * 2) / 10;
  NS_LOG_INFO (
      "Llamando al método GetReceived() del nodo servidor UDP: " << udpServer->GetReceived ());
  NS_LOG_INFO ("Llamando al método GetLost() del nodo servidor UDP: " << udpServer->GetLost ());
  NS_LOG_INFO ("Retardo medio: " << retardo_final << " ms");

  Ptr<Queue<Packet>> cola_aux_l1 = bridge_lan1->GetDevice (0)
                                       ->GetObject<CsmaNetDevice> ()
                                       ->GetQueue ()
                                       ->GetObject<DropTailQueue<Packet>> ();
  Ptr<Queue<Packet>> cola_aux_l2 = bridge_lan1->GetDevice (0)
                                       ->GetObject<CsmaNetDevice> ()
                                       ->GetQueue ()
                                       ->GetObject<DropTailQueue<Packet>> ();

  double paq_rec_l1 = cola_aux_l1->GetTotalReceivedPackets ();
  double paq_rec_l2 = cola_aux_l2->GetTotalReceivedPackets ();

  double paq_rec_totales = paq_rec_l1 + paq_rec_l2;

  double paq_perd_l1 = colaL1->GetTotalDroppedPackets ();
  double paq_perd_l2 = colaL2->GetTotalDroppedPackets ();
  double paq_perd_l3 = colaL3->GetTotalDroppedPackets ();

  double total_paquetes_perdidos = paq_perd_l1 + paq_perd_l2 + paq_perd_l3;

  int usuarios_totales = nodos_lan1 + nodos_lan2;

  double porc_paq_perd = (total_paquetes_perdidos / (paq_rec_totales)) * 100;

  Simulator::Destroy ();

  NS_LOG_INFO ("% paq perdidos: " << porc_paq_perd);

  return retardo_final;

  //return porc_paq_perd;
}

/**
 *  Función [random]
 * Función que nos permite generar números
 * aleatorios dentro de un determinado rango
*/
int
random (int low, int high)
{
  std::uniform_int_distribution<> dist (low, high);
  return dist (gen);
}

/**
 *  Función [grafica]
 * Función que generará la gráfica de 
 * retardo medio en función del número de fuentes
*/

void
grafica (int nodos_lan1, int nodos_lan2, DataRate capacidad_lan1, DataRate capacidad_lan2,
         DataRate capacidad_lan3, Time stop_time, Time t_sim, std::string t_cola, bool trafico)
{
  int nodos_lan1_in = nodos_lan1;
  int nodos_lan2_in = nodos_lan2;

  double colaAux = std::stod (t_cola);

  Gnuplot grafica;
  grafica.SetTitle (
      "Tasa de los enlaces - LAN 1: " + std::to_string (capacidad_lan1.GetBitRate () * 0.0000010) +
      "Mbps" + " - LAN 2: " + std::to_string (capacidad_lan2.GetBitRate () * 0.0000010) + "Mbps" +
      " - LAN 3: " + std::to_string (capacidad_lan3.GetBitRate () * 0.0000010) + "Mbps");
  grafica.SetLegend ("Numero de fuentes [fuentes]", "Retardo medio [ms]");

  for (int i = 1; i < NUM_CURVAS; i++)
    {
      Average<double> puntos;
      double IC = 0.0;
      Gnuplot2dDataset curva ("Número inicial de fuentes: " +
                              std::to_string (nodos_lan1_in + nodos_lan2_in));
      curva.SetStyle (Gnuplot2dDataset::LINES_POINTS);
      curva.SetErrorBars (Gnuplot2dDataset::Y);

      for (uint32_t fac = 0; fac < NUM_PUNTOS; fac += 1)
        {
          // ABSCISAS
          for (uint32_t iteracion = 0; iteracion < ITERACIONES; iteracion++)
            {
              NS_LOG_DEBUG ("Generando el punto [" << iteracion << "] ...");
              std::string colaAuxString = std::to_string (
                  colaAux); //Vamos a manejar la cola              // Procedemos a la simulación

              //Convertir la cola de string a double
              double colaAux_toDouble = std::stod (colaAuxString);
              std::stringstream temp;
              temp << colaAux_toDouble << "p";
              std::string colaAux_toDouble_toString = temp.str ();

              // Actualizamos el punto actual con los datos obtenidos de la simulación

              double dato =
                  escenario (nodos_lan1_in, nodos_lan2_in, capacidad_lan1, capacidad_lan2,
                             capacidad_lan3, stop_time, t_sim, colaAux_toDouble_toString, trafico);
              NS_LOG_DEBUG ("\n\tvalor [" << iteracion << "]\t-> " << dato << "\n");
              puntos.Update (dato);
            }

          NS_LOG_DEBUG ("Generación de puntos finalizada");

          // Cálculo del intervalo de confianza
          IC = T_STUDENT_8_95 * sqrt (puntos.Var () / puntos.Count ());

          curva.Add (nodos_lan1_in + nodos_lan2_in, puntos.Avg (), IC);
          NS_LOG_DEBUG ("\n\n[======] PUNTO AÑADIDO A LA CURVA [======]\n\n");
          //colaAux = colaAux + 10;
          nodos_lan1_in = nodos_lan1_in + 5;
          nodos_lan2_in = nodos_lan2_in + 5;
        }
      grafica.AddDataset (curva);
      colaAux = std::stod (t_cola);
    }

  // Generación de ficheros
  NS_LOG_DEBUG ("Generando archivo 'grafica.plt'...");

  std::ofstream fichero ("grafica.plt");
  grafica.GenerateOutput (fichero);
  fichero << "pause -1" << std::endl;
  fichero.close ();
}

/**
 *  Función [grafica_perd]
 * Función que generará la gráfica de porcentaje
 * de paquetes perdidos en función del número de fuentes
*/

void
grafica_perd (int nodos_lan1, int nodos_lan2, DataRate capacidad_lan1, DataRate capacidad_lan2,
              DataRate capacidad_lan3, Time stop_time, Time t_sim, std::string t_cola, bool trafico)
{
  int nodos_lan1_in = nodos_lan1;
  int nodos_lan2_in = nodos_lan2;

  double colaAux = std::stod (t_cola);

  Gnuplot grafica;
  grafica.SetTitle (
      "Tasa de los enlaces - LAN 1: " + std::to_string (capacidad_lan1.GetBitRate () * 0.0000010) +
      "Mbps" + " - LAN 2: " + std::to_string (capacidad_lan2.GetBitRate () * 0.0000010) + "Mbps" +
      " - LAN 3: " + std::to_string (capacidad_lan3.GetBitRate () * 0.0000010) + "Mbps");
  grafica.SetLegend ("Numero de fuentes [fuentes]", "Porcentaje de paquetes perdidos [%]");

  for (int i = 1; i < NUM_CURVAS; i++)
    {
      Average<double> puntos;
      double IC = 0.0;
      Gnuplot2dDataset curva ("Número inicial de fuentes: " +
                              std::to_string (nodos_lan1_in + nodos_lan2_in));
      curva.SetStyle (Gnuplot2dDataset::LINES_POINTS);
      curva.SetErrorBars (Gnuplot2dDataset::Y);

      for (uint32_t fac = 0; fac < NUM_PUNTOS; fac += 1)
        {
          // ABSCISAS
          for (uint32_t iteracion = 0; iteracion < ITERACIONES; iteracion++)
            {
              NS_LOG_DEBUG ("Generando el punto [" << iteracion << "] ...");
              std::string colaAuxString = std::to_string (
                  colaAux); //Vamos a manejar la cola              // Procedemos a la simulación

              //Convertir la cola de string a double
              double colaAux_toDouble = std::stod (colaAuxString);
              std::stringstream temp;
              temp << colaAux_toDouble << "p";
              std::string colaAux_toDouble_toString = temp.str ();

              // Actualizamos el punto actual con los datos obtenidos de la simulación

              double dato =
                  escenario (nodos_lan1_in, nodos_lan2_in, capacidad_lan1, capacidad_lan2,
                             capacidad_lan3, stop_time, t_sim, colaAux_toDouble_toString, trafico);
              NS_LOG_DEBUG ("\n\tvalor [" << iteracion << "]\t-> " << dato << "\n");
              puntos.Update (dato);
            }

          NS_LOG_DEBUG ("Generación de puntos finalizada");

          // Cálculo del intervalo de confianza
          IC = T_STUDENT_8_95 * sqrt (puntos.Var () / puntos.Count ());

          curva.Add (nodos_lan1_in + nodos_lan2_in, puntos.Avg (), IC);
          NS_LOG_DEBUG ("\n\n[======] PUNTO AÑADIDO A LA CURVA [======]\n\n");
          //colaAux = colaAux + 10;
          nodos_lan1_in = nodos_lan1_in + 5;
          nodos_lan2_in = nodos_lan2_in + 5;
        }
      grafica.AddDataset (curva);
      colaAux = std::stod (t_cola);
    }

  // Generación de ficheros
  NS_LOG_DEBUG ("Generando archivo 'grafica.plt'...");

  std::ofstream fichero ("grafica.plt");
  grafica.GenerateOutput (fichero);
  fichero << "pause -1" << std::endl;
  fichero.close ();
}