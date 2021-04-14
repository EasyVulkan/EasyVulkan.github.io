# EasyVulkan.github.io
面向懒人的Vulkan中文教程<br>
https://easyvulkan.github.io/

21.04.15 更新：

完成 Ch2-4 即时帧和队列族所有权转移
具体更新内容见教程主页。

关于代码文件
* VKBase.h中修改了一处注释
* VKBase+.h中，为texture::CopyBufferToImage2d中的两个dstStage为VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT的vkCmdPipelineBarrier添加了VK_DEPENDENCY_BY_REGION_BIT，几乎没卵用，因为读图一般不是渲染过程中实时发生的操作，写上去聊胜于无，为说明你想用这个bit的话能用。

未来更新预定（要是有反响。我觉得做教程最怕的就是，自己花了时间，结果却没人在乎）：
* 顶点和索引
* 实例化
* push constant
* uniform buffer
* dynamic uniform buffer
* 2D贴图以及生成mipmap
* 2D贴图数组以及生成mipmap
* 立方体贴图以及生成mipmap
* 多重采样
* 深度附件
* 离屏渲染
* 输入附件（附带延迟渲染）

代码中目前已写的示例:
* 第一个三角形（FirstTriangle.hpp）
* 即时帧（FramesInFlight.hpp）
* 队列族所有权转移（QueueFamilyOwnershipTransfer.hpp）
* 立方体贴图（SkyboxAndCube.hpp）
* 输入附件和延迟渲染（DeferredToScreen.hpp）
