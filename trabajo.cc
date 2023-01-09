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

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Trabajo");

Ptr<Node> PuenteHelper (NodeContainer nodosLan, NetDeviceContainer &d_nodosLan, DataRate tasa);
double escenario (int nodos, DataRate capacidad, int tam_paq, Time stop_time, Time t_sim, double intervalo,
                   std::string t_cola);

/**
 *  Función [main]
 * Es la esencial para el correcto funcionamiento del programa. Será la encargada de ejecutar el escenario que corresponda
 * en cada momento.
*/
int
main (int argc, char *argv[])
{

  Time::SetResolution(Time::NS);
  
  CommandLine cmd;
  int nFuentes = 6; // Número de Fuentes
  int tam_pkt = 1024; // Tamaño del Paquete
  Time stop_time ("20s"); // Tiempo de parada para las fuentes
  DataRate cap_tx ("100000kb/s"); // Capacidad de transmisión (100Mb/s)
  Time tSim ("20s"); // Tiempo de Simulación
  double intervalo = 0.001; // Intervalo entre paquetes

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
  
    escenario(nFuentes, cap_tx, tam_pkt, stop_time, tSim, intervalo, tam_cola);
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
  NetDeviceContainer d_puertosBridge;
  CsmaHelper h_csma;
  BridgeHelper h_bridge;
  Ptr<Node> puente = CreateObject<Node> ();
  h_csma.SetChannelAttribute ("DataRate", DataRateValue (tasa));
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
escenario (int nodos, DataRate capacidad, int tam_paq, Time stop_time, Time t_sim,
            double intervalo, std::string t_cola)
{

  //CREACIÓN DE LA LAN CONECTADA AL SWITCH

  NS_LOG_INFO ("Hay [" << nodos << "] Fuentes");
  NodeContainer c_fuentes;
  c_fuentes.Create (nodos);
  Ptr<Node> n_servidor = CreateObject<Node> ();
  NodeContainer c_todos (n_servidor);
  c_todos.Add (c_fuentes);

  NS_LOG_DEBUG ("Creando InternetStackHelper...");
  InternetStackHelper h_pila;
  h_pila.SetIpv6StackInstall (false);
  h_pila.Install (c_todos);

  NetDeviceContainer c_dispositivos;
  NS_LOG_DEBUG ("Creando puente...");
  Ptr<Node> bridge = PuenteHelper (c_todos, c_dispositivos, capacidad);

    //Cambiamos MTU para no tener que fragmentar
  for (uint32_t i = 0; i < c_dispositivos.GetN (); i++)
    {
      c_dispositivos.Get (i)->GetObject<CsmaNetDevice> ()->SetMtu (10000);
    }

  NS_LOG_DEBUG ("Creando Ipv4AddressHelper e Ipv4InterfaceContainer...");
  Ipv4AddressHelper h_direcciones ("10.20.30.0", "255.255.255.0");
  Ipv4InterfaceContainer c_interfaces = h_direcciones.Assign (c_dispositivos);
  NS_LOG_DEBUG ("Asignando direcciones IP...");
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  NS_LOG_DEBUG ("Creando UdpServer...");
  Ptr<UdpServer> udpserver = CreateObject<UdpServer> ();
  c_todos.Get (0)->AddApplication (udpserver);

  UintegerValue puerto;
  udpserver->GetAttribute ("Port", puerto);

  OnOffHelper h_onoff ("ns3::UdpSocketFactory",
                       InetSocketAddress (c_interfaces.GetAddress (0), puerto.Get ()));
  h_onoff.SetAttribute ("PacketSize", UintegerValue (tam_paq));
  h_onoff.SetAttribute ("OnTime", StringValue ("ns3::ExponentialRandomVariable[Mean=0.35]")); //Se establece el atributo OnTime a 0.35
  h_onoff.SetAttribute ("OffTime", StringValue ("ns3::ExponentialRandomVariable[Mean=0.65]")); //Se establece el atributo Offtime a 0.65
  h_onoff.SetAttribute ("DataRate", StringValue ("64kbps")); //Se establece el regimen binario a 16kbps
  h_onoff.SetAttribute ("StopTime", TimeValue (stop_time)); //Se establece el tiempo de parada a los 50 segundos
  NS_LOG_DEBUG("Atributos del objeto h_onoff modificados");
  
  IntegerValue reg_binFuente = tam_paq * 8 / intervalo;
  NS_LOG_INFO ("Tiempo entre paquetes: " << intervalo << "s");
  NS_LOG_INFO ("Régimen binario de las fuentes: " << reg_binFuente.Get () << " bps");

  DataRate rgFuentes (reg_binFuente.Get ());
  //h_onoff.SetConstantRate (rgFuentes, tam_paq);

  ApplicationContainer c_app = h_onoff.Install (c_fuentes);

  for (int i = 0; i < nodos; i++)
    {
      c_app.Get (i)->SetStopTime (stop_time);
    }

  NS_LOG_INFO ("Hay [" << bridge->GetNDevices () << "] dispositivos");
  //Cola del server


  Ptr<DropTailQueue<Packet>> cola_DP = bridge->GetDevice (0)
                                           ->GetObject<CsmaNetDevice> ()
                                           ->GetQueue ()
                                           ->GetObject<DropTailQueue<Packet>> ();
  QueueSizeValue tam_cola;
  cola_DP->SetAttribute ("MaxSize", QueueSizeValue (QueueSize (t_cola)));
  cola_DP->GetAttribute ("MaxSize", tam_cola);

  NS_LOG_INFO (
      "Tamaño de la cola del puerto del switch conectado al servidor: " << tam_cola.Get ());
  NS_LOG_INFO ("[Arranca la simulación] tiempo de simulación: " << t_sim.GetSeconds () << " s");

  NS_LOG_DEBUG ("Creando objeto_retardo...");
  //Retardo objeto_retardo = Retardo (c_dispositivos.Get (0), c_dispositivos.Get (1));

  Simulator::Stop (stop_time); //Falta un tiempo, si no la simulación no termina
  Simulator::Run ();
  NS_LOG_INFO ("--[Simulación completada]--");

  int paquetesPerdidos = cola_DP->GetTotalDroppedPackets ();
  NS_LOG_INFO ("Paquetes perdidos en la cola: " << paquetesPerdidos);


  for (uint32_t i = 0; i < nodos; i++)
    {
      Ptr<Queue<Packet>> cola_aux = bridge->GetDevice (i)
                                        ->GetObject<CsmaNetDevice> ()
                                        ->GetQueue ()
                                        ->GetObject<DropTailQueue<Packet>> ();
      
      NS_LOG_INFO ("Paquetes RECIBIDOS en la cola del puerto ["
                   << i << "]: " << cola_aux->GetTotalReceivedPackets ());
    }

      NS_LOG_INFO (
      "Llamando al método GetReceived() del nodo servidor UDP: " << udpserver->GetReceived ());


  Simulator::Destroy ();

  //return objeto_retardo.GetRetardoMedio ();
return 0;
}
