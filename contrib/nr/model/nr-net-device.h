/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

// Copyright (c) 2019 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
//
// SPDX-License-Identifier: GPL-2.0-only

#ifndef SRC_NR_MODEL_NR_NET_DEVICE_H_
#define SRC_NR_MODEL_NR_NET_DEVICE_H_

#include "nr-phy.h"

#include <ns3/net-device.h>

namespace ns3
{

class Node;
class Packet;

/**
 * \ingroup ue
 * \ingroup gnb
 * \brief The NrNetDevice class
 *
 * This is the base class for NrUeNetDevice and NrGnbNetDevice.
 */
class NrNetDevice : public NetDevice
{
  public:
    /**
     * \brief GetTypeId
     * \return the object type id
     */
    static TypeId GetTypeId();

    /**
     * \brief NrNetDevice
     */
    NrNetDevice();
    /**
     * \brief ~NrNetDevice
     */
    ~NrNetDevice() override;

    // inherit
    void SetIfIndex(const uint32_t index) override;
    uint32_t GetIfIndex() const override;
    Ptr<Channel> GetChannel() const override;
    void SetAddress(Address address) override;
    Address GetAddress() const override;
    bool SetMtu(const uint16_t mtu) override;
    uint16_t GetMtu() const override;
    bool IsLinkUp() const override;
    void AddLinkChangeCallback(Callback<void> callback) override;
    bool IsBroadcast() const override;
    Address GetBroadcast() const override;
    bool IsMulticast() const override;
    Address GetMulticast(Ipv4Address multicastGroup) const override;
    bool IsBridge() const override;
    bool IsPointToPoint() const override;
    bool SendFrom(Ptr<Packet> packet,
                  const Address& source,
                  const Address& dest,
                  uint16_t protocolNumber) override;
    Ptr<Node> GetNode() const override;
    void SetNode(Ptr<Node> node) override;
    bool NeedsArp() const override;
    Address GetMulticast(Ipv6Address addr) const override;
    void SetReceiveCallback(ReceiveCallback cb) override;
    void SetPromiscReceiveCallback(PromiscReceiveCallback cb) override;
    bool SupportsSendFrom() const override;
    bool Send(Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber) override;
    void Receive(Ptr<Packet> p);

  protected:
    void DoDispose() override;

    NetDevice::ReceiveCallback m_rxCallback;
    virtual bool DoSend(Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber) = 0;

  private:
    Mac48Address m_macaddress;
    Ptr<Node> m_node;
    mutable uint16_t m_mtu;
    bool m_linkUp;
    uint32_t m_ifIndex;
};

} // namespace ns3

#endif /* SRC_NR_MODEL_NR_NET_DEVICE_H_ */
