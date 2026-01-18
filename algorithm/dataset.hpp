#ifndef DATASET_HPP
#define DATASET_HPP

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/**
 * @brief 空间对象 o = (ρ, φ)
 * 对应数据集中的原始点，包含坐标和关键字 ID
 */
struct SpatialObject {
    int id;      // 对象唯一标识
    double x;    // 换算后的平面坐标 x (单位: km)
    double y;    // 换算后的平面坐标 y (单位: km)
    int keyword; // 关键字 φ (类别 ID)

    SpatialObject() : id(0), x(0.0), y(0.0), keyword(0) {}
    
    SpatialObject(int _id, int _keyword, double lat, double lon)
        : id(_id), keyword(_keyword) {
        // 使用等距圆柱投影存储基础平面坐标 (单位: km)
        const double R = 6371.0; 
        x = R * lon * M_PI / 180.0;
        y = R * lat * M_PI / 180.0;
    }
};

// 空间范围和对象集合的封装
class Spatial {
public:
    std::vector<SpatialObject> objects;
    double x_min, x_max, y_min, y_max;

    Spatial() : x_min(0), x_max(0), y_min(0), y_max(0) {}

    /**
     * @brief 从 CSV 文件加载数据集
     * @param filePath CSV 文件的绝对路径
     * @param hasHeader 是否包含表头，默认为 true
     * @return 是否加载成功
     */
    bool load(const std::string& filePath, bool hasHeader = true) {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file " << filePath << std::endl;
            return false;
        }

        objects.clear();
        std::string line;

        if (hasHeader && std::getline(file, line)) {}

        while (std::getline(file, line)) {
            if (line.empty()) continue;
            std::stringstream ss(line);
            std::string vId, vCatId, latStr, lonStr;

            if (std::getline(ss, vId, ',') &&
                std::getline(ss, vCatId, ',') &&
                std::getline(ss, latStr, ',') &&
                std::getline(ss, lonStr, ',')) {
                try {
                    int id = std::stoi(vId);
                    double lat = std::stod(latStr);
                    double lon = std::stod(lonStr);
                    int kw = std::stoi(vCatId); 
                    objects.emplace_back(id, kw, lat, lon);
                } catch (...) {}
            }
        }
        file.close();

        if (!objects.empty()) {
            const double R = 6371.0; 
            double sumLat = 0;
            for (const auto& o : objects) sumLat += o.y / R; 
            double avgLat = sumLat / objects.size();
            double cosLat = std::cos(avgLat);
            
            for (auto& o : objects) o.x *= cosLat;

            x_min = x_max = objects[0].x;
            y_min = y_max = objects[0].y;
            for (const auto& o : objects) {
                x_min = std::min(x_min, o.x);
                x_max = std::max(x_max, o.x);
                y_min = std::min(y_min, o.y);
                y_max = std::max(y_max, o.y);
            }

            std::sort(objects.begin(), objects.end(), [](const SpatialObject& o1, const SpatialObject& o2) {
                return o1.x < o2.x;
            });
        }
        std::cout << "Successfully loaded " << objects.size() << " objects." << std::endl;
        return true;
    }

    /**
     * @brief 获取子集 (用于测试或特定区域挖掘)
     */
    Spatial getSubset(size_t limit) const {
        Spatial sub;
        if (objects.empty()) return sub;
        size_t n = std::min(limit, objects.size());
        sub.objects.assign(objects.begin(), objects.begin() + n);
        
        sub.x_min = sub.x_max = sub.objects[0].x;
        sub.y_min = sub.y_max = sub.objects[0].y;
        for (const auto& o : sub.objects) {
            sub.x_min = std::min(sub.x_min, o.x);
            sub.x_max = std::max(sub.x_max, o.x);
            sub.y_min = std::min(sub.y_min, o.y);
            sub.y_max = std::max(sub.y_max, o.y);
        }
        return sub;
    }
};

/**
 * @brief 计算两个空间对象之间的欧几里得距离
 * @return 距离 (单位: km)
 */
inline double getDistance(const SpatialObject& o1, const SpatialObject& o2) {
    double dx = o1.x - o2.x;
    double dy = o1.y - o2.y;
    return std::sqrt(dx * dx + dy * dy);
}

/**
 * @brief 基础欧几里得距离 (用于不具备原始纬度信息时的辅助计算)
 * @return 距离 (单位: km)
 */
inline double getDistance(double x1, double y1, double x2, double y2) {
    double dx = x1 - x2;
    double dy = y1 - y2;
    return std::sqrt(dx * dx + dy * dy);
}

#endif // DATASET_HPP
