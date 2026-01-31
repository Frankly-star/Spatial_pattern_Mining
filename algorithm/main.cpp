#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include "dataset.hpp"
#include "rectangular.hpp"
#include "fspm.hpp"
#include "fspm+.hpp"

using namespace std;

int main() {
    string filePath = "d:/WORKSPACE/Spatial_pattern_Mining/datasets/fsq_10_25.csv";
    Spatial db;
    
    if (db.load(filePath)) {
        
        RectangularSketch S;
        S=S.fromString("1 1 3 6 1 327 1 313 1");

        // 1. Basic FSPM
        // cout << ">>> Running Basic FSPM..." << endl;
        // auto start = chrono::high_resolution_clock::now();
        // auto results = fspm(db, S, 0.1, 2, 1);
        // auto end = chrono::high_resolution_clock::now();

        // chrono::duration<double> diff = end - start;

        // cout << "[FSPM] Found " << results.size() << " frequent patterns." << endl;
        // cout << "[FSPM] Time spent: " << diff.count() << " seconds." << endl;

        // 2. FSPM+
        // cout << "\n>>> Running FSPM+ (Sweep-Line Optimization)..." << endl;
        // auto start2 = chrono::high_resolution_clock::now();
        // auto results2 = fspm_plus::fspm_plus(db, S, 0.5, 2);
        // auto end2 = chrono::high_resolution_clock::now();

        // chrono::duration<double> diff2 = end2 - start2;

        // cout << "[FSPM+] Found " << results2.size() << " frequent patterns." << endl;
        // cout << "[FSPM+] Time spent: " << diff2.count() << " seconds." << endl;

        // // 3. FSPM+ with Signature
        // cout << "\n>>> Running FSPM+ with Signature (Sweep-Line + Y-Discretization)..." << endl;
        // auto start3 = chrono::high_resolution_clock::now();
        // auto results3 = fspm_plus::signature_sweep_line(db, S, 0.1, 2);
        // auto end3 = chrono::high_resolution_clock::now();

        // chrono::duration<double> diff3 = end3 - start3;

        // cout << "[FSPM+ Sig] Found " << results3.size() << " frequent patterns." << endl;
        // cout << "[FSPM+ Sig] Time spent: " << diff3.count() << " seconds." << endl;

        // 4. Tree Optimized FSPM+
        cout << "\n>>> Running Tree Optimized FSPM+..." << endl;
        auto start4 = chrono::high_resolution_clock::now();
        auto results4 = fspm_plus::tree_optimized_fspm(db, S, 0.05, 500);
        auto end4 = chrono::high_resolution_clock::now();

        chrono::duration<double> diff4 = end4 - start4;

        cout << "[Tree Opt] Found " << results4.size() << " frequent patterns." << endl;
        cout << "[Tree Opt] Time spent: " << diff4.count() << " seconds." << endl;
    }
    return 0;
} 