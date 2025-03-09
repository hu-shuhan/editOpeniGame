//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_SectionTypes_h
#define odb_SectionTypes_h

// Begin local includes
#include <odb_Section.h>
#include <kseO_ShellSection.h>
#include <kseO_ConnectorSection.h>
#include <kseO_BeamSection.h>
#include <kseO_GeometryShellSection.h>
#include <kseO_HomogeneousFluidSection.h>
#include <kseO_HomogeneousShellSection.h>
#include <kseO_HomogeneousSolidSection.h>
#include <kseO_CompositeShellSection.h>
#include <kseO_GasketSection.h>
#include <kseO_MembraneSection.h>
#include <kseO_TrussSection.h>
#include <kseO_CohesiveSection.h>
#include <kseO_AcousticInfiniteSection.h>
#include <kseO_AcousticInterfaceSection.h>
#include <kseO_TransverseShearShell.h>
#include <kseO_TransverseShearBeam.h>
#include <kseO_PEGSection.h>
#include <kseO_SurfaceSection.h>
#include <kseO_PointSection.h>
#include <kseO_DiscreteSection.h>
#include <kseO_SectionLayer.h>
#include <kseO_LayerProperties.h>
#include <kseO_RebarLayers.h>
#include <kseO_ApiExtension.h>
#include <kseO_SectionContainer.h>
#include <kseO_SequenceSectionLayer.h>
#include <kseO_SequenceLayerProperties.h>
#include <kbpO_ArbitraryProfile.h>
#include <kbpO_BoxProfile.h>
#include <kbpO_CircularProfile.h>
#include <kbpO_GeneralizedProfile.h>
#include <kbpO_HexagonalProfile.h>
#include <kbpO_IProfile.h>
#include <kbpO_LProfile.h>
#include <kbpO_PipeProfile.h>
#include <kbpO_Profile.h>
#include <kbpO_RectangularProfile.h>
#include <kbpO_TProfile.h>
#include <kbpO_TrapezoidalProfile.h>
#include <kbpO_ProfileContainer.h>
#include <kseO_GeneralStiffnessSection.h>
#include <kseO_CompositeSolidSection.h>
#include <kseO_EulerianSection.h>
#include <kseO_FluidPipeSection.h>

typedef kseO_ShellSection odb_ShellSection;
typedef kseO_BeamSection odb_BeamSection;
typedef kseO_CohesiveSection odb_CohesiveSection;
typedef kseO_GeometryShellSection odb_GeometryShellSection;
typedef kseO_HomogeneousFluidSection odb_HomogeneousFluidSection;
typedef kseO_HomogeneousShellSection odb_HomogeneousShellSection;
typedef kseO_HomogeneousSolidSection odb_HomogeneousSolidSection;
typedef kseO_CompositeShellSection odb_CompositeShellSection;
typedef kseO_GasketSection odb_GasketSection;
typedef kseO_MembraneSection odb_MembraneSection;
typedef kseO_TrussSection odb_TrussSection;
typedef kseO_TransverseShearShell odb_TransverseShearShell;
typedef kseO_TransverseShearBeam odb_TransverseShearBeam;
typedef kseO_PEGSection odb_PEGSection;
typedef kseO_SurfaceSection odb_SurfaceSection;
typedef kseO_LayerProperties odb_LayerProperties;
typedef kseO_SectionLayer odb_SectionLayer;
typedef kseO_RebarLayers odb_RebarLayers;
typedef kseO_ApiExtension odb_SectionApi;
typedef kseO_SectionContainer odb_SectionContainer;
typedef kseO_SectionContainerIT odb_SectionContainerIT;
typedef kseO_SequenceSectionLayer odb_SequenceSectionLayer;
typedef kseO_SequenceLayerProperties odb_SequenceLayerProperties;
typedef kseO_PointSection odb_PointSection;
typedef kseO_DiscreteSection odb_DiscreteSection;
typedef kbpO_ArbitraryProfile odb_ArbitraryProfile;
typedef kbpO_BoxProfile odb_BoxProfile;
typedef kbpO_CircularProfile odb_CircularProfile;
typedef kbpO_GeneralizedProfile odb_GeneralizedProfile;
typedef kbpO_HexagonalProfile odb_HexagonalProfile;
typedef kbpO_IProfile odb_IProfile;
typedef kbpO_LProfile odb_LProfile;
typedef kbpO_PipeProfile odb_PipeProfile;
typedef kbpO_Profile odb_Profile;
typedef kbpO_RectangularProfile odb_RectangularProfile;
typedef kbpO_TProfile odb_TProfile;
typedef kbpO_TrapezoidalProfile odb_TrapezoidalProfile;
typedef kbpO_ProfileContainer odb_ProfileContainer;
typedef kbpO_ProfileContainerIT odb_ProfileContainerIT;
typedef kseO_AcousticInfiniteSection odb_AcousticInfiniteSection;
typedef kseO_AcousticInterfaceSection odb_AcousticInterfaceSection;
typedef kseO_ConnectorSection odb_ConnectorSection;
typedef kseO_GeneralStiffnessSection odb_GeneralStiffnessSection;
typedef kseO_CompositeSolidSection odb_CompositeSolidSection;
typedef kseO_EulerianSection odb_EulerianSection;
typedef kseO_FluidPipeSection odb_FluidPipeSection;
#endif
