import pandas as pd
import glob
import os
import csv
import sys

class DatasetManager:
    def __init__(self):
        # Base directory is assumed to be the parent of the 'scripts' directory
        self.base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        self.datasets_dir = os.path.join(self.base_dir, 'datasets')
        self.fsq_base_dir = os.path.join(self.datasets_dir, 'fsq-os-places')
        
    def _get_paths(self):
        return {
            'cat_parquet': os.path.join(self.fsq_base_dir, 'categories', 'parquet', 'categories.zstd.parquet'),
            'places_pattern': os.path.join(self.fsq_base_dir, 'places', 'parquet', '*.parquet'),
            # Output for the specific 10-file extraction request
            'output_fsq_subset': os.path.join(self.datasets_dir, 'fsq_10_files.csv'),
        }

    def process_fsq_parquet(self, start=0, end=10):
        """
        Extracts data from parquet files in range [start, end).
        Combines logic from convert_fsq.py and norm_foursquare.py.
        """
        paths = self._get_paths()
        
        # Determine output filename
        if end is None:
            if start == 0:
                out_name = 'fsq_all_files.csv'
            else:
                out_name = f'fsq_{start}_to_end.csv'
        else:
            if start == 0:
                out_name = f'fsq_{end}_files.csv'
            else:
                out_name = f'fsq_{start}_{end}.csv'
        
        paths['output_fsq_subset'] = os.path.join(self.datasets_dir, out_name)

        print(f"Starting Foursquare Parquet processing (Range: {start} to {end})...")
        
        # 1. Process Categories
        if not os.path.exists(paths['cat_parquet']):
            print(f"Error: Category file not found at {paths['cat_parquet']}")
            return

        print("Reading categories...")
        cat_df = pd.read_parquet(paths['cat_parquet'])
        # Map category_id to a continuous integer index (0, 1, 2...)
        unique_cats = sorted(cat_df['category_id'].unique())
        cat_map = {cat_id: i for i, cat_id in enumerate(unique_cats)}
        print(f"Loaded {len(cat_map)} categories.")

        # 2. Process Places
        files = sorted(glob.glob(paths['places_pattern']))
        if not files:
            print(f"Error: No place parquet files found at {paths['places_pattern']}")
            return

        # Filter files by range
        if end is not None:
            files = files[start:end]
        else:
            files = files[start:]
            
        print(f"Selected {len(files)} files for processing (Indices {start} to {end}).")

        seen_ids = set()
        next_venue_id = 0
        
        # Prepare content list to avoid keeping file open/append ambiguity, 
        # but for large data, streaming write is better.
        # We will use streaming write to a temp file then normalize.
        temp_output = paths['output_fsq_subset'] + ".temp"
        
        print(f"Writing temporary data to {temp_output}...")
        with open(temp_output, 'w', newline='', encoding='utf-8') as f:
            writer = csv.writer(f)
            writer.writerow(['venueId', 'venueCategoryId', 'latitude', 'longitude'])
            
            for file_path in files:
                print(f"Processing {os.path.basename(file_path)}...")
                try:
                    df = pd.read_parquet(file_path, columns=['fsq_place_id', 'fsq_category_ids', 'latitude', 'longitude'])
                except Exception as e:
                    print(f"Error reading {file_path}: {e}")
                    continue

                # Drop rows with no categories
                df = df.dropna(subset=['fsq_category_ids'])

                # Extract first category
                # Helper to safely get first element
                def get_first_cat(cats):
                    if cats is not None and len(cats) > 0:
                        return cats[0]
                    return None

                df['cat_id'] = df['fsq_category_ids'].apply(get_first_cat)
                df = df.dropna(subset=['cat_id'])
                
                # Filter for categories in our map
                df = df[df['cat_id'].isin(cat_map)]
                
                # Deduplicate within this chunk
                df = df.drop_duplicates(subset=['fsq_place_id'])
                
                # Global deduplication
                # Note: For massive datasets, keeping seen_ids in memory is expensive, 
                # but for 10 files it's fine.
                df = df[~df['fsq_place_id'].isin(seen_ids)]
                seen_ids.update(df['fsq_place_id'])
                
                if df.empty:
                    print("  No new valid records in this file.")
                    continue

                # Map categories to int
                df['venueCategoryId'] = df['cat_id'].map(cat_map).astype(int)
                
                # Map venueId (Continuous integer)
                num_new = len(df)
                df['venueId'] = range(next_venue_id, next_venue_id + num_new)
                next_venue_id += num_new
                
                # Write chunk
                df[['venueId', 'venueCategoryId', 'latitude', 'longitude']].to_csv(f, header=False, index=False)
                print(f"  Added {num_new} records. Total venues: {next_venue_id}")

        print("Raw extraction complete. Starting Normalization (Reindexing)...")
        # 3. Apply final Normalization (Reindexing based on the collected full set)
        # This ensures IDs are perfectly contiguous 0..N without gaps 
        # (though our streaming appoach above was already pretty good, re-checking is safer).
        self._normalize_csv(temp_output, paths['output_fsq_subset'])
        
        if os.path.exists(temp_output):
            os.remove(temp_output)
            
        print(f"Success! Final dataset saved to {paths['output_fsq_subset']}")

    def normalize_dataset_file(self, file_path):
        """
        Generic function to normalize a dataset file.
        Expects columns: venueId, venueCategoryId, latitude, longitude
        """
        if not os.path.exists(file_path):
            print(f"File not found: {file_path}")
            return
        self._normalize_csv(file_path, file_path) # Overwrite

    def _normalize_csv(self, input_path, output_path):
        """
        Reads CSV, removes NaNs, reindexes venueId and venueCategoryId to be contiguous integers.
        """
        print(f"Normalizing {input_path} -> {output_path} ...")
        df = pd.read_csv(input_path)
        
        # Drop NaNs
        initial_len = len(df)
        df = df.dropna().reset_index(drop=True)
        if len(df) < initial_len:
            print(f"  Dropped {initial_len - len(df)} NaN rows.")

        # Ensure correct columns if they exist under different names (simple heuristic)
        # For this specific project, we assume standard columns from our extraction.
        
        # Reindex venueId
        if 'venueId' in df.columns:
            unique_venues = sorted(df['venueId'].unique())
            venue_map = {old: new for new, old in enumerate(unique_venues)}
            df['venueId'] = df['venueId'].map(venue_map).astype(int)
        
        # Reindex venueCategoryId
        if 'venueCategoryId' in df.columns:
            unique_cats = sorted(df['venueCategoryId'].unique())
            cat_map = {old: new for new, old in enumerate(unique_cats)}
            df['venueCategoryId'] = df['venueCategoryId'].map(cat_map).astype(int)
        
        df.to_csv(output_path, index=False)
        print(f"  Saved {len(df)} records. Unique Venues: {df['venueId'].nunique()}, Unique Cats: {df['venueCategoryId'].nunique()}")

    def normalize_gowalla_legacy(self, input_path, output_path):
        """
        Combined logic from norm_gowalla.py
        """
        print(f"Processing Gowalla from {input_path}...")
        if not os.path.exists(input_path):
            print("  Input path not found.")
            return

        try:
            # Gowalla: venueCategory, time, latitude, longitude, venueId (tab separated)
            df = pd.read_csv(input_path, sep='\t', header=None, 
                             names=['venueCategory', 'time', 'latitude', 'longitude', 'venueId'])
        except Exception as e:
            print(f"Error reading file: {e}")
            return

        # Select cols
        df = df[['venueId', 'venueCategory', 'latitude', 'longitude']]
        # Temporary rename for unified processing if needed, but norm_gowalla keeps venueCategory distinct from Id for a moment?
        # Actually norm_gowalla logic had a 'reindex' function that expected venueCategoryId.
        # We will map venueCategory strings/ids to integers.
        
        df = df.rename(columns={'venueCategory': 'venueCategoryId'}) # Treat the raw cat as the ID column for reindexing

        df = df.drop_duplicates(subset=['venueId'])
        df = df.sort_values(by=['venueId', 'venueCategoryId', 'latitude', 'longitude'])
        
        # Save temp
        df.to_csv(output_path, index=False)
        
        # Apply standard normalization (reindexing)
        self._normalize_csv(output_path, output_path)

    def normalize_nyc(self, path):
        """
        Combined logic from norm_nyc_tky.py
        """
        self._normalize_csv(path, path)

if __name__ == "__main__":
    manager = DatasetManager()
    
    # Simple CLI to handle the user's request
    if len(sys.argv) > 1:
        cmd = sys.argv[1]
        
        if cmd == "fsq_all":
            manager.process_fsq_parquet(start=0, end=None)
        elif cmd.startswith("fsq_"):
            parts = cmd.split("_")
            try:
                if len(parts) == 2:
                    # fsq_10 -> end=10
                    end = int(parts[1])
                    manager.process_fsq_parquet(start=0, end=end)
                elif len(parts) == 3:
                    # fsq_10_20 -> start=10, end=20
                    start = int(parts[1])
                    end = int(parts[2])
                    manager.process_fsq_parquet(start=start, end=end)
                else:
                    raise ValueError
            except ValueError:
                 print(f"Invalid command format: {cmd}. Expected fsq_<n> or fsq_<start>_<end>")
        elif cmd == "norm":
             if len(sys.argv) > 2:
                 manager.normalize_dataset_file(sys.argv[2])
             else:
                 print("Usage: python dataset_manager.py norm <file_path>")
        else:
            print("Unknown command. Available: fsq_<n>, fsq_<start>_<end>, fsq_all, norm")
    else:
        # Default action as per user request: "Extract 10 parquet and initialize"
        print("No arguments provided. Defaulting to: Extract 10 FSQ parquets.")
        manager.process_fsq_parquet(start=0, end=10)
