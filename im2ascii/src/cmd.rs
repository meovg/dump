use std::env;
use std::collections::HashMap;

type ArgumentList = HashMap<String, String>;

type Flags = Vec<String>;

// Parser structure
pub struct Parser {
    prefixes: Option<Vec<String>>
}

// Parser implementation
impl Parser {
    // Constructor
    // @return Parser
    pub fn new() -> Parser {
        Parser {
            prefixes: None
        }
    }

    // Sets the prefix for the arguments
    // param prefixes Vec<String>
    // @return Parser
    #[allow(dead_code)]
    pub fn strict_prefix(&mut self, prefixes: Vec<String>) -> &Self {
        self.prefixes = Some(prefixes);
        self
    }

    // Parses the given cmd arguments
    // @return Tuple(ArgumentList, Flags)
    pub fn parse(&self) -> (ArgumentList, Flags) {
        let mut args_list = ArgumentList::new();
        let mut flags = Flags::new();
        let mut arg_name = String::new();
        let mut i = 0;

        let arg_prefixes = match self.prefixes {
            Some(ref prefixes) => prefixes.clone(),
            None => vec!("-".to_owned(), "--".to_owned())
        };

        for (index, argument) in env::args().collect::<Vec<String>>().iter().enumerate() {
            for prefix in &arg_prefixes {
                if argument.starts_with(&prefix[..]) {
                    if arg_name != "" {
                        flags.push(arg_name.to_owned())
                    }
                    arg_name = argument[prefix.len()..argument.len()].to_owned();
                    i = index + 1;
                } else if index == i && arg_name != "" {
                    args_list.insert(arg_name.to_owned(), argument.to_owned());
                    arg_name = "".to_owned();
                }
            }
        }

        if arg_name != "" {
            flags.push(arg_name.to_owned());
        }

        (args_list, flags)
    }
}