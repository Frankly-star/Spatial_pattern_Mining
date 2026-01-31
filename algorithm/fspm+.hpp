#ifndef FSPM_PLUS_HPP
#define FSPM_PLUS_HPP

#include <vector>
#include <string>
#include <algorithm>
#include <map>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include "dataset.hpp"
#include "rectangular.hpp"
#include "fspm.hpp"

namespace fspm_plus {

    // Event for Sweep Line
    struct SweepEvent {
        std::string type; // "top" or "bottom"
        double y;
        double x_min, x_max;
        const SpatialObject* obj;

        // Sort: Descending Y. If equal, check X. If equal, 'bottom' (add) before 'top' (remove).
        bool operator<(const SweepEvent& other) const {
            if (std::abs(y - other.y) > 0) {
                return y > other.y;
            }
            // Secondary sort key: x_min (ascending)
            if (std::abs(x_min - other.x_min) > 0) {
                return x_min < other.x_min;
            }
            if (type != other.type) {
                // "bottom" should come before "top"
                return type == "bottom";
            }
            return false;
        }
    };

    // Horizontal segment (Window)
    struct SweepWindow {
        double x_start, x_end;
        std::unordered_map<int, int> current_keywords;
    };

    // Equality check for keywords map
    inline bool keywords_equal(const std::unordered_map<int, int>& a, const std::unordered_map<int, int>& b) {
        return a == b;
    }

    // Check if current keywords satisfy the target requirement (subset check)
    inline bool has_subset_keywords(const std::unordered_map<int, int>& target, const std::unordered_map<int, int>& current) {
        for (const auto& pair : target) {
            auto it = current.find(pair.first);
            if (it == current.end() || it->second < pair.second) {
                return false;
            }
        }
        return true;
    }

    // Function to check if a rectangle intersects any region in the set V
    inline bool is_rectangle_intersecting_regions(const RectangularRegion& r, const std::vector<RectangularRegion>& regions) {
        for (const auto& v_rect : regions) {
            // Check intersection
            if (std::max(r.x_min, v_rect.x_min) < std::min(r.x_max, v_rect.x_max) &&
                std::max(r.y_min, v_rect.y_min) < std::min(r.y_max, v_rect.y_max)) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Optimization spatial pruning via Sweep-Line algorithm
     * 
     * @param D The spatial database
     * @param S The target sketch
     * @return std::vector<RectangularRegion> The valid regions (loci of valid window top-lefts)
     */
    inline std::vector<RectangularRegion> spatial_pruning(const Spatial& D, const RectangularSketch& S) {
        double a = S.size.a;
        double b = S.size.b;
        std::vector<SweepEvent> E;
        E.reserve(D.objects.size() * 2);

        for (const auto& obj : D.objects) {
            // OPTIMIZATION: Only consider objects with keywords in Sketch
            if (S.K.find(obj.keyword) == S.K.end()) {
                continue;
            }

            // R_o = [x, x+a] x [y, y+b]
            // "top" event at y (low Y, remove) -> actually top edge of R_o which is y
            // "bottom" event at y+b (high Y, add) -> bottom edge of R_o which is y+b
            // Descending sweep.
            SweepEvent top = { "top", obj.y, obj.x, obj.x + a, &obj }; 
            SweepEvent bottom = { "bottom", obj.y + b, obj.x, obj.x + a, &obj };
            E.push_back(bottom);
            E.push_back(top);
        }

        // Sort descending Y
        std::sort(E.begin(), E.end());

        // Initialize Windows covering the relevant X range
        std::vector<SweepWindow> W;
        double min_val = D.objects.empty() ? 0 : D.x_min;
        double max_val = D.objects.empty() ? 0 : D.x_max + a;
        W.push_back({ min_val, max_val, {} });

        std::vector<RectangularRegion> V; // Valid regions
        double y_pre = E.empty() ? 0 : E[0].y; 

        int count = 0;
        int total = E.size();

        for (const auto& e : E) {
            count++;
            if (count % 100 == 0) {
                 std::cout << "\r[FSPM+] Sweep-Line: " << count << "/" << total << " (W size: " << W.size() << ")    " << std::flush;
            }

            // 1. Check vertical gap
            if (y_pre > e.y) {
                for (const auto& w : W) {
                    if (has_subset_keywords(S.K, w.current_keywords)) {
                        V.emplace_back(w.x_start, e.y, w.x_end, y_pre);
                    }
                }
            }

            // 2. Update windows
            std::vector<SweepWindow> next_W;
            next_W.reserve(W.size() * 2);

            for (const auto& w : W) {
                double overlap_start = std::max(w.x_start, e.x_min);
                double overlap_end = std::min(w.x_end, e.x_max);

                if (overlap_start < overlap_end) {
                    if (w.x_start < overlap_start) {
                        next_W.push_back({ w.x_start, overlap_start, w.current_keywords });
                    }

                    SweepWindow overlap_win = { overlap_start, overlap_end, w.current_keywords };
                    if (e.type == "bottom") {
                         overlap_win.current_keywords[e.obj->keyword]++;
                    } else {
                        if (overlap_win.current_keywords.count(e.obj->keyword)) {
                            overlap_win.current_keywords[e.obj->keyword]--;
                             if (overlap_win.current_keywords[e.obj->keyword] <= 0) {
                                 overlap_win.current_keywords.erase(e.obj->keyword);
                             }
                        }
                    }
                    next_W.push_back(overlap_win);

                    if (overlap_end < w.x_end) {
                        next_W.push_back({ overlap_end, w.x_end, w.current_keywords });
                    }
                } else {
                    next_W.push_back(w);
                }
            }
            
            // 3. Merge
            W.clear();
            if (!next_W.empty()) {
                W.push_back(next_W[0]);
                for (size_t i = 1; i < next_W.size(); ++i) {
                    if (keywords_equal(next_W[i].current_keywords, W.back().current_keywords)) {
                        W.back().x_end = next_W[i].x_end;
                    } else {
                        W.push_back(next_W[i]);
                    }
                }
            }

            y_pre = e.y;
        }

        return V;
    }

    /**
     * @brief Common Candidate Generation Logic
     */
    inline std::vector<Instance> generate_candidates(const Spatial& D, const RectangularSketch& S) {
        // 1. Get Valid Regions (Candidate Loci)
        std::vector<RectangularRegion> V_raw = spatial_pruning(D, S);
        
        // 2. Merge Vertically Adjacent Regions
        // To avoid generating thousands of duplicate instances from sliced horizontal strips,
        // we merge strips that align vertically (same x_min, x_max, and adjacent y).
        if (!V_raw.empty()) {
            std::sort(V_raw.begin(), V_raw.end(), [](const RectangularRegion& a, const RectangularRegion& b) {
                if (std::abs(a.x_min - b.x_min) > 1e-9) return a.x_min < b.x_min;
                if (std::abs(a.x_max - b.x_max) > 1e-9) return a.x_max < b.x_max;
                return a.y_max > b.y_max; // Descending Y for easier merging bottom-up (or top-down)
            });
        }

        std::vector<RectangularRegion> V;
        if (!V_raw.empty()) {
            V.push_back(V_raw[0]);
            for (size_t i = 1; i < V_raw.size(); ++i) {
                RectangularRegion& last = V.back();
                const RectangularRegion& curr = V_raw[i];

                // Check alignment
                bool alignedX = std::abs(last.x_min - curr.x_min) < 1e-9 && std::abs(last.x_max - curr.x_max) < 1e-9;
                // Check abutment (last.y_min should be approx curr.y_max since we sorted descending Y)
                bool abutmentY = std::abs(last.y_min - curr.y_max) < 1e-9;

                if (alignedX && abutmentY) {
                    // Merge
                    last.y_min = curr.y_min;
                } else {
                    V.push_back(curr);
                }
            }
        }
        
        std::vector<Instance> C;

        double a = S.size.a;
        double b = S.size.b;

        // 3. Extract Instances from Regions
        // We assume D.objects is sorted by X (as per dataset convention/fspm requirement)
        
        for (const auto& r : V) {
            // One valid region -> One candidate instance
            // We align the window to the bottom-left of the valid region key area.
            double x = r.x_min;
            double y = r.y_min; // For extraction, we just pick one point (e.g. min corner) from the locus
            
            // Window coverage: [x, x+a] x [y, y+b]
            
            // Find objects in this window
            auto it_start = std::lower_bound(D.objects.begin(), D.objects.end(), x, [](const SpatialObject& o, double val) {
                return o.x < val;
            });
            auto it_end = std::upper_bound(D.objects.begin(), D.objects.end(), x + a, [](double val, const SpatialObject& o) {
                return val < o.x;
            });

            std::vector<SpatialObject> O_I;
            std::unordered_map<int, int> K_I;

            for (auto it = it_start; it != it_end; ++it) {
                if (it->y >= y && it->y <= y + b) {
                    O_I.push_back(*it);
                    K_I[it->keyword]++;
                }
            }

            // Check if keywords sufficient
            bool sketchMatch = true;
            for (auto const& [kw, count] : S.K) {
                if (K_I[kw] < count) {
                    sketchMatch = false; 
                    break;
                }
            }

            if (sketchMatch) {
                // Greedy extraction of instances from this window
                std::vector<bool> used(O_I.size(), false);
                while (true) {
                    Instance inst(a, b, x + a/2.0, y + b/2.0); 
                    std::vector<size_t> currentAttemptIndices;
                    bool success = true;

                    for (auto const& [kw, count] : S.K) {
                        int foundInRange = 0;
                        for (size_t i = 0; i < O_I.size(); ++i) {
                            if (!used[i] && O_I[i].keyword == kw) {
                                // Check if already selected in current attempt (redundant with used check but safe)
                                bool alreadyIn = false;
                                for(size_t idx : currentAttemptIndices) if(idx==i) alreadyIn=true;
                                if(alreadyIn) continue;

                                currentAttemptIndices.push_back(i);
                                if (++foundInRange == count) break;
                            }
                        }
                        if (foundInRange < count) {
                            success = false;
                            break;
                        }
                    }

                    if (success) {
                        // Mark used
                        for (size_t idx : currentAttemptIndices) {
                            used[idx] = true;
                            SpatialObject subObj = O_I[idx];
                            subObj.x -= x; // coordinates relative to window bounds
                            subObj.y -= y;
                            inst.O_P.push_back(subObj);
                        }

                        C.push_back(inst);
                    } else {
                        break; // Cannot extract more instances from this window
                    }
                }
            }
        }

        // Deduplicate candidates (Robustness against overlapping regions)
        if (!C.empty()) {
            std::sort(C.begin(), C.end(), [](const Instance& a, const Instance& b) {
                if (a.O_P.size() != b.O_P.size()) return a.O_P.size() < b.O_P.size();
                for (size_t i = 0; i < a.O_P.size(); ++i) {
                    if (a.O_P[i].id != b.O_P[i].id) return a.O_P[i].id < b.O_P[i].id;
                }
                return false; 
            });
            C.erase(std::unique(C.begin(), C.end(), [](const Instance& a, const Instance& b) {
                if (a.O_P.size() != b.O_P.size()) return false;
                for (size_t i = 0; i < a.O_P.size(); ++i) {
                    if (a.O_P[i].id != b.O_P[i].id) return false;
                }
                return true;
            }), C.end());
        }

        return C;
    }

    /**
     * @brief Execute FSPM+ Algorithm
     * Directly extracts patterns from valid regions found by Sweep-Line.
     */
    inline std::vector<RectangularPattern> fspm_plus(const Spatial& D, const RectangularSketch& S, double epsilon, int min_freq) {
        std::vector<Instance> C = generate_candidates(D, S);

        std::cout << "\n[FSPM+] Found " << C.size() << " candidate instances matching the sketch." << std::endl;

        // 3. Pattern Grouping and Frequency Counting (Logic from FSPM)
        std::vector<RectangularPattern> R;
        std::vector<bool> processed(C.size(), false);
        
        double a = S.size.a;
        double b = S.size.b;
        
        for (size_t i = 0; i < C.size(); ++i) {
            if (processed[i]) continue;

            const Instance& I_ref = C[i];
            RectangularPattern P(a, b);
            P.O_P = I_ref.O_P; // Use reference instance configuration

            std::vector<std::set<int>> F_set(P.O_P.size());
            for (size_t j = 0; j < P.O_P.size(); ++j) {
                F_set[j].insert(P.O_P[j].id);
            }

            // Find matches
            for (size_t k = i + 1; k < C.size(); ++k) {
                if (processed[k]) continue;

                std::vector<int> mapping;
                // Match with tolerance
                if (P.getMatching(C[k], epsilon, a / 2.0, b / 2.0, a / 2.0, b / 2.0, mapping)) {
                    processed[k] = true; // Mark as grouped
                    // Collect IDs for frequency count
                    for (size_t j = 0; j < P.O_P.size(); ++j) {
                        F_set[j].insert(C[k].O_P[mapping[j]].id);
                    }
                }
            }

            // Check min_freq
            bool isFrequent = true;
            for (const auto& ids : F_set) {
                if ((int)ids.size() < min_freq) {
                    isFrequent = false;
                    break;
                }
            }

            if (isFrequent) {
                R.push_back(P);
            }
        }

        return R;
    }

    // --- Tier 2: VP-Tree Structures ---
    
    using RDV = std::vector<double>;

    struct VPTreeNode {
        RDV center;
        int pattern_index; // Index in the result vector R
        double mu; // Partition radius
        VPTreeNode *left = nullptr;
        VPTreeNode *right = nullptr;

        VPTreeNode(const RDV& c, int idx) : center(c), pattern_index(idx), mu(0.0) {}
        ~VPTreeNode() { delete left; delete right; }
    };

    // Calculate Chebyshev distance (L_inf)
    inline double chebyshev_dist(const RDV& a, const RDV& b) {
        double max_d = 0.0;
        for (size_t i = 0; i < a.size(); ++i) {
            double d = std::abs(a[i] - b[i]);
            if (d > max_d) max_d = d;
        }
        return max_d;
    }

    // Search for any node within 2*epsilon distance
    inline int vpt_search(VPTreeNode* node, const RDV& query, double threshold) {
        if (!node) return -1;

        double d = chebyshev_dist(node->center, query);
        if (d <= threshold) {
            // Found a matching cluster representative
            return node->pattern_index;
        }

        // Branch and bound
        int res = -1;

        // Try left child if potential overlap
        if (d - threshold < node->mu) {
            res = vpt_search(node->left, query, threshold);
            if (res != -1) return res;
        }

        // Try right child if potential overlap
        if (d + threshold >= node->mu) {
            res = vpt_search(node->right, query, threshold);
            if (res != -1) return res;
        }

        return -1;
    }

    // Insert a new cluster representative
    inline void vpt_insert(VPTreeNode*& node, const RDV& p, int idx) {
        if (!node) {
            node = new VPTreeNode(p, idx);
            return;
        }

        // If leaf (no children yet), splitting strategy:
        // Identify distance to new point as threshold mu for future splits
        if (!node->left && !node->right) {
            node->mu = chebyshev_dist(node->center, p);
            node->left = new VPTreeNode(p, idx);
        } else {
            double d = chebyshev_dist(node->center, p);
            if (d < node->mu) {
                vpt_insert(node->left, p, idx);
            } else {
                vpt_insert(node->right, p, idx);
            }
        }
    }


    /**
     * @brief Tree-optimized FSPM+ Algorithm
     * Replaces linear grouping with Keyword-Grouped VP-Tree Indexing.
     */
    inline std::vector<RectangularPattern> tree_optimized_fspm(const Spatial& D, const RectangularSketch& S, double epsilon, int min_freq) {
        // 1. Get Candidate Instances
        std::vector<Instance> C = generate_candidates(D, S);
        std::cout << "\n[Tree Opt] Found " << C.size() << " candidate instances." << std::endl;

        // --- TREE GROUPING LOGIC ---
        
        // Tier 1: Keyword Map -> Tier 2: VP-Tree Root
        std::map<std::string, VPTreeNode*> tier1_index;
        
        // Results
        std::vector<RectangularPattern> R; // Stores representative patterns
        // We also need to store the frequency sets for each representative
        std::vector<std::vector<std::set<int>>> R_freq_sets;

        double a = S.size.a;
        double b = S.size.b;

        for (const auto& inst : C) {
            // 1. Canonical Sort (on copy, to generate key)
            Instance P = inst; 
            std::sort(P.O_P.begin(), P.O_P.end(), [](const SpatialObject& a, const SpatialObject& b) {
                if (a.keyword != b.keyword) return a.keyword < b.keyword;
                if (std::abs(a.y - b.y) > 1e-9) return a.y < b.y;
                if (std::abs(a.x - b.x) > 1e-9) return a.x < b.x;
                return a.id < b.id;
            });

            // 2. Extract Type Signature (Tier 1 Key)
            std::string type_key = "";
            for (const auto& o : P.O_P) {
                type_key += std::to_string(o.keyword) + ",";
            }

            // 3. Extract RDV (Relative Displacement Vector) for Tier 2
            double y0 = P.O_P[0].y; // Origin is Y of first object in canonical order
            RDV rdv;
            rdv.reserve(P.O_P.size());
            for (const auto& o : P.O_P) {
                rdv.push_back(o.y - y0);
            }

            // 4. Search in Tier 1
            int match_idx = -1;
            if (tier1_index.find(type_key) != tier1_index.end()) {
                // Search in VP-Tree (Tier 2)
                match_idx = vpt_search(tier1_index[type_key], rdv, 2.0 * epsilon);
            }

            // 5. Handle Match or Insert
            if (match_idx != -1) {
                auto& F_set = R_freq_sets[match_idx];
                for (size_t i = 0; i < P.O_P.size(); ++i) {
                    F_set[i].insert(P.O_P[i].id);
                }
            } else {
                // New Group
                int new_idx = R.size();
                RectangularPattern new_pat(a, b);
                new_pat.O_P = P.O_P; // Use the canonical sorted form as representative
                R.push_back(new_pat);

                // Init frequency set
                std::vector<std::set<int>> f_set(P.O_P.size());
                for (size_t i = 0; i < P.O_P.size(); ++i) {
                    f_set[i].insert(P.O_P[i].id);
                }
                R_freq_sets.push_back(f_set);

                // Insert into Index
                vpt_insert(tier1_index[type_key], rdv, new_idx);
            }
        }

        // Cleanup Trees
        for(auto& pair : tier1_index) {
            delete pair.second;
        }

        // Compute frequencies for sorting
        using PatFreq = std::pair<RectangularPattern, int>;
        std::vector<PatFreq> sorted_R;
        
        for (size_t i=0; i<R.size(); ++i) {
            int min_sup = 1e9;
            for (const auto& ids : R_freq_sets[i]) {
                if ((int)ids.size() < min_sup) min_sup = (int)ids.size();
            }
            if (min_sup >= min_freq) {
                sorted_R.push_back({R[i], min_sup});
            }
        }
        
        // Sort by frequency descending
        std::sort(sorted_R.begin(), sorted_R.end(), [](const PatFreq& a, const PatFreq& b) {
            return a.second > b.second;
        });

        // Output to file
        std::ofstream out_file("scripts/output_patterns.txt");
        bool header_written = false;
        
        for (const auto& pf : sorted_R) {
            // Write Sketch header if not written (optional - assumes homogeneous patterns)
            if (!header_written && !pf.first.O_P.empty()) {
               // Optional: Write keyword types? 
               // For now just write the patterns
            }
            
            // Format: 
            // Frequency: F
            // W H N
            // ID X Y KW
            auto str = pf.first.toString();
            
            std::cout << "Frequency: " << pf.second << std::endl;
            std::cout << str << std::endl;
            
            if (out_file.is_open()) {
                out_file << "Frequency: " << pf.second << "\n";
                out_file << str << "\n";
            }
        }
        if (out_file.is_open()) out_file.close();

        // Convert back to simple vector for return
        std::vector<RectangularPattern> final_R;
        for(const auto& pf : sorted_R) final_R.push_back(pf.first);

        return final_R;
    }

    /**
     * @brief Signature-based Sweep-Line Algorithm with Y-axis discretization
     */
    inline std::vector<RectangularPattern> signature_sweep_line(const Spatial& D, const RectangularSketch& S, double epsilon, int min_freq) {
        // 1. Get Candidate Instances
        std::vector<Instance> C = generate_candidates(D, S);
        std::cout << "\n[Signature Sweep-Line] Found " << C.size() << " candidate instances." << std::endl;

        double a = S.size.a;
        double b = S.size.b;

        // --- SIGNATURE GROUPING LOGIC ---
        
        // 1. Compute Signatures
        std::vector<std::vector<int>> signatures(C.size());
        for(size_t i=0; i < C.size(); ++i) {
            // OPTIMIZATION: Do NOT sort C[i] in place. Sort a temp structure for calculating signature.
            // This preserves C[i]'s order (from dataset) which might be optimal for getMatching/memory locality.
            
            std::vector<SpatialObject> sortedParams = C[i].O_P;
            std::sort(sortedParams.begin(), sortedParams.end(), [](const SpatialObject& a, const SpatialObject& b) {
                if (a.keyword != b.keyword) return a.keyword < b.keyword;
                if (std::abs(a.y - b.y) > 1e-9) return a.y < b.y;
                if (std::abs(a.x - b.x) > 1e-9) return a.x < b.x;
                return a.id < b.id;
            });

            double minY = 1e20;
            for(const auto& o : sortedParams) if(o.y < minY) minY = o.y;

            signatures[i].reserve(sortedParams.size());
            for(const auto& o : sortedParams) {
                double delta = o.y - minY;
                signatures[i].push_back(static_cast<int>(std::floor(delta / (2.0 * epsilon))));
            }
        }

        std::vector<RectangularPattern> R;
        std::vector<bool> processed(C.size(), false);
        
        for (size_t i = 0; i < C.size(); ++i) {
            if (processed[i]) continue;

            const Instance& I_ref = C[i];
            RectangularPattern P(a, b);
            P.O_P = I_ref.O_P; 

            std::vector<std::set<int>> F_set(P.O_P.size());
            for (size_t j = 0; j < P.O_P.size(); ++j) {
                F_set[j].insert(P.O_P[j].id);
            }

            // Grouping with Signature Pruning
            for (size_t k = i + 1; k < C.size(); ++k) {
                if (processed[k]) continue;

                // PRUNING START
                if (signatures[i].size() != signatures[k].size()) continue; 
                
                bool sigMatch = true;
                for (size_t s = 0; s < signatures[i].size(); ++s) {
                    if (std::abs(signatures[i][s] - signatures[k][s]) > 1) { 
                        sigMatch = false;
                        break;
                    }
                }
                if (!sigMatch) continue;
                // PRUNING END

                std::vector<int> mapping;
                if (P.getMatching(C[k], epsilon, a / 2.0, b / 2.0, a / 2.0, b / 2.0, mapping)) {
                    processed[k] = true; 
                    for (size_t j = 0; j < P.O_P.size(); ++j) {
                        F_set[j].insert(C[k].O_P[mapping[j]].id);
                    }
                }
            }

            bool isFrequent = true;
            for (const auto& ids : F_set) {
                if ((int)ids.size() < min_freq) {
                    isFrequent = false;
                    break;
                }
            }

            if (isFrequent) {
                R.push_back(P);
            }
        }

        return R;
    }
}

#endif // FSPM_PLUS_HPP
