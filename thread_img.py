import argparse
import os
from bs4 import BeautifulSoup
from urllib import request, parse

# todo: expand the options to save as original filename
parser = argparse.ArgumentParser(description="buggy 4chan thread image downloader")
parser.add_argument('url', nargs=1, help='URL of the thread')
args = parser.parse_args()

# todo: validate URL

# default folder base for downloads
download_base = os.path.join(os.getenv('userprofile'), 'Downloads', '4chan')

# get board name and thread ID from URL under the format https://boards.4chan(nel).org/[board]/thread/[thread_id]#[non-op_post_id]
thread_url = args.url[0].strip()
board = thread_url.split('/')[3]
thread_id = thread_url.split('/')[5].split('#')[0]

# path of folder containing thread's images 
dest_folder_path = os.path.join(download_base, board, thread_id)
if not os.path.exists(dest_folder_path):
    os.makedirs(dest_folder_path)

# scrape thread and parse HTML response
print(f'Scraping /{board}/ thread #{thread_id}')
response = request.urlopen(thread_url)
soup = BeautifulSoup(response, 'html.parser')

# traverse through image elements, extract image URL, and other infos
print('Done, finding images')
print(f'Downloaded images located at {dest_folder_path}')
files = soup.find_all(class_="file")

for i, file in enumerate(files):
    href = file.find(class_="fileText").find("a")
    img_url = "http:" + href["href"]
    img_info = file.find(class_="mFileInfo").get_text()
    img_orig_name = href.get_text()
    print(f"[{i+1}/{len(files)}] {img_url} ({img_info}) (Original: {img_orig_name})")

    # download image
    file_name = os.path.basename(parse.urlparse(img_url).path)
    dest_path = os.path.join(dest_folder_path, file_name)
    if not os.path.isfile(dest_path):
        request.urlretrieve(img_url, dest_path)
        print(f"[{i+1}/{len(files)}] {file_name} downloaded")
    else:
        print(f"[{i+1}/{len(files)}] {file_name} skipped. File already exists")
print("Done!")

# todo: refactor this mess