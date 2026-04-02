@echo off
echo ================================
echo ODrive 电机调试脚本
echo ================================
echo.
echo 正在启动 odrivetool...
echo.
echo 请在 odrivetool 中手动执行以下命令：
echo.
echo 1. 检查状态：
echo    odrv0.vbus_voltage
echo    hex(odrv0.axis0.error)
echo    hex(odrv0.axis0.motor.error)
echo    hex(odrv0.axis0.encoder.error)
echo.
echo 2. 清除错误：
echo    odrv0.axis0.error = 0
echo    odrv0.axis0.motor.error = 0
echo    odrv0.axis0.encoder.error = 0
echo.
echo 3. 进入闭环控制：
echo    odrv0.axis0.requested_state = 8
echo    odrv0.axis0.current_state
echo.
echo 4. 测试电机：
echo    odrv0.axis0.controller.input_vel = 0.5
echo    odrv0.axis0.encoder.vel_estimate
echo.
echo 5. 停止电机：
echo    odrv0.axis0.controller.input_vel = 0
echo    odrv0.axis0.requested_state = 1
echo.
echo ========================================
echo.
odrivetool
