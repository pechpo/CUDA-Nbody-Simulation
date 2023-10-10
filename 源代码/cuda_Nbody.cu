#include <ctime>
#include <cstdio>

const float softeningFactor = 1e-8;

// 考虑物体j吸引物体i，增加物体i的对应分加速度
__device__ void addAcceleration(float4 &pi, float4 &pj, float3 &ai)
{
    // 获得位矢
    float3 distVector = {0.0f, 0.0f, 0.0f};
    distVector.x = pj.x - pi.x;
    distVector.y = pj.y - pi.y;
    distVector.z = pj.z - pi.z;

    // 计算标量部分
    float distSquared = distVector.x * distVector.x + distVector.y * distVector.y + distVector.z * distVector.z;
    distSquared += softeningFactor;
    float invDist = rsqrtf(distSquared);
    float invDistCubic = invDist * invDist * invDist;
    float coff = pj.w * invDistCubic;

    // 累加加速度分矢量
    ai.x += distVector.x * coff;
    ai.y += distVector.y * coff;
    ai.z += distVector.z * coff;
}

// 计算某个物体的总加速度
__device__ float3 calcGrossAcc(float4 &bodyPos, float4 *positions, int numBatches)
{
    extern __shared__ float4 sharedPos[]; // 某个批次的物体的位置，存放在共享内存中

    float3 acc = {0.0f, 0.0f, 0.0f};

    for (unsigned int batch = 0; batch < numBatches; ++batch) // 利用block分批次并行处理所有物体，每次读入线程数个物体，正好每个线程搬一个物体数据
    {
        sharedPos[threadIdx.x] = positions[batch * blockDim.x + threadIdx.x]; // 并行读入当前批次的物体位置到共享内存中
        __syncthreads();                                                      // 并行同步

        // tile calculation.
#pragma unroll 128
        for (unsigned int index = 0; index < blockDim.x; ++index) // 对于当前批次的所有物体，考虑其对于目标物体的加速度
        {
            addAcceleration(bodyPos, sharedPos[index], acc);
        }
        __syncthreads();
    }

    return acc;
}

// 已知上一时间的所有物体位置，根据万有引力公式计算下一时间的某个物体位置
__global__ void calcGrossPos(
    float4 *newPos,
    float4 *oldPos,
    float3 *velocity,
    float deltaTime,
    float damping,
    unsigned int numBatches,
    unsigned int numBodies) // 由于线程自动补足，numBodies没用。而且计算时，空物体无质量，不会贡献加速度。
{
    int index = blockIdx.x * blockDim.x + threadIdx.x; // 线程负责的物体

    float4 pos = oldPos[index];

    float3 accel = calcGrossAcc(pos, oldPos, numBatches); // 计算目标物体的下一个位置，block内的线程会协作转移数据，从而采用更高效的共享内存计算

    // 更新物体速度
    float3 vel = velocity[index];
    vel.x += accel.x * deltaTime;
    vel.y += accel.y * deltaTime;
    vel.z += accel.z * deltaTime;
    vel.x *= damping;
    vel.y *= damping;
    vel.z *= damping;

    // 更新物体位置
    pos.x += vel.x * deltaTime;
    pos.y += vel.y * deltaTime;
    pos.z += vel.z * deltaTime;

    // 放到新的外部数组中
    newPos[index] = pos;
    velocity[index] = vel;

    // 由于速度是与计算独立的，而新位置放到了新数组内，因此不用考虑并行同步问题
}

const float damping = 1;
const int blockSize = 256;
const int T = 1000;        // 计算轮数
const float deltaTime = 1; // 一秒一个时间点，方便计算
const float G = 1e5;

int main()
{
    // 主机端读入质点数据
    int n;
    float4 *hPos;
    float3 *hVel;
    freopen("data.in", "r", stdin);
    scanf("%d", &n);

    // 计算并行参数
    int numBlocks = (n + blockSize - 1) / blockSize; // 一个线程负责一个物体的加速度计算，那么物体数除以block大小就是block个数
    int sharedMemSize = blockSize * sizeof(float4);

    //继续读入和分配空间
    int n_new = numBlocks * blockSize;  //扩充数组，避免溢出
    size_t bytes4 = n_new * sizeof(float4);
    size_t bytes3 = n_new * sizeof(float3);
    hPos = (float4 *)malloc(bytes4);
    hVel = (float3 *)malloc(bytes3);
    for (int i = 0; i < n; ++i)
    {
        scanf("%f%f%f", &hPos[i].x, &hPos[i].y, &hPos[i].z);
        scanf("%f%f%f", &hVel[i].x, &hVel[i].y, &hVel[i].z);
        scanf("%f", &hPos[i].w);
        hPos[i].w *= G;
    }
    fclose(stdin);

    // 计时
    clock_t start = clock();

    // 设备端申请内存并复制数据
    float4 *dPos, *nPos;
    float3 *dVel;
    cudaMalloc(&dPos, bytes4);
    cudaMalloc(&dVel, bytes3);
    cudaMemcpy(dPos, hPos, bytes4, cudaMemcpyHostToDevice);
    cudaMemcpy(dVel, hVel, bytes3, cudaMemcpyHostToDevice);
    cudaMalloc(&nPos, bytes4);

    // 创建历史记录数组
    float4 *history[T];

    // 开始每轮按顺序模拟
    for (int i = 0; i < T; ++i)
    {
        calcGrossPos<<<numBlocks, blockSize, sharedMemSize>>>(nPos, dPos, dVel, deltaTime, damping, numBlocks, n);
        cudaDeviceSynchronize(); // 注意，在所有物体都被计算完后，才开始更新和输出位置
        cudaMemcpy(dPos, nPos, bytes4, cudaMemcpyDeviceToDevice);
        history[i] = (float4 *)malloc(bytes4);
        cudaMemcpy(history[i], nPos, bytes4, cudaMemcpyDeviceToHost);
    }

    // 计时
    clock_t end = clock();
    float elapsedTime = 1.0 * (end - start) / CLOCKS_PER_SEC;
    puts("Calculation Completed.");
    printf("Elapsed Time = %fs\n", elapsedTime);
    puts("Result Output Start.");

    // 重定向输出
    freopen("data.out", "w", stdout);
    printf("%d %d\n", n, T);
    for (int i = 0; i < T; ++i)
    {
        for (int j = 0; j < n; ++j)
            printf("%f %f %f\n", history[i][j].x, history[i][j].y, history[i][j].z);
        puts("");
    }
    fclose(stdout);

    // 释放设备内存
    cudaFree(dPos);
    cudaFree(dVel);
    cudaFree(nPos);

    // 释放主机内存
    free(hPos);
    free(hVel);
    for (int i = 0; i < T; ++i)
        free(history[i]);

    puts("Process End.");

    return 0;
}