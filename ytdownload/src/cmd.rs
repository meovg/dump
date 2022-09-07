use std::path::Path;
use std::process::Command;

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