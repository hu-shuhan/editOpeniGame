#ifndef iGameEncoder_h
#define iGameEncoder_h

#include "iGameFilter.h"
#include "iGameSurfaceMesh.h"
#include "iGameCoderBase.h"

IGAME_NAMESPACE_BEGIN
class Encoder : public Filter {
public:
    I_OBJECT(Encoder);
    static Pointer New() { return new Encoder; }

	bool Execute() override {

        auto input = GetInput(0);
        switch (input->GetDataObjectType()) {
            case IG_SURFACE_MESH: {
                auto mesh = DynamicCast<SurfaceMesh>(input);
                EncodeToBuffer(mesh, m_Buffer);
            } break;
            default:
                return false;
        }
        
        return true;
    }

    const std::vector<uint8_t>& GetBuffer() const { return m_Buffer; }

    struct Header {
        int32_t VERSION;
        int32_t SIZE;
        int32_t MESH_TYPE;
        int32_t POINT_SIZE;
        int32_t POINT_OFFSET;
        int32_t CELL_SIZE;
        int32_t CELL_OFFSET;
        int32_t DATA_OFFSET;
    };

    static bool EncodeToBuffer(SurfaceMesh::Pointer mesh,
        std::vector<uint8_t>& buffer) {
        const int PartVertexThr = 1 << 16;
        iGame::AttributeSet* attrs = mesh->GetAttributeSet();

        comp::Partitioner p(mesh);
        p(PartVertexThr);
        auto& blocks = p.GetBlocks();
        auto& chunks = p.GetChunks();
        auto& label = p.GetLabel();

        size_t point_length = 0;
        point_length += 4;
        point_length += blocks.size() * 8;
        for (auto it = blocks.begin(); it != blocks.end(); it++) {
            point_length += (4 * 3 + 4 + 4 + 4 + it->pn() * 10);
            // min3 range bits pn
        }

        size_t cell_length = 4;
        cell_length += mesh->GetNumberOfFaces() * 4;
        cell_length += mesh->GetFaces()->GetCellIdArray()->GetNumberOfIds() * 4;

        size_t data_length = 0;
        for (int id = 0; id < attrs->GetNumberOfAttributes(); id++) {
            auto& attr = attrs->GetAttribute(id);
            if (attr.isDeleted) continue;
            data_length += 2;
            data_length += attr.pointer->GetName().size();
            data_length += 6;
            data_length += attr.pointer->GetDimension() *
                           attr.pointer->GetNumberOfElements() * 4;
        }
        
        size_t length =
                sizeof(Header) + point_length + cell_length + data_length;

        buffer.resize(length);
        uint8_t* op = buffer.data();

        Header header;
        header.VERSION = 1;
        header.SIZE = sizeof(Header) + length;
        header.MESH_TYPE = IG_SURFACE_MESH;
        header.POINT_SIZE = mesh->GetNumberOfPoints();
        header.POINT_OFFSET = sizeof(Header);
        
        header.CELL_SIZE = mesh->GetNumberOfFaces();
        header.CELL_OFFSET = sizeof(Header) + point_length;

        header.DATA_OFFSET = sizeof(Header) + point_length + cell_length;

        using namespace comp;
        op = Store(op, &header, sizeof(Header));

        size_t block_begin = sizeof(Header) + 2 + blocks.size() * 8;

        op = Store(op, static_cast<uint16_t>(blocks.size()));
        for (auto it = blocks.begin(); it != blocks.end(); it++) {
            op = Store(op, block_begin);
            block_begin += (4 * 3 + 4 + 4 + 4 + it->pn() * 10);
            // min3 range bits pn
        }

        // µã
        for (auto it = blocks.begin(); it != blocks.end(); it++) {
            Vector3d min = it->min;
            op = Store(op, static_cast<float>(min[0]));
            op = Store(op, static_cast<float>(min[1]));
            op = Store(op, static_cast<float>(min[2]));

            double range = 0;
            auto& ids = it->ids;
            for (int i = 0; i < ids.size(); ++i) {
                auto& p = mesh->GetPoint(ids[i]);
                range = std::max(range, fabs(p[0] - min[0]));
                range = std::max(range, fabs(p[1] - min[1]));
                range = std::max(range, fabs(p[2] - min[2]));
            }

            Quantizer q;
            const uint32_t max_quantized_bits = 16;
            const uint32_t max_quantized_value = (1u << 16) - 1;
            q.Init(range, max_quantized_value);

            op = Store(op, static_cast<float>(range));
            op = Store(op, static_cast<uint32_t>(max_quantized_bits));
            op = Store(op, static_cast<uint32_t>(it->pn()));

            for (int i = 0; i < ids.size(); ++i) {
                auto& p = mesh->GetPoint(ids[i]);
                uint32_t val0 = q.QuantizeFloat(p[0] - min[0]);
                uint32_t val1 = q.QuantizeFloat(p[1] - min[1]);
                uint32_t val2 = q.QuantizeFloat(p[2] - min[2]);

                op = Store(op, static_cast<uint16_t>(val0));
                op = Store(op, static_cast<uint16_t>(val1));
                op = Store(op, static_cast<uint16_t>(val2));
                op = Store(op, static_cast<uint32_t>(ids[i]));
            }
        }     

        // Cell
        igIndex ids[32]{};
        for (int i = 0; i < mesh->GetNumberOfFaces(); i++) {
            int size = mesh->GetFacePointIds(i, ids);
            if (size == 3) {
                op = Store(op, static_cast<uint16_t>(IG_TRIANGLE));
            } else if (size == 4) {
                op = Store(op, static_cast<uint16_t>(IG_QUAD));
            } else {
                op = Store(op, static_cast<uint16_t>(IG_POLYGON));
            }
            op = Store(op, static_cast<uint16_t>(size));
            for (int j = 0; j < size; j++) {
                op = Store(op, static_cast<uint32_t>(ids[j]));
            }
        }

        float val[16]{};
        for (int id = 0; id < attrs->GetNumberOfAttributes(); id++) {
            auto& attr = attrs->GetAttribute(id);
            if (attr.isDeleted) continue;
            op = Store(op,
                       static_cast<uint16_t>(attr.pointer->GetName().size()));
            op = Store(op, (void*) attr.pointer->GetName().c_str(),
                       attr.pointer->GetName().size());
            op = Store(op, static_cast<uint16_t>(attr.attachmentType));
            op = Store(op, static_cast<uint16_t>(attr.type));
            op = Store(op, static_cast<uint16_t>(attr.pointer->GetDimension()));

            for (int i = 0; i < attr.pointer->GetNumberOfElements(); i++) {
                attr.pointer->GetElement(i, val);
                for (int d = 0; d < attr.pointer->GetDimension(); d++) {
                    op = Store(op, val[d]);
                }
            }
        }
        return true;
    }

protected: 
    Encoder()
	{
		SetNumberOfInputs(1);
	}
    ~Encoder() override = default;

    std::vector<uint8_t> m_Buffer{};
};


IGAME_NAMESPACE_END
#endif