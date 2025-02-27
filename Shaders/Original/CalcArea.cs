#version 460
  #define MAX_DIST            50
  #define MAX_ROTATIONS_LEVEL 6
  #define MAX_ROTATIONS       (1<<MAX_ROTATIONS_LEVEL)
  #define DIST_TOP            0
  #define DIST_RIGHT          (MAX_ROTATIONS>>2)
  #define DIST_BOTTOM         ((MAX_ROTATIONS*2)>>2)
  #define DIST_LEFT           ((MAX_ROTATIONS*3)>>2)
  layout(local_size_z=MAX_ROTATIONS) in;
  layout(binding=0)buffer SSBO0{ivec4 DistanceMap[MAX_ROTATIONS*2];uint HeightMap[];};
  layout(binding=1)buffer SSBO1{uint PlacementMap[];};
  layout(binding=2)buffer SSBO2{uint SheetWidth,HeightMapWidth,Rotations,reserved; uint SheetHeightMap[10000+MAX_DIST];uvec2 NFP[];};
  void main(){
    uint i,j,k;
    if(gl_LocalInvocationID.z>>Rotations==0){
      uint angle=gl_LocalInvocationID.z<<(MAX_ROTATIONS_LEVEL-Rotations);
      uint n=(gl_GlobalInvocationID.x<<Rotations)+gl_LocalInvocationID.z;

      if(PlacementMap[n]==0){
        uvec2 curPlace=uvec2(NFP[gl_GlobalInvocationID.x].x&0xFFFF,NFP[gl_GlobalInvocationID.x].x>>16);
        switch(NFP[gl_GlobalInvocationID.x].y){
          case 0:curPlace=uvec2(curPlace.x,curPlace.y-(DistanceMap[angle+DIST_LEFT].z+DistanceMap[angle+DIST_BOTTOM].y));break;
          case 1:curPlace=uvec2(curPlace.x+(DistanceMap[angle+DIST_BOTTOM].x-DistanceMap[angle+DIST_LEFT].y),curPlace.y);break;
          case 2:curPlace=curPlace.xy-uvec2(DistanceMap[angle+DIST_RIGHT].y+DistanceMap[angle+DIST_LEFT].y,DistanceMap[angle+DIST_BOTTOM].y-DistanceMap[angle+DIST_RIGHT].x);break;
                }  

        uint Area=0;
        for(i=0;i<curPlace.x;i++)
          Area+=SheetHeightMap[i];
        i=HeightMapWidth*angle;
        uint len=HeightMap[i]+curPlace.x;
        for(j=curPlace.x,k=i+1;j<len;j++,k++)
          Area+=max(SheetHeightMap[j],HeightMap[k]+curPlace.y);
        for(j=len;j<SheetWidth;j++)
          Area+=SheetHeightMap[j];
        PlacementMap[n]=Area;
      }         
    }
  }