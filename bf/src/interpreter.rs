use std::fs::File;
use std::io::{self, Read, Write};
use crate::error::{BfError, BfiError};

#[derive(Clone, Copy)]
pub enum Opcode {
    Next,
    Previous,
    Increment,
    Decrement,
    Output,
    Input,
    LoopStart(usize),
    LoopEnd(usize),
    None,
}

impl From<char> for Opcode {
    fn from(token: char) -> Self {
        match token {
            '>' => Opcode::Next,
            '<' => Opcode::Previous,
            '+' => Opcode::Increment,
            '-' => Opcode::Decrement,
            '.' => Opcode::Output,
            ',' => Opcode::Input,
            '[' => Opcode::LoopStart(0),
            ']' => Opcode::LoopEnd(0),
            _ => Opcode::None,
        }
    }
}

impl Opcode {
    pub fn is_some(self) -> bool {
        match self {
            Opcode::None => false,
            _ => true,
        }
    }
}

pub struct Interpreter {
    stack: Vec<usize>,
    app: Vec<Opcode>,
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
        let op: Opcode = token.into();

        if op.is_some() {
            let index = self.app.len();
            let instruction = match op {
                cmd @ Opcode::LoopStart(_) => {
                    self.stack.push(index);
                    cmd
                }
                Opcode::LoopEnd(_) => {
                    let start = match self.stack.pop() {
                        Some(value) => value,
                        None => return Err(BfiError::NoLoopStart.into()),
                    };
                    self.app[start] = Opcode::LoopStart(index);
                    Opcode::LoopEnd(start)
                }
                op => op,
            };
            self.app.push(instruction);
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

    fn run_instruction(&mut self) -> Result<(), BfError> {
        match self.app[self.instr_ptr] {
            Opcode::Next => {
                if self.data_ptr + 1 == self.tape.len() {
                    return Err(BfiError::OutTapeUpperBound.into());
                }
                self.data_ptr += 1;
            }
            Opcode::Previous => {
                if self.data_ptr == 0 {
                    return Err(BfiError::OutTapeLowerBound.into());
                }
                self.data_ptr -= 1;
            }
            Opcode::Increment => {
                self.tape[self.data_ptr] = self.tape[self.data_ptr].overflowing_add(1).0;
            }
            Opcode::Decrement => {
                self.tape[self.data_ptr] = self.tape[self.data_ptr].overflowing_sub(1).0;
            }
            Opcode::Output => {
                print!("{}", self.tape[self.data_ptr] as char);
                io::stdout().flush()?;
            }
            Opcode::Input => {
                self.tape[self.data_ptr] = io::stdin()
                    .bytes().next()
                    .unwrap().unwrap();
            }
            Opcode::LoopStart(index) => {
                if self.tape[self.data_ptr] == 0 {
                    self.instr_ptr = index;
                }
            }
            Opcode::LoopEnd(index) => {
                if self.tape[self.data_ptr] != 0 {
                    self.instr_ptr = index;
                }
            }
            _ => ()
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
