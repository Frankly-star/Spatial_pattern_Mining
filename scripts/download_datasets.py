from pyiceberg.catalog import load_catalog
import polars as pl
import os

ACCESS_TOKEN = "eyJhbGciOiJIUzI1NiJ9.eyJhY3RvclR5cGUiOiJVU0VSIiwiYWN0b3JJZCI6InByb2QtZnNxLXVzZXItMTQxNTU1MDEzNCIsInR5cGUiOiJQRVJTT05BTCIsInZlcnNpb24iOiIyIiwianRpIjoiZjlmMTlhYTUtOWExZS00ZjNmLWEyZGQtYmViZmZkNzY4ODhiIiwic3ViIjoicHJvZC1mc3EtdXNlci0xNDE1NTUwMTM0IiwiZXhwIjoxNzcxMjE4NTg0LCJpc3MiOiJkYXRhaHViLW1ldGFkYXRhLXNlcnZpY2UifQ.BafoCGOyWP6DG6xYr4JXQPvqkw1_0KVlZ8cVqe-3Nec" 

def download_foursquare_data():
    if ACCESS_TOKEN == "YOUR_ACCESS_TOKEN":
        print("Error: 请先在 download_datasets.py 中设置您的 Foursquare Access Token。")
        return

    # 1. 加载 Foursquare Iceberg Catalog
    print("Loading Foursquare catalog...")
    catalog = load_catalog(
        "default",
        **{
            "warehouse": "places",
            "uri": "https://catalog.h3-hub.foursquare.com/iceberg",
            "token": ACCESS_TOKEN,
            "header.content-type": "application/vnd.api+json",
            "rest-metrics-reporting-enabled": "false",
        },
    )

    # 2. 获取输出目录
    base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    datasets_dir = os.path.join(base_dir, 'datasets')
    if not os.path.exists(datasets_dir):
        os.makedirs(datasets_dir)

    # 3. 下载特定的 OS Categories 表
    namespaces = "datasets"
    table_name = "places_os"

    print(f"Connecting to '{namespaces}.{table_name}'...")
    try:
        table_path = (namespaces, table_name)
        table = catalog.load_table(table_path)
        
        output_file = os.path.join(datasets_dir, f"{table_name}.parquet")

        print(f"Fetching data from '{table_name}' using PyIceberg & PyArrow...")
        
        # 1. 使用 PyIceberg 扫描表并获取 Arrow 数据流
        # scan() 会自动处理 Foursquare Catalog 返回的 S3 临时凭证
        scanner = table.scan()
        arrow_table = scanner.to_arrow()

        # 2. 将内容存入 Parquet
        import pyarrow.parquet as pq
        pq.write_table(arrow_table, output_file)
        
        print(f"\nSuccessfully saved to {output_file}")
        print(f"Total rows: {len(arrow_table)}")
                    
    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    download_foursquare_data()
