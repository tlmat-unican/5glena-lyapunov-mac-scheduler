/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

// Copyright (c) 2019 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
//
// SPDX-License-Identifier: GPL-2.0-only

#include "nr-mac-scheduler-ofdma-dpp.h"

#include "nr-mac-scheduler-ue-info-dpp.h"

#include <ns3/log.h>

#include <fstream>

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("NrMacSchedulerOfdmaDPP");
NS_OBJECT_ENSURE_REGISTERED(NrMacSchedulerOfdmaDPP);

std::ofstream outputFileTBS("tbs.txt");

TypeId
NrMacSchedulerOfdmaDPP::GetTypeId()
{
    static TypeId tid = 
        TypeId("ns3::NrMacSchedulerOfdmaDPP")
            .SetParent<NrMacSchedulerOfdma>()
            .AddConstructor<NrMacSchedulerOfdmaDPP>()
            .AddAttribute(
                "DppV",
                "Value of the weight value to consider the objective function",
                DoubleValue(99),
                MakeDoubleAccessor(&NrMacSchedulerOfdmaDPP::m_v_lyapunov),
                MakeDoubleChecker<float>(0))
            .AddAttribute(
                "enableVirtualQueue",
                "Enable the throughput virtual queue",
                BooleanValue(true),
                MakeBooleanAccessor(&NrMacSchedulerOfdmaDPP::m_enableVirtualQueue),
                MakeBooleanChecker())
                ;

    return tid;
}

NrMacSchedulerOfdmaDPP::NrMacSchedulerOfdmaDPP()
    : NrMacSchedulerOfdma()
{
  
}

std::shared_ptr<NrMacSchedulerUeInfo>
NrMacSchedulerOfdmaDPP::CreateUeRepresentation(
    const NrMacCschedSapProvider::CschedUeConfigReqParameters& params) const
{
    NrMacSchedulerUeInfoDPP::SetDppV (m_v_lyapunov);

    NS_LOG_FUNCTION(this);
    return std::make_shared<NrMacSchedulerUeInfoDPP>(
        params.m_rnti,
        params.m_beamConfId,
        std::bind(&NrMacSchedulerOfdmaDPP::GetNumRbPerRbg, this));
}

void
NrMacSchedulerOfdmaDPP::BeforeDlSched(const UePtrAndBufferReq& ue,
                                      const FTResources& assignableInIteration) const
{
    NS_LOG_FUNCTION(this);
    auto uePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoDPP>(ue.first);
    //  TODO Obtener FH capacity tput constraint
}

void
NrMacSchedulerOfdmaDPP::AssignedDlResources(const UePtrAndBufferReq& ue,
                                            [[maybe_unused]] const FTResources& assigned,
                                            [[maybe_unused]] const FTResources& totAssigned) const
{
    NS_LOG_FUNCTION(this);
    auto uePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoDPP>(ue.first);
    double timeSlot = 1e-3;
    if(m_enableVirtualQueue)
    {
        uePtr->UpdateDlTputVirtualQueue(totAssigned, timeSlot, m_dlAmc);
    }
}

void
NrMacSchedulerOfdmaDPP::NotAssignedDlResources(
    const UePtrAndBufferReq& ue,
    [[maybe_unused]] const FTResources& assigned,
    [[maybe_unused]] const FTResources& totAssigned) const
{
    NS_LOG_FUNCTION(this);
    auto uePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoDPP>(ue.first);
    double timeSlot = 1e-3;
    uePtr->UpdateDlTputVirtualQueue(assigned, timeSlot, m_dlAmc);
}

void
NrMacSchedulerOfdmaDPP::AssignedUlResources(const UePtrAndBufferReq& ue,
                                            [[maybe_unused]] const FTResources& assigned,
                                            [[maybe_unused]] const FTResources& totAssigned) const
{
    NS_LOG_FUNCTION(this);
    GetFirst GetUe;
    GetUe(ue)->UpdateUlMetric(m_ulAmc);
}

std::function<bool(const NrMacSchedulerNs3::UePtrAndBufferReq& lhs,
                   const NrMacSchedulerNs3::UePtrAndBufferReq& rhs)>
NrMacSchedulerOfdmaDPP::GetUeCompareDlFn() const
{
    return NrMacSchedulerUeInfoDPP::CompareUeWeightsDl;
}

std::function<bool(const NrMacSchedulerNs3::UePtrAndBufferReq& lhs,
                   const NrMacSchedulerNs3::UePtrAndBufferReq& rhs)>
NrMacSchedulerOfdmaDPP::GetUeCompareUlFn() const
{
    return NrMacSchedulerUeInfoDPP::CompareUeWeightsUl;
}

NrMacSchedulerNs3::BeamSymbolMap
NrMacSchedulerOfdmaDPP::AssignDLRBG(uint32_t symAvail, const ActiveUeMap& activeDl) const
{
    NS_LOG_DEBUG("-------------------- At time " << Simulator::Now());
    NS_LOG_FUNCTION(this);

    NS_LOG_DEBUG("# beams active flows: " << activeDl.size() << ", # sym: " << symAvail);

    GetFirst GetBeamId;
    GetSecond GetUeVector;
    BeamSymbolMap symPerBeam = GetSymPerBeam(symAvail, activeDl);

    // Iterate through the different beams
    auto ctr = 0;
    for (const auto& el : activeDl)
    {
        NS_LOG_DEBUG(Simulator::Now() << " |  Beam " << ctr++);
        // Distribute the RBG evenly among UEs of the same beam
        uint32_t beamSym = symPerBeam.at(GetBeamId(el));
        uint32_t rbgAssignable = 1 * beamSym;
        std::vector<UePtrAndBufferReq> ueVector;
        FTResources assigned(0, 0);
        const std::vector<uint8_t> dlNotchedRBGsMask = GetDlNotchedRbgMask();
        uint32_t resources = dlNotchedRBGsMask.size() > 0
                                 ? std::count(dlNotchedRBGsMask.begin(), dlNotchedRBGsMask.end(), 1)
                                 : GetBandwidthInRbg();
        NS_ASSERT(resources > 0);

        for (const auto& ue : GetUeVector(el))
        {
            ueVector.emplace_back(ue);
        }

        NS_LOG_DEBUG("\n------------------------- NEW SLOT -------------------------\n" <<
        "beamSym = " << beamSym << ", rbgAssignable = " << rbgAssignable <<
        ", resources = " << resources << ", ueVector.size() = " << ueVector.size() <<
        "\n------------------------------------------------------------\n");

        for (auto& ue : ueVector)
        {
            BeforeDlSched(
                ue,
                FTResources(
                    rbgAssignable * beamSym,
                    beamSym)); // TODO Obtener FH capacity tput constraint. Ahora mismo no hace nada
        }

        NS_LOG_DEBUG("+++++ ueVector.size() = " << ueVector.size() << ", dlNotchedRBGsMask.size() = "
                                          << dlNotchedRBGsMask.size()
                                          << ", resources = " << resources);

        NS_LOG_DEBUG("resources = " << resources);

        // Llama al scheduler
        NrMacSchedulerUeInfoDPP::LyapunovDPP(ueVector, resources, m_dlAmc);

        // TODO asignación
        // Assign 1 RBG for each available symbols for the beam,
        // and then update the count of available resources
        for (auto& ue : ueVector)
        {
            GetFirst GetUe;
            auto uePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoDPP>(GetUe(ue));

            // Distribute the resources allocated by the scheduler (m_dlRBGallocated) to this UE
            GetUe(ue)->m_dlRBG = rbgAssignable*uePtr->m_dlRBGallocated; // assign RBGs
            assigned.m_rbg += rbgAssignable*uePtr->m_dlRBGallocated; // Counter
            GetUe(ue)->m_dlSym = beamSym;                // assign sym
            assigned.m_sym = beamSym;                    // Counter
            NS_LOG_DEBUG("q = " << ue.second);
            NS_LOG_DEBUG("m_dlRBG = " << GetUe(ue)->m_dlRBG);
            NS_LOG_DEBUG("m_dlSym = " << unsigned(GetUe(ue)->m_dlSym));
            NS_LOG_DEBUG("assigned.m_sym = " << unsigned(assigned.m_sym));
            NS_LOG_DEBUG("m_dlRBGallocated = " << uePtr->m_dlRBGallocated);
            resources = resources - uePtr->m_dlRBGallocated; // Resources are RBG, so they do not consider the beamSym

            // Update metrics
            NS_LOG_INFO("Assigned " << GetUe(ue)->m_dlRBG << " DL RBG, spanned over " << beamSym
                                    << " SYM, to UE " << GetUe(ue)->m_rnti);
            if (GetUe(ue)->m_dlRBG > 0) // If resources have been allocated to this UE
            {
                // Following call to AssignedDlResources would update the
                // TB size in the NrMacSchedulerUeInfo of this particular UE
                // according the Rank Indicator reported by it. Only one call
                // to this method is enough even if the UE reported rank indicator 2,
                // since the number of RBG assigned to both the streams are the same.
                // NS_LOG_FUNCTION(this);
                AssignedDlResources(ue, FTResources(uePtr->m_dlRBG, beamSym), assigned);
                NS_LOG_INFO("Quedan " << resources << " recursos por asignar");
            }
            else // If no resources have been allocated to this UE
            {
                // NS_LOG_FUNCTION(this);
                NotAssignedDlResources(
                    ue,
                    FTResources(uePtr->m_dlRBGallocated * rbgAssignable * beamSym, beamSym),
                    assigned);
            }
        }
        // Save TBS of all UEs
        saveTBS(ueVector);
    }

    return symPerBeam;
}

void
NrMacSchedulerOfdmaDPP::saveTBS(
    const std::vector<ns3::NrMacSchedulerNs3::UePtrAndBufferReq> ueVector)
{
    static bool c = true;
    if (c)
    {
        outputFileTBS << "time\tue\ttbs\n";
    }
    c = false;
    for (const auto& ue : ueVector)
    {
        outputFileTBS << Simulator::Now().ToDouble(Time::MS) << "\t" <<  ue.first->m_rnti << "\t" << ue.first->m_dlTbSize.at(0) << "\n";
    }
}

} // namespace ns3
