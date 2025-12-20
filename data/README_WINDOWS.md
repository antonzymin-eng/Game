# Map Generation on Windows

## Quick Start

### Prerequisites

1. **Install Python**
   - Download: https://www.python.org/downloads/
   - **IMPORTANT**: During installation, check ✅ "Add Python to PATH"
   - Recommended: Python 3.11 or newer

2. **Verify Python Installation**
   ```powershell
   python --version
   ```
   Should show: `Python 3.x.x`

---

## Running Map Generation

### Automatic (Batch Script)

```powershell
cd C:\Users\azymi\Documents\Game\data
.\regenerate_maps.bat
```

This runs the full pipeline automatically.

---

### Manual (Step by Step)

If the batch script doesn't work, run each step individually:

```powershell
cd C:\Users\azymi\Documents\Game\data

# Step 1: Download missing GeoJSON data
python download_missing_geojson.py

# Step 2: Generate country maps
python generate_europe_maps.py

# Step 3: Calculate adjacencies
python calculate_adjacencies.py

# Step 4: Combine into Europe map
python update_combined_europe_with_adjacencies.py
```

---

## Common Issues

### "Python was not found"

**Solution:**
1. Install Python from https://www.python.org/downloads/
2. During installation, check ✅ "Add Python to PATH"
3. Restart PowerShell after installation

---

### "cannot be loaded because running scripts is disabled"

If you see an error about execution policy:

```powershell
# Allow script execution (run as Administrator)
Set-ExecutionPolicy RemoteSigned -Scope CurrentUser
```

---

### Missing Dependencies

If you get "No module named 'fiona'" or similar:

```powershell
# Install dependencies
pip install fiona

# Alternative if above fails
pip install urllib3
```

**Note**: `fiona` is optional. The script will work without it using basic methods.

---

## Using WSL (Alternative)

If you have Windows Subsystem for Linux (WSL):

```bash
# In WSL terminal
cd /mnt/c/Users/azymi/Documents/Game/data
./regenerate_maps.sh
```

This lets you use the Linux versions of the scripts.

---

## Manual Download (If Scripts Fail)

If the automatic download doesn't work, download GeoJSON manually:

1. **Natural Earth Data**
   - Visit: https://www.naturalearthdata.com/downloads/10m-cultural-vectors/
   - Download: "Admin 1 – States, Provinces" (10m scale)
   - Extract to: `C:\Users\azymi\Documents\Game\data\maps\geojson_source\`

2. **Extract for specific countries**
   - Use QGIS (free GIS software) or
   - Use online converters to filter by country

---

## File Paths on Windows

- **Your game folder**: `C:\Users\azymi\Documents\Game`
- **Data folder**: `C:\Users\azymi\Documents\Game\data`
- **Maps folder**: `C:\Users\azymi\Documents\Game\data\maps`
- **GeoJSON source**: `C:\Users\azymi\Documents\Game\data\maps\geojson_source`

---

## Verification

After running regeneration:

```powershell
# Check if combined map exists
dir maps\map_europe_combined.json

# Check file size (should be > 100 KB)
```

If you have `jq` installed (optional):
```powershell
# Install jq
choco install jq

# Then check statistics
jq ".map_region.provinces | length" maps\map_europe_combined.json
```

---

## PowerShell Commands Quick Reference

```powershell
# Navigate to game data folder
cd C:\Users\azymi\Documents\Game\data

# List files
dir

# Run Python script
python script_name.py

# Run batch file
.\regenerate_maps.bat

# Check Python version
python --version

# Install Python package
pip install package_name
```

---

## Support

If you encounter issues:

1. Check Python is installed: `python --version`
2. Check you're in the right folder: `cd C:\Users\azymi\Documents\Game\data`
3. Try running scripts individually instead of batch file
4. Check logs for specific error messages

See `MAP_GENERATION_GUIDE.md` for detailed troubleshooting.

---

## Linux vs Windows Path Notes

| Linux Path | Windows Path |
|------------|--------------|
| `/home/user/Game/data` | `C:\Users\azymi\Documents\Game\data` |
| `./script.sh` | `.\script.bat` |
| `python3` | `python` |
| `/` (forward slash) | `\` (backslash) |

On Windows:
- Use `\` for paths in commands
- Use `python` instead of `python3`
- Use `.bat` files instead of `.sh` files
- PowerShell is different from bash
