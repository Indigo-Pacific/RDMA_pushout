/*
 * Copyright 2007 University of Washington
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
 * Author:  Tom Henderson (tomhend@u.washington.edu)
 */

#ifndef PACKET_SINK_H
#define PACKET_SINK_H

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/ptr.h"
#include "ns3/seq-ts-size-header.h"
#include "ns3/traced-callback.h"

#include <unordered_map>

namespace ns3
{

class Address;
class Socket;
class Packet;

/**
 * \ingroup applications
 * \defgroup packetsink PacketSink
 *
 * This application was written to complement OnOffApplication, but it
 * is more general so a PacketSink name was selected.  Functionally it is
 * important to use in multicast situations, so that reception of the layer-2
 * multicast frames of interest are enabled, but it is also useful for
 * unicast as an example of how you can write something simple to receive
 * packets at the application layer.  Also, if an IP stack generates
 * ICMP Port Unreachable errors, receiving applications will be needed.
 */

/**
 * \ingroup packetsink
 *
 * \brief Receive and consume traffic generated to an IP address and port
 *
 * This application was written to complement OnOffApplication, but it
 * is more general so a PacketSink name was selected.  Functionally it is
 * important to use in multicast situations, so that reception of the layer-2
 * multicast frames of interest are enabled, but it is also useful for
 * unicast as an example of how you can write something simple to receive
 * packets at the application layer.  Also, if an IP stack generates
 * ICMP Port Unreachable errors, receiving applications will be needed.
 *
 * The constructor specifies the Address (IP address and port) and the
 * transport protocol to use.   A virtual Receive () method is installed
 * as a callback on the receiving socket.  By default, when logging is
 * enabled, it prints out the size of packets and their address.
 * A tracing source to Receive() is also available.
 */
class PacketSink : public Application
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();
    PacketSink();

    ~PacketSink() override;

    /**
     * \return the total bytes received in this sink app
     */
    uint64_t GetTotalRx() const;

    /**
     * \return pointer to listening socket
     */
    Ptr<Socket> GetListeningSocket() const;

    /**
     * \return list of pointers to accepted sockets
     */
    std::list<Ptr<Socket>> GetAcceptedSockets() const;

    /**
     * TracedCallback signature for a reception with addresses and SeqTsSizeHeader
     *
     * \param p The packet received (without the SeqTsSize header)
     * \param from From address
     * \param to Local address
     * \param header The SeqTsSize header
     */
    typedef void (*SeqTsSizeCallback)(Ptr<const Packet> p,
                                      const Address& from,
                                      const Address& to,
                                      const SeqTsSizeHeader& header);
    
    /*New Modification*/
    void SetFlowEndCallback(Callback<void, bool, uint32_t> flowEnd);
    void SetAtoaFlowEndCallback(Callback<void, uint32_t> atoaFlowEnd);
    /*New Modification*/

  protected:
    void DoDispose() override;

  private:
    // inherited from Application base class.
    void StartApplication() override; // Called at time specified by Start
    void StopApplication() override;  // Called at time specified by Stop

    /**
     * \brief Handle a packet received by the application
     * \param socket the receiving socket
     */
    void HandleRead(Ptr<Socket> socket);
    /**
     * \brief Handle an incoming connection
     * \param socket the incoming connection socket
     * \param from the address the connection is from
     */
    void HandleAccept(Ptr<Socket> socket, const Address& from);
    /**
     * \brief Handle an connection close
     * \param socket the connected socket
     */
    void HandlePeerClose(Ptr<Socket> socket);
    /**
     * \brief Handle an connection error
     * \param socket the connected socket
     */
    void HandlePeerError(Ptr<Socket> socket);

    /**
     * \brief Packet received: assemble byte stream to extract SeqTsSizeHeader
     * \param p received packet
     * \param from from address
     * \param localAddress local address
     *
     * The method assembles a received byte stream and extracts SeqTsSizeHeader
     * instances from the stream to export in a trace source.
     */
    void PacketReceived(const Ptr<Packet>& p, const Address& from, const Address& localAddress);

    /**
     * \brief Hashing for the Address class
     */
    struct AddressHash
    {
        /**
         * \brief operator ()
         * \param x the address of which calculate the hash
         * \return the hash of x
         *
         * Should this method go in address.h?
         *
         * It calculates the hash taking the uint32_t hash value of the IPv4 or IPv6 address.
         * It works only for InetSocketAddresses (IPv4 version) or Inet6SocketAddresses (IPv6
         * version)
         */
        size_t operator()(const Address& x) const
        {
            if (InetSocketAddress::IsMatchingType(x))
            {
                InetSocketAddress a = InetSocketAddress::ConvertFrom(x);
                return Ipv4AddressHash()(a.GetIpv4());
            }
            else if (Inet6SocketAddress::IsMatchingType(x))
            {
                Inet6SocketAddress a = Inet6SocketAddress::ConvertFrom(x);
                return Ipv6AddressHash()(a.GetIpv6());
            }

            NS_ABORT_MSG("PacketSink: unexpected address type, neither IPv4 nor IPv6");
            return 0; // silence the warnings.
        }
    };

    std::unordered_map<Address, Ptr<Packet>, AddressHash> m_buffer; //!< Buffer for received packets

    // In the case of TCP, each socket accept returns a new socket, so the
    // listening socket is stored separately from the accepted sockets
    Ptr<Socket> m_socket;                //!< Listening socket
    std::list<Ptr<Socket>> m_socketList; //!< the accepted sockets

    Address m_local;      //!< Local address to bind to (address and port)
    uint16_t m_localPort; //!< Local port to bind to
    uint64_t m_totalRx;   //!< Total bytes received
    TypeId m_tid;         //!< Protocol TypeId

    bool m_enableSeqTsSizeHeader{false}; //!< Enable or disable the export of SeqTsSize header

    /* Modification */
    uint64_t        TotalQueryBytes;
    Time            m_recvAt;
    uint32_t        priority;
    uint32_t        flowId;
    uint32_t        m_priorCustom;
    uint32_t        sender_priority;
    /* Modification */

    /*New Modification*/
    bool m_isReduce;
    uint32_t m_allreduceId;
    uint32_t m_atoaNodeId;
    /*New Modification*/

    /// Traced Callback: received packets, source address.
    TracedCallback<Ptr<const Packet>, const Address&> m_rxTrace;
    /// Callback for tracing the packet Rx events, includes source and destination addresses
    TracedCallback<Ptr<const Packet>, const Address&, const Address&> m_rxTraceWithAddresses;
    /// Callbacks for tracing the packet Rx events, includes source, destination addresses, and
    /// headers
    TracedCallback<Ptr<const Packet>, const Address&, const Address&, const SeqTsSizeHeader&>
        m_rxTraceWithSeqTsSize;

    /* Modification */
    TracedCallback<double, double,bool,uint32_t> m_flowFinishTrace;
    /* Modification */

    /*New Modification*/
    Callback<void, bool, uint32_t> m_flowEnd;
    Callback<void, uint32_t> m_atoaFlowEnd;
    /*New Modification*/

};

} // namespace ns3

#endif /* PACKET_SINK_H */
