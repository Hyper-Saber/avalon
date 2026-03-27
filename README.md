# Avalon

<!--toc:start-->
- [Avalon](#avalon)
<!--toc:end-->

WIP
尝试写一个基于Vulkan的引擎

使用c++23 modules
世界数据更新->ecs
渲染->render graph
渲染数据更新->
  pushconstants: model, normalmatrix, materialindex, textureslots[7] (待修改,扩展textureslots,可能移除normalmatrix(根据pushconstants大小限制))
  set 0, binding 0: materialDatas[],一个巨大ssbo存放所有材质的color,float等.后续可能添加新binding用于不同类型的结构体,也可能使用类似pushconstants的slot形式
  set 0, binding 1: textures[]
  set 0, binding 2: samplers[]
  set 1, binding 0: sceneGlobals
现在每帧只需要绑定两次,set0 bindless, set1 sceneglobals;
