/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

// Copyright (c) 2017 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
//
// SPDX-License-Identifier: GPL-2.0-only

#ifndef COMPONENT_CARRIER_NR_UE_H
#define COMPONENT_CARRIER_NR_UE_H

#include "ns3/nr-phy.h"
#include <ns3/component-carrier.h>
#include <ns3/nr-ue-phy.h>
#include <ns3/nstime.h>
#include <ns3/object.h>
#include <ns3/packet.h>

namespace ns3
{

class NrUeMac;

/**
 * \ingroup ue-bwp
 * \brief Bandwidth part representation for a UE
 *
 */
class BandwidthPartUe : public ComponentCarrier
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * \brief BandwidthPartUe constructor
     */
    BandwidthPartUe();

    /**
     * \brief ~BandwidthPartUe
     */
    ~BandwidthPartUe() override;

    /**
     * \return a pointer to the physical layer.
     */
    Ptr<NrUePhy> GetPhy() const;

    /**
     * \return a pointer to the MAC layer.
     */
    Ptr<NrUeMac> GetMac() const;

    /**
     * Set NrUePhy
     * \param s a pointer to the NrUePhy
     */
    void SetPhy(Ptr<NrUePhy> s);

    /**
     * Set the NrGnbMac
     * \param s a pointer to the NrGnbMac
     */
    void SetMac(Ptr<NrUeMac> s);

    void SetDlBandwidth(uint16_t bw) override
    {
        m_dlBandwidth = bw;
    }

    void SetUlBandwidth(uint16_t bw) override
    {
        m_ulBandwidth = bw;
    }

  protected:
    /**
     * \brief DoDispose method inherited from Object
     */
    void DoDispose() override;

  private:
    Ptr<NrUePhy> m_phy; ///< the Phy instance of this eNodeB component carrier
    Ptr<NrUeMac> m_mac; ///< the MAC instance of this eNodeB component carrier
};

} // namespace ns3

#endif /* COMPONENT_CARRIER_UE_H */
