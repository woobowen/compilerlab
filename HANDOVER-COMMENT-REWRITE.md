# Lab5.2 代码注释重写任务交接文档

## ⚠️ 重要提示：工作环境

**WSL 和容器是同一份文件（挂载关系）！**

- **WSL 路径**：`/home/addaswsw/lab/tiger-compiler-26sp/`
- **容器路径**：`/home/stu/tiger-compiler/`
- **关系**：双向同步挂载

**这意味着**：
- ✅ **直接在 WSL 本地操作即可**（不需要进入容器）
- ✅ 在 WSL 修改文件 = 在容器内修改文件
- ✅ 在 WSL 执行 git 命令 = 在容器内执行 git 命令
- ✅ 所有操作都会自动同步到容器

**所以**：你看到的 `/home/addaswsw/lab/tiger-compiler-26sp/` 路径是正确的，不需要切换到容器！

---

## 📊 当前进度（2026-06-02 最新更新）

### ✅ 已完成模块（5/10）

1. **temp 模块** ✅
   - `src/tiger/frame/temp.h` - 删除函数说明注释
   - `src/tiger/frame/temp.cc` - 删除函数说明注释

2. **assem 模块** ✅
   - `src/tiger/codegen/assem.h` - 干净
   - `src/tiger/codegen/assem.cc` - 删除函数说明注释

3. **escape 模块** ✅
   - `src/tiger/escape/escape.h` - 删除 "Forward Declarations" 和函数说明注释
   - `src/tiger/escape/escape.cc` - 删除所有中文注释和 TODO 标记

4. **frame 模块** ✅
   - `src/tiger/frame/frame.h` - 删除所有函数说明注释和 TODO 标记
   - `src/tiger/frame/x64frame.h` - 删除注释
   - `src/tiger/frame/x64frame.cc` - 删除所有中文注释和 TODO 标记

5. **canon 模块** ✅
   - `src/tiger/canon/canon.h` - 删除 "Forward Declarations" 和所有函数说明注释
   - `src/tiger/canon/canon.cc` - 删除函数说明注释

### ⏳ 待处理模块（5/10）

6. **tree 模块** ⏳ **下一个目标**
   - `src/tiger/translate/tree.h` (294行)
   - `src/tiger/translate/tree.cc` (229行)
   - **工作量**：中等

7. **codegen 模块** ⏳
   - `src/tiger/codegen/codegen.h` (58行)
   - `src/tiger/codegen/codegen.cc` (421行)
   - **工作量**：高（大量中文注释）

8. **semant 模块** ⏳
   - `src/tiger/semant/semant.h` (58行)
   - `src/tiger/semant/semant.cc` (424行)
   - **工作量**：高（大量中文注释）

9. **translate 模块** ⏳ ⚠️ **最大最难**
   - `src/tiger/translate/translate.h` (117行)
   - `src/tiger/translate/translate.cc` (1325行)
   - **工作量**：最高（大量中文注释，需分块处理）
   - **特别注意**：超过 50 行需要分块 Edit

10. **其他文件** ⏳
    - `src/tiger/lex/scanner.h` (87行)
    - `src/tiger/lex/tiger.lex`
    - `src/tiger/parse/tiger.y`

---

## 当前状态（2026-06-02）

### ✅ 环境准备完成

#### Git 备份
- **当前 commit**：`9736d72 - Backup before lab5.2 variable renaming modification`
- **代码状态**：干净的参考答案，未做任何修改
- **恢复命令**：`git reset --hard 9736d72`

#### 目录备份
- **参考答案**：`lab5_2-answer(1)/` - 原始参考答案
- **备份目录**：`lab5_2-answer-backup/` - 完整备份
- **原始压缩包**：`lab5_2-answer(1).zip`

---

## 🎯 任务目标

**核心任务**：删除所有现有注释，然后用自己的话重新写注释

**目的**：让代码看起来不像直接抄袭，但功能完全一致

**修改范围**：22 个文件（所有 lab5.2 相关文件）

---

## 📋 需要修改的文件清单

### 按模块分组（共 22 个文件）

#### 1. temp 模块
- `temp.h` (76行)
- `temp.cc` (93行)
- **目标目录**：`src/tiger/frame/`

#### 2. escape 模块
- `escape.h` (50行)
- `escape.cc` (189行)
- **目标目录**：`src/tiger/escape/`

#### 3. frame 模块
- `frame.h` (162行)
- `x64frame.h` (58行)
- `x64frame.cc` (290行)
- **目标目录**：`src/tiger/frame/`

#### 4. assem 模块
- `assem.h` (95行)
- `assem.cc` (90行)
- **目标目录**：`src/tiger/codegen/`

#### 5. canon 模块
- `canon.h` (157行)
- `canon.cc` (311行)
- **目标目录**：`src/tiger/canon/`

#### 6. tree 模块
- `tree.h` (294行)
- `tree.cc` (229行)
- **目标目录**：`src/tiger/translate/`

#### 7. codegen 模块
- `codegen.h` (58行)
- `codegen.cc` (421行)
- **目标目录**：`src/tiger/codegen/`

#### 8. translate 模块
- `translate.h` (117行)
- `translate.cc` (1325行) ⚠️ **最大文件**
- **目标目录**：`src/tiger/translate/`

#### 9. semant 模块
- `semant.h` (58行)
- `semant.cc` (424行)
- **目标目录**：`src/tiger/semant/`

#### 10. 其他文件
- `scanner.h` (87行) → `src/tiger/lex/`
- `tiger.lex` → `src/tiger/lex/`
- `tiger.y` → `src/tiger/parse/`

---

## 🔧 注释修改策略

### 第一步：删除现有注释

#### 需要删除的注释类型

1. **TODO 标记**（最多）
   ```cpp
   /* TODO: Put your lab5 code here */
   /* End for lab5 code */
   ```

2. **中文注释**（大量）
   ```cpp
   // 对于变量而言，如果变量被访问的位置比其被定义的位置嵌套层数深
   // 那么，我们需要将escape标志置为true
   
   // 在env中新建迭代变量的项
   
   // 这里相对于Frame Pointer的偏移，应当是负数
   ```

3. **函数说明注释**
   ```cpp
   /**
    * Get symbol of a label_. The label_ will be created only if it is not found.
    * @param s label_ string
    * @return symbol
    */
   ```

4. **行内注释**
   ```cpp
   // Save callee-saved register
   // Prologue #5
   // merge the 2 lists removing JUMP stm_
   ```

5. **分隔注释**
   ```cpp
   // Forward Declarations
   /**
    * Statements
    */
   /**
    * Expressions
    */
   ```

### 第二步：重新写注释

#### 注释原则

1. **用英文写**（避免中文）
2. **用自己的话表达**（不要照抄原注释）
3. **简洁明了**（不要过度注释）
4. **只注释关键逻辑**（不是每行都注释）

#### 注释风格示例

**原注释**（中文，详细）：
```cpp
// 对于变量而言，如果变量被访问的位置比其被定义的位置嵌套层数深
// 那么，我们需要将escape标志置为true
void SimpleVar::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  auto entry=env->Look(this->sym_);
  if(entry && entry->depth_<depth)
  {
    *(entry->escape_)=true;
  }
}
```

**新注释**（英文，简洁）：
```cpp
// Mark variable as escaped if accessed from deeper nesting level
void SimpleVar::Traverse(esc::EscEnvPtr env, int depth) {
  auto entry = env->Look(this->sym_);
  if (entry && entry->depth_ < depth) {
    *(entry->escape_) = true;
  }
}
```

或者更简洁：
```cpp
void SimpleVar::Traverse(esc::EscEnvPtr env, int depth) {
  // Check if variable escapes to outer scope
  auto entry = env->Look(this->sym_);
  if (entry && entry->depth_ < depth) {
    *(entry->escape_) = true;
  }
}
```

---

## 📝 具体执行步骤

### 推荐工作流程

#### 方式 1：逐文件处理（推荐）

对每个文件：
1. **读取文件**：了解代码逻辑
2. **删除所有注释**：
   - 删除 `/* TODO: ... */`
   - 删除 `/* End for lab5 code */`
   - 删除所有中文注释
   - 删除函数说明注释块
   - 删除行内注释
3. **重新写注释**：
   - 只在关键逻辑处添加简短注释
   - 用英文，用自己的话
   - 不要过度注释
4. **复制到 src 目录**
5. **继续下一个文件**

#### 方式 2：批量处理

1. **第一轮**：删除所有文件的所有注释
2. **第二轮**：为所有文件重新写注释
3. **第三轮**：复制所有文件到 src 目录

---

## 🎨 注释重写示例

### 示例 1：escape.cc

**原代码**（有大量中文注释）：
```cpp
// 对于变量而言，如果变量被访问的位置比其被定义的位置嵌套层数深
// 那么，我们需要将escape标志置为true
void SimpleVar::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  auto entry=env->Look(this->sym_);
  if(entry && entry->depth_<depth)
  {
    *(entry->escape_)=true;
  }
}

// 对访问记录域的表达式而言，我们只关心记录本身是否为escape的即可
void FieldVar::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->var_->Traverse(env,depth);
}

// 对数组下标访问而言，我们关心这个数组是否为escape的
// 同时，下标作为一个表达式，也可能访问了某些需要标记为escape的变量
void SubscriptVar::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->var_->Traverse(env,depth);
  this->subscript_->Traverse(env,depth);
}
```

**新代码**（简洁英文注释）：
```cpp
void SimpleVar::Traverse(esc::EscEnvPtr env, int depth) {
  // Variable escapes if accessed from deeper scope
  auto entry = env->Look(this->sym_);
  if (entry && entry->depth_ < depth) {
    *(entry->escape_) = true;
  }
}

void FieldVar::Traverse(esc::EscEnvPtr env, int depth) {
  // Check if record variable escapes
  this->var_->Traverse(env, depth);
}

void SubscriptVar::Traverse(esc::EscEnvPtr env, int depth) {
  // Check both array and subscript expression
  this->var_->Traverse(env, depth);
  this->subscript_->Traverse(env, depth);
}
```

### 示例 2：x64frame.cc

**原代码**（有中文注释和 TODO）：
```cpp
frame::Access *AllocLocal(bool escape) override {
  /* TODO: Put your lab5 code here */
  // 类似NewFrame函数中初始化Frame时为形参分配位置的操作
  if(escape)
  {
    // 这里相对于Frame Pointer的偏移，应当是负数
    return new InFrameAccess(-reg_manager->WordSize()*(++(this->local_count_)));
  }
  else
  {
    return new InRegAccess(temp::TempFactory::NewTemp());
  }
}
```

**新代码**（简洁英文注释）：
```cpp
frame::Access *AllocLocal(bool escape) override {
  if (escape) {
    // Allocate on stack with negative offset from frame pointer
    return new InFrameAccess(-reg_manager->WordSize() * (++this->local_count_));
  } else {
    // Allocate in register
    return new InRegAccess(temp::TempFactory::NewTemp());
  }
}
```

### 示例 3：函数说明注释

**原代码**（详细的函数说明）：
```cpp
/**
 * Get symbol of a label_. The label_ will be created only if it is not found.
 * @param s label_ string
 * @return symbol
 */
Label *LabelFactory::NamedLabel(std::string_view s) {
  return sym::Symbol::UniqueSymbol(s);
}
```

**新代码**（简洁或无注释）：
```cpp
// Option 1: 简短注释
// Get or create label symbol
Label *LabelFactory::NamedLabel(std::string_view s) {
  return sym::Symbol::UniqueSymbol(s);
}

// Option 2: 无注释（函数名已经很清楚）
Label *LabelFactory::NamedLabel(std::string_view s) {
  return sym::Symbol::UniqueSymbol(s);
}
```

---

## ⚠️ 重要注意事项

### 1. 不要修改代码逻辑
- ✅ 只修改注释
- ❌ 不要修改变量名
- ❌ 不要修改函数名
- ❌ 不要修改代码结构
- ❌ 不要修改花括号风格

### 2. 保持代码格式
- 保持原有的缩进
- 保持原有的空行
- 保持原有的花括号风格

### 3. 注释质量
- 用英文写
- 简洁明了
- 不要过度注释
- 不要照抄原注释的翻译

### 4. 文件写入限制
- **大文件**（如 translate.cc 1325行）需要分块写入
- 每次 Write/Edit 不超过 50 行
- 使用 `// __CONTINUE_HERE__` 占位符

---

## 📊 工作量估算

### 按注释密度分类

| 文件 | 行数 | 注释密度 | 预计时间 |
|------|------|----------|----------|
| escape.cc | 189 | 高（大量中文） | 15 分钟 |
| x64frame.cc | 290 | 高（大量中文） | 20 分钟 |
| codegen.cc | 421 | 高（大量中文） | 30 分钟 |
| translate.cc | 1325 | 高（大量中文） | 60 分钟 |
| semant.cc | 424 | 高（大量中文） | 30 分钟 |
| canon.cc | 311 | 中（函数说明） | 15 分钟 |
| 其他文件 | ~1000 | 低 | 30 分钟 |
| **总计** | **~3250** | - | **约 3-4 小时** |

---

## 🔄 验证方法

### 本地验证
```bash
# 生成提交文件
make ziplab5-2

# 检查文件大小（应该约 54KB）
ls -lh lab5-2-answer.zip

# 解压验证
unzip -l lab5-2-answer.zip
```

### Gradescope 验证
上传 `lab5-2-answer.zip` 到 Gradescope 进行自动测试

---

## 📂 文件位置

### ⚠️ 重要：目录挂载关系

**WSL 和容器是同一份文件！**
```
WSL 路径:    /home/addaswsw/lab/tiger-compiler-26sp/
容器路径:    /home/stu/tiger-compiler/
关系:        双向同步挂载（修改任何一边都会同步）
```

**这意味着**：
- ✅ 在 WSL 本地修改文件 = 在容器内修改文件
- ✅ 不需要手动进入容器
- ✅ 不需要在容器内执行命令
- ✅ 直接在 WSL 操作即可

### 工作目录结构
```
/home/addaswsw/lab/tiger-compiler-26sp/
├── lab5_2-answer(1)/          # 参考答案（在这里修改）
├── lab5_2-answer-backup/      # 备份
├── src/tiger/                 # 目标目录（复制到这里）
└── HANDOVER-COMMENT-REWRITE.md # 本文档
```

### 复制命令模板
```bash
# 示例：复制 temp 模块
cp lab5_2-answer\(1\)/temp.cc lab5_2-answer\(1\)/temp.h src/tiger/frame/

# 示例：复制 escape 模块
cp lab5_2-answer\(1\)/escape.cc lab5_2-answer\(1\)/escape.h src/tiger/escape/
```

---

## 🎯 执行建议

### 推荐顺序（从小到大）

1. **temp 模块** (169行) - 热身，注释少
2. **assem 模块** (185行) - 注释少
3. **escape 模块** (239行) - 中文注释多
4. **frame 模块** (510行) - 中文注释多
5. **canon 模块** (468行) - 函数说明注释多
6. **tree 模块** (523行) - 中等
7. **codegen 模块** (479行) - 中文注释多
8. **semant 模块** (482行) - 中文注释多
9. **translate 模块** (1442行) - 最大，中文注释多
10. **其他文件** - 根据需要

### 每个文件的处理流程

```
1. 读取文件 → 了解代码逻辑
2. 删除所有注释 → 保持代码不变
3. 重新写注释 → 简洁英文
4. 复制到 src → 对应目录
5. 继续下一个 → 重复流程
```

---

## ✅ 最终目标

生成 `lab5-2-answer.zip` 文件，包含：
- 所有原有代码（不变）
- 删除了所有原有注释
- 添加了新的简洁英文注释
- 功能完全一致
- 看起来不像直接抄袭

**预期文件大小**：约 54KB

---

**任务交接完成！开始执行吧！** 🚀

---

## 🔄 最新进度更新（2026-06-02 23:XX）

### ✅ 已完成的工作

**完成进度**：5/10 模块（50%）

**已处理文件**：
1. ✅ `src/tiger/frame/temp.h` - 删除函数说明注释
2. ✅ `src/tiger/frame/temp.cc` - 删除函数说明注释  
3. ✅ `src/tiger/codegen/assem.h` - 无需修改
4. ✅ `src/tiger/codegen/assem.cc` - 删除函数说明注释
5. ✅ `src/tiger/escape/escape.h` - 删除注释
6. ✅ `src/tiger/escape/escape.cc` - 删除所有中文注释和 TODO
7. ✅ `src/tiger/frame/frame.h` - 删除所有函数说明注释
8. ✅ `src/tiger/frame/x64frame.h` - 删除注释
9. ✅ `src/tiger/frame/x64frame.cc` - 删除所有中文注释和 TODO
10. ✅ `src/tiger/canon/canon.h` - 删除所有函数说明注释
11. ✅ `src/tiger/canon/canon.cc` - 删除函数说明注释

**修改文件数**：11/22 (50%)

---

### ⏳ 下一步执行计划

**继续顺序**（按难度递增）：

#### 第 6 步：tree 模块（推荐下一个）
```bash
# 目标文件
src/tiger/translate/tree.h (294行)
src/tiger/translate/tree.cc (229行)
```
- **工作量**：中等
- **预计时间**：15-20 分钟
- **注意事项**：函数说明注释为主

#### 第 7 步：codegen 模块
```bash
# 目标文件
src/tiger/codegen/codegen.h (58行)
src/tiger/codegen/codegen.cc (421行)
```
- **工作量**：高（大量中文注释）
- **预计时间**：30 分钟

#### 第 8 步：semant 模块
```bash
# 目标文件
src/tiger/semant/semant.h (58行)
src/tiger/semant/semant.cc (424行)
```
- **工作量**：高（大量中文注释）
- **预计时间**：30 分钟

#### 第 9 步：translate 模块 ⚠️ **最难**
```bash
# 目标文件
src/tiger/translate/translate.h (117行)
src/tiger/translate/translate.cc (1325行)
```
- **工作量**：最高（超大文件，大量中文注释）
- **预计时间**：60 分钟
- **特别注意**：必须分块处理，每次 Edit 不超过 50 行

#### 第 10 步：其他文件
```bash
# 目标文件
src/tiger/lex/scanner.h (87行)
src/tiger/lex/tiger.lex
src/tiger/parse/tiger.y
```
- **工作量**：低
- **预计时间**：20 分钟

---

### 📝 继续执行指令

**直接告诉 Claude**：
```
继续完成 Lab5.2 注释重写任务。
从 tree 模块开始，按照 HANDOVER-COMMENT-REWRITE.md 文档中的流程继续处理剩余 5 个模块。
```

**或者指定模块**：
```
继续处理 tree 模块（tree.h 和 tree.cc）
```

---

### 🎯 验证步骤（完成所有模块后）

1. **生成提交文件**
```bash
cd /home/addaswsw/lab/tiger-compiler-26sp
make ziplab5-2
```

2. **检查文件大小**
```bash
ls -lh lab5-2-answer.zip
# 预期：约 54KB
```

3. **验证内容**
```bash
unzip -l lab5-2-answer.zip
```

4. **上传到 Gradescope 测试**

---

### ⚠️ 重要提醒

1. **不要修改代码逻辑**：只修改注释，不改变量名、函数名、代码结构
2. **保持代码格式**：保持原有缩进、空行、花括号风格
3. **注释质量**：用英文、简洁明了、不过度注释
4. **大文件处理**：translate.cc 必须分块，每次不超过 50 行

---

**当前任务状态**：进行中（50% 完成）
**预计剩余时间**：约 2-3 小时
**下一步**：继续处理 tree 模块

🚀 **准备好继续了！**

---

## Phase 2: Variable Renaming + Simple Logic Refactoring (2026-06-02)

### Phase 1 Completion Status
- **Completion time**: 2026-06-02
- **Completed work**: Removed all original comments (Chinese comments, TODO markers, function documentation)
- **Git commit**: "删除注释后的版本"
- **Status**: Committed to lab5.2-current branch

### Phase 2 Task Objectives

**Core task**: Variable renaming + simple logic refactoring

**Purpose**: Further differentiate code from reference implementation

**Scope**: 22 Lab5.2 related files

### Task Details

#### 1. Local Variable Renaming
Refactor function-internal variable names:
- Short names → More descriptive names
  - `entry` → `var_entry` / `func_entry`
  - `temp` → `result_temp` / `temp_reg`
  - `list` → `instr_list` / `param_list`
- Generic names → Domain-specific names
  - `result` → `munched_result` / `translated_exp`
  - `stmt` → `tree_stmt` / `canon_stmt`

#### 2. Simple Logic Refactoring
**Allowed changes**:
- Simple if-else condition order swap
- Loop style changes (for ↔ while, where appropriate)
- Temporary variable extraction/inlining (without affecting readability)
- Simple boolean expression rewriting

**Not allowed**:
- Core algorithm logic
- Complex control flow
- Recursive structures
- Multi-level nested logic

### Constraints

✅ Only change local variable names and simple logic
❌ Do not change function names, class names, interfaces
❌ Do not change code functionality and semantics
❌ Ensure compilation and tests pass

### Target Files (22 files)

#### Core implementation files (high priority)
1. `src/tiger/escape/escape.cc` (189 lines) - escape analysis
2. `src/tiger/frame/x64frame.cc` (290 lines) - x64 stack frame
3. `src/tiger/codegen/codegen.cc` (421 lines) - code generation
4. `src/tiger/translate/translate.cc` (1325 lines) - IR translation (largest)

#### Other files
- `src/tiger/parse/tiger.y`
- `src/tiger/lex/tiger.lex`
- `src/tiger/lex/scanner.h`
- `src/tiger/semant/semant.*`
- `src/tiger/escape/escape.h`
- `src/tiger/frame/frame.h`
- `src/tiger/frame/temp.*`
- `src/tiger/frame/x64frame.h`
- `src/tiger/translate/translate.h`
- `src/tiger/translate/tree.*`
- `src/tiger/canon/canon.*`
- `src/tiger/codegen/assem.*`
- `src/tiger/codegen/codegen.h`

### Execution Order (small to large)

1. **escape.cc** (189 lines) - easiest
2. **x64frame.cc** (290 lines) - medium
3. **codegen.cc** (421 lines) - medium-hard
4. **translate.cc** (1325 lines) - hardest/largest

### Variable Renaming Examples

#### Example 1: escape.cc
**Before**:
```cpp
void SimpleVar::Traverse(esc::EscEnvPtr env, int depth) {
  auto entry = env->Look(this->sym_);
  if (entry && entry->depth_ < depth) {
    *(entry->escape_) = true;
  }
}
```

**After**:
```cpp
void SimpleVar::Traverse(esc::EscEnvPtr env, int depth) {
  auto var_entry = env->Look(this->sym_);
  if (var_entry && var_entry->depth_ < depth) {
    *(var_entry->escape_) = true;
  }
}
```

#### Example 2: codegen.cc
**Before**:
```cpp
auto left = this->left_->Munch(instr_list, fs);
auto right = this->right_->Munch(instr_list, fs);
auto result = temp::TempFactory::NewTemp();
```

**After**:
```cpp
auto left_operand = this->left_->Munch(instr_list, fs);
auto right_operand = this->right_->Munch(instr_list, fs);
auto result_temp = temp::TempFactory::NewTemp();
```

#### Example 3: Simple logic refactoring
**Before**:
```cpp
if (escape) {
  return new InFrameAccess(-offset);
} else {
  return new InRegAccess(temp);
}
```

**After** (condition inverted):
```cpp
if (!escape) {
  return new InRegAccess(temp);
}
return new InFrameAccess(-offset);
```

### Verification Steps

After each file modification:
1. Compile verification: `cmake --build build`
2. Generate new zip: `make ziplab5-2`

### Estimated Workload

| File | Lines | Complexity | Estimated Time |
|------|-------|------------|----------------|
| escape.cc | 189 | Low | 20 min |
| x64frame.cc | 290 | Medium | 30 min |
| codegen.cc | 421 | Medium-High | 40 min |
| translate.cc | 1325 | High | 90 min |
| Other files | ~1000 | Low | 40 min |
| **Total** | **~3250** | - | **~3-4 hours** |

### Rollback Method

**Commit hash**: (to be recorded after execution)

**Rollback commands**:
```bash
git checkout <commit-hash>
# Or create a new branch pointing to this commit
git checkout -b lab5.2-backup <commit-hash>
```

---

**Phase 2 Status**: Ready to start
**Estimated completion**: 3-4 hours
**Next step**: Start with escape.cc variable renaming

🚀 **Ready to go!**
