use clap::Parser;
use indicatif::{ProgressBar, ProgressStyle};
use std::fs;
use std::io::{self, copy, Read};
use std::path::{Path, PathBuf};
use std::process::Command;
use url::Url;

#[derive(Parser)]
struct Flags {
    #[clap(help="The Youtube URL of video")]
    url: String,

    #[clap(long="name", short='o', help="Name of the downloaded file", default_value="")]
    filename: String,

    #[clap(long="audio", short='a', help="Option to download and convert to audio file")]
    toaudio: bool,

    #[clap(long="audiofmt", short='f', help="Audio file format", default_value="mp3")]
    audiofmt: String,

    #[clap(long="bestvideo", help="Choose the best video quality. Download speed may be significantly slower")]
    bestvq: bool,
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

    let video_info = match get_video_info(video_id) {
        Err(err) => {
            println!("Cannot request YouTube for video info.\n[{:?}]", err);
            return;
        }
        Ok(info) => info,
    };

    let title = (if args.filename.is_empty() {
        format!("[{}] {}", video_id, get_video_title(&video_info).unwrap())
    } else {
        args.filename
    })
    // alter video file name so it does not contain illegal characters in windows directory
    // ref: https://stackoverflow.com/questions/1976007/
    .trim()
    .replace(
        &['<', '>', '|', '\'', '\"', '\\', '/', '?', '*'][..],
        r#"--"#,
    );

    let need_best_video = !args.toaudio && args.bestvq;

    let direct_url = match get_download_url(&video_info, need_best_video) {
        Err(err) => {
            println!("There's problem getting download link. Aborting.\n[{:?}]", err);
            return;
        }
        Ok(url) => url,
    };

    if direct_url.vid.is_empty() || (need_best_video && direct_url.aud.is_empty()) {
        println!("Cannot get the direct video URL.\nThe video may be restricted, deleted, or the YouTube API has been updated ahead of this tool");
        return;
    }

    // if the user needs best video quality, download a separate adaptive video format and
    // the standard video stream for audio, then merge two tracks (somehow) into one
    // note that one cannot pick the best video quality and only need audio track
    if need_best_video {
        let video_filename = format!("{}-v.mp4", title);
        let audio_filename = format!("{}-a.mp4", title);

        // download the video track
        println!("Getting stream with video");
        let video_pathbuf = match download(direct_url.vid, &video_filename) {
            Err(err) => {
                println!("Problem with downloading occurs. Aborting\n[{:?}]", err);
                return;
            }
            Ok(vid_pb) => vid_pb,
        };

        // download the video containing audio track...
        println!("Getting stream with audio");
        let audio_pathbuf = match download(direct_url.aud, &audio_filename) {
            Err(err) => {
                println!("Problem with downloading occurs. Aborting\n[{:?}]", err);
                return;
            }
            Ok(aud_pb) => aud_pb,
        };

        let result_filename = format!("{}.mp4", title);
        let result_path = Path::new(&result_filename);

        // link them together using ffmpeg
        println!("Linking video and audio tracks into a complete video");

        if let Err(err) = link_tracks(video_pathbuf.as_path(), audio_pathbuf.as_path(), result_path) {
            println!("There's a problem linking the video tracks. Aborting\n[{:?}]", err);
            return;
        }
    } else {
        let video_filename = format!("{}.mp4", title);
        let video_pathbuf = match download(direct_url.vid, &video_filename) {
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

            if let Err(err) = save_audio(video_file, audio_file) {
                println!("Cannot delete the base video file. Please delete it yourself if not needed.\n[{:?}]", err);
                return;
            }
        }
    }
    println!("Download complete");
}

fn get_video_id(video_url: &str) -> anyhow::Result<&str> {
    // not going to be specific on several watch url cases
    // just the 'youtube.com/watch?v=' and 'youtu.be/' only
    let re = regex::Regex::new(r"(?:https://www\.youtube\.com/watch\?v=|https://youtu\.be/)(.*?)(&.*)*$")?;
    let id = re.captures(video_url).unwrap().get(1).unwrap();
    Ok(id.as_str())
}

fn get_video_info(video_id: &str) -> anyhow::Result<serde_json::Value> {
    // shamefully steal the player's access key from InnerTube
    // (found at https://github.com/tombulled/innertube/blob/main/innertube/config.py)
    let player_url = "https://youtubei.googleapis.com/youtubei/v1/player?key=AIzaSyAO_FJ2SlqU8Q4STEHLGCilw_Y9_11qcW8";
    let response = ureq::post(player_url).send_json(ureq::json!({
        "videoId": video_id,
        "context": {
            "client": {
                "clientName": "ANDROID",
                "clientVersion": "16.02"
            }
        }
    }))?;
    let response_string = response.into_string()?;
    let video_info: serde_json::Value = serde_json::from_str(response_string.as_str())?;

    Ok(video_info)
}

fn get_video_title(video_info: &serde_json::Value) -> anyhow::Result<&str> {
    Ok(video_info["videoDetails"]["title"].as_str().unwrap_or("videoplayback"))
}

// mfw anyhow crate does not support multiple return values
struct DownloadUrl<'a> {
    vid: &'a str,
    aud: &'a str,
}

fn get_download_url(video_info: &serde_json::Value, need_best_video: bool) -> anyhow::Result<DownloadUrl> {
    if need_best_video {
        Ok(DownloadUrl {
            vid: get_best_video_download_url(video_info).unwrap_or(""),
            aud: get_std_download_url(video_info).unwrap_or(""),
        })
    } else {
        Ok(DownloadUrl {
            vid: get_std_download_url(video_info).unwrap_or(""),
            aud: "",
        })
    }
}

// attempt to get the link that has finest video quality,
// adaptive stream formats do not seem to have sound, so just link a video with sound
fn get_best_video_download_url(video_info: &serde_json::Value) -> anyhow::Result<&str> {
    let formats = match video_info["streamingData"]["adaptiveFormats"].as_array() {
        Some(t) => t,
        None => return Ok(""),
    };

    let vq: [&str; 8] = ["tiny", "small", "medium", "large", "hd720", "hd1080", "hd1440", "hd2160"];

    let mut last_vq = "".to_string();
    let mut target_url = "";

    for format in formats.iter() {
        if !format["mimeType"].to_string().contains("video/") {
            continue;
        }
        let this_vq = format["quality"].as_str().unwrap_or("");
        let improved = (last_vq.is_empty() && !this_vq.is_empty()) || is_better_quality(&vq, this_vq, last_vq.as_str());

        if improved {
            target_url = format["url"].as_str().unwrap_or("");
            last_vq = String::from(this_vq);
        }
    }

    Ok(target_url)
}

fn get_std_download_url(video_info: &serde_json::Value) -> anyhow::Result<&str> {
    let formats = match video_info["streamingData"]["formats"].as_array() {
        Some(t) => t,
        None => return Ok(""),
    };

    let mut target_url = "";

    let vq: [&str; 6] = ["tiny", "small", "medium", "large", "hd720", "hd1080"];
    let aq: [&str; 3] = ["AUDIO_QUALITY_LOW", "AUDIO_QUALITY_MEDIUM", "AUDIO_QUALITY_HIGH"];

    let mut last_vq = "".to_string();
    let mut last_aq = "".to_string();

    for format in formats.iter() {
        if !format["mimeType"].to_string().contains("video/") {
            continue;
        }
        let this_aq = format["audioQuality"].as_str().unwrap_or("");
        let this_vq = format["quality"].as_str().unwrap_or("");

        let is_better_aq = (last_aq.is_empty() && !this_aq.is_empty()) || is_better_quality(&aq, this_aq, last_aq.as_str());
        let is_better_vq = (last_vq.is_empty() && !this_vq.is_empty()) || is_better_quality(&vq, this_vq, last_vq.as_str());
        let is_same_or_better_aq = is_better_aq || (this_aq == last_aq);
        let is_same_or_better_vq = is_better_vq || (this_vq == last_vq);
        let improved = (is_better_aq && is_same_or_better_vq) || (is_better_vq && is_same_or_better_aq);

        if improved {
            target_url = format["url"].as_str().unwrap_or("");
            last_vq = String::from(this_vq);
            last_aq = String::from(this_aq);
        }
    }

    Ok(target_url)
}

fn is_better_quality(qualities: &[&str], this_quality: &str, last_quality: &str) -> bool {
    let last_quality_index = qualities.iter().position(|&x| x == last_quality).unwrap();

    // find if this_quality is in the list to the right of last_quality
    qualities
        .iter()
        .skip(last_quality_index + 1)
        .any(|&x| x == this_quality)
}

// wrap the progress bar around a struct that implements Read trait
// to advance the progress bar as the data is being read
struct Downloader<'a, R> {
    reader: R,
    progress_bar: &'a ProgressBar,
}

impl<R: Read> Read for Downloader<'_, R> {
    fn read(&mut self, buf: &mut [u8]) -> io::Result<usize> {
        self.reader.read(buf).map(|sz| {
            self.progress_bar.inc(sz as u64);
            sz
        })
    }
}

fn download(url: &str, filename: &str) -> anyhow::Result<PathBuf> {
    let parsed_url = Url::parse(url)?;
    let init_request = ureq::get(parsed_url.as_str()).call()?;

    // request download url for video size
    let video_size = init_request.header("Content-Length").unwrap_or("0").parse::<u64>()?;

    // create a progress bar during download
    let bar = ProgressBar::new(video_size);
    bar.set_style(ProgressStyle::default_bar()
        .template("[{elapsed_precise}] [{bar:40.white/black}] {bytes}/{total_bytes} ({eta})")
        .unwrap()
        .progress_chars("=>."),
    );

    let video_file = Path::new(filename);
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

    let response = ureq::get(parsed_url.as_str()).call()?;

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

fn save_audio(input_file: &Path, output_file: &Path) -> anyhow::Result<()> {
    Command::new("ffmpeg")
        .arg("-i").arg(input_file)
        .arg("-ar").arg("44100")
        .arg("-vn")
        .arg("-loglevel").arg("quiet")
        .arg(output_file)
        .output()
        .expect("FFmpeg is missing, please install to convert to audio file");

    fs::remove_file(&input_file)?;
    Ok(())
}

// ref: https://stackoverflow.com/questions/11779490/
fn link_tracks(input_video: &Path, input_audio: &Path, output_file: &Path) -> anyhow::Result<()> {
    Command::new("ffmpeg")
        .arg("-i").arg(input_video)
        .arg("-i").arg(input_audio)
        .arg("-map").arg("0:v")
        .arg("-map").arg("1:a")
        .arg("-c:v").arg("copy")
        .arg("-shortest")
        .arg("-loglevel").arg("quiet")
        .arg(output_file)
        .output()
        .expect("FFmpeg is missing, please install to link video tracks");

    fs::remove_file(&input_video)?;
    fs::remove_file(&input_audio)?;
    Ok(())
}
