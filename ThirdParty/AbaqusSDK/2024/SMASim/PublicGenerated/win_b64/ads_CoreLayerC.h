//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CoreLayerC_h
#define ads_CoreLayerC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment Layer of the latest level of form Core */

/** Table to capture the beam general section with generalized properties. */
#define ads_BGeneralGeometryTable (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 0))

/** Taper dimensions for Box profile. */
#define ads_Box_taper (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 1))

/** Box profile taper width */
#define ads_Box_taper_a (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 2))

/** Box profile taper height */
#define ads_Box_taper_b (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 3))

/** Box profile taper right wall thickness */
#define ads_Box_taper_t1 (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 4))

/** Box profile taper left wall thickness */
#define ads_Box_taper_t2 (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 5))

/** Box profile taper floor thickness */
#define ads_Box_taper_t3 (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 6))

/** Box profile taper ceiling thickness */
#define ads_Box_taper_t4 (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 7))

/** Taper dimensions for Channel profile. */
#define ads_Channel_taper (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 8))

/** Channel profile taper b1 */
#define ads_Channel_taper_b1 (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 9))

/** Channel profile taper b2 */
#define ads_Channel_taper_b2 (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 10))

/** Channel profile taper height */
#define ads_Channel_taper_h (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 11))

/** Channel profile taper length */
#define ads_Channel_taper_l (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 12))

/** Channel profile taper offset */
#define ads_Channel_taper_o (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 13))

/** Channel profile taper thickness 1 */
#define ads_Channel_taper_t1 (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 14))

/** Channel profile taper thickness 2 */
#define ads_Channel_taper_t2 (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 15))

/** Channel profile taper thickness 3 */
#define ads_Channel_taper_t3 (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 16))

/** Taper dimensions for Circular profile. */
#define ads_Circular_taper (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 17))

/** A positive Float specifying the taper r dimension (outer radius) of the circular profile. */
#define ads_Circular_taper_radius (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 18))

/** Taper dimensions for Hat profile. */
#define ads_Hat_taper (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 19))

/** Hat profile taper b */
#define ads_Hat_taper_b (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 20))

/** Hat profile taper b1 */
#define ads_Hat_taper_b1 (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 21))

/** Hat profile taper b2 */
#define ads_Hat_taper_b2 (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 22))

/** Hat profile taper height */
#define ads_Hat_taper_h (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 23))

/** Hat profile taper length */
#define ads_Hat_taper_l (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 24))

/** Hat profile taper thickness 1 */
#define ads_Hat_taper_t1 (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 25))

/** Hat profile taper thickness 2 */
#define ads_Hat_taper_t2 (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 26))

/** Hat profile taper thickness 3 */
#define ads_Hat_taper_t3 (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 27))

/** Taper dimensions for Hexagonal profile. */
#define ads_Hexagonal_taper (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 28))

/** A positive Float specifying the taper r dimension (outer radius) of the hexagonal profile. */
#define ads_Hexagonal_taper_d (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 29))

/** A positive Float specifying the taper t dimension (wall thickness) of the hexagonal profile. */
#define ads_Hexagonal_taper_t (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 30))

/** Taper dimensions for I profile. */
#define ads_I_taper (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 31))

/** A Float specifying the taper b1 dimension (bottom flange width) of the I profile. */
#define ads_I_taper_b1 (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 32))

/** A Float specifying the taper b2 dimension (top flange width) of the I profile. */
#define ads_I_taper_b2 (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 33))

/** A Float specifying the taper h dimension (height) of the I profile. */
#define ads_I_taper_h (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 34))

/** A Float specifying the taper l dimension (offset of 1-axis from the bottom flange surface) of the I profile. */
#define ads_I_taper_l (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 35))

/** A Float specifying the taper t1 dimension (bottom flange thickness) of the I profile. */
#define ads_I_taper_t1 (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 36))

/** A Float specifying the taper t2 dimension (top flange thickness) of the I profile. */
#define ads_I_taper_t2 (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 37))

/** A Float specifying the taper t3 dimension (web thickness) of the I profile. */
#define ads_I_taper_t3 (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 38))

/** Taper dimensions for L profile. */
#define ads_L_taper (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 39))

/** A positive Float specifying the taper a dimension (flange length) of the L profile. */
#define ads_L_taper_a (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 40))

/** A positive Float specifying the taper b dimension (flange length) of the L profile. */
#define ads_L_taper_b (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 41))

/** A positive Float specifying the taper t1 dimension (flange thickness) of the L profile. */
#define ads_L_taper_t1 (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 42))

/** A positive Float specifying the taper t2 dimension (flange thickness) of the L profile. */
#define ads_L_taper_t2 (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 43))

/** Layer type for Section_Beam layers. */
#define ads_Layer_Beam_Arbitrary (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 44))

/** Each arbitrary profile consists of one or more sequential segments captured using ftable part of a Field_FTable. */
#define ads_Layer_Beam_Arbitrary_segments (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 45))

/** Layer type for Section_Beam layers. */
#define ads_Layer_Beam_Box (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 46))

/** Box profile width */
#define ads_Layer_Beam_Box_a (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 47))

/** Box profile height */
#define ads_Layer_Beam_Box_b (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 48))

/** Box profile right wall thickness */
#define ads_Layer_Beam_Box_t1 (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 49))

/** Box profile left wall thickness */
#define ads_Layer_Beam_Box_t2 (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 50))

/** Box profile floor thickness */
#define ads_Layer_Beam_Box_t3 (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 51))

/** Box profile ceiling thickness */
#define ads_Layer_Beam_Box_t4 (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 52))

#define ads_Layer_Beam_Box_taper (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 53))

/** Layer type for Section_Beam layers. */
#define ads_Layer_Beam_Channel (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 54))

/** Channel profile b1 */
#define ads_Layer_Beam_Channel_b1 (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 55))

/** Channel profile b2 */
#define ads_Layer_Beam_Channel_b2 (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 56))

/** Channel profile height */
#define ads_Layer_Beam_Channel_h (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 57))

/** Channel profile length */
#define ads_Layer_Beam_Channel_l (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 58))

/** Channel profile taper offset */
#define ads_Layer_Beam_Channel_o (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 59))

/** Channel profile thickness 1 */
#define ads_Layer_Beam_Channel_t1 (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 60))

/** Channel profile thickness 2 */
#define ads_Layer_Beam_Channel_t2 (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 61))

/** Channel profile thickness 3 */
#define ads_Layer_Beam_Channel_t3 (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 62))

#define ads_Layer_Beam_Channel_taper (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 63))

/** Layer type for Section_Beam layers. */
#define ads_Layer_Beam_Circular (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 64))

#define ads_Layer_Beam_Circular_distribution (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 65))

/** A positive Float specifying the r dimension (outer radius) of the circular profile. For more information, see "Beam cross-section library," Section 26.3.9 of the Abaqus Analysis User's Manual. */
#define ads_Layer_Beam_Circular_radius (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 66))

#define ads_Layer_Beam_Circular_taper (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 67))

/** Layer type for Section_Beam layers. */
#define ads_Layer_Beam_Elbow (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 68))

/** Outside radius of the pipe. */
#define ads_Layer_Beam_Elbow_outerRadius (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 69))

/** Pipe wall thickness. */
#define ads_Layer_Beam_Elbow_thickness (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 70))

/** Elbow torus radius, R, measured to the pipe axis. For a straight pipe, set torusRadius=0. */
#define ads_Layer_Beam_Elbow_torusRadius (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 71))

/** Layer type for beam general section with generalized profile. */
#define ads_Layer_Beam_General (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 72))

#define ads_Layer_Beam_General_table (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 73))

#define ads_Layer_Beam_General_taperTable (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 74))

/** Layer type for Section_Beam layers. */
#define ads_Layer_Beam_Hat (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 75))

/** Hat profile b */
#define ads_Layer_Beam_Hat_b (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 76))

/** Hat profile b1 */
#define ads_Layer_Beam_Hat_b1 (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 77))

/** Hat profile b2 */
#define ads_Layer_Beam_Hat_b2 (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 78))

/** Hat profile height */
#define ads_Layer_Beam_Hat_h (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 79))

/** Hat profile length */
#define ads_Layer_Beam_Hat_l (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 80))

/** Hat profile thickness 1 */
#define ads_Layer_Beam_Hat_t1 (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 81))

/** Hat profile thickness 2 */
#define ads_Layer_Beam_Hat_t2 (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 82))

/** Hat profile thickness 3 */
#define ads_Layer_Beam_Hat_t3 (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 83))

#define ads_Layer_Beam_Hat_taper (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 84))

/** Layer type for Section_Beam layers. */
#define ads_Layer_Beam_Hexagonal (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 85))

/** A positive Float specifying the r dimension (outer radius) of the hexagonal profile. For more information, see "Beam cross-section library," Section 26.3.9 of the Abaqus Analysis User's Manual. */
#define ads_Layer_Beam_Hexagonal_d (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 86))

/** A positive Float specifying the t dimension (wall thickness) of the hexagonal profile. */
#define ads_Layer_Beam_Hexagonal_t (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 87))

#define ads_Layer_Beam_Hexagonal_taper (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 88))

/** Layer type for Section_Beam layers. */
#define ads_Layer_Beam_I (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 89))

/** A Float specifying the b1 dimension (bottom flange width) of the I profile. */
#define ads_Layer_Beam_I_b1 (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 90))

/** A Float specifying the b2 dimension (top flange width) of the I profile. */
#define ads_Layer_Beam_I_b2 (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 91))

/** A Float specifying the h dimension (height) of the I profile. */
#define ads_Layer_Beam_I_h (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 92))

/** A Float specifying the l dimension (offset of 1-axis from the bottom flange surface) of the I profile. For more information, see "Beam cross-section library," Section 26.3.9 of the Abaqus Analysis User's Manual. */
#define ads_Layer_Beam_I_l (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 93))

/** A Float specifying the t1 dimension (bottom flange thickness) of the I profile. */
#define ads_Layer_Beam_I_t1 (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 94))

/** A Float specifying the t2 dimension (top flange thickness) of the I profile. */
#define ads_Layer_Beam_I_t2 (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 95))

/** A Float specifying the t3 dimension (web thickness) of the I profile. */
#define ads_Layer_Beam_I_t3 (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 96))

#define ads_Layer_Beam_I_taper (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 97))

/** Layer type for Section_Beam layers. */
#define ads_Layer_Beam_L (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 98))

/** A positive Float specifying the a dimension (flange length) of the L profile. For more information, see "Beam cross-section library," Section 26.3.9 of the Abaqus Analysis User's Manual. */
#define ads_Layer_Beam_L_a (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 99))

/** A positive Float specifying the b dimension (flange length) of the L profile. */
#define ads_Layer_Beam_L_b (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 100))

/** A positive Float specifying the t1 dimension (flange thickness) of the L profile. */
#define ads_Layer_Beam_L_t1 (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 101))

/** A positive Float specifying the t2 dimension (flange thickness) of the L profile. */
#define ads_Layer_Beam_L_t2 (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 102))

#define ads_Layer_Beam_L_taper (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 103))

/** Use Link for 1-D solid elements and truss elements. */
#define ads_Layer_Beam_Link (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 104))

/** Pipe wall thickness. */
#define ads_Layer_Beam_Link_area (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 105))

/** Layer type for beam general section with meshed profile. */
#define ads_Layer_Beam_Meshed (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 106))

/** Layer type for beam general section with non linear generalized profile. */
#define ads_Layer_Beam_Nonlinear_General (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 107))

#define ads_Layer_Beam_Nonlinear_General_table (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 108))

/** Layer type for Section_Beam layers. */
#define ads_Layer_Beam_Pipe (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 109))

/** Float specifying the outer radius of the pipe. For more information, see "Beam cross-section library," Section 26.3.9 of the Abaqus Analysis User's Manual. */
#define ads_Layer_Beam_Pipe_r (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 110))

/** A Float specifying the wall thickness of the pipe. */
#define ads_Layer_Beam_Pipe_t (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 111))

#define ads_Layer_Beam_Pipe_taper (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 112))

/** Layer type for Section_Beam layers. */
#define ads_Layer_Beam_Rectangular (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 113))

/** A positive Float specifying the a dimension of the rectangular profile. For more information, see "Beam cross-section library," Section 26.3.9 of the Abaqus Analysis User's Manual. */
#define ads_Layer_Beam_Rectangular_a (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 114))

/** A positive Float specifying the b dimension of the rectangular profile. */
#define ads_Layer_Beam_Rectangular_b (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 115))

#define ads_Layer_Beam_Rectangular_taper (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 116))

/** Layer type for Section_Beam layers. */
#define ads_Layer_Beam_Thick_Pipe (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 117))

/** Float specifying the outer radius of the pipe. For more information, see "Beam cross-section library," Section 26.3.9 of the Abaqus Analysis User's Manual. */
#define ads_Layer_Beam_Thick_Pipe_r (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 118))

/** A Float specifying the wall thickness of the pipe. */
#define ads_Layer_Beam_Thick_Pipe_t (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 119))

#define ads_Layer_Beam_Thick_Pipe_taper (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 120))

/** Layer type for Section_Beam layers. */
#define ads_Layer_Beam_Trapezoidal (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 121))

/** A positive Float specifying the a dimension of the Trapezoidal profile. For more information, see "Beam cross-section library," Section 26.3.9 of the Abaqus Analysis User's Manual. */
#define ads_Layer_Beam_Trapezoidal_a (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 122))

/** A positive Float specifying the b dimension of the Trapezoidal profile. */
#define ads_Layer_Beam_Trapezoidal_b (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 123))

/** A positive Float specifying the c dimension of the Trapezoidal profile. */
#define ads_Layer_Beam_Trapezoidal_c (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 124))

/** A positive Float specifying the d dimension of the Trapezoidal profile. */
#define ads_Layer_Beam_Trapezoidal_d (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 125))

#define ads_Layer_Beam_Trapezoidal_taper (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 126))

/** Layer type for Section_Connector layers. */
#define ads_Layer_Connector_Assembled (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 127))

/** Layer type for Section_Connector layers. */
#define ads_Layer_Connector_Basic (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 128))

/** Layer type for Section_Connector layers. */
#define ads_Layer_Connector_Dashpot12 (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 129))

/** Layer type for Section_Connector layers. */
#define ads_Layer_Connector_DashpotA (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 130))

/** Layer type for Section_Connector layers. */
#define ads_Layer_Connector_EPJointMember (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 131))

/** conical spud can cone angle in degrees. Enter a blank, zero, or 180 for a cylindrical spud can. */
#define ads_Layer_Connector_EPJointMember_coneAngle (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 132))

/** Diameter of spud can cylindrical portion. */
#define ads_Layer_Connector_EPJointMember_diameter (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 133))

/** Layer type for Section_Connector layers. */
#define ads_Layer_Connector_EPJointSpud (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 134))

/** conical spud can cone angle in degrees. Enter a blank, zero, or 180 for a cylindrical spud can. */
#define ads_Layer_Connector_EPJointSpud_coneAngle (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 135))

/** Diameter of spud can cylindrical portion. */
#define ads_Layer_Connector_EPJointSpud_diameter (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 136))

/** Layer type for Section_Connector layers. */
#define ads_Layer_Connector_FlowConverter (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 137))

/** Layer type for Section_Connector layers. */
#define ads_Layer_Connector_JointC (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 138))

/** Layer type for Section_Connector layers. */
#define ads_Layer_Connector_Retractor (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 139))

/** Layer type for Section_Connector layers. */
#define ads_Layer_Connector_Slipring (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 140))

/** Contact angle (in radians) made by belt wrapping around node b (optional). In Abaqus/Standard the default value is 0.0. In Abaqus/Explicit the contact angle is computed automatically if it is not specified. */
#define ads_Layer_Connector_Slipring_contactAngle (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 141))

/** Layer type for Section_Connector layers. */
#define ads_Layer_Connector_Spring12 (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 142))

/** Layer type for Section_Connector layers. */
#define ads_Layer_Connector_SpringA (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 143))

/** Layer type for Section_Connector layers. */
#define ads_Layer_Connector_TubeSupportCyl (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 144))

/** Diameter of the hole in the support plate */
#define ads_Layer_Connector_TubeSupportCyl_distance (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 145))

/** Tube outside diameter. */
#define ads_Layer_Connector_TubeSupportCyl_tubeDiameter (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 146))

/** Layer type for Section_Connector layers. */
#define ads_Layer_Connector_TubeSupportUni (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 147))

/** Distance between the parallel support plates on opposite sides of the tube. */
#define ads_Layer_Connector_TubeSupportUni_distance (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 148))

/** Tube outside diameter. */
#define ads_Layer_Connector_TubeSupportUni_tubeDiameter (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 149))

/** Layer type for Section_Constraint layers. */
#define ads_Layer_Constraint (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 150))

/** Layer type for Section_Continuum layers. */
#define ads_Layer_Continuum (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 151))

/** Layer type for Section_Continuum_Eulerian layers. */
#define ads_Layer_Continuum_Eulerian (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 152))

/** Thickness ratio for the layer. */
#define ads_Layer_Continuum_thickness (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 153))

/** Layer type for Section_Discrete layers. */
#define ads_Layer_Discrete (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 154))

/** The distribution table or the probability density function for the radius of the discrete elements */
#define ads_Layer_Discrete_distribution (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 155))

/** Size of the elements */
#define ads_Layer_Discrete_size (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 156))

#define ads_Layer_Discrete_table (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 157))

/** Layer type for Section_Interface layers. */
#define ads_Layer_Interface_Asymmetric (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 158))

/** Layer type for Section_Interface layers. */
#define ads_Layer_Interface_DragChain (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 159))

/** Interpretation of length depends on 2D or 3D */
#define ads_Layer_Interface_DragChain_length (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 160))

/** Layer type for Section_Interface layers. */
#define ads_Layer_Interface_Normal (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 161))

/** Initial gap for GAPUNI and gasket elements. */
#define ads_Layer_Interface_Normal_initialGap (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 162))

/** Used for gasket elements. */
#define ads_Layer_Interface_Normal_initialVoid (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 163))

/** Thickness for the layer. */
#define ads_Layer_Interface_Normal_thickness (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 164))

/** Layer type for Section_Interface layers. */
#define ads_Layer_Interface_Radial (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 165))

/** Radial clearance for GAPCYL, GAPSHERE, and tube to tube elements. */
#define ads_Layer_Interface_Radial_radialClearance (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 166))

/** Layer type for Section_Lumped layers. */
#define ads_Layer_Lumped (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 167))

/** Layer type for Section_Shell layers. */
#define ads_Layer_Rebar_Angular (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 168))

/** Spacing of rebar in the plane of the membrane, shell, or surface element. The value should be given in terms of angular spacing in degrees. */
#define ads_Layer_Rebar_Angular_angularSpacing (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 169))

/** Cross-sectional area of the rebar. */
#define ads_Layer_Rebar_Angular_area (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 170))

/** Position of the rebar in the shell section thickness direction. This value is given as the distance of the rebar from the middle surface of the shell, positive in the direction of the positive normal to the shell. This value is modified if the NODAL THICKNESS parameter is included with the *SHELL SECTION option for the underlying shell element. This entry has no meaning for rebar in either membrane or surface elements. */
#define ads_Layer_Rebar_Angular_position (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 171))

/** Layer type for Section_Shell layers. */
#define ads_Layer_Rebar_Constant (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 172))

/** Cross-sectional area of the rebar. */
#define ads_Layer_Rebar_Constant_area (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 173))

/** Position of the rebar in the shell section thickness direction. This value is given as the distance of the rebar from the middle surface of the shell, positive in the direction of the positive normal to the shell. This value is modified if the NODAL THICKNESS parameter is included with the *SHELL SECTION option for the underlying shell element. This entry has no meaning for rebar in either membrane or surface elements. */
#define ads_Layer_Rebar_Constant_position (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 174))

/** Spacing of rebar in the plane of the membrane, shell, or surface element. The value is given as a length measure. */
#define ads_Layer_Rebar_Constant_spacing (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 175))

/** Layer type for Section_Shell layers. */
#define ads_Layer_Rebar_LiftEquation (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 176))

/** Cross-sectional area of the rebar. */
#define ads_Layer_Rebar_LiftEquation_area (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 177))

/** Extension ratio, e, for rebar defined with GEOMETRY=LIFT EQUATION. In a tire e represents the pre-strain that occurs during the curing process; e =1 means a 100% extension. */
#define ads_Layer_Rebar_LiftEquation_extensionRatio (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 178))

/** Position of the rebar in the shell section thickness direction. This value is given as the distance of the rebar from the middle surface of the shell, positive in the direction of the positive normal to the shell. This value is modified if the NODAL THICKNESS parameter is included with the *SHELL SECTION option for the underlying shell element. This entry has no meaning for rebar in either membrane or surface elements. */
#define ads_Layer_Rebar_LiftEquation_position (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 179))

/** Radius, , of the rebar defined with GEOMETRY=LIFT EQUATION. The value is the position of the rebar in the uncured geometry, measured with respect to the axis of rotation in a cylindrical coordinate system. */
#define ads_Layer_Rebar_LiftEquation_radius (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 180))

/** Spacing of rebar in the plane of the membrane, shell, or surface element. The value is given as a length measure. */
#define ads_Layer_Rebar_LiftEquation_spacing (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 181))

/** Layer type for Section_Beam layers. */
#define ads_Layer_Rebar_Line (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 182))

#define ads_Layer_Rebar_Line_area (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 183))

#define ads_Layer_Rebar_Line_locX1 (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 184))

#define ads_Layer_Rebar_Line_locX2 (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 185))

#define ads_MMecChildParticleTable (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 186))

/** Taper dimensions for Pipe profile. */
#define ads_Pipe_taper (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 187))

/** Float specifying the taper outer radius of the pipe. */
#define ads_Pipe_taper_r (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 188))

/** A Float specifying the taper wall thickness of the pipe. */
#define ads_Pipe_taper_t (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 189))

/** Taper dimensions for Rectangular profile. */
#define ads_Rectangular_taper (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 190))

/** A positive Float specifying the taper a dimension of the rectangular profile. */
#define ads_Rectangular_taper_a (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 191))

/** A positive Float specifying the taper b dimension of the rectangular profile. */
#define ads_Rectangular_taper_b (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 192))

/** Taper dimensions for Thick_Pipe profile. */
#define ads_Thick_Pipe_taper (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 193))

/** Float specifying the taper outer radius of the pipe. */
#define ads_Thick_Pipe_taper_r (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 194))

/** A Float specifying the taper wall thickness of the pipe. */
#define ads_Thick_Pipe_taper_t (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 195))

/** Taper dimensions for Trapezoidal profile. */
#define ads_Trapezoidal_taper (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 196))

/** A positive Float specifying the taper a dimension of the Trapezoidal profile. */
#define ads_Trapezoidal_taper_a (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 197))

/** A positive Float specifying the taper b dimension of the Trapezoidal profile. */
#define ads_Trapezoidal_taper_b (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 198))

/** A positive Float specifying the taper c dimension of the Trapezoidal profile. */
#define ads_Trapezoidal_taper_c (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 199))

/** A positive Float specifying the taper d dimension of the Trapezoidal profile. */
#define ads_Trapezoidal_taper_d (ads_CoreFragmentTypeIndex(ads_CoreLayerFragment, 200))

/** 
Enum with association roles. */
enum ads_Box_taper_aRolesEnm
{
    ads_Box_taper_a_child,
    ads_Box_taper_a_parent
};

/** 
Enum with association roles. */
enum ads_Box_taper_bRolesEnm
{
    ads_Box_taper_b_child,
    ads_Box_taper_b_parent
};

/** 
Enum with association roles. */
enum ads_Box_taper_t1RolesEnm
{
    ads_Box_taper_t1_child,
    ads_Box_taper_t1_parent
};

/** 
Enum with association roles. */
enum ads_Box_taper_t2RolesEnm
{
    ads_Box_taper_t2_child,
    ads_Box_taper_t2_parent
};

/** 
Enum with association roles. */
enum ads_Box_taper_t3RolesEnm
{
    ads_Box_taper_t3_child,
    ads_Box_taper_t3_parent
};

/** 
Enum with association roles. */
enum ads_Box_taper_t4RolesEnm
{
    ads_Box_taper_t4_child,
    ads_Box_taper_t4_parent
};

/** 
Enum with association roles. */
enum ads_Channel_taper_b1RolesEnm
{
    ads_Channel_taper_b1_child,
    ads_Channel_taper_b1_parent
};

/** 
Enum with association roles. */
enum ads_Channel_taper_b2RolesEnm
{
    ads_Channel_taper_b2_child,
    ads_Channel_taper_b2_parent
};

/** 
Enum with association roles. */
enum ads_Channel_taper_hRolesEnm
{
    ads_Channel_taper_h_child,
    ads_Channel_taper_h_parent
};

/** 
Enum with association roles. */
enum ads_Channel_taper_lRolesEnm
{
    ads_Channel_taper_l_child,
    ads_Channel_taper_l_parent
};

/** 
Enum with association roles. */
enum ads_Channel_taper_oRolesEnm
{
    ads_Channel_taper_o_child,
    ads_Channel_taper_o_parent
};

/** 
Enum with association roles. */
enum ads_Channel_taper_t1RolesEnm
{
    ads_Channel_taper_t1_child,
    ads_Channel_taper_t1_parent
};

/** 
Enum with association roles. */
enum ads_Channel_taper_t2RolesEnm
{
    ads_Channel_taper_t2_child,
    ads_Channel_taper_t2_parent
};

/** 
Enum with association roles. */
enum ads_Channel_taper_t3RolesEnm
{
    ads_Channel_taper_t3_child,
    ads_Channel_taper_t3_parent
};

/** 
Enum with association roles. */
enum ads_Circular_taper_radiusRolesEnm
{
    ads_Circular_taper_radius_child,
    ads_Circular_taper_radius_parent
};

/** 
Enum with association roles. */
enum ads_Hat_taper_bRolesEnm
{
    ads_Hat_taper_b_child,
    ads_Hat_taper_b_parent
};

/** 
Enum with association roles. */
enum ads_Hat_taper_b1RolesEnm
{
    ads_Hat_taper_b1_child,
    ads_Hat_taper_b1_parent
};

/** 
Enum with association roles. */
enum ads_Hat_taper_b2RolesEnm
{
    ads_Hat_taper_b2_child,
    ads_Hat_taper_b2_parent
};

/** 
Enum with association roles. */
enum ads_Hat_taper_hRolesEnm
{
    ads_Hat_taper_h_child,
    ads_Hat_taper_h_parent
};

/** 
Enum with association roles. */
enum ads_Hat_taper_lRolesEnm
{
    ads_Hat_taper_l_child,
    ads_Hat_taper_l_parent
};

/** 
Enum with association roles. */
enum ads_Hat_taper_t1RolesEnm
{
    ads_Hat_taper_t1_child,
    ads_Hat_taper_t1_parent
};

/** 
Enum with association roles. */
enum ads_Hat_taper_t2RolesEnm
{
    ads_Hat_taper_t2_child,
    ads_Hat_taper_t2_parent
};

/** 
Enum with association roles. */
enum ads_Hat_taper_t3RolesEnm
{
    ads_Hat_taper_t3_child,
    ads_Hat_taper_t3_parent
};

/** 
Enum with association roles. */
enum ads_Hexagonal_taper_dRolesEnm
{
    ads_Hexagonal_taper_d_child,
    ads_Hexagonal_taper_d_parent
};

/** 
Enum with association roles. */
enum ads_Hexagonal_taper_tRolesEnm
{
    ads_Hexagonal_taper_t_child,
    ads_Hexagonal_taper_t_parent
};

/** 
Enum with association roles. */
enum ads_I_taper_b1RolesEnm
{
    ads_I_taper_b1_child,
    ads_I_taper_b1_parent
};

/** 
Enum with association roles. */
enum ads_I_taper_b2RolesEnm
{
    ads_I_taper_b2_child,
    ads_I_taper_b2_parent
};

/** 
Enum with association roles. */
enum ads_I_taper_hRolesEnm
{
    ads_I_taper_h_child,
    ads_I_taper_h_parent
};

/** 
Enum with association roles. */
enum ads_I_taper_lRolesEnm
{
    ads_I_taper_l_child,
    ads_I_taper_l_parent
};

/** 
Enum with association roles. */
enum ads_I_taper_t1RolesEnm
{
    ads_I_taper_t1_child,
    ads_I_taper_t1_parent
};

/** 
Enum with association roles. */
enum ads_I_taper_t2RolesEnm
{
    ads_I_taper_t2_child,
    ads_I_taper_t2_parent
};

/** 
Enum with association roles. */
enum ads_I_taper_t3RolesEnm
{
    ads_I_taper_t3_child,
    ads_I_taper_t3_parent
};

/** 
Enum with association roles. */
enum ads_L_taper_aRolesEnm
{
    ads_L_taper_a_child,
    ads_L_taper_a_parent
};

/** 
Enum with association roles. */
enum ads_L_taper_bRolesEnm
{
    ads_L_taper_b_child,
    ads_L_taper_b_parent
};

/** 
Enum with association roles. */
enum ads_L_taper_t1RolesEnm
{
    ads_L_taper_t1_child,
    ads_L_taper_t1_parent
};

/** 
Enum with association roles. */
enum ads_L_taper_t2RolesEnm
{
    ads_L_taper_t2_child,
    ads_L_taper_t2_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_Arbitrary_segmentsRolesEnm
{
    ads_Layer_Beam_Arbitrary_segments_child,
    ads_Layer_Beam_Arbitrary_segments_parent
};

/** 
Enum with record members. */
enum ads_Layer_Beam_BoxMembersEnm
{
    ads_Layer_Beam_Box_numPoints_1,
    ads_Layer_Beam_Box_numPoints_2
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_Box_aRolesEnm
{
    ads_Layer_Beam_Box_a_child,
    ads_Layer_Beam_Box_a_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_Box_bRolesEnm
{
    ads_Layer_Beam_Box_b_child,
    ads_Layer_Beam_Box_b_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_Box_t1RolesEnm
{
    ads_Layer_Beam_Box_t1_child,
    ads_Layer_Beam_Box_t1_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_Box_t2RolesEnm
{
    ads_Layer_Beam_Box_t2_child,
    ads_Layer_Beam_Box_t2_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_Box_t3RolesEnm
{
    ads_Layer_Beam_Box_t3_child,
    ads_Layer_Beam_Box_t3_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_Box_t4RolesEnm
{
    ads_Layer_Beam_Box_t4_child,
    ads_Layer_Beam_Box_t4_parent
};

/** Enum with association roles. */
enum ads_Layer_Beam_Box_taperRolesEnm
{
    ads_Layer_Beam_Box_taper_child,
    ads_Layer_Beam_Box_taper_parent
};

/** 
Enum with record members. */
enum ads_Layer_Beam_ChannelMembersEnm
{
    ads_Layer_Beam_Channel_numPoints_1,
    ads_Layer_Beam_Channel_numPoints_2
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_Channel_b1RolesEnm
{
    ads_Layer_Beam_Channel_b1_child,
    ads_Layer_Beam_Channel_b1_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_Channel_b2RolesEnm
{
    ads_Layer_Beam_Channel_b2_child,
    ads_Layer_Beam_Channel_b2_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_Channel_hRolesEnm
{
    ads_Layer_Beam_Channel_h_child,
    ads_Layer_Beam_Channel_h_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_Channel_lRolesEnm
{
    ads_Layer_Beam_Channel_l_child,
    ads_Layer_Beam_Channel_l_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_Channel_oRolesEnm
{
    ads_Layer_Beam_Channel_o_child,
    ads_Layer_Beam_Channel_o_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_Channel_t1RolesEnm
{
    ads_Layer_Beam_Channel_t1_child,
    ads_Layer_Beam_Channel_t1_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_Channel_t2RolesEnm
{
    ads_Layer_Beam_Channel_t2_child,
    ads_Layer_Beam_Channel_t2_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_Channel_t3RolesEnm
{
    ads_Layer_Beam_Channel_t3_child,
    ads_Layer_Beam_Channel_t3_parent
};

/** Enum with association roles. */
enum ads_Layer_Beam_Channel_taperRolesEnm
{
    ads_Layer_Beam_Channel_taper_child,
    ads_Layer_Beam_Channel_taper_parent
};

/** 
Enum with record members. */
enum ads_Layer_Beam_CircularMembersEnm
{
    ads_Layer_Beam_Circular_numPoints_1,
    ads_Layer_Beam_Circular_numPoints_2
};

/** Enum with association roles. */
enum ads_Layer_Beam_Circular_distributionRolesEnm
{
    ads_Layer_Beam_Circular_distribution_referent,
    ads_Layer_Beam_Circular_distribution_referrer
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_Circular_radiusRolesEnm
{
    ads_Layer_Beam_Circular_radius_child,
    ads_Layer_Beam_Circular_radius_parent
};

/** Enum with association roles. */
enum ads_Layer_Beam_Circular_taperRolesEnm
{
    ads_Layer_Beam_Circular_taper_child,
    ads_Layer_Beam_Circular_taper_parent
};

/** 
Enum with record members. */
enum ads_Layer_Beam_ElbowMembersEnm
{
    ads_Layer_Beam_Elbow_numFourierModes,
    ads_Layer_Beam_Elbow_numPoints_1,
    ads_Layer_Beam_Elbow_numPoints_2
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_Elbow_outerRadiusRolesEnm
{
    ads_Layer_Beam_Elbow_outerRadius_child,
    ads_Layer_Beam_Elbow_outerRadius_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_Elbow_thicknessRolesEnm
{
    ads_Layer_Beam_Elbow_thickness_child,
    ads_Layer_Beam_Elbow_thickness_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_Elbow_torusRadiusRolesEnm
{
    ads_Layer_Beam_Elbow_torusRadius_child,
    ads_Layer_Beam_Elbow_torusRadius_parent
};

/** Enum with association roles. */
enum ads_Layer_Beam_General_tableRolesEnm
{
    ads_Layer_Beam_General_table_child,
    ads_Layer_Beam_General_table_parent
};

/** Enum with association roles. */
enum ads_Layer_Beam_General_taperTableRolesEnm
{
    ads_Layer_Beam_General_taperTable_child,
    ads_Layer_Beam_General_taperTable_parent
};

/** 
Enum with record members. */
enum ads_Layer_Beam_HatMembersEnm
{
    ads_Layer_Beam_Hat_numPoints_1,
    ads_Layer_Beam_Hat_numPoints_2
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_Hat_bRolesEnm
{
    ads_Layer_Beam_Hat_b_child,
    ads_Layer_Beam_Hat_b_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_Hat_b1RolesEnm
{
    ads_Layer_Beam_Hat_b1_child,
    ads_Layer_Beam_Hat_b1_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_Hat_b2RolesEnm
{
    ads_Layer_Beam_Hat_b2_child,
    ads_Layer_Beam_Hat_b2_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_Hat_hRolesEnm
{
    ads_Layer_Beam_Hat_h_child,
    ads_Layer_Beam_Hat_h_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_Hat_lRolesEnm
{
    ads_Layer_Beam_Hat_l_child,
    ads_Layer_Beam_Hat_l_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_Hat_t1RolesEnm
{
    ads_Layer_Beam_Hat_t1_child,
    ads_Layer_Beam_Hat_t1_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_Hat_t2RolesEnm
{
    ads_Layer_Beam_Hat_t2_child,
    ads_Layer_Beam_Hat_t2_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_Hat_t3RolesEnm
{
    ads_Layer_Beam_Hat_t3_child,
    ads_Layer_Beam_Hat_t3_parent
};

/** Enum with association roles. */
enum ads_Layer_Beam_Hat_taperRolesEnm
{
    ads_Layer_Beam_Hat_taper_child,
    ads_Layer_Beam_Hat_taper_parent
};

/** 
Enum with record members. */
enum ads_Layer_Beam_HexagonalMembersEnm
{
    ads_Layer_Beam_Hexagonal_numPoints_1
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_Hexagonal_dRolesEnm
{
    ads_Layer_Beam_Hexagonal_d_child,
    ads_Layer_Beam_Hexagonal_d_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_Hexagonal_tRolesEnm
{
    ads_Layer_Beam_Hexagonal_t_child,
    ads_Layer_Beam_Hexagonal_t_parent
};

/** Enum with association roles. */
enum ads_Layer_Beam_Hexagonal_taperRolesEnm
{
    ads_Layer_Beam_Hexagonal_taper_child,
    ads_Layer_Beam_Hexagonal_taper_parent
};

/** 
Enum with record members. */
enum ads_Layer_Beam_IMembersEnm
{
    ads_Layer_Beam_I_numPoints_1,
    ads_Layer_Beam_I_numPoints_2,
    ads_Layer_Beam_I_numPoints_3
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_I_b1RolesEnm
{
    ads_Layer_Beam_I_b1_child,
    ads_Layer_Beam_I_b1_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_I_b2RolesEnm
{
    ads_Layer_Beam_I_b2_child,
    ads_Layer_Beam_I_b2_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_I_hRolesEnm
{
    ads_Layer_Beam_I_h_child,
    ads_Layer_Beam_I_h_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_I_lRolesEnm
{
    ads_Layer_Beam_I_l_child,
    ads_Layer_Beam_I_l_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_I_t1RolesEnm
{
    ads_Layer_Beam_I_t1_child,
    ads_Layer_Beam_I_t1_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_I_t2RolesEnm
{
    ads_Layer_Beam_I_t2_child,
    ads_Layer_Beam_I_t2_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_I_t3RolesEnm
{
    ads_Layer_Beam_I_t3_child,
    ads_Layer_Beam_I_t3_parent
};

/** Enum with association roles. */
enum ads_Layer_Beam_I_taperRolesEnm
{
    ads_Layer_Beam_I_taper_child,
    ads_Layer_Beam_I_taper_parent
};

/** 
Enum with record members. */
enum ads_Layer_Beam_LMembersEnm
{
    ads_Layer_Beam_L_numPoints_1,
    ads_Layer_Beam_L_numPoints_2
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_L_aRolesEnm
{
    ads_Layer_Beam_L_a_child,
    ads_Layer_Beam_L_a_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_L_bRolesEnm
{
    ads_Layer_Beam_L_b_child,
    ads_Layer_Beam_L_b_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_L_t1RolesEnm
{
    ads_Layer_Beam_L_t1_child,
    ads_Layer_Beam_L_t1_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_L_t2RolesEnm
{
    ads_Layer_Beam_L_t2_child,
    ads_Layer_Beam_L_t2_parent
};

/** Enum with association roles. */
enum ads_Layer_Beam_L_taperRolesEnm
{
    ads_Layer_Beam_L_taper_child,
    ads_Layer_Beam_L_taper_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_Link_areaRolesEnm
{
    ads_Layer_Beam_Link_area_child,
    ads_Layer_Beam_Link_area_parent
};

/** Enum with association roles. */
enum ads_Layer_Beam_Nonlinear_General_tableRolesEnm
{
    ads_Layer_Beam_Nonlinear_General_table_child,
    ads_Layer_Beam_Nonlinear_General_table_parent
};

/** 
Enum with record members. */
enum ads_Layer_Beam_PipeMembersEnm
{
    ads_Layer_Beam_Pipe_numPoints_1
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_Pipe_rRolesEnm
{
    ads_Layer_Beam_Pipe_r_child,
    ads_Layer_Beam_Pipe_r_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_Pipe_tRolesEnm
{
    ads_Layer_Beam_Pipe_t_child,
    ads_Layer_Beam_Pipe_t_parent
};

/** Enum with association roles. */
enum ads_Layer_Beam_Pipe_taperRolesEnm
{
    ads_Layer_Beam_Pipe_taper_child,
    ads_Layer_Beam_Pipe_taper_parent
};

/** 
Enum with record members. */
enum ads_Layer_Beam_RectangularMembersEnm
{
    ads_Layer_Beam_Rectangular_numPoints_1,
    ads_Layer_Beam_Rectangular_numPoints_2
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_Rectangular_aRolesEnm
{
    ads_Layer_Beam_Rectangular_a_child,
    ads_Layer_Beam_Rectangular_a_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_Rectangular_bRolesEnm
{
    ads_Layer_Beam_Rectangular_b_child,
    ads_Layer_Beam_Rectangular_b_parent
};

/** Enum with association roles. */
enum ads_Layer_Beam_Rectangular_taperRolesEnm
{
    ads_Layer_Beam_Rectangular_taper_child,
    ads_Layer_Beam_Rectangular_taper_parent
};

/** 
Enum with record members. */
enum ads_Layer_Beam_Thick_PipeMembersEnm
{
    ads_Layer_Beam_Thick_Pipe_numPoints_1,
    ads_Layer_Beam_Thick_Pipe_numPoints_2
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_Thick_Pipe_rRolesEnm
{
    ads_Layer_Beam_Thick_Pipe_r_child,
    ads_Layer_Beam_Thick_Pipe_r_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_Thick_Pipe_tRolesEnm
{
    ads_Layer_Beam_Thick_Pipe_t_child,
    ads_Layer_Beam_Thick_Pipe_t_parent
};

/** Enum with association roles. */
enum ads_Layer_Beam_Thick_Pipe_taperRolesEnm
{
    ads_Layer_Beam_Thick_Pipe_taper_child,
    ads_Layer_Beam_Thick_Pipe_taper_parent
};

/** 
Enum with record members. */
enum ads_Layer_Beam_TrapezoidalMembersEnm
{
    ads_Layer_Beam_Trapezoidal_numPoints_1,
    ads_Layer_Beam_Trapezoidal_numPoints_2
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_Trapezoidal_aRolesEnm
{
    ads_Layer_Beam_Trapezoidal_a_child,
    ads_Layer_Beam_Trapezoidal_a_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_Trapezoidal_bRolesEnm
{
    ads_Layer_Beam_Trapezoidal_b_child,
    ads_Layer_Beam_Trapezoidal_b_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_Trapezoidal_cRolesEnm
{
    ads_Layer_Beam_Trapezoidal_c_child,
    ads_Layer_Beam_Trapezoidal_c_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Beam_Trapezoidal_dRolesEnm
{
    ads_Layer_Beam_Trapezoidal_d_child,
    ads_Layer_Beam_Trapezoidal_d_parent
};

/** Enum with association roles. */
enum ads_Layer_Beam_Trapezoidal_taperRolesEnm
{
    ads_Layer_Beam_Trapezoidal_taper_child,
    ads_Layer_Beam_Trapezoidal_taper_parent
};

/** 
Enum with record members. */
enum ads_Layer_Connector_AssembledMembersEnm
{
    ads_Layer_Connector_Assembled_assembled
};

enum ads_Layer_Connector_Assembled_assembledEnm
{
    ads_Layer_Connector_Assembled_assembled_BEAM,
    ads_Layer_Connector_Assembled_assembled_BUSHING,
    ads_Layer_Connector_Assembled_assembled_CVJOINT,
    ads_Layer_Connector_Assembled_assembled_CYLINDRICAL,
    ads_Layer_Connector_Assembled_assembled_HINGE,
    ads_Layer_Connector_Assembled_assembled_PLANAR,
    ads_Layer_Connector_Assembled_assembled_TRANSLATOR,
    ads_Layer_Connector_Assembled_assembled_UJOINT,
    ads_Layer_Connector_Assembled_assembled_WELD
};

/** 
Enum with record members. */
enum ads_Layer_Connector_BasicMembersEnm
{
    ads_Layer_Connector_Basic_rotation,
    ads_Layer_Connector_Basic_translation
};

enum ads_Layer_Connector_Basic_rotationEnm
{
    ads_Layer_Connector_Basic_rotation_ALIGN,
    ads_Layer_Connector_Basic_rotation_CARDAN,
    ads_Layer_Connector_Basic_rotation_CONSTANT_VELOCITY,
    ads_Layer_Connector_Basic_rotation_EULER,
    ads_Layer_Connector_Basic_rotation_FLEXION_TORSION,
    ads_Layer_Connector_Basic_rotation_NULL,
    ads_Layer_Connector_Basic_rotation_PROJECTION_FLEXION_TORSION,
    ads_Layer_Connector_Basic_rotation_REVOLUTE,
    ads_Layer_Connector_Basic_rotation_ROTATION,
    ads_Layer_Connector_Basic_rotation_ROTATION_ACCELEROMETER,
    ads_Layer_Connector_Basic_rotation_UNIVERSAL
};

enum ads_Layer_Connector_Basic_translationEnm
{
    ads_Layer_Connector_Basic_translation_ACCELEROMETER,
    ads_Layer_Connector_Basic_translation_AXIAL,
    ads_Layer_Connector_Basic_translation_CARTESIAN,
    ads_Layer_Connector_Basic_translation_JOIN,
    ads_Layer_Connector_Basic_translation_LINK,
    ads_Layer_Connector_Basic_translation_NULL,
    ads_Layer_Connector_Basic_translation_PROJECTION_CARTESIAN,
    ads_Layer_Connector_Basic_translation_RADIAL_THRUST,
    ads_Layer_Connector_Basic_translation_SLIDE_PLANE,
    ads_Layer_Connector_Basic_translation_SLOT
};

/** 
Enum with record members. */
enum ads_Layer_Connector_Dashpot12MembersEnm
{
    ads_Layer_Connector_Dashpot12_secondComponent
};

/** 
Enum with association roles. */
enum ads_Layer_Connector_EPJointMember_coneAngleRolesEnm
{
    ads_Layer_Connector_EPJointMember_coneAngle_child,
    ads_Layer_Connector_EPJointMember_coneAngle_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Connector_EPJointMember_diameterRolesEnm
{
    ads_Layer_Connector_EPJointMember_diameter_child,
    ads_Layer_Connector_EPJointMember_diameter_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Connector_EPJointSpud_coneAngleRolesEnm
{
    ads_Layer_Connector_EPJointSpud_coneAngle_child,
    ads_Layer_Connector_EPJointSpud_coneAngle_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Connector_EPJointSpud_diameterRolesEnm
{
    ads_Layer_Connector_EPJointSpud_diameter_child,
    ads_Layer_Connector_EPJointSpud_diameter_parent
};

/** 
Enum with record members. */
enum ads_Layer_Connector_FlowConverterMembersEnm
{
    ads_Layer_Connector_FlowConverter_materialFlowScale
};

/** 
Enum with record members. */
enum ads_Layer_Connector_RetractorMembersEnm
{
    ads_Layer_Connector_Retractor_materialFlowScale
};

/** 
Enum with association roles. */
enum ads_Layer_Connector_Slipring_contactAngleRolesEnm
{
    ads_Layer_Connector_Slipring_contactAngle_child,
    ads_Layer_Connector_Slipring_contactAngle_parent
};

/** 
Enum with record members. */
enum ads_Layer_Connector_Spring12MembersEnm
{
    ads_Layer_Connector_Spring12_secondComponent
};

/** 
Enum with association roles. */
enum ads_Layer_Connector_TubeSupportCyl_distanceRolesEnm
{
    ads_Layer_Connector_TubeSupportCyl_distance_child,
    ads_Layer_Connector_TubeSupportCyl_distance_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Connector_TubeSupportCyl_tubeDiameterRolesEnm
{
    ads_Layer_Connector_TubeSupportCyl_tubeDiameter_child,
    ads_Layer_Connector_TubeSupportCyl_tubeDiameter_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Connector_TubeSupportUni_distanceRolesEnm
{
    ads_Layer_Connector_TubeSupportUni_distance_child,
    ads_Layer_Connector_TubeSupportUni_distance_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Connector_TubeSupportUni_tubeDiameterRolesEnm
{
    ads_Layer_Connector_TubeSupportUni_tubeDiameter_child,
    ads_Layer_Connector_TubeSupportUni_tubeDiameter_parent
};

/** 
Enum with record members. */
enum ads_Layer_ContinuumMembersEnm
{
    ads_Layer_Continuum_numPoints
};

/** 
Enum with association roles. */
enum ads_Layer_Continuum_thicknessRolesEnm
{
    ads_Layer_Continuum_thickness_child,
    ads_Layer_Continuum_thickness_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Discrete_distributionRolesEnm
{
    ads_Layer_Discrete_distribution_referent,
    ads_Layer_Discrete_distribution_referrer
};

/** 
Enum with association roles. */
enum ads_Layer_Discrete_sizeRolesEnm
{
    ads_Layer_Discrete_size_child,
    ads_Layer_Discrete_size_parent
};

/** Enum with association roles. */
enum ads_Layer_Discrete_tableRolesEnm
{
    ads_Layer_Discrete_table_child,
    ads_Layer_Discrete_table_parent
};

/** 
Enum with record members. */
enum ads_Layer_Interface_AsymmetricMembersEnm
{
    ads_Layer_Interface_Asymmetric_angle,
    ads_Layer_Interface_Asymmetric_mode
};

/** 
Enum with association roles. */
enum ads_Layer_Interface_DragChain_lengthRolesEnm
{
    ads_Layer_Interface_DragChain_length_child,
    ads_Layer_Interface_DragChain_length_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Interface_Normal_initialGapRolesEnm
{
    ads_Layer_Interface_Normal_initialGap_child,
    ads_Layer_Interface_Normal_initialGap_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Interface_Normal_initialVoidRolesEnm
{
    ads_Layer_Interface_Normal_initialVoid_child,
    ads_Layer_Interface_Normal_initialVoid_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Interface_Normal_thicknessRolesEnm
{
    ads_Layer_Interface_Normal_thickness_child,
    ads_Layer_Interface_Normal_thickness_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Interface_Radial_radialClearanceRolesEnm
{
    ads_Layer_Interface_Radial_radialClearance_child,
    ads_Layer_Interface_Radial_radialClearance_parent
};

/** 
Enum with record members. */
enum ads_Layer_Rebar_AngularMembersEnm
{
    ads_Layer_Rebar_Angular_isoparametricDirection
};

enum ads_Layer_Rebar_Angular_isoparametricDirectionEnm
{
    ads_Layer_Rebar_Angular_isoparametricDirection_DIRECTION_1,
    ads_Layer_Rebar_Angular_isoparametricDirection_DIRECTION_2,
    ads_Layer_Rebar_Angular_isoparametricDirection_DIRECTION_3
};

/** 
Enum with association roles. */
enum ads_Layer_Rebar_Angular_angularSpacingRolesEnm
{
    ads_Layer_Rebar_Angular_angularSpacing_child,
    ads_Layer_Rebar_Angular_angularSpacing_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Rebar_Angular_areaRolesEnm
{
    ads_Layer_Rebar_Angular_area_child,
    ads_Layer_Rebar_Angular_area_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Rebar_Angular_positionRolesEnm
{
    ads_Layer_Rebar_Angular_position_child,
    ads_Layer_Rebar_Angular_position_parent
};

/** 
Enum with record members. */
enum ads_Layer_Rebar_ConstantMembersEnm
{
    ads_Layer_Rebar_Constant_isoparametricDirection
};

enum ads_Layer_Rebar_Constant_isoparametricDirectionEnm
{
    ads_Layer_Rebar_Constant_isoparametricDirection_DIRECTION_1,
    ads_Layer_Rebar_Constant_isoparametricDirection_DIRECTION_2,
    ads_Layer_Rebar_Constant_isoparametricDirection_DIRECTION_3
};

/** 
Enum with association roles. */
enum ads_Layer_Rebar_Constant_areaRolesEnm
{
    ads_Layer_Rebar_Constant_area_child,
    ads_Layer_Rebar_Constant_area_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Rebar_Constant_positionRolesEnm
{
    ads_Layer_Rebar_Constant_position_child,
    ads_Layer_Rebar_Constant_position_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Rebar_Constant_spacingRolesEnm
{
    ads_Layer_Rebar_Constant_spacing_child,
    ads_Layer_Rebar_Constant_spacing_parent
};

/** 
Enum with record members. */
enum ads_Layer_Rebar_LiftEquationMembersEnm
{
    ads_Layer_Rebar_LiftEquation_isoparametricDirection
};

enum ads_Layer_Rebar_LiftEquation_isoparametricDirectionEnm
{
    ads_Layer_Rebar_LiftEquation_isoparametricDirection_DIRECTION_1,
    ads_Layer_Rebar_LiftEquation_isoparametricDirection_DIRECTION_2,
    ads_Layer_Rebar_LiftEquation_isoparametricDirection_DIRECTION_3
};

/** 
Enum with association roles. */
enum ads_Layer_Rebar_LiftEquation_areaRolesEnm
{
    ads_Layer_Rebar_LiftEquation_area_child,
    ads_Layer_Rebar_LiftEquation_area_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Rebar_LiftEquation_extensionRatioRolesEnm
{
    ads_Layer_Rebar_LiftEquation_extensionRatio_child,
    ads_Layer_Rebar_LiftEquation_extensionRatio_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Rebar_LiftEquation_positionRolesEnm
{
    ads_Layer_Rebar_LiftEquation_position_child,
    ads_Layer_Rebar_LiftEquation_position_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Rebar_LiftEquation_radiusRolesEnm
{
    ads_Layer_Rebar_LiftEquation_radius_child,
    ads_Layer_Rebar_LiftEquation_radius_parent
};

/** 
Enum with association roles. */
enum ads_Layer_Rebar_LiftEquation_spacingRolesEnm
{
    ads_Layer_Rebar_LiftEquation_spacing_child,
    ads_Layer_Rebar_LiftEquation_spacing_parent
};

/** Enum with association roles. */
enum ads_Layer_Rebar_Line_areaRolesEnm
{
    ads_Layer_Rebar_Line_area_child,
    ads_Layer_Rebar_Line_area_parent
};

/** Enum with association roles. */
enum ads_Layer_Rebar_Line_locX1RolesEnm
{
    ads_Layer_Rebar_Line_locX1_child,
    ads_Layer_Rebar_Line_locX1_parent
};

/** Enum with association roles. */
enum ads_Layer_Rebar_Line_locX2RolesEnm
{
    ads_Layer_Rebar_Line_locX2_child,
    ads_Layer_Rebar_Line_locX2_parent
};

/** 
Enum with association roles. */
enum ads_Pipe_taper_rRolesEnm
{
    ads_Pipe_taper_r_child,
    ads_Pipe_taper_r_parent
};

/** 
Enum with association roles. */
enum ads_Pipe_taper_tRolesEnm
{
    ads_Pipe_taper_t_child,
    ads_Pipe_taper_t_parent
};

/** 
Enum with association roles. */
enum ads_Rectangular_taper_aRolesEnm
{
    ads_Rectangular_taper_a_child,
    ads_Rectangular_taper_a_parent
};

/** 
Enum with association roles. */
enum ads_Rectangular_taper_bRolesEnm
{
    ads_Rectangular_taper_b_child,
    ads_Rectangular_taper_b_parent
};

/** 
Enum with association roles. */
enum ads_Thick_Pipe_taper_rRolesEnm
{
    ads_Thick_Pipe_taper_r_child,
    ads_Thick_Pipe_taper_r_parent
};

/** 
Enum with association roles. */
enum ads_Thick_Pipe_taper_tRolesEnm
{
    ads_Thick_Pipe_taper_t_child,
    ads_Thick_Pipe_taper_t_parent
};

/** 
Enum with association roles. */
enum ads_Trapezoidal_taper_aRolesEnm
{
    ads_Trapezoidal_taper_a_child,
    ads_Trapezoidal_taper_a_parent
};

/** 
Enum with association roles. */
enum ads_Trapezoidal_taper_bRolesEnm
{
    ads_Trapezoidal_taper_b_child,
    ads_Trapezoidal_taper_b_parent
};

/** 
Enum with association roles. */
enum ads_Trapezoidal_taper_cRolesEnm
{
    ads_Trapezoidal_taper_c_child,
    ads_Trapezoidal_taper_c_parent
};

/** 
Enum with association roles. */
enum ads_Trapezoidal_taper_dRolesEnm
{
    ads_Trapezoidal_taper_d_child,
    ads_Trapezoidal_taper_d_parent
};

#endif
