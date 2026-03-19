struct Vertex {
  float3 position;
  float3 color;
};

static const Vertex kTriangleVertices[3] = {
    // 顶点 0: 顶部 (红色)
    {float3(0.0f, -0.5f, 0.0f), float3(1.0f, 0.0f, 0.0f)},
    // 顶点 1: 右下 (绿色)
    {float3(0.5f, 0.5f, 0.0f), float3(0.0f, 1.0f, 0.0f)},
    // 顶点 2: 左下 (蓝色)
    {float3(-0.5f, 0.5f, 0.0f), float3(0.0f, 0.0f, 1.0f)}};

// Shader 输出结构体
struct VSOutput {
  float4 pos : SV_POSITION; // 必选：系统占位符
  float3 color : COLOR;     // 可选：用于插值传递给像素着色器
};

// ==============================================================================
// Vertex Shader
// 注意：没有定义入参（VSInput），完全依赖 SV_VertexID
// ==============================================================================
VSOutput VsMain(uint vertexID : SV_VertexID) {
  VSOutput output;

  // 1. 安全检查（虽然 vkCmdDraw(3) 保证了不会越界，但这是好的工业习惯）
  uint id = vertexID % 3;

  // 2. 根据 ID 查找静态数据
  Vertex v = kTriangleVertices[id];

  // 3. 填充输出
  output.pos = float4(v.position, 1.0f); // 转换为齐次坐标
  output.color = v.color;

  return output;
}

// ==============================================================================
// Pixel (Fragment) Shader
// ==============================================================================
float4 FsMain(VSOutput input) : SV_TARGET {
  // 此时 input.color 已经是经过 GPU 硬件插值后的颜色
  return float4(input.color, 1.0f); // 加上 Alpha 通道
}
