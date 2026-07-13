# modules/ —— 外设驱动
每个传感器/外设一个子目录，统一实现 `xxx_init()/xxx_measure()`（见 include/health_if.h），未完成时返回 `HS_NOT_IMPLEMENTED`。
