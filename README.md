# VInput

### 自动使用Logitech或Razer驱动实现虚拟键盘和鼠标输入
如果你有Logitech鼠标，则为了避免兼容性问题，将自动使用Razer驱动

如果你有Razer鼠标，则为了避免兼容性问题，将自动使用Logitech驱动

为了防止出现驱动安装失败的问题，建议使用管理员运行

仅支持Windows 10/11 X64 

调用示例
```
import VInput


if VInput.initialize(): # 安装并初始化驱动，每次DLL加载都需要调用
    print("驱动初始化成功")
else:
    print("初始化失败，请检查管理员权限和驱动文件")

VInput.mouse_move(100, 100)    # 鼠标移动100,100 px
VInput.mouse_move_to(100, 100)    # 鼠标移动到100,100
VInput.mouse_click(VInput.VK_LBUTTON)    # 左键单击
VInput.keyboard_click(VInput.VK_A)    # 点击A键


VInput.shutdown() # 需要卸载驱动时使用，否则无需调用
```
