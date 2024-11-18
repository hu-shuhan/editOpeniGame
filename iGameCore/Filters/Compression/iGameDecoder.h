#ifndef iGameDecoder_h
#define iGameDecoder_h

#include "iGameFilter.h"
#include "iGameCoderBase.h"

IGAME_NAMESPACE_BEGIN
class Decoder : public Filter {
public:
    I_OBJECT(Decoder);
    static Pointer New() { return new Decoder; }

	bool Execute() override { 
        DecodeFromBuffer(m_Buffer);
        SetOutput(m_DataObject);
        return true;
    }

    void SetBuffer(const std::vector<uint8_t>& buffer) {
        m_Buffer = std::move(buffer);
    }

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

    bool DecodeFromBuffer(std::vector<uint8_t>& buffer) {
        uint8_t* ip = buffer.data();

        using namespace comp;

        CellArray::Pointer Cells = CellArray::New();
        Points::Pointer Points = Points::New();

        Header header;
        ip = Load(ip, header);
        switch (header.MESH_TYPE) {
            case IG_SURFACE_MESH: {
                auto mesh = SurfaceMesh::New();
                mesh->SetPoints(Points);
                mesh->SetFaces(Cells);

                m_DataObject = mesh;
            }
            default:
                break;
        }

        uint16_t block_size;
        ip = Load(ip, block_size);

        std::vector<size_t> block_begin(block_size);
        ip = Load(ip, block_begin.data(), (int) block_size * 8);

        struct Block {
            float min0, min1, min2;
            float range;
            uint32_t max_quantized_bits;
            uint32_t pn;
        };

        Points->Resize(header.POINT_SIZE);
        for (int i = 0; i < block_size; ++i) {
            Block block;
            ip = Load(ip, block);

            struct tempPoint {
                uint16_t x, y, z;
                uint32_t id;
            };
            std::vector<tempPoint> points(block.pn);
            //std::cout << points.size() * sizeof tempPoint << " "
            //          << block.pn << std::endl;
            for (int k = 0; k < block.pn; k++) {
                ip = Load(ip, points[k].x);
                ip = Load(ip, points[k].y);
                ip = Load(ip, points[k].z);
                ip = Load(ip, points[k].id);
            }
           


            Dequantizer dq;
            const uint32_t max_quantized_value =
                    (1u << block.max_quantized_bits) - 1;
            dq.Init(block.range, max_quantized_value);

            for (int k = 0; k < block.pn; k++) {
                float val0 = dq.DequantizeFloat(
                                     static_cast<uint32_t>(points[k].x)) +
                             block.min0;
                float val1 = dq.DequantizeFloat(
                                     static_cast<uint32_t>(points[k].y)) +
                             block.min1;
                float val2 = dq.DequantizeFloat(
                                     static_cast<uint32_t>(points[k].z)) +
                             block.min2;

                Points->SetPoint(points[k].id, Point(val0, val1, val2));
            }
        }
   
        uint32_t index[32]{};
        for (int i = 0; i < header.CELL_SIZE; i++) {
            struct tempCell {
                uint16_t type, size;
            };
            tempCell cell;
            ip = Load(ip, cell);
            ip = Load(ip, index, cell.size * 4);
            
            Cells->AddCellIds(reinterpret_cast<igIndex*>(index), cell.size);
        }
        return true;
    }

protected:
    Decoder()
	{
		SetNumberOfOutputs(1);
	}
    ~Decoder() override = default;

    std::vector<uint8_t> m_Buffer{};
    DataObject::Pointer m_DataObject{};
};
IGAME_NAMESPACE_END
#endif