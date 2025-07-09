
RWTexture2D<float4> outputTexture;
[numthreads(16,16,1)]
void main(
    uint3 dispatchThreadID : SV_DispatchThreadID,
    uint3 groupThreadID : SV_GroupThreadID
    ){
  uint2 texelCoord = dispatchThreadID.xy;
  uint width, height;
  outputTexture.GetDimensions(width, height);
  uint2 size = uint2(width, height);

  if(texelCoord.x < size.x && texelCoord.y < size.y){
    float4 color = float4(0.0, 0.0, 0.0, 1.0);

    if(groupThreadID.x != 0 && groupThreadID.y != 0){
      color.x = float(texelCoord.x) / float(size.x);
      color.y = float(texelCoord.y) / float(size.y);
      color.z = 0.5;
    }

    outputTexture[texelCoord] = color;
  }
}



//
// //GLSL version to use #version 460
//
// //size of a workgroup for compute
// layout (local_size_x = 16, local_size_y = 16) in;
//
// //descriptor bindings for the pipeline
// layout(rgba16f,set = 0, binding = 0) uniform image2D image;
//
//
// void main() 
// {
//   ivec2 texelCoord = ivec2(gl_GlobalInvocationID.xy);
//   ivec2 size = imageSize(image);
//
//   if(texelCoord.x < size.x && texelCoord.y < size.y)
//   {
//     vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
//
//     if(gl_LocalInvocationID.x != 0 && gl_LocalInvocationID.y != 0)
//     {
//       color.x = float(texelCoord.x)/(size.x);
//       color.y = float(texelCoord.y)/(size.y);	
//     }
//
//     imageStore(image, texelCoord, color);
//   }
// }
