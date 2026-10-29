"""
	A python script to minify the HTML/CSS/JS files of a website.
	
	Minifiers by Andrew Chilton (https://twitter.com/andychilton).
"""

import os, requests
import gzip
import shutil
import glob
import subprocess

# 🔹 Chemin absolu du dossier où se trouve ce script
script_dir = os.path.dirname(os.path.abspath(__file__))
# 🔹 Dossier racine du projet (2 niveaux au-dessus, par exemple)
#    adapte ce niveau si ton script est dans un autre sous-dossier
project_dir = os.path.abspath(os.path.join(script_dir, ".."))
# 🔹 Construction du chemin cible

DEST = os.path.join(project_dir, "esp32", "data", "webapp")
# Editable web source lives in its separate webGUI checkout. Override this
# path with WEBGUI_SRC when the checkout is mounted elsewhere.
SRC = os.environ.get("WEBGUI_SRC", "/Volumes/web/demo")

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
	files_minified = 0

	for r, d, f in os.walk(SRC):		
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
    #scriptOutput = subprocess.call(['map_folder.bat'])
    #print(scriptOutput)

    #add filetypes (extensions only) to be gzipped before uploading. Everything else will be copied directly
    filetypes_to_gzip = ['ttf','js', 'html','css','json']
    #gzip_exceptions = ['interfaces.json']
    gzip_exceptions = []
    minified_files = ['js', 'html','css','json']
    filetypes_to_ignore = ['php','db','mac','log','conf','csv']
    
    print('[COPY/GZIP DATA FILES]')

    #data_dir = env.get('PROJECTDATA_DIR')
    data_dir = DEST
    

    all_files = glob.glob(os.path.join(SRC, '*.*'))

    files_minified = []
    for extension in minified_files:
        files_minified.extend(glob.glob(os.path.join(SRC, '*.' + extension)))	

    files_to_ignore = []
    for extension in filetypes_to_ignore:
        files_to_ignore.extend(glob.glob(os.path.join(SRC, '*.' + extension)))
    
    files_to_copy = list(set(all_files) - set(files_to_ignore) - set(files_minified))
    print('  files to copy: ' + str(files_to_copy))

    for filename in gzip_exceptions:
        fName = os.path.join(SRC, filename)
        print('    Add exception =>'+fName)    
        files_to_copy.append(fName)  		

    for file in files_to_copy:
        print('  Copying file: ' + file + ' to data dir: '+DEST)
        shutil.copy(file, DEST)

    files_to_gzip = []
    for extension in filetypes_to_gzip:		
        files = glob.glob(os.path.join(DEST, '*.' + extension))
        # Filtrer les exceptions
        files = [f for f in files if os.path.basename(f) not in gzip_exceptions]
        # Ajouter à la liste finale
        files_to_gzip.extend(files)

    print('  files to gzip: ' + str(files_to_gzip))

    for file in files_to_gzip:
        print('  Gziping file: ' + file + ' to data dir: '+data_dir)		
        with open(file, 'rb') as f_in:
            with gzip.open(os.path.join(DEST, os.path.basename(file) + '.gz'), 'wb') as f_out:
                shutil.copyfileobj(f_in, f_out)
        os.remove(file)


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
