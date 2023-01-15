#include "ns3/ptr.h"
#include "ns3/udp-server.h"
#include "ns3/nstime.h"
#include "ns3/average.h"

using namespace ns3;

class Retardo
{
public:
           Retardo      (Ptr<NetDevice> emisor,Ptr<NetDevice> receptor);
  void     PaqueteRecibido (Ptr<const Packet> paquete);
  void     PaqueteEnviado (Ptr<const Packet> paquete);
  double   GetRetardoMedio(void);
private:
  Time instante;
  Average<double> retardo;
};
