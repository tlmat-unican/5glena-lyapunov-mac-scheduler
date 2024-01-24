/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

// Copyright (c) 2022 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
//
// SPDX-License-Identifier: GPL-2.0-only

#include "nr-mac-scheduler-ue-info-dpp.h"

#include <ns3/log.h>

#include <fstream>

#include <chrono>

#include <ctime>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("NrMacSchedulerUeInfoDPP");

double NrMacSchedulerUeInfoDPP::m_v_lyapunov = 5.1e6;

std::ofstream outputFileG("g.txt");
std::ofstream outputFileQ("q.txt");
std::ofstream outputFileAlpha("alpha.txt");

void
NrMacSchedulerUeInfoDPP::UpdateDlTputVirtualQueue(const NrMacSchedulerNs3::FTResources& assigned,
                                           double timeSlot,
                                           const Ptr<const NrAmc>& amc)
{
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG("amc->CalculateTbSize(mcs), nprb) = " << amc->CalculateTbSize(m_dlMcs.at(0), assigned.m_rbg*GetNumRbPerRbg()));

    NrMacSchedulerUeInfo::UpdateDlMetric(amc); // Actualiza m_dlTbSize
    uint32_t tbSize = 0;
    for (const auto& it : m_dlTbSize)
    {
        tbSize += it;
    }

    //m_currTputDl = static_cast<double>(tbSize) / (assigned.m_sym);
    m_currTputDl = static_cast<double>(tbSize)*8/timeSlot;
    NS_LOG_DEBUG("tbSize = " << static_cast<double>(tbSize) << ", m_sym = " << static_cast<double>(assigned.m_sym) << ", assigned.m_rbg = " << assigned.m_rbg);
    if (std::isnan(m_currTputDl))
    {
        m_currTputDl = 0.0;
    }
    uint32_t avg_gfbr = 0.0, gfbr = 0.0;
    int i = 0;
    for (const auto& ueLcg : m_dlLCG){
        std::vector<uint8_t> ueActiveLCs = ueLcg.second->GetActiveLCIds();
        for (const auto lcId : ueActiveLCs){
            std::unique_ptr<NrMacSchedulerLC>& LCPtr = ueLcg.second->GetLC(lcId);
            gfbr += unsigned(LCPtr->m_eRabGuaranteedBitrateDl);
            i++;
            NS_LOG_DEBUG("LC" << LCPtr->m_id << " - GFBR = " << unsigned(LCPtr->m_eRabGuaranteedBitrateDl));
        }
    }
    avg_gfbr = gfbr/i;
    NS_LOG_DEBUG("UE" << m_rnti << " - AVRG_GFBR = " << avg_gfbr);

    m_g = std::max(m_g + avg_gfbr - m_currTputDl, 0.0);
    NS_LOG_DEBUG("m_currTputDl = " << m_currTputDl << ", G de rnti " << m_rnti << " actualizada a " << m_g);
}

void NrMacSchedulerUeInfoDPP::saveQueuesState(const std::vector<ns3::NrMacSchedulerNs3::UePtrAndBufferReq> ueVector){
    static bool c = true;
    if(c){
        outputFileG << "time\tue\tg\n";
        outputFileQ << "time\tue\tq\n"; 
    }
    c = false;
    for(const auto& ue: ueVector){
        outputFileG << Simulator::Now().ToDouble (Time::MS) << "\t" << ue.first->m_rnti << "\t" << std::dynamic_pointer_cast<NrMacSchedulerUeInfoDPP>(ue.first)->m_g << "\n";
        outputFileQ << Simulator::Now().ToDouble (Time::MS) << "\t" <<  ue.first->m_rnti << "\t" << ue.second << "\n";
    }
}

void NrMacSchedulerUeInfoDPP::saveRBGallocation(const std::vector<ns3::NrMacSchedulerNs3::UePtrAndBufferReq> ueVector){
    static bool c = true;
    if(c){
        outputFileAlpha << "time\tUE\tresources\n";
    }
    c = false;
    for(const auto& ue: ueVector){
        outputFileAlpha << Simulator::Now().ToDouble (Time::MS) << "\t" << ue.first->m_rnti << "\t" << std::dynamic_pointer_cast<NrMacSchedulerUeInfoDPP>(ue.first)->m_dlRBGallocated << "\n";
    }
}

void NrMacSchedulerUeInfoDPP::LyapunovDPP(const std::vector<ns3::NrMacSchedulerNs3::UePtrAndBufferReq> ueVector, double resources, const Ptr<const NrAmc>& amc)
{
    std::vector<std::shared_ptr<NrMacSchedulerUeInfoDPP>> uePtrContainer;
    for (const auto& ue : ueVector){
        auto uePtr = std::dynamic_pointer_cast<NrMacSchedulerUeInfoDPP>(ue.first);
        if (uePtr) {
            NS_LOG_DEBUG("rnti = " << uePtr->m_rnti);
            uePtrContainer.push_back(uePtr);
        }
    }
    
    saveQueuesState(ueVector);

    // Parámetros del problema
    int N = ueVector.size();
    double gamma = std::numeric_limits<double>::infinity(); // TODO constraint FH capacity
    
    /* Crear un objeto de problema LP */
    glp_prob *lp;
    lp = glp_create_prob();
    glp_set_prob_name(lp, "Drift_plus_penalty");
    glp_set_obj_dir(lp, GLP_MIN);

    // Añadir columnas para las variables de decisión
    glp_add_cols(lp, N);
    for (int n = 1; n <= N; n++) {
        glp_set_col_name(lp, n, ("alpha_" + std::to_string(n)).c_str());
        glp_set_col_kind(lp, n, GLP_CV);
        glp_set_col_bnds(lp, n, GLP_LO, 0.0, 0.0);
        //glp_set_col_bnds(lp, n, GLP_DB, 0.0, resources);

        NS_ASSERT_MSG(uePtrContainer[n-1]->m_dlMcs.size() == 1, "Multiple streams not supported");
        glp_set_obj_coef(lp, n, (m_v_lyapunov
                                -amc->CalculateTbSize(uePtrContainer[n-1]->m_dlMcs.at(0), resources)
                                *(ueVector[n-1].second+uePtrContainer[n-1]->m_g)));
    }

    // Rows for constraints
    glp_add_rows(lp, 2);
    glp_set_row_name(lp, 1, "con1");
    glp_set_row_bnds(lp, 1, GLP_UP, 0.0, resources);
    glp_set_row_name(lp, 2, "con2");
    glp_set_row_bnds(lp, 2, GLP_UP, 0.0, gamma);

    glp_set_obj_name(lp, "obj");

    int *ia = (int*)malloc((1 + 2*N) * sizeof(int));
    int *ja = (int*)malloc((1 + 2*N) * sizeof(int));
    double *ar = (double*)malloc((1 + 2*N) * sizeof(double));
    int count = 1;
    for (int i = 1; i < 3; i++) {
        for (int j = 1; j <= N; j++) {
            ia[count] = i;
            ja[count] = j;
            ar[count] = 1;
            count++;
        }
    }

    glp_load_matrix(lp, 2*N, ia, ja, ar);
    free(ia);
    free(ja);
    free(ar);

    // Solve problem
    glp_term_out(GLP_OFF);
    glp_simplex(lp, NULL);
    glp_intopt(lp, NULL);

    // Solution
    // printf("Objective Value: %f\n", glp_get_obj_val(lp));
    std::vector<uint32_t> alpha;

    for (int n = 1; n <= N; n++) {
        uePtrContainer[n-1]->m_dlRBGallocated = glp_get_col_prim(lp, n);
        alpha.emplace_back(glp_get_col_prim(lp, n));
        NS_LOG_DEBUG("Sol. UE" << uePtrContainer[n-1]->m_rnti << " = " << uePtrContainer[n-1]->m_dlRBGallocated);
    }

    // Print decision
    saveRBGallocation(ueVector);
    
    // Free
    glp_delete_prob(lp);
}

bool NrMacSchedulerUeInfoDPP::CompareUeWeightsDl(const NrMacSchedulerNs3::UePtrAndBufferReq& lue, const NrMacSchedulerNs3::UePtrAndBufferReq& rue)
{   
    return false;
}

} // namespace ns3
