/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 ResiliNets, ITTC, University of Kansas 
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
 * Authors: Siddharth Gangadhar <siddharth@ittc.ku.edu>,
 *          Truc Anh N. Nguyen <annguyen@ittc.ku.edu>,
 *          Greeshma Umapathi
 *
 * James P.G. Sterbenz <jpgs@ittc.ku.edu>, director
 * ResiliNets Research Group  http://wiki.ittc.ku.edu/resilinets
 * Information and Telecommunication Technology Center (ITTC)
 * and Department of Electrical Engineering and Computer Science
 * The University of Kansas Lawrence, KS USA.
 *
 * Work supported in part by NSF FIND (Future Internet Design) Program
 * under grant CNS-0626918 (Postmodern Internet Architecture),
 * NSF grant CNS-1050226 (Multilayer Network Resilience Analysis and Experimentation on GENI),
 * US Department of Defense (DoD), and ITTC at The University of Kansas.
 */

#include "tcp-westwood.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "rtt-estimator.h"
#include "tcp-socket-base.h"

NS_LOG_COMPONENT_DEFINE ("TcpJersey");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TcpJersey);

TypeId
TcpJersey::GetTypeId (void)
{
  static TypeId tid = TypeId("ns3::TcpJersey")
    .SetParent<TcpNewReno>()
    .SetGroupName ("Internet")
    .AddConstructor<TcpJersey>()
    .AddTraceSource("ABE", "Available Bandwidth Estimated",
                    MakeTraceSourceAccessor(&TcpJersey::m_currentBW),
                    "ns3::TracedValueCallback::Double")
  ;
  return tid;
}

TcpJersey::TcpJersey (void) :
  TcpNewReno (),
  m_currentBW (0),
  m_lastSampleBW (0),
  m_lastBW (0),
  m_ackedSegments (0),
  m_IsCount (false)
{
  NS_LOG_FUNCTION (this);
}

TcpJersey::TcpJersey (const TcpJersey& sock) :
  TcpNewReno (sock),
  m_currentBW (sock.m_currentBW),
  m_lastSampleBW (sock.m_lastSampleBW),
  m_lastBW (sock.m_lastBW),
  m_pType (sock.m_pType),
  m_fType (sock.m_fType),
  m_IsCount (sock.m_IsCount)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC ("Invoked the copy constructor");
}

TcpJersey::~TcpJersey (void)
{
}

void
TcpJersey::PktsAcked (Ptr<TcpSocketState> tcb, uint32_t packetsAcked,
                        const Time& rtt)
{
  NS_LOG_FUNCTION (this << tcb << packetsAcked << rtt);

  if (rtt.IsZero ())
    {
      NS_LOG_WARN ("RTT measured is zero!");
      return;
    }

  m_ackedSegments += packetsAcked;
  ABE (tcb,rtt);
 }


void
TcpJersey::ABE (const Time &rtt, Ptr<TcpSocketState> tcb)
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT (!rtt.IsZero ());

  Time now = Simulator::Now ();

  Time Tw =  rtt * m_K;
  double delta = now.GetSeconds () - m_prevAckTime.GetSeconds ();
  m_prevAckTime = now;

  if ((now - m_tLast) >= rtt)
    {
      double temp = (Tw.GetSeconds () * m_currentBW) + (m_ackedSegments * tcb->m_segmentSize);
      m_currentBW = temp / (delta + Tw.GetSeconds ());
      m_tLast = now;
      m_ackedSegments = 0;
    }

}

uint32_t
TcpJersey::GetSsThresh (Ptr<const TcpSocketState> tcb,
                          uint32_t bytesInFlight)
{
  NS_UNUSED (bytesInFlight);
  NS_LOG_LOGIC ("CurrentBW: " << m_currentBW << " minRtt: " <<
                tcb->m_minRtt << " ssthresh: " <<
                m_currentBW * static_cast<double> (tcb->m_minRtt.GetSeconds ()));
  
  uint32_t ownd = m_currentRTT.GetSeconds () * m_currentBW / tcb->m_segmentSize;

  return std::max (2*tcb->m_segmentSize , ownd);
}

 Ptr<TcpCongestionOps>
 TcpJersey::Fork ()
 {
   return CreateObject<TcpJersey> (*this);
 }

} // namespace ns3
