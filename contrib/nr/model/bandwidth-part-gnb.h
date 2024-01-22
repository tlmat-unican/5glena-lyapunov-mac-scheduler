/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

// Copyright (c) 2017 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
//
// SPDX-License-Identifier: GPL-2.0-only

#ifndef COMPONENT_CARRIER_GNB_H
#define COMPONENT_CARRIER_GNB_H

#include "ns3/nr-phy.h"
#include <ns3/component-carrier.h>
#include <ns3/nr-gnb-phy.h>
#include <ns3/nstime.h>
#include <ns3/object.h>
#include <ns3/packet.h>
#include <ns3/pointer.h>

namespace ns3
{

class NrGnbMac;
class NrMacScheduler;

/**
 * \ingroup gnb-bwp
 * \brief GNB bandwidth part representation
 *
 * Defines a single bandwidth part for the GNB.
 */
class BandwidthPartGnb : public ComponentCarrierBaseStation
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    BandwidthPartGnb();

    ~BandwidthPartGnb() override;

    /**
     * \return a pointer to the physical layer.
     */
    Ptr<NrGnbPhy> GetPhy();

    /**
     * \return a pointer to the MAC layer.
     */
    Ptr<NrGnbMac> GetMac();

    /**
     * \return a pointer to the Mac Scheduler.
     */
    Ptr<NrMacScheduler> GetScheduler();

    /**
     * Set the NrGnbPhy
     * \param s a pointer to the NrGnbPhy
     */
    void SetPhy(Ptr<NrGnbPhy> s);
    /**
     * Set the NrGnbMac
     * \param s a pointer to the NrGnbMac
     */
    void SetMac(Ptr<NrGnbMac> s);

    /**
     * Set the FfMacScheduler Algorithm
     * \param s a pointer to the FfMacScheduler
     */
    void SetNrMacScheduler(Ptr<NrMacScheduler> s);

    void SetDlBandwidth(uint16_t bw) override
    {
        m_dlBandwidth = bw;
    }

    void SetUlBandwidth(uint16_t bw) override
    {
        m_ulBandwidth = bw;
    }

    /**
     * \brief Set this bandwidth part as primary.
     * \param primaryCarrier true or false.
     *
     * Unfortunately, for the "false" value, the method will do nothing. Every carrier
     * starts as "not primary", so please, if you have to use SetAsPrimary (false)
     * think two times.
     */
    void SetAsPrimary(bool primaryCarrier);

  protected:
    /**
     * \brief DoDispose method inherited from Object
     */
    void DoDispose() override;

  private:
    Ptr<NrGnbPhy> m_phy;             ///< the Phy instance of this eNodeB component carrier
    Ptr<NrGnbMac> m_mac;             ///< the MAC instance of this eNodeB component carrier
    Ptr<NrMacScheduler> m_scheduler; ///< the scheduler instance of this eNodeB component carrier
};

} // namespace ns3

#endif /* COMPONENT_CARRIER_H */
