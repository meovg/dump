use std::fs;
use std::path::Path;
use std::io::{self, Read, copy};

use clap::Parser;
use regex::Regex;
use url::Url;
use indicatif::{ProgressBar, ProgressStyle};

#[derive(Parser)]
struct Flags {
    #[clap(help="The Youtube URL of video")]
    url: String,

    #[clap(long="audio", short='a', help="Option to download and convert to audio file")]
    toaudio: bool,

    #[clap(long="format", short='f', help="Audio file format", default_value="mp3")]
    audiofmt: String,
}

fn get_video_id(video_url: &str) -> &str {
    // not going to be specific on several watch url cases
    // just the 'youtube.com/watch?v=' and 'youtu.be/' only 
    let re = Regex::new(
        r"(?:https://www\.youtube\.com/watch\?v=|https://youtu\.be/)(.*?)(&.*)*$").unwrap();

    re.captures(video_url).unwrap()
        .get(1).unwrap()
        .as_str()
}

fn get_video_info(video_url: &str) -> serde_json::Value {
    let video_id = get_video_id(video_url);

    // shamefully steal the player's access key from InnerTube
    // (found at https://github.com/tombulled/innertube/blob/main/innertube/config.py)
    let player_url = "https://youtubei.googleapis.com/youtubei/v1/player\
        ?key=AIzaSyAO_FJ2SlqU8Q4STEHLGCilw_Y9_11qcW8";

    let request = ureq::post(&player_url).send_json(ureq::json!({
        "videoId": video_id,
        "context": {
            "client": {
                "clientName": "ANDROID",
                "clientVersion": "16.02"
            }
        }
    })).unwrap();

    let respond = request.into_string().unwrap();
    let video_info: serde_json::Value = serde_json::from_str(respond.as_str()).unwrap();

    video_info
}

fn get_video_title(video_info: &serde_json::Value) -> &str {
    video_info["videoDetails"]["title"]
        .as_str()
        .unwrap_or("")
}

fn get_video_download_url(video_info: &serde_json::Value) -> &str {
    let formats = match video_info["streamingData"]["formats"].as_array() {
        Some(t) => t,
        None => return "",
    };

    let adaptive_formats = match video_info["streamingData"]["adaptiveFormats"].as_array() {
        Some(t) => t,
        None => return "",
    };

    let mut target_url = "";

    // list possible stream quality
    let vq: [&str; 6] = ["tiny", "small", "medium", "large", "hd720", "hd1080"];
    let aq: [&str; 3] = ["AUDIO_QUALITY_LOW", "AUDIO_QUALITY_MEDIUM", "AUDIO_QUALITY_HIGH"];

    let mut last_vq = "".to_string();
    let mut last_aq = "".to_string();

    fn is_better_quality(
        qualities: &[&str],
        this_quality: &str,
        last_quality: &str
    ) -> bool {
        let last_quality_index = qualities.iter()
            .position(|&x| x == last_quality).unwrap();

        // find if this_quality is in the list to the right of last_quality
        qualities.iter().skip(last_quality_index + 1).any(|&x| x == this_quality)
    }

    // find not-so-horrible stream format
    for format in formats.iter().chain(adaptive_formats) {
        let this_vq = format["quality"].as_str().unwrap_or("");
        let this_aq = format["audioQuality"].as_str().unwrap_or("");

        let is_better_aq = (last_aq.is_empty() && !this_aq.is_empty())
            || is_better_quality(&aq, this_aq, last_aq.as_str());

        let is_better_vq = (last_vq.is_empty() && !this_vq.is_empty())
            || is_better_quality(&vq, this_vq, last_vq.as_str());

        let is_same_or_better_aq = is_better_aq || (this_aq == last_aq);
        let is_same_or_better_vq = is_better_vq || (this_vq == last_vq);

        let is_better_quality = (is_better_aq && is_same_or_better_vq)
            || (is_better_vq && is_same_or_better_aq);

        if format["mimeType"].to_string().contains("video/")
            && format["quality"] != ureq::json!(null)
            && last_vq.is_empty() && !this_vq.is_empty()
            || is_better_quality
        {
            target_url = format["url"].as_str().unwrap_or("");
            last_vq = String::from(this_vq);
            last_aq = String::from(this_aq);
        }
    }

    target_url
}

struct Downloader<'a, R> {
    reader: R,
    progress_bar: &'a ProgressBar,
}

impl<R: Read> Read for Downloader<'_, R> {
    fn read(&mut self, buf: &mut[u8]) -> io::Result<usize> {
        self.reader.read(buf).map(|sz| {
            self.progress_bar.inc(sz as u64);
            sz
        })
    }
}

fn download(url: &str, filename: &str) {
    let parsed_url = Url::parse(url).unwrap();
    let init_request = ureq::get(parsed_url.as_str()).call().unwrap();

    // request download url for video size
    let size = init_request.header("Content-Length")
        .unwrap_or("0")
        .parse::<usize>().unwrap();

    // create a progress bar during download
    let bar = ProgressBar::new(size as u64);
    bar.set_style(ProgressStyle::default_bar()
        .template("[{elapsed_precise}] [{bar:40.white/black}] {bytes}/{total_bytes} ({eta})")
        .unwrap()
        .progress_chars("=>."),
    );

    let target_file = Path::new(filename);
    let mut request = ureq::get(parsed_url.as_str());

    // file exists, continue donwloading where it has left off
    if target_file.exists() {
        let file_size = target_file.metadata().unwrap().len() - 1;
        request = ureq::get(parsed_url.as_str())
            .set("Range", &format!("bytes={}-", size)).to_owned();
        bar.inc(file_size); 
    }

    let response = request.call().unwrap();

    let mut src = Downloader {
        reader: response.into_reader(),
        progress_bar: &bar,
    };

    let mut dst = fs::OpenOptions::new()
        .create(true)
        .append(true)
        .open(&target_file).unwrap();

    let _ = copy(&mut src, &mut dst).unwrap();

    bar.finish();
}

fn main() {
    let args = Flags::parse();
    let ans = get_video_id(&args.url);
    println!("{}", ans);
    let info = get_video_info(&args.url);

    let title = get_video_title(&info);
    let file_name = format!("{}.mp4", title);

    let link = get_video_download_url(&info);

    download(link, file_name.as_str());
    // println!("{:?}", link);
}