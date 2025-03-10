//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ddb_StateVariableEnums_h
#define ddb_StateVariableEnums_h

enum ddb_ElementVariableIdEnumeration
{
  ddb_STRESS,            // stress
  ddb_E_STRAIN,          // elastic strain
  ddb_P_STRAIN,          // plastic strain
  ddb_TEMPERATURE,       // temperature at the integration points

  ddb_ISO_THERM_STRAIN,  // thermal strain
  ddb_ORTHO_THERM_STRAIN,
  ddb_ANISO_THERM_STRAIN,

  ddb_EP_STRAIN,         // effective plastic strain
  ddb_MPDENSITY,         // material point density

  ddb_HG_FORCE,          // hourglass force
  ddb_DEFGRAD,           // deformation gradient
  ddb_HG_MOMENT,         // hourglass moment
  ddb_CURVATURE,         // curvature of shell or beam elements
  ddb_MIDSIDENORMAL,     // normals on shell edges
  ddb_TV_SHR_STRN_CURV,  // transverse shear strain in curvilinear coordinates
  ddb_DRILLMOMENT,       // out-of-plane drilling moment at shell nodes
  ddb_DIRECTOR,          // the local coordinate system
  ddb_THICKNESS,         // current thickness of shell or membrane elements

  ddb_ElementVariableIdCount
};
typedef enum ddb_ElementVariableIdEnumeration ddb_ElementVariableIdEnm;

enum ddb_NodeVariableIdEnumeration
{
  ddb_NodeVariableIdUndefined = 0,
  ddb_NodeVariableIdFirstValid,

  ddb_DISPLACEMENT = ddb_NodeVariableIdFirstValid,
  ddb_VELOCITY,
  ddb_ROTDISPLACEMENT,
  ddb_ROTVELOCITY,

  ddb_NodeVariableIdCount
};
typedef enum ddb_NodeVariableIdEnumeration ddb_NodeVariableIdEnm;

enum ddb_VariableDataTypeEnumeration
{
  ddb_SCALAR,
  ddb_VECTOR,
  ddb_TENSOR,
  ddb_SYMTENSOR,
  ddb_NODESCALAR,
  ddb_EDGESCALAR,
  ddb_EDGEVECTOR,
  ddb_QUATERNION,
  ddb_TENSORF32,

  ddb_VariableDataTypeCount
};
typedef enum ddb_VariableDataTypeEnumeration ddb_VariableDataTypeEnm;

enum ddb_DataTypeDimensionEnumeration
{
  ddb_3D_FULL_NUM_COMP    = 6,  /*    11,22,33,12,13,23  */
  ddb_3D_PLANAR_NUM_COMP  = 4,  /*    11,22,33,12  */
  ddb_3D_SURFACE_NUM_COMP = 3,  /*    11,22,12  */
  ddb_2D_PLANAR_NUM_COMP  = 4,  /*    11,22,33,12  */
  ddb_2D_SURFACE_NUM_COMP = 3   /*    11,22,12  */
};
typedef enum ddb_DataTypeDimensionEnumeration ddb_DataTypeDimensionEnm;


enum ddb_SectionTypeEnumeration
{
  // NOTE: Enumerants should not be removed.  They may be renamed, but
  // the position of useful enumerants should not change.
  ddb_SectionTypeUndefined = 0,
  ddb_UNDEFINED_SECTION = 0,
  ddb_UNIMPLEMENTED_SECTION = 1,

  ddb_SOLID_SECTION,

  ddb_SHELL_SECTION,
  ddb_SHELL_GENERAL_SECTION,

  ddb_BEAM_SECTION,
  ddb_BEAM_GENERAL_SECTION,

  ddb_RIGID_BODY,
   
  ddb_SectionTypeCount
};
typedef enum ddb_SectionTypeEnumeration ddb_SectionTypeEnm;

enum ddb_SectionIntEnumeration
{
  // NOTE: Enumerants should not be removed.  They may be renamed, but
  // the position of useful enumerants should not change.
  ddb_SectionIntUndefined = 0,
  ddb_NOSECTINT = 1,

  ddb_SIMPSON,

  ddb_GAUSS,

  ddb_SectionIntCount
};
typedef enum ddb_SectionIntEnumeration ddb_SectionIntEnm;


enum ddb_AnnotationTypeEnumeration
{
  // NOTE: Enumerants should not be removed.  They may be renamed, but
  // the position of useful enumerants should not change.
  ddb_TYPE_UNDEFINED = 0,
  ddb_TEXT = 1,
  ddb_ARROW
};

typedef enum ddb_AnnotationTypeEnumeration ddb_AnnotationTypeEnm;

enum ddb_ProcedureTypeEnumeration
{
  // NOTE: Enumerants should not be removed.  They may be renamed, but
  // the position of useful enumerants should not change.
  ddb_ProcedureTypeUndefined                 = 0,

  /*   the order here matches the order in the systems manual, 5.2.10  */

  ddb_Static_Stress_Auto                     = 1,  /*   *STATIC  */
  ddb_Static_Stress_Direct                   = 2,  /*   *STATIC, DIRECT  */

  ddb_Dynamic_Hilber_Auto                    = 3,  /*   *DYNAMIC  */
  ddb_Dynamic_Hilber_Direct                  = 4,  /*   *DYNAMIC, DIRECT  */
  ddb_Subspace_Dynamics                      = 5,  /*   *DYNAMIC, SUBSPACE  */
  ddb_Explicit_Auto                          = 6,  /*   *DYNAMIC, EXPLICIT  */

  ddb_Proc_Creep                             = 7,  /*   *VISCO  */
						  
  ddb_SS_Heat_Transfer                       = 8,  /*   *HEAT TRANSFER, STEADY STATE  */
  ddb_Transient_Heat_Transfer_Direct         = 9,  /*   *HEAT TRANSFER  */
  ddb_Transient_Heat_Transfer_Auto           = 10, /*   *HEAT TRANSFER, DELTAMX=__  */
						  
  ddb_SS_Mass_Diffusion                      = 11, /*   *MASS DIFFUSION, STEADY STATE  */
  ddb_Transient_Mass_Diffusion_Direct        = 12, /*   *MASS DIFFUSION  */
  ddb_Transient_Mass_Diffusion_Auto          = 13, /*   *MASS DIFFUSION, DCMAX=__  */

  ddb_Modal_Extraction                       = 14, /*   *FREQUENCY  */
  ddb_Buckle                                 = 15, /*   *BUCKLE  */
  ddb_Complex_Frequency                      = 16, /*   *COMPLEX FREQUENCY  */
  
  // 17 is still available

  ddb_Geostatic                              = 18, /*   *GEOSTATIC  */

  ddb_SS_Soil_Direct                         = 19, /*   *SOILS  */
  ddb_SS_Soil_Auto                           = 20, /*   *SOILS, CETOL=__ and/or UTOL=__  */
  ddb_Consolidation_Soil_Direct              = 21, /*   *SOILS, CONSOLIDATION  */
  ddb_Consolidation_Soil_Auto                = 22, /*   *SOILS, CONSOLIDATION, CETOL=__ and/or UTOL=__ */

  ddb_SS_Coupled_Temp_Disp                   = 23, /*   *COUPLED TEMPERATURE-DISPLACEMENT, STEADY STATE */
  ddb_Transient_Coupled_Temp_Disp_Direct     = 24, /*   *COUPLED TEMPERATURE-DISPLACEMENT  */
  ddb_Transient_Coupled_Temp_Disp_Auto       = 25, /*   *COUPLED TEMPERATURE-DISPLACEMENT, */
                                                   /*    CETOL=__  and/or DELTMX=__        */

  ddb_SS_Coupled_Thermal_Elect               = 26, /*   *COUPLED THERMAL-ELECTRICAL, STEADY STATE  */
  ddb_Transient_Coupled_Thermal_Elect_Direct = 27, /*   *COUPLED THERMAL-ELECTRICAL  */
  ddb_Transient_Coupled_Thermal_Elect_Auto   = 28, /*   *COUPLED THERMAL-ELECTRICAL, DELTMX=__  */
                                                   /*    and/or MXDEM=__  */

  ddb_USA_Added_Mass_Generation              = 29, /*   *USA ADDDED MASS GENERATION  */

  ddb_SS_Transport_Auto                      = 30, /*   *STEADY STATE TRANSPORT  */
  ddb_SS_Transport_Direct                    = 31, /*   *STEADY STATE TRANSPORT, DIRECT  */

  ddb_Response_Spectrum                      = 32, /*   *RESPONSE SPECTRUM  */
  ddb_Modal_Dynamics                         = 33, /*   *MODAL DYNAMIC  */
  ddb_Steady_State_Dynamics                  = 34, /*   *STEADY STATE DYNAMICS  */
  ddb_Random_Response                        = 35, /*   *RANDOM RESPONSE  */

  ddb_Anneal                                 = 36, /*   *ANNEAL  */

  ddb_Explicit_Coupled_Temp_Disp             = 37, /*   *DYNAMIC TEMPERATURE-DISPLACEMENT,EXPLICT  */
  
  ddb_Static_Linear_Perturbation             = 38, /*   *STEP PERTURBATION, *STATIC */
  ddb_Static_Riks                            = 39, /*   *STATIC, RIKS */
  ddb_Substructure_Generate                  = 40, /*   *SUBSTRUCTURE GENERATE */
  ddb_SS_Transport_Riks                      = 41, /*   *STEADY STATE TRANSPORT, RIKS */
  ddb_SS_Transport_Linear_Perturbation       = 42, /*   *STEP, PERTURBATION, *STEADY STATE TRANSPORT */

  ddb_Direct_Cyclic_Auto                     = 43, /*   *DIRECT CYCLIC  */
  ddb_Direct_Cyclic_Direct                   = 44, /*   *DIRECT CYCLIC, DIRECT  */

  ddb_Beam_Section_Generate                  = 45, /*   *BEAM SECTION GENERATE  */
  
  ddb_Matrix_Generate                        = 46,  /*   *MATRIX GENERATE */
  ddb_Incompressible_Navier_Stokes_Flow      = 47,  /* *CFD, INCOMPRESSIBLE NAVIER STOKES */
  ddb_Compressible_Navier_Stokes_Flow        = 48,  /* *CFD, INCOMPRESSIBLE NAVIER STOKES */
  ddb_Emag_Harmonic                          = 49,  /* *ELECTROMAGNTICS, LOW FREQUENCY, TIME HARMONIC */
  ddb_SS_Coupled_Elect_Temp_Disp              = 50, /*   *COUPLED ELECT-TEMP-DISP, STEADY STATE */
  ddb_Transient_Coupled_Elect_Temp_Disp_Direct = 51, /*   *COUPLED ELECT-TEMP-DISP  */
  ddb_Transient_Coupled_Elect_Temp_Disp_Auto   = 52, /*   *COUPLED ELECT-TEMP-DISP, */
  ddb_Emag_Magnetostatic                       = 53, /*   *MAGNETOSTATIC, direct */
  ddb_Emag_Magnetostatic_Auto                  = 54, /*   *MAGNETOSTATIC, auto */
  ddb_Emag_hfth                                = 55,  /*   *Electromagnetic, high freq, time harmonic */
  ddb_Emag_lftd_Direct                         = 56,  /*   *Electromagnetic, low freq, time domain, direct */
  ddb_Emag_lftd_Auto                           = 57,  /*   *Electromagnetic, low freq, time domain, auto */

  ddb_Fatigue_Auto                             = 58, /*   *FATIGUE  */
  ddb_Fatigue_Direct                           = 59, /*   *FATIGUE, DIRECT  */
  ddb_OneStep_Unfold                           = 60, /*   *One-StepInverse, Unfold  */

  ddb_Reduced_Basis_Generate                   = 61, /* *REDUCED BASIS GENERATE */

  ddb_ProcedureTypeCount
};
typedef enum ddb_ProcedureTypeEnumeration ddb_ProcedureTypeEnm;

enum ddb_AnalysisTypeEnumeration
{
  // NOTE: Enumerants should not be removed.  They may be renamed, but
  // the position of useful enumerants should not change.
  ddb_AnalysisTypeUndefined = 0,

  ddb_Standard,
  ddb_Explicit,
  ddb_Cfd,

  ddb_AnalysisTypeCount
};
typedef enum ddb_AnalysisTypeEnumeration ddb_AnalysisTypeEnm;

#endif
