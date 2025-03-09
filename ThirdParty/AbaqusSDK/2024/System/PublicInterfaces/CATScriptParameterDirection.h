#ifndef CATScriptParameterDirection_h
#define CATScriptParameterDirection_h

// COPYRIGHT DASSAULT SYSTEMES 2000

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

/**
 * Possible directions for the parameters used for the macros generation.
 * @param CATParamIn
 * In parameter.
 * @param CATParamOut
 * Out parameter.
 * @param CATParamInOut
 * InOut parameter.
 * @see CATIScriptMethodCall
 */
enum CATScriptParameterDirection{CATParamIn,
                                 CATParamOut,
                                 CATParamInOut};

#endif

