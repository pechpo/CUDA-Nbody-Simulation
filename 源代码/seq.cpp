/**
 * N-Body Simulation 串行计算程序
 * 软化因子的平方取 1E-8，G 与 dt 的乘积取 1E5
 * 将计算得到的数据储存在 data.ini 中
*/
#include <cstdio>
#include <cmath>
#include <ctime>
#include <algorithm>
#include <vector>
#include <cassert>
#include <sys/time.h>
typedef double Real;
using std::pair;
using std::vector;
const Real G = 1E5;
const Real softeps2 = 1E-8;
const Real dt = 1;
struct Vec3 {
    Real x, y, z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(Real ix, Real iy, Real iz) : x(ix), y(iy), z(iz) {}
};
Vec3 operator - (const Vec3& u, const Vec3& v) {
    return Vec3(u.x - v.x, u.y - v.y, u.z - v.z);
}
Vec3 operator += (Vec3& u, const Vec3& v) {
    u.x += v.x;
    u.y += v.y;
    u.z += v.z;
    return u;
}
Vec3 operator * (const Vec3& u, Real k) {
    return Vec3(u.x * k, u.y * k, u.z * k);
}
Real mod2(Vec3 u) {
    return u.x * u.x + u.y * u.y + u.z * u.z;
}
struct Point {
    Real mass;
    Vec3 pos, vel;
};
Vec3 getAcceleration(Point u, Point v) {
    Vec3 r = v.pos - u.pos;
    return r * (v.mass * pow(mod2(r) + softeps2, -1.5)) * G;
}
int main() {
    // 计时器
    vector<timeval> timeStamp;
    auto initTimer = [&timeStamp]() { timeStamp.clear(); timeStamp.reserve(2); };
    auto recordTime = [&timeStamp](){
        struct timeval tim;
        gettimeofday(&tim, NULL);
        timeStamp.push_back(tim);
    };
    auto duration = [&timeStamp]() {
        assert(timeStamp.size() == 2);
        int res = timeStamp[1].tv_usec - timeStamp[0].tv_usec;
        res += (timeStamp[1].tv_sec - timeStamp[0].tv_sec) * 1000000;
        return res;
    };
    const int turns = 1000;
    int n;
    // init
    FILE *fp = fopen("data.in", "r");
    fscanf(fp, "%d", &n);
    vector<Point> g(n);
    for (int i = 0; i < n; i++) {
        fscanf(fp, "%lf%lf%lf", &g[i].pos.x, &g[i].pos.y, &g[i].pos.z);
        fscanf(fp, "%lf%lf%lf", &g[i].vel.x, &g[i].vel.y, &g[i].vel.z);
        fscanf(fp, "%lf", &g[i].mass);
    }
    fclose(fp);

    fp = fopen("data.out", "w");
    fprintf(fp, "%d %d\n", n, turns);

    /*for (int i = 0; i < n; i++) {
        g[i].mass = 10;
        auto drand = []() {
            return rand() & 32767;
        };
        g[i].pos = Vec3(drand(), drand(), drand());
        g[i].vel = Vec3();
    }*/

    int usec = 0;
    for (int T = 0; T < turns; T++) {
        for (int i = 0; i < n; i++) {
            initTimer();
            recordTime();
            Vec3 a;
            for (int j = 0; j < n; j++) {
                a += getAcceleration(g[i], g[j]);
            }
            g[i].pos += g[i].vel * dt;
            g[i].vel += a * dt;
            recordTime();
            usec += duration();
            fprintf(fp, "%lf %lf %lf\n", g[i].pos.x, g[i].pos.y, g[i].pos.z);
        }
    }
    fclose(fp);
    printf("time: %d usecs (with n = %d and turns = %d)\n", usec, n, turns);
    return 0;
}