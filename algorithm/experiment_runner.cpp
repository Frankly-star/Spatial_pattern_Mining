#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <fstream>
#include <random>
#include <algorithm> 
#include <map>
#include <unordered_map>
#include "dataset.hpp"
#include "rectangular.hpp"
#include "fspm.hpp"
#include "fspm+.hpp"

using namespace std;

// --- Helper Functions ---

// Get frequent keywords to form meaningful sketches
vector<int> getFrequentKeywords(const Spatial& db, int limit) {
    unordered_map<int, int> counts;
    for (const auto& obj : db.objects) {
        counts[obj.keyword]++;
    }
    vector<pair<int, int>> sorted;
    for (const auto& p : counts) sorted.push_back(p);
    sort(sorted.begin(), sorted.end(), [](const pair<int,int>& a, const pair<int,int>& b) {
        return a.second > b.second; // Descending
    });
    vector<int> result;
    for (int i = 0; i < min((int)sorted.size(), limit); i++) {
        result.push_back(sorted[i].first);
    }
    return result;
}

// Random Sample (Scalability & Random Dist)
Spatial sampleRandom(const Spatial& original, double ratio) {
    Spatial sample;
    sample.x_min = original.x_min; sample.x_max = original.x_max;
    sample.y_min = original.y_min; sample.y_max = original.y_max;
    
    // Copy and shuffle
    vector<SpatialObject> objs = original.objects;
    // Use a fixed seed for reproducibility
    mt19937 g(42);
    shuffle(objs.begin(), objs.end(), g);
    
    size_t newSize = (size_t)(objs.size() * ratio);
    if (newSize == 0 && !objs.empty()) newSize = 1;
    
    sample.objects.assign(objs.begin(), objs.begin() + newSize);
    return sample;
}

// Dense Region Sample (Clustered Dist)
Spatial sampleDense(const Spatial& original, double ratio) {
    Spatial sample;
    sample.x_min = original.x_min; sample.x_max = original.x_max;
    sample.y_min = original.y_min; sample.y_max = original.y_max;

    if (original.objects.empty()) return sample;

    // Calculate Centroid
    double cx = 0, cy = 0;
    for(const auto& o : original.objects) { cx += o.x; cy += o.y; }
    cx /= original.objects.size();
    cy /= original.objects.size();

    // Sort by distance to centroid
    vector<pair<double, int>> dists;
    dists.reserve(original.objects.size());
    for(int i=0; i<original.objects.size(); ++i) {
        double d = pow(original.objects[i].x - cx, 2) + pow(original.objects[i].y - cy, 2);
        dists.push_back({d, i});
    }
    sort(dists.begin(), dists.end()); // Ascending distance (closest first)

    size_t newSize = (size_t)(original.objects.size() * ratio);
    if (newSize == 0 && !original.objects.empty()) newSize = 1;

    for(size_t i=0; i<newSize; ++i) {
        sample.objects.push_back(original.objects[dists[i].second]);
    }
    return sample;
}

// --- Experiment Runner ---

void run_experiments() {
    string outputPath = "d:/WORKSPACE/Spatial_pattern_Mining/scripts/experiment_results.csv";
    ofstream csv(outputPath);
    if (!csv.is_open()) {
        cerr << "Failed to open output file: " << outputPath << endl;
        return;
    }
    // Updated header
    csv << "Experiment,Dataset,Algorithm,SketchSize,NumAttributes,DataScale,Distribution,Time(s),PatternsFound\n";

    double epsilon = 0.05; 
    int min_freq = 5; 
    
    // --- Algorithmic Runner Helper ---
    auto execute_algo = [&](string expName, string datasetName, Spatial& useDb, string algoName, const RectangularSketch& S, string distType, double scale) {
        cout << "    [" << expName << "] " << datasetName << " (" << distType << ", " << (int)(scale*100) << "%) Algo: " << algoName << flush;
        auto start = chrono::high_resolution_clock::now();
        size_t count = 0;
        
        if (algoName == "FSPM") {
            auto res = fspm(useDb, S, epsilon, min_freq);
            count = res.size();
        } else if (algoName == "FSPM+") {
            auto res = fspm_plus::fspm_plus(useDb, S, epsilon, min_freq);
            count = res.size();
        } else if (algoName == "Signature") {
            auto res = fspm_plus::signature_sweep_line(useDb, S, epsilon, min_freq);
            count = res.size();
        } else if (algoName == "SignatureX") {
            auto res = fspm_plus::signature_sweep_line_x(useDb, S, epsilon, min_freq);
            count = res.size();
        } else if (algoName == "TreeOpt") {
            auto res = fspm_plus::tree_optimized_fspm(useDb, S, epsilon, min_freq);
            count = res.size();
        }
        
        auto end = chrono::high_resolution_clock::now();
        double duration = chrono::duration<double>(end-start).count();
        cout << " -> " << duration << "s (" << count << " patterns)" << endl;
        
        csv << expName << "," << datasetName << "," << algoName << "," << S.size.a << "," << S.K.size() << "," << scale << "," << distType << "," << duration << "," << count << "\n";
        csv.flush();
    };

    // =========================================================
    // Exp 1 & 2: Sketch Size & Attribute Count (Original)
    // =========================================================
    vector<string> basic_datasets = {
        "d:/WORKSPACE/Spatial_pattern_Mining/datasets/fsq_10_25.csv",
        "d:/WORKSPACE/Spatial_pattern_Mining/datasets/Gowalla.csv",
        "d:/WORKSPACE/Spatial_pattern_Mining/datasets/NYC_TKY.csv"
    };

    for (const auto& path : basic_datasets) {
        Spatial db;
        if (!db.load(path)) continue;
        string name = path.substr(path.find_last_of("/\\") + 1);
        cout << "\nProcessing Basic Experiments for " << name << endl;
        
        auto keywords = getFrequentKeywords(db, 20);
        if (keywords.empty()) continue;

        // Params
        vector<double> sizes = {0.5, 1.0, 2.0, 3.0}; // km
        vector<int> attrs = {2, 3, 4, 5};
        double def_size = 1.0;
        int def_attr = 3;

        // Vary Size
        for (double sz : sizes) {
            RectangularSketch S(sz, sz);
            for(int k=0; k<def_attr && k<keywords.size(); ++k) S.addKeyword(keywords[k]);
            execute_algo("VarySize", name, db, "FSPM", S, "Original", 1.0);
            execute_algo("VarySize", name, db, "FSPM+", S, "Original", 1.0);
            execute_algo("VarySize", name, db, "Signature", S, "Original", 1.0);
            execute_algo("VarySize", name, db, "TreeOpt", S, "Original", 1.0);
        }
        // Vary Attrs
        for (int at : attrs) {
            RectangularSketch S(def_size, def_size);
            for(int k=0; k<at && k<keywords.size(); ++k) S.addKeyword(keywords[k]);
            execute_algo("VaryAttr", name, db, "FSPM", S, "Original", 1.0);
            execute_algo("VaryAttr", name, db, "FSPM+", S, "Original", 1.0);
            execute_algo("VaryAttr", name, db, "Signature", S, "Original", 1.0);
            execute_algo("VaryAttr", name, db, "TreeOpt", S, "Original", 1.0);
        }
    }


    // =========================================================
    // Exp 3 & 4: Distribution & Scalability (on fsq_10_25)
    // =========================================================
    {
        string path = "d:/WORKSPACE/Spatial_pattern_Mining/datasets/fsq_10_25.csv";
        Spatial fullDb;
        if (fullDb.load(path)) {
            string name = "fsq_10_25.csv";
            cout << "\nProcessing Distribution & Scalability for " << name << endl;
            auto keywords = getFrequentKeywords(fullDb, 10);
            RectangularSketch S(1.0, 1.0); // Default sketch
            for(int k=0; k<3 && k<keywords.size(); ++k) S.addKeyword(keywords[k]);

            // Scalability: 20% to 100% (Random)
            vector<double> scales = {0.2, 0.4, 0.6, 0.8, 1.0};
            for (double sc : scales) {
                Spatial sub = sampleRandom(fullDb, sc);
                execute_algo("Scalability", name, sub, "FSPM+", S, "Random", sc);
                execute_algo("Scalability", name, sub, "Signature", S, "Random", sc);
                execute_algo("Scalability", name, sub, "TreeOpt", S, "Random", sc);
            }

            // Distribution: Dense vs Random (Fixed Scale = 50%)
            double distScale = 0.5;
            Spatial dense = sampleDense(fullDb, distScale);
            Spatial random = sampleRandom(fullDb, distScale);
            
            // Compare on Dense
            execute_algo("Distribution", name, dense, "FSPM+", S, "Dense", distScale);
            execute_algo("Distribution", name, dense, "Signature", S, "Dense", distScale);
            execute_algo("Distribution", name, dense, "TreeOpt", S, "Dense", distScale);

            // Compare on Random (Already running similar in scalability, but explicit here for clarity in CSV)
            execute_algo("Distribution", name, random, "FSPM+", S, "Random", distScale);
            execute_algo("Distribution", name, random, "Signature", S, "Random", distScale);
            execute_algo("Distribution", name, random, "TreeOpt", S, "Random", distScale);
        }
    }

    // =========================================================
    // Exp 5: FSPM+ Signature X vs Y (on fsq_1_files)
    // =========================================================
    {
        // Note: User specified fsq_1_files.csv
        string path = "d:/WORKSPACE/Spatial_pattern_Mining/datasets/fsq_1_files.csv";
        Spatial db;
        if (db.load(path)) {
            string name = "fsq_1_files.csv";
            cout << "\nProcessing Signature X/Y for " << name << endl;
            auto keywords = getFrequentKeywords(db, 10);
            
            // Run a few variations to be sure
            vector<double> sigsizes = {1.0, 2.0};
            for (double sz : sigsizes) {
                RectangularSketch S(sz, sz);
                for(int k=0; k<3 && k<keywords.size(); ++k) S.addKeyword(keywords[k]);
                
                // Compare Signature (Y-Dominant usually) vs SignatureX
                execute_algo("SigAxis", name, db, "Signature", S, "Original", 1.0);
                execute_algo("SigAxis", name, db, "SignatureX", S, "Original", 1.0);
            }
        } else {
             cerr << "Failed to find fsq_1_files.csv for Signature X/Y experiment." << endl;
        }
    }

    csv.close();
    cout << "All experiments completed. Check " << outputPath << endl;
}

int main() {
    run_experiments();
    return 0;
}
