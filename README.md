<<<<<<< HEAD
# SyncFlow
A high-performance, zero-copy, multi-consumer video streaming framework in C++17. Inspired by NVIDIA DriveOS (NvSciStream/NvSciBuffer/NvSciSync).
=======
# SyncFlow — 高性能多消费者零拷贝视频流框架

SyncFlow 是一个用 **C++17** 实现的 **单生产者 → 多消费者 (SPMC)** 实时音视频数据流框架。

框架从 NVIDIA DriveOS 的 **NvSciStream / NvSciSync / NvSciBuf** 架构中提取核心思想，**完全用标准 C++ 独立复现**，聚焦于**多消费者并发访问共享图像数据时的同步安全与零拷贝效率**。

---

## 🎯 项目定位与价值

- **教学级工业框架复现**：用最精简的代码（核心 < 2000 行）清晰展示 NvSciStream 的“数据‑控制分离”、“无锁同步”、“零拷贝池化”三大设计支柱。
- **嵌入式与实时系统友好**：无异常、无 RTTI、两阶段初始化、固定内存分配，可轻松移植到资源受限环境。

---

## 🧠 架构设计

###  三层架构

```
Manager (流程编排)
   │
   ▼
Channel (数据流基础设施集合器)
   ├── RingPacketPool (环形 Packet 池)
   ├── 连接器 (Port)
   ├── 同步策略 (丢帧/超时)
   └── Watchdog (异常恢复)
   │
   ▼
Module (业务插件)
   ├── CameraModule (普通生产者 目前未实现)
   ├── CudaModule  (普通消费者+生产者 目前未实现)
   ├── YoloModule   (普通消费者 目前未实现)
   ├──VirtualPacketModule (虚拟生产者)
   └── DisplayModule(透传消费者)
   
```

---

## ⚡ 核心技术亮点与难点

### 亮点 1：从并发痛点推演出的 `consumer_mask` 无锁同步

- 完整推演链条：单播 → 引用计数 → 长持锁 → 短持锁+先改状态 → 重复消费竞态 → **原子位图 + epoch**。
- 用一条 `fetch_and(…)` 原子指令实现“最后一位消费者关门回收”。
- `epoch` 序列号防止 ABA 问题，并支撑异常场景下的超时回收安全性。

### 亮点 2：数据与控制分离的零拷贝内存架构

- `ImageBuffer` (数据) 只读共享，`Packet` (控制) 独立流转。
- 多消费者格式冲突通过“共享只读主 Buffer + 独立 ModBuffer”解决。
- 模块间永远只传递指针/引用，像素数据仅在被捕获时 DMA 写入一次，全程不拷贝。

### 亮点 3：双层流控与自愈机制

- **第一层（预防）**：消费者可配置内部跳帧策略，根据自身处理能力主动降采样。
- **第二层（兜底）**：独立的 `Watchdog` 线程检测消费者心跳，超时强制回收槽位、剥离僵尸消费者，并通过递增 `epoch` 防止 ABA。
- **第三层（保护）**：生产者写入前检查槽位级水位线，目标槽位未释放时阻塞或丢帧，确保不会覆盖正在读取中的数据。

### 亮点 4：务实的工程简化与取舍

- **队列级丢帧替代全链路背压**：清醒认识到实时性需求与实现成本，做出务实权衡。
- **两阶段初始化**：无参构造 + `init()` 返回 `Status`，杜绝构造函数抛异常，便于集成到不支持异常的环境。

### 难点 1：环形索引的“空/满”区分与内存屏障

- 使用逻辑上无限递增的 `write_index` / `read_index`，通过差值判断队列状态。
- 利用 `fetch_add(release)` 和 `load(acquire)` 建立生产-消费之间的内存屏障，保证写入完成后消费者看到完整数据。

### 难点 2：多消费者各自独立追赶下的槽位回收

- 每个消费者维护私有 `read_index`，不互相阻塞。
- 让每个槽位维护一个由 `Deleter` 推进的 `slot.read_index`，仅当该槽位所有 `shared_ptr` 引用计数归零时才允许覆盖，彻底消除全局水位线的竞态窗口。

### 难点 3：级联消费者（CPU → GPU → CPU）的同步边界

- 纯 CPU 链路使用环形索引极简流转。
- 一旦跨越硬件域（如 CUDA 处理），在 `Packet` 中保留硬件 Fence 字段（如 CUDA Event）进行跨硬件同步，清晰分层。

---

## 🔧 构建与运行

### 依赖
- C++17 兼容编译器 (GCC 8+ / Clang 10+)
- CMake ≥ 3.15
- (可选) GoogleTest (单元测试)

### 编译
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

### 运行端到端示例 (单生产者 → 多消费者)
```bash
./examples/multi_consumer_demo
```

### 运行单元测试
```bash
./tests/test_ring_pool
./tests/test_consumer_mask
./tests/test_watchdog
```

---

## 📊 性能与可观测性

框架内置轻量级监控指标（通过原子计数器获取）：

- 累计生产帧数、丢弃帧数
- 各消费者当前积压 = `write_index` - 消费者 `read_index`
- Watchdog 触发次数

可以在测试或运行时打印，也可对接外部监控。

---

## 🚀 未来规划

- [ ] 集成真实相机CAMERA适配层
- [ ] 支持 CUDA/TensorRT 异步推理插件
- [ ] 进程间 IPC 共享内存支持（mmap/DMA-BUF）
- [ ] 实现多路相机 Manager 动态路由

---

---

## 🙏 致谢

本项目设计思想深度参考了以下工业级框架/机制：
- **NVIDIA DriveOS**: NvSciStream / NvSciSync / NvSciBuf
- **GStreamer**: GstBuffer / GstMeta 机制
- **Linux DMA-BUF**: 零拷贝共享模型

SyncFlow 是参考成熟框架基础上的教学复现，代码为个人独立完成。
```
>>>>>>> 49d4f41 (RingPacketPool功能部署)
