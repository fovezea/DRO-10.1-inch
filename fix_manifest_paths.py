#!/usr/bin/env python3
"""
Script to remove 'path' fields from repository_info sections in idf_component.yml files.
This prevents configuration errors when moving ESP-IDF projects.

Usage:
    python fix_manifest_paths.py [--backup] [--dry-run] [--verbose]

Options:
    --backup     Create backup files before modifying
    --dry-run    Show what would be changed without making changes
    --verbose    Show detailed output
"""

import os
import re
import glob
import argparse
import shutil
from datetime import datetime

def create_backup(file_path):
    """Create a backup of the file with timestamp."""
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    backup_path = f"{file_path}.backup_{timestamp}"
    shutil.copy2(file_path, backup_path)
    return backup_path

def fix_manifest_file(file_path, create_backup_files=False, dry_run=False, verbose=False):
    """Remove path field from repository_info section in a manifest file."""
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # More comprehensive pattern to match repository_info section with path field
        # This handles various indentation styles and formats
        pattern = r'(repository_info:\s*\n(?:\s+[^p][^a][^t][^h][^\n]*\n)*)\s+path:\s*[^\n]*\n'
        
        # Check if there's a path field to remove
        if re.search(pattern, content):
            if verbose:
                print(f"Found path field in: {file_path}")
            
            if dry_run:
                print(f"Would fix: {file_path}")
                return True
            
            # Create backup if requested
            if create_backup_files:
                backup_path = create_backup(file_path)
                if verbose:
                    print(f"Created backup: {backup_path}")
            
            # Remove the path line
            new_content = re.sub(pattern, r'\1', content)
            
            # Only write if content actually changed
            if new_content != content:
                with open(file_path, 'w', encoding='utf-8') as f:
                    f.write(new_content)
                
                print(f"Fixed: {file_path}")
                return True
            else:
                if verbose:
                    print(f"No changes needed for: {file_path}")
                return False
        else:
            if verbose:
                print(f"No path field found in: {file_path}")
            return False
            
    except Exception as e:
        print(f"Error processing {file_path}: {e}")
        return False

def find_manifest_files():
    """Find all idf_component.yml files in the project."""
    manifest_files = []
    
    # Common locations for ESP-IDF projects
    search_patterns = [
        "**/idf_component.yml",
        "components/**/idf_component.yml",
        "main/idf_component.yml"
    ]
    
    for pattern in search_patterns:
        files = glob.glob(pattern, recursive=True)
        manifest_files.extend(files)
    
    # Remove duplicates while preserving order
    seen = set()
    unique_files = []
    for file_path in manifest_files:
        if file_path not in seen:
            seen.add(file_path)
            unique_files.append(file_path)
    
    return unique_files

def main():
    """Main function to find and fix all idf_component.yml files in the project."""
    parser = argparse.ArgumentParser(
        description="Remove path fields from repository_info sections in idf_component.yml files",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
    python fix_manifest_paths.py                    # Fix all files
    python fix_manifest_paths.py --backup           # Create backups before fixing
    python fix_manifest_paths.py --dry-run          # Show what would be changed
    python fix_manifest_paths.py --verbose          # Show detailed output
        """
    )
    
    parser.add_argument('--backup', action='store_true',
                       help='Create backup files before modifying')
    parser.add_argument('--dry-run', action='store_true',
                       help='Show what would be changed without making changes')
    parser.add_argument('--verbose', action='store_true',
                       help='Show detailed output')
    
    args = parser.parse_args()
    
    # Find all idf_component.yml files
    manifest_files = find_manifest_files()
    
    if not manifest_files:
        print("No idf_component.yml files found in current directory")
        return
    
    print(f"Found {len(manifest_files)} manifest files")
    
    if args.dry_run:
        print("DRY RUN MODE - No files will be modified")
    
    fixed_count = 0
    for file_path in manifest_files:
        if fix_manifest_file(file_path, args.backup, args.dry_run, args.verbose):
            fixed_count += 1
    
    print(f"\nSummary:")
    print(f"  Files processed: {len(manifest_files)}")
    print(f"  Files fixed: {fixed_count}")
    
    if fixed_count > 0 and not args.dry_run:
        print(f"\nNext steps:")
        print(f"  1. Run 'idf.py reconfigure' to verify the fix")
        print(f"  2. Test your build with 'idf.py build'")

if __name__ == "__main__":
    main() 