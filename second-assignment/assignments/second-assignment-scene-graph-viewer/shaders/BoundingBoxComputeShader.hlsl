struct AABB
{
    float4 lowerLeftBottom;
    float4 upperRightTop;
};

struct AAABBPoints
{
    float4 firstPoint;
    float4 secondPoint;   
    float4 thirdPoint;   
    float4 fourthPoint;   
    float4 fifthPoint;   
    float4 sixthPoint;   
    float4 seventhPoint;
    float4 eighthPoint;
};

StructuredBuffer<AABB> inputAABB : register(t0);
RWStructuredBuffer<AAABBPoints> computedAABBPoints : register(u0);

[numthreads(128, 1, 1)]
void main(
    int3 groupId: SV_GroupID,
    int3 threadId: SV_GroupThreadID)
{
    const uint NUMBER_OF_MESHES = 27;
    if(threadId.x < NUMBER_OF_MESHES)
    {
        float4 lowerLeftBottomOfAABB = inputAABB[threadId.x].lowerLeftBottom;
        float4 upperRightTopOfAABB = inputAABB[threadId.x].upperRightTop;
    
        computedAABBPoints[threadId.x].firstPoint = lowerLeftBottomOfAABB;
        computedAABBPoints[threadId.x].secondPoint = float4(lowerLeftBottomOfAABB.x, upperRightTopOfAABB.y, lowerLeftBottomOfAABB.z, lowerLeftBottomOfAABB.w);
        computedAABBPoints[threadId.x].thirdPoint = float4(upperRightTopOfAABB.x, upperRightTopOfAABB.y, lowerLeftBottomOfAABB.z, lowerLeftBottomOfAABB.w);
        computedAABBPoints[threadId.x].fourthPoint = float4(upperRightTopOfAABB.x, lowerLeftBottomOfAABB.y, lowerLeftBottomOfAABB.z, lowerLeftBottomOfAABB.w);
        computedAABBPoints[threadId.x].fifthPoint = float4(lowerLeftBottomOfAABB.x, upperRightTopOfAABB.y, upperRightTopOfAABB.z, lowerLeftBottomOfAABB.w);
        computedAABBPoints[threadId.x].sixthPoint = upperRightTopOfAABB;
        computedAABBPoints[threadId.x].seventhPoint = float4(lowerLeftBottomOfAABB.x, lowerLeftBottomOfAABB.y, upperRightTopOfAABB.z, lowerLeftBottomOfAABB.w);
        computedAABBPoints[threadId.x].eighthPoint = float4(upperRightTopOfAABB.x, lowerLeftBottomOfAABB.y, upperRightTopOfAABB.z, lowerLeftBottomOfAABB.w);
    }
}