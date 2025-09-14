struct MyOutputVertex
{
    float4 ndcPos : SV_Position;
};

/// <summary>
/// Constants that can change every frame.
/// </summary>
cbuffer PerFrameConstants : register(b0)
{
    float4x4 projectionMatrix;
    float3 boundingBoxColor;
}

/// <summary>
/// Constants that can change per Mesh/Draw call.
/// </summary>
cbuffer PerMeshConstants : register(b1)
{
    float4x4 modelViewMatrix;
}


/// <summary>
/// Constants that can change per Mesh/Draw call.
/// </summary>
cbuffer AABBConstants : register(b3)
{
    float4 firstPoint;
    float4 secondPoint;
    float4 thirdPoint;
    float4 fourthPoint;
    float4 fifthPoint;
    float4 sixthPoint;
    float4 seventhPoint;
    float4 eighthPoint;
}

static float4 obbVertices[] =
{
    firstPoint, secondPoint, thirdPoint, fourthPoint, fifthPoint, sixthPoint, seventhPoint, eighthPoint
};


static uint2 lineIndices[] =
{
    uint2(0, 1),
    uint2(1, 2),
    uint2(2, 3),
    uint2(0, 3),
    uint2(1, 4),
    uint2(4, 5),
    uint2(5, 2),
    uint2(0, 6),
    uint2(6, 7),
    uint2(7, 3),
    uint2(6, 4),
    uint2(5, 7)
};


// Vlaue was found empirically
static float collisionAvoidanceValue = 0.0035f;

[outputtopology("line")]
[numthreads(12, 1, 1)]
void MS_main(
    in int3 threadId : SV_GroupThreadID,
    out vertices MyOutputVertex verts[8],
    out indices uint2 lines[12])
{
    SetMeshOutputCounts(8, 12);

    if (threadId.x < 8)
    {
        MyOutputVertex vertex;
        vertex.ndcPos = mul(projectionMatrix, (mul(modelViewMatrix, obbVertices[threadId.x])));
        
        // Adding a small value to avoid overlap/collision with the underlying surfaces
        vertex.ndcPos.y = vertex.ndcPos.y + collisionAvoidanceValue;
        verts[threadId.x] = vertex;
    }
    
    lines[threadId.x] = lineIndices[threadId.x];
    

}

float4 PS_main(MyOutputVertex input) : SV_TARGET
{
    return float4(boundingBoxColor, 1.0f);
}
