#ifndef iGameDataObject_h
#define iGameDataObject_h

#include "iGameAttributeSet.h"
#include "iGameBoundingBox.h"
#include "iGameMetadata.h"
#include "iGamePropertyTree.h"
#include "iGameScalarsToColors.h"
#include "iGameStreamingData.h"
#include "iGameDeformationData.h"
#include "iGameCellArray.h"

IGAME_NAMESPACE_BEGIN
class DataObject : public Object {
public:
    I_OBJECT(DataObject);
    static Pointer New() { return new DataObject; }

    void SetUniqueDataObjectId() { m_UniqueId = GetIncrementDataObjectId(); }
    DataObjectId GetDataObjectId() { return m_UniqueId; }

    static Pointer CreateDataObject(IGenum type);

    virtual IGenum GetDataObjectType() const { return IG_DATA_OBJECT; }

    virtual bool ShallowCopy(Pointer o) { return true; }
    virtual bool DeepCopy(Pointer o) { return true; }

    virtual Points::Pointer GetPoints() { return nullptr; }
    virtual CellArray::Pointer GetCellArray() { return nullptr; }

    StreamingData::Pointer GetTimeFrames();
    // 仅用于“查询是否存在时间序列”之类的场景，避免 GetTimeFrames() 产生空对象的副作用
    StreamingData::Pointer PeekTimeFrames() const { return m_TimeFrames; }
    DeformationData::Pointer GetDeformationData();

    /*Set the data for Animation.*/
    void SetTimeFrames(StreamingData::Pointer p) { m_TimeFrames = p; }
    /*Set the data for Warping.*/
    void SetDeformationData(DeformationData::Pointer  p){m_DeformationData = p;}
    /*Update Animation.*/
    void UpdateAnimation(int keyframe_idx);
    void ApplyAnimationFrame(StreamingType frameType,
                             const std::vector<Object::Pointer>& frameData);
    void SetAttributeSet(AttributeSet::Pointer p);
    AttributeSet* GetAttributeSet() { return m_Attributes.get(); }
    int GetCurrentAttributeIndex() { return m_AttributeIndex; }
    int GetCurrentAttributeDimension(){return m_AttributeDimension;}
    Metadata* GetMetadata() { return m_Metadata.get(); }
    PropertyTree* GetProperties() { return m_Properties.get(); }
    const BoundingBox& GetBoundingBox();

    class SubDataObjectsHelper : public Object {
    public:
        I_OBJECT(SubDataObjectsHelper);
        static Pointer New() { return new SubDataObjectsHelper; };
        using SubDataObjectMap = std::map<DataObjectId, DataObject::Pointer>;
        using Iterator = SubDataObjectMap::iterator;
        using ConstIterator = SubDataObjectMap::const_iterator;


        DataObject::Pointer GetSubDataObject(DataObjectId id) {
            auto it = m_SubDataObjects.find(id);
            if (it == End()) { return nullptr; }
            return it->second;
        }

        DataObjectId AddSubDataObject(DataObject::Pointer obj) {
            for (auto& data: m_SubDataObjects) {
                if (data.second == obj) { return -1; }
            }
            m_SubDataObjects.insert(
                    std::make_pair<>(obj->GetDataObjectId(), obj));
            this->Modified();
            return obj->GetDataObjectId();
        }

        void RemoveSubDataObject(DataObjectId id) {
            auto it = m_SubDataObjects.find(id);
            if (it == End()) { return; }
            m_SubDataObjects.erase(id);
            this->Modified();
        }

        void ClearSubDataObject() {
            m_SubDataObjects.clear();
            this->Modified();
        }

        bool HasSubDataObject() noexcept {
            return m_SubDataObjects.size() != 0;
        }

        int GetNumberOfSubDataObjects() noexcept {
            return m_SubDataObjects.size();
        }

        Iterator Begin() noexcept { return m_SubDataObjects.begin(); }
        ConstIterator Begin() const noexcept {
            return m_SubDataObjects.begin();
        }

        Iterator End() noexcept { return m_SubDataObjects.end(); }
        ConstIterator End() const noexcept { return m_SubDataObjects.end(); }

    private:
        SubDataObjectsHelper() {}
        ~SubDataObjectsHelper()
                override { /*std::cout << "Helper Desctructure\n";*/ }

        DataObject* m_parentObject{nullptr};
        SubDataObjectMap m_SubDataObjects;
    };

    using SubIterator = typename SubDataObjectsHelper::Iterator;
    using SubConstIterator = typename SubDataObjectsHelper::ConstIterator;

    //	void UpdateAttributeSetRange();

    DataObject::Pointer GetSubDataObject(DataObjectId id);
    void SetParentDataObject(DataObject* parent) { m_Parent = parent; }

    /* Recollect Sub data object's MIN/MAX DataRange to update the current dataObject's data range. */
    bool ReCollectSubDataObjectDataRange();
    /* Update Sub data Object's data range to the parent data range. */
    bool UpdateSubDataObjectDataRange();
    // 仅建立父子层级，不触发绘制数据转换和属性范围聚合
    DataObjectId AttachSubDataObject(DataObject::Pointer obj);
    DataObjectId AddSubDataObject(DataObject::Pointer obj);
    void RemoveSubDataObject(DataObjectId id);
    void ClearSubDataObject();
    bool HasParentDataObject() noexcept { return m_Parent != nullptr; }

    bool HasSubDataObject() noexcept;
    int GetNumberOfSubDataObjects() noexcept;
    SubIterator SubDataObjectIteratorBegin();
    SubConstIterator SubDataObjectIteratorBegin() const;
    SubIterator SubDataObjectIteratorEnd();
    SubConstIterator SubDataObjectIteratorEnd() const;

    DataObject* FindParent();
    //Get real size of DataObject, Only the memory of a large batch of arrays is taken into account 
    virtual IGsize GetRealMemorySize();

protected:
    ~DataObject() override{/*std::cout << "Destructed\n";*/};
    DataObject() {
        m_Attributes = AttributeSet::New();
        m_Attributes->m_DataObject = this;

        m_Metadata = Metadata::New();
        m_UniqueId = GetIncrementDataObjectId();
        m_BoundingHelper = Object::New();
        m_Properties = PropertyTree::New();

        m_AttributeHelper = Object::New();
    }

    virtual void ComputeBoundingBox() {}

    DataObjectId m_UniqueId{};
    StreamingData::Pointer m_TimeFrames{};
    DeformationData::Pointer m_DeformationData{};
    AttributeSet::Pointer m_Attributes{};
    Metadata::Pointer m_Metadata{};
    PropertyTree::Pointer m_Properties{};

    BoundingBox m_Bounding{};
    Object::Pointer m_BoundingHelper{};

    friend class SubDataObjectsHelper;
    SmartPointer<SubDataObjectsHelper> m_SubDataObjectsHelper{};
    DataObject* m_Parent{nullptr};


private:
    DataObjectId GetIncrementDataObjectId();
    void SetParent(DataObject* parent);

public:
    virtual bool IsDrawable() { return false; }
    virtual ScalarsToColors::Pointer GetColorMapper() { return m_ColorMapper; }
    void SetColorMapper(ScalarsToColors::Pointer cm) { m_ColorMapper = cm; }

    int GetAttributeIndex();
    void SetAttributeIndex(int index);
    int GetAttributeDimension();

protected:
    int m_AttributeIndex{-1};
    int m_AttributeDimension{-1};
    Object::Pointer m_AttributeHelper{};

    ScalarsToColors::Pointer m_ColorMapper = ScalarsToColors::New();
};

//template<typename Functor, typename... Args>
//inline void DataObject::ProcessSubDataObjects(Functor&& functor,
//                                              Args&&... args) {
//    if (HasSubDataObject()) {
//        for (auto it = m_SubDataObjectsHelper->Begin();
//             it != m_SubDataObjectsHelper->End(); ++it) {
//            (it->second->*functor)(std::forward<Args>(args)...);
//        }
//    }
//}

IGAME_NAMESPACE_END
#endif
