#include "ns3/object-base.h"
#include "ns3/log.h"
#include "retardo.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Retardo");

Retardo::Retardo(Ptr<NetDevice> emisor,Ptr<NetDevice> receptor)
{
  receptor->TraceConnectWithoutContext ("PhyTxBegin", MakeCallback(&Retardo::PaqueteRecibido,this));  
  emisor->TraceConnectWithoutContext("PhyRxEnd", MakeCallback(&Retardo::PaqueteEnviado, this));
}


void
Retardo::PaqueteRecibido (Ptr<const Packet> paquete)
{
  NS_LOG_FUNCTION (paquete);
  Time intervalo = Simulator::Now() - instante;
  retardo.Update(intervalo.GetMilliSeconds());
}

void
Retardo::PaqueteEnviado (Ptr<const Packet> paquete)
{
  NS_LOG_FUNCTION (paquete);
  instante=Simulator::Now();
}

double Retardo::GetRetardoMedio(void){
  return retardo.Avg();
}

