/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

// Copyright (c) 2019 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "nr-mac-scheduler-ns3.h"
#include <glpk.h>

namespace ns3
{

/**
 * \ingroup scheduler
 * \brief UE representation for a round-robin scheduler
 *
 * The UE representation does not store any additional information,
 * but provides a way for a RR scheduler to order the UE based on the assigned
 * RBG.
 *
 * \see CompareUeWeightsDl
 */
class NrMacSchedulerUeInfoDPP : public NrMacSchedulerUeInfo
{
  public:
    /**
     * \brief NrMacSchedulerUeInfoDPP constructor
     * \param rnti RNTI of the UE
     * \param beamConfId BeamConfId of the UE
     * \param fn A function that tells how many RB per RBG
     */
    NrMacSchedulerUeInfoDPP(uint16_t rnti,
                            BeamConfId beamConfId,
                            const GetRbPerRbgFn& fn)
        : NrMacSchedulerUeInfo(rnti, beamConfId, fn)
    {
    }

    static void SetDppV (double dppV) {
      m_v_lyapunov = dppV;
    }

    /**
     * \brief Reset DL scheduler info
     *
     * Zeroes the average throughput as well as the current throughput.
     *
     * It calls also NrMacSchedulerUeInfoQos::ResetDlSchedInfo.
     */
    void ResetDlSchedInfo() override
    {
        m_currTputDl = 0.0;
        NrMacSchedulerUeInfo::ResetDlSchedInfo();
    }
    
    /**
     * \brief Update metrics for downlink
     * \param totAssigned the resources assigned
     * \param timeWindow the time window
     * \param amc a pointer to the AMC
     *
     * To be defined
     * 
     */
    void UpdateDlTputVirtualQueue(const NrMacSchedulerNs3::FTResources& totAssigned,
                        double timeSlot,
                        const Ptr<const NrAmc>& amc);

    static void LyapunovDPP(const std::vector<ns3::NrMacSchedulerNs3::UePtrAndBufferReq> ueVector, double resources, const Ptr<const NrAmc>& amc);

    /**
     * \brief comparison function object (i.e. an object that satisfies the
     * requirements of Compare) which returns ​true if the first argument is less
     * than (i.e. is ordered before) the second.
     * \param lue Left UE
     * \param rue Right UE
     * \return true if the assigned RBG of lue is less than the assigned RBG of rue
     *
     * The ordering is made by considering the RBG. An UE with 0 RBG will always
     * be the first (i.e., has an higher priority) in a RR scheduler. The objective
     * is to distribute evenly all the resources, in order to have the same RBG
     * number for all the UEs.
     */
    static bool CompareUeWeightsDl(const NrMacSchedulerNs3::UePtrAndBufferReq& lue,
                                   const NrMacSchedulerNs3::UePtrAndBufferReq& rue);

    /**
     * \brief comparison function object (i.e. an object that satisfies the
     * requirements of Compare) which returns ​true if the first argument is less
     * than (i.e. is ordered before) the second.
     * \param lue Left UE
     * \param rue Right UE
     * \return true if the assigned RBG of lue is less than the assigned RBG of rue
     *
     * The ordering is made by considering the RBG. An UE with 0 RBG will always
     * be the first (i.e., has an higher priority) in a RR scheduler. The objective
     * is to distribute evenly all the resources, in order to have the same RBG
     * number for all the UEs.
     */
    static bool CompareUeWeightsUl(const NrMacSchedulerNs3::UePtrAndBufferReq& lue,
                                   const NrMacSchedulerNs3::UePtrAndBufferReq& rue)
    {
        return (lue.first->m_ulRBG < rue.first->m_ulRBG);
    }

    /**
     * \brief Function to save simulation results to a file
    */
    static void saveResultsToFile(const std::vector<uint32_t>& simulationResults, std::ofstream& file);
    static void saveQueuesState(const std::vector<ns3::NrMacSchedulerNs3::UePtrAndBufferReq> ueVector);
    static void saveRBGallocation(const std::vector<ns3::NrMacSchedulerNs3::UePtrAndBufferReq> ueVector);

    double m_currTputDl{0.0};      //!< Current slot throughput in downlink
                                   //!< (can be a symbol or a RBG)
    double m_g{0.0}; //!<  Virtual queues
    uint32_t m_dlRBGallocated{0}; //!< RBGs assigned by the scheduler
    static double m_v_lyapunov; //!<  V config parameter for Lyaponuv drif-plus-penalty
};
} // namespace ns3
