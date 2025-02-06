use iced::alignment::{Horizontal, Vertical};
use iced::keyboard::{key, Event as KeyEvent, Key};
use iced::widget::image::{Handle, Image, Viewer};
use iced::widget::{self, Column, Container, Row};
use iced::window::settings::PlatformSpecific;
use iced::window::Settings;
use iced::{application, event, Event, Length::Fill, Subscription, Theme};
use std::env;
use std::fs;
use std::path::{Path, PathBuf, MAIN_SEPARATOR};

const BORDER_WIDTH: f32 = 2.0;
const WINDOW_WIDTH: f32 = 1200.0;
const WINDOW_HEIGHT: f32 = 900.0;

#[derive(Debug, Clone, Copy)]
enum Message {
    NextImage,
    PrevImage,
    SetCurrent(usize),
}

struct File {
    handle: Handle,
    path: String,
}

impl File {
    fn new(path: PathBuf, home: Option<&Path>) -> Option<Self> {
        const IMAGE_EXTS: &[&str] = &["avif", "bmp", "jpg", "jpeg", "png", "svg", "webp", "tiff"];

        if path
            .extension()
            .and_then(|e| e.to_str())
            .map(|e| !IMAGE_EXTS.contains(&e))
            .unwrap_or(true)
        {
            return None;
        };

        let canon = path.canonicalize().unwrap_or_else(|_| path.clone());
        let handle = Handle::from_path(path);
        let path = home
            .and_then(|p| canon.strip_prefix(p).ok())
            .map(|p| format!("~{}{}", MAIN_SEPARATOR, p.display()))
            .unwrap_or_else(|| canon.into_os_string().to_string_lossy().into_owned());

        Some(File { handle, path })
    }
}

struct ImageViewer {
    current: usize,
    files: Vec<File>,
}

impl Default for ImageViewer {
    fn default() -> Self {
        let home = dirs::home_dir();
        let home = home.as_deref();

        // TODO: The program loads ALL files initially, which greatly slows down
        // the process on execution.
        let mut files = vec![];
        for path in env::args_os().skip(1) {
            let path = PathBuf::from(path);
            if path.is_file() {
                if let Some(file) = File::new(path, home) {
                    files.push(file);
                }
            } else if path.is_dir() {
                files.extend(
                    fs::read_dir(&path)
                        .unwrap()
                        .flatten()
                        .map(|e| e.path())
                        .filter(|p| p.is_file())
                        .flat_map(|p| File::new(p, home)),
                );
            }
        }

        Self { current: 0, files }
    }
}

impl ImageViewer {
    fn title(&self) -> String {
        if let Some(file) = self.files.get(self.current) {
            format!("{} ({}/{})", file.path, self.current + 1, self.files.len())
        } else {
            "Image Viewer".to_string()
        }
    }

    fn update(&mut self, message: Message) {
        match message {
            Message::NextImage if self.current < self.files.len() - 1 => self.current += 1,
            Message::PrevImage if self.current > 0 => self.current -= 1,
            Message::SetCurrent(idx) if idx < self.files.len() => self.current = idx,
            _ => {}
        }
    }

    fn create_tray(&self) -> Column<Message> {
        const WIDTH: f32 = 150.0;
        const MARGIN: f32 = 10.0;
        const IMAGE_WIDTH: f32 = WIDTH - MARGIN * 2.0 - BORDER_WIDTH * 2.0;

        let start = self.current.saturating_sub(2);
        let end = (start + 5).min(self.files.len());
        let mut col = Column::new()
            .spacing(MARGIN)
            .padding(MARGIN)
            .width(WIDTH)
            .align_x(Horizontal::Center);

        for idx in start..end {
            let image = Image::new(&self.files[idx].handle);
            let image = widget::button(image).on_press(Message::SetCurrent(idx));
            col = if idx == self.current {
                col.push(image.padding(BORDER_WIDTH))
            } else {
                col.push(image.padding(0.0).width(IMAGE_WIDTH))
            };
        }
        col
    }

    fn create_viewer(&self) -> Container<Message> {
        if let Some(file) = self.files.get(self.current) {
            widget::container(Viewer::new(&file.handle).width(Fill).height(Fill)).center(Fill)
        } else {
            widget::container(widget::text("No image")).center(Fill)
        }
    }

    fn view(&self) -> Row<Message> {
        let mut row = Row::new().align_y(Vertical::Center);
        if self.files.len() > 1 {
            row = row.push(self.create_tray());
        }
        row.push(self.create_viewer())
    }

    fn subscription(&self) -> Subscription<Message> {
        event::listen_with(|event, _status, _id| match event {
            Event::Keyboard(KeyEvent::KeyPressed { key, .. }) => match key {
                Key::Named(key::Named::ArrowLeft | key::Named::ArrowUp) => Some(Message::PrevImage),
                Key::Named(key::Named::ArrowRight | key::Named::ArrowDown) => {
                    Some(Message::NextImage)
                }
                _ => None,
            },
            _ => None,
        })
    }

    fn theme(&self) -> Theme {
        Theme::Dark
    }
}

fn print_help() {
    let help_message = r#"Usage: image-viewer [directory] ([directory-2]...)
A simple image viewer built with Rust and Iced.

Arguments:
  [directory]    Path to the directory containing images.
  --help         Show this help message."#;

    println!("{}", help_message);
}

fn main() -> iced::Result {
    let mut args = env::args_os();
    args.next(); // Skips executable's name
    match args.next() {
        Some(arg) if arg == "--help" || arg == "-h" => {
            print_help();
            Ok(())
        }
        Some(_) => {
            let size = (WINDOW_WIDTH, WINDOW_HEIGHT).into();
            application(ImageViewer::title, ImageViewer::update, ImageViewer::view)
                .subscription(ImageViewer::subscription)
                .theme(ImageViewer::theme)
                .window(Settings {
                    size,
                    max_size: Some(size),
                    // Only specific to Loonix
                    platform_specific: PlatformSpecific {
                        application_id: "image-viewer".to_string(),
                        override_redirect: false,
                    },
                    ..Default::default()
                })
                .run()
        }
        None => {
            print_help();
            Ok(())
        }
    }
}
