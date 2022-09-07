use regex::Regex;

pub fn get_video_id(video_url: &str) -> anyhow::Result<&str> {
    // not going to be specific on several watch url cases
    // just the 'youtube.com/watch?v=' and 'youtu.be/' only 
    let re = Regex::new(
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
    Ok(video_info["videoDetails"]["title"].as_str().unwrap_or(""))
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

    Ok(target_url)
}