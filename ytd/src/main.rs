use clap::Parser;
use std::path::Path;
use std::fs;

mod download;
mod fetcher;
mod cmd;

#[derive(Parser)]
struct Flags {
    #[clap(help="The Youtube URL of video")]
    url: String,

    #[clap(long="audio", short='a', help="Option to download and convert to audio file")]
    toaudio: bool,

    #[clap(long="format", short='f', help="Audio file format", default_value="mp3")]
    audiofmt: String,
}

fn main() -> anyhow::Result<()> {
    let args = Flags::parse();

    let id = fetcher::get_video_id(&args.url)?;
    let info = fetcher::get_video_info(&id)?;
    let title = fetcher::get_video_title(&info)?;

    let video_filename = format!("[{}] {}.mp4",
        id,
        title.trim().replace(&['|', '\'', '\"', '\\', '/', '?', '*'][..], r#"--"#)
    );

    let link = fetcher::get_video_download_url(&info)?;
    let video_pathbuf = download::download(link, &video_filename)?;

    let video_file = video_pathbuf.as_path();

    // option to download audio file
    // use ffmpeg to convert to audio file and delete the original video file
    if args.toaudio {
        println!("Converting to audio...");

        let audio_filename = video_filename.trim().replace("mp4", &args.audiofmt);
        let audio_file = Path::new(&audio_filename);

        cmd::save_audio(video_file, audio_file);
        fs::remove_file(&video_file)?;
    }

    println!("Download complete");

    Ok(())
}