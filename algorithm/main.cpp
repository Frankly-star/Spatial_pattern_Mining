#include <iostream>
#include <vector>
#include <string>
#include "dataset.hpp"
#include "rectangular.hpp"
#include "fspm.hpp"
using namespace std;

int main() {
    string filePath = "d:/WORKSPACE/Spatial_pattern_Mining/datasets/Gowalla.csv";
    Spatial db;
    
    if (db.load(filePath)) {
        
        RectangularSketch S(1.0, 1.0);
        S.addKeyword(0); 

        auto results = fspm(db, S, 0.1, 2, 1.0);

        cout << "Found " << results.size() << " frequent patterns." << endl;
    }
    return 0;
} 