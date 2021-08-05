# 视频浏览器

一个解析视频地址的浏览器

[示例下载](https://lucidu.cn/article/jqdkgl)

## adb

```
# 远程调试
adb tcpip 5000
adb connect 192.168.0.101

// 打印已安装程序列表
adb shell "pm list packages"

// 安装程序
adb shell
settings put global package_verifier_enable 0
adb install v2rayNG_1.6.16_armeabi-v7a.apk

// 安装分割程序
adb install-multiple 1.apk 2.apk 3.apk 4.apk 5.apk 6.apk 7.apk 8.apk
```