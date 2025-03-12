#include "iGameFaceTable.h"

IGAME_NAMESPACE_BEGIN

igIndex FaceTable::IsFace(igIndex* face, int size) {
    //InsertionSort(face, this->Interval);
    igIndex i, index = face[0], k = 0, sum = face[0];
    for (i = 1; i < size; i++) // 找到最小的索引作为index，这样能保存很少的邻接关系
    {
        sum += face[i];
        if (face[i] < index)
        {
            index = face[i];
            k = i;
        }
    }

    if (index >= this->Size || this->Array[index] == nullptr)
    {
        return (-1);
    }

    igIndex f[16], j, t;
    for (i = 0; i < this->Array[index]->GetNumberOfIds(); i += 2)
    {
        igIndex idx = this->Array[index]->GetId(i);           // 该面在Faces数组中的索引
        igIndex index_sum = this->Array[index]->GetId(i + 1); // 该面的顶点索引和
        if (sum == index_sum)                                 // 只有当顶点索引和相同时，才进行下一步检查
        {
            this->Faces->GetCellIds(idx, f);                  // 获取该面

            // 另一种方式，遍历size次，看有几个索引相同
            int c = 0;
            for (j = 0; j < size; j++)
            {
                for (t = 0; t < size; t++)
                {
                    if (face[j] == f[t])
                    {
                        c++;
                        break;
                    }
                }
            }
            if (c == size) return idx;
        }

    }
    return (-1);
}

void FaceTable::InsertFace(igIndex* face, int size) {
    igIndex i, index = face[0], sum = face[0];
    for (i = 1; i < size; i++)
    {
        sum += face[i];
        if (face[i] < index)
        {
            index = face[i];
        }
    }

    if (index >= this->Size)
    {
        igIndex newSize = std::max(index + 1, this->Size * 2);
        this->Resize(newSize);
    }

    if (this->Array[index] == nullptr)
    {
        this->Array[index] = IdArray::New();
        this->Array[index]->Resize(12);
    }

    this->Array[index]->AddId(this->NumberOfFaces);
    if (sum < 0)
    {
        //Error("sum exceeds the maximum int range!")
    }
    this->Array[index]->AddId(sum); 
    this->Faces->AddCellIds(face, size);
    this->NumberOfFaces++;
}

igIndex FaceTable::GetNumberOfFaces() { return this->NumberOfFaces; }
CellArray::Pointer FaceTable::GetOutput() { return this->Faces; }

FaceTable::FaceTable() {
    this->Size = 0;
    this->NumberOfFaces = 0;
    this->Faces = CellArray::New();
}


void FaceTable::Resize(igIndex newSize) {
    this->Array.resize(newSize);
    this->Size = newSize;
}


IGAME_NAMESPACE_END