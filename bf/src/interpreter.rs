use crate::error::{BfError, BfiError};
use std::fs::File;
use std::io::{self, Read, Write};

#[derive(Clone, Copy)]
pub enum Command {
    Next,
    Previous,
    Increment,
    Decrement,
    Output,
    Input,
    LoopStart(usize),
    LoopEnd(usize),
}

impl From<char> for Command {
    fn from(token: char) -> Self {
        match token {
            '>' => Command::Next,
            '<' => Command::Previous,
            '+' => Command::Increment,
            '-' => Command::Decrement,
            '.' => Command::Output,
            ',' => Command::Input,
            '[' => Command::LoopStart(0),
            ']' => Command::LoopEnd(0),
            token => panic!("Unknown opcode `{}`", token),
        }
    }
}

impl Command {
    pub fn is_opcode(t: char) -> bool {
        t == '>' || t == '<' || t == '+' || t == '-'
        || t == '.'|| t == ',' || t == '[' || t == ']'
    }
}

pub struct Interpreter {
    stack: Vec<usize>,
    app: Vec<Command>,
    tape: Vec<u8>,
    instr_ptr: usize,
    data_ptr: usize,
}

impl Interpreter {
    pub fn new(tape_size: usize) -> Interpreter {
        Interpreter {
            stack: Vec::new(),
            tape: vec![0; tape_size],
            app: Vec::new(),
            instr_ptr: 0,
            data_ptr: tape_size / 2,
        }
    }

    fn parse_token(&mut self, token: char) -> Result<(), BfError> {
        if Command::is_opcode(token) {
            let index = self.app.len();
            let cmd = match token.into() {
                command @ Command::LoopStart(_) => {
                    self.stack.push(index);
                    command
                }
                Command::LoopEnd(_) => {
                    let start = match self.stack.pop() {
                        Some(value) => value,
                        None => return Err(BfiError::NoLoopStart.into()),
                    };
                    self.app[start] = Command::LoopStart(index);
                    Command::LoopEnd(start)
                }
                cmd => cmd,
            };
            self.app.push(cmd);
        }

        Ok(())
    }

    fn from_buffer(&mut self, buf: &String) -> Result<(), BfError> {
        if buf.trim().is_empty() {
            return Err(BfiError::EmptyCode.into());
        }

        for token in buf.chars() {
            self.parse_token(token)?;
        }

        match !self.stack.is_empty() {
            true => Err(BfiError::UnmatchedBracket.into()),
            false => Ok(()),
        }
    }

    pub fn from_stdin(&mut self) -> Result<(), BfError> {
        print!(">>> ");
        io::stdout().flush()?;

        let mut buf = String::new();
        io::stdin().read_line(&mut buf)?;

        self.from_buffer(&buf)
    }

    pub fn from_file(&mut self, file_name: &str) -> Result<(), BfError> {
        let mut file = File::open(&file_name)?;

        let mut buf = String::new();
        file.read_to_string(&mut buf)?;

        self.from_buffer(&buf)
    }

    pub fn run_instruction(&mut self) -> Result<(), BfError> {
        match self.app[self.instr_ptr] {
            Command::Next => {
                if self.data_ptr + 1 == self.tape.len() {
                    return Err(BfiError::OutTapeUpperBound.into());
                }
                self.data_ptr += 1;
            }
            Command::Previous => {
                if self.data_ptr == 0 {
                    return Err(BfiError::OutTapeLowerBound.into());
                }
                self.data_ptr -= 1;
            }
            Command::Increment => {
                self.tape[self.data_ptr] = self.tape[self.data_ptr].overflowing_add(1).0;
            }
            Command::Decrement => {
                self.tape[self.data_ptr] = self.tape[self.data_ptr].overflowing_sub(1).0;
            }
            Command::Output => {
                print!("{}", self.tape[self.data_ptr] as char);
                let _ = io::stdout().flush();
            }
            Command::Input => {
                self.tape[self.data_ptr] = io::stdin()
                    .bytes()
                    .next()
                    .unwrap()
                    .unwrap();
            }
            Command::LoopStart(index) => {
                if self.tape[self.data_ptr] == 0 {
                    self.instr_ptr = index;
                }
            }
            Command::LoopEnd(index) => {
                if self.tape[self.data_ptr] != 0 {
                    self.instr_ptr = index;
                }
            }
        }
        self.instr_ptr += 1;

        Ok(())
    }

    pub fn execute(&mut self) -> Result<(), BfError> {
        while self.instr_ptr < self.app.len() {
            self.run_instruction()?;
        }

        Ok(())
    }
}