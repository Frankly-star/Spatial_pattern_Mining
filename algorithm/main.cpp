#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include "dataset.hpp"
#include "rectangular.hpp"
#include "fspm.hpp"
using namespace std;

int main() {
    string filePath = "d:/WORKSPACE/Spatial_pattern_Mining/datasets/Gowalla.csv";
    Spatial db;
    
    if (db.load(filePath)) {
        
        RectangularSketch S(1, 1);
        S.addKeyword(2); 

        auto start = chrono::high_resolution_clock::now();
        auto results = fspm(db, S, 0.1, 2, 1);
        auto end = chrono::high_resolution_clock::now();

        chrono::duration<double> diff = end - start;

        cout << "Found " << results.size() << " frequent patterns." << endl;
        cout << "Time spent: " << diff.count() << " seconds." << endl;
    }
    return 0;
} 