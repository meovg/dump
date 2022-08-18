extern crate time;
extern crate image;

mod cmd;

use std::error::Error;
use std::fs::File;
use std::io::prelude::*;
use std::path::Path;
use image::GenericImageView;
use image::imageops::FilterType;
use cmd::Parser;

fn write_to_file(output_name: &str, contents: &String) -> Result<(), Box<dyn Error>> {
    let mut file = File::create(output_name)?;

    if true == false {
        return Err(Box::from("Is not a directory!"));
    }

    file.write_all(contents.as_bytes())?;
    Ok(())
}

fn convert_to_ascii(input_name: &str, scale: f64) -> String {
    let img = image::open(&Path::new(input_name)).unwrap();

    let ascii_set = ["$", "%", "W", "*", "h", "d", "w", "O", "L", "U", "z", "u", "r",
        "t", "|", "1", "[", "-", "~", "i", "I", "'", ".", " "];

    let mut art = String::new();
    let mut last_y = 0;

    let new_width = (img.width() as f64 / scale).round() as u32;
    let new_height = (img.height() as f64 / scale).round() as u32;
    let small_img = img.resize(new_width, new_height, FilterType::Nearest);

    println!("Original size: {}x{},  Result size: {}x{}",
        img.width(), img.height(), small_img.width(), small_img.height());

    for pixel in small_img.pixels() {
        if last_y != pixel.1 {
            art.push_str("\n");
            last_y = pixel.1;
        }

        let [r, g, b, _a] = pixel.2.0;
        let luminance = (0.2126 * r as f64 + 0.7152 * g as f64 + 0.0722 * b as f64) as f64;
        let index = ((luminance / 255.0) * (ascii_set.len() - 1) as f64).round() as usize;
        art.push_str(ascii_set[index]);
    }
    art
}

fn main() {
    let args = Parser::new().parse().0;

    let input_name = args.get("input")
        .or(args.get("i"))
        .expect("No image file argument provided");

    let output_name = args.get("output")
        .or(args.get("o"))
        .expect("No output file argument provided");

    let scale = match args.get("scale") {
        Some(val) => val.parse::<f64>().unwrap(),
        None => 4.0
    };

    let start = time::precise_time_ns();
    let result = convert_to_ascii(input_name, scale);
    let _ = write_to_file(output_name, &result);

    let tval = (time::precise_time_ns() - start) / 1000000;
    println!("Completed in: {}ms", tval);
}