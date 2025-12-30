#!/usr/bin/env python3
"""
WebApp Build Script for ESP-IDF Embedded Files
Combines webapp.html, webapp.css, and webapp.js into a single minified HTML file

Commands:
  install  - Install required Python dependencies
  compile  - Compile and minify webapp files
"""

import sys
import os
import subprocess


def install_dependencies():
    """Install Python dependencies from requirements.txt"""
    script_dir = os.path.dirname(os.path.abspath(__file__))
    requirements_file = os.path.join(script_dir, 'requirements.txt')
    
    if not os.path.exists(requirements_file):
        print(f"❌ Requirements file not found: {requirements_file}")
        sys.exit(1)
    
    print("📦 Installing Python dependencies...")
    print(f"   From: {requirements_file}")
    print()
    
    # Upgrade pip first
    print("🔧 Upgrading pip...")
    result = subprocess.run([sys.executable, '-m', 'pip', 'install', '--upgrade', 'pip'],
                          capture_output=True, text=True)
    if result.returncode != 0:
        print(f"⚠️  Warning: Failed to upgrade pip")
        print(result.stderr)
    
    # Install requirements
    print("🔧 Installing requirements...")
    result = subprocess.run([sys.executable, '-m', 'pip', 'install', '-r', requirements_file],
                          capture_output=True, text=True)
    
    if result.returncode != 0:
        print(f"❌ Failed to install dependencies:")
        print(result.stderr)
        sys.exit(1)
    
    # Verify installation
    print("🔍 Verifying installation...")
    result = subprocess.run([sys.executable, '-c', 'import htmlmin, rjsmin, rcssmin'],
                          capture_output=True, text=True)
    
    if result.returncode != 0:
        print(f"❌ Verification failed - dependencies not properly installed")
        sys.exit(1)
    
    print("✅ Dependencies installed successfully")
    sys.exit(0)


def compile_webapp(html_file, css_file, js_file, output_file):
    """Compile and minify webapp files"""
    # Check and auto-install dependencies if needed
    try:
        import htmlmin
        import rjsmin
        import rcssmin
    except ImportError:
        print("📦 Installing required dependencies...")
        install_dependencies()
        # Try importing again after installation
        try:
            import htmlmin
            import rjsmin
            import rcssmin
        except ImportError as e:
            print(f"❌ Error: Failed to import after installation: {e}")
            sys.exit(1)
    
    # Redefine minification functions locally
    def minify_css_local(css_file):
        if not os.path.exists(css_file):
            print(f"❌ CSS file not found: {css_file}")
            sys.exit(1)
        with open(css_file, 'r', encoding='utf-8') as f:
            css = f.read()
        minified = rcssmin.cssmin(css)
        return minified, len(css), len(minified)
    
    def minify_js_local(js_file):
        if not os.path.exists(js_file):
            print(f"❌ JavaScript file not found: {js_file}")
            sys.exit(1)
        with open(js_file, 'r', encoding='utf-8') as f:
            js = f.read()
        minified = rjsmin.jsmin(js)
        return minified, len(js), len(minified)
    
    print(f"🔧 Building webapp from:")
    print(f"   HTML: {os.path.basename(html_file)}")
    print(f"   CSS:  {os.path.basename(css_file)}")
    print(f"   JS:   {os.path.basename(js_file)}")
    print()
    
    # Minify CSS
    print("📦 Minifying CSS...")
    css_minified, css_orig, css_min_size = minify_css_local(css_file)
    css_saved = css_orig - css_min_size
    print(f"   Original: {css_orig:,} bytes")
    print(f"   Minified: {css_min_size:,} bytes")
    print(f"   Saved: {css_saved:,} bytes ({(css_saved/css_orig*100):.1f}%)")
    print()
    
    # Minify JavaScript
    print("📦 Minifying JavaScript...")
    js_minified, js_orig, js_min_size = minify_js_local(js_file)
    js_saved = js_orig - js_min_size
    print(f"   Original: {js_orig:,} bytes")
    print(f"   Minified: {js_min_size:,} bytes")
    print(f"   Saved: {js_saved:,} bytes ({(js_saved/js_orig*100):.1f}%)")
    print()
    
    # Read HTML template
    if not os.path.exists(html_file):
        print(f"❌ HTML file not found: {html_file}")
        sys.exit(1)
    
    with open(html_file, 'r', encoding='utf-8') as f:
        html = f.read()
    
    # Replace placeholders
    html = html.replace('/* CSS_PLACEHOLDER */', css_minified)
    html = html.replace('/* JS_PLACEHOLDER */', js_minified)
    
    html_size_before_minify = len(html)
    
    # Minify final HTML
    print("📦 Minifying HTML...")
    html_minified = htmlmin.minify(
        html,
        remove_comments=True,
        remove_empty_space=True,
        remove_all_empty_space=False,
        reduce_boolean_attributes=True,
        remove_optional_attribute_quotes=False,
        keep_pre=True
    )
    
    html_min_size = len(html_minified)
    html_saved = html_size_before_minify - html_min_size
    print(f"   Combined (before): {html_size_before_minify:,} bytes")
    print(f"   Minified (after):  {html_min_size:,} bytes")
    print(f"   Saved: {html_saved:,} bytes ({(html_saved/html_size_before_minify*100):.1f}%)")
    print()
    
    # Write output
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(html_minified)
    
    # Calculate total savings
    total_orig = css_orig + js_orig + (html_size_before_minify - css_orig - js_orig)
    total_final = html_min_size
    total_saved = total_orig - total_final
    
    print("=" * 60)
    print(f"✅ Build complete: {os.path.basename(output_file)}")
    print(f"   Total original:  {total_orig:,} bytes")
    print(f"   Total minified:  {total_final:,} bytes")
    print(f"   Total saved:     {total_saved:,} bytes ({(total_saved/total_orig*100):.1f}%)")
    print("=" * 60)


def main():
    if len(sys.argv) < 2:
        print("Usage:")
        print(f"  {sys.argv[0]} install")
        print(f"  {sys.argv[0]} compile <webapp.html> <webapp.css> <webapp.js> <output.html>")
        sys.exit(1)
    
    command = sys.argv[1]
    
    if command == 'install':
        install_dependencies()
    
    elif command == 'compile':
        if len(sys.argv) != 6:
            print("Usage: build_webapp.py compile <webapp.html> <webapp.css> <webapp.js> <output.html>")
            sys.exit(1)
        
        html_file = sys.argv[2]
        css_file = sys.argv[3]
        js_file = sys.argv[4]
        output_file = sys.argv[5]
        
        try:
            compile_webapp(html_file, css_file, js_file, output_file)
        except Exception as e:
            print(f"❌ Error building webapp: {e}")
            import traceback
            traceback.print_exc()
            sys.exit(1)
    
    else:
        print(f"❌ Unknown command: {command}")
        print("Available commands: install, compile")
        sys.exit(1)


if __name__ == '__main__':
    main()
