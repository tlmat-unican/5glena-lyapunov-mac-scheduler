/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

// Copyright (c) 2022 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
//
// SPDX-License-Identifier: GPL-2.0-only

#include "nr-mac-scheduler-tdma-qos.h"

#include "nr-mac-scheduler-ue-info-qos.h"

#include <ns3/double.h>
#include <ns3/log.h>

#include <algorithm>
#include <functional>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("NrMacSchedulerTdmaQos");
NS_OBJECT_ENSURE_REGISTERED(NrMacSchedulerTdmaQos);

TypeId
NrMacSchedulerTdmaQos::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::NrMacSchedulerTdmaQos")
            .SetParent<NrMacSchedulerTdmaRR>()
            .AddConstructor<NrMacSchedulerTdmaQos>()
            .AddAttribute("FairnessIndex",
                          "Value (between 0 and 1) that defines the PF metric (1 is the "
                          "traditional 3GPP PF, 0 is RR in throughput",
                          DoubleValue(1),
                          MakeDoubleAccessor(&NrMacSchedulerTdmaQos::SetFairnessIndex,
                                             &NrMacSchedulerTdmaQos::GetFairnessIndex),
                          MakeDoubleChecker<double>(0, 1))
            .AddAttribute(
                "LastAvgTPutWeight",
                "Weight of the last average throughput in the average throughput calculation",
                DoubleValue(99),
                MakeDoubleAccessor(&NrMacSchedulerTdmaQos::SetTimeWindow,
                                   &NrMacSchedulerTdmaQos::GetTimeWindow),
                MakeDoubleChecker<double>(0));
    return tid;
}

NrMacSchedulerTdmaQos::NrMacSchedulerTdmaQos()
    : NrMacSchedulerTdmaRR()
{
    NS_LOG_FUNCTION(this);
}

void
NrMacSchedulerTdmaQos::SetFairnessIndex(double v)
{
    NS_LOG_FUNCTION(this);
    m_alpha = v;
}

double
NrMacSchedulerTdmaQos::GetFairnessIndex() const
{
    NS_LOG_FUNCTION(this);
    return m_alpha;
}

void
NrMacSchedulerTdmaQos::SetTimeWindow(double v)
{
    NS_LOG_FUNCTION(this);
    m_timeWindow = v;
}

double
NrMacSchedulerTdmaQos::GetTimeWindow() const
{
    NS_LOG_FUNCTION(this);
    return m_timeWindow;
}

std::shared_ptr<NrMacSchedulerUeInfo>
NrMacSchedulerTdmaQos::CreateUeRepresentation(
    const NrMacCschedSapProvider::CschedUeConfigReqParameters& params) const
{
    NS_LOG_FUNCTION(this);
    return std::make_shared<NrMacSchedulerUeInfoQos>(
        m_alpha,
        params.m_rnti,
        params.m_beamConfId,
        std::bind(&NrMacSchedulerTdmaQos::GetNumRbPerRbg, this));
}

std::function<bool(const NrMacSchedulerNs3::UePtrAndBufferReq& lhs,
                   const NrMacSchedulerNs3::UePtrAndBufferReq& rhs)>
NrMacSchedulerTdmaQos::GetUeCompareDlFn() const
{
    return NrMacSchedulerUeInfoQos::CompareUeWeightsDl;
}

std::function<bool(const NrMacSchedulerNs3::UePtrAndBufferReq& lhs,
                   const NrMacSchedulerNs3::UePtrAndBufferReq& rhs)>
NrMacSchedulerTdmaQos::GetUeCompareUlFn() const
{
    return NrMacSchedulerUeInfoQos::CompareUeWeightsUl;
}

void
NrMacSchedulerTdmaQos::AssignedDlResources(const UePtrAndBufferReq& ue,
                                           [[maybe_unused]] const FTResources& assigned,
                                           const FTResources& totAssigned) const
{
    NS_LOG_FUNCTION(this);
    auto uePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoQos>(ue.first);
    uePtr->UpdateDlQosMetric(totAssigned, m_timeWindow, m_dlAmc);
}

void
NrMacSchedulerTdmaQos::NotAssignedDlResources(
    const NrMacSchedulerNs3::UePtrAndBufferReq& ue,
    [[maybe_unused]] const NrMacSchedulerNs3::FTResources& notAssigned,
    const NrMacSchedulerNs3::FTResources& totAssigned) const
{
    NS_LOG_FUNCTION(this);
    auto uePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoQos>(ue.first);
    uePtr->UpdateDlQosMetric(totAssigned, m_timeWindow, m_dlAmc);
}

void
NrMacSchedulerTdmaQos::AssignedUlResources(const UePtrAndBufferReq& ue,
                                           [[maybe_unused]] const FTResources& assigned,
                                           const FTResources& totAssigned) const
{
    NS_LOG_FUNCTION(this);
    auto uePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoQos>(ue.first);
    uePtr->UpdateUlQosMetric(totAssigned, m_timeWindow, m_ulAmc);
}

void
NrMacSchedulerTdmaQos::NotAssignedUlResources(
    const NrMacSchedulerNs3::UePtrAndBufferReq& ue,
    [[maybe_unused]] const NrMacSchedulerNs3::FTResources& notAssigned,
    const NrMacSchedulerNs3::FTResources& totAssigned) const
{
    NS_LOG_FUNCTION(this);
    auto uePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoQos>(ue.first);
    uePtr->UpdateUlQosMetric(totAssigned, m_timeWindow, m_ulAmc);
}

void
NrMacSchedulerTdmaQos::BeforeDlSched(const UePtrAndBufferReq& ue,
                                     const FTResources& assignableInIteration) const
{
    NS_LOG_FUNCTION(this);
    auto uePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoQos>(ue.first);
    uePtr->CalculatePotentialTPutDl(assignableInIteration, m_dlAmc);
}

void
NrMacSchedulerTdmaQos::BeforeUlSched(const UePtrAndBufferReq& ue,
                                     const FTResources& assignableInIteration) const
{
    NS_LOG_FUNCTION(this);
    auto uePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoQos>(ue.first);
    uePtr->CalculatePotentialTPutUl(assignableInIteration, m_ulAmc);
}

} // namespace ns3
