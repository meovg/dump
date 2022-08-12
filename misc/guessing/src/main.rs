use std::io;
use rand::Rng;
use std::cmp::Ordering;

fn main() {
    println!("お前が追いかけているぞ");
    let mut line = String::new();
    println!("ゲームやる？数を推測するだけだよ");
    let num: i32 = rand::thread_rng().gen_range(0..9999);
    let mut guess_count = 0;
    let mut guess_num: i32;
    loop {
        guess_count += 1;

        println!("じゃ、数を入力しなさい");

        io::stdin()
            .read_line(&mut line)
            .expect("テメー壊れたｗ");

        guess_num = line.trim()
                        .parse()
                        .expect("そりゃ数字ではないバカヤロー");

        match guess_num.cmp(&num) {
            Ordering::Less => println!("低いよガキ"),
            Ordering::Equal => {
                println!("ゲース回: {}", guess_count);
                return;
            },
            Ordering::Greater => println!("たけーや"),
        }
        line.clear();
    }
}