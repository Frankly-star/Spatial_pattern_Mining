#ifndef FSPM_HPP
#define FSPM_HPP

#include <vector>
#include <unordered_map>
#include <set>
#include <iostream>
#include <algorithm>
#include "dataset.hpp"
#include "rectangular.hpp"

/**
 * @brief Frequent Spatial Pattern Mining (FSPM) 算法实现
 * 
 * @param D 空间数据集封装 (包含已排序对象和范围)
 * @param S 输入的矩形 Sketch (a x b, K)
 * @param epsilon 容差 (用于坐标匹配)
 * @param min_freq 最小支持度 (频数)
 * @param step 滑动窗口步长
 * @return std::vector<RectangularPattern> 频繁模式集合
 */
inline std::vector<RectangularPattern> fspm(
    const Spatial& D, 
    const RectangularSketch& S, 
    double epsilon, 
    int min_freq, 
    double step = 1.0
) {
    std::vector<RectangularPattern> R;
    std::vector<Instance> C;

    if (D.objects.empty()) return R;

    double a = S.size.a;
    double b = S.size.b;

    // 2. 滑动窗口搜索：在 D 中寻找满足 Sketch S 的所有实例 C
    // 对应算法中第一个 While 循环
    int total_x_steps = (D.x_max - a >= D.x_min) ? (int)((D.x_max - a - D.x_min) / step) + 1 : 0;
    int current_x_step = 0;

    for (double x = D.x_min; x <= D.x_max - a; x += step) {
        current_x_step++;
        if (current_x_step % 10 == 0 || current_x_step == total_x_steps) {
            float progress = (float)current_x_step / total_x_steps;
            int barWidth = 50;
            std::cout << "\r[FSPM] Candidate searching: [";
            int pos = barWidth * progress;
            for (int i = 0; i < barWidth; ++i) {
                if (i < pos) std::cout << "=";
                else if (i == pos) std::cout << ">";
                else std::cout << " ";
            }
            std::cout << "] " << int(progress * 100.0) << " %" << std::flush;
        }
        // 二分查找筛选出 x 范围内的点 (D.objects 已排序)
        auto it_start = std::lower_bound(D.objects.begin(), D.objects.end(), x, [](const SpatialObject& o, double val) {
            return o.x < val;
        });
        auto it_end = std::upper_bound(D.objects.begin(), D.objects.end(), x + a, [](double val, const SpatialObject& o) {
            return val < o.x;
        });

        for (double y = D.y_min; y <= D.y_max - b; y += step) {
            RectangularRegion rect(x, y, x + a, y + b);
            std::vector<SpatialObject> O_I;
            std::unordered_map<int, int> K_I;
            
            // 在 x 筛选后的基础上筛选 y
            for (auto it = it_start; it != it_end; ++it) {
                if (it->y >= y && it->y <= y + b) {
                    O_I.push_back(*it);
                    K_I[it->keyword]++;
                }
            }

            // 检查关键字分布 K_I 是否与 Sketch S.K 匹配
            bool sketchMatch = (K_I.size() == S.K.size());
            if (sketchMatch) {
                for (auto const& [kw, count] : S.K) {
                    auto it = K_I.find(kw);
                    if (it == K_I.end() || it->second != count) {
                        sketchMatch = false;
                        break;
                    }
                }
            }

            if (sketchMatch) {
                // 创建实例 I = (rect_I, O_I)
                // Instance 的中心点用于对齐
                Instance inst(a, b, rect.centerX(), rect.centerY());
                inst.O_P = O_I;
                C.push_back(inst);
            }
        }
    }

    std::cout << "\n[FSPM] Found " << C.size() << " candidate instances matching the sketch." << std::endl;

    // 3. 模式分组与支持度计算
    // 对应算法中第二个 While 循环
    std::vector<bool> processed(C.size(), false);
    for (size_t i = 0; i < C.size(); ++i) {
        if (i % 10 == 0 || i == C.size() - 1) {
            float progress = (float)(i + 1) / C.size();
            int barWidth = 50;
            std::cout << "\r[FSPM] Pattern grouping:    [";
            int pos = barWidth * progress;
            for (int k = 0; k < barWidth; ++k) {
                if (k < pos) std::cout << "=";
                else if (k == pos) std::cout << ">";
                else std::cout << " ";
            }
            std::cout << "] " << int(progress * 100.0) << " %" << std::flush;
        }

        if (processed[i]) continue;

        // I = pop(C)
        const Instance& I_ref = C[i];
        
        // P = (a x b, O_I)
        RectangularPattern P(a, b);
        P.O_P = I_ref.O_P;

        std::vector<size_t> matchIndices;
        matchIndices.push_back(i);

        // F(o) 用于存储每个模式对象对应的数据库中唯一对象 ID 集合
        std::vector<std::set<int>> F_set(P.O_P.size());
        
        // 引用实例自身的映射 (恒等映射)
        for (size_t j = 0; j < P.O_P.size(); ++j) {
            F_set[j].insert(P.O_P[j].id);
        }

        // 寻找所有匹配 P 的其他实例
        for (size_t k = i + 1; k < C.size(); ++k) {
            if (processed[k]) continue;

            std::vector<int> mapping;
            // 匹配逻辑：将 P 与 C[k] 进行对齐并检查坐标容差 epsilon
            // P 是抽象模式，以其几何中心 (a/2, b/2) 为基准
            // C[k] 是具体实例，以其中心 (C[k].x, C[k].y) 为基准
            if (P.getMatching(C[k], epsilon, a / 2.0, b / 2.0, C[k].x, C[k].y, mapping)) {
                matchIndices.push_back(k);
                // 记录映射到的数据库对象 u 的 ID
                for (size_t j = 0; j < P.O_P.size(); ++j) {
                    F_set[j].insert(C[k].O_P[mapping[j]].id);
                }
            }
        }

        // 检查支持度：对所有模式对象 o，其对应的数据库对象集合大小需 >= min_freq
        bool isFrequent = true;
        for (const auto& ids : F_set) {
            if ((int)ids.size() < min_freq) {
                isFrequent = false;
                break;
            }
        }

        if (isFrequent) {
            R.push_back(P);
            // C <- C \ I_P
            for (size_t idx : matchIndices) {
                processed[idx] = true;
            }
            std::cout << "[FSPM] Pattern found with " << matchIndices.size() << " instances." << std::endl;
        } else {
            // 不频繁则继续，只标记当前 i 处理过
            processed[i] = true;
        }
    }
    std::cout << std::endl;

    return R;
}

#endif // FSPM_HPP
