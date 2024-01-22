/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

// Copyright (c) 2020 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
//
// SPDX-License-Identifier: GPL-2.0-only

#include "ideal-beamforming-algorithm.h"

#include "beam-manager.h"
#include "nr-gnb-net-device.h"
#include "nr-gnb-phy.h"
#include "nr-spectrum-phy.h"
#include "nr-ue-net-device.h"
#include "nr-ue-phy.h"

#include <ns3/angles.h>
#include <ns3/double.h>
#include <ns3/mobility-module.h>
#include <ns3/multi-model-spectrum-channel.h>
#include <ns3/node.h>
#include <ns3/nr-spectrum-value-helper.h>
#include <ns3/uinteger.h>
#include <ns3/uniform-planar-array.h>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("IdealBeamformingAlgorithm");
NS_OBJECT_ENSURE_REGISTERED(CellScanBeamforming);
NS_OBJECT_ENSURE_REGISTERED(CellScanBeamformingAzimuthZenith);
NS_OBJECT_ENSURE_REGISTERED(DirectPathBeamforming);
NS_OBJECT_ENSURE_REGISTERED(QuasiOmniDirectPathBeamforming);
NS_OBJECT_ENSURE_REGISTERED(OptimalCovMatrixBeamforming);

TypeId
IdealBeamformingAlgorithm::GetTypeId()
{
    static TypeId tid = TypeId("ns3::IdealBeamformingAlgorithm").SetParent<Object>();
    return tid;
}

TypeId
CellScanBeamforming::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::CellScanBeamforming")
            .SetParent<IdealBeamformingAlgorithm>()
            .AddConstructor<CellScanBeamforming>()
            .AddAttribute("BeamSearchAngleStep",
                          "Angle step when searching for the best beam",
                          DoubleValue(30),
                          MakeDoubleAccessor(&CellScanBeamforming::SetBeamSearchAngleStep,
                                             &CellScanBeamforming::GetBeamSearchAngleStep),
                          MakeDoubleChecker<double>());

    return tid;
}

void
CellScanBeamforming::SetBeamSearchAngleStep(double beamSearchAngleStep)
{
    m_beamSearchAngleStep = beamSearchAngleStep;
}

double
CellScanBeamforming::GetBeamSearchAngleStep() const
{
    return m_beamSearchAngleStep;
}

BeamformingVectorPair
CellScanBeamforming::GetBeamformingVectors(const Ptr<NrSpectrumPhy>& gnbSpectrumPhy,
                                           const Ptr<NrSpectrumPhy>& ueSpectrumPhy) const
{
    NS_ABORT_MSG_IF(gnbSpectrumPhy == nullptr || ueSpectrumPhy == nullptr,
                    "Something went wrong, gnb or UE PHY layer not set.");
    double distance = gnbSpectrumPhy->GetMobility()->GetDistanceFrom(ueSpectrumPhy->GetMobility());
    NS_ABORT_MSG_IF(distance == 0,
                    "Beamforming method cannot be performed between two devices that are placed in "
                    "the same position.");

    Ptr<SpectrumChannel> gnbSpectrumChannel =
        gnbSpectrumPhy
            ->GetSpectrumChannel(); // SpectrumChannel should be const.. but need to change ns-3-dev
    Ptr<SpectrumChannel> ueSpectrumChannel = ueSpectrumPhy->GetSpectrumChannel();

    Ptr<const PhasedArraySpectrumPropagationLossModel> gnbThreeGppSpectrumPropModel =
        gnbSpectrumChannel->GetPhasedArraySpectrumPropagationLossModel();
    Ptr<const PhasedArraySpectrumPropagationLossModel> ueThreeGppSpectrumPropModel =
        ueSpectrumChannel->GetPhasedArraySpectrumPropagationLossModel();
    NS_ASSERT_MSG(gnbThreeGppSpectrumPropModel == ueThreeGppSpectrumPropModel,
                  "Devices should be connected on the same spectrum channel");

    std::vector<int> activeRbs;
    for (size_t rbId = 0; rbId < gnbSpectrumPhy->GetRxSpectrumModel()->GetNumBands(); rbId++)
    {
        activeRbs.push_back(rbId);
    }

    Ptr<const SpectrumValue> fakePsd = NrSpectrumValueHelper::CreateTxPowerSpectralDensity(
        0.0,
        activeRbs,
        gnbSpectrumPhy->GetRxSpectrumModel(),
        NrSpectrumValueHelper::UNIFORM_POWER_ALLOCATION_BW);
    Ptr<SpectrumSignalParameters> fakeParams = Create<SpectrumSignalParameters>();
    fakeParams->psd = fakePsd->Copy();

    double max = 0;
    double maxTxTheta = 0;
    double maxRxTheta = 0;
    uint16_t maxTxSector = 0;
    uint16_t maxRxSector = 0;
    PhasedArrayModel::ComplexVector maxTxW;
    PhasedArrayModel::ComplexVector maxRxW;

    UintegerValue uintValue;
    gnbSpectrumPhy->GetAntenna()->GetAttribute("NumRows", uintValue);
    uint32_t txNumRows = static_cast<uint32_t>(uintValue.Get());
    ueSpectrumPhy->GetAntenna()->GetAttribute("NumRows", uintValue);
    uint32_t rxNumRows = static_cast<uint32_t>(uintValue.Get());

    NS_ASSERT(gnbSpectrumPhy->GetAntenna()->GetObject<PhasedArrayModel>()->GetNumberOfElements() &&
              ueSpectrumPhy->GetAntenna()->GetObject<PhasedArrayModel>()->GetNumberOfElements());

    uint16_t numRowsTx = static_cast<uint16_t>(txNumRows);
    uint16_t numRowsRx = static_cast<uint16_t>(rxNumRows);
    for (double txTheta = 60; txTheta < 121; txTheta = txTheta + m_beamSearchAngleStep)
    {
        for (uint16_t txSector = 0; txSector <= numRowsTx; txSector++)
        {
            NS_ASSERT(txSector < UINT16_MAX);

            gnbSpectrumPhy->GetBeamManager()->SetSector(txSector, txTheta);
            PhasedArrayModel::ComplexVector txW =
                gnbSpectrumPhy->GetBeamManager()->GetCurrentBeamformingVector();

            if (maxTxW.GetSize() == 0)
            {
                maxTxW = txW; // initialize maxTxW
            }

            for (double rxTheta = 60; rxTheta < 121;
                 rxTheta = static_cast<uint16_t>(rxTheta + m_beamSearchAngleStep))
            {
                for (uint16_t rxSector = 0; rxSector <= numRowsRx; rxSector++)
                {
                    NS_ASSERT(rxSector < UINT16_MAX);

                    ueSpectrumPhy->GetBeamManager()->SetSector(rxSector, rxTheta);
                    PhasedArrayModel::ComplexVector rxW =
                        ueSpectrumPhy->GetBeamManager()->GetCurrentBeamformingVector();

                    if (maxRxW.GetSize() == 0)
                    {
                        maxRxW = rxW; // initialize maxRxW
                    }

                    NS_ABORT_MSG_IF(txW.GetSize() == 0 || rxW.GetSize() == 0,
                                    "Beamforming vectors must be initialized in order to calculate "
                                    "the long term matrix.");

                    Ptr<SpectrumValue> rxPsd =
                        gnbThreeGppSpectrumPropModel->CalcRxPowerSpectralDensity(
                            fakeParams,
                            gnbSpectrumPhy->GetMobility(),
                            ueSpectrumPhy->GetMobility(),
                            gnbSpectrumPhy->GetAntenna()->GetObject<PhasedArrayModel>(),
                            ueSpectrumPhy->GetAntenna()->GetObject<PhasedArrayModel>());

                    size_t nbands = rxPsd->GetSpectrumModel()->GetNumBands();
                    double power = Sum(*rxPsd) / nbands;

                    NS_LOG_LOGIC(
                        " Rx power: "
                        << power << "txTheta " << txTheta << " rxTheta " << rxTheta << " tx sector "
                        << (M_PI * static_cast<double>(txSector) / static_cast<double>(txNumRows) -
                            0.5 * M_PI) /
                               (M_PI)*180
                        << " rx sector "
                        << (M_PI * static_cast<double>(rxSector) / static_cast<double>(rxNumRows) -
                            0.5 * M_PI) /
                               (M_PI)*180);

                    if (max < power)
                    {
                        max = power;
                        maxTxSector = txSector;
                        maxRxSector = rxSector;
                        maxTxTheta = txTheta;
                        maxRxTheta = rxTheta;
                        maxTxW = txW;
                        maxRxW = rxW;
                    }
                }
            }
        }
    }

    BeamformingVector gnbBfv =
        BeamformingVector(std::make_pair(maxTxW, BeamId(maxTxSector, maxTxTheta)));
    BeamformingVector ueBfv =
        BeamformingVector(std::make_pair(maxRxW, BeamId(maxRxSector, maxRxTheta)));

    NS_LOG_DEBUG(
        "Beamforming vectors for gNB with node id: "
        << gnbSpectrumPhy->GetMobility()->GetObject<Node>()->GetId()
        << " and UE with node id: " << ueSpectrumPhy->GetMobility()->GetObject<Node>()->GetId()
        << " are txTheta " << maxTxTheta << " rxTheta " << maxRxTheta << " tx sector "
        << (M_PI * static_cast<double>(maxTxSector) / static_cast<double>(txNumRows) - 0.5 * M_PI) /
               (M_PI)*180
        << " rx sector "
        << (M_PI * static_cast<double>(maxRxSector) / static_cast<double>(rxNumRows) - 0.5 * M_PI) /
               (M_PI)*180);

    NS_ASSERT(maxTxW.GetSize() && maxRxW.GetSize());

    return BeamformingVectorPair(std::make_pair(gnbBfv, ueBfv));
}

TypeId
CellScanBeamformingAzimuthZenith::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CellScanBeamformingAzimuthZenith")
                            .SetParent<IdealBeamformingAlgorithm>()
                            .AddConstructor<CellScanBeamformingAzimuthZenith>();

    return tid;
}

BeamformingVectorPair
CellScanBeamformingAzimuthZenith::GetBeamformingVectors(
    const Ptr<NrSpectrumPhy>& gnbSpectrumPhy,
    const Ptr<NrSpectrumPhy>& ueSpectrumPhy) const
{
    NS_ABORT_MSG_IF(gnbSpectrumPhy == nullptr || ueSpectrumPhy == nullptr,
                    "Something went wrong, gnb or UE PHY layer not set.");
    double distance = gnbSpectrumPhy->GetMobility()->GetDistanceFrom(ueSpectrumPhy->GetMobility());
    NS_ABORT_MSG_IF(distance == 0,
                    "Beamforming method cannot be performed between "
                    "two devices that are placed in the same position.");

    Ptr<SpectrumChannel> gnbSpectrumChannel =
        gnbSpectrumPhy
            ->GetSpectrumChannel(); // SpectrumChannel should be const.. but need to change ns-3-dev
    Ptr<SpectrumChannel> ueSpectrumChannel = ueSpectrumPhy->GetSpectrumChannel();

    Ptr<const PhasedArraySpectrumPropagationLossModel> gnbThreeGppSpectrumPropModel =
        gnbSpectrumChannel->GetPhasedArraySpectrumPropagationLossModel();
    Ptr<const PhasedArraySpectrumPropagationLossModel> ueThreeGppSpectrumPropModel =
        ueSpectrumChannel->GetPhasedArraySpectrumPropagationLossModel();
    NS_ASSERT_MSG(gnbThreeGppSpectrumPropModel == ueThreeGppSpectrumPropModel,
                  "Devices should be connected on the same spectrum channel");

    std::vector<int> activeRbs;
    for (size_t rbId = 0; rbId < gnbSpectrumPhy->GetRxSpectrumModel()->GetNumBands(); rbId++)
    {
        activeRbs.push_back(rbId);
    }

    Ptr<const SpectrumValue> fakePsd = NrSpectrumValueHelper::CreateTxPowerSpectralDensity(
        0.0,
        activeRbs,
        gnbSpectrumPhy->GetRxSpectrumModel(),
        NrSpectrumValueHelper::UNIFORM_POWER_ALLOCATION_BW);
    Ptr<SpectrumSignalParameters> fakeParams = Create<SpectrumSignalParameters>();
    fakeParams->psd = fakePsd->Copy();

    double max = 0;
    double maxTxAzimuth = 0;
    double maxRxAzimuth = 0;
    double maxTxZenith = 0;
    double maxRxZenith = 0;
    PhasedArrayModel::ComplexVector maxTxW;
    PhasedArrayModel::ComplexVector maxRxW;

    UintegerValue uintValue;
    gnbSpectrumPhy->GetAntenna()->GetAttribute("NumRows", uintValue);
    ueSpectrumPhy->GetAntenna()->GetAttribute("NumRows", uintValue);

    NS_ASSERT(gnbSpectrumPhy->GetAntenna()->GetObject<PhasedArrayModel>()->GetNumberOfElements() &&
              ueSpectrumPhy->GetAntenna()->GetObject<PhasedArrayModel>()->GetNumberOfElements());

    for (uint i = 0; i < m_azimuth.size(); i++)
    {
        double azimuthTx = m_azimuth[i];
        for (uint ii = 0; ii < m_zenith.size(); ii++)
        {
            double zenithTx = m_zenith[ii];

            gnbSpectrumPhy->GetBeamManager()->SetSectorAz(azimuthTx, zenithTx);
            PhasedArrayModel::ComplexVector txW =
                gnbSpectrumPhy->GetBeamManager()->GetCurrentBeamformingVector();

            if (maxTxW.GetSize() == 0)
            {
                maxTxW = txW; // initialize maxTxW
            }

            for (uint iii = 0; iii < m_azimuth.size(); iii++)
            {
                double azimuthRx = m_azimuth[iii];
                for (uint iiii = 0; iiii < m_zenith.size(); iiii++)
                {
                    double zenithRx = m_zenith[iiii];

                    ueSpectrumPhy->GetBeamManager()->SetSectorAz(azimuthRx, zenithRx);
                    PhasedArrayModel::ComplexVector rxW =
                        ueSpectrumPhy->GetBeamManager()->GetCurrentBeamformingVector();

                    if (maxRxW.GetSize() == 0)
                    {
                        maxRxW = rxW; // initialize maxRxW
                    }

                    NS_ABORT_MSG_IF(txW.GetSize() == 0 || rxW.GetSize() == 0,
                                    "Beamforming vectors must be initialized in "
                                    "order to calculate the long term matrix.");

                    Ptr<SpectrumValue> rxPsd =
                        gnbThreeGppSpectrumPropModel->CalcRxPowerSpectralDensity(
                            fakeParams,
                            gnbSpectrumPhy->GetMobility(),
                            ueSpectrumPhy->GetMobility(),
                            gnbSpectrumPhy->GetAntenna()->GetObject<PhasedArrayModel>(),
                            ueSpectrumPhy->GetAntenna()->GetObject<PhasedArrayModel>());

                    size_t nbands = rxPsd->GetSpectrumModel()->GetNumBands();
                    double power = Sum(*rxPsd) / nbands;

                    NS_LOG_LOGIC(" Rx power: " << power << " azimuthTx " << azimuthTx
                                               << " zenithTx " << zenithTx << " azimuthRx "
                                               << azimuthRx << " zenithRx " << zenithRx);

                    if (max < power)
                    {
                        max = power;
                        maxTxAzimuth = azimuthTx;
                        maxRxAzimuth = azimuthRx;
                        maxTxZenith = zenithTx;
                        maxRxZenith = zenithRx;
                        maxTxW = txW;
                        maxRxW = rxW;
                    }
                }
            }
        }
    }

    BeamformingVector gnbBfv = BeamformingVector(
        std::make_pair(maxTxW, BeamId(static_cast<uint16_t>(maxTxAzimuth), maxTxZenith)));
    BeamformingVector ueBfv = BeamformingVector(
        std::make_pair(maxRxW, BeamId(static_cast<uint16_t>(maxRxAzimuth), maxRxZenith)));

    NS_LOG_DEBUG("Beamforming vectors for gNB with node id: "
                 << gnbSpectrumPhy->GetMobility()->GetObject<Node>()->GetId()
                 << " and UE with node id: "
                 << ueSpectrumPhy->GetMobility()->GetObject<Node>()->GetId() << " are azimuthTx "
                 << maxTxAzimuth << " zenithTx " << maxTxZenith << " azimuthRx " << maxRxAzimuth
                 << " zenithRx " << maxRxZenith);

    NS_ASSERT(maxTxW.GetSize() && maxRxW.GetSize());

    return BeamformingVectorPair(std::make_pair(gnbBfv, ueBfv));
}

TypeId
CellScanQuasiOmniBeamforming::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::CellScanQuasiOmniBeamforming")
            .SetParent<IdealBeamformingAlgorithm>()
            .AddConstructor<CellScanQuasiOmniBeamforming>()
            .AddAttribute("BeamSearchAngleStep",
                          "Angle step when searching for the best beam",
                          DoubleValue(30),
                          MakeDoubleAccessor(&CellScanQuasiOmniBeamforming::SetBeamSearchAngleStep,
                                             &CellScanQuasiOmniBeamforming::GetBeamSearchAngleStep),
                          MakeDoubleChecker<double>());

    return tid;
}

void
CellScanQuasiOmniBeamforming::SetBeamSearchAngleStep(double beamSearchAngleStep)
{
    m_beamSearchAngleStep = beamSearchAngleStep;
}

double
CellScanQuasiOmniBeamforming::GetBeamSearchAngleStep() const
{
    return m_beamSearchAngleStep;
}

BeamformingVectorPair
CellScanQuasiOmniBeamforming::GetBeamformingVectors(const Ptr<NrSpectrumPhy>& gnbSpectrumPhy,
                                                    const Ptr<NrSpectrumPhy>& ueSpectrumPhy) const
{
    NS_ABORT_MSG_IF(gnbSpectrumPhy == nullptr || ueSpectrumPhy == nullptr,
                    "Something went wrong, gnb or UE PHY layer not set.");
    double distance = gnbSpectrumPhy->GetMobility()->GetDistanceFrom(ueSpectrumPhy->GetMobility());
    NS_ABORT_MSG_IF(distance == 0,
                    "Beamforming method cannot be performed between two devices that are placed in "
                    "the same position.");

    Ptr<const PhasedArraySpectrumPropagationLossModel> txThreeGppSpectrumPropModel =
        gnbSpectrumPhy->GetSpectrumChannel()->GetPhasedArraySpectrumPropagationLossModel();
    Ptr<const PhasedArraySpectrumPropagationLossModel> rxThreeGppSpectrumPropModel =
        ueSpectrumPhy->GetSpectrumChannel()->GetPhasedArraySpectrumPropagationLossModel();
    NS_ASSERT_MSG(txThreeGppSpectrumPropModel == rxThreeGppSpectrumPropModel,
                  "Devices should be connected to the same spectrum channel");

    std::vector<int> activeRbs;
    for (size_t rbId = 0; rbId < gnbSpectrumPhy->GetRxSpectrumModel()->GetNumBands(); rbId++)
    {
        activeRbs.push_back(rbId);
    }

    Ptr<const SpectrumValue> fakePsd = NrSpectrumValueHelper::CreateTxPowerSpectralDensity(
        0.0,
        activeRbs,
        gnbSpectrumPhy->GetRxSpectrumModel(),
        NrSpectrumValueHelper::UNIFORM_POWER_ALLOCATION_BW);
    Ptr<SpectrumSignalParameters> fakeParams = Create<SpectrumSignalParameters>();
    fakeParams->psd = fakePsd->Copy();

    double max = 0;
    double maxTxTheta = 0;
    uint16_t maxTxSector = 0;
    PhasedArrayModel::ComplexVector maxTxW;

    UintegerValue uintValue;
    gnbSpectrumPhy->GetAntenna()->GetAttribute("NumRows", uintValue);
    uint32_t txNumRows = static_cast<uint32_t>(uintValue.Get());

    ueSpectrumPhy->GetBeamManager()
        ->ChangeToQuasiOmniBeamformingVector(); // we have to set it inmediatelly to q-omni so that
                                                // we can perform calculations when calling spectrum
                                                // model above

    PhasedArrayModel::ComplexVector rxW =
        ueSpectrumPhy->GetBeamManager()->GetCurrentBeamformingVector();
    BeamformingVector ueBfv = std::make_pair(rxW, OMNI_BEAM_ID);

    uint16_t numRows = static_cast<uint16_t>(txNumRows);
    for (double txTheta = 60; txTheta < 121; txTheta = txTheta + m_beamSearchAngleStep)
    {
        for (uint16_t txSector = 0; txSector <= numRows; txSector++)
        {
            NS_ASSERT(txSector < UINT16_MAX);

            gnbSpectrumPhy->GetBeamManager()->SetSector(txSector, txTheta);
            PhasedArrayModel::ComplexVector txW =
                gnbSpectrumPhy->GetBeamManager()->GetCurrentBeamformingVector();

            NS_ABORT_MSG_IF(txW.GetSize() == 0 || rxW.GetSize() == 0,
                            "Beamforming vectors must be initialized in order to calculate the "
                            "long term matrix.");
            Ptr<SpectrumValue> rxPsd = txThreeGppSpectrumPropModel->CalcRxPowerSpectralDensity(
                fakeParams,
                gnbSpectrumPhy->GetMobility(),
                ueSpectrumPhy->GetMobility(),
                gnbSpectrumPhy->GetAntenna()->GetObject<PhasedArrayModel>(),
                ueSpectrumPhy->GetAntenna()->GetObject<PhasedArrayModel>());

            size_t nbands = rxPsd->GetSpectrumModel()->GetNumBands();
            double power = Sum(*rxPsd) / nbands;

            NS_LOG_LOGIC(" Rx power: "
                         << power << "txTheta " << txTheta << " tx sector "
                         << (M_PI * static_cast<double>(txSector) / static_cast<double>(txNumRows) -
                             0.5 * M_PI) /
                                (M_PI)*180);

            if (max < power)
            {
                max = power;
                maxTxSector = txSector;
                maxTxTheta = txTheta;
                maxTxW = txW;
            }
        }
    }

    BeamformingVector gnbBfv =
        BeamformingVector(std::make_pair(maxTxW, BeamId(maxTxSector, maxTxTheta)));

    NS_LOG_DEBUG(
        "Beamforming vectors for gNB with node id: "
        << gnbSpectrumPhy->GetMobility()->GetObject<Node>()->GetId()
        << " and UE with node id: " << ueSpectrumPhy->GetMobility()->GetObject<Node>()->GetId()
        << " are txTheta " << maxTxTheta << " tx sector "
        << (M_PI * static_cast<double>(maxTxSector) / static_cast<double>(txNumRows) - 0.5 * M_PI) /
               (M_PI)*180);

    return BeamformingVectorPair(std::make_pair(gnbBfv, ueBfv));
}

TypeId
DirectPathBeamforming::GetTypeId()
{
    static TypeId tid = TypeId("ns3::DirectPathBeamforming")
                            .SetParent<IdealBeamformingAlgorithm>()
                            .AddConstructor<DirectPathBeamforming>();
    return tid;
}

BeamformingVectorPair
DirectPathBeamforming::GetBeamformingVectors(const Ptr<NrSpectrumPhy>& gnbSpectrumPhy,
                                             const Ptr<NrSpectrumPhy>& ueSpectrumPhy) const
{
    NS_LOG_FUNCTION(this);

    Ptr<const UniformPlanarArray> gnbAntenna =
        gnbSpectrumPhy->GetAntenna()->GetObject<UniformPlanarArray>();
    Ptr<const UniformPlanarArray> ueAntenna =
        ueSpectrumPhy->GetAntenna()->GetObject<UniformPlanarArray>();

    PhasedArrayModel::ComplexVector gNbAntennaWeights =
        CreateDirectPathBfv(gnbSpectrumPhy->GetMobility(),
                            ueSpectrumPhy->GetMobility(),
                            gnbAntenna);
    // store the antenna weights
    BeamformingVector gnbBfv =
        BeamformingVector(std::make_pair(gNbAntennaWeights, BeamId::GetEmptyBeamId()));

    PhasedArrayModel::ComplexVector ueAntennaWeights =
        CreateDirectPathBfv(ueSpectrumPhy->GetMobility(), gnbSpectrumPhy->GetMobility(), ueAntenna);
    // store the antenna weights
    BeamformingVector ueBfv =
        BeamformingVector(std::make_pair(ueAntennaWeights, BeamId::GetEmptyBeamId()));

    return BeamformingVectorPair(std::make_pair(gnbBfv, ueBfv));
}

TypeId
QuasiOmniDirectPathBeamforming::GetTypeId()
{
    static TypeId tid = TypeId("ns3::QuasiOmniDirectPathBeamforming")
                            .SetParent<DirectPathBeamforming>()
                            .AddConstructor<QuasiOmniDirectPathBeamforming>();
    return tid;
}

BeamformingVectorPair
QuasiOmniDirectPathBeamforming::GetBeamformingVectors(const Ptr<NrSpectrumPhy>& gnbSpectrumPhy,
                                                      const Ptr<NrSpectrumPhy>& ueSpectrumPhy) const
{
    NS_LOG_FUNCTION(this);
    Ptr<const UniformPlanarArray> gnbAntenna =
        gnbSpectrumPhy->GetAntenna()->GetObject<UniformPlanarArray>();
    Ptr<const UniformPlanarArray> ueAntenna =
        ueSpectrumPhy->GetAntenna()->GetObject<UniformPlanarArray>();

    // configure gNb beamforming vector to be quasi omni
    UintegerValue numRows;
    UintegerValue numColumns;
    gnbAntenna->GetAttribute("NumRows", numRows);
    gnbAntenna->GetAttribute("NumColumns", numColumns);
    BeamformingVector gnbBfv =
        std::make_pair(CreateQuasiOmniBfv(numRows.Get(), numColumns.Get()), OMNI_BEAM_ID);

    // configure UE beamforming vector to be directed towards gNB
    PhasedArrayModel::ComplexVector ueAntennaWeights =
        CreateDirectPathBfv(ueSpectrumPhy->GetMobility(), gnbSpectrumPhy->GetMobility(), ueAntenna);
    // store the antenna weights
    BeamformingVector ueBfv =
        BeamformingVector(std::make_pair(ueAntennaWeights, BeamId::GetEmptyBeamId()));

    return BeamformingVectorPair(std::make_pair(gnbBfv, ueBfv));
}

TypeId
DirectPathQuasiOmniBeamforming::GetTypeId()
{
    static TypeId tid = TypeId("ns3::DirectPathQuasiOmniBeamforming")
                            .SetParent<DirectPathBeamforming>()
                            .AddConstructor<DirectPathQuasiOmniBeamforming>();
    return tid;
}

BeamformingVectorPair
DirectPathQuasiOmniBeamforming::GetBeamformingVectors(const Ptr<NrSpectrumPhy>& gnbSpectrumPhy,
                                                      const Ptr<NrSpectrumPhy>& ueSpectrumPhy) const
{
    NS_LOG_FUNCTION(this);
    Ptr<const UniformPlanarArray> gnbAntenna =
        gnbSpectrumPhy->GetAntenna()->GetObject<UniformPlanarArray>();
    Ptr<const UniformPlanarArray> ueAntenna =
        ueSpectrumPhy->GetAntenna()->GetObject<UniformPlanarArray>();

    // configure ue beamforming vector to be quasi omni
    UintegerValue numRows;
    UintegerValue numColumns;
    ueAntenna->GetAttribute("NumRows", numRows);
    ueAntenna->GetAttribute("NumColumns", numColumns);
    BeamformingVector ueBfv =
        std::make_pair(CreateQuasiOmniBfv(numRows.Get(), numColumns.Get()), OMNI_BEAM_ID);

    // configure gNB beamforming vector to be directed towards UE
    PhasedArrayModel::ComplexVector gnbAntennaWeights =
        CreateDirectPathBfv(gnbSpectrumPhy->GetMobility(),
                            ueSpectrumPhy->GetMobility(),
                            gnbAntenna);
    // store the antenna weights
    BeamformingVector gnbBfv =
        BeamformingVector(std::make_pair(gnbAntennaWeights, BeamId::GetEmptyBeamId()));

    return BeamformingVectorPair(std::make_pair(gnbBfv, ueBfv));
}

TypeId
OptimalCovMatrixBeamforming::GetTypeId()
{
    static TypeId tid = TypeId("ns3::OptimalCovMatrixBeamforming")
                            .SetParent<IdealBeamformingAlgorithm>()
                            .AddConstructor<OptimalCovMatrixBeamforming>();

    return tid;
}

BeamformingVectorPair
OptimalCovMatrixBeamforming::GetBeamformingVectors(
    [[maybe_unused]] const Ptr<NrSpectrumPhy>& gnbSpectrumPhy,
    [[maybe_unused]] const Ptr<NrSpectrumPhy>& ueSpectrumPhy) const
{
    NS_LOG_FUNCTION(this);
    return BeamformingVectorPair();
}

} // namespace ns3
