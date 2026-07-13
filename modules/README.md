# modules/ —— 外设驱动
每个传感器/外设占一个子目录，统一实现 `xxx_init()`（上电初始化）与 `xxx_measure()`（采集一次），
接口约定见 include/health_if.h。未完成的模块先以返回 `HS_NOT_IMPLEMENTED` 的桩占位，
保证整机始终可编译、可联调，再逐步替换为真实现。
