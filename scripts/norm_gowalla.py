import pandas as pd
import os

def normalize_gowalla(input_path, output_path):
    print(f"Processing {input_path}...")
    
    # Gowalla 数据集通常是制表符分隔，没有表头
    # 根据用户描述：venueCategory, time, latitude, longitude, venueId
    try:
        df = pd.read_csv(input_path, sep='\t', header=None, 
                         names=['venueCategory', 'time', 'latitude', 'longitude', 'venueId'])
    except Exception as e:
        print(f"Error reading file: {e}")
        return

    # 抽取需要的列（去除 time）
    df_norm = df[['venueId', 'venueCategory', 'latitude', 'longitude']]
    
    # 按照 venueId 去重
    print("Deduplicating by venueId...")
    df_norm = df_norm.drop_duplicates(subset=['venueId'])
    
    # 按照 venueId, venueCategory, latitude, longitude 排序
    print("Sorting data...")
    df_norm = df_norm.sort_values(by=['venueId', 'venueCategory', 'latitude', 'longitude'])
    
    # 保存结果
    df_norm.to_csv(output_path, index=False)
    print(f"Saved normalized gowalla dataset to {output_path}")
    print(f"Original records: {len(df)}, Normalized records: {len(df_norm)}")

def reindex_gowalla(file_path):
    print(f"Reading {file_path}...")
    df = pd.read_csv(file_path)
    
    # 1. Reindex venueId
    print("Reindexing venueId...")
    unique_venues = sorted(df['venueId'].unique())
    venue_map = {old_id: new_id for new_id, old_id in enumerate(unique_venues)}
    df['venueId'] = df['venueId'].map(venue_map)
    
    # 2. Reindex venueCategoryId
    print("Reindexing venueCategoryId...")
    unique_categories = sorted(df['venueCategoryId'].unique())
    cat_map = {old_id: new_id for new_id, old_id in enumerate(unique_categories)}
    df['venueCategoryId'] = df['venueCategoryId'].map(cat_map)
    
    # Save the result
    df.to_csv(file_path, index=False)
    print(f"Successfully reindexed. Results saved to {file_path}")
    print(f"Total Unique Venues: {len(unique_venues)}")
    print(f"Total Unique Categories: {len(unique_categories)}")

if __name__ == "__main__":
    base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    input_file = os.path.join(base_dir, 'datasets', 'Gowalla_totalCheckins.txt')
    output_file = os.path.join(base_dir, 'datasets', 'Gowalla_norm.csv')
    
    if os.path.exists(input_file):
        normalize_gowalla(input_file, output_file)
    else:
        print(f"File not found: {input_file}")

    current_dir = os.path.dirname(os.path.abspath(__file__))
    target_file = os.path.join(os.path.dirname(current_dir), 'datasets', 'Gowalla.csv')
    
    if os.path.exists(target_file):
        reindex_gowalla(target_file)
    else:
        print(f"Error: {target_file} not found.")
