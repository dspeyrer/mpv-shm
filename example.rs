use std::{fs, io, slice};

use shm::Shm;

fn main() {
	let mut args = std::env::args();

	let program = args.next().expect("program name should be present");

	let Some((path, shm_name)) = args.next().and_then(|path| Some((path, args.next()?))) else {
		println!("usage: {program} <file_path> <shm_name>");
		return;
	};

	let info = fs::metadata(&path).expect("file does not exist");

	let shm = Shm::create(&shm_name, info.len() as usize).expect("shm should be constructible");

	io::Read::read_exact(&mut fs::File::open(path).expect("file should be openable"), unsafe {
		slice::from_raw_parts_mut(shm.ptr(), info.len() as usize)
	})
	.expect("file should be readable");

	println!("Press enter to continue...");

	let _ = io::Read::read_exact(&mut io::stdin(), &mut [0]);
}
