//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_Node_h
#define odb_Node_h

// Begin local includes
#include <mem_AllocationOperatorsTiny.h>
#include <odb_Types.h>


// Forward declarations
class ddb_Node;
class odb_Assembly;

class odb_Node : public mem_AllocationOperatorsTiny
{
 public:

    // documented and supported user interfaces

    // member access
    int label() const { return _userLabel; };
    odb_SequenceFloat coordinate() const { return odb_SequenceFloat(_coord, 3); };
    const float* coordinates() const { return _coord; };
	

    // comparison operator
    bool operator==(const odb_Node& rhs) const { return !notEqual(rhs); };
    bool operator!=(const odb_Node& rhs) const { return notEqual(rhs); };
	
    // undocumented and unsupported

    odb_Node();
    odb_Node(int index, int userLabel, int meshIndex, const float* coord);
    odb_Node(const ddb_Node& node);

    int index() const { return _index; };
    int meshIndex() const { return _meshIndex; };
    ddb_Node node() const;
    bool notEqual(const odb_Node& rhs) const;
	odb_String instanceName(const odb_Assembly& assembly) const;

 private:
    int _index;
    int _userLabel;
    int _meshIndex;
    float _coord[3];
};

#endif // odb_Node_h
