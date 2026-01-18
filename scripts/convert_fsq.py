import pandas as pd
import glob
import os
import csv

# Paths
base_dir = r'd:\WORKSPACE\Spatial_pattern_Mining\datasets\fsq-os-places'
cat_parquet = os.path.join(base_dir, 'categories', 'parquet', 'categories.zstd.parquet')
cat_csv = os.path.join(base_dir, 'categories', 'categories.csv')
places_pattern = os.path.join(base_dir, 'places', 'parquet', '*.parquet')
output_csv = os.path.join(r'd:\WORKSPACE\Spatial_pattern_Mining\datasets', 'fsq_os_places_merged.csv')

def convert_categories():
    print("Converting categories to CSV...")
    df = pd.read_parquet(cat_parquet)
    df.to_csv(cat_csv, index=False)
    print(f"Categories saved to {cat_csv}")
    
    # Create mapping: category_id -> continuous index
    # We sort to ensure determinism
    unique_cats = sorted(df['category_id'].unique())
    cat_map = {cat_id: i for i, cat_id in enumerate(unique_cats)}
    return cat_map

def convert_places(cat_map):
    print("Processing places...")
    files = sorted(glob.glob(places_pattern))
    
    # We need to map original IDs to continuous venueId
    # and deduplicate across files.
    seen_ids = set()
    next_venue_id = 0
    
    with open(output_csv, 'w', newline='', encoding='utf-8') as f:
        writer = csv.writer(f)
        writer.writerow(['venueId', 'venueCategoryId', 'latitude', 'longitude'])
        
        for file_path in files:
            print(f"Reading {os.path.basename(file_path)}...")
            # Only read necessary columns
            try:
                df = pd.read_parquet(file_path, columns=['fsq_place_id', 'fsq_category_ids', 'latitude', 'longitude'])
            except Exception as e:
                print(f"Error reading {file_path}: {e}")
                continue
            
            # Process in a vectorized way if possible, or iterate
            # Since fsq_category_ids is a list, we need to handle it
            
            # Drop rows with no categories
            df = df.dropna(subset=['fsq_category_ids'])
            
            # Extract first category
            # Use a helper to get first item or None
            def get_first_cat(cats):
                if cats is not None and len(cats) > 0:
                    return cats[0]
                return None
            
            df['cat_id'] = df['fsq_category_ids'].apply(get_first_cat)
            df = df.dropna(subset=['cat_id'])
            
            # Filter for categories in our map
            df = df[df['cat_id'].isin(cat_map)]
            
            # Deduplicate and reindex
            df = df.drop_duplicates(subset=['fsq_place_id'])
            df = df[~df['fsq_place_id'].isin(seen_ids)]
            seen_ids.update(df['fsq_place_id'])
            
            # Map categories
            df['venueCategoryId'] = df['cat_id'].map(cat_map)
            
            # Map venueId
            num_new = len(df)
            df['venueId'] = range(next_venue_id, next_venue_id + num_new)
            next_venue_id += num_new
            
            # Write to CSV
            df[['venueId', 'venueCategoryId', 'latitude', 'longitude']].to_csv(f, header=False, index=False)
            
            print(f"Added {num_new} new points. Total venueId reaches {next_venue_id}")

    print(f"Done! Merged CSV saved to {output_csv}")

if __name__ == "__main__":
    c_map = convert_categories()
    convert_places(c_map)
