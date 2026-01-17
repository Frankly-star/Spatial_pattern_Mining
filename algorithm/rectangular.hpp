#ifndef RECTANGULAR_HPP
#define RECTANGULAR_HPP

#include <unordered_map>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include "dataset.hpp"

/**
 * @brief 矩形区域的大小 (a x b)
 */
struct Rectangular {
    double a; // 宽
    double b; // 高

    Rectangular(double width = 0.0, double height = 0.0) : a(width), b(height) {}
};

/**
 * @brief Rectangular Sketch S = (a x b, K)
 * K 是一个字典，存储区域内关键字 ID 及其出现次数
 */
struct RectangularSketch {
    Rectangular size; // 矩形大小
    std::unordered_map<int, int> K; // 关键字分布

    RectangularSketch() = default;
    
    RectangularSketch(double width, double height) 
        : size(width, height) {}

    RectangularSketch(const Rectangular& rect, const std::unordered_map<int, int>& dictionary)
        : size(rect), K(dictionary) {}

    /**
     * @brief 记录一个关键字的出现
     */
    void addKeyword(int keyword) {
        K[keyword]++;
    }

    /**
     * @brief 判断 Sketch 是否为空
     */
    bool isEmpty() const {
        return K.empty();
    }
};

/**
 * @brief Rectangular Pattern P = (a x b, O_P)
 */
struct RectangularPattern {
    Rectangular size; // 矩形尺寸
    std::vector<SpatialObject> O_P; // 区域内空间对象集合

    RectangularPattern(double width = 0.0, double height = 0.0) 
        : size(width, height) {}

    /**
     * @brief 向 Pattern 中添加空间对象
     */
    void addObject(const SpatialObject& obj) {
        O_P.push_back(obj);
    }

    /**
     * @brief 判断两个抽象 Pattern 是否匹配 (默认已对齐)
     */
    bool match(const RectangularPattern& other, double epsilon) const {
        if (std::abs(size.a - other.size.a) == 0 || std::abs(size.b - other.size.b) == 0) {
            return false;
        }
        if (O_P.size() != other.O_P.size()) return false;
        if (O_P.empty()) return true;

        std::unordered_map<int, std::vector<int>> K1, K2;
        for (int i = 0; i < (int)O_P.size(); ++i) K1[O_P[i].keyword].push_back(i);
        for (int i = 0; i < (int)other.O_P.size(); ++i) K2[other.O_P[i].keyword].push_back(i);

        if (K1.size() != K2.size()) return false;
        for (auto const& [kw, ids1] : K1) {
            auto it2 = K2.find(kw);
            if (it2 == K2.end() || it2->second.size() != ids1.size()) return false;
            if (!canMatchGroup(ids1, it2->second, other, epsilon, 0, 0, 0, 0)) return false;
        }
        return true;
    }

protected:
    /**
     * @brief 核心匹配逻辑：验证存在双射 f
     */
    bool canMatchGroup(const std::vector<int>& ids1, const std::vector<int>& ids2, 
                       const RectangularPattern& other, double eps,
                       double x1, double y1, double x2, double y2) const {
        int n = ids1.size();
        std::vector<std::vector<int>> adj(n);
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                // 计算对齐后的相对位移
                double dx = std::abs((O_P[ids1[i]].x - (x1 + size.a/2)) - (other.O_P[ids2[j]].x - (x2 + other.size.a/2)));
                double dy = std::abs((O_P[ids1[i]].y - (y1 + size.b/2)) - (other.O_P[ids2[j]].y - (y2 + other.size.b/2)));
                if (dx <= eps && dy <= eps) adj[i].push_back(j);
            }
        }
        std::vector<int> matchL(n, -1);
        int count = 0;
        for (int i = 0; i < n; ++i) {
            std::vector<bool> vis(n, false);
            if (dfs(i, adj, vis, matchL)) count++;
        }
        return count == n;
    }

    bool dfs(int u, const std::vector<std::vector<int>>& adj, std::vector<bool>& vis, std::vector<int>& matchL) const {
        for (int v : adj[u]) {
            if (!vis[v]) {
                vis[v] = true;
                if (matchL[v] < 0 || dfs(matchL[v], adj, vis, matchL)) { matchL[v] = u; return true; }
            }
        }
        return false;
    }
};

/**
 * @brief Instance 是 Pattern 在数据库中的具体体现
 */
struct Instance : public RectangularPattern {
    double x, y; // 实例在数据库中的起始坐标 (左下角)

    Instance(double width = 0.0, double height = 0.0, double _x = 0.0, double _y = 0.0) 
        : RectangularPattern(width, height), x(_x), y(_y) {}

    /**
     * @brief 判断该实例是否为某 Pattern 的 Instance
     */
    bool isInstanceOf(const RectangularPattern& pattern, double epsilon) const {
        if (std::abs(size.a - pattern.size.a) == 0 || std::abs(size.b - pattern.size.b) == 0) return false;
        if (O_P.size() != pattern.O_P.size()) return false;

        std::unordered_map<int, std::vector<int>> Ki, Kp;
        for (int i = 0; i < (int)O_P.size(); ++i) Ki[O_P[i].keyword].push_back(i);
        for (int i = 0; i < (int)pattern.O_P.size(); ++i) Kp[pattern.O_P[i].keyword].push_back(i);

        if (Ki.size() != Kp.size()) return false;
        for (auto const& [kw, idsi] : Ki) {
            auto itp = Kp.find(kw);
            if (itp == Kp.end() || itp->second.size() != idsi.size()) return false;
            
            // 实例匹配：使用自身的 (x, y) 和 Pattern 的默认中心 (0.5a, 0.5b) 进行 Translation 
            if (!canMatchGroup(idsi, itp->second, pattern, epsilon, x, y, 0, 0)) return false;
        }
        return true;
    }
};

#endif // RECTANGULAR_HPP
