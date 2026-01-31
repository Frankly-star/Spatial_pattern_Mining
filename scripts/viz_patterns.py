import matplotlib.pyplot as plt
import numpy as np
import os
import re

def parse_patterns(file_path):
    patterns = []
    
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read().strip()
    
    # Split by blocks (assuming specific format)
    # The format seems to be:
    # 6 313 327 (Header, optional)
    # 1 1 3 (Rect info: W H N)
    # ID X Y KW (Object 1) 
    # ID X Y KW (Object 2)
    # ...
    
    lines = content.split('\n')
    
    # Try to parse blocks
    current_pattern = None
    
    # Common Foursquare Category IDs Mapping (Generic)
    kw_names = {
        6: 'Home (private)',
        313: 'Shopping Mall',
        327: 'Restaurant',
        158: 'Gym',
        7: 'Clothing Store',
        299: 'Department Store',
        312: 'Metro Station',
        336: 'Convenience Store',
        348: 'Residential',
        258: 'Coffee Shop',
        69: 'Office',
        140: 'Park'
    }
    
    i = 0
    # Skip first line if it's just keyword list (e.g. "6 313 327")
    if len(lines) > 0 and ' ' in lines[0] and '.' not in lines[0]:
        pass

    last_freq = 0
    while i < len(lines):
        line = lines[i].strip()
        if not line: 
            i+=1
            continue
            
        parts = line.split()
        
        # Check for Frequency line
        if len(parts) == 2 and parts[0] == 'Frequency:':
            try:
                last_freq = int(parts[1])
            except ValueError:
                pass
            i += 1
            continue

        # Check if it is a header pattern line "W H N"
        if len(parts) == 3 and '.' not in parts[2]: # N is integer
            try:
                # Attempt to parse as W H N
                w = float(parts[0])
                h = float(parts[1])
                n = int(parts[2])
                
                # It's a header
                current_pattern = {
                    'size': (w, h),
                    'objects': [],
                    'frequency': last_freq
                }
                
                # Next n lines are objects
                for k in range(n):
                    i += 1
                    if i >= len(lines): break
                    obj_line = lines[i].strip().split()
                    if len(obj_line) >= 4:
                        obj_id = int(obj_line[0])
                        x = float(obj_line[1])
                        y = float(obj_line[2])
                        kw = int(obj_line[3])
                        current_pattern['objects'].append({
                            'id': obj_id, 'x': x, 'y': y, 'kw': kw, 
                            'name': kw_names.get(kw, str(kw))
                        })
                
                patterns.append(current_pattern)
            except ValueError:
                pass
        i += 1
        
    return patterns

def plot_patterns(patterns, output_img):
    n = len(patterns)
    if n == 0:
        print("No patterns found.")
        return

    cols = min(n, 5)
    rows = (n + cols - 1) // cols
    
    fig, axes = plt.subplots(rows, cols, figsize=(4*cols, 4*rows))
    
    # If fig has single axes, it's not an array unless we force it, but numpy array of axes is tricky 
    # when n=1 and subplots returns a single Axes object, NOT an array.
    if n == 1:
        # If rows=1, cols=1, plt returns a single ax object
        if not isinstance(axes, (list, np.ndarray)):
            axes = [axes]
    else:
        axes = axes.flatten()
    
    # Define colors for keywords dynamically based on what's present
    all_kws = set()
    for p in patterns:
        for o in p['objects']:
            all_kws.add(o['kw'])
    
    # Color palette
    palette = ['blue', 'red', 'green', 'orange', 'purple', 'cyan', 'brown', 'magenta']
    markers_list = ['o', 's', '^', 'D', 'v', 'p', '*', 'h']
    
    colors = {}
    markers = {}
    
    sorted_kws = sorted(list(all_kws))
    for i, kw in enumerate(sorted_kws):
        colors[kw] = palette[i % len(palette)]
        markers[kw] = markers_list[i % len(markers_list)]
    
    for idx, pat in enumerate(patterns):
        ax = axes[idx]
        w, h = pat['size']
        
        # Draw box
        ax.set_xlim(0, max(w, 1.05)) # Assuming normalized approx 0-1 usually
        ax.set_ylim(0, max(h, 1.05))
        ax.set_aspect('equal')
        
        # Plot objects
        for obj in pat['objects']:
            kw = obj['kw']
            c = colors.get(kw, 'black')
            m = markers.get(kw, 'x')
            ax.scatter(obj['x'], obj['y'], c=c, marker=m, s=100, label=obj['name'])
            # ax.text(obj['x'], obj['y'], str(obj['kw']), fontsize=9)
            
            # Draw line to center? Or just show distribution.
        
        # Handle duplicates in legend
        handles, labels = ax.get_legend_handles_labels()
        by_label = dict(zip(labels, handles))
        if idx == 0: # Only legend on first? or all?
            ax.legend(by_label.values(), by_label.keys(), loc='upper right', fontsize='small')
            
        freq = pat.get('frequency', '?')
        ax.set_title(f"Pattern {idx+1} (Freq: {freq})")
        ax.grid(True, linestyle='--', alpha=0.5)

    # Hide empty subplots
    for k in range(idx+1, len(axes)):
        axes[k].axis('off')

    plt.tight_layout()
    plt.savefig(output_img)
    print(f"Saved visualization to {output_img}")

if __name__ == "__main__":
    script_dir = os.path.dirname(os.path.abspath(__file__))
    input_file = os.path.join(script_dir, 'output_patterns.txt')
    output_img = os.path.join(script_dir, '..', 'picture', 'patterns_viz.png')
    
    if not os.path.exists(os.path.dirname(output_img)):
        os.makedirs(os.path.dirname(output_img))
        
    pats = parse_patterns(input_file)
    print(f"Parsed {len(pats)} patterns.")
    plot_patterns(pats, output_img)
