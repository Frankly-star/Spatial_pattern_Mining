#include<bits/stdc++.h>
#include "dataset.hpp"
#include "rectangular.hpp"
using namespace std;

// 定义全局数据集变量
vector<SpatialObject> dataset;

int main() {
    string filePath = "../datasets/Gowalla.csv";
    if (loadDataset(filePath)) {
        // 数据已加载到全局变量 dataset 中
        std::cout << "First venue: " << dataset[0].id << std::endl;
        double d = getDistance(dataset[0].x, dataset[0].y, 
                               dataset[1].x, dataset[1].y);
        std::cout << "Distance: " << d << " kilometers" << std::endl;
    }
    return 0;
} 