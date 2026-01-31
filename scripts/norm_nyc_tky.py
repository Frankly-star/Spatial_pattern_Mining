import pandas as pd
import os

def process_datasets():
    # 1. 定义路径
    base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    datasets_dir = os.path.join(base_dir, 'datasets')
    output_path = os.path.join(datasets_dir, 'NYC_TKY.csv')

    # 检查文件是否存在
    if not os.path.exists(output_path):
        print(f"Error: 找不到 {output_path}")
        return

    # 2. 直接读取现有的文件并清理
    print(f"Cleaning {output_path}...")
    df = pd.read_csv(output_path)
    
    # 3. 逻辑逻辑：保留 venueId_new 并重命名为 venueId，删除旧的 venueId 和 venueCategory
    # 如果 venueId_new 存在，说明是需要处理的旧格式
    if 'venueId_new' in df.columns:
        df = df[['venueId_new', 'venueCategoryId', 'latitude', 'longitude']]
        df = df.rename(columns={'venueId_new': 'venueId'})
    else:
        # 如果已经处理过，确保列顺序正确
        df = df[['venueId', 'venueCategoryId', 'latitude', 'longitude']]

    # 4. 保存覆盖
    df.to_csv(output_path, index=False)
    print(f"Done! Updated {output_path} to final format.")

if __name__ == "__main__":
    process_datasets()
