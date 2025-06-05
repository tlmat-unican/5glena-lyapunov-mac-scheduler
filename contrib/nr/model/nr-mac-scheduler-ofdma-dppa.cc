/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

// Copyright (c) 2019 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
//
// SPDX-License-Identifier: GPL-2.0-only

#include "nr-mac-scheduler-ofdma-dppa.h"

#include "nr-mac-scheduler-ue-info-dppa.h"

#include <ns3/log.h>

#include <fstream>

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("NrMacSchedulerOfdmaDPPA");
NS_OBJECT_ENSURE_REGISTERED(NrMacSchedulerOfdmaDPPA);

TypeId
NrMacSchedulerOfdmaDPPA::GetTypeId()
{
    static TypeId tid = 
        TypeId("ns3::NrMacSchedulerOfdmaDPPA")
            .SetParent<NrMacSchedulerOfdma>()
            .AddConstructor<NrMacSchedulerOfdmaDPPA>()
            .AddAttribute(
                "DppV",
                "Value of the weight value to consider the objective function",
                DoubleValue(0),
                MakeDoubleAccessor(&NrMacSchedulerOfdmaDPPA::m_v_lyapunov),
                MakeDoubleChecker<double>(0))
            .AddAttribute(
                "DppWeightQ",
                "Value of the weight value to consider the queue Q",
                DoubleValue(1),
                MakeDoubleAccessor(&NrMacSchedulerOfdmaDPPA::m_weight_q),
                MakeDoubleChecker<double>(0))
            .AddAttribute(
                "DppWeightG",
                "Value of the weight value to consider the queu G",
                DoubleValue(1),
                MakeDoubleAccessor(&NrMacSchedulerOfdmaDPPA::m_weight_g),
                MakeDoubleChecker<double>(0))
            .AddAttribute(
                "EnableVirtualQueue",
                "Enable the throughput virtual queue",
                BooleanValue(true),
                MakeBooleanAccessor(&NrMacSchedulerOfdmaDPPA::m_enableVirtualQueue),
                MakeBooleanChecker())
                ;

    return tid;
}

NrMacSchedulerOfdmaDPPA::NrMacSchedulerOfdmaDPPA()
    : NrMacSchedulerOfdma()
{
  
}

std::shared_ptr<NrMacSchedulerUeInfo>
NrMacSchedulerOfdmaDPPA::CreateUeRepresentation(
    const NrMacCschedSapProvider::CschedUeConfigReqParameters& params) const
{
    NrMacSchedulerUeInfoDPPA::SetDppWeights (m_v_lyapunov, m_weight_q, m_weight_g);

    NS_LOG_FUNCTION(this);
    return std::make_shared<NrMacSchedulerUeInfoDPPA>(
        params.m_rnti,
        params.m_beamConfId,
        std::bind(&NrMacSchedulerOfdmaDPPA::GetNumRbPerRbg, this));
}

void NrMacSchedulerOfdmaDPPA::saveQueuesState(const ns3::NrMacSchedulerNs3::UePtrAndBufferReq ue) const{
    static std::ofstream outputFileG("queue_g.txt");
    static std::ofstream outputFileQ("queue_q.txt");
    static bool c = true;
    if(c){
        outputFileG << "time\tue\tg\n";
        outputFileQ << "time\tue\tq\n"; 
    }
    c = false;
    outputFileG << Simulator::Now().ToDouble (Time::MS) << "\t" << ue.first->m_rnti << "\t" << std::dynamic_pointer_cast<NrMacSchedulerUeInfoDPPA>(ue.first)->m_g << "\n";
    outputFileQ << Simulator::Now().ToDouble (Time::MS) << "\t" <<  ue.first->m_rnti << "\t" << ue.second << "\n";
}

void NrMacSchedulerOfdmaDPPA::saveRBGallocation(const ns3::NrMacSchedulerNs3::UePtrAndBufferReq ue, const NrMacSchedulerNs3::FTResources& assigned) const{
    static std::ofstream outputFileAlpha("alpha.txt");
    static bool c = true;
    if(c){
        outputFileAlpha << "time\tue\tresources\tmcs\tULcqi\tULsinr\n";
    }
    c = false;
    outputFileAlpha << Simulator::Now().ToDouble (Time::MS) << "\t" << ue.first->m_rnti << "\t" << assigned.m_rbg << "\t" << (int)ue.first->m_dlMcs.at(0) << "\t" << (ue.first->m_dlCqi.m_wbCqi.size() > 0 ? ue.first->m_dlCqi.m_wbCqi[0] : 0) << "\t" << (ue.first->m_ulCqi.m_sinr.size() > 0 ? ue.first->m_ulCqi.m_sinr[0] : 0) << "\n";
}

NrMacSchedulerNs3::BeamSymbolMap
NrMacSchedulerOfdmaDPPA::AssignDLRBG(uint32_t symAvail, const ActiveUeMap& activeDl) const
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
        uint32_t beamSym = symPerBeam.at(GetBeamId(el)); // Number of symbols available for assignment
        uint32_t rbgAssignable = 1 * beamSym;
        std::vector<UePtrAndBufferReq> ueVector; // Active UEs, i.e. with data to send
        FTResources assigned(0, 0); // Total number of resources assigned (RBGs, symbols)
        const std::vector<uint8_t> dlNotchedRBGsMask = GetDlNotchedRbgMask();
        uint32_t resources = dlNotchedRBGsMask.size() > 0
                                 ? std::count(dlNotchedRBGsMask.begin(), dlNotchedRBGsMask.end(), 1)
                                 : GetBandwidthInRbg(); // Number of RBGs (OFDMA) available for assignment

        for (const auto& ue : GetUeVector(el))
        {
            ueVector.emplace_back(ue);
        }

        NS_LOG_INFO("\n------------------------- NEW SLOT -------------------------\n" <<
        "beamSym = " << beamSym << ", rbgAssignable = " << rbgAssignable <<
        ", resources = " << resources << ", ueVector.size() = " << ueVector.size() <<
        "\n------------------------------------------------------------\n");

        for (auto& ue : ueVector)
        {
            BeforeDlSched(ue,FTResources(rbgAssignable*resources,beamSym)); // Update the K metric for all the UEs in ueVector. TODO request the FH capacity control the current tput/RBGs constraint
        }

        NS_LOG_DEBUG("+++++ ueVector.size() = " << ueVector.size() << ", dlNotchedRBGsMask.size() = "
                                          << dlNotchedRBGsMask.size()
                                          << ", resources = " << resources);

        NS_LOG_DEBUG("resources = " << resources);

        // Llama al scheduler
        GetFirst GetUe;
        //std::cout << "ueVector:" << std::endl;
        for (auto it: ueVector){
            NS_LOG_INFO("- UE" << it.first->m_rnti);
            //std::cout << "- UE" << it.first->m_rnti << std::endl;
        }

        std::sort(ueVector.begin(), ueVector.end(), GetUeCompareDlFn()); // Sort UEs in ueVector as a function of K from smallest to largest

        NS_LOG_INFO("Llama sort(" << GetUe(*ueVector.begin())->m_rnti << ", " << GetUe(ueVector.back())->m_rnti << ")");
        NS_LOG_INFO("Usuarios ordenados: ");

        //std::cout << "priority vector:" << std::endl;
        for (auto it: ueVector){
            NS_LOG_INFO("- UE" << GetUe(it)->m_rnti);
            //std::cout << "- UE" << GetUe(it)->m_rnti << std::endl;
        }
        
        // Here the ueVector is sorted by priority, but we need to check if V is sufficiently high that not resources must be allocated
        auto prio_ue = ueVector.begin(); // UE to whom all resources are assigned a priori

        // Assigment:
        // Assign 1 RBG for each available symbols for the beam,
        // and then update the count of available resources
        while (resources > 0 && prio_ue != ueVector.end()){ // As long as there are resources and UEs continue with the assignment
            if (-m_v_lyapunov*50 >= std::dynamic_pointer_cast<NrMacSchedulerUeInfoDPPA>(GetUe(*prio_ue))->m_k){ // The UE can receive resources
                NS_LOG_INFO("Priority to UE" << GetUe(*prio_ue)->m_rnti);
            }else{
                NS_LOG_INFO("Discard UE" << GetUe(*prio_ue)->m_rnti);
                break;
            }
            // If there are two streams we add the TbSizes of the two streams to satisfy the bufQueueSize
            uint32_t tbSize = 0;
            for (const auto& it : GetUe(*prio_ue)->m_dlTbSize)
            {
                tbSize += it;
            }
            NS_LOG_INFO("Current tbSize is " << tbSize << " and the buffer is " << prio_ue->second);
            if (prio_ue->second > tbSize){ // Check that the UE has enough data
                GetUe(*prio_ue)->m_dlRBG += rbgAssignable; // Assign 1 RBG for each available symbols for the beam
                assigned.m_rbg += rbgAssignable; // Counter of assigned resources
                GetUe(*prio_ue)->m_dlSym = beamSym; // assign symbols
                assigned.m_sym = beamSym; // Counter of assigned resources
                resources -= 1; // Counter of available resourecs. Resources are RBG (OFDMA), so they do not consider the beamSym
                // Update metrics
                AssignedDlResources(*prio_ue, FTResources(rbgAssignable, beamSym), assigned);
                NS_LOG_INFO("Assigned " << GetUe(*prio_ue)->m_dlRBG << " DL RBG, spanned over " << beamSym
                                        << " SYM, to UE " << GetUe(*prio_ue)->m_rnti << ", " << resources << " resources remain");
            }else{
                prio_ue++; // Go to the next UE in the ueVector
            }
        }

        // Update metrics
        for (auto it: ueVector){
            if (GetUe(it)->m_dlRBG == 0)
            {
                // NS_LOG_FUNCTION(this);
                NotAssignedDlResources(it,FTResources(GetUe(it)->m_dlRBG, beamSym), assigned); // Update metrics for the UEs without resources
            }else{
                // Update G here and not in AssignedDlResources() because we need to call UpdateDlTputVirtualQueue with the full resource assignment
                auto uePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoDPPA>(GetUe(it));
                if(m_enableVirtualQueue)
                {
                    double timeSlot = m_macSchedSapUser->GetSlotPeriod().GetNanoSeconds()/1e9;
                    //std::cout << "SlotPeriod: " << m_macSchedSapUser->GetSlotPeriod().GetNanoSeconds()/1e9 << std::endl;
                    double aux = uePtr->m_g;
                    uePtr->UpdateDlTputVirtualQueue(FTResources(GetUe(it)->m_dlRBG, beamSym), timeSlot, m_dlAmc);
                    NS_LOG_INFO("Virtual queue UE" << uePtr->m_rnti << " updated " << aux << " -> " << uePtr->m_g);
                }
                saveRBGallocation(it, FTResources(GetUe(it)->m_dlRBG, beamSym));
                saveQueuesState(it);
            }
        }
    }

    return symPerBeam;
}

void
NrMacSchedulerOfdmaDPPA::BeforeDlSched(const UePtrAndBufferReq& ue,
                                      const FTResources& assignableInIteration) const
{
    NS_LOG_FUNCTION(this);
    auto uePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoDPPA>(ue.first);

    //  TODO Obtener FH capacity tput constraint

    // Update K
    uePtr->UpdateDlK(ue.second, assignableInIteration, m_dlAmc, m_macSchedSapUser->GetSlotPeriod().GetNanoSeconds()/1e9);
    // uePtr->m_k = -(ue.second + uePtr->m_g) * m_dlAmc->CalculateTbSize(uePtr->m_dlMcs.at(0), assignableInIteration.m_rbg*GetNumRbPerRbg());
    NS_LOG_INFO("CalculateTbSize con MCS de " << (int)uePtr->m_dlMcs.at(0) << ", #RBs de " << assignableInIteration.m_rbg*GetNumRbPerRbg());
    NS_LOG_INFO("UE" << uePtr->m_rnti << "->m_k = -" 
            << m_dlAmc->CalculateTbSize(uePtr->m_dlMcs.at(0), assignableInIteration.m_rbg*GetNumRbPerRbg()) 
            << " * (" << m_weight_q << " * " << ue.second << " + "
            << m_weight_g << " * " << uePtr->m_g << ") = " 
            << uePtr->m_k);
}

void
NrMacSchedulerOfdmaDPPA::AssignedDlResources(const UePtrAndBufferReq& ue,
                                            [[maybe_unused]] const FTResources& assigned,
                                            [[maybe_unused]] const FTResources& totAssigned) const
{
    NS_LOG_FUNCTION(this);
    // auto uePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoDPPA>(ue.first);
    // if(m_enableVirtualQueue)
    // {
    //     double timeSlot = 1e-3;
    //     double aux = uePtr->m_g;
    //     uePtr->UpdateDlTputVirtualQueue(totAssigned, timeSlot, m_dlAmc);
    //     NS_LOG_INFO("Virtual queue UE" << uePtr->m_rnti << " updated " << aux << " -> " << uePtr->m_g);
    // }
    // saveRBGallocation(ue, assigned);
    // saveQueuesState(ue);
    ue.first->UpdateDlMetric(m_dlAmc); // Actualiza m_dlTbSize
}

void
NrMacSchedulerOfdmaDPPA::NotAssignedDlResources(
    const UePtrAndBufferReq& ue,
    [[maybe_unused]] const FTResources& assigned,
    [[maybe_unused]] const FTResources& totAssigned) const
{
    NS_LOG_FUNCTION(this);
    auto uePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoDPPA>(ue.first);
    if(m_enableVirtualQueue)
    {
        //double timeSlot = 1e-3;
        double timeSlot = m_macSchedSapUser->GetSlotPeriod().GetNanoSeconds()/1e9;
        uePtr->UpdateDlTputVirtualQueue(assigned, timeSlot, m_dlAmc);
    }
    saveRBGallocation(ue, assigned);
    saveQueuesState(ue);
    //ue.first->UpdateDlMetric(m_dlAmc); // Actualiza m_dlTbSize
}

void
NrMacSchedulerOfdmaDPPA::AssignedUlResources(const UePtrAndBufferReq& ue,
                                            [[maybe_unused]] const FTResources& assigned,
                                            [[maybe_unused]] const FTResources& totAssigned) const
{
    NS_LOG_FUNCTION(this);
    GetFirst GetUe;
    GetUe(ue)->UpdateUlMetric(m_ulAmc);
}

std::function<bool(const NrMacSchedulerNs3::UePtrAndBufferReq& lhs,
                   const NrMacSchedulerNs3::UePtrAndBufferReq& rhs)>
NrMacSchedulerOfdmaDPPA::GetUeCompareDlFn() const
{
    return NrMacSchedulerUeInfoDPPA::CompareUeWeightsDl;
}

std::function<bool(const NrMacSchedulerNs3::UePtrAndBufferReq& lhs,
                   const NrMacSchedulerNs3::UePtrAndBufferReq& rhs)>
NrMacSchedulerOfdmaDPPA::GetUeCompareUlFn() const
{
    return NrMacSchedulerUeInfoDPPA::CompareUeWeightsUl;
}

} // namespace ns3
