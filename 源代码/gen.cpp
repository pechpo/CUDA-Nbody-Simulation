/**
 * 首先调用此程序生成数据 data.in
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
    const int turns = 100;
    int n = 1000;
    // init
    FILE *fp = fopen("data.in", "w");
    fprintf(fp, "%d\n", n);
    vector<Point> g(n);
    for (int i = 0; i < n; i++) {
        auto drand = [](int x) {
            return rand() & x;
        };
        g[i].mass = 1 + drand(15);
        int diff = 16384;
        if (i + i > n) diff = -diff;
        g[i].pos = Vec3(drand(32767), drand(32767) + diff, drand(32767));
        g[i].vel = Vec3(drand(15), drand(15), drand(15));
    }
    for (int i = 0; i < n; i++) {
        fprintf(fp, "%.0lf %.0lf %.0lf ", g[i].pos.x, g[i].pos.y, g[i].pos.z);
        fprintf(fp, "%.0lf %.0lf %.0lf ", g[i].vel.x, g[i].vel.y, g[i].vel.z);
        fprintf(fp, "%.0lf\n", g[i].mass);
    }
    fclose(fp);
    return 0;
}