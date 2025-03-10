//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
///////////////////////////////////////////////////////////////////////////////
//
// File Name: bme_Types.h
//
// Description:
//
//   This header file defines the variable types and enumerations used only in
//   the KERNEL portion of the orphan mesh module.
//

#ifndef bme_Types_h
#define bme_Types_h

//
// SECTION: System Includes
//

//
// Begin Local Includes
//
#include <omi_types.h>
#include <omi_limits.h>

// End local includes

//
// SECTION: Forward declarations
//

//
// SECTION: Type defs
//
typedef  unsigned int bme_BitVar;

//
// SECTION: Constants
//

// The following is used in hash tables to indicate an empty bin.
const  uint  bme_EmptyBin = INT_MAX;

//
// SECTION: Structs
//

// Face
struct  bme_Face
{
    bme_BitVar  ElementIndex : 29;
    bme_BitVar  FaceIndex    :  3;
};

// Edgelist data structure
struct bme_Edge
{
    bme_BitVar  ElementIndex : 27;
    bme_BitVar  FaceIndex    :  3;
    bme_BitVar  EdgeIndex    :  2;
};

//
// SECTION: Enumerations
//

// Type of Edge display
enum bme_EdgeDisplayEnum {
    EdgeOrphan = 1000,
    EdgeFeature,
    EdgeExterior,
    EdgeAll,
    EdgeNone
};
	
// Type of component (needed in picking)
enum bme_CType {
    BmeNode = 1000,
    BmeEdge,
    BmeFace,
    BmeElement,
    BmeMesh,
    BmeNone
};

enum bme_BeamProfileEnm
{
    bme_BoxBeam = 1,
    bme_PipeBeam,
    bme_CircularBeam,
    bme_RectangularBeam,
    bme_HexagonalBeam,
    bme_TrapezoidalBeam,
    bme_IBeam,
    bme_TBeam,
    bme_LBeam,
    bme_ArbitraryBeam,
    bme_GeneralizedBeam,
    bme_TaperedBeam,
    bme_CBeam,
    bme_HatBeam,
    bme_NoneBeamProfile
};

#endif /* bme_Types_h */
