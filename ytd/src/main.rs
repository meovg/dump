use clap::Parser;
use regex::Regex;

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
                "clientName": "TVHTML5",
                "clientVersion": "7.20211231"
            }
        }
    })).unwrap();

    let respond = request.into_string().unwrap();
    let video_info: serde_json::Value = serde_json::from_str(respond.as_str()).unwrap();

    video_info
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

    let vq: [&str; 6] = ["tiny", "small", "medium", "large", "hd720", "hd1080"];
    let aq: [&str; 3] = ["AUDIO_QUALITY_LOW", "AUDIO_QUALITY_MEDIUM", "AUDIO_QUALITY_HIGH"];

    let mut last_vq = "".to_string();
    let mut last_aq = "".to_string();

    for format in formats.iter().chain(adaptive_formats) {
        let this_vq = format["quality"].as_str().unwrap_or("");
        let this_aq = format["audioQuality"].as_str().unwrap_or("");

        let is_better_aq = (last_aq.is_empty() && !this_aq.is_empty())
            || (last_aq == aq[0] && (this_aq == aq[1] || this_aq == aq[2]))
            || (last_aq == aq[1] && this_aq == aq[2]);

        let is_better_vq = (last_vq.is_empty() && !this_vq.is_empty())
            || (last_vq == vq[0] && (
                this_vq == vq[1]
                || this_vq == vq[2]
                || this_vq == vq[3]
                || this_vq == vq[4]
                || this_vq == vq[5]))
            || (last_vq == vq[1] && (
                this_vq == vq[2] || this_vq == vq[3] || this_vq == vq[4] || this_vq == vq[5]))
            || (last_vq == vq[2] && (this_vq == vq[3] || this_vq == vq[4] || this_vq == vq[5]))
            || (last_vq == vq[3] && (this_vq == vq[4] || this_vq == vq[5]))
            || (last_vq == vq[4] && this_vq == vq[5]);

        let is_same_or_better_aq = is_better_aq || (this_aq == last_aq);
        let is_same_or_better_vq = is_better_vq || (this_vq == last_vq);

        let is_better_quality = (is_better_aq && is_same_or_better_vq)
            || (is_better_vq && is_same_or_better_aq);

        if format["mimeType"].to_string().contains("video/")
            && format["quality"] != ureq::json!(null)
            && last_vq.is_empty() && !this_vq.is_empty()
            || is_better_quality
        {
            target_url = format["url"].as_str().unwrap();
            last_vq = String::from(this_vq);
            last_aq = String::from(this_aq);
        }
    }

    target_url
}

fn main() {
    let args = Flags::parse();
    let ans = get_video_id(&args.url);
    println!("{}", ans);
    let info = get_video_info(&args.url);
    let link = get_video_download_url(&info);
    println!("{:?}", link);
}