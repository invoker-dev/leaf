
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
    }

    outputTexture[texelCoord] = color;
  }
}

RWTexture2D<float4> outputTexture;
