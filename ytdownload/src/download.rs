use std::io::{self, Read, copy};
use std::path::{Path, PathBuf};
use std::fs;
use std::process::Command;
use url::Url;
use indicatif::{ProgressBar, ProgressStyle};

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