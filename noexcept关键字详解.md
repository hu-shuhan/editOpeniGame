# C++ noexcept 关键字详解

## 什么是 noexcept？

`noexcept` 是 C++11 引入的关键字，用于**异常规范（Exception Specification）**，告诉编译器某个函数**不会抛出异常**。

## 基本语法

### 1. 函数声明不抛出异常

```cpp
void MyFunction() noexcept;
```

**含义**：这个函数保证不会抛出任何异常。

### 2. 条件性 noexcept

```cpp
void MyFunction() noexcept(true);   // 不抛出异常
void MyFunction() noexcept(false); // 可能抛出异常
```

**含义**：根据条件决定是否可能抛出异常。

### 3. 在 iGameVis 中的使用

```cpp
// 构造函数
SmartPointer(ObjectType* p) noexcept
    : m_Pointer(p) {
    this->Register();
}

// 析构函数
~SmartPointer() { this->UnRegister(); }

// 操作符重载
ObjectType* operator->() const noexcept { return m_Pointer; }
```

---

## 为什么需要 noexcept？

### 1. 性能优化

**编译器优化**：
- 如果函数标记为 `noexcept`，编译器知道不会抛出异常
- 可以移除异常处理代码，生成更高效的代码
- 某些标准库容器（如 `std::vector`）在移动操作时会检查 `noexcept`

**示例**：
```cpp
// 没有 noexcept
void CopyData(const Data& src) {
    // 编译器需要生成异常处理代码
    m_Data = src;  // 可能抛出异常
}

// 有 noexcept
void CopyData(const Data& src) noexcept {
    // 编译器知道不会抛出异常，可以优化
    m_Data = src;  // 保证不抛出异常
}
```

### 2. 移动语义优化

**std::vector 的行为**：
- 如果移动构造函数是 `noexcept`，`std::vector` 会优先使用移动
- 如果不是 `noexcept`，`std::vector` 可能使用拷贝（更安全但更慢）

**示例**：
```cpp
class MyClass {
public:
    // 移动构造函数
    MyClass(MyClass&& other) noexcept {  // ✅ 标记为 noexcept
        m_Data = std::move(other.m_Data);
    }
    
    // 如果没有 noexcept
    // MyClass(MyClass&& other) {  // ❌ 可能抛出异常
    //     std::vector 会使用拷贝而不是移动
    // }
};

std::vector<MyClass> vec;
vec.push_back(MyClass());  // 如果移动构造函数是 noexcept，使用移动
                           // 否则使用拷贝（更慢）
```

### 3. 契约保证

**文档作用**：
- 告诉调用者：这个函数不会抛出异常
- 调用者不需要写 `try-catch`
- 提高代码可读性

---

## noexcept 的实际效果

### 示例1：基本使用

```cpp
// 标记为 noexcept
void SafeFunction() noexcept {
    int x = 10;
    int y = 20;
    int sum = x + y;  // 不会抛出异常
}

// 没有 noexcept
void UnsafeFunction() {
    throw std::runtime_error("Error");  // 可能抛出异常
}

// 调用
try {
    SafeFunction();   // 不需要 try-catch（保证不抛出）
    UnsafeFunction(); // 需要 try-catch
} catch (...) {
    // 只会捕获 UnsafeFunction 的异常
}
```

### 示例2：在智能指针中的应用

```cpp
// 构造函数标记为 noexcept
SmartPointer(ObjectType* p) noexcept
    : m_Pointer(p) {
    this->Register();  // 只是增加引用计数，不会抛出异常
}

// 为什么是 noexcept？
// 1. Register() 只是原子操作 ++m_ReferenceCount，不会失败
// 2. 指针赋值不会抛出异常
// 3. 构造函数失败不会导致异常，而是返回空指针
```

### 示例3：操作符重载

```cpp
// 操作符重载标记为 noexcept
ObjectType* operator->() const noexcept { 
    return m_Pointer;  // 只是返回指针，不会抛出异常
}

ObjectType& operator*() const noexcept { 
    return *m_Pointer;  // 解引用，不会抛出异常
}

// 为什么是 noexcept？
// 指针操作（返回、解引用）本身不会抛出异常
// 如果指针无效，是未定义行为，不是异常
```

---

## noexcept 的规则

### 1. 如果函数抛出异常会怎样？

```cpp
void MyFunction() noexcept {
    throw std::runtime_error("Error");  // ❌ 违反 noexcept 约定
}

// 结果：
// - 程序会调用 std::terminate()
// - 程序立即终止（不会调用析构函数）
// - 这是严重错误，应该避免
```

**重要**：如果标记了 `noexcept`，就**绝对不能**抛出异常！

### 2. 析构函数应该总是 noexcept

```cpp
class MyClass {
public:
    ~MyClass() noexcept {  // ✅ 推荐：析构函数应该是 noexcept
        // 清理资源
    }
    
    // 或者不写，默认就是 noexcept
    ~MyClass() {  // 默认 noexcept
    }
};
```

**原因**：
- 析构函数在异常处理过程中被调用
- 如果析构函数抛出异常，会导致 `std::terminate()`
- 标准库要求析构函数是 `noexcept`

### 3. 移动操作应该标记 noexcept

```cpp
class MyClass {
public:
    // 移动构造函数
    MyClass(MyClass&& other) noexcept {  // ✅ 推荐
        m_Data = std::move(other.m_Data);
    }
    
    // 移动赋值运算符
    MyClass& operator=(MyClass&& other) noexcept {  // ✅ 推荐
        if (this != &other) {
            m_Data = std::move(other.m_Data);
        }
        return *this;
    }
};
```

**原因**：
- 标准库容器会检查移动操作是否是 `noexcept`
- 如果是 `noexcept`，会使用移动（更快）
- 如果不是，会使用拷贝（更安全但更慢）

---

## 在 iGameVis 中的使用场景

### 场景1：构造函数

```cpp
// 默认构造函数
constexpr SmartPointer() noexcept = default;

// 从指针构造
SmartPointer(ObjectType* p) noexcept
    : m_Pointer(p) {
    this->Register();  // 原子操作，不会抛出异常
}
```

**为什么是 noexcept**：
- 指针赋值不会抛出异常
- `Register()` 只是原子操作，不会失败
- 构造函数失败不会抛出异常

### 场景2：移动构造函数

```cpp
// 移动构造函数
SmartPointer(SmartPointer<ObjectType>&& p) noexcept
    : m_Pointer(p.m_Pointer) {
    p.m_Pointer = nullptr;  // 只是赋值，不会抛出异常
}
```

**为什么是 noexcept**：
- 指针赋值不会抛出异常
- 移动操作应该是高效的，不应该抛出异常
- 标准库容器会检查这个

### 场景3：操作符重载

```cpp
// 解引用操作符
ObjectType* operator->() const noexcept { 
    return m_Pointer;  // 返回指针，不会抛出异常
}

ObjectType& operator*() const noexcept { 
    return *m_Pointer;  // 解引用，不会抛出异常
}
```

**为什么是 noexcept**：
- 指针操作本身不会抛出异常
- 如果指针无效，是未定义行为，不是异常

### 场景4：Swap 操作

```cpp
void Swap(SmartPointer& other) noexcept {
    ObjectType* tmp = this->m_Pointer;
    this->m_Pointer = other.m_Pointer;
    other.m_Pointer = tmp;  // 只是交换指针，不会抛出异常
}
```

**为什么是 noexcept**：
- 指针交换操作不会抛出异常
- `Swap` 操作应该是高效的，不应该抛出异常

---

## noexcept 的检查

### 1. noexcept 操作符

```cpp
// 检查函数是否声明为 noexcept
bool isNoexcept = noexcept(MyFunction());

if (isNoexcept) {
    std::cout << "MyFunction 是 noexcept" << std::endl;
}
```

### 2. 条件性 noexcept

```cpp
// 根据条件决定是否 noexcept
template<typename T>
void Process(T&& value) noexcept(noexcept(std::move(value))) {
    // 如果 std::move(value) 是 noexcept，这个函数也是 noexcept
    auto moved = std::move(value);
}
```

---

## 最佳实践

### ✅ 应该使用 noexcept 的情况

1. **析构函数**：总是应该 `noexcept`
   ```cpp
   ~MyClass() noexcept { }
   ```

2. **移动操作**：移动构造函数和移动赋值运算符
   ```cpp
   MyClass(MyClass&&) noexcept { }
   MyClass& operator=(MyClass&&) noexcept { }
   ```

3. **简单操作**：指针操作、基本类型操作
   ```cpp
   int* GetPointer() const noexcept { return m_Ptr; }
   ```

4. **Swap 操作**：交换操作应该是 `noexcept`
   ```cpp
   void Swap(MyClass& other) noexcept { }
   ```

### ❌ 不应该使用 noexcept 的情况

1. **可能抛出异常的函数**
   ```cpp
   void ReadFile() {
       // 可能抛出 std::ifstream::failure
       std::ifstream file("data.txt");
   }
   ```

2. **调用可能抛出异常的函数**
   ```cpp
   void ProcessData() {
       data->Process();  // Process() 可能抛出异常
   }
   ```

3. **不确定的函数**：如果不确定是否会抛出异常，不要标记 `noexcept`

---

## 总结

### 核心概念

1. **noexcept** = 保证函数不会抛出异常
2. **性能优化**：编译器可以生成更高效的代码
3. **移动语义**：标准库容器会检查 `noexcept` 来决定使用移动还是拷贝
4. **契约保证**：告诉调用者函数的行为

### 在 iGameVis 中的使用

- ✅ **构造函数**：`noexcept`（指针操作不会抛出异常）
- ✅ **移动操作**：`noexcept`（提高性能）
- ✅ **操作符重载**：`noexcept`（指针操作不会抛出异常）
- ✅ **Swap 操作**：`noexcept`（交换操作不会抛出异常）

### 记忆要点

- **noexcept** = "不会抛出异常"
- 如果标记了 `noexcept`，就绝对不能抛出异常
- 析构函数和移动操作应该总是 `noexcept`
- 简单操作（指针、基本类型）通常是 `noexcept`
