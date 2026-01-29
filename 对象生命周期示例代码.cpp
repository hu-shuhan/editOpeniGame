/**
 * iGameVis 对象生命周期和智能指针使用示例
 * 
 * 这个文件展示了如何使用 iGameVis 的对象系统和智能指针
 * 注意：这是示例代码，不能直接编译运行
 */

#include "iGameObject.h"
#include "iGameSmartPointer.h"
#include <iostream>
#include <vector>

// ============================================
// 示例1：基本对象创建和使用
// ============================================
void Example1_BasicUsage() {
    std::cout << "=== 示例1：基本使用 ===" << std::endl;
    
    // 1. 使用工厂方法创建对象
    Object::Pointer obj = Object::New();
    // 此时引用计数 = 1
    
    std::cout << "创建对象，引用计数 = 1" << std::endl;
    
    // 2. 设置对象名称
    obj->SetName("MyObject");
    
    // 3. 创建另一个引用
    Object::Pointer obj2 = obj;
    // 此时引用计数 = 2（两个智能指针都引用同一对象）
    
    std::cout << "创建第二个引用，引用计数 = 2" << std::endl;
    
    // 4. 两个指针都指向同一个对象
    std::cout << "obj 名称: " << obj->GetName() << std::endl;
    std::cout << "obj2 名称: " << obj2->GetName() << std::endl;
    
    // 5. 修改对象（通过任一指针）
    obj2->SetName("ModifiedObject");
    std::cout << "通过 obj2 修改后，obj 名称: " << obj->GetName() << std::endl;
    
    // 6. 智能指针自动管理内存
    // 当 obj 和 obj2 都超出作用域时，对象自动删除
    std::cout << "函数结束时，智能指针自动销毁，对象自动删除" << std::endl;
}

// ============================================
// 示例2：引用计数的变化
// ============================================
void Example2_ReferenceCounting() {
    std::cout << "\n=== 示例2：引用计数变化 ===" << std::endl;
    
    Object::Pointer obj1;
    std::cout << "创建空指针 obj1" << std::endl;
    
    {
        Object::Pointer obj2 = Object::New();
        std::cout << "在作用域内创建 obj2，引用计数 = 1" << std::endl;
        
        obj1 = obj2;  // 赋值操作
        std::cout << "obj1 = obj2，引用计数 = 2" << std::endl;
        
        // obj2 即将销毁
    }  // obj2 在这里销毁，引用计数变为 1
    
    std::cout << "obj2 销毁后，obj1 仍然有效，引用计数 = 1" << std::endl;
    std::cout << "obj1 名称: " << obj1->GetName() << std::endl;
    
    // obj1 在函数结束时销毁，对象被删除
}

// ============================================
// 示例3：函数参数传递
// ============================================
void ProcessObject(Object::Pointer obj) {
    // obj 是值传递，这里会调用拷贝构造函数
    // 引用计数 +1
    std::cout << "函数内：引用计数 +1" << std::endl;
    obj->SetName("Processed");
    // 函数结束时，obj 销毁，引用计数 -1
}

void Example3_FunctionParameters() {
    std::cout << "\n=== 示例3：函数参数传递 ===" << std::endl;
    
    Object::Pointer obj = Object::New();
    std::cout << "创建对象，引用计数 = 1" << std::endl;
    
    ProcessObject(obj);  // 传递时引用计数 +1，函数返回后 -1
    std::cout << "函数调用后，引用计数恢复 = 1" << std::endl;
    std::cout << "对象名称: " << obj->GetName() << std::endl;
}

// ============================================
// 示例4：函数返回值
// ============================================
Object::Pointer CreateObject(const std::string& name) {
    Object::Pointer obj = Object::New();
    obj->SetName(name);
    return obj;  // 返回值时使用移动语义，引用计数不变
}

void Example4_FunctionReturn() {
    std::cout << "\n=== 示例4：函数返回值 ===" << std::endl;
    
    Object::Pointer obj = CreateObject("ReturnedObject");
    std::cout << "从函数返回对象，引用计数 = 1" << std::endl;
    std::cout << "对象名称: " << obj->GetName() << std::endl;
}

// ============================================
// 示例5：容器中使用
// ============================================
void Example5_ContainerUsage() {
    std::cout << "\n=== 示例5：容器中使用 ===" << std::endl;
    
    std::vector<Object::Pointer> objects;
    
    // 添加对象到容器
    for (int i = 0; i < 3; i++) {
        Object::Pointer obj = Object::New();
        obj->SetName("Object" + std::to_string(i));
        objects.push_back(obj);
        // push_back 会调用拷贝构造，引用计数 +1
        // 临时 obj 销毁后，引用计数 -1
        // 最终容器中的引用计数 = 1
    }
    
    std::cout << "容器中有 " << objects.size() << " 个对象" << std::endl;
    
    // 访问容器中的对象
    for (size_t i = 0; i < objects.size(); i++) {
        std::cout << "objects[" << i << "] 名称: " 
                  << objects[i]->GetName() << std::endl;
    }
    
    // 清空容器
    objects.clear();
    std::cout << "容器清空后，所有对象自动删除" << std::endl;
}

// ============================================
// 示例6：时间戳的使用
// ============================================
void Example6_TimeStamp() {
    std::cout << "\n=== 示例6：时间戳使用 ===" << std::endl;
    
    Object::Pointer obj = Object::New();
    
    // 获取初始时间戳
    TimeStamp initialTime = obj->GetMTime();
    std::cout << "初始时间戳: " << initialTime.GetMTime() << std::endl;
    
    // 修改对象
    obj->Modified();
    TimeStamp modifiedTime = obj->GetMTime();
    std::cout << "修改后时间戳: " << modifiedTime.GetMTime() << std::endl;
    
    // 比较时间戳
    if (modifiedTime > initialTime) {
        std::cout << "对象已被修改！" << std::endl;
    }
}

// ============================================
// 示例7：类型转换（假设有继承关系）
// ============================================
/*
// 假设 PointSet 继承自 DataObject
void Example7_TypeConversion() {
    std::cout << "\n=== 示例7：类型转换 ===" << std::endl;
    
    // 创建派生类对象
    PointSet::Pointer pointSet = PointSet::New();
    pointSet->SetName("MyPointSet");
    
    // 向上转换（派生类 -> 基类）
    DataObject::Pointer dataObj = pointSet;
    std::cout << "向上转换成功，引用计数不变" << std::endl;
    
    // 向下转换（基类 -> 派生类）
    PointSet::Pointer pointSet2 = DynamicCast<PointSet>(dataObj);
    if (pointSet2.IsNotNull()) {
        std::cout << "向下转换成功" << std::endl;
        std::cout << "对象名称: " << pointSet2->GetName() << std::endl;
    } else {
        std::cout << "向下转换失败" << std::endl;
    }
}
*/

// ============================================
// 示例8：空指针检查
// ============================================
void Example8_NullCheck() {
    std::cout << "\n=== 示例8：空指针检查 ===" << std::endl;
    
    Object::Pointer obj1;  // 空指针
    Object::Pointer obj2 = Object::New();  // 有效指针
    
    // 方法1：使用 IsNull() / IsNotNull()
    if (obj1.IsNull()) {
        std::cout << "obj1 是空指针" << std::endl;
    }
    
    if (obj2.IsNotNull()) {
        std::cout << "obj2 不是空指针" << std::endl;
    }
    
    // 方法2：使用 bool 转换
    if (obj1) {
        std::cout << "obj1 有效" << std::endl;
    } else {
        std::cout << "obj1 无效" << std::endl;
    }
    
    // 方法3：使用 GetPointer()
    if (obj1.GetPointer() == nullptr) {
        std::cout << "obj1 是空指针" << std::endl;
    }
}

// ============================================
// 示例9：避免循环引用（重要！）
// ============================================
/*
// 错误示例：循环引用
class Parent : public Object {
public:
    I_OBJECT(Parent);
    static Pointer New() { return new Parent; }
    
    void SetChild(Child::Pointer child) {
        m_Child = child;  // 错误：会导致循环引用
    }
    
private:
    Child::Pointer m_Child;  // 拥有所有权
};

class Child : public Object {
public:
    I_OBJECT(Child);
    static Pointer New() { return new Child; }
    
    void SetParent(Parent::Pointer parent) {
        m_Parent = parent;  // 错误：会导致循环引用
    }
    
private:
    Parent::Pointer m_Parent;  // 拥有所有权
};

// 正确示例：使用原始指针打破循环
class Parent : public Object {
public:
    I_OBJECT(Parent);
    static Pointer New() { return new Parent; }
    
    void SetChild(Child::Pointer child) {
        m_Child = child;  // Parent 拥有 Child
    }
    
private:
    Child::Pointer m_Child;  // 拥有所有权
};

class Child : public Object {
public:
    I_OBJECT(Child);
    static Pointer New() { return new Child; }
    
    void SetParent(Parent* parent) {  // 使用原始指针，不拥有所有权
        m_Parent = parent;
    }
    
private:
    Parent* m_Parent;  // 不拥有所有权，只是引用
};
*/

// ============================================
// 主函数
// ============================================
int main() {
    Example1_BasicUsage();
    Example2_ReferenceCounting();
    Example3_FunctionParameters();
    Example4_FunctionReturn();
    Example5_ContainerUsage();
    Example6_TimeStamp();
    Example8_NullCheck();
    
    return 0;
}

// ============================================
// 关键要点总结
// ============================================
/*
1. 创建对象：始终使用 New() 工厂方法
   Object::Pointer obj = Object::New();

2. 传递对象：按值传递智能指针，自动管理引用计数
   void Function(Object::Pointer obj);

3. 检查空指针：使用 IsNull() / IsNotNull() 或 bool 转换
   if (obj.IsNotNull()) { ... }

4. 类型转换：使用 DynamicCast
   Derived::Pointer derived = DynamicCast<Derived>(base);

5. 避免循环引用：使用原始指针存储"不拥有所有权"的引用

6. 时间戳：使用 Modified() 标记对象修改，用于依赖检测

7. 内存管理：完全自动，不需要手动 delete
*/
