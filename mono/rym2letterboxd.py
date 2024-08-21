# Tool to convert your RateYourMusic film catalog (obtained using the "Export
# your film catalog" button under the profile page) into a format that can be
# imported to your Letterboxd account
# Details about importing film log to your Letterboxd can be found here:
# https://letterboxd.com/about/importing-data/

# Known issues:
# - RYM added a Cloudflare protection layer on top of it so it's nearly impossible
#   to scrape the site.
# - The film title in data exported from RYM does not contain grammatical article
#   like a/an/the or das, un, les..., which may cause Letterboxd to look for a
#   completely different film (for example 'Boot' instead of 'Das Boot')

import argparse
import csv
import sys

def filter_data(filename):
    with open(filename, mode='r', encoding='utf-8') as file:
        reader = csv.DictReader(file)

        filtered_data = ["Title,Year,Rating10,WatchedDate"]
        for row in reader:
            # Check if Ownership is not 'w'
            if row['Ownership'] == 'w':
                continue

            # Extract the required columns
            title = row['Title']
            if "," in title:
                title = "\"" + title + "\""
            year = row['Release_Date'].split(u'/', 1)[0]
            rating = row['Rating']
            watched_date = row['Catalog_Date'].replace(u'/', u'-')
            filtered_data.append(",".join([title, year, rating, watched_date]))

        return filtered_data

def write_to_file(filename, data):
    with open(filename, mode='w', encoding='utf-8') as file:
        file.write('\n'.join(data))

def write_to_stdout(data):
    sys.stdout.write('\n'.join(data))

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Convert RYM film data to format defined by Letterboxd')
    parser.add_argument('filename', help='RYM film data in SVG')
    parser.add_argument('-o', '--output', type=str, default=None,
                        help='Output filename in SVG. Skip this flag if you prefer standard output')

    args = parser.parse_args()
    data = filter_data(args.filename)
    if args.output:
        write_to_file(args.output, data)
    else:
        write_to_stdout(data)