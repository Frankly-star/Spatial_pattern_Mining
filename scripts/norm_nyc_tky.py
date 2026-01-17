import pandas as pd
import os

def process_datasets():
    # 1. 定义路径
    base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    datasets_dir = os.path.join(base_dir, 'datasets')
    
    nyc_input = os.path.join(datasets_dir, 'dataset_TSMC2014_NYC.csv')
    tky_input = os.path.join(datasets_dir, 'dataset_TSMC2014_TKY.csv')
    output_path = os.path.join(datasets_dir, 'merged_NYC_TKY.csv')

    # 检查输入文件
    if not os.path.exists(nyc_input) or not os.path.exists(tky_input):
        print(f"Error: 确保 {nyc_input} 和 {tky_input} 存在。")
        return

    # 2. 读取并初步抽取
    print("Reading and normalizing NYC...")
    df_nyc = pd.read_csv(nyc_input, low_memory=False)[['venueId', 'venueCategory', 'latitude', 'longitude']]
    
    print("Reading and normalizing TKY...")
    df_tky = pd.read_csv(tky_input, low_memory=False)[['venueId', 'venueCategory', 'latitude', 'longitude']]

    # 3. 合并
    print("Merging NYC and TKY datasets...")
    df_merged = pd.concat([df_nyc, df_tky], ignore_index=True)

    # 4. 去重（保证 venueId 唯一）
    print("Deduplicating by venueId...")
    df_merged = df_merged.drop_duplicates(subset=['venueId']).reset_index(drop=True)

    # 5. 为 venueCategory 分配递增 ID (从 0 开始)
    print("Assigning venueCategoryId...")
    unique_categories = sorted(df_merged['venueCategory'].unique())
    cat_map = {cat: i for i, cat in enumerate(unique_categories)}
    df_merged['venueCategoryId'] = df_merged['venueCategory'].map(cat_map)

    # 6. 为每个地点分配新的递增 ID (venueId_new, 从 0 开始)
    print("Assigning new venueId...")
    df_merged['venueId_new'] = range(len(df_merged))

    # 7. 整理列顺序
    cols = ['venueId_new', 'venueId', 'venueCategoryId', 'venueCategory', 'latitude', 'longitude']
    df_merged = df_merged[cols]

    # 8. 保存最终结果
    df_merged.to_csv(output_path, index=False)
    print(f"\nFinal result saved to: {output_path}")
    print(f"Total Unique Venues: {len(df_merged)}")
    print(f"Total Unique Categories: {len(unique_categories)}")

if __name__ == "__main__":
    process_datasets()
