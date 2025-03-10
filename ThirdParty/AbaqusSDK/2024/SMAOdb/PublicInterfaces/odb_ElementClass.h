//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_ElementClass_h
#define odb_ElementClass_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <bme_ElementData.h>

class odb_ElementClass  : public mem_AllocationOperators
{
public:   
    const cow_String & typeLabel() const;
    const cow_String & categoryLabel() const;
    int numElements() const;
    int numNodesPerElement() const;
    int numIntegrationPointsPerElement() const;
    int numFacesPerElement() const;
    int maxIntegrationPointsPerFace() const;
    int numIntegrationPointsPerFace(int face) const;

private:
    bme_ElementDataCOW  m_data;
    int m_index;

public:   // undocumented and unsupported
    odb_ElementClass();
    odb_ElementClass(const bme_ElementData& data, int index);

};

#endif // #ifndef odb_ElementClass_h
