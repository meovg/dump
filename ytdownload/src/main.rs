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

fn main() {
    let args = Flags::parse();

    let video_id = match fetcher::get_video_id(&args.url) {
        Err(err) => {
            println!("Cannot parse the YouTube video ID: [{:?}]", err);
            return; // i'm scared of panics
        }
        Ok(id) => id,
    };

    let video_info = match fetcher::get_video_info(&video_id) {
        Err(err) => {
            println!("Cannot request YouTube for video info: [{:?}]", err);
            return;
        }
        Ok(info) => info,
    };

    let title = fetcher::get_video_title(&video_info).unwrap();

    // alter video file name so it does not contain illegal characters in windows directory
    // ref: https://stackoverflow.com/questions/1976007/
    let video_filename = format!("[{}] {}.mp4",
        video_id,
        title.trim().replace(&['<', '>',  '|', '\'', '\"', '\\', '/', '?', '*'][..], r#"--"#)
    );

    let video_link = match fetcher::get_video_download_url(&video_info) {
        Err(err) => {
            println!("Cannot find a suitable video stream to download: [{:?}]", err);
            return;
        }
        Ok("") => {
            println!(
                "Cannot get the direct video URL.\nThe video may be restricted, deleted, \
                or the YouTube API has been updated beyond this method of getting URL.");
            return;
        }
        Ok(link) => link,
    };

    let video_pathbuf = match download::download(video_link, &video_filename) {
        Err(err) => {
            println!("Problem occured. Video download is halted: [{:?}]", err);
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

        cmd::save_audio(video_file, audio_file);
        fs::remove_file(&video_file).unwrap();
    }

    println!("Download complete");
}