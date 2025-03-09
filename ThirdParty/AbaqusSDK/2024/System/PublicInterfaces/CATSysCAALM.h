#ifndef CAALM_INCLUDE
#define CAALM_INCLUDE

// Copyright Dassault Systemes 2001
/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */
#include "JS0CATLM.h"

/**
 * Class providing access to CATIA's licensing subsystem. 
 */
class ExportedByJS0CATLM CATSysCAALM {

public:

    /**
      * Status code returned.
      * @param success
      *   The requested action has completed successfully.
      * @param error
      *   Catch-all error code.
      * @param notInitialized
      *   <tt>Initialize</tt> has not been called.
      * @param alreadyInitialized
      *   The licensing subsystem is already initialized by way of Initialize
      *   or another CATIA's function. CAALM might not function.
      * @param invalidPartner
      *   The specified CAA2 partner name and key are not recognized.
      * @param invavlidProduct
      *   The specified product is unknown or cannot be handled by the current
      *   CAA2 partner.
      * @param invalidICPath
      *   The specified directoty does not contain any product identity card.
      * @param invalidHandle
      *   The specified Handle variable does not contain valid data.
      */
    enum CATSysCAALMStatus {
        success = 0,
        error,
        notInitialized,
        alreadyInitialized,
        notRequested,
        invalidPartner,
        invalidProduct,
        invalidICPath,
        invalidHandle
    };

    /**
      * This is the function to be implemented by the CAA2 partner and passed
      * to <a href="#Initialize(char*,unsigned long,RequestCallback,Handle&)">Initialize</a>. It is not part of CATSysCAALM but included here only for
      * documentation purpose.
      * <br>
      * It is called when the licensing subsystem needs to probe, request,
      * check, release... a license. Data returned represents a boolean 
      * indicating whether the operation has succeeded or failed.
      * @param iProductName
      *   The name of the product of the form ABC.prd
      * @param iProductID
      *   The 32-bit integer specified by the SetLicense of the product
      *   identity card.
      * @param ioData
      *   Two 32-bit integers that are used for verification. If the operation
      *   succeeds then ioData[0] must be XORed with the partner key
      *   and ioData[1] must also by XORed the product ID (2nd parameter).
      *   Any other value implies that the operation has failed.<br>
      *   On input, bits 12-15 of the first word ((ioData[0] >> 12) & 0xf)
      *   represents a value describing the operation to be performed:
      *   <DIR>
      *      <LI>0 - check that the license is present
      *      <LI>1 - request the license
      *      <LI>2 - check that the license (previously requested) is still granted
      *      <LI>3 - release the license (previously requested)
      *      <LI>4 - check if the product must be displayed in Tools/Options/Licensing tab (success -> displayed)
      *   </DIR>
      *
      * Here is an example:
      * int callback (const char *iProductName,
                      unsigned long iProductID,
                      unsigned long ioData[2]);
      */
    typedef int (*RequestCallback) (const char *iProductName,
                                    unsigned long iProductID,
                                    unsigned long ioData[2]);

    /**
      * Type that will contain the CAA2 partner data.
      */
    typedef void *Handle;

    /**
      * This is not a function member of the CATSysCAALM class but an entry 
      * point to be defined in a shared library.<br>
      * Partners must normally deliver a shared library with a name
      * of the form <B><EM>ABC</EM>_LM</B> where <B><EM>ABC</EM></B> is the
      * partner name as defined in the product identification card.
      * The purpose of this entry point is to let the partner code perform all
      * required initializations and to call <a href="#Initialize(char*,unsigned long,RequestCallback,Handle&)">Initialize</a>.<br>
      * Please note that <tt>LM_Setup</tt> should be declared as <tt>extern "C"</tt>.
      * On Windows systems, it should also be declared as an exported symbol
      * with <tt>__declspec (export)</tt>.
      */
    void LM_Setup ();

    /**
      * Initializes the licensing subsystem with the CAA2 partner data.
      * This call should only be made by the <a href="#LM_Setup()">LM_Setup</a>
      * entry point.
      * @param iPartnerName
      *   The name of the CAA2 partner as specified by the SetPartner
      *   in the product identity card.
      * @param iPartnerKey
      *   The confidential 32-bit integer assigned specifically to the
      *   CAA2 partner by DS.
      * @param iRCB
      *   The pointer to a function implemented by the CAA2 partner that will
      *   be called for license operations. See <a href="#callback(char*,unsigned long,unsigned long[2])">RequestCallback</a> for details.
      * @param ohandle
      *   A Handle variable that will contain the CAA2 partner data upon 
      *   return.
      * @return
      *   success<br>
      *   invalidPartner<br>
      */
    static int    Initialize (const char *iPartnerName,
                              unsigned long iPartnerKey,
                              RequestCallback iRCB,
                              Handle& ohandle);

    /**
      * Initializes the licensing subsystem. This will call the  <a href="#LM_Setup()">LM_Setup</a>
      * entry point. Use this method in non-interactive mode.
      * @return
      *   success<br>
      *   error<br>
      */
    static int    Start();


    /**
      * Releases licenses requested and resources used by the CAA2 partner.
      * @param iohandle
      *   The Handle variable that was passed to <a href="#Initialize(char*,unsigned long,RequestCallback,Handle&)">Initialize</a>. 
      * @return
      *   success<br>
      *   invalidHandle<br>
      */
    static int    Stop (Handle ihandle);

    /**
      * Requests a product for enabling.
      * @param ihandle
      *   The Handle variable that was passed to <a href="#Initialize(char*,unsigned long,RequestCallback,Handle&)">Initialize</a>.
      * @param iProductName
      *   The name of the product like ABC.prd
      * @return
      *   sucess<br>
      *   error - data returned by the callback is invalid.<br>
      *   notInilialized<br>
      *   invalidProduct<br>
      *   invalidHandle<br>
      */
    static int    RequestProduct (Handle ihandle, const char *iProductName);


    /**
      * Releases a product that is enabled.
      * @param ihandle
      *   The Handle variable that was passed to <a href="#Initialize(char*,unsigned long,RequestCallback,Handle&)">Initialize</a>.
      * @param iProductName
      *   The name of the product like ABC.prd
      * @return
      *   sucess<br>
      *   error - data returned by the callback is invalid.<br>
      *   notInilialized<br>
      *   invalidProduct<br>
      *   invalidHandle<br>
      */
    static int    ReleaseProduct (Handle ihandle, const char *iProductName);


    /**
      * Updates the workbenches, works only with a CATInteractiveApplication.
      * @return
      *   sucess<br>
      *   error - unable to update workbenches.<br>
      */
    static int    UpdateWorkbenches();


    /**
      * Checks whether a product is enabled or not.
      * @param iProductName
      *   The name of the product (ie "ABC.prd")
      * @return
      *   0 - the product is not enabled.<br>
      *   non 0 - the product is enabled.
      */
    static int    IsProductAuthorized (const char *iproductName);

};

#endif
