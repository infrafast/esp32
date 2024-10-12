"""
	A python script to minify the HTML/CSS/JS files of a website.
	
	Minifiers by Andrew Chilton (https://twitter.com/andychilton).
"""

import os, requests
import gzip
import shutil
import glob
import subprocess

DEST = "src\data\webapp"

def minify_json(json_file, dest, overwrite=False):
	with open(json_file, 'r', encoding='utf-8') as f:
		json = f.read()
	
	#req = requests.post('https://www.toptal.com/developers/cssminifier/api/raw', {'input': css}) 
	
	filename = os.path.basename(json_file)
	save_path = os.path.join(dest, filename)

	#save_path = css_file
	if not overwrite:
		save_path = save_path[:-5] + '.min.json'
	with open(save_path, 'w', encoding='utf-8') as f:
		#f.write(req.text)
		f.write(json)

	print(f"Minifying {json_file} to {save_path}...")

def minify_css(css_file, dest, overwrite=False):
	with open(css_file, 'r', encoding='utf-8') as f:
		css = f.read()
	
	req = requests.post('https://www.toptal.com/developers/cssminifier/api/raw', {'input': css}) 
	
	filename = os.path.basename(css_file)
	save_path = os.path.join(dest, filename)


	#save_path = css_file
	if not overwrite:
		save_path = save_path[:-4] + '.min.css'
	with open(save_path, 'w', encoding='utf-8') as f:
		f.write(req.text)
	print(f"Minifying {css_file} to {save_path}...")	

def minify_js(js_file, dest, overwrite=False):
	with open(js_file, 'r', encoding='utf-8') as f:
		js = f.read()
	
	req = requests.post('https://www.toptal.com/developers/javascript-minifier/api/raw', {'input': js})
	
	filename = os.path.basename(js_file)
	save_path = os.path.join(dest, filename)

	#save_path = js_file
	if not overwrite:
		save_path = save_path[:-3] + '.min.js'
	with open(save_path, 'w', encoding='utf-8') as f:
		f.write(req.text)
	print(f"Minifying {js_file} to {save_path}...")	

def minify_html(html_file, dest, overwrite=False):
	with open(html_file, 'r', encoding='utf-8') as f:
		html = f.read()

	req = requests.post('https://www.toptal.com/developers/html-minifier/api/raw', {'input': html})
	
	filename = os.path.basename(html_file)
	save_path = os.path.join(dest, filename)

	#save_path = html_file
	if not overwrite:
		save_path = save_path[:-5] + '.min.html'
	with open(save_path, 'w', encoding='utf-8') as f:
		f.write(req.text)
	print(f"Minifying {html_file} to {save_path}...")

def minify():
	print(r"  __  __ _       _  __ _           ")
	print(r" |  \/  (_)_ __ (_)/ _(_) ___ _ __ ")
	print(r" | |\/| | | '_ \| | |_| |/ _ \ '__|")
	print(r" | |  | | | | | | |  _| |  __/ |   ")
	print(r" |_|  |_|_|_| |_|_|_| |_|\___|_|   ")
	print("Simple Python Utility to Minify HTML/CSS/JS!", '\n')
	
	OVERWRITE = True 
	#PATH = os.getcwd()
	#print("Type the folder path with all the files. Leave blank for current directory.")
	#input_path = input("Path: ").strip()
	#if input_path:
	#	PATH = input_path
	#print()
	#print("Would you like to overwrite the files with the minified variants? Type 'yes' or 'no'")
	#input_overwrite = input("Overwrite: ").strip()
	#if input_overwrite in {'yes', 'yep', 'true', 'overwrite'}:
	#	OVERWRITE = True
	#print()

	PATH = "K:"
	#DEST = "C:\PlatformIO\esp32\src\data\webapp"	
	files_minified = 0

	for r, d, f in os.walk(PATH):
		for file_name in f:
			file_path = os.path.join(r, file_name)
			if ".min" in file_path:
				print(f"skip {file_path}...")
			elif file_name.endswith('.css'):
				minify_css(file_path, DEST, OVERWRITE)
				files_minified += 1
			elif file_name.endswith('.js'):
				minify_js(file_path, DEST, OVERWRITE)
				files_minified += 1
			elif file_name.endswith('.html'):
				minify_html(file_path, DEST, OVERWRITE)
				files_minified += 1
			elif file_name.endswith('.json'):
				minify_json(file_path, DEST, OVERWRITE)
				files_minified += 1				
		break                  
	
	print()
	print(f"Minified {files_minified} files.")

def prep_data_folder():
    scriptOutput = subprocess.call(['map_folder.bat'])
    print(scriptOutput)

    #add filetypes (extensions only) to be gzipped before uploading. Everything else will be copied directly
    filetypes_to_gzip = ['js', 'html','css','json']
    filetypes_to_ignore = ['php','db','mac','log','conf','csv']
    
    print('[COPY/GZIP DATA FILES]')

    #data_dir = env.get('PROJECTDATA_DIR')
    data_dir = DEST
    #data_src_dir = os.path.join(env.get('PROJECT_DIR'), 'data_src')
    data_src_dir = 'K:'

    all_files = glob.glob(os.path.join(data_src_dir, '*.*'))
	
    files_minified = []
    for extension in filetypes_to_gzip:
        files_minified.extend(glob.glob(os.path.join(data_src_dir, '*.' + extension)))	

    files_to_gzip = []
    for extension in filetypes_to_gzip:
        files_to_gzip.extend(glob.glob(os.path.join(data_dir, '*.' + extension)))  
    print('  files to gzip: ' + str(files_to_gzip))

    files_to_ignore = []
    for extension in filetypes_to_ignore:
        files_to_ignore.extend(glob.glob(os.path.join(data_src_dir, '*.' + extension)))
    
    files_to_copy = list(set(all_files) - set(files_to_ignore) - set(files_minified))
    print('  files to copy: ' + str(files_to_copy))

    for file in files_to_copy:
        print('  Copying file: ' + file + ' to data dir: '+data_dir)
        shutil.copy(file, data_dir)

    for file in files_to_gzip:
        print('  Gziping file: ' + file + ' to data dir: '+data_dir)		
        with open(file, 'rb') as f_in:
            with gzip.open(os.path.join(data_dir, os.path.basename(file) + '.gz'), 'wb') as f_out:
                shutil.copyfileobj(f_in, f_out)
        os.remove(file)

    print('[/COPY/GZIP DATA FILES]')

def purgefolder():
	print('******** PURGING FOLDER *********')
	files_to_delete = glob.glob(os.path.join(DEST, '*.*'))
	for file in files_to_delete:
		os.remove(file)


def workflow():
	purgefolder()
	minify()
	prep_data_folder()



workflow()