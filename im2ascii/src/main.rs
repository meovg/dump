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

fn main() {
    let (args, flags) = Parser::new().parse();

    let input_name = args.get("input")
        .or(args.get("i"))
        .expect("No image file argument provided");

    let output_name = args.get("output")
        .or(args.get("o"))
        .expect("No output file argument provided");

    let ratio = match args.get("resolution").or(args.get("r")) {
        Some(val) => val.parse::<u32>().unwrap(),
        None => 5
    };

    let start = time::precise_time_ns();
    let result = convert_to_ascii(input_name, output_name, ratio);
    let tval = (time::precise_time_ns() - start) / 1000000;

    if flags.contains(&"preview".to_owned()) || flags.contains(&"p".to_owned()) {
        #[allow(unused_must_use)] {
            generate_preview(&output_name.replace(".txt", ".html"), &result);
        }
    };

    println!("Completed in: {}ms", tval);
}

fn convert_to_ascii(input_name: &str, output_name: &str, ratio: u32) -> String {
    let img = image::open(&Path::new(input_name)).unwrap();
    // @%#*+=-:. 
    let ascii_set = ["@", "%", "#", "0", "+", "=", "-", ";", ".", ",", "'", " "];
    let mut art = String::new();
    let mut last_y = 0;

    let small_img = img.resize(img.width() / ratio, img.height() / ratio,
        FilterType::Nearest);

    println!("Original size: {}x{}  Result size: {}x{}",
        img.width(), img.height(), small_img.width(), small_img.height());

    for pixel in small_img.pixels() {
        if last_y != pixel.1 {
            art.push_str("\n");
            last_y = pixel.1;
        }

        let [r, g, b, a] = pixel.2.0;
        let brightness = ((r as u64 + g as u64 + b as u64 + a as u64) / 4) as f64;

        let index = ((brightness / 255.0) * (ascii_set.len() - 1) as f64).round() as usize;
        art.push_str(ascii_set[index])
    }

    match write_to_file(output_name, &art) {
        Ok(_) => (),
        _ => ()
    };

    art
}

fn write_to_file(file_name: &str, contents: &String) -> Result<String, Box<dyn Error>> {
    let mut file = File::create(file_name).unwrap();
    if true == false {
        return Err(Box::from("Is not a directory!"));
    }

    file.write_all(contents.as_bytes())?;
    Ok("Done".into())
}

fn generate_preview(file_name: &str, contents: &String) -> Result<String, Box<dyn Error>> {
    let mut src = File::open("preview.html")?;
    let mut data = String::new();
    src.read_to_string(&mut data)?;
    drop(src);  // Close the file early

    let new_data = data.replace("{templateContents}", &*contents);

    let mut dst = File::create(&file_name)?;
    dst.write(new_data.as_bytes())?;

    Ok(String::from("Done"))
}