import pandas as pd
import os

def convert_foursquare_parquet_to_csv(input_path, output_path):
    print(f"Reading {input_path}...")
    try:
        # 读取 Parquet 文件
        df = pd.read_parquet(input_path)
        
        # 1. 保存原始转换后的全量 CSV
        full_csv = output_path.replace('.csv', '_full.csv')
        df.to_csv(full_csv, index=False)
        print(f"Saved full conversion to {full_csv}")
        
        # 2. 提取唯一的地点信息并规范化 (与 NYC/TKY, Gowalla 格式一致)
        print("Normalizing unique venues...")
        v1 = df[['id_1', 'categories_1', 'latitude_1', 'longitude_1']].rename(
            columns={'id_1': 'venueId', 'categories_1': 'venueCategory', 'latitude_1': 'latitude', 'longitude_1': 'longitude'}
        )
        v2 = df[['id_2', 'categories_2', 'latitude_2', 'longitude_2']].rename(
            columns={'id_2': 'venueId', 'categories_2': 'venueCategory', 'latitude_2': 'latitude', 'longitude_2': 'longitude'}
        )
        
        df_norm = pd.concat([v1, v2], ignore_index=True)
        df_norm = df_norm.drop_duplicates(subset=['venueId'])
        df_norm = df_norm.sort_values(by=['venueId'])
        
        df_norm.to_csv(output_path, index=False)
        print(f"Saved normalized Foursquare CSV to {output_path}")
        print(f"Original pairs: {len(df)}, Unique venues: {len(df_norm)}")
        
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    input_file = os.path.join(base_dir, 'datasets', 'Foursquare.parquet')
    output_file = os.path.join(base_dir, 'datasets', 'Foursquare.csv')
    
    if os.path.exists(input_file):
        convert_foursquare_parquet_to_csv(input_file, output_file)
    else:
        print(f"File not found: {input_file}")
