//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ads_CoreUnitC_h
#define ads_CoreUnitC_h

// Begin local includes
#include <ads_CoreFragments.h>

/** All record and association types, for the fragment Unit of the latest level of form Core */

/** Library for string-keyed physical dimensions. */
#define ads_Focus_physicalDimensionLib (ads_CoreFragmentTypeIndex(ads_CoreUnitFragment, 0))

/** String-keyed unit system for document. */
#define ads_Focus_unitSystem (ads_CoreFragmentTypeIndex(ads_CoreUnitFragment, 1))

/** Represents a PhysicalDimension, in the sense of length, time, force etc. Every physical dimension represents a formula in powers of the seven base physical dimensions, length, mass, time, electric current, temperature, amount of substance, and luminous intensity. Each exponent can be fractional, and is represented by a numerator-demominator pair. The denominator is always positive, and is as small as it can be to represent the fraction correctly. There are definite rules for the construction of the string which provides the symbol name of a physical dimension. The rules allow a physical dimension to be determined from its symbol name and vice-versa. These rules are as follows: For the base physical dimensions, index temperature and the dimensionless physical dimension, the symbol names are as specified individually below. For all other physical dimensions, in order to construct its name: 1. Write its formula in the base physical dimensions, representing the base physical dimensions through their own symbol names. 2. Omit any exponents of value 1 in the formula; include no base dimensions which have an exponent 0 in the formula. 3. Arrange the formula so the base physical dimensions appear in the order they appear below, that is, M..L..T..E..K..A..C. 4. Write down what you see from left to right, each base symbol followed immediately by what we'll call the symbol form of its exponent: 5. To put an exponent in symbol form, as a fraction it must be normalized; represent minus in the numerator with a prefix 0. If the denominator is 1, only write the numerator; otherwise, write the numerator, an underscore, and the denominator. */
#define ads_PhysicalDimension (ads_CoreFragmentTypeIndex(ads_CoreUnitFragment, 2))

/** Focus based library of string keyed Physical Dimension records */
#define ads_PhysicalDimensionLib (ads_CoreFragmentTypeIndex(ads_CoreUnitFragment, 3))

/** String-keyed Physical Dimensions. */
#define ads_PhysicalDimensionLib_physicalDimensions (ads_CoreFragmentTypeIndex(ads_CoreUnitFragment, 4))

/** Represents a UnitSystem, in the sense of meter, kilogram, second etc. */
#define ads_UnitSystem (ads_CoreFragmentTypeIndex(ads_CoreUnitFragment, 5))

/** 
Enum with association roles. */
enum ads_Focus_physicalDimensionLibRolesEnm
{
    ads_Focus_physicalDimensionLib_child,
    ads_Focus_physicalDimensionLib_parent
};

/** 
Enum with association roles. */
enum ads_Focus_unitSystemRolesEnm
{
    ads_Focus_unitSystem_child,
    ads_Focus_unitSystem_parent
};

/** 
Enum with record members. */
enum ads_PhysicalDimensionMembersEnm
{
    ads_PhysicalDimension_amountOfSubstanceDenom,
    ads_PhysicalDimension_amountOfSubstanceNum,
    ads_PhysicalDimension_angleDenom,
    ads_PhysicalDimension_angleNum,
    ads_PhysicalDimension_electricCurrentDenom,
    ads_PhysicalDimension_electricCurrentNum,
    ads_PhysicalDimension_isIndex,
    ads_PhysicalDimension_lengthDenom,
    ads_PhysicalDimension_lengthNum,
    ads_PhysicalDimension_luminousIntensityDenom,
    ads_PhysicalDimension_luminousIntensityNum,
    ads_PhysicalDimension_massDenom,
    ads_PhysicalDimension_massNum,
    ads_PhysicalDimension_solidAngleDenom,
    ads_PhysicalDimension_solidAngleNum,
    ads_PhysicalDimension_temperatureDenom,
    ads_PhysicalDimension_temperatureNum,
    ads_PhysicalDimension_timeDenom,
    ads_PhysicalDimension_timeNum
};

/** 
Enum with association roles. */
enum ads_PhysicalDimensionLib_physicalDimensionsRolesEnm
{
    ads_PhysicalDimensionLib_physicalDimensions_child,
    ads_PhysicalDimensionLib_physicalDimensions_parent
};

/** 
Enum with record members. */
enum ads_UnitSystemMembersEnm
{
    ads_UnitSystem_amtOfSubToSIConvFactor,
    ads_UnitSystem_angleToSIConvFactor,
    ads_UnitSystem_electricCurrentToSIConvFactor,
    ads_UnitSystem_lengthToSIConvFactor,
    ads_UnitSystem_lumIntensityToSIConvFactor,
    ads_UnitSystem_massToSIConvFactor,
    ads_UnitSystem_solidAngleToSIConvFactor,
    ads_UnitSystem_temperatureToSIConvFactor,
    ads_UnitSystem_temperatureToSIOffset,
    ads_UnitSystem_timeToSIConvFactor
};

#endif
