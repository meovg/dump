# this is an error-prone script to download all media of a 4chan thread
# at this point, i'm still assuming that anons are kind enough to not post screamers

import argparse
import os
import time
import urllib.error
import http.client
from bs4 import BeautifulSoup
from urllib import request, parse

# todo: expand the options to save as original filename
def main():
    parser = argparse.ArgumentParser(description="buggy 4chan thread image downloader")
    parser.add_argument('url', nargs=1, help='URL of the thread')
    args = parser.parse_args()

    url, board, thread_id = validate_url(args.url[0].strip())
    dest = get_dest_folder_path(board, thread_id)
    print(f"Downloads stored at {dest}")
    download_images(url, dest)


# todo: validate URL
def validate_url(base_url):
    url = base_url
    board = url.split('/')[3]
    thread_id = url.split('/')[5].split('#')[0]
    return url, board, thread_id


# default folder base for downloads
def get_dest_folder_path(board, thread_id):
    download_base = os.path.join(os.getenv('userprofile'), 'Downloads', '4chan')
    dest_folder_path = os.path.join(download_base, board, thread_id)
    if not os.path.exists(dest_folder_path):
        os.makedirs(dest_folder_path)
    return dest_folder_path


def load_thread(url):
    # scrape thread and parse HTML response
    req = request.Request(url, headers={'User-Agent': 'Deez Nuts'})
    return request.urlopen(req).read().decode('utf-8')


# traverse through image elements, extract image URL, and other infos
def get_media_details(res):
    media = []
    doc = BeautifulSoup(res, 'html.parser')
    files = doc.find_all(class_="file")
    for i, file in enumerate(files):
        # print(file)
        anc = file.find(class_="fileText").find("a")
        url = "http:" + anc['href']
        info = file.find(class_="mFileInfo").text
        try:
            orig_name = anc['title']
        except KeyError:
            orig_name = anc.text
        media.append((url, info, orig_name))
        # print(f"[{i+1}/{len(files)}] {img_url} ({img_info}) (Original: {img_orig_name})")
    return media


def download_images(thread_url, dest_folder_path):
    while True:
        try:
            response = load_thread(thread_url)
            media_details = get_media_details(response)
            total_cnt = len(media_details)

            for i, m in enumerate(media_details):
                img_url, info, orig_name = m
                print(f"[{i+1}/{total_cnt}] {img_url} ({info}) (Original: {orig_name})")

                file_name = os.path.basename(parse.urlparse(img_url).path)

                dest_path = os.path.join(dest_folder_path, file_name)
                if not os.path.isfile(dest_path):
                    request.urlretrieve(img_url, dest_path)
                    print(f"[{i+1}/{total_cnt}] {file_name} downloaded")
                else:
                    print(f"[{i+1}/{total_cnt}] {file_name} skipped. File already exists")
            print("Done!")
            break
        except urllib.error.HTTPError:
            time.sleep(10)
            try:
                load_thread(thread_url)
            except urllib.error.HTTPError:
                print(f"Couldn't access thread")
                break
            continue
        except (urllib.error.URLError, http.client.BadStatusLine, http.client.IncompleteRead):
            print(f"-ACK!")
            raise


# todo: refactor this mess
if __name__ == "__main__":
    main()