# 八亿级模型 C/S 使用说明

模型包含 **801,357,926 个三角形**，已打包，无需重新打包。以下操作在同一台电脑上进行。

模型包位置：

```text
D:\10亿网格\csfull-0919-121100\server\DRIVAER-CP-SURF801357926-MULTIBLOCK-SIZED-USTAR.tar.zst
```

## 1. 启动 DataServer

打开 PowerShell，执行以下命令，启动后保持窗口打开：



```powershell
$server = "D:\10亿网格\发行包\iGameVis-2.0-Windows-x64-Full-20260923-Rebuild\iGameVisDataServer.exe"
New-Item -ItemType Directory -Force "D:\iGameVis-Publish" | Out-Null  #该命令为创建文件夹、有的话就不用创建

& $server --pack "D:\MyModels\Car01" --file "D:\iGameVis-Publish\Car01.tar.zst" --compression-level 3 --port 34571  #模型在D:\MyModels\Car01下，你可以使用任意文件，不过该目录下只存在单一模型文件，这个命令可以把这个文件打包为tar.zst至发布目录

& $server --root "D:\iGameVis-Publish" --bind 127.0.0.1 --port 34571
```

等待出现 `READY listening` 后，进行客户端操作。

## 2. 客户端打开模型

双击以下程序：

```text
D:\10亿网格\发行包\iGameVis-2.0-Windows-x64-Full-20260923-Rebuild\iGameVis.exe
```

打开 **File → Remote Model Library...**，填写：

| 字段 | 内容 |
| --- | --- |
| Host | `127.0.0.1` |
| Port | `34571` |
| Cache | `D:\iGameVis-ClientCache` |

点击 **Fetch**，选择对应的发布包，再点击 **Open**。


使用结束后，在 DataServer 窗口按 **Ctrl+C** 停止服务。
