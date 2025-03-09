//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
// ODB API Legal Notice
//
// This software is protected by international copyright laws and treaties. 
// You may use or reproduce this software only if you do so under a license
// from ABAQUS, Inc. and only in accordance with the terms of such license.
//
// ?2000-2006 ABAQUS, Inc. All rights reserved.
//
// U.S. GOVERNMENT USERS: The ABAQUS Software and its documentation are 
// "commercial items," specifically "commercial computer software" and 
// "commercial computer software documentation" and, consistent with 
// FAR 12.212 and DFARS 227.7202, as applicable, are provided with
// restricted rights in accordance with license terms.
//
// TRADEMARKS
// The trademarks and service marks ("trademarks") in this package are the
// property of ABAQUS, Inc. or third parties. You are not permitted to use
// these trademarks without the prior written consent of ABAQUS, Inc. or
// such third parties.
//
// The following are trademarks or registered trademarks of ABAQUS, Inc.
// or its subsidiaries in the United States and/or other countries: ABAQUS, 
// ABAQUS/Standard, ABAQUS/Explicit, ABAQUS/CAE, ABAQUS/Viewer, ABAQUS/Aqua,
// ABAQUS/Design, ABAQUS/Foundation, ABAQUS/AMS, ABAQUS for CATIA V5, VCCT
// for ABAQUS, DDAM for ABAQUS, Unified FEA and the ABAQUS Logo.
//
// Other company, product, and service names may be trademarks or service
// marks of their respective owners. For additional information concerning
// trademarks, copyrights, and licenses, see the Legal Notices at 
// http://www.abaqus.com/products/products_legal.html.

#ifndef odb_api_h
#define odb_api_h

#include <stdio.h>

// Begin local includes
#include <odb_AccelerationBC.h>
#include <odb_Assembly.h>
#include <odb_AuxiliaryData.h>
#include <odb_AnalysisError.h>
#include <odb_AnalysisWarning.h>
#include <odb_AnalyticSurface.h>
#include <odb_Annotation.h>
#include <odb_AnnotationRepository.h>
#include <odb_Text.h>
#include <odb_Arrow.h>
#include <odb_BCRepository.h>
#include <odb_BodyHeatFlux.h>
#include <odb_ConcentratedForce.h>
#include <odb_ConcentratedHeatFlux.h>
#include <odb_ConPropAssignSequence.h>
#include <odb_CompatibilityError.h>
#include <odb_ConstraintRepository.h>
#include <odb_ContactControlsRepository.h>
#include <odb_ContactDiagnostic.h>
#include <odb_ContactProperty.h>
#include <odb_Coupling.h>
#include <odb_DatumCsys.h>
#include <odb_DatumCsysRepository.h>
#include <odb_DiagnosticAttempt.h>
#include <odb_DiagnosticData.h>
#include <odb_DiagnosticIncrement.h>
#include <odb_DiagnosticIteration.h>
#include <odb_DiagnosticStep.h>
#include <odb_DictionaryInt.h>
#include <odb_DictionaryIntInt.h>
#include <odb_DictionaryStringFloat.h>
#include <odb_DictionaryStringInt.h>
#include <odb_DisplacementBC.h>
#include <odb_DisplayBody.h>
#include <odb_DistributedBC.h>
#include <odb_DistributedLoad.h>
#include <odb_ElemCtrlParams.h>
#include <odb_Enum.h>
#include <odb_Equation.h>
#include <odb_Exceptions.h>
#include <odb_FieldOutputRequest.h>
#include <odb_FORequestRepository.h>
#include <odb_FilmCondition.h>
#include <odb_FromFile.h>
#include <odb_GeneralFieldBase.h>
#include <odb_Gravity.h>
#include <odb_HistoryOutputRequest.h>
#include <odb_HORequestRepository.h>
#include <odb_IPRepository.h>
#include <odb_Instance.h>
#include <odb_InstanceRepository.h>
#include <odb_Interaction.h>
#include <odb_InteractionProperty.h>
#include <odb_InteractionRepository.h>
#include <odb_JobTime.h>
#include <odb_Load.h>
#include <odb_LoadRepository.h>
#include <odb_MassScaling.h>
#include <odb_MathOperators.h>
#include <odb_Moment.h>
#include <odb_MPC.h>
#include <odb_NormalBehavior.h>
#include <odb_OdbRepository.h>
#include <odb_Orientations.h>
#include <odb_Part.h>
#include <odb_PartRepository.h>
#include <odb_PredefinedField.h>
#include <odb_PredefinedFieldRepository.h>
#include <odb_Pressure.h>
#include <odb_PropagatingComponent.h>
#include <odb_Residual.h>
#include <odb_RestartRequest.h>
#include <odb_NodalDofs.h>
#include <odb_RegionPairSequence.h>
#include <odb_RotationalBodyForce.h>
#include <odb_RigidBody.h>
#include <odb_ScalarLoad.h>
#include <odb_Section.h>
#include <odb_SectionAssignment.h>
#include <odb_SectionCategory.h>
#include <odb_SectionCategoryRepository.h>
#include <odb_SectorDefinition.h>
#include <odb_SequenceAnalysisError.h>
#include <odb_SequenceAnalysisWarning.h>
#include <odb_SequenceAuxiliaryData.h>
#include <odb_SequenceCompatibilityError.h>
#include <odb_SequenceDiagnosticAttempt.h>
#include <odb_SequenceDiagnosticIncrement.h>
#include <odb_SequenceDiagnosticIteration.h>
#include <odb_SequenceDiagnosticStep.h>
#include <odb_SequenceResidual.h>
#include <odb_SequenceRigidBody.h>
#include <odb_SequenceSectionAssignment.h>
#include <odb_ShellSolidCoupling.h>
#include <odb_Step.h>
#include <odb_StepRepository.h>
#include <odb_SubmodelBC.h>
#include <odb_SurfaceHeatFlux.h>
#include <odb_SurfaceToSurfaceContactStd.h>
#include <odb_SurfaceToSurfaceContactExp.h>
#include <odb_TangentialBehavior.h>
#include <odb_Temperature.h>
#include <odb_Tie.h>
#include <odb_Velocity.h>
#include <odb_TemperatureBC.h>
#include <odb_ThermalConductance.h>
#include <odb_Types.h>
#include <odb_TypeIdentifier.h>
#include <odb_VectorLoad.h>
#include <odb_VelocityBC.h>
#include <odb_WipReporter.h>
#include <odb_Timer.h>
extern "C" void odb_initializeAPI();
extern "C" void odb_finalizeAPI();
// End local includes

#endif // odb_api_h
