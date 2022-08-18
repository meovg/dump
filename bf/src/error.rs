use std::error;
use std::fmt;
use std::io;

#[derive(Debug)]
pub enum BfError {
	InOutput(io::Error),
	Interpret(BfiError),
}

#[derive(Debug)]
pub enum BfiError {
	EmptyCode,
	NoLoopStart,
	UnmatchedBracket,
	OutTapeUpperBound,
	OutTapeLowerBound,
}

impl error::Error for BfError {}
impl error::Error for BfiError {}

impl fmt::Display for BfError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
        	BfError::InOutput(ref suberr) => suberr.fmt(f),
        	BfError::Interpret(ref suberr) => suberr.fmt(f),
        }
    }
}

impl fmt::Display for BfiError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
    	let message = match self {
    		BfiError::EmptyCode => "Code is empty",
    		BfiError::NoLoopStart => "Loop start is not found",
    		BfiError::UnmatchedBracket => "Number of brackets is not balanced",
    		BfiError::OutTapeLowerBound => "Data pointer reaches below the memory range",
    		BfiError::OutTapeUpperBound => "Data pointer reaches above the memory range",
    	};
        write!(f, "{}", message)
    }
}

impl From<io::Error> for BfError {
    fn from(error: io::Error) -> Self {
        BfError::InOutput(error)
    }
}

impl From<BfiError> for BfError {
    fn from(error: BfiError) -> Self {
        BfError::Interpret(error)
    }
}