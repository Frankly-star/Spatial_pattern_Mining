import pandas as pd
import os

cat_path = 'd:/WORKSPACE/Spatial_pattern_Mining/datasets/fsq-os-places/categories/parquet/categories.zstd.parquet'
place_path = 'd:/WORKSPACE/Spatial_pattern_Mining/datasets/fsq-os-places/places/parquet/places-00000.zstd.parquet'

print("--- Categories Schema ---")
try:
    df_cat = pd.read_parquet(cat_path)
    print(df_cat.dtypes)
    print(df_cat.head())
except Exception as e:
    print(f"Error reading categories: {e}")

print("\n--- Places Schema ---")
try:
    df_place = pd.read_parquet(place_path)
    print(df_place.dtypes)
    print(df_place.head())
except Exception as e:
    print(f"Error reading places: {e}")
