#ifndef CATGetBacboneConnection_H
#define CATGetBacboneConnection_H

// COPYRIGHT DASSAULT SYSTEMES 2000

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

#include "CATICommunicator.h" 
/**
 * Create the backbone bus  connexion.
 * <br><b>Role</b>: This method instanciates the bus connection
 * @param CATBusDescriptor
 *    <b>Legal Values</b>:<tt>CATDefaultBus</tt>
 * @param iFlags
 *    <b>Legal Values</b>: 0
 * @return
 *   the pointer to a CATICommunicator corresponding to the logical bus
 *   in the application
 */
ExportedByCATSysCommunication CATICommunicator *CATGetBackboneConnection(
                   CATBusDescriptor iDesc=CATDefaultBus ,
                   int iFlags=0);

#endif
