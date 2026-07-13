# bsp/ —— 板级支持包（Board Support Package）
对芯片外设做一层统一封装，向上层屏蔽寄存器与 HAL 细节。包含：I2C 共享总线、两路 UART、
公共 GPIO/LED、微秒级延时，以及基于片内 Flash 模拟的 EEPROM 存储。
modules 与 app 只调用本层导出的接口，不直接操作 HAL，以便移植与统一维护。
