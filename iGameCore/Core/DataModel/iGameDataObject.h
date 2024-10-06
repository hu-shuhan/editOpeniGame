#ifndef iGameDataObject_h
#define iGameDataObject_h

#include "iGameAttributeSet.h"
#include "iGameBoundingBox.h"
#include "iGameMetadata.h"
#include "iGamePropertyTree.h"
#include "iGameScalarsToColors.h"
#include "iGameStreamingData.h"

IGAME_NAMESPACE_BEGIN
class Scene;

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

    StreamingData::Pointer GetTimeFrames();
    //	SmartPointer<StreamingData> GetTimeFrames();
    void SetTimeFrames(StreamingData::Pointer p) { m_TimeFrames = p; }

    void SetAttributeSet(AttributeSet::Pointer p) { m_Attributes = p; }
    AttributeSet* GetAttributeSet() { return m_Attributes.get(); }
    Metadata* GetMetadata() { return m_Metadata.get(); }
    PropertyTree* GetPropertys() { return m_Propertys.get(); }
    const BoundingBox& GetBoundingBox() {
        ComputeBoundingBox();
        if (this->HasSubDataObject()) {
            for (auto it = SubDataObjectIteratorBegin();
                 it != SubDataObjectIteratorEnd(); ++it)
                m_Bounding.add(it->second->GetBoundingBox());
        }
        return m_Bounding;
    }

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
    //Get real size of DataObject
    virtual IGsize GetRealMemorySize() { return 0; };

protected:
    DataObject() {
        m_Attributes = AttributeSet::New();
        m_Metadata = Metadata::New();
        //        m_TimeFrames = StreamingData::New();
        m_UniqueId = GetIncrementDataObjectId();
        m_BoundingHelper = Object::New();
        m_Propertys = PropertyTree::New();

        // Test...
        auto prop1 = m_Propertys->AddProperty(Variant::Int, "Size");
        prop1->SetValue(0);
        prop1->SetEnabled(true);
        auto prop2 = prop1->AddSubProperty(Variant::Int, "x");
        prop2->SetValue(1);
        prop2->SetEnabled(true);
        auto prop3 = prop1->AddSubProperty(Variant::Int, "y");
        prop3->SetValue(1);
        prop3->SetEnabled(true);

        m_Propertys->AddProperty(Variant::Int, "Width");
        m_Propertys->AddProperty(Variant::Int, "Height");
        m_Propertys->AddProperty(Variant::Int, "Length");
    }
    ~DataObject() override{/*std::cout << "Destructed\n";*/};

    virtual void ComputeBoundingBox() {}

    DataObjectId m_UniqueId{};
    StreamingData::Pointer m_TimeFrames{};
    AttributeSet::Pointer m_Attributes{};
    Metadata::Pointer m_Metadata{};
    PropertyTree::Pointer m_Propertys{};

    BoundingBox m_Bounding{};
    Object::Pointer m_BoundingHelper{};

    friend class SubDataObjectsHelper;
    SmartPointer<SubDataObjectsHelper> m_SubDataObjectsHelper{};
    DataObject* m_Parent{nullptr};


    template<typename Functor, typename... Args>
    void ProcessSubDataObjects(Functor&& functor, Args&&... args);

private:
    DataObjectId GetIncrementDataObjectId();
    void SetParent(DataObject* parent);

public:
    //virtual void Draw(Scene*);
    //virtual void DrawPhase1(Scene*);
    //virtual void DrawPhase2(Scene*);
    //virtual void TestOcclusionResults(Scene*);
    //virtual void ConvertToDrawableData();

    virtual void ChangeDrawable(bool drawScalar) { m_Drawable = drawScalar; }
    virtual bool IsDrawable() { return m_Drawable; }
    virtual ScalarsToColors::Pointer GetColorMapper() { return m_ColorMapper; }

    virtual void ViewCloudPicture(Scene*, int index, int dimension = -1);
    void ViewCloudPictureOfModel(Scene*, int index, int dimension = -1);

    int GetAttributeIndex();
    int GetAttributeDimension();

    int GetTimeframeIndex();
    void SwitchToCurrentTimeframe(int timeIndex);

protected:
    bool m_Drawable{false};
    int m_AttributeIndex{-1};
    int m_AttributeDimension{-1};
    int m_CurrentTimeframeIndex{-1};
    ScalarsToColors::Pointer m_ColorMapper = ScalarsToColors::New();
};

template<typename Functor, typename... Args>
inline void DataObject::ProcessSubDataObjects(Functor&& functor,
                                              Args&&... args) {
    if (HasSubDataObject()) {
        for (auto it = m_SubDataObjectsHelper->Begin();
             it != m_SubDataObjectsHelper->End(); ++it) {
            (it->second->*functor)(std::forward<Args>(args)...);
        }
    }
}

IGAME_NAMESPACE_END
#endif