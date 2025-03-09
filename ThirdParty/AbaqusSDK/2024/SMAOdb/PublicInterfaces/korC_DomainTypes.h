//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/*   -*- mode: c++ -*-   */
#ifndef korC_DomainTypes_h
#define korC_DomainTypes_h

// Includes

// Begin local includes

// Enum order here will also determine order in the output request GUI
enum korC_DomainTypeEnm
{
  korC_DOMAIN_TYPE_WHOLE_MODEL = 0,
  korC_DOMAIN_TYPE_SET,
  korC_DOMAIN_TYPE_BOLT_LOAD,
  korC_DOMAIN_TYPE_COMPOSITE_LAYUP,
  korC_DOMAIN_TYPE_CONTOUR_INTEGRAL,
  korC_DOMAIN_TYPE_FASTENER,
  korC_DOMAIN_TYPE_GENERAL_CONTACT_SURFACE,
  korC_DOMAIN_TYPE_INTEGRATED_OUTPUT_SECTION,
  korC_DOMAIN_TYPE_INTERACTION,
  korC_DOMAIN_TYPE_SKIN,
  korC_DOMAIN_TYPE_SPRING,
  korC_DOMAIN_TYPE_STRINGER,
  korC_DOMAIN_TYPE_SUBSTRUCTURE,
  korC_DOMAIN_TYPE_ASSEMBLED_FASTENER,

  korC_DOMAIN_TYPE_NONE,

  // OBSOLETE value below:
  //   TO DO: Remove when named connectors are eliminated in 6.6
  korC_DOMAIN_TYPE_CONNECTOR,

  // Last
  korC_DOMAIN_TYPE_LAST
};

bool korC_IsRegionBasedDomain( korC_DomainTypeEnm val );
#endif
// kseC_DomainTypes_h
