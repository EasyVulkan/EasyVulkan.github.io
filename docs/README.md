# EasyVulkan.github.io
入门级Vulkan中文教程<br>
主页：https://easyvulkan.github.io/

联系请：<br>
1.捉虫/答疑，发Issue，开新的，或者发在[【长期】 捉虫/答疑集中贴](https://github.com/EasyVulkan/EasyVulkan.github.io/issues/7)。<br>
2.反馈/交流/答疑，发Discussion。

2024.06.25 [Ch8-5 sRGB色彩空间和开启HDR](https://easyvulkan.github.io/Ch8-5%20sRGB%E8%89%B2%E5%BD%A9%E7%A9%BA%E9%97%B4%E4%B8%8E%E5%BC%80%E5%90%AFHDR.html)完成。<br>
2024.04.02 代码里加了CMakeLists.txt（我不确定Visual Studio之外是什么效果，谁愿意的话可以fork到私有仓库，帮我改一下然后pull request给我）

## 章节目录

### 到画出三角形为止（请依序阅读）

[Ch1-0 准备工作](https://easyvulkan.github.io/Ch1-0%20%E5%87%86%E5%A4%87%E5%B7%A5%E4%BD%9C.html)<br>
[Ch1-1 创建GLFW窗口](https://easyvulkan.github.io/Ch1-1%20%E5%88%9B%E5%BB%BAGLFW%E7%AA%97%E5%8F%A3.html)<br>
[Ch1-2 初始化流程](https://easyvulkan.github.io/Ch1-2%20%E5%88%9D%E5%A7%8B%E5%8C%96%E6%B5%81%E7%A8%8B.html)<br>
[Ch1-3 创建VK实例与逻辑设备](https://easyvulkan.github.io/Ch1-3%20%E5%88%9B%E5%BB%BAVK%E5%AE%9E%E4%BE%8B%E4%B8%8E%E9%80%BB%E8%BE%91%E8%AE%BE%E5%A4%87.html)<br>
[Ch1-4 创建交换链](https://easyvulkan.github.io/Ch1-4%20%E5%88%9B%E5%BB%BA%E4%BA%A4%E6%8D%A2%E9%93%BE.html)<br>
[Ch2-0 代码整理及一些辅助类](https://easyvulkan.github.io/Ch2-0%20%E4%BB%A3%E7%A0%81%E6%95%B4%E7%90%86%E5%8F%8A%E4%B8%80%E4%BA%9B%E8%BE%85%E5%8A%A9%E7%B1%BB.html)<br>
[Ch2-1 Rendering Loop](https://easyvulkan.github.io/Ch2-1%20Rendering%20Loop.html)<br>
[Ch2-2 创建渲染通道和帧缓冲](https://easyvulkan.github.io/Ch2-2%20%E5%88%9B%E5%BB%BA%E6%B8%B2%E6%9F%93%E9%80%9A%E9%81%93%E5%92%8C%E5%B8%A7%E7%BC%93%E5%86%B2.html)<br>
[Ch2-3 创建管线并绘制三角形](https://easyvulkan.github.io/Ch2-3%20%E5%88%9B%E5%BB%BA%E7%AE%A1%E7%BA%BF%E5%B9%B6%E7%BB%98%E5%88%B6%E4%B8%89%E8%A7%92%E5%BD%A2.html)

### 各种示例（建议依序阅读）

[Ch7-1 初识顶点缓冲区](https://easyvulkan.github.io/Ch7-1%20%E5%88%9D%E8%AF%86%E9%A1%B6%E7%82%B9%E7%BC%93%E5%86%B2%E5%8C%BA.html)<br>
[Ch7-2 初识索引缓冲区](https://easyvulkan.github.io/Ch7-2%20%E5%88%9D%E8%AF%86%E7%B4%A2%E5%BC%95%E7%BC%93%E5%86%B2%E5%8C%BA.html)<br>
[Ch7-3 初识实例化绘制](https://easyvulkan.github.io/Ch7-3%20%E5%88%9D%E8%AF%86%E5%AE%9E%E4%BE%8B%E5%8C%96%E7%BB%98%E5%88%B6.html)<br>
[Ch7-4 初识Push Constant](https://easyvulkan.github.io/Ch7-4%20%E5%88%9D%E8%AF%86Push%20Constant.html)<br>
[Ch7-5 初识Uniform缓冲区](https://easyvulkan.github.io/Ch7-5%20%E5%88%9D%E8%AF%86Uniform%E7%BC%93%E5%86%B2%E5%8C%BA.html)<br>
[Ch7-6 拷贝图像到屏幕](https://easyvulkan.github.io/Ch7-6%20%E6%8B%B7%E8%B4%9D%E5%9B%BE%E5%83%8F%E5%88%B0%E5%B1%8F%E5%B9%95.html)<br>
[Ch7-7 使用贴图](https://easyvulkan.github.io/Ch7-7%20%E4%BD%BF%E7%94%A8%E8%B4%B4%E5%9B%BE.html)<br>
[Ch8-1 离屏渲染](https://easyvulkan.github.io/Ch8-1%20%E7%A6%BB%E5%B1%8F%E6%B8%B2%E6%9F%93.html)<br>
[Ch8-2 深度测试和深度可视化](https://easyvulkan.github.io/Ch8-2%20%E6%B7%B1%E5%BA%A6%E6%B5%8B%E8%AF%95%E5%92%8C%E6%B7%B1%E5%BA%A6%E5%8F%AF%E8%A7%86%E5%8C%96.html)<br>
[Ch8-3 输入附件示例：延迟渲染](https://easyvulkan.github.io/Ch8-3%20%E5%BB%B6%E8%BF%9F%E6%B8%B2%E6%9F%93.html)<br>
[Ch8-4 预乘Alpha](https://easyvulkan.github.io/Ch8-4%20%E9%A2%84%E4%B9%98Alpha.html)<br>
[Ch8-5 sRGB色彩空间和开启HDR](https://easyvulkan.github.io/Ch8-5%20sRGB%E8%89%B2%E5%BD%A9%E7%A9%BA%E9%97%B4%E4%B8%8E%E5%BC%80%E5%90%AFHDR.html)

若要生成mipmap，请阅读Ch7-6和Ch7-7。

###  Vulkan1.0开始的核心功能（按需阅读）

[Ch3-1 同步原语](https://easyvulkan.github.io/Ch3-1%20%E5%90%8C%E6%AD%A5%E5%8E%9F%E8%AF%AD.html)<br>
[Ch3-2 图像与缓冲区](https://easyvulkan.github.io/Ch3-2%20%E5%9B%BE%E5%83%8F%E4%B8%8E%E7%BC%93%E5%86%B2%E5%8C%BA.html)<br>
[Ch3-3 管线布局和管线](https://easyvulkan.github.io/Ch3-3%20%E7%AE%A1%E7%BA%BF%E5%B8%83%E5%B1%80%E5%92%8C%E7%AE%A1%E7%BA%BF.html)<br>
[Ch3-4 渲染通道和帧缓冲](https://easyvulkan.github.io/Ch3-4%20%E6%B8%B2%E6%9F%93%E9%80%9A%E9%81%93%E5%92%8C%E5%B8%A7%E7%BC%93%E5%86%B2.html)<br>
[Ch3-5 命令缓冲区（有部分未写完）](https://easyvulkan.github.io/Ch3-5%20%E5%91%BD%E4%BB%A4%E7%BC%93%E5%86%B2%E5%8C%BA.html)<br>
[Ch3-6 描述符](https://easyvulkan.github.io/Ch3-6%20%E6%8F%8F%E8%BF%B0%E7%AC%A6.html)<br>
[Ch3-7 采样器](https://easyvulkan.github.io/Ch3-7%20%E9%87%87%E6%A0%B7%E5%99%A8.html)<br>
[Ch3-8 查询](https://easyvulkan.github.io/Ch3-8%20%E6%9F%A5%E8%AF%A2.html)<br>
[Ch4-1 着色器模组和GLSL基本语法（有部分未写完）](https://easyvulkan.github.io/Ch4-1%20%E7%9D%80%E8%89%B2%E5%99%A8%E6%A8%A1%E7%BB%84.html)<br>

### Vulkan1.1后的新增核心功能（按需阅读）

[Ch6-0 使用新版本功能](https://easyvulkan.github.io/Ch6-0%20%E4%BD%BF%E7%94%A8%E6%96%B0%E7%89%88%E6%9C%AC%E7%89%B9%E6%80%A7.html)<br>
[Ch6-1 无图像帧缓冲](https://easyvulkan.github.io/Ch6-1%20%E6%97%A0%E5%9B%BE%E5%83%8F%E5%B8%A7%E7%BC%93%E5%86%B2.html)<br>
[Ch6-2 动态渲染](https://easyvulkan.github.io/Ch6-2%20%E5%8A%A8%E6%80%81%E6%B8%B2%E6%9F%93.html)

### 附录

[Ap1-1 运行期编译GLSL](https://easyvulkan.github.io/Ap1-1%20%E8%BF%90%E8%A1%8C%E6%9C%9F%E7%BC%96%E8%AF%91GLSL.html)

## 更新计划（猴年马月？）

停...停更中。-_-||

待更新的内容清单：<br>
Ch4-4 几何着色器<br>
Ch4-5 细分着色器<br>
Ch4-6 计算着色器<br>
Ch5-4 立方体贴图（[封装代码见此](https://github.com/EasyVulkan/EasyVulkan.github.io/blob/main/solution/EasyVulkan_Ch7/VKBase+.h#L1234)，光看代码和我写的英语注释大概不太好懂...）<br>
Ch7-8 多重采样与超采样（[创建多重采样的渲染通道的代码](https://github.com/EasyVulkan/EasyVulkan.github.io/blob/main/solution/EasyVulkan_Ch7/EasyVulkan.hpp#L177)）<br>
Ch8-6 立方体贴图应用实例：天空盒（暂定，[以前随便写的示例代码见此](https://github.com/EasyVulkan/EasyVulkan.github.io/blob/main/solution/EasyVulkan_Ch8/Ch8-6.hpp)）<br>
Ch8-7 曲面细分与置换贴图<br>
Ch8-8 计算着色器和Storage缓冲区应用实例：2D粒子效果（暂定）<br>
Ch9-1 几何着色器应用实例：将ERP图像转到立方体贴图（暂定）<br>

如果我目前写的都看完了，那你也应该已经比较熟悉Vulkan API了，可以找别的教程接着看！

### 为什么停更了？

**Reason 1:** 越写越颓<br>
我当初开坑这套教程的时间是21年，其实是挺想写个类似LearnOpenGL一样的网站的，顺带再以自己写的内容为课件出一套视频教程，<br>
不过我属实是高估自己的耐心和实际写一整套网页的工程量了。<br>
因为我想实现令自己满意的网页格式，尤其是去除多余的文字空格、换行，以及精准的全代码着色，<br>
因此这套教程除了目录和标题外，都是直接写的HTML代码而非简单的RST语法。

**Reason 2:** 多年下来没把教程写完，结果如今网上Vulkan相关的学习资料已经相当多了（虽然有些机翻看着属实是不讲人话）<br>
基于这点，倒不是说不会接着写后续，只是我得好好思考一下这个系列今后的撰文方向，以及我EasyVulkan的这个账号还能够提供些什么。