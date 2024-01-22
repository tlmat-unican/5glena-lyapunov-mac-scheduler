/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

// Copyright (c) 2019 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
//
// SPDX-License-Identifier: GPL-2.0-only

#include "nr-net-device.h"

#include <ns3/channel.h>
#include <ns3/ipv4-l3-protocol.h>
#include <ns3/ipv6-header.h>
#include <ns3/ipv6-l3-protocol.h>
#include <ns3/log.h>
#include <ns3/node.h>
#include <ns3/uinteger.h>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("NrNetDevice");

NS_OBJECT_ENSURE_REGISTERED(NrNetDevice);

TypeId
NrNetDevice::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::NrNetDevice")
            .SetParent<NetDevice>()
            .AddAttribute("Mtu",
                          "The MAC-level Maximum Transmission Unit",
                          UintegerValue(30000),
                          MakeUintegerAccessor(&NrNetDevice::SetMtu, &NrNetDevice::GetMtu),
                          MakeUintegerChecker<uint16_t>());

    return tid;
}

NrNetDevice::NrNetDevice()
{
    NS_LOG_FUNCTION(this);
}

NrNetDevice::~NrNetDevice()
{
    NS_LOG_FUNCTION(this);
}

void
NrNetDevice::DoDispose()
{
    m_node = nullptr;
    NetDevice::DoDispose();
}

void
NrNetDevice::SetIfIndex(const uint32_t index)
{
    m_ifIndex = index;
}

uint32_t
NrNetDevice::GetIfIndex() const
{
    return m_ifIndex;
}

Ptr<Channel>
NrNetDevice::GetChannel() const
{
    return nullptr;
}

void
NrNetDevice::SetAddress(Address address)
{
    NS_LOG_FUNCTION(this << address);
    m_macaddress = Mac48Address::ConvertFrom(address);
}

Address
NrNetDevice::GetAddress() const
{
    NS_LOG_FUNCTION(this);
    return m_macaddress;
}

bool
NrNetDevice::SetMtu(const uint16_t mtu)
{
    m_mtu = mtu;
    return true;
}

uint16_t
NrNetDevice::GetMtu() const
{
    return m_mtu;
}

bool
NrNetDevice::IsLinkUp() const
{
    return m_linkUp;
}

void
NrNetDevice::AddLinkChangeCallback([[maybe_unused]] Callback<void> callback)
{
}

bool
NrNetDevice::IsBroadcast() const
{
    return false;
}

Address
NrNetDevice::GetBroadcast() const
{
    return Mac48Address::GetBroadcast();
}

bool
NrNetDevice::IsMulticast() const
{
    return false;
}

Address
NrNetDevice::GetMulticast([[maybe_unused]] Ipv4Address multicastGroup) const
{
    return Mac48Address("01:00:5e:00:00:00");
}

bool
NrNetDevice::IsBridge() const
{
    return false;
}

bool
NrNetDevice::IsPointToPoint() const
{
    return false;
}

bool
NrNetDevice::SendFrom([[maybe_unused]] Ptr<Packet> packet,
                      [[maybe_unused]] const Address& source,
                      [[maybe_unused]] const Address& dest,
                      [[maybe_unused]] uint16_t protocolNumber)
{
    NS_FATAL_ERROR("Send from not supported");
    return false;
}

Ptr<Node>
NrNetDevice::GetNode() const
{
    return m_node;
}

void
NrNetDevice::SetNode(Ptr<Node> node)
{
    m_node = node;
}

bool
NrNetDevice::NeedsArp() const
{
    return false;
}

Address
NrNetDevice::GetMulticast([[maybe_unused]] Ipv6Address addr) const
{
    return Address();
}

void
NrNetDevice::SetReceiveCallback(ReceiveCallback cb)
{
    NS_LOG_FUNCTION(this);
    m_rxCallback = cb;
}

void
NrNetDevice::SetPromiscReceiveCallback([[maybe_unused]] PromiscReceiveCallback cb)
{
}

bool
NrNetDevice::SupportsSendFrom() const
{
    return false;
}

void
NrNetDevice::Receive(Ptr<Packet> p)
{
    NS_LOG_FUNCTION(this << p);

    Ipv4Header ipv4Header;
    Ipv6Header ipv6Header;

    if (p->PeekHeader(ipv4Header) != 0)
    {
        NS_LOG_INFO("IPv4 stack...");
        m_rxCallback(this, p, Ipv4L3Protocol::PROT_NUMBER, Address());
    }
    else if (p->PeekHeader(ipv6Header) != 0)
    {
        NS_LOG_INFO("IPv6 stack...");
        m_rxCallback(this, p, Ipv6L3Protocol::PROT_NUMBER, Address());
    }
    else
    {
        NS_ABORT_MSG("NrNetDevice::Receive - Unknown IP type...");
    }
}

bool
NrNetDevice::Send(Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber)
{
    bool ret = DoSend(packet, dest, protocolNumber);
    return ret;
}

} // namespace ns3
