import pandas as pd
import os

def normalize_foursquare():
    # 1. 定义路径
    base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    input_file = os.path.join(base_dir, 'datasets', 'foursquare.csv')

    if not os.path.exists(input_file):
        print(f"Error: {input_file} not found.")
        return

    # 2. 读取数据
    print(f"Reading {input_file}...")
    df = pd.read_csv(input_file)
    print(f"Initial row count: {len(df)}")

    # 3. 去掉存在 nan 的行
    print("Removing rows with NaN values...")
    df = df.dropna().reset_index(drop=True)
    print(f"Row count after removing NaNs: {len(df)}")

    # 4. 保证 venueId 从 0 开始并且连续
    print("Reindexing venueId...")
    unique_venues = sorted(df['venueId'].unique())
    venue_map = {old_id: new_id for new_id, old_id in enumerate(unique_venues)}
    df['venueId'] = df['venueId'].map(venue_map).astype(int)

    # 5. 保证 venueCategoryId 从 0 开始并且连续
    print("Reindexing venueCategoryId...")
    unique_categories = sorted(df['venueCategoryId'].unique())
    cat_map = {old_id: new_id for new_id, old_id in enumerate(unique_categories)}
    df['venueCategoryId'] = df['venueCategoryId'].map(cat_map).astype(int)

    # 6. 保存结果
    df.to_csv(input_file, index=False)
    print(f"Successfully processed and saved to {input_file}")
    print(f"Final records: {len(df)}")
    print(f"Total Unique Venues: {len(unique_venues)}")
    print(f"Total Unique Categories: {len(unique_categories)}")

if __name__ == "__main__":
    normalize_foursquare()
