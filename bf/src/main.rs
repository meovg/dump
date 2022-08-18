pub mod cmd;
pub mod error;
pub mod interpreter;

use cmd::Parser;
use interpreter::Interpreter;

fn main() {
    let args = Parser::new().parse().0;

    let tape_size = match args.get("memsize").or(args.get("sz")) {
        Some(val) => val.parse::<usize>().unwrap(),
        None => 65_536,
    };

    let mut bfi = Interpreter::new(tape_size);

    let import_stage = match args.get("file").or(args.get("f")) {
        Some(file_name) => bfi.from_file(file_name),
        None => bfi.from_stdin(),
    };

    if let Err(err) = import_stage {
        panic!("ERROR: {:?}", err);
    }

    let execute_stage = bfi.execute();

    if let Err(err) = execute_stage {
        panic!("ERROR: {:?}", err);
    }
}