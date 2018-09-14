
Joylink v2.0.7

Joylink 相关资料和原始 SDK 请参考 https://smartdev.jd.com/docCenterDownload/list/2 。
但由于Joylink 资料处于不断变更中，所以无法保证上述链接地址一直有效。具体 Joylink 资料，和 Joylink 云平台构建方法，请以 Joylink 官方资料为准。
本工程支持 joylink 一键配网、softap 配网以及 ble 配网。

Joylink SDK 相关版本信息为：
  * JoyLink2.0 SDK V2.0.7
  * joylink_smnt_v3.0.11
  * joylink_softap_V3.0.4

## Joylink 云平台搭建
  * 请参考 Joylink 官方相关指导文档
  
## Joylink 手机 APP
  * 请根据 Joylink 官方相关指导文档，从 Joylink 官方获取
  
## 工程下载
  * 下载全新工程  
    运行命令: `git clone --recursive https://github.com/espressif/esp32-joylink-demo.git`
  * 更新已有工程  
    运行命令: `git pull`
  * 更新默认 SDK:  
    运行命令: `git submodule update --recursive --init`
    
## 编译方法
  * 进入工程目录，直接运行 make 编译
  * 通过 make flash 进行烧录，关于 ESP_IDF 的更多信息，请参考 https://github.com/espressif/esp-idf
  * 可以通过 `make menuconfig > Component config > Joylink` 来修改自己的设备信息
  * 可以通过修改 `components/joylink_cloud/port/extern` 目录下的文件，来适配自己的设备
