/*
 * Copyright (c) 2010 Georgia Institute of Technology
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
 * Author: George F. Riley <riley@ece.gatech.edu>
 */

#include "bulk-send-application.h"

#include "ns3/address.h"
#include "ns3/boolean.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/socket.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/uinteger.h"

/* Modification */
#include "ns3/flow-id-tag.h"
#include "ns3/custom-priority-tag.h"
/* Modification */

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("BulkSendApplication");

NS_OBJECT_ENSURE_REGISTERED(BulkSendApplication);

TypeId
BulkSendApplication::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::BulkSendApplication")
            .SetParent<Application>()
            .SetGroupName("Applications")
            .AddConstructor<BulkSendApplication>()
            .AddAttribute("SendSize",
                          "The amount of data to send each time.",
                          UintegerValue(512),
                          MakeUintegerAccessor(&BulkSendApplication::m_sendSize),
                          MakeUintegerChecker<uint64_t>(1))
            .AddAttribute("Remote",
                          "The address of the destination",
                          AddressValue(),
                          MakeAddressAccessor(&BulkSendApplication::m_peer),
                          MakeAddressChecker())
            .AddAttribute("Local",
                          "The Address on which to bind the socket. If not set, it is generated "
                          "automatically.",
                          AddressValue(),
                          MakeAddressAccessor(&BulkSendApplication::m_local),
                          MakeAddressChecker())
            .AddAttribute("MaxBytes",
                          "The total number of bytes to send. "
                          "Once these bytes are sent, "
                          "no data  is sent again. The value zero means "
                          "that there is no limit.",
                          UintegerValue(0),
                          MakeUintegerAccessor(&BulkSendApplication::m_maxBytes),
                          MakeUintegerChecker<uint64_t>())
            .AddAttribute("Protocol",
                          "The type of protocol to use.",
                          TypeIdValue(TcpSocketFactory::GetTypeId()),
                          MakeTypeIdAccessor(&BulkSendApplication::m_tid),
                          MakeTypeIdChecker())
            .AddAttribute("EnableSeqTsSizeHeader",
                          "Add SeqTsSizeHeader to each packet",
                          BooleanValue(false),
                          MakeBooleanAccessor(&BulkSendApplication::m_enableSeqTsSizeHeader),
                          MakeBooleanChecker())
            /* Modification */
            .AddAttribute ("FlowId", "Flow id Once these bytes are sent", UintegerValue (5),
                                MakeUintegerAccessor (&BulkSendApplication::m_flowId),
                                MakeUintegerChecker<uint64_t> ())
            .AddAttribute ("InitialCwnd", "InitialCwnd", UintegerValue (10),
                        MakeUintegerAccessor (&BulkSendApplication::InitialCwnd),
                        MakeUintegerChecker<uint32_t> ())

            .AddAttribute ("sendAt", "sendAtTime", TimeValue (Seconds (0.0)),
                        MakeTimeAccessor (&BulkSendApplication::m_sendAt),
                        MakeTimeChecker ())
            .AddAttribute ("priorityCustom","priority of sent packets",UintegerValue (1),
                        MakeUintegerAccessor (&BulkSendApplication::m_priorCustom),
                        MakeUintegerChecker<uint8_t> ())
            .AddAttribute ("priorityClassifier","priority of sent packets in the buffer",UintegerValue (1),
                        MakeUintegerAccessor (&BulkSendApplication::m_priorClassifier),
                        MakeUintegerChecker<uint8_t> ())
            .AddAttribute  ("priority","priority of all sockets created by this sink", UintegerValue(0),
                            MakeUintegerAccessor(&BulkSendApplication::priority), MakeUintegerChecker<uint32_t>())
            /* Modification */
            .AddTraceSource("Tx",
                            "A new packet is sent",
                            MakeTraceSourceAccessor(&BulkSendApplication::m_txTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("TxWithSeqTsSize",
                            "A new packet is created with SeqTsSizeHeader",
                            MakeTraceSourceAccessor(&BulkSendApplication::m_txTraceWithSeqTsSize),
                            "ns3::PacketSink::SeqTsSizeCallback");
    return tid;
}

BulkSendApplication::BulkSendApplication()
    : m_socket(nullptr),
      m_connected(false),
      m_totBytes(0),
      m_unsentPacket(nullptr)
{
    NS_LOG_FUNCTION(this);
    /*Modification*/
    delay=false;
    /*Modification*/
}

BulkSendApplication::~BulkSendApplication()
{
    NS_LOG_FUNCTION(this);
}

void
BulkSendApplication::SetMaxBytes(uint64_t maxBytes)
{
    NS_LOG_FUNCTION(this << maxBytes);
    m_maxBytes = maxBytes;
}

Ptr<Socket>
BulkSendApplication::GetSocket() const
{
    NS_LOG_FUNCTION(this);
    return m_socket;
}

void
BulkSendApplication::DoDispose()
{
    NS_LOG_FUNCTION(this);

    m_socket = nullptr;
    m_unsentPacket = nullptr;
    // chain up
    Application::DoDispose();
}

// Application Methods
void
BulkSendApplication::StartApplication() // Called at time specified by Start
{
    NS_LOG_FUNCTION(this);
    Address from;

    // Create the socket if not already
    if (!m_socket)
    {
        m_socket = Socket::CreateSocket(GetNode(), m_tid);
        int ret = -1;

        /* Modification */
        m_socket->SetAttribute("InitialCwnd",UintegerValue(InitialCwnd));
        m_socket->SetAttribute("flowId",UintegerValue(m_flowId));
        m_socket->SetAttribute("mypriority",UintegerValue(m_priorCustom));
        m_socket->SetPriority(priority);
        /* Modification */

        // Fatal error if socket type is not NS3_SOCK_STREAM or NS3_SOCK_SEQPACKET
        if (m_socket->GetSocketType() != Socket::NS3_SOCK_STREAM &&
            m_socket->GetSocketType() != Socket::NS3_SOCK_SEQPACKET)
        {
            NS_FATAL_ERROR("Using BulkSend with an incompatible socket type. "
                           "BulkSend requires SOCK_STREAM or SOCK_SEQPACKET. "
                           "In other words, use TCP instead of UDP.");
        }

        if (!m_local.IsInvalid())
        {
            NS_ABORT_MSG_IF((Inet6SocketAddress::IsMatchingType(m_peer) &&
                             InetSocketAddress::IsMatchingType(m_local)) ||
                                (InetSocketAddress::IsMatchingType(m_peer) &&
                                 Inet6SocketAddress::IsMatchingType(m_local)),
                            "Incompatible peer and local address IP version");
            ret = m_socket->Bind(m_local);
        }
        else
        {
            if (Inet6SocketAddress::IsMatchingType(m_peer))
            {
                ret = m_socket->Bind6();
            }
            else if (InetSocketAddress::IsMatchingType(m_peer))
            {
                ret = m_socket->Bind();
            }
        }

        if (ret == -1)
        {
            NS_FATAL_ERROR("Failed to bind socket");
        }

        m_socket->Connect(m_peer);
        /* Modification */
        m_socket->SetPriority(priority);
        /* Modification */
        m_socket->ShutdownRecv();
        m_socket->SetConnectCallback(MakeCallback(&BulkSendApplication::ConnectionSucceeded, this),
                                     MakeCallback(&BulkSendApplication::ConnectionFailed, this));
        m_socket->SetSendCallback(MakeCallback(&BulkSendApplication::DataSend, this));
    }
    if (m_connected)
    {
        m_socket->GetSockName(from);
        SendData(from, m_peer);
    }
}

/* Modification */
void BulkSendApplication::SendQuery(const Address &from, const Address &to){
  delay=false;
  if(m_connected)
  SendData(from, to);
}
/* Modification */

void
BulkSendApplication::StopApplication() // Called at time specified by Stop
{
    NS_LOG_FUNCTION(this);

    if (m_socket)
    {
        m_socket->Close();
        m_connected = false;
    }
    else
    {
        NS_LOG_WARN("BulkSendApplication found null socket to close in StopApplication");
    }
}

// Private helpers

void
BulkSendApplication::SendData(const Address& from, const Address& to)
{
    NS_LOG_FUNCTION(this);

    while (m_maxBytes == 0 || m_totBytes < m_maxBytes)
    { // Time to send more

        /* Modification */
        // start sending at the specified time, not now.
        if(m_sendAt>Simulator::Now()){
            delay=true;
            Simulator::Schedule(m_sendAt-Simulator::Now(), &BulkSendApplication::SendQuery,this,from,to);
        }
        if(delay){
            break;
        }
        /* Modification */

        // uint64_t to allow the comparison later.
        // the result is in a uint32_t range anyway, because
        // m_sendSize is uint32_t.
        uint64_t toSend = m_sendSize;
        // Make sure we don't send too many
        if (m_maxBytes > 0)
        {
            toSend = std::min(toSend, m_maxBytes - m_totBytes);
        }

        NS_LOG_LOGIC("sending packet at " << Simulator::Now());

        Ptr<Packet> packet;
        if (m_unsentPacket)
        {
            packet = m_unsentPacket;
            toSend = packet->GetSize();
        }
        else if (m_enableSeqTsSizeHeader)
        {
            SeqTsSizeHeader header;
            header.SetSeq(m_seq++);
            header.SetSize(toSend);
            NS_ABORT_IF(toSend < header.GetSerializedSize());
            packet = Create<Packet>(toSend - header.GetSerializedSize());
            // Trace before adding header, for consistency with PacketSink
            m_txTraceWithSeqTsSize(packet, from, to, header);
            packet->AddHeader(header);
        }
        else
        {
            packet = Create<Packet>(toSend);
        }

        /* Modification */

        // Flow ID Tag
        FlowIdTag flowid(m_flowId);
        packet->AddPacketTag(flowid);

        // Custom priority tag
        MyPriorityTag a;
        a.SetPriority(m_priorCustom);
        packet->AddPacketTag(a);

        /* Modification */

        int actual = m_socket->Send(packet);
        if ((unsigned)actual == toSend)
        {
            m_totBytes += actual;
            m_txTrace(packet);
            m_unsentPacket = nullptr;
        }
        else if (actual == -1)
        {
            // We exit this loop when actual < toSend as the send side
            // buffer is full. The "DataSent" callback will pop when
            // some buffer space has freed up.
            NS_LOG_DEBUG("Unable to send packet; caching for later attempt");
            m_unsentPacket = packet;
            break;
        }
        else if (actual > 0 && (unsigned)actual < toSend)
        {
            // A Linux socket (non-blocking, such as in DCE) may return
            // a quantity less than the packet size.  Split the packet
            // into two, trace the sent packet, save the unsent packet
            NS_LOG_DEBUG("Packet size: " << packet->GetSize() << "; sent: " << actual
                                         << "; fragment saved: " << toSend - (unsigned)actual);
            Ptr<Packet> sent = packet->CreateFragment(0, actual);
            Ptr<Packet> unsent = packet->CreateFragment(actual, (toSend - (unsigned)actual));
            m_totBytes += actual;
            m_txTrace(sent);
            m_unsentPacket = unsent;
            break;
        }
        else
        {
            NS_FATAL_ERROR("Unexpected return value from m_socket->Send ()");
        }
    }
    // Check if time to close (all sent)
    if (m_totBytes == m_maxBytes && m_connected)
    {
        m_socket->Close();
        m_connected = false;
    }
}

void
BulkSendApplication::ConnectionSucceeded(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    NS_LOG_LOGIC("BulkSendApplication Connection succeeded");
    m_connected = true;
    Address from;
    Address to;
    socket->GetSockName(from);
    socket->GetPeerName(to);
    SendData(from, to);
}

void
BulkSendApplication::ConnectionFailed(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    NS_LOG_LOGIC("BulkSendApplication, Connection Failed");
}

void
BulkSendApplication::DataSend(Ptr<Socket> socket, uint32_t)
{
    NS_LOG_FUNCTION(this);

    if (m_connected)
    { // Only send new data if the connection has completed
        Address from;
        Address to;
        socket->GetSockName(from);
        socket->GetPeerName(to);
        SendData(from, to);
    }
}

} // Namespace ns3
