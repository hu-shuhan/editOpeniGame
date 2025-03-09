//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CoreFESystemC_h
#define ads_CoreFESystemC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment FESystem of the latest level of form Core */

/** Type representing a bulk implementation for compact element matrix. */
#define ads_BulkElementMatrix (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 0))

/** Integer distribution to store indices of the element matrices. */
#define ads_BulkElementMatrix_indices (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 1))

/** Relational distribution to store location of the element matrix within indices and values data. */
#define ads_BulkElementMatrix_location (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 2))

/** Distribution to store values of the element matrices. */
#define ads_BulkElementMatrix_values (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 3))

/** A record representing a load case. */
#define ads_Case (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 4))

/** A collection of load cases */
#define ads_CaseCollection (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 5))

#define ads_DofCaseGrid (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 6))

#define ads_DofModeComplexNumberPartGrid (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 7))

/** The grid to be used in element-by-element loads output of Generic System. */
#define ads_ElementDofCaseGrid (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 8))

/** The grid to be used in element-by-element matrix output of Generic System. */
#define ads_ElementDofDofGrid (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 9))

/** The grid to be used for storing data of the element operators. */
#define ads_ElementMatrixEntryIndexGrid (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 10))

/** Base type for finite element systems. FE systems store computation results of modeling tasks. */
#define ads_FESystem (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 11))

/** System load cases. */
#define ads_FESystem_cases (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 12))

/** Any collection sets needed for systems. */
#define ads_FESystem_commonSets (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 13))

/** A collection of modes in a system. */
#define ads_FESystem_modeCollection (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 14))

/** Set identifying the solver dof order. The size of the set is equal or less than the number of system dofs. */
#define ads_FESystem_solverDofOrdering (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 15))

/** User specific matrices by names. */
#define ads_FESystem_userMatrices (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 16))

/** GenericSystem stores system matrices for a model. The matrices can be generated in different ways, for example with a MatrixGenerationTask. */
#define ads_GenericSystem (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 17))

/** System element load matrices - imaginary complex part. */
#define ads_GenericSystem_bulkElementImaginaryLoad (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 18))

/** System element load matrices. */
#define ads_GenericSystem_bulkElementLoad (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 19))

/** System element mass matrices. */
#define ads_GenericSystem_bulkElementMass (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 20))

/** Element operators by name for specific workflows. */
#define ads_GenericSystem_bulkElementOperators (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 21))

/** System element stiffness matrices. */
#define ads_GenericSystem_bulkElementStiffness (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 22))

/** System element structural damping matrices. */
#define ads_GenericSystem_bulkElementStructuralDamping (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 23))

/** System element structural viscous matrices. */
#define ads_GenericSystem_bulkElementViscousDamping (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 24))

/** System element load matrices - imaginary complex part. */
#define ads_GenericSystem_elementImaginaryLoad (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 25))

/** System element load matrices. */
#define ads_GenericSystem_elementLoad (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 26))

/** System element mass matrices. */
#define ads_GenericSystem_elementMass (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 27))

/** System element stiffness matrices. */
#define ads_GenericSystem_elementStiffness (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 28))

/** System element structural damping matrices. */
#define ads_GenericSystem_elementStructuralDamping (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 29))

/** System element viscous damping matrices. */
#define ads_GenericSystem_elementViscousDamping (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 30))

/** System load matrix - imaginary complex part. */
#define ads_GenericSystem_imaginaryLoad (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 31))

/** A list of nodes to be used as connection points (interface). */
#define ads_GenericSystem_interfaceNodes (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 32))

/** System load matrix. */
#define ads_GenericSystem_load (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 33))

/** System mass matrix. */
#define ads_GenericSystem_mass (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 34))

/** System stiffness matrix. */
#define ads_GenericSystem_stiffness (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 35))

/** System structural damping matrix. */
#define ads_GenericSystem_structuralDamping (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 36))

/** System viscous damping matrix. */
#define ads_GenericSystem_viscousDamping (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 37))

/** Matrix Entry Index collection for system element operators (matrices). */
#define ads_GlobalCollections_matrixEntryIndexCollection (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 38))

/** Type representing an index for compact storage of sparse data. */
#define ads_MatrixEntryIndex (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 39))

/** A collection of matrix entry indices. */
#define ads_MatrixEntryIndexCollection (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 40))

/** Relational distribution to store domains of the element operators. */
#define ads_Mesh_elementDomains (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 41))

/** Modal representation of a system. Examples of tasks that can generate a modal system: FrequencyTask, ComplexFrequencyTask, and BuckleTask. */
#define ads_ModalSystem (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 42))

/** Modal acoustic-structural coupling matrix. */
#define ads_ModalSystem_acousticCoupling (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 43))

/** Dof-based acoustic-structural coupling matrix. */
#define ads_ModalSystem_acousticDofCoupling (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 44))

/** Set identifying the acoustic modes. */
#define ads_ModalSystem_acousticModes (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 45))

/** Acoustic modal viscous damping matrix. */
#define ads_ModalSystem_acousticViscousDamping (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 46))

/** Damping ratio of compex modes. */
#define ads_ModalSystem_complexDampingRatio (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 47))

/** Mass-weighted composite modal damping matrix. */
#define ads_ModalSystem_compositeDampingMass (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 48))

/** Stiffness-weighted composite modal damping matrix. */
#define ads_ModalSystem_compositeDampingStiffness (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 49))

/** Effective mass matrix. */
#define ads_ModalSystem_effectiveMass (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 50))

/** A distribution which stores the Eigenvalue sensitivities. This distribution maps {mode,parameter,complexNumberPart} --> double VALUE, where VALUE is defined to be: (del eigenvalue_mode / del parameter) (take given complexNumberPart). */
#define ads_ModalSystem_eigenvalueSensitivities (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 51))

/** The eigenvalues of the modal system. */
#define ads_ModalSystem_eigenvalues (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 52))

/** Generalized mass matrix. */
#define ads_ModalSystem_generalizedMass (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 53))

/** Mode shapes imaginary part. */
#define ads_ModalSystem_imaginaryModeShapes (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 54))

/** Mode shapes (real). */
#define ads_ModalSystem_modeShapes (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 55))

/** In the cyclic symmetric structure, for each natural frequency the number of nodal diameters is reported. Nodal diameter is a diameter along which all displacement solutions are equal to zero. */
#define ads_ModalSystem_nodalDiameters (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 56))

/** Matrix of participation factors. */
#define ads_ModalSystem_participationFactors (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 57))

/** Projected mode shapes imaginary part. */
#define ads_ModalSystem_projectedImaginaryModeShapes (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 58))

/** Projected mode shapes (real). */
#define ads_ModalSystem_projectedModeShapes (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 59))

/** Set identifying the residual modes. */
#define ads_ModalSystem_residualModes (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 60))

/** Set identifying the singular acoustic modes. */
#define ads_ModalSystem_singularAcousticModes (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 61))

/** Stiffness matrix. */
#define ads_ModalSystem_stiffness (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 62))

/** Modal structural damping matrix. */
#define ads_ModalSystem_structuralDamping (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 63))

/** Set identifying the structural modes. */
#define ads_ModalSystem_structuralModes (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 64))

/** Modal viscous damping matrix. */
#define ads_ModalSystem_viscousDamping (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 65))

#define ads_ModeModeComplexNumberPartGrid (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 66))

/** Used for PSD matrix of generalized responses in Random Response */
#define ads_ModeModeFrequencyComplexNumberPartGrid (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 67))

/** Used for variance matrix of generalized responses in Random Response */
#define ads_ModeModeFrequencyGrid (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 68))

#define ads_ModeSpectrumGrid (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 69))

/** Collection of load cases. Load cases are used as dimensions in distributions and thus they need a collection. Tasks will have their own associations with Case records used in those tasks. */
#define ads_Model_caseCollection (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 70))

/** FE systems of the model: Generic, Modal, Substructure */
#define ads_Model_systems (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 71))

/** The grid to be used in acoustic contribution factors output of Modal Event. */
#define ads_NodeFreqComplexCaseGrid (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 72))

/** The grid to be used in acoustic contribution factors output of Modal Event. */
#define ads_NodeFreqModeComplexCaseGrid (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 73))

/** The grid to be used in acoustic contribution factors output of Modal Event. */
#define ads_NodeFreqNodeComplexCaseGrid (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 74))

/** Anchor for response data for linear dynamics steps. */
#define ads_Response (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 75))

/** Acoustic contribution factors of GRID type representing portions of the acoustic response. */
#define ads_Response_acfGrid (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 76))

/** Acoustic contribution factors of LOAD type representing portions of the acoustic response. */
#define ads_Response_acfLoad (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 77))

/** Acoustic contribution factors of MODAL ACOUSTIC type representing portions of the acoustic response. */
#define ads_Response_acfModalAcoustic (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 78))

/** Acoustic contribution factors of MODAL LOAD type representing portions of the acoustic response. */
#define ads_Response_acfModalLoad (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 79))

/** Acoustic contribution factors of MODAL STRUCTURAL type representing portions of the acoustic response. */
#define ads_Response_acfModalStructural (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 80))

/** Acoustic contribution factors of PANEL type representing portions of the acoustic response. */
#define ads_Response_acfPanel (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 81))

/** Discrete frequency (harmonic) or time (transient) values. Grid types: Time; Frequency. */
#define ads_Response_frameValues (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 82))

/** Generalized variables: GU, GV, GA, etc. Grid types: Mode x Frequency x Re/Im x Case; Mode x Time; Mode x Spectrum; Mode x Mode x Frequency. */
#define ads_Response_generalizedVariables (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 83))

/** Modal energy: KE, SE, Work. Grid types: Mode x Frequency x Re/Im x Case; Mode x Time. */
#define ads_Response_modalEnergy (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 84))

/** Physical variables (field): U, V, A, RF, etc. Grid types: Dof x Frequency x Re/Im x Case; Dof x Time; Dof. */
#define ads_Response_nodalDofField (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 85))

/** Physical variables (history): U, V, A, RF, etc. Grid types: Dof x Frequency x Re/Im x Case; Dof x Time; Dof. */
#define ads_Response_nodalDofHistory (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 86))

/** Whole model energy: AllKE, AllSE, AllWork. Grid types: Frequency x Re/Im x Case; Time. */
#define ads_Response_wholeModelEnergy (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 87))

/** Response data for linear dynamics steps. */
#define ads_Step_Lin_response (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 88))

/** SubstructureSystem stores matrices for a substructure. */
#define ads_SubstructureSystem (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 89))

/** The list of active system nodal dofs including solver dofs and slave MPC dofs. */
#define ads_SubstructureSystem_activeNodalDofs (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 90))

/** Coordinates of the center of mass */
#define ads_SubstructureSystem_centerOfMass (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 91))

/** A list of elements to define a coarse substructure display representation. */
#define ads_SubstructureSystem_displayElements (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 92))

/** A set of nodes representing dynamic modes of substructure. The nodes are presumably internal and have by one degree of freedom. */
#define ads_SubstructureSystem_dynamicModeNodes (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 93))

#define ads_SubstructureSystem_eigenvalues (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 94))

#define ads_SubstructureSystem_energy (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 95))

/** Node sets consisted of retained nodes. */
#define ads_SubstructureSystem_interfaceNodeSets (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 96))

#define ads_SubstructureSystem_load (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 97))

#define ads_SubstructureSystem_lumpedMass (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 98))

#define ads_SubstructureSystem_mass (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 99))

#define ads_SubstructureSystem_modeShapes (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 100))

/** A list of the nodes to monitor displacements. */
#define ads_SubstructureSystem_monitorNodes (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 101))

/** Element sets for output. */
#define ads_SubstructureSystem_outputElementSets (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 102))

/** Node sets for output. */
#define ads_SubstructureSystem_outputNodeSets (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 103))

#define ads_SubstructureSystem_recovery (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 104))

/** A set of retained degrees of freedom of the substructure system. */
#define ads_SubstructureSystem_retainedDofs (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 105))

/** A list of nodes retained by user (connection points). */
#define ads_SubstructureSystem_retainedNodes (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 106))

#define ads_SubstructureSystem_stiffness (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 107))

#define ads_SubstructureSystem_structuralDamping (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 108))

/** Total volume, mass vector, and rotary inertia tensor */
#define ads_SubstructureSystem_totalInertia (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 109))

#define ads_SubstructureSystem_viscousDamping (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 110))

/** Modal system generated by Frequency, ComplexFrequency, Buckle and other tasks. */
#define ads_Task_modalSystem (ads_CoreFragmentTypeIndex(ads_CoreFESystemFragment, 111))

/** 
Enum with record members. */
enum ads_BulkElementMatrixMembersEnm
{
    ads_BulkElementMatrix_loads,
    ads_BulkElementMatrix_symmetric
};

/** 
Enum with association roles. */
enum ads_BulkElementMatrix_indicesRolesEnm
{
    ads_BulkElementMatrix_indices_child,
    ads_BulkElementMatrix_indices_parent
};

/** 
Enum with association roles. */
enum ads_BulkElementMatrix_locationRolesEnm
{
    ads_BulkElementMatrix_location_child,
    ads_BulkElementMatrix_location_parent
};

/** 
Enum with association roles. */
enum ads_BulkElementMatrix_valuesRolesEnm
{
    ads_BulkElementMatrix_values_child,
    ads_BulkElementMatrix_values_parent
};

/** Enum with grid dimensions. */
enum ads_DofCaseGridDimensionsEnm
{
    ads_DofCaseGrid_case,
    ads_DofCaseGrid_dof
};

/** Enum with grid dimensions. */
enum ads_DofModeComplexNumberPartGridDimensionsEnm
{
    ads_DofModeComplexNumberPartGrid_complex,
    ads_DofModeComplexNumberPartGrid_dof,
    ads_DofModeComplexNumberPartGrid_mode
};

/** 
Enum with grid dimensions. */
enum ads_ElementDofCaseGridDimensionsEnm
{
    ads_ElementDofCaseGrid_case,
    ads_ElementDofCaseGrid_dof,
    ads_ElementDofCaseGrid_element
};

/** 
Enum with grid dimensions. */
enum ads_ElementDofDofGridDimensionsEnm
{
    ads_ElementDofDofGrid_column,
    ads_ElementDofDofGrid_element,
    ads_ElementDofDofGrid_row
};

/** 
Enum with grid dimensions. */
enum ads_ElementMatrixEntryIndexGridDimensionsEnm
{
    ads_ElementMatrixEntryIndexGrid_element,
    ads_ElementMatrixEntryIndexGrid_index
};

/** 
Enum with record members. */
enum ads_FESystemMembersEnm
{
    ads_FESystem_id
};

/** 
Enum with association roles. */
enum ads_FESystem_casesRolesEnm
{
    ads_FESystem_cases_referent,
    ads_FESystem_cases_referrer
};

/** 
Enum with association roles. */
enum ads_FESystem_commonSetsRolesEnm
{
    ads_FESystem_commonSets_referent,
    ads_FESystem_commonSets_referrer
};

/** 
Enum with association roles. */
enum ads_FESystem_modeCollectionRolesEnm
{
    ads_FESystem_modeCollection_child,
    ads_FESystem_modeCollection_parent
};

/** 
Enum with association roles. */
enum ads_FESystem_solverDofOrderingRolesEnm
{
    ads_FESystem_solverDofOrdering_referent,
    ads_FESystem_solverDofOrdering_referrer
};

/** 
Enum with association roles. */
enum ads_FESystem_userMatricesRolesEnm
{
    ads_FESystem_userMatrices_child,
    ads_FESystem_userMatrices_parent
};

/** 
Enum with record members. */
enum ads_GenericSystemMembersEnm
{
    ads_GenericSystem_id
};

/** 
Enum with association roles. */
enum ads_GenericSystem_bulkElementImaginaryLoadRolesEnm
{
    ads_GenericSystem_bulkElementImaginaryLoad_child,
    ads_GenericSystem_bulkElementImaginaryLoad_parent
};

/** 
Enum with association roles. */
enum ads_GenericSystem_bulkElementLoadRolesEnm
{
    ads_GenericSystem_bulkElementLoad_child,
    ads_GenericSystem_bulkElementLoad_parent
};

/** 
Enum with association roles. */
enum ads_GenericSystem_bulkElementMassRolesEnm
{
    ads_GenericSystem_bulkElementMass_child,
    ads_GenericSystem_bulkElementMass_parent
};

/** 
Enum with association roles. */
enum ads_GenericSystem_bulkElementOperatorsRolesEnm
{
    ads_GenericSystem_bulkElementOperators_child,
    ads_GenericSystem_bulkElementOperators_parent
};

/** 
Enum with association roles. */
enum ads_GenericSystem_bulkElementStiffnessRolesEnm
{
    ads_GenericSystem_bulkElementStiffness_child,
    ads_GenericSystem_bulkElementStiffness_parent
};

/** 
Enum with association roles. */
enum ads_GenericSystem_bulkElementStructuralDampingRolesEnm
{
    ads_GenericSystem_bulkElementStructuralDamping_child,
    ads_GenericSystem_bulkElementStructuralDamping_parent
};

/** 
Enum with association roles. */
enum ads_GenericSystem_bulkElementViscousDampingRolesEnm
{
    ads_GenericSystem_bulkElementViscousDamping_child,
    ads_GenericSystem_bulkElementViscousDamping_parent
};

/** 
Enum with association roles. */
enum ads_GenericSystem_elementImaginaryLoadRolesEnm
{
    ads_GenericSystem_elementImaginaryLoad_child,
    ads_GenericSystem_elementImaginaryLoad_parent
};

/** 
Enum with association roles. */
enum ads_GenericSystem_elementLoadRolesEnm
{
    ads_GenericSystem_elementLoad_child,
    ads_GenericSystem_elementLoad_parent
};

/** 
Enum with association roles. */
enum ads_GenericSystem_elementMassRolesEnm
{
    ads_GenericSystem_elementMass_child,
    ads_GenericSystem_elementMass_parent
};

/** 
Enum with association roles. */
enum ads_GenericSystem_elementStiffnessRolesEnm
{
    ads_GenericSystem_elementStiffness_child,
    ads_GenericSystem_elementStiffness_parent
};

/** 
Enum with association roles. */
enum ads_GenericSystem_elementStructuralDampingRolesEnm
{
    ads_GenericSystem_elementStructuralDamping_child,
    ads_GenericSystem_elementStructuralDamping_parent
};

/** 
Enum with association roles. */
enum ads_GenericSystem_elementViscousDampingRolesEnm
{
    ads_GenericSystem_elementViscousDamping_child,
    ads_GenericSystem_elementViscousDamping_parent
};

/** 
Enum with association roles. */
enum ads_GenericSystem_imaginaryLoadRolesEnm
{
    ads_GenericSystem_imaginaryLoad_child,
    ads_GenericSystem_imaginaryLoad_parent
};

/** 
Enum with association roles. */
enum ads_GenericSystem_interfaceNodesRolesEnm
{
    ads_GenericSystem_interfaceNodes_referent,
    ads_GenericSystem_interfaceNodes_referrer
};

/** 
Enum with association roles. */
enum ads_GenericSystem_loadRolesEnm
{
    ads_GenericSystem_load_child,
    ads_GenericSystem_load_parent
};

/** 
Enum with association roles. */
enum ads_GenericSystem_massRolesEnm
{
    ads_GenericSystem_mass_child,
    ads_GenericSystem_mass_parent
};

/** 
Enum with association roles. */
enum ads_GenericSystem_stiffnessRolesEnm
{
    ads_GenericSystem_stiffness_child,
    ads_GenericSystem_stiffness_parent
};

/** 
Enum with association roles. */
enum ads_GenericSystem_structuralDampingRolesEnm
{
    ads_GenericSystem_structuralDamping_child,
    ads_GenericSystem_structuralDamping_parent
};

/** 
Enum with association roles. */
enum ads_GenericSystem_viscousDampingRolesEnm
{
    ads_GenericSystem_viscousDamping_child,
    ads_GenericSystem_viscousDamping_parent
};

/** 
Enum with association roles. */
enum ads_GlobalCollections_matrixEntryIndexCollectionRolesEnm
{
    ads_GlobalCollections_matrixEntryIndexCollection_child,
    ads_GlobalCollections_matrixEntryIndexCollection_parent
};

/** 
Enum with association roles. */
enum ads_Mesh_elementDomainsRolesEnm
{
    ads_Mesh_elementDomains_child,
    ads_Mesh_elementDomains_parent
};

/** 
Enum with record members. */
enum ads_ModalSystemMembersEnm
{
    ads_ModalSystem_id,
    ads_ModalSystem_extraSystem,
    ads_ModalSystem_numberSectors
};

/** 
Enum with association roles. */
enum ads_ModalSystem_acousticCouplingRolesEnm
{
    ads_ModalSystem_acousticCoupling_child,
    ads_ModalSystem_acousticCoupling_parent
};

/** 
Enum with association roles. */
enum ads_ModalSystem_acousticDofCouplingRolesEnm
{
    ads_ModalSystem_acousticDofCoupling_child,
    ads_ModalSystem_acousticDofCoupling_parent
};

/** 
Enum with association roles. */
enum ads_ModalSystem_acousticModesRolesEnm
{
    ads_ModalSystem_acousticModes_referent,
    ads_ModalSystem_acousticModes_referrer
};

/** 
Enum with association roles. */
enum ads_ModalSystem_acousticViscousDampingRolesEnm
{
    ads_ModalSystem_acousticViscousDamping_child,
    ads_ModalSystem_acousticViscousDamping_parent
};

/** 
Enum with association roles. */
enum ads_ModalSystem_complexDampingRatioRolesEnm
{
    ads_ModalSystem_complexDampingRatio_child,
    ads_ModalSystem_complexDampingRatio_parent
};

/** 
Enum with association roles. */
enum ads_ModalSystem_compositeDampingMassRolesEnm
{
    ads_ModalSystem_compositeDampingMass_child,
    ads_ModalSystem_compositeDampingMass_parent
};

/** 
Enum with association roles. */
enum ads_ModalSystem_compositeDampingStiffnessRolesEnm
{
    ads_ModalSystem_compositeDampingStiffness_child,
    ads_ModalSystem_compositeDampingStiffness_parent
};

/** 
Enum with association roles. */
enum ads_ModalSystem_effectiveMassRolesEnm
{
    ads_ModalSystem_effectiveMass_child,
    ads_ModalSystem_effectiveMass_parent
};

/** 
Enum with association roles. */
enum ads_ModalSystem_eigenvalueSensitivitiesRolesEnm
{
    ads_ModalSystem_eigenvalueSensitivities_child,
    ads_ModalSystem_eigenvalueSensitivities_parent
};

/** 
Enum with association roles. */
enum ads_ModalSystem_eigenvaluesRolesEnm
{
    ads_ModalSystem_eigenvalues_child,
    ads_ModalSystem_eigenvalues_parent
};

/** 
Enum with association roles. */
enum ads_ModalSystem_generalizedMassRolesEnm
{
    ads_ModalSystem_generalizedMass_child,
    ads_ModalSystem_generalizedMass_parent
};

/** 
Enum with association roles. */
enum ads_ModalSystem_imaginaryModeShapesRolesEnm
{
    ads_ModalSystem_imaginaryModeShapes_child,
    ads_ModalSystem_imaginaryModeShapes_parent
};

/** 
Enum with association roles. */
enum ads_ModalSystem_modeShapesRolesEnm
{
    ads_ModalSystem_modeShapes_child,
    ads_ModalSystem_modeShapes_parent
};

/** 
Enum with association roles. */
enum ads_ModalSystem_nodalDiametersRolesEnm
{
    ads_ModalSystem_nodalDiameters_child,
    ads_ModalSystem_nodalDiameters_parent
};

/** 
Enum with association roles. */
enum ads_ModalSystem_participationFactorsRolesEnm
{
    ads_ModalSystem_participationFactors_child,
    ads_ModalSystem_participationFactors_parent
};

/** 
Enum with association roles. */
enum ads_ModalSystem_projectedImaginaryModeShapesRolesEnm
{
    ads_ModalSystem_projectedImaginaryModeShapes_child,
    ads_ModalSystem_projectedImaginaryModeShapes_parent
};

/** 
Enum with association roles. */
enum ads_ModalSystem_projectedModeShapesRolesEnm
{
    ads_ModalSystem_projectedModeShapes_child,
    ads_ModalSystem_projectedModeShapes_parent
};

/** 
Enum with association roles. */
enum ads_ModalSystem_residualModesRolesEnm
{
    ads_ModalSystem_residualModes_referent,
    ads_ModalSystem_residualModes_referrer
};

/** 
Enum with association roles. */
enum ads_ModalSystem_singularAcousticModesRolesEnm
{
    ads_ModalSystem_singularAcousticModes_referent,
    ads_ModalSystem_singularAcousticModes_referrer
};

/** 
Enum with association roles. */
enum ads_ModalSystem_stiffnessRolesEnm
{
    ads_ModalSystem_stiffness_child,
    ads_ModalSystem_stiffness_parent
};

/** 
Enum with association roles. */
enum ads_ModalSystem_structuralDampingRolesEnm
{
    ads_ModalSystem_structuralDamping_child,
    ads_ModalSystem_structuralDamping_parent
};

/** 
Enum with association roles. */
enum ads_ModalSystem_structuralModesRolesEnm
{
    ads_ModalSystem_structuralModes_referent,
    ads_ModalSystem_structuralModes_referrer
};

/** 
Enum with association roles. */
enum ads_ModalSystem_viscousDampingRolesEnm
{
    ads_ModalSystem_viscousDamping_child,
    ads_ModalSystem_viscousDamping_parent
};

/** Enum with grid dimensions. */
enum ads_ModeModeComplexNumberPartGridDimensionsEnm
{
    ads_ModeModeComplexNumberPartGrid_column,
    ads_ModeModeComplexNumberPartGrid_complex,
    ads_ModeModeComplexNumberPartGrid_row
};

/** 
Enum with grid dimensions. */
enum ads_ModeModeFrequencyComplexNumberPartGridDimensionsEnm
{
    ads_ModeModeFrequencyComplexNumberPartGrid_column,
    ads_ModeModeFrequencyComplexNumberPartGrid_complex,
    ads_ModeModeFrequencyComplexNumberPartGrid_frequency,
    ads_ModeModeFrequencyComplexNumberPartGrid_row
};

/** 
Enum with grid dimensions. */
enum ads_ModeModeFrequencyGridDimensionsEnm
{
    ads_ModeModeFrequencyGrid_column,
    ads_ModeModeFrequencyGrid_frequency,
    ads_ModeModeFrequencyGrid_row
};

/** Enum with grid dimensions. */
enum ads_ModeSpectrumGridDimensionsEnm
{
    ads_ModeSpectrumGrid_column,
    ads_ModeSpectrumGrid_row
};

/** 
Enum with association roles. */
enum ads_Model_caseCollectionRolesEnm
{
    ads_Model_caseCollection_child,
    ads_Model_caseCollection_parent
};

/** 
Enum with association roles. */
enum ads_Model_systemsRolesEnm
{
    ads_Model_systems_child,
    ads_Model_systems_parent
};

/** 
Enum with grid dimensions. */
enum ads_NodeFreqComplexCaseGridDimensionsEnm
{
    ads_NodeFreqComplexCaseGrid_case,
    ads_NodeFreqComplexCaseGrid_complex,
    ads_NodeFreqComplexCaseGrid_frequency,
    ads_NodeFreqComplexCaseGrid_node
};

/** 
Enum with grid dimensions. */
enum ads_NodeFreqModeComplexCaseGridDimensionsEnm
{
    ads_NodeFreqModeComplexCaseGrid_case,
    ads_NodeFreqModeComplexCaseGrid_complex,
    ads_NodeFreqModeComplexCaseGrid_frequency,
    ads_NodeFreqModeComplexCaseGrid_mode,
    ads_NodeFreqModeComplexCaseGrid_node
};

/** 
Enum with grid dimensions. */
enum ads_NodeFreqNodeComplexCaseGridDimensionsEnm
{
    ads_NodeFreqNodeComplexCaseGrid_acousticNode,
    ads_NodeFreqNodeComplexCaseGrid_case,
    ads_NodeFreqNodeComplexCaseGrid_complex,
    ads_NodeFreqNodeComplexCaseGrid_frequency,
    ads_NodeFreqNodeComplexCaseGrid_structuralNode
};

/** 
Enum with association roles. */
enum ads_Response_acfGridRolesEnm
{
    ads_Response_acfGrid_child,
    ads_Response_acfGrid_parent
};

/** 
Enum with association roles. */
enum ads_Response_acfLoadRolesEnm
{
    ads_Response_acfLoad_child,
    ads_Response_acfLoad_parent
};

/** 
Enum with association roles. */
enum ads_Response_acfModalAcousticRolesEnm
{
    ads_Response_acfModalAcoustic_child,
    ads_Response_acfModalAcoustic_parent
};

/** 
Enum with association roles. */
enum ads_Response_acfModalLoadRolesEnm
{
    ads_Response_acfModalLoad_child,
    ads_Response_acfModalLoad_parent
};

/** 
Enum with association roles. */
enum ads_Response_acfModalStructuralRolesEnm
{
    ads_Response_acfModalStructural_child,
    ads_Response_acfModalStructural_parent
};

/** 
Enum with association roles. */
enum ads_Response_acfPanelRolesEnm
{
    ads_Response_acfPanel_child,
    ads_Response_acfPanel_parent
};

/** 
Enum with association roles. */
enum ads_Response_frameValuesRolesEnm
{
    ads_Response_frameValues_child,
    ads_Response_frameValues_parent
};

/** 
Enum with association roles. */
enum ads_Response_generalizedVariablesRolesEnm
{
    ads_Response_generalizedVariables_child,
    ads_Response_generalizedVariables_parent
};

/** 
Enum with association roles. */
enum ads_Response_modalEnergyRolesEnm
{
    ads_Response_modalEnergy_child,
    ads_Response_modalEnergy_parent
};

/** 
Enum with association roles. */
enum ads_Response_nodalDofFieldRolesEnm
{
    ads_Response_nodalDofField_child,
    ads_Response_nodalDofField_parent
};

/** 
Enum with association roles. */
enum ads_Response_nodalDofHistoryRolesEnm
{
    ads_Response_nodalDofHistory_child,
    ads_Response_nodalDofHistory_parent
};

/** 
Enum with association roles. */
enum ads_Response_wholeModelEnergyRolesEnm
{
    ads_Response_wholeModelEnergy_child,
    ads_Response_wholeModelEnergy_parent
};

/** 
Enum with association roles. */
enum ads_Step_Lin_responseRolesEnm
{
    ads_Step_Lin_response_child,
    ads_Step_Lin_response_parent
};

/** 
Enum with record members. */
enum ads_SubstructureSystemMembersEnm
{
    ads_SubstructureSystem_id,
    ads_SubstructureSystem_flexibleBody,
    ads_SubstructureSystem_largePreload
};

enum ads_SubstructureSystem_flexibleBodyEnm
{
    ads_SubstructureSystem_flexibleBody_ADAMS,
    ads_SubstructureSystem_flexibleBody_EXCITE,
    ads_SubstructureSystem_flexibleBody_GENERIC,
    ads_SubstructureSystem_flexibleBody_NONE,
    ads_SubstructureSystem_flexibleBody_SID,
    ads_SubstructureSystem_flexibleBody_SIMPACK
};

/** 
Enum with association roles. */
enum ads_SubstructureSystem_activeNodalDofsRolesEnm
{
    ads_SubstructureSystem_activeNodalDofs_referent,
    ads_SubstructureSystem_activeNodalDofs_referrer
};

/** 
Enum with association roles. */
enum ads_SubstructureSystem_centerOfMassRolesEnm
{
    ads_SubstructureSystem_centerOfMass_child,
    ads_SubstructureSystem_centerOfMass_parent
};

/** 
Enum with association roles. */
enum ads_SubstructureSystem_displayElementsRolesEnm
{
    ads_SubstructureSystem_displayElements_referent,
    ads_SubstructureSystem_displayElements_referrer
};

/** 
Enum with association roles. */
enum ads_SubstructureSystem_dynamicModeNodesRolesEnm
{
    ads_SubstructureSystem_dynamicModeNodes_referent,
    ads_SubstructureSystem_dynamicModeNodes_referrer
};

/** Enum with association roles. */
enum ads_SubstructureSystem_eigenvaluesRolesEnm
{
    ads_SubstructureSystem_eigenvalues_child,
    ads_SubstructureSystem_eigenvalues_parent
};

/** Enum with association roles. */
enum ads_SubstructureSystem_energyRolesEnm
{
    ads_SubstructureSystem_energy_child,
    ads_SubstructureSystem_energy_parent
};

/** 
Enum with association roles. */
enum ads_SubstructureSystem_interfaceNodeSetsRolesEnm
{
    ads_SubstructureSystem_interfaceNodeSets_referent,
    ads_SubstructureSystem_interfaceNodeSets_referrer
};

/** Enum with association roles. */
enum ads_SubstructureSystem_loadRolesEnm
{
    ads_SubstructureSystem_load_child,
    ads_SubstructureSystem_load_parent
};

/** Enum with association roles. */
enum ads_SubstructureSystem_lumpedMassRolesEnm
{
    ads_SubstructureSystem_lumpedMass_child,
    ads_SubstructureSystem_lumpedMass_parent
};

/** Enum with association roles. */
enum ads_SubstructureSystem_massRolesEnm
{
    ads_SubstructureSystem_mass_child,
    ads_SubstructureSystem_mass_parent
};

/** Enum with association roles. */
enum ads_SubstructureSystem_modeShapesRolesEnm
{
    ads_SubstructureSystem_modeShapes_child,
    ads_SubstructureSystem_modeShapes_parent
};

/** 
Enum with association roles. */
enum ads_SubstructureSystem_monitorNodesRolesEnm
{
    ads_SubstructureSystem_monitorNodes_referent,
    ads_SubstructureSystem_monitorNodes_referrer
};

/** 
Enum with association roles. */
enum ads_SubstructureSystem_outputElementSetsRolesEnm
{
    ads_SubstructureSystem_outputElementSets_referent,
    ads_SubstructureSystem_outputElementSets_referrer
};

/** 
Enum with association roles. */
enum ads_SubstructureSystem_outputNodeSetsRolesEnm
{
    ads_SubstructureSystem_outputNodeSets_referent,
    ads_SubstructureSystem_outputNodeSets_referrer
};

/** Enum with association roles. */
enum ads_SubstructureSystem_recoveryRolesEnm
{
    ads_SubstructureSystem_recovery_child,
    ads_SubstructureSystem_recovery_parent
};

/** 
Enum with association roles. */
enum ads_SubstructureSystem_retainedDofsRolesEnm
{
    ads_SubstructureSystem_retainedDofs_referent,
    ads_SubstructureSystem_retainedDofs_referrer
};

/** 
Enum with association roles. */
enum ads_SubstructureSystem_retainedNodesRolesEnm
{
    ads_SubstructureSystem_retainedNodes_referent,
    ads_SubstructureSystem_retainedNodes_referrer
};

/** Enum with association roles. */
enum ads_SubstructureSystem_stiffnessRolesEnm
{
    ads_SubstructureSystem_stiffness_child,
    ads_SubstructureSystem_stiffness_parent
};

/** Enum with association roles. */
enum ads_SubstructureSystem_structuralDampingRolesEnm
{
    ads_SubstructureSystem_structuralDamping_child,
    ads_SubstructureSystem_structuralDamping_parent
};

/** 
Enum with association roles. */
enum ads_SubstructureSystem_totalInertiaRolesEnm
{
    ads_SubstructureSystem_totalInertia_child,
    ads_SubstructureSystem_totalInertia_parent
};

/** Enum with association roles. */
enum ads_SubstructureSystem_viscousDampingRolesEnm
{
    ads_SubstructureSystem_viscousDamping_child,
    ads_SubstructureSystem_viscousDamping_parent
};

/** 
Enum with association roles. */
enum ads_Task_modalSystemRolesEnm
{
    ads_Task_modalSystem_referent,
    ads_Task_modalSystem_referrer
};

#endif
