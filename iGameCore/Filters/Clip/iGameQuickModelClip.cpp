#include "iGameQuickModelClip.h"
#include "iGameBasedCellClipCases.h"

IGAME_NAMESPACE_BEGIN

#define LIST_INIT_SIZE 4096
#define INIT_SIZE_PER_LIST 1024;
#define HASH_LIST_SIZE 8192
#define POOL_SIZE 512

//QuickClipperPointList::edge中的插值点列表，内部存储为QuickClipperInterpolatePoint
//QuickClipperEdgeHash::存储edge的两个顶点以及中间插值点的id
//QuickClipperEdgeHashTable::edge的hash表
//QuickClipperCellList::Cell列表，内部用igIndex存储数据
//QuickClipperCenterPointList::Cell的中心点列表，内部存储为QuickClipperCenterPoint


struct QuickClipperInterpolatePoint {
    igIndex m_PointId[2]{0, 0};
    double m_Percent{0.0};
};

class QuickClipperPointList {
public:
    QuickClipperPointList();
    virtual ~QuickClipperPointList();

    igIndex AddPoint(igIndex, igIndex, double);
    igIndex GetNumberOfPoints() const { return m_CurrentListId * m_PointNumPerList + m_CurrentListPointNum; };
    int GetNumberOfLists() const { return m_CurrentListId + 1; };
    //根据listid得到list，同时返回该list的点数量
    int GetList(int, const QuickClipperInterpolatePoint*&) const;

protected:
    int m_CurrentListId{0};
    igIndex m_CurrentListPointNum{0};
    int m_ListSize{0};
    int m_PointNumPerList{0};
    QuickClipperInterpolatePoint** m_List{nullptr};
};
QuickClipperPointList::QuickClipperPointList() {
    m_ListSize = LIST_INIT_SIZE;
    m_PointNumPerList = INIT_SIZE_PER_LIST;
    m_List = new QuickClipperInterpolatePoint*[m_ListSize];
    m_List[0] = new QuickClipperInterpolatePoint[m_PointNumPerList];
    for (int i = 1; i < m_ListSize; i++) { m_List[i] = nullptr; }
    m_CurrentListId = 0;
    m_CurrentListPointNum = 0;
};
QuickClipperPointList::~QuickClipperPointList() {
    int i = 0;
    while (m_List[i] && i < m_ListSize) { delete[] m_List[i++]; }
    delete m_List;
}
igIndex QuickClipperPointList::AddPoint(igIndex p0, igIndex p1, double percent) {
    //当前list放满了
    if (m_CurrentListPointNum >= m_PointNumPerList) {
        //没有空的list了，重新开空间，原理同于vector
        if ((m_CurrentListId + 1) >= m_ListSize) {
            QuickClipperInterpolatePoint** newList = new QuickClipperInterpolatePoint*[2 * m_ListSize];
            for (int i = 0, j = m_ListSize; i < m_ListSize; i++, j++) {
                newList[i] = m_List[i];
                newList[j] = nullptr;
            }
            m_ListSize *= 2;
            delete[] m_List;
            m_List = newList;
        }
        m_CurrentListId++;
        m_List[m_CurrentListId] = new QuickClipperInterpolatePoint[m_PointNumPerList];
        m_CurrentListPointNum = 0;
    }
    m_List[m_CurrentListId][m_CurrentListPointNum].m_PointId[0] = p0;
    m_List[m_CurrentListId][m_CurrentListPointNum].m_PointId[1] = p1;
    m_List[m_CurrentListId][m_CurrentListPointNum].m_Percent = percent;
    m_CurrentListPointNum++;
    return GetNumberOfPoints() - 1;
}
int QuickClipperPointList::GetList(int listId, const QuickClipperInterpolatePoint*& list) const {
    if (listId < 0 || listId > m_CurrentListId) {
        list = nullptr;
        return 0;
    }
    list = m_List[listId];
    if (listId == m_CurrentListId) return m_CurrentListPointNum;
    else
        return m_PointNumPerList;
}

class QuickClipperEdgeHash {
public:
    QuickClipperEdgeHash() = default;
    virtual ~QuickClipperEdgeHash() = default;

    int GetPointId() { return m_PointId; }
    void SetData(int id1, int id2, int pid) {
        m_id1 = id1;
        m_id2 = id2;
        m_PointId = pid;
        next = nullptr;
    }
    bool IsEqual(int id1, int id2) { return m_id1 == id1 && m_id2 == id2; }
    void SetNextEdge(QuickClipperEdgeHash* e) { this->next = e; }
    QuickClipperEdgeHash* GetNextEdge() { return this->next; }

protected:
    //Edge的两个点，保证m_id1<m_id2
    int m_id1{-1}, m_id2{-1};
    //Edge上的插值点的id
    int m_PointId{-1};
    QuickClipperEdgeHash* next{nullptr};
};


//内存管理，还没写完
class QuickClipperEdgeHashMemoryManager {
public:
    QuickClipperEdgeHashMemoryManager() {};
    virtual ~QuickClipperEdgeHashMemoryManager() {
        int npools = static_cast<int>(edgeHashEntrypool.size());
        for (int i = 0; i < npools; i++) {
            QuickClipperEdgeHash* pool = edgeHashEntrypool[i];
            delete[] pool;
        }
    };

    void AllocateEdgeHashEntryPool() {
        if (freeEntryindex == 0) {
            QuickClipperEdgeHash* newlist = new QuickClipperEdgeHash[POOL_SIZE];
            edgeHashEntrypool.push_back(newlist);

            for (int i = 0; i < POOL_SIZE; i++) { freeEntrylist[i] = &(newlist[i]); }

            freeEntryindex = POOL_SIZE;
        }
    }
    inline QuickClipperEdgeHash* GetFreeEdgeHashEntry() {
        if (freeEntryindex <= 0) { AllocateEdgeHashEntryPool(); }
        freeEntryindex--;
        return freeEntrylist[freeEntryindex];
    }

protected:
    int freeEntryindex = 0;
    QuickClipperEdgeHash* freeEntrylist[HASH_LIST_SIZE];
    std::vector<QuickClipperEdgeHash*> edgeHashEntrypool;
};

class QuickClipperEdgeHashTable {
public:
    QuickClipperEdgeHashTable(int, QuickClipperPointList&);
    virtual ~QuickClipperEdgeHashTable();

    igIndex AddPoint(igIndex, igIndex, double);
    QuickClipperPointList& GetPointList() { return m_PointList; };

protected:
    int GetHashKey(igIndex, igIndex);

    int m_HashNum;
    QuickClipperPointList& m_PointList;
    QuickClipperEdgeHash** m_EdgeHash;
    QuickClipperEdgeHashMemoryManager m_EHMManager;
};
QuickClipperEdgeHashTable::QuickClipperEdgeHashTable(int hashNum, QuickClipperPointList& pointList)
    : m_PointList(pointList) {
    m_HashNum = hashNum;
    m_EdgeHash = new QuickClipperEdgeHash*[m_HashNum];
    for (int i = 0; i < m_HashNum; i++) { m_EdgeHash[i] = nullptr; }
}
QuickClipperEdgeHashTable::~QuickClipperEdgeHashTable() {
    //第二维的hash表靠内存管理器来处理
    delete[] m_EdgeHash;
}
igIndex QuickClipperEdgeHashTable::AddPoint(igIndex p0, igIndex p1, double percent) {
    //统一让小的在前面
    if (p1 < p0) {
        std::swap(p0, p1);
        percent = 1 - percent;
    }
    int key = this->GetHashKey(p0, p1);
    QuickClipperEdgeHash* current = m_EdgeHash[key];
    while (current) {
        if (current->IsEqual(p0, p1)) { return current->GetPointId(); }
        current = current->GetNextEdge();
    }
    QuickClipperEdgeHash* newHash = m_EHMManager.GetFreeEdgeHashEntry();

    igIndex newPoint = m_PointList.AddPoint(p0, p1, percent);
    newHash->SetData(p0, p1, newPoint);
    newHash->SetNextEdge(m_EdgeHash[key]);
    m_EdgeHash[key] = newHash;
    return newPoint;
}
int QuickClipperEdgeHashTable::GetHashKey(igIndex p0, igIndex p1) {
    //自定义hash表
    unsigned int res = ((unsigned int) p0 * 42911U + (unsigned int) p1 * 235711U) % m_HashNum;
    int key = int(res);
    //hash乘法的时候可能会出现溢出的情况，强转可能会小于0？
    //if (key < 0) {
    // std::cout<<"overfloaw\n";
    //	key+= m_HashNum;
    //}
    return key;
}

class QuickClipperDataObjectFromVolume {
public:
    QuickClipperDataObjectFromVolume(igIndex initSize);
    QuickClipperDataObjectFromVolume(igIndex pointNum, igIndex initSize);
    virtual ~QuickClipperDataObjectFromVolume() = default;

    igIndex AddPoint(igIndex p0, igIndex p1, double percent) {
        return m_PrevPointsNum + m_Edges.AddPoint(p0, p1, percent);
    }

protected:
    int m_PrevPointsNum;
    QuickClipperPointList m_PointList;
    QuickClipperEdgeHashTable m_Edges;
};

QuickClipperDataObjectFromVolume::QuickClipperDataObjectFromVolume(igIndex initSize)
    : m_PointList(), m_Edges(initSize, m_PointList) {
    m_PrevPointsNum = 0;
}
QuickClipperDataObjectFromVolume::QuickClipperDataObjectFromVolume(igIndex pointNum, igIndex initSize)
    : m_PointList(), m_Edges(initSize, m_PointList) {
    m_PrevPointsNum = pointNum;
}


class QuickClipperCellList {
public:
    QuickClipperCellList(int size);
    virtual ~QuickClipperCellList();
    virtual int GetIGAMEType() const = 0;

    int GetCellSize() const { return this->m_CellSize; }
    int GetNumberOfCells() const { return m_CurrentListId * m_CellNumPerList + m_CurrentListCellNum; }
    int GetNumberOfLists() const { return this->m_CurrentListId + 1; }
    //根据listid得到list，同时返回该list的点数量
    int GetList(int, const igIndex*&) const;
    void CheckListEnough();

protected:
    igIndex** m_List;
    int m_CurrentListId;
    int m_CurrentListCellNum;
    int m_ListSize;
    int m_CellNumPerList;
    int m_CellSize;
};

QuickClipperCellList::QuickClipperCellList(int size) {
    m_CellSize = size;
    m_ListSize = LIST_INIT_SIZE;
    m_CellNumPerList = INIT_SIZE_PER_LIST;

    m_List = new igIndex*[m_ListSize];
    m_List[0] = new igIndex[(m_CellSize + 1) * m_CellNumPerList];

    for (int i = 1; i < m_ListSize; i++) { m_List[i] = nullptr; }
    m_CurrentListId = 0;
    m_CurrentListCellNum = 0;
}
QuickClipperCellList::~QuickClipperCellList() {
    int i = 0;
    while (m_List[i] && i < m_ListSize) { delete[] m_List[i++]; }
    delete m_List;
}
int QuickClipperCellList::GetList(int listId, const igIndex*& list) const {
    if (listId < 0 || listId > m_CurrentListId) {
        list = nullptr;
        return 0;
    }
    list = m_List[listId];
    if (listId == m_CurrentListId) return m_CurrentListCellNum;
    else
        return m_CellNumPerList;
}
void QuickClipperCellList::CheckListEnough() {
    //当前list放满了
    if (m_CurrentListCellNum >= m_CellNumPerList) {
        //没有空的list了，重新开空间，原理同于vector
        if ((m_CurrentListId + 1) >= m_ListSize) {
            igIndex** newList = new igIndex*[2 * m_ListSize];
            for (int i = 0, j = m_ListSize; i < m_ListSize; i++, j++) {
                newList[i] = m_List[i];
                newList[j] = nullptr;
            }
            m_ListSize *= 2;
            delete[] m_List;
            m_List = newList;
        }
        m_CurrentListId++;
        m_List[m_CurrentListId] = new igIndex[m_CellNumPerList * (m_CellSize + 1)];
        m_CurrentListCellNum = 0;
    }
    return;
}
class QuickClipperTetraList : public QuickClipperCellList {
public:
    QuickClipperTetraList() : QuickClipperCellList(4) {};
    ~QuickClipperTetraList() = default;
    int GetIGAMEType() const override { return IG_TETRA; }
    void AddTetra(igIndex cellId, igIndex p0, igIndex p1, igIndex p2, igIndex p3) {
        CheckListEnough();
        int idx = (m_CellSize + 1) * m_CurrentListCellNum;
        m_List[m_CurrentListId][idx + 0] = cellId;
        m_List[m_CurrentListId][idx + 1] = p0;
        m_List[m_CurrentListId][idx + 2] = p1;
        m_List[m_CurrentListId][idx + 3] = p2;
        m_List[m_CurrentListId][idx + 4] = p3;
        m_CurrentListCellNum++;
    }
};
class QuickClipperHexList : public QuickClipperCellList {
public:
    QuickClipperHexList() : QuickClipperCellList(8) {};
    ~QuickClipperHexList() = default;
    int GetIGAMEType() const override { return IG_HEXAHEDRON; }
    void AddHex(igIndex cellId, igIndex p0, igIndex p1, igIndex p2, igIndex p3, igIndex p4, igIndex p5, igIndex p6,
                igIndex p7) {
        CheckListEnough();
        int idx = (m_CellSize + 1) * m_CurrentListCellNum;
        m_List[m_CurrentListId][idx + 0] = cellId;
        m_List[m_CurrentListId][idx + 1] = p0;
        m_List[m_CurrentListId][idx + 2] = p1;
        m_List[m_CurrentListId][idx + 3] = p2;
        m_List[m_CurrentListId][idx + 4] = p3;
        m_List[m_CurrentListId][idx + 5] = p4;
        m_List[m_CurrentListId][idx + 6] = p5;
        m_List[m_CurrentListId][idx + 7] = p6;
        m_List[m_CurrentListId][idx + 8] = p7;
        m_CurrentListCellNum++;
    }
};
class QuickClipperPrismList : public QuickClipperCellList {
public:
    QuickClipperPrismList() : QuickClipperCellList(6) {};
    ~QuickClipperPrismList() = default;
    int GetIGAMEType() const override { return IG_PRISM; }
    void AddPrism(igIndex cellId, igIndex p0, igIndex p1, igIndex p2, igIndex p3, igIndex p4, igIndex p5) {
        CheckListEnough();
        int idx = (m_CellSize + 1) * m_CurrentListCellNum;
        m_List[m_CurrentListId][idx + 0] = cellId;
        m_List[m_CurrentListId][idx + 1] = p0;
        m_List[m_CurrentListId][idx + 2] = p1;
        m_List[m_CurrentListId][idx + 3] = p2;
        m_List[m_CurrentListId][idx + 4] = p3;
        m_List[m_CurrentListId][idx + 5] = p4;
        m_List[m_CurrentListId][idx + 6] = p5;
        m_CurrentListCellNum++;
    }
};
class QuickClipperPyramidList : public QuickClipperCellList {
public:
    QuickClipperPyramidList() : QuickClipperCellList(5) {};
    ~QuickClipperPyramidList() = default;
    int GetIGAMEType() const override { return IG_PYRAMID; }
    void AddPyramid(igIndex cellId, igIndex p0, igIndex p1, igIndex p2, igIndex p3, igIndex p4) {
        CheckListEnough();
        int idx = (m_CellSize + 1) * m_CurrentListCellNum;
        m_List[m_CurrentListId][idx + 0] = cellId;
        m_List[m_CurrentListId][idx + 1] = p0;
        m_List[m_CurrentListId][idx + 2] = p1;
        m_List[m_CurrentListId][idx + 3] = p2;
        m_List[m_CurrentListId][idx + 4] = p3;
        m_List[m_CurrentListId][idx + 5] = p4;
        m_CurrentListCellNum++;
    }
};
class QuickClipperTriangleList : public QuickClipperCellList {
public:
    QuickClipperTriangleList() : QuickClipperCellList(3) {};
    ~QuickClipperTriangleList() = default;
    int GetIGAMEType() const override { return IG_TRIANGLE; }
    void AddTriangle(igIndex cellId, igIndex p0, igIndex p1, igIndex p2) {
        CheckListEnough();
        int idx = (m_CellSize + 1) * m_CurrentListCellNum;
        m_List[m_CurrentListId][idx + 0] = cellId;
        m_List[m_CurrentListId][idx + 1] = p0;
        m_List[m_CurrentListId][idx + 2] = p1;
        m_List[m_CurrentListId][idx + 3] = p2;
        m_CurrentListCellNum++;
    }
};
class QuickClipperQuadList : public QuickClipperCellList {
public:
    QuickClipperQuadList() : QuickClipperCellList(4) {};
    ~QuickClipperQuadList() = default;
    int GetIGAMEType() const override { return IG_QUAD; }
    void AddQuad(igIndex cellId, igIndex p0, igIndex p1, igIndex p2, igIndex p3) {
        CheckListEnough();
        int idx = (m_CellSize + 1) * m_CurrentListCellNum;
        m_List[m_CurrentListId][idx + 0] = cellId;
        m_List[m_CurrentListId][idx + 1] = p0;
        m_List[m_CurrentListId][idx + 2] = p1;
        m_List[m_CurrentListId][idx + 3] = p2;
        m_List[m_CurrentListId][idx + 4] = p3;
        m_CurrentListCellNum++;
    }
};
class QuickClipperLineList : public QuickClipperCellList {
public:
    QuickClipperLineList() : QuickClipperCellList(2) {};
    ~QuickClipperLineList() = default;
    int GetIGAMEType() const override { return IG_LINE; }
    void AddLine(igIndex cellId, igIndex p0, igIndex p1) {
        CheckListEnough();
        int idx = (m_CellSize + 1) * m_CurrentListCellNum;
        m_List[m_CurrentListId][idx + 0] = cellId;
        m_List[m_CurrentListId][idx + 1] = p0;
        m_List[m_CurrentListId][idx + 2] = p1;
        m_CurrentListCellNum++;
    }
};
class QuickClipperVertexList : public QuickClipperCellList {
public:
    QuickClipperVertexList() : QuickClipperCellList(1) {};
    ~QuickClipperVertexList() = default;
    int GetIGAMEType() const override { return IG_VERTEX; }
    void AddVertex(igIndex cellId, igIndex p0) {
        CheckListEnough();
        int idx = (m_CellSize + 1) * m_CurrentListCellNum;
        m_List[m_CurrentListId][idx + 0] = cellId;
        m_List[m_CurrentListId][idx + 1] = p0;
        m_CurrentListCellNum++;
    }
};

struct QuickClipperCenterPoint {
    igIndex pointNum;
    //最多只有六面体有八个点，这边不用vector，而是直接固定最大size
    int pIds[8];
};
class QuickClipperCenterPointList {
public:
    QuickClipperCenterPointList();
    virtual ~QuickClipperCenterPointList();

    igIndex AddPoint(igIndex, igIndex*);
    igIndex GetNumberOfPoints() const { return m_CurrentListId * m_PointNumPerList + m_CurrentListPointNum; };
    int GetNumberOfLists() const { return m_CurrentListId + 1; };
    //根据listid得到list，同时返回该list的点数量
    int GetList(int, const QuickClipperCenterPoint*&) const;

protected:
    int m_CurrentListId{0};
    igIndex m_CurrentListPointNum{0};
    int m_ListSize{0};
    int m_PointNumPerList{0};
    QuickClipperCenterPoint** m_List{nullptr};
};
QuickClipperCenterPointList::QuickClipperCenterPointList() {
    m_ListSize = LIST_INIT_SIZE;
    m_PointNumPerList = INIT_SIZE_PER_LIST;

    m_List = new QuickClipperCenterPoint*[m_ListSize];
    m_List[0] = new QuickClipperCenterPoint[m_PointNumPerList];
    for (int i = 1; i < m_ListSize; i++) { m_List[i] = nullptr; }

    m_CurrentListId = 0;
    m_CurrentListPointNum = 0;
}
QuickClipperCenterPointList::~QuickClipperCenterPointList() {
    int i = 0;
    while (m_List[i] && i < m_ListSize) { delete[] m_List[i++]; }
    delete m_List;
}
igIndex QuickClipperCenterPointList::AddPoint(igIndex pNum, igIndex* vhs) {
    //当前list放满了
    if (m_CurrentListPointNum >= m_PointNumPerList) {
        //没有空的list了，重新开空间，原理同于vector
        if ((m_CurrentListId + 1) >= m_ListSize) {
            QuickClipperCenterPoint** newList = new QuickClipperCenterPoint*[2 * m_ListSize];
            for (int i = 0, j = m_ListSize; i < m_ListSize; i++, j++) {
                newList[i] = m_List[i];
                newList[j] = nullptr;
            }
            m_ListSize *= 2;
            delete[] m_List;
            m_List = newList;
        }
        m_CurrentListId++;
        m_List[m_CurrentListId] = new QuickClipperCenterPoint[m_PointNumPerList];
        m_CurrentListPointNum = 0;
    }
    m_List[m_CurrentListId][m_CurrentListPointNum].pointNum = pNum;
    for (int i = 0; i < pNum; i++) { m_List[m_CurrentListId][m_CurrentListPointNum].pIds[i] = vhs[i]; }
    m_CurrentListPointNum++;
    return GetNumberOfPoints() - 1;
}
int QuickClipperCenterPointList::GetList(int listId, const QuickClipperCenterPoint*& list) const {
    if (listId < 0 || listId > m_CurrentListId) {
        list = nullptr;
        return 0;
    }
    list = m_List[listId];
    if (listId == m_CurrentListId) return m_CurrentListPointNum;
    else
        return m_PointNumPerList;
}

struct QuickClipperCommonPointsStructure {
    bool hasPtsList;
    double* pts_ptr;
    int* dims;
    double* X;
    double* Y;
    double* Z;
};


class QuickClipperVolumeFromVolume : public QuickClipperDataObjectFromVolume {
public:
    QuickClipperVolumeFromVolume(int precision, igIndex pointNum, igIndex initSize);
    ~QuickClipperVolumeFromVolume() override = default;


    igIndex AddCenterPoint(int n, igIndex* p) { return -1 - m_CenterPoints.AddPoint(static_cast<igIndex>(n), p); }
    void AddTetra(igIndex cellId, igIndex p0, igIndex p1, igIndex p2, igIndex p3) {
        this->m_Tetras.AddTetra(cellId, p0, p1, p2, p3);
    }
    void AddHex(igIndex cellId, igIndex p0, igIndex p1, igIndex p2, igIndex p3, igIndex p4, igIndex p5, igIndex p6,
                igIndex p7) {
        this->m_Hexes.AddHex(cellId, p0, p1, p2, p3, p4, p5, p6, p7);
    }
    void AddPrism(igIndex cellId, igIndex p0, igIndex p1, igIndex p2, igIndex p3, igIndex p4, igIndex p5) {
        this->m_Prisms.AddPrism(cellId, p0, p1, p2, p3, p4, p5);
    }
    void AddPyramid(igIndex cellId, igIndex p0, igIndex p1, igIndex p2, igIndex p3, igIndex p4) {
        this->m_Pyramids.AddPyramid(cellId, p0, p1, p2, p3, p4);
    }
    void AddTriangle(igIndex cellId, igIndex p0, igIndex p1, igIndex p2) {
        this->m_Triangles.AddTriangle(cellId, p0, p1, p2);
    }
    void AddQuad(igIndex cellId, igIndex p0, igIndex p1, igIndex p2, igIndex p3) {
        this->m_Quads.AddQuad(cellId, p0, p1, p2, p3);
    }
    void AddLine(igIndex cellId, igIndex p0, igIndex p1) { this->m_Lines.AddLine(cellId, p0, p1); }
    void AddVertex(igIndex cellId, igIndex p0) { this->m_Vertices.AddVertex(cellId, p0); }
    void ConstructUM(UnstructuredMesh::Pointer, AttributeSet::Pointer inData, Points::Pointer, CellArray::Pointer,
                     UnsignedIntArray::Pointer, CharArray::Pointer);
    void CopyAttributeSetData(igIndex outPointNum, igIndex outCellNum, AttributeSet::Pointer inData,
                              AttributeSet::Pointer outData, igIndex* ptLookup, igIndex edgePointStart,
                              igIndex centerStart, igIndex* originCell);

protected:
    QuickClipperCenterPointList m_CenterPoints;
    QuickClipperHexList m_Hexes;
    QuickClipperPrismList m_Prisms;
    QuickClipperPyramidList m_Pyramids;
    QuickClipperTetraList m_Tetras;
    QuickClipperQuadList m_Quads;
    QuickClipperTriangleList m_Triangles;
    QuickClipperLineList m_Lines;
    QuickClipperVertexList m_Vertices;

    //上面的八种，存储基类指针，并存储用了多少种类
    QuickClipperCellList* m_Cells[8];
    const int m_CellTypeNum;

    int m_OutPrecision;
};
QuickClipperVolumeFromVolume::QuickClipperVolumeFromVolume(int precision, igIndex pointNum, igIndex initSize)
    : m_OutPrecision(precision), QuickClipperDataObjectFromVolume(pointNum, initSize), m_CellTypeNum(8) {
    m_Cells[0] = &m_Tetras;
    m_Cells[1] = &m_Pyramids;
    m_Cells[2] = &m_Prisms;
    m_Cells[3] = &m_Hexes;
    m_Cells[4] = &m_Quads;
    m_Cells[5] = &m_Triangles;
    m_Cells[6] = &m_Lines;
    m_Cells[7] = &m_Vertices;
}

void QuickClipperVolumeFromVolume::ConstructUM(UnstructuredMesh::Pointer output, AttributeSet::Pointer inData,
                                               Points::Pointer inPoints, CellArray::Pointer inCells,
                                               UnsignedIntArray::Pointer inTypes, CharArray::Pointer CellVisible) {
    igIndex i = 0, j = 0, k = 0, l = 0;
    if (!output) return;
    auto cellVisible = CellVisible->RawPointer();
    auto inCellNum = inCells->GetNumberOfCells();
    igIndex* ptLookup = new igIndex[m_PrevPointsNum];
    std::fill(ptLookup, ptLookup + m_PrevPointsNum, -1);
    igIndex numUsed = 0;
    const igIndex* pts = nullptr;
    igIndex pNum = 0;
    for (i = 0; i < inCellNum; i++) {
        if (cellVisible[i] == 1) {
            pNum = inCells->GetCellIds(i, pts);
            for (j = 0; j < pNum; j++) {
                if (ptLookup[pts[j]] == -1) { ptLookup[pts[j]] = numUsed++; }
            }
        }
    }
    for (i = 0; i < m_CellTypeNum; i++) {
        int nlists = m_Cells[i]->GetNumberOfLists();
        int npts_per_shape = m_Cells[i]->GetCellSize();
        for (j = 0; j < nlists; j++) {
            const igIndex* list;
            int listSize = m_Cells[i]->GetList(j, list);
            for (k = 0; k < listSize; k++) {
                list++; // skip the cell id entry
                for (l = 0; l < npts_per_shape; l++) {
                    int pt = *list++;
                    if (pt >= 0 && pt < m_PrevPointsNum) {
                        if (ptLookup[pt] == -1) { ptLookup[pt] = numUsed++; }
                    }
                }
            }
        }
    }
    Points::Pointer outPoints = Points::New();
    igIndex centerStart = numUsed + m_PointList.GetNumberOfPoints();
    igIndex outPointNum = centerStart + m_CenterPoints.GetNumberOfPoints();
    outPoints->Resize(outPointNum);
    //std::copy(inPoints->RawPointer(), inPoints->RawPointer() + inPoints->GetNumberOfPoints() * 3, outPoints->RawPointer());

    for (i = 0; i < m_PrevPointsNum; i++) {
        if (ptLookup[i] == -1) { continue; }
        outPoints->SetPoint(ptLookup[i], inPoints->GetPoint(i));
    }

    igIndex ptIdx = numUsed;
    //
    // Now construct all the points that are along edges and new and add
    // them to the points list.
    //
    int nLists = m_PointList.GetNumberOfLists();
    for (i = 0; i < nLists; i++) {
        const QuickClipperInterpolatePoint* pe_list = nullptr;
        int nPts = m_PointList.GetList(i, pe_list);
        Point pt = {.0, .0, .0};
        double p = .0, bp = .0;
        for (j = 0; j < nPts; j++) {
            const QuickClipperInterpolatePoint& pe = pe_list[j];
            Point& pt1 = inPoints->GetPoint(pe.m_PointId[0]);
            Point& pt2 = inPoints->GetPoint(pe.m_PointId[1]);
            // Now that we have the original points, calculate the new one.
            p = pe.m_Percent;
            bp = 1.0 - p;
            pt[0] = pt1[0] * p + pt2[0] * bp;
            pt[1] = pt1[1] * p + pt2[1] * bp;
            pt[2] = pt1[2] * p + pt2[2] * bp;
            outPoints->SetPoint(ptIdx++, pt);
        }
    }
    //
    // Now construct the new "centroid" points and add them to the points list.
    //
    nLists = m_CenterPoints.GetNumberOfLists();
    for (i = 0; i < nLists; i++) {
        const QuickClipperCenterPoint* ce_list = nullptr;
        int nPts = m_CenterPoints.GetList(i, ce_list);

        for (j = 0; j < nPts; j++) {
            Point pts[8];
            Point pt = {0.0, 0.0, 0.0};
            double weights[8] = {.0};
            double weight_factor;
            const QuickClipperCenterPoint& ce = ce_list[j];
            weight_factor = 1.0 / ce.pointNum;
            for (k = 0; k < ce.pointNum; k++) {
                igIndex id = 0;
                if (ce.pIds[k] < 0) {
                    id = centerStart - 1 - ce.pIds[k];
                } else if (ce.pIds[k] >= m_PrevPointsNum) {
                    id = numUsed + (ce.pIds[k] - m_PrevPointsNum);
                } else {
                    id = ptLookup[ce.pIds[k]];
                }
                outPoints->GetPoint(id, pts[k]);
                pt[0] += pts[k][0];
                pt[1] += pts[k][1];
                pt[2] += pts[k][2];
            }
            pt[0] *= weight_factor;
            pt[1] *= weight_factor;
            pt[2] *= weight_factor;
            outPoints->SetPoint(ptIdx++, pt);
        }
    }

    // Now set up the shapes and the cell data.
    //
    int nlists;
    igIndex outCellNum = 0;
    igIndex conn_size = 0;
    for (i = 0; i < inCellNum; i++) {
        if (cellVisible[i] == 1) {
            outCellNum++;
            conn_size += inCells->GetCellSize(i);
        }
    }
    for (i = 0; i < m_CellTypeNum; i++) {
        igIndex ns = m_Cells[i]->GetNumberOfCells();
        outCellNum += ns;
        conn_size += static_cast<igIndex>(m_Cells[i]->GetCellSize()) * ns;
    }
    IdArray::Pointer OutConn = IdArray::New();
    OutConn->Resize(conn_size);
    UnsignedIntArray::Pointer OutOffset = UnsignedIntArray::New();
    OutOffset->Resize(outCellNum + 1);
    UnsignedIntArray::Pointer OutType = UnsignedIntArray::New();
    OutType->Resize(outCellNum);
    auto outConn = OutConn->RawPointer();
    auto outOffset = OutOffset->RawPointer();
    auto outType = OutType->RawPointer();
    igIndex* ocLookup = new igIndex[outCellNum];

    unsigned int offset = 0;
    for (i = 0; i < inCellNum; i++) {
        if (cellVisible[i] == 1) {
            pNum = inCells->GetCellIds(i, pts);
            for (j = 0; j < pNum; j++) { *outConn++ = ptLookup[pts[j]]; }
            *outOffset++ = offset;
            offset += static_cast<unsigned int>(pNum);
            *outType++ = inTypes->GetValue(i);
            *ocLookup++ = i;
        }
    }

    igIndex vhs[IGAME_CELL_MAX_SIZE] = {0};
    for (i = 0; i < m_CellTypeNum; i++) {
        const igIndex* list;
        nlists = m_Cells[i]->GetNumberOfLists();
        int shapesize = m_Cells[i]->GetCellSize();
        int cell_type = m_Cells[i]->GetIGAMEType();
        for (j = 0; j < nlists; j++) {
            int listSize = m_Cells[i]->GetList(j, list);
            for (k = 0; k < listSize; k++) {
                *ocLookup++ = *list;
                for (l = 0; l < shapesize; l++) {
                    if (list[l + 1] < 0) {
                        vhs[l] = centerStart - 1 - list[l + 1];
                    } else if (list[l + 1] >= m_PrevPointsNum) {
                        vhs[l] = numUsed + (list[l + 1] - m_PrevPointsNum);
                    } else {
                        vhs[l] = ptLookup[list[l + 1]];
                    }
                }
                list += shapesize + 1;
                for (l = 0; l < shapesize; l++) { *outConn++ = vhs[l]; }
                *outOffset++ = offset;
                offset += static_cast<unsigned int>(shapesize);
                *outType++ = static_cast<unsigned int>(cell_type);
            }
        }
    }
    *outOffset++ = offset;
    auto OutCells = CellArray::New();
    OutCells->SetData(OutConn, OutOffset);
    output->SetPoints(outPoints);
    output->SetCells(OutCells, OutType);
    ocLookup -= outCellNum;
    auto outData = AttributeSet::New();
    this->CopyAttributeSetData(outPointNum, outCellNum, inData, outData, ptLookup, numUsed, centerStart, ocLookup);
    output->SetAttributeSet(outData);

    delete[] ptLookup;
    delete[] ocLookup;
}

void QuickClipperVolumeFromVolume::CopyAttributeSetData(igIndex outPointNum, igIndex outCellNum,
                                                        AttributeSet::Pointer inData, AttributeSet::Pointer outData,
                                                        igIndex* ptLookup, igIndex edgePointStart, igIndex centerStart,
                                                        igIndex* originCell) {
    igIndex i = 0, j = 0, k = 0, l = 0, arrayId = 0;
    int dimension = 0;
    auto inAllAttr = inData->GetAllAttributes();
    double values[IGAME_CELL_MAX_SIZE] = {0};
    double values_1[IGAME_CELL_MAX_SIZE] = {0};
    double values_2[IGAME_CELL_MAX_SIZE] = {0};
    for (arrayId = 0; arrayId < inAllAttr->GetNumberOfElements(); arrayId++) {
        auto attr = inAllAttr->GetElement(arrayId);
        auto inArray = attr.pointer;
        auto outArray = FloatArray::New();
        outArray->SetName(inArray->GetName());
        outArray->SetDimension(inArray->GetDimension());
        if (attr.attachmentType == IG_CELL) {
            outArray->Resize(outCellNum);
            for (i = 0; i < outCellNum; i++) {
                inArray->GetElement(originCell[i], values);
                outArray->SetElement(i, values);
            }
            outData->AddAttribute(attr.type, attr.attachmentType, outArray, attr.GetDataRange());
        } else if (attr.attachmentType == IG_POINT) {
            outArray->Resize(outPointNum);
            dimension = inArray->GetDimension();
            for (i = 0; i < m_PrevPointsNum; i++) {
                if (ptLookup[i] != -1) {
                    inArray->GetElement(i, values);
                    outArray->SetElement(ptLookup[i], values);
                }
            }
            igIndex ptIdx = edgePointStart;
            auto nLists = m_PointList.GetNumberOfLists();
            for (i = 0; i < nLists; i++) {
                const QuickClipperInterpolatePoint* pe_list = nullptr;
                int nPts = m_PointList.GetList(i, pe_list);
                double p = .0, bp = .0;
                for (j = 0; j < nPts; j++) {
                    const QuickClipperInterpolatePoint& pe = pe_list[j];
                    p = pe.m_Percent;
                    bp = 1.0 - p;
                    inArray->GetElement(pe.m_PointId[0], values_1);
                    inArray->GetElement(pe.m_PointId[1], values_2);
                    for (k = 0; k < dimension; k++) { values[k] = values_1[k] * p + values_2[k] * bp; }
                    outArray->SetElement(ptIdx++, values);
                }
            }
            nLists = m_CenterPoints.GetNumberOfLists();
            for (i = 0; i < nLists; i++) {
                const QuickClipperCenterPoint* ce_list = nullptr;
                int nPts = m_CenterPoints.GetList(i, ce_list);
                for (j = 0; j < nPts; j++) {
                    double weight_factor;
                    const QuickClipperCenterPoint& ce = ce_list[j];
                    weight_factor = 1.0 / ce.pointNum;
                    double avg_values[IGAME_CELL_MAX_SIZE] = {0};
                    for (k = 0; k < ce.pointNum; k++) {
                        igIndex id = 0;
                        if (ce.pIds[k] < 0) {
                            id = centerStart - 1 - ce.pIds[k];
                        } else if (ce.pIds[k] >= m_PrevPointsNum) {
                            id = edgePointStart + (ce.pIds[k] - m_PrevPointsNum);
                        } else {
                            id = ptLookup[ce.pIds[k]];
                        }
                        outArray->GetElement(id, values);
                        for (l = 0; l < dimension; l++) { avg_values[l] += values[l]; }
                    }
                    for (l = 0; l < dimension; l++) { avg_values[l] *= weight_factor; }
                    outArray->SetElement(ptIdx++, avg_values);
                }
            }
            outData->AddAttribute(attr.type, attr.attachmentType, outArray, attr.GetDataRange());
        }
    }
}
// ============================================================================
// ======================= QuickModelClip ( begin) ============================
// ============================================================================
QuickModelClip::QuickModelClip() {}
QuickModelClip::~QuickModelClip() {}

bool QuickModelClip::ExecuteWithUnstructuredMesh(UnstructuredMesh::Pointer um) {
    auto m_UnstructuredMesh = um;
    if (!m_UnstructuredMesh) return false;

    //不能使用打表速切的部分
    UnstructuredMesh::Pointer RestPart = nullptr;
    igIndex RestNum = 0;

    UnstructuredMesh::Pointer OutMesh = UnstructuredMesh::New();

    auto inData = m_UnstructuredMesh->GetAttributeSet();
    auto inPoints = m_UnstructuredMesh->GetPoints();
    auto inPointNum = m_UnstructuredMesh->GetNumberOfPoints();
    auto inCells = m_UnstructuredMesh->GetCells();
    auto inTypes = m_UnstructuredMesh->GetCellTypes();
    igIndex inCellNum = m_UnstructuredMesh->GetNumberOfCells();


    DoubleArray::Pointer PointClipArray = DoubleArray::New();
    CharArray::Pointer CellVisible = CharArray::New();
    ComputePointValueAndCellVisible(inPoints, inCells, PointClipArray, CellVisible);
    auto PointClipValue = PointClipArray->RawPointer();
    auto cellVisible = CellVisible->RawPointer();


    igIndex i = 0, j = 0, k = 0;
    const igIndex* vhs = nullptr;
    int vcnt = 0;


    QuickClipperVolumeFromVolume* VFV =
            new QuickClipperVolumeFromVolume(0, inPointNum, int(pow(double(inCellNum), double(0.6667f))) * 5 + 100);

    bool couldClip = false;
    for (i = 0; i < inCellNum; i++) {
        if (cellVisible[i]) { continue; }
        auto cellType = IGCellType(inTypes->GetValue(i));
        vcnt = inCells->GetCellIds(i, vhs);
        switch (cellType) {
            case IG_TETRA:
            case IG_PYRAMID:
            case IG_PRISM:
            case IG_HEXAHEDRON:
            case IG_TRIANGLE:
            case IG_QUAD:
            case IG_LINE:
            case IG_VERTEX:
                couldClip = true;
                break;
            default:
                couldClip = false;
                break;
        }
        if (couldClip) {
            int caseIndex = 0;
            double cellValues[8] = {0};
            for (j = vcnt - 1; j >= 0; j--) {
                cellValues[j] = PointClipValue[vhs[j]];
                caseIndex += (cellValues[j] >= 0 ? 1 : 0);
                caseIndex <<= j ? 1 : 0;
            }
            int startIdx = 0;
            int nOutputs = 0;
            typedef const int EDGEIDXS[2];
            EDGEIDXS* edgeVtxs = nullptr;
            unsigned char* thisCase = nullptr;
            switch (cellType) {
                case IG_TETRA:
                    startIdx = vtkTableBasedClipperClipTables::StartClipShapesTet[caseIndex];
                    thisCase = &vtkTableBasedClipperClipTables::ClipShapesTet[startIdx];
                    nOutputs = vtkTableBasedClipperClipTables::NumClipShapesTet[caseIndex];
                    edgeVtxs = (EDGEIDXS*) vtkTableBasedClipperTriangulationTables::TetVerticesFromEdges;
                    break;

                case IG_PYRAMID:
                    startIdx = vtkTableBasedClipperClipTables::StartClipShapesPyr[caseIndex];
                    thisCase = &vtkTableBasedClipperClipTables::ClipShapesPyr[startIdx];
                    nOutputs = vtkTableBasedClipperClipTables::NumClipShapesPyr[caseIndex];
                    edgeVtxs = (EDGEIDXS*) vtkTableBasedClipperTriangulationTables::PyramidVerticesFromEdges;
                    break;

                case IG_PRISM:
                    startIdx = vtkTableBasedClipperClipTables::StartClipShapesWdg[caseIndex];
                    thisCase = &vtkTableBasedClipperClipTables::ClipShapesWdg[startIdx];
                    nOutputs = vtkTableBasedClipperClipTables::NumClipShapesWdg[caseIndex];
                    edgeVtxs = (EDGEIDXS*) vtkTableBasedClipperTriangulationTables::WedgeVerticesFromEdges;
                    break;

                case IG_HEXAHEDRON:
                    startIdx = vtkTableBasedClipperClipTables::StartClipShapesHex[caseIndex];
                    thisCase = &vtkTableBasedClipperClipTables::ClipShapesHex[startIdx];
                    nOutputs = vtkTableBasedClipperClipTables::NumClipShapesHex[caseIndex];
                    edgeVtxs = (EDGEIDXS*) vtkTableBasedClipperTriangulationTables::HexVerticesFromEdges;
                    break;


                case IG_TRIANGLE:
                    startIdx = vtkTableBasedClipperClipTables::StartClipShapesTri[caseIndex];
                    thisCase = &vtkTableBasedClipperClipTables::ClipShapesTri[startIdx];
                    nOutputs = vtkTableBasedClipperClipTables::NumClipShapesTri[caseIndex];
                    edgeVtxs = (EDGEIDXS*) vtkTableBasedClipperTriangulationTables::TriVerticesFromEdges;
                    break;

                case IG_QUAD:
                    startIdx = vtkTableBasedClipperClipTables::StartClipShapesQua[caseIndex];
                    thisCase = &vtkTableBasedClipperClipTables::ClipShapesQua[startIdx];
                    nOutputs = vtkTableBasedClipperClipTables::NumClipShapesQua[caseIndex];
                    edgeVtxs = (EDGEIDXS*) vtkTableBasedClipperTriangulationTables::QuadVerticesFromEdges;
                    break;


                case IG_LINE:
                    startIdx = vtkTableBasedClipperClipTables::StartClipShapesLin[caseIndex];
                    thisCase = &vtkTableBasedClipperClipTables::ClipShapesLin[startIdx];
                    nOutputs = vtkTableBasedClipperClipTables::NumClipShapesLin[caseIndex];
                    edgeVtxs = (EDGEIDXS*) vtkTableBasedClipperTriangulationTables::LineVerticesFromEdges;
                    break;

                case IG_VERTEX:
                    startIdx = vtkTableBasedClipperClipTables::StartClipShapesVtx[caseIndex];
                    thisCase = &vtkTableBasedClipperClipTables::ClipShapesVtx[startIdx];
                    nOutputs = vtkTableBasedClipperClipTables::NumClipShapesVtx[caseIndex];
                    edgeVtxs = nullptr;
                    break;
            }
            int intrpIds[4] = {-1, -1, -1, -1};
            for (j = 0; j < nOutputs; j++) {
                int nCellPts = 0;
                int theColor = -1;
                int intrpIdx = -1;
                unsigned char theShape = *thisCase++;
                switch (theShape) {
                    case ST_HEX:
                        nCellPts = 8;
                        theColor = *thisCase++;
                        break;
                    case ST_WDG:
                        nCellPts = 6;
                        theColor = *thisCase++;
                        break;
                    case ST_PYR:
                        nCellPts = 5;
                        theColor = *thisCase++;
                        break;
                    case ST_TET:
                        nCellPts = 4;
                        theColor = *thisCase++;
                        break;
                    case ST_QUA:
                        nCellPts = 4;
                        theColor = *thisCase++;
                        break;
                    case ST_TRI:
                        nCellPts = 3;
                        theColor = *thisCase++;
                        break;
                    case ST_LIN:
                        nCellPts = 2;
                        theColor = *thisCase++;
                        break;

                    case ST_VTX:
                        nCellPts = 1;
                        theColor = *thisCase++;
                        break;
                    case ST_PNT:
                        //插入的点，一般为均值插值得到的质心点
                        intrpIdx = *thisCase++;
                        theColor = *thisCase++;
                        nCellPts = *thisCase++;
                        break;
                    default:
                        igError("invalid cell type!");
                }
                if ((!this->m_InsideOut && theColor == COLOR0) || (this->m_InsideOut && theColor == COLOR1)) {
                    thisCase += nCellPts;
                    continue;
                }
                igIndex cellIds[8] = {0};
                for (k = 0; k < nCellPts; k++) {
                    unsigned char pntIndex = *thisCase++;
                    if (pntIndex <= P7) {
                        cellIds[k] = vhs[pntIndex];
                    } else if (pntIndex >= EA && pntIndex <= EL) {
                        int pt1Index = edgeVtxs[pntIndex - EA][0];
                        int pt2Index = edgeVtxs[pntIndex - EA][1];
                        if (pt2Index < pt1Index) {
                            int temp = pt2Index;
                            pt2Index = pt1Index;
                            pt1Index = temp;
                        }
                        double pt1ToPt2 = cellValues[pt2Index] - cellValues[pt1Index];
                        double pt1ToIso = 0.0 - cellValues[pt1Index];
                        double p1Weight = 1.0 - pt1ToIso / pt1ToPt2;

                        igIndex pntIndx1 = vhs[pt1Index];
                        igIndex pntIndx2 = vhs[pt2Index];

                        cellIds[k] = VFV->AddPoint(pntIndx1, pntIndx2, p1Weight);
                    } else if (pntIndex >= N0 && pntIndex <= N3) {
                        cellIds[k] = intrpIds[pntIndex - N0];
                    } else {
                        igError("An invalid output case was found in the ClipCases.");
                    }
                }
                switch (theShape) {
                    case ST_HEX:
                        VFV->AddHex(i, cellIds[0], cellIds[1], cellIds[2], cellIds[3], cellIds[4], cellIds[5],
                                    cellIds[6], cellIds[7]);
                        break;

                    case ST_WDG:
                        VFV->AddPrism(i, cellIds[0], cellIds[1], cellIds[2], cellIds[3], cellIds[4], cellIds[5]);
                        break;

                    case ST_PYR:
                        VFV->AddPyramid(i, cellIds[0], cellIds[1], cellIds[2], cellIds[3], cellIds[4]);
                        break;

                    case ST_TET:
                        VFV->AddTetra(i, cellIds[0], cellIds[1], cellIds[2], cellIds[3]);
                        break;

                    case ST_QUA:
                        VFV->AddQuad(i, cellIds[0], cellIds[1], cellIds[2], cellIds[3]);
                        break;

                    case ST_TRI:
                        VFV->AddTriangle(i, cellIds[0], cellIds[1], cellIds[2]);
                        break;

                    case ST_LIN:
                        VFV->AddLine(i, cellIds[0], cellIds[1]);
                        break;

                    case ST_VTX:
                        VFV->AddVertex(i, cellIds[0]);
                        break;

                    case ST_PNT:
                        intrpIds[intrpIdx] = VFV->AddCenterPoint(nCellPts, cellIds);
                        break;
                }
            }
            edgeVtxs = nullptr;
            thisCase = nullptr;
        } else if (cellType == IG_POLYHEDRON) {
            return this->ModelClip::ExecuteWithUnstructuredMesh(um);
        } else {
            return this->ModelClip::ExecuteWithUnstructuredMesh(um);
        }
        vhs = nullptr;
    }

    if (RestNum > 0) {

    } else {
        VFV->ConstructUM(OutMesh, inData, inPoints, inCells, inTypes, CellVisible);
    }
    this->SetOutput(0, OutMesh);
    delete VFV;
    VFV = nullptr;
    return true;
}
bool QuickModelClip::ExecuteWithVolumeMesh(VolumeMesh::Pointer m_VolumeMesh) {
    if (!m_VolumeMesh) return false;
    if (m_VolumeMesh->GetIsPolyhedronType()) { return this->ExecuteWithVolumeMeshWithPolyhedronType(m_VolumeMesh); }
    UnsignedIntArray::Pointer Types = UnsignedIntArray::New();
    auto inCells = m_VolumeMesh->GetVolumes();
    auto inCcnt = m_VolumeMesh->GetNumberOfVolumes();
    Types->Resize(inCcnt);
    unsigned int type = 0;
    int vcnt = 0;
    for (int i = 0; i < inCcnt; i++) {
        vcnt = inCells->GetCellSize(i);
        if (vcnt == 4) {
            type = IG_TETRA;
        } else if (vcnt == 5) {
            type = IG_PYRAMID;
        } else if (vcnt == 6) {
            type = IG_PRISM;
        } else if (vcnt == 8) {
            type = IG_HEXAHEDRON;
        }
        Types->SetValue(i, type);
    }
    auto um = UnstructuredMesh::New();
    um->SetPoints(m_VolumeMesh->GetPoints());
    um->SetCells(inCells, Types);
    um->SetAttributeSet(m_VolumeMesh->GetAttributeSet());
    return ExecuteWithUnstructuredMesh(um);
}
bool QuickModelClip::ExecuteWithSurfaceMesh(SurfaceMesh::Pointer m_SurfaceMesh) {
    if (!m_SurfaceMesh) return false;
    UnsignedIntArray::Pointer Types = UnsignedIntArray::New();
    auto inFaces = m_SurfaceMesh->GetFaces();
    auto inFcnt = m_SurfaceMesh->GetNumberOfFaces();
    Types->Resize(inFcnt);
    unsigned int type = 0;
    int vcnt = 0;
    for (int i = 0; i < inFcnt; i++) {
        vcnt = inFaces->GetCellSize(i);
        if (vcnt == 3) {
            type = IG_TRIANGLE;
        } else if (vcnt == 4) {
            type = IG_QUAD;
        } else {
            return this->ModelClip::ExecuteWithSurfaceMesh(m_SurfaceMesh);
		}
		Types->SetValue(i, type);
	}
	auto um = UnstructuredMesh::New();
	um->SetPoints(m_SurfaceMesh->GetPoints());
	um->SetCells(inFaces, Types);
	um->SetAttributeSet(m_SurfaceMesh->GetAttributeSet());
	return ExecuteWithUnstructuredMesh(um);
}


IGAME_NAMESPACE_END