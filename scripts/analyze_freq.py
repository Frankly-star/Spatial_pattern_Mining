import pandas as pd
import os

def analyze_frequencies():
    base_dir = r'd:\WORKSPACE\Spatial_pattern_Mining\datasets\fsq-os-places'
    cat_parquet = os.path.join(base_dir, 'categories', 'parquet', 'categories.zstd.parquet')
    # Updated to point to an existing file found in dir
    data_csv = r'd:\WORKSPACE\Spatial_pattern_Mining\datasets\fsq_10_25.csv'

    if not os.path.exists(data_csv):
        print(f"Dataset file not found: {data_csv}")
        # Try fallbacks
        data_csv = r'd:\WORKSPACE\Spatial_pattern_Mining\datasets\fsq_all_files.csv'
        if not os.path.exists(data_csv):
            print("No processed dataset found. calculating mapping only...")
            data_csv = None

    print("1. Loading Categories Definition...")
    df_cat = pd.read_parquet(cat_parquet)
    
    # Replicate the deterministic mapping logic from dataset_manager.py
    # "unique_cats = sorted(cat_df['category_id'].unique())"
    # "cat_map = {cat_id: i for i, cat_id in enumerate(unique_cats)}"
    
    unique_cats_raw = sorted(df_cat['category_id'].unique())
    
    # Map Int ID -> Raw ID
    int_to_raw = {i: raw for i, raw in enumerate(unique_cats_raw)}
    
    # Map Raw ID -> Name
    # Need to handle potential duplicates in raw df or just pick first
    raw_to_name = df_cat.set_index('category_id')['category_name'].to_dict()

    if data_csv:
        print(f"2. Loading Data from {data_csv} ...")
        # Only read venueCategoryId column to save memory
        df_data = pd.read_csv(data_csv, usecols=['venueCategoryId'])
        
        print(f"3. Counting Frequencies (Total rows: {len(df_data)})...")
        counts = df_data['venueCategoryId'].value_counts().head(50)
        
        print("\n--- TOP 50 MOST FREQUENT CATEGORIES ---")
        print(f"{'IntID':<8} {'Count':<10} {'Name'}")
        print("-" * 50)
        
        for int_id, count in counts.items():
            raw_id = int_to_raw.get(int_id)
            name = raw_to_name.get(raw_id, "Unknown")
            print(f"{int_id:<8} {count:<10} {name}")
            
    else:
        print("Skipping data counts (file missing).")

if __name__ == "__main__":
    analyze_frequencies()
