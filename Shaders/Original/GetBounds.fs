#version 460
   #define MAX_ROTATIONS_LEVEL 6
   #define MAX_ROTATIONS (1<<MAX_ROTATIONS_LEVEL)
   layout(binding=0) buffer SSBO0{ivec4 DistanceMap[MAX_ROTATIONS*2];int HeightMap[];};
   in vec2 TexCoord;
   out vec4 Color;
   layout(location=0)uniform int ScreenWidth;
   uniform sampler2D image;
   void main(){
     if(texture(image,TexCoord).r>0){
       atomicMax(HeightMap[gl_PrimitiveID*ScreenWidth+int(gl_FragCoord.x)],int(gl_FragCoord.y));
       atomicMax(DistanceMap[gl_PrimitiveID].x,(int(gl_FragCoord.y)<<16)+int(gl_FragCoord.x));
       atomicMin(DistanceMap[gl_PrimitiveID].z,(int(-gl_FragCoord.y)<<16)+int(gl_FragCoord.x));
       }
   }
