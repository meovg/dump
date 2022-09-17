use clap::Parser;
use std::io::{self, Read, copy};
use std::path::{Path, PathBuf};
use std::fs;
use std::process::Command;
use url::Url;
use indicatif::{ProgressBar, ProgressStyle};

#[derive(Parser)]
struct Flags {
    #[clap(help="The Youtube URL of video")]
    url: String,

    #[clap(long="name", short='o', help="Name of the downloaded file", default_value="")]
    filename: String,

    #[clap(long="audio", short='a', help="Option to download and convert to audio file")]
    toaudio: bool,

    #[clap(long="format", short='f', help="Audio file format", default_value="mp3")]
    audiofmt: String,
}

fn main() {
    let args = Flags::parse();

    let video_id = match get_video_id(&args.url) {
        Err(err) => {
            println!("Cannot parse the YouTube video ID:\n[{:?}]", err);
            return; // i'm scared of panics
        }
        Ok(id) => id,
    };

    let video_info = match get_video_info(&video_id) {
        Err(err) => {
            println!("Cannot request YouTube for video info.\n[{:?}]", err);
            return;
        }
        Ok(info) => info,
    };

    let title = if args.filename.is_empty() {
        format!("[{}] {}", video_id, get_video_title(&video_info).unwrap())
    } else {
        args.filename
    };

    // alter video file name so it does not contain illegal characters in windows directory
    // ref: https://stackoverflow.com/questions/1976007/
    let video_filename = format!("{}.mp4",
        title.trim().replace(&['<', '>',  '|', '\'', '\"', '\\', '/', '?', '*'][..], r#"--"#)
    );

    let video_link = match get_video_download_url(&video_info) {
        Err(err) => {
            println!("There's problem getting download link. Aborted.\n[{:?}]", err);
            return;
        }
        Ok("") => {
            println!(
                "Cannot get the direct video URL.\nThe video may be restricted, deleted, \
                or the YouTube API has been updated beyond this method of getting URL."
            );
            return;
        }
        Ok(link) => link,
    };

    let video_pathbuf = match download(video_link, &video_filename) {
        Err(err) => {
            println!("Problem occured. Video download is halted\n[{:?}]", err);
            return;
        }
        Ok(vid_pb) => vid_pb,
    };

    let video_file = video_pathbuf.as_path();

    // option to download audio file
    // use ffmpeg to convert to audio file and delete the original video file
    if args.toaudio {
        println!("Converting to audio...");

        let audio_filename = video_filename.trim().replace("mp4", &args.audiofmt);
        let audio_file = Path::new(&audio_filename);

        save_audio(video_file, audio_file);
        match fs::remove_file(&video_file) {
            Err(err) => {
                println!("Cannot delete the base video file.\n\
                    Please delete it yourself if not needed.\n[{:?}]", err);
            }
            Ok(_) => (),
        }
    }

    println!("Download complete");
}

pub fn get_video_id(video_url: &str) -> anyhow::Result<&str> {
    // not going to be specific on several watch url cases
    // just the 'youtube.com/watch?v=' and 'youtu.be/' only 
    let re = regex::Regex::new(
        r"(?:https://www\.youtube\.com/watch\?v=|https://youtu\.be/)(.*?)(&.*)*$")?;

    let id = re.captures(video_url).unwrap().get(1).unwrap();
    
    Ok(id.as_str())
}

pub fn get_video_info(video_id: &str) -> anyhow::Result<serde_json::Value> {
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
    }))?;

    let respond = request.into_string()?;
    let video_info: serde_json::Value = serde_json::from_str(respond.as_str())?;

    Ok(video_info)
}

pub fn get_video_title(video_info: &serde_json::Value) -> anyhow::Result<&str> {
    Ok(video_info["videoDetails"]["title"].as_str().unwrap_or("videoplayback"))
}

pub fn get_video_download_url(video_info: &serde_json::Value) -> anyhow::Result<&str> {
    let formats = match video_info["streamingData"]["formats"].as_array() {
        Some(t) => t,
        None => return Ok(""),
    };

    let adaptive_formats = match video_info["streamingData"]["adaptiveFormats"].as_array() {
        Some(t) => t,
        None => return Ok(""),
    };

    let mut target_url = "";

    // list possible stream quality
    let vq: [&str; 6] = ["tiny", "small", "medium", "large", "hd720", "hd1080"];
    let aq: [&str; 3] = ["AUDIO_QUALITY_LOW", "AUDIO_QUALITY_MEDIUM", "AUDIO_QUALITY_HIGH"];

    let mut last_vq = "".to_string();
    let mut last_aq = "".to_string();

    fn is_better_quality(qualities: &[&str], this_quality: &str, last_quality: &str) -> bool {
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

    Ok(target_url)
}

// wrap the progress bar around a struct that implements Read trait
// to advance the progress bar as the data is being read
pub struct Downloader<'a, R> {
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

pub fn download(url: &str, filename: &str) -> anyhow::Result<PathBuf> {
    let parsed_url = Url::parse(url)?;
    let init_request = ureq::get(parsed_url.as_str()).call()?;

    // request download url for video size
    let video_size = init_request.header("Content-Length")
        .unwrap_or("0")
        .parse::<u64>()?;

    // create a progress bar during download
    let bar = ProgressBar::new(video_size);
    bar.set_style(ProgressStyle::default_bar()
        .template("[{elapsed_precise}] [{bar:40.white/black}] {bytes}/{total_bytes} ({eta})")
        .unwrap()
        .progress_chars("=>."),
    );

    let video_file = Path::new(filename);

    let request = ureq::get(parsed_url.as_str());

    // skip when the file has been downloaded, or remove the existing file
    if video_file.exists() {
        let file_size = video_file.metadata()?.len();
        if video_size == file_size {
            println!("Video has already been downloaded");
            return Ok(video_file.to_path_buf());
        } else {
            fs::remove_file(&video_file)?;
        }
    }

    let response = request.call()?;

    let mut src = Downloader {
        reader: response.into_reader(),
        progress_bar: &bar,
    };

    let mut dst = fs::OpenOptions::new()
        .create(true)
        .append(true)
        .open(&video_file)?;

    println!("Downloading the video as {:?}", filename);
    let _ = copy(&mut src, &mut dst)?;
    bar.finish();

    Ok(video_file.to_path_buf())
}

pub fn save_audio(input_file: &Path, output_file: &Path) {
    Command::new("ffmpeg")
        .arg("-i").arg(input_file)
        .arg("-ar").arg("44100")
        .arg("-vn")
        .arg("-loglevel").arg("quiet")
        .arg(output_file)
        .output()
        .expect("FFmpeg is missing, please install to convert to audio file");
}