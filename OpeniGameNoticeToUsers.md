# OpeniGame使用手册

## 框架结构

<img src=".\Resources\Images\image-20240729152419670.png" alt="image-20240729152419670" style="zoom:75%;" align = "left"/>

1. iGameCore目录下是核心模块，包括数据结构、算法、渲染等子模块；Qt目录下是前端页面模块
2. CellModel目录包含基本的元素对象类，如Triangle、Quad、Tetra等，需要继承自Cell对象
3. Common目录包含框架内基本的对象，如Object、ArrayObject等
4. DataModel目录包含数据结构对象，如SurfaceMesh、VolumeMesh等，需要继承自DataObject对象
5. Filters目录包含所有的算法对象，需要继承自Filter对象
6. IO目录包含所有的输入输出对象
7. Rendering目录包含渲染相关的类

## 命名规则（暂定）

1. 文件名使用 iGame+名称
2. 类名不使用 iGame前缀，使用驼峰命名法，首字母大写
3. 函数名使用驼峰命名法，如果继承自Object，首字母大写，如果像Vector等对象，函数名首字母小写
4. 成员变量使用 m_名称（驼峰命名法，首字母大写）

<img src=".\Resources\Images\image-20240729165349742.png" alt="image-20240729165349742" style="zoom:50%;" align = "left"/>

## 简化类图

<img src=".\Resources\Images\image-20240802170101351.png" alt="image-20240802170101351" style="zoom:80%;" />

<img src=".\Resources\Images\image-20240802170121931.png" alt="image-20240802170121931" style="zoom:80%;" />

## 架构设计

<img src=".\Resources\Images\image-20240802170152717.png" alt="image-20240802170152717" style="zoom:50%;" />

### Scene

Scene场景包含了需要渲染的各类信息和渲染对象，有光源、摄像机、背景、Shader、交互器、数据对象列表。需要在OpenGL的上下文中进行渲染，默认会调用DataObject的ConvertToDrawableData和Draw进行绘制。成员变量有：

1. m_Models：std::map<DataObjectId, DataObject::Pointer> 数据对象列表
2. m_CurrentObjectId：当前数据对象的id
3. m_CurrentObject：DataObject*  当前数据对象的指针

对于写算法的同学，不需要关注Scene等渲染逻辑

### Object

Object是整个框架的基类，管理对象的生命周期、时间戳等，如果需要使用框架内的智能指针，必须继承Object

1.生命周期：通过具有原子性的ReferenceCount来管理生命周期，对应有Register和UnRegister函数对对象进行使用，需要配合指针指针SmartPointer使用。有一个非常重要的概念：工厂模式，即对外不暴露构造函数和析构函数，只暴露New函数，New函数返回该对象的智能指针，这可以保证所有的指针都是智能指针，确保不会内存泄漏。

```
mutable std::atomic<int> m_ReferenceCount{};
virtual void Register() const {
    ++m_ReferenceCount;
}
virtual void UnRegister() const noexcept {
    if (--m_ReferenceCount <= 0)
    {
        delete this;
    }
}
```

2.时间戳（TimeStamp）：时间戳有一个极其重要的功能，即控制数据的自动更新，所有对象的时间戳都是系统唯一的。例如一个对象A依赖于对象B，当对象A检测到对象B发生变化时，更新对象A的状态，其关键就是对象A如何检测到对象B的变化，可以通过比较对象A和对象B的时间戳，来判断对象A是否比对象B状态新，如果对象A的时间戳 <
对象B的时间戳，说明对象B已经更新过了，那么对象A也需要更新。当对象发生变化时，则需要调用

> obj->Modified()

3. I_OBJECT宏：是类定义中最基本的宏，主要是删除类的赋值构造，防止对象复制。还定义了Pointer，所有对象的"类名::Pointer" 都是"
   SmartPointer<类名>" 的别称，用于简化代码

```
#define I_OBJECT(TypeName)                         \
  TypeName(const TypeName &) = delete;             \
  TypeName & operator=(const TypeName &) = delete; \
  TypeName(TypeName &&) = delete;                  \
  TypeName & operator=(TypeName &&) = delete;      \
  using Self = TypeName;                           \
  using Pointer = SmartPointer<Self>;              \
  using ConstPointer = SmartPointer<const Self>; 
```

所有继承Object的类，都需要加I_OBJECT宏，新建一个类的模板如下

```
#ifndef iGameDataObject_h
#define iGameDataObject_h

#include "iGameObject.h"

IGAME_NAMESPACE_BEGIN
class DataObject : public Object {
public:
	I_OBJECT(DataObject);
	static Pointer New() { return new DataObject; }

protected:
	DataObject() {}
	~DataObject() override = default;
	
	A m_A;
};
IGAME_NAMESPACE_END
#endif
```

#### DataObject

DataObject是最基本的数据对象，所有可渲染的数据对象都需要继承DataObject。成员变量有：

1. m_UniqueId：该对象的唯一标识
2. m_PropertySet：保存所有的属性数组，如标量、矢量、张量数组等
3. m_Metadata：保存所有的元数据，如名称、文件名、文件路径等，是与Qt信息传递的重要组件，通常Qt需要从Metadata中拿取页面中显示所需要的信息
4. m_BoundingBox和m_BoundingHelper：保存数据对象的包围盒
5. m_SubDataObjectsHelper：管理所有的子数据节点，如果该对象是分支数据节点的话，最好该对象的类型就是DataObject或者专门用于管理子数据节点的对象（还未定义）
6. m_Parent：指向父数据节点的指针
7. 与渲染相关的成员：m_Visibility（可见性）、m_Drawable（是否可渲染）等

####PointSet

PointSet继承自DataObject，用于管理所有的点集，有快速查找某个点的功能，成员变量有：

1. m_Points：用于保存所有点的数组
2. m_PointDeleteMarker：用于标识点是否被删除，由于使用数组保存，无法对某个点直接删除，所有用伪删除标记
3. m_InEditStatus：编辑标志，删除点前需要开启

成员函数(部分)：

1. void RequestEditStatus()：在删除点前需要调用，会生成并初始化m_PointDeleteMarker
2. void GarbageCollection()：在操作完成后调用，清除伪删除的元素

### SurfaceMesh

SurfaceMesh继承自PointSet，表面网格类，成员变量有：

1. m_Edges和m_EdgeLinks：保存所有边的顶点id和顶点的邻接边
2. m_Faces和m_FaceLinks：保存所有面的顶点id和顶点的邻接面
3. m_FaceEdges和m_FaceEdgeLinks：保存所有面的边id和边的邻接面
4. m_EdgeDeleteMarker和m_FaceDeleteMarker：边和面的删除标志对象

成员函数(部分)：

```
// 使用方法: igIndex ids[64]{}, 用一个足够大的栈数组(根据要获取的数据定)来获取，栈数组不需要申请空间，速度比堆内存数组快
// int size = GetEdgePointIds(id, ids);
int GetEdgePointIds(const IGsize edgeId, igIndex* ptIds);   // 获取边的顶点id
int GetFacePointIds(const IGsize faceId, igIndex* ptIds);   // 获取面的顶点id
int GetFaceEdgeIds(const IGsize faceId, igIndex* edgeIds);  // 获取面的边id

void BuildEdges();         // 根据面来构建所有的边
void BuildEdgeLinks();     // 构建顶点的邻接边
void BuildFaceLinks();     // 构建顶点的邻接面
void BuildFaceEdgeLinks(); // 构建边的邻接面

// 也是用一个足够大的栈数组来获取
int GetPointToOneRingPoints(const IGsize ptId, igIndex* ptIds);   // 获取顶点的1邻接顶点
int GetPointToNeighborEdges(const IGsize ptId, igIndex* edgeIds); // 获取顶点的邻接边
int GetPointToNeighborFaces(const IGsize ptId, igIndex* faceIds); // 获取顶点的邻接面
int GetEdgeToNeighborFaces(const IGsize edgeId, igIndex* faceIds);// 获取边的邻接面(共享边)
int GetEdgeToOneRingFaces(const IGsize edgeId, igIndex* faceIds); // 获取边的1邻域面(共享顶点)
int GetFaceToNeighborFaces(const IGsize faceId, igIndex* faceIds);// 获取边的邻接面(共享边)
int GetFaceToOneRingFaces(const IGsize faceId, igIndex* faceIds); // 获取边的1邻域面(共享顶点)

void MakeEditStatusOn();
void MakeEditStatusOff();
	
void RequestEditStatus();  // 开启编辑模式，并生成邻接关系等数据结构
void GarbageCollection();  // 回收删除的数据
```

#####注意：算法在使用数据结构之前一定要先RequestEditStatus，如果数据发生变化需要调用Modified()
来更新状态，使其重新映射为渲染数据；如果删除了某些元素，一定要调用GarbageCollection垃圾回收，否则会渲染出错。

当前网格结构的继承关系：

<img src=".\Resources\Images\image-20240729155402114.png" alt="image-20240729155402114" style="zoom:80%;" />

## 如何实现自己的数据结构

1. 在DataModel目录下添加头文件和cpp文件
2. 继承DataObject
3. 实现ComputeBoundingBox、Draw、ConvertToDrawableData虚函数

```
#ifndef iGameSourceData_h
#define iGameSourceData_h

#include "iGameDataObject.h"

IGAME_NAMESPACE_BEGIN
class SourceData : public DataObject {
public:
	I_OBJECT(SourceData);
	static Pointer New() { return new SourceData; }

	struct Source {
		Vector3d position;
		Vector3d direction;
		double size;
	};

protected:
	SourceData() { }
	~SourceData() override = default;
	
	void ComputeBoundingBox() override {
	  ...
	}
	
	ElementArray<Source> m_Buffer{};

public:
	void Draw(Scene*) override {}
	void ConvertToDrawableData() override{}
	bool IsDrawable() override { return true; }

private:
	GLVertexArray m_PointVAO;
	GLBuffer m_PositionVBO, m_ColorVBO;

	FloatArray::Pointer m_Positions{};
	FloatArray::Pointer m_Colors{};
	bool m_UseColor{false};
};
IGAME_NAMESPACE_END
#endif
```

## 算法

### Filter（未完）

Filter是所有算法的基类，控制算法的输入输出、算法的执行，输出算法执行的进度，一个算法可以有多个输入和输出，可以通过接口设置输入数据，获取输出数据。其成员变量有：

1. m_Inputs：算法的输入数据，是DataObject列表，其下标是该算法的输入端口
2. m_Outputs：算法的输出数据，是DataObject列表，其下标是该算法的输出端口
3. m_Progress：算法执行的进度，显示在前端的进度条中，调用UpdateProgres(double amount)更新进度条，amout属于0-1，主ui的进度条会自动变化。

算法处理流程如下图所示：

<img src=".\Resources\Images\image-20240729192157602.png" alt="image-20240729192157602" style="zoom:50%;" />

### 如何实现自己的算法

1. 在Filters目录下新建文件夹或者是头文件以及cpp文件，注意好命名规范。

2. 需要继承Filter或其子类

3. 在构造函数中设置输入输出的个数

4. 重写Execute方法

   3.1 通过GetInput获取输入数据对象，对其进行向下类型转换为算法需要的对象

   3.2 通过SetOutput设置输出数据对象，如果输入对象是新对象的话，需要在Qt中把这个对象添加到Scene中去

```
#ifndef FilterPoints_h
#define FilterPoints_h

#include "iGameFilter.h"
#include "iGamePointSet.h"
#include <random>

IGAME_NAMESPACE_BEGIN
class FilterPoints : public Filter {
public:
	I_OBJECT(FilterPoints);
	static Pointer New() { return new FilterPoints; }

	void SetFilterRate(double value) {
		m_FilterRate = value;
	}

	bool Execute() override 
	{ 
		m_PointSet = DynamicCast<PointSet>(GetInput(0)); // 向下类型转换
		if (m_PointSet == nullptr)
		{
			return false;
		}
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_real_distribution<> dis(0.0, 1.0);

		m_PointSet->RequestEditStatus();
		for (int i = 0; i < m_PointSet->GetNumberOfPoints(); i++) {
			double random_number = dis(gen);
			if (random_number <= m_FilterRate) {
				m_PointSet->DeletePoint(i);
			}
		}
		m_PointSet->GarbageCollection();
		
		SetOutput(0, m_PointSet); // 设置输出数据
		return true;
	}

protected:
	FilterPoints()
	{
		SetNumberOfInputs(1);
		SetNumberOfOutputs(1);
	}
	~FilterPoints() override = default;

	PointSet::Pointer m_PointSet{};
	double m_FilterRate{ 0.5 };
};
IGAME_NAMESPACE_END
#endif
```

4. 在QtMainWindow中添加代码逻辑

在ui界面的某个成员（按钮、动作等都可以）进行该算法的调用执行，需要自己编写connect函数，具体可以查看示例。在igQtMainWindow中的initAllFilters()
函数内部定义具体的connect，可以使用lambda表达式。

```
connect(ui->action_test_04, &QAction::triggered, this, [&](bool checked) {
    SurfaceMeshFilterTest::Pointer fp = SurfaceMeshFilterTest::New();
    fp->SetInput(rendererWidget->GetScene()->GetCurrentObject());
    fp->Update();
    rendererWidget->update();
    });
```

如果有新的数据对象

	connect(ui->action_test_03, &QAction::triggered, this, [&](bool checked) {
		SurfaceMesh::Pointer mesh = SurfaceMesh::New();
		Points::Pointer points = Points::New();
		points->AddPoint(0, 0, 0);
		points->AddPoint(1, 0, 0);
		points->AddPoint(0, 1, 0);
	
		CellArray::Pointer faces = CellArray::New();
		faces->AddCellId3(0, 1, 2);
		
		mesh->SetPoints(points);
		mesh->SetFaces(faces);
		
		mesh->SetName("undefined_mesh");    // 这里需要设置一下名字，会显示在模型列表上
		rendererWidget->AddDataObject(mesh);// 把新的数据对象添加到Scene中
		});

1. 生成一个Dialog

```
igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this);
```

2. 设置算法名称和介绍

> void setFilterTitle(const QString& title); // 设置名称

> void setFilterDescription(const QString& text); // 设置介绍

2. 设置参数

> int addParameter(WidgetType type, const QString& title, const QString& defaultValue)

@param: Widget类型，参数名称，参数默认值

@return: 返回item的id，用于获取参数

```
int targetId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "Target number of faces", "1000");
int reductionId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "Reduction (0..1)", "0");
int thresholdId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "Quality threshold", "0.1");
int preserveId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX, "Preserve Boundary of the mesh", "false");
```

3. 添加回调函数

```
方法一：
dialog->setApplyFunctor(&VolumeMeshFilterTest::Execute);

方法二：
dialog->setApplyFunctor([&]() { 
    std::cout << "123\n";
});
```

4. 获取参数

> double getDouble(int i, bool& ok)  // 获取double参数
>
> int getInt(int i, bool& ok)                  // 获取int参数
>
> bool getChecked(int i, bool& ok)    // 获取bool参数

例子：

```
igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this);
int targetId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "Target number of faces", "1000");
int reductionId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "Reduction (0..1)", "0");
int thresholdId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "Quality threshold", "0.1");
int preserveId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX, "Preserve Boundary of the mesh", "false");
dialog->show();

bool ok;
std::cout << dialog->getInt(targetId, ok) << std::endl;
std::cout << dialog->getDouble(reductionId, ok) << std::endl;
std::cout << dialog->getDouble(thresholdId, ok) << std::endl;
std::cout << (dialog->getChecked(preserveId, ok) ? "true" : "false") << std::endl;

dialog->setApplyFunctor([&]() { 
    std::cout << "123\n";
});
```

## 数据存储对象

### ArrayObject、FlatArray

ArrayObject是FlatArray的基类，其有两种元素：Value和Element，Value是基础元素，如float、double、int等，Element是多个Value的组合，ArrayObject将二维数组扁平化为一维数组，将Element的多个Value按顺序存储下来，这样的好处可以按索引访问Value，Element顺序排列也利于直接转为渲染数据，需要设置ElementSize，即一个Element有多少个Value，且所有的Element的ElementSize都是相同的。

其提供抽象数据数组接口，方便在不知道其存储是何类型下获取数据：

> virtual double GetValue(const IGsize _Pos) = 0; // 获取默认为double类型的Value
> virtual void SetValue(IGsize _Pos, double value) = 0;
>
> virtual void GetElement(const IGsize _Pos, float* _Element) = 0; // 获取默认为float类型的Element传入一个足够大的指针，可以
> virtual void GetElement(const IGsize _Pos, double* _Element) = 0;// 是栈数组指针，也可以是vector<>的指针

常见使用方法：

    ArrayObject::Pointer arr = ...;
    方法一：
    std::vector<float> vec(arr->GetElementSize()); // 注意不要写在循环内部，否则会重复申请空间，效率很低
    for(int i = 0; i < arr->GetNumberOfElements(); i++){
    	arr->GetElement(i, vec.data());
    }
    方法二：
    float color[8]{}; // 写在循环外部或内部都可以，栈数组不需要申请堆空间
    for(int i = 0; i < arr->GetNumberOfElements(); i++){
    	arr->GetElement(i, color);
    }

FlatArray是存储数据的模版类，其类定义：

> template<typename TValue>
> class FlatArray : public ArrayObject, private std::vector<TValue>

内部存储实现还是std::vector，使用前需要设置ElementSize，默认为1，改变容器大小的几个基本函数：

> void Initialize()

释放所有空间，数据不会保留

> void Reserve(const IGsize _NewElementNum)

预申请空间，注意参数是Element的数量，如果ElementSize=3，则真正申请的空间大小为3*_NewElementNum

> void Resize(const IGsize _NewElementNum)

重新申请空间，注意参数是Element的数量

> void Reset()

空间大小不会发生变化，只是标识元素个数的变量初始化，该方法在获取动态数据时十分有用，例如

    void GetIds(const IGsize _Pos, IntArray::Pointer ids){
    	ids->Reset();
    	... // 装填数据
    	ids->AddElement();
    }
    
    IntArray::Pointer ids = IntArray::New(); // 注意写在循环外面
    for(int i = 0; i < size; i++){
    	GetIds(i, ids); // ids是一个动态数组，但是基本不会有效率损失，因为在其内部只是调用Reset让size=0
    	ids...
    }

> void Squeeze()

释放多余的空间

``框架内的容器基本都是这5个方法，但要注意参数含义``

然后我们定义了

> IntArray，DoubleArray，FloatArray等等所有基本类型数组

在创建数组时最好使用具体的类型，不要直接

> ArrayObject::Pointer arr = IntArray::New()

ArrayObject只作为通用接口使用

### ElementArray

ElementArray和上面那个数组不同，主要代替std::vector<>的作用，Element可以是任何类型数据

> template<typename TElement>
> class ElementArray : public Object, private std::vector<TElement>

我们定义了

> StringArray，IdArray等，具体实现见代码

### CellArray

CellArray用于保存Cell元素的索引，如面的顶点索引、面的边索引。CellArray中有Offsets和Buffer数组，Offsets用于存储元素在Buffer数组中的初始位置。在添加元素时，当所有元素的大小都一致时，不会使用Offsets数组，只有遇到不同大小的元素才会初始化Offsets数组，目的是减少单一元素的存储空间

> IGsize AddCellIds(igIndex* cell, int ncell)

添加一系列的id，需要指定大小

> int GetCellIds(const IGsize cellId, igIndex* cell)

获取元素的一系列id，返回大小，需要传入一个足够大的指针

## 模型导入导出

1. 目前支持vtk（高阶网格还没适配）、vts、pvd、obj（纯网格）、ply、off、mesh文件的读取
2. 目前支持vtk（目前高阶网格还没适配，只支持输出标量矢量）、obj（纯网格）、ply、off、mesh文件的输出
3. 如果想自行生成可视化数据（即标量），可调用this->manager->GetCurrentModel()->DataSet->GetPointData()->AddScalars(
   ScalarData);
   其中，manager为全局的静态管理器，需要使用manager=iGame::iGameManager::Instance()声明。主ui上点击帮助->
   关于iGameMeshView可以生成一个点坐标的标量和类似于曲率的标量。目前暂时只支持点数据，因为面数据渲染规则不一样还未做适配（但是可以自己手动绘制）

## 渲染

### OpenGL支持

目前提供``OpenGL3.3``和``OpenGL4.6``版本支持，如果程序打开是黑屏，有可能是机器或驱动并不支持``OpenGL4.6``
，可以使用``OpenGL3.3``
代替，具体修改部分请打开``iGameCore/Rendering/CMakeLists.txt``，将相关代码替换为如下代码：

```cmake
IF (WIN32)
    target_compile_definitions(${MODULE_NAME} PUBLIC IGAME_OPENGL_VERSION_330)
ELSEIF (APPLE)
    target_compile_definitions(${MODULE_NAME} PUBLIC IGAME_OPENGL_VERSION_330)
ELSEIF (UNIX)
    target_compile_definitions(${MODULE_NAME} PUBLIC IGAME_OPENGL_VERSION_330)
ENDIF ()
```

Mac系统最高只支持``OpenGL4.1``，因此必须设置为``OpenGL3.3``，还需要在``main.cpp``中修改相关代码

```Cpp
format.setVersion(4, 6); //Mac set to format.setVersion(3, 3);
```

### 透明度显示功能

透明度显示为高级功能，由于采用次序无关透明度(Order Independent Transparency, OIT)，因此在本项目必须在``OpenGL4.6``
版本才会生效，在``OpenGL3.3``版本中设置透明度不会产生作用。

### 遮挡剔除功能

遮挡剔除为加速渲染功能，由于需要用到计算着色器（Compute Shader），因此在本项目必须在``OpenGL4.6``版本才能开启。

> **注意:** 该功能只能在开启MSAA的情况下使用（还没适配未启用MSAA的情况）

### Painter 类用法

``Painter``类用于在场景中绘制各种图元，如点、线、三角形、矩形和立方体等。

创建``Painter``实例：

```cpp
Painter::Pointer painter = Painter::New();
```

``Painter``创建完成后，需要设置画笔（Pen）和画刷（Brush），画笔用于绘制图元的边框，画刷用于绘制图元的填充。

```cpp
// 创建画笔
Pen::Pointer pen = Pen::New();
// 设置画笔粗细
pen->SetWidth(10);
// 设置画笔颜色
pen->SetColor(Color::Green);

// 创建画刷
Brush::Pointer brush = Brush::New();
// 设置画刷颜色
brush->SetColor(Color::Red);

// Painter设置
painter->SetPen(pen);
painter->SetBrush(brush);
```

设置完成后就可以进行绘制，绘制调用会返回一个句柄作为唯一标识符，后续可以根据该句柄进行显示/隐藏或删除操作。

```cpp
IGuint handle = painter->DrawPoint(Point{0.0f, 0.0f, 0.0f});

// 隐藏该图元
painter->Hide(handle);
// 显示该图元
painter->Show(handle);
// 删除该图元
painter->Delete(handle);
```

如果只想绘制边框则需要把画刷颜色设置为``None``，例如绘制Bounding-Box时只需要绘制立方体边框。

```cpp
Painter::Pointer painter = Painter::New();

// 设置画笔粗细
painter->SetPen(3);
// 设置画笔颜色
painter->SetPen(Color::LightBlue);
// 设置画刷颜色为无填充
painter->SetBrush(Color::None);

// 绘制Bounding-Box
Point p1{0.0f, 0.0f, 0.0f};
Point p7{1.0f, 1.0f, 1.0f};
painter->DrawCube(p1, p7);
```

``Painter``创建后拥有默认画笔和画刷。

```cpp
Painter::Pointer painter = Painter::New();

// 设置默认画笔粗细
painter->SetPen(10);
// 设置默认画笔颜色
painter->SetPen(Color::Green);
// 设置默认画刷颜色
painter->SetBrush(Color::Red);
```

## 相关问题

### 子模块导入后构建失败

有些外部库会添加卸载指令，如果重名后就会导致冲突。

```cmake
add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/EigenUninstall.cmake)
``` 

目前有两个外部库添加卸载指令，任意选择其一注释相关代码即可（在各自根目录的``CmakeLists.txt``中）：

1. CGNS：72-73行
2. Eigen：648-649行o

## About

由于Openigame还在不断的完善，因此在使用过程中可能会遇到些许问题，所有遇到的bug都可以给我们提issue，如果有解决办法就更好了。如果有具体需求，可以自行实现，然后告诉我们，我们把他移植到正式版本中，如果该需求会很适用，也可以告知我们我们在后续事先并添加。

- 数据结构部分有疑问可以联系
    - 陈凯，微信：ck1204453420
    - 徐松，微信：XSong_1012
- QT组件部分有疑问可以联系
    - 马楷煜，微信：Fredr1ck_
- 渲染部分有疑问可以联系
    - 徐将杰，微信：Sumzeek_

