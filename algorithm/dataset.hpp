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

// 全局变量用于存储数据集
extern std::vector<SpatialObject> dataset;

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

/**
 * @brief 从 CSV 文件加载数据集
 * @param filePath CSV 文件的绝对路径
 * @param hasHeader 是否包含表头，默认为 true
 * @return 是否加载成功
 */
inline bool loadDataset(const std::string& filePath, bool hasHeader = true) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filePath << std::endl;
        return false;
    }

    dataset.clear();
    std::string line;

    // 跳过表头
    if (hasHeader && std::getline(file, line)) {
        // 第一行已读取
    }

    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        std::stringstream ss(line);
        std::string vId, vCatId, latStr, lonStr;

        // 假设格式为: venueId,venueCategoryId,latitude,longitude
        if (std::getline(ss, vId, ',') &&
            std::getline(ss, vCatId, ',') &&
            std::getline(ss, latStr, ',') &&
            std::getline(ss, lonStr, ',')) {
            
            try {
                int id = std::stoi(vId);
                double lat = std::stod(latStr);
                double lon = std::stod(lonStr);
                int kw = std::stoi(vCatId); 
                dataset.emplace_back(id, kw, lat, lon);
            } catch (const std::exception& e) {
                std::cerr << "Error parsing line: " << line << " - " << e.what() << std::endl;
            }
        }
    }

    file.close();

    // 关键修正：对所有点的 x 坐标进行一次性缩放修正
    if (!dataset.empty()) {
        const double R = 6371.0; 
        double sumLat = 0;
        for (const auto& o : dataset) sumLat += o.y / R; 
        double avgLat = sumLat / dataset.size();
        double cosLat = std::cos(avgLat);
        
        for (auto& o : dataset) {
            o.x *= cosLat;
        }
        std::cout << "Projection adjusted using cos(avgLat) = " << cosLat << std::endl;
    }

    std::cout << "Successfully loaded " << dataset.size() << " objects." << std::endl;
    return true;
}

#endif // DATASET_HPP
