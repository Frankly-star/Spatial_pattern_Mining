import pandas as pd
import os

def list_categories():
    base_dir = r'd:\WORKSPACE\Spatial_pattern_Mining\datasets\fsq-os-places'
    cat_parquet = os.path.join(base_dir, 'categories', 'parquet', 'categories.zstd.parquet')
    
    if not os.path.exists(cat_parquet):
        print("Category file not found.")
        return

    df = pd.read_parquet(cat_parquet)
    # df likely has 'category_id', 'name', maybe 'parent_category_id'
    
    # Sort by name for easier reading
    print("Columns:", df.columns)
    # The actual column is 'category_name' based on previous error
    name_col = 'category_name'
    
    print("\nSample Categories:")
    print(df[['category_id', name_col]].sample(20))
    
    # Let's save a simpler csv for quick lookup if needed, or just search in memory
    # keyword 'Restaurant', 'Park', 'School', 'Mall' often make good patterns
    
    # search_terms = ['Restaurant', 'Park', 'School', 'Coffee', 'Bar', 'Gym', 'Hotel', 'Museum', 'Beach']
    # search_terms = ['Metro', 'Subway', 'Station', 'Office', 'Corporate', 'Building', 'University', 'College', 'Gate']
    
    # 3rd attempt: Focus on "Last Mile Commute" (Metro -> Convenience/Fast Food -> Residential)
    search_terms = ['Residential', 'Apartment', 'Condo', 'Convenience', 'Grocery', 'Supermarket', 'Fast Food', 'Bus', 'Stop']
    
    print("\n--- Search Results ---")
    for term in search_terms:
        matches = df[df[name_col].str.contains(term, case=False, na=False)]
        if not matches.empty:
            print(f"\nMatches for '{term}':")
            print(matches[['category_id', name_col]].head(5))

    # Also we need to know the integer ID mapping used in our dataset_manager.py
    # dataset_manager.py sorts unique category_ids and enumerates them.
    # We should replicate that to give the user the correct integer IDs.
    
    unique_cats = sorted(df['category_id'].unique())
    cat_map = {cat_id: i for i, cat_id in enumerate(unique_cats)}
    
    print("\n--- Suggested Case Study Candidates (with mapped Integer IDs) ---")
    
    # Helper to find ID by name
    def find_id(name_part):
        matches = df[df[name_col].str.contains(name_part, case=False, na=False)]
        if matches.empty:
            return None
        # Pick the most generic/shortest name usually preferred, or just the first
        # Let's pick the one with shortest name
        best_match = matches.loc[matches[name_col].str.len().idxmin()]
        cat_id = best_match['category_id']
        mapped_id = cat_map.get(cat_id)
        return best_match[name_col], mapped_id

    candidates = [
        # Case 1: "Going Home" - The Commuter Trap
        # Pattern: Metro Station -> Convenience Store -> Apartment/Residential
        # Hypothesis: Convenience Store is closer to Metro than Residential is.
        ['Metro Station', 'Convenience Store', 'Residential Building'],

        # Case 2: "Transit Hub Interchange"
        # Pattern: Metro Station -> Bus Station -> Parking
        # Hypothesis: Bus Station is very close to Metro, Parking is slightly further ring.
        ['Metro Station', 'Bus Station', 'Parking'],

        # Case 3: "Lunch Break"
        # Pattern: Office -> Fast Food -> Park
        # Hypothesis: Fast Food is closer to Office than Park is.
        ['Office Building', 'Fast Food Restaurant', 'Park']
    ]
    
    for group in candidates:
        print(f"\nGroup: {group}")
        for name in group:
            result = find_id(name)
            if result is not None:
                real_name, int_id = result
                print(f"  - {name} -> '{real_name}' (ID: {int_id})")
            else:
                print(f"  - {name} -> Not Found")

if __name__ == "__main__":
    list_categories()
