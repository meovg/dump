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

fn write_to_file(output_name: &str, contents: &String) -> Result<String, Box<dyn Error>> {
    let mut file = File::create(output_name)?;
    if true == false {
        return Err(Box::from("Is not a directory!"));
    }

    file.write_all(contents.as_bytes())?;
    Ok("Done".into())
}

fn convert_to_ascii(input_name: &str, scale: u32) -> String {
    let img = image::open(&Path::new(input_name)).unwrap();
    let ascii_set = ["@", "%", "#", "0", "+", "=", "-", ";", ".", ",", "'", " "];
    let mut art = String::new();
    let mut last_y = 0;

    let new_width = img.width() / scale;
    let new_height = img.height() / scale;

    let small_img = img.resize(new_width, new_height, FilterType::Nearest);

    println!("Original size: {}x{},  Result size: {}x{}",
        img.width(), img.height(), small_img.width(), small_img.height());

    for pixel in small_img.pixels() {
        if last_y != pixel.1 {
            art.push_str("\n");
            last_y = pixel.1;
        }

        let [r, g, b, a] = pixel.2.0;
        let brightness = ((r as u64 + g as u64 + b as u64 + a as u64) / 4) as f64;

        let index = ((brightness / 255.0) * (ascii_set.len() - 1) as f64).round() as usize;
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
        Some(val) => val.parse::<u32>().unwrap(),
        None => 5,
    };

    let start = time::precise_time_ns();
    let result = convert_to_ascii(input_name, scale);
    #[allow(unused_must_use)] {
        write_to_file(output_name, &result);
    }
    let tval = (time::precise_time_ns() - start) / 1000000;

    println!("Completed in: {}ms", tval);
}