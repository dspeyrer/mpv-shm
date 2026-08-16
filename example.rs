use std::{fs, io, ptr, slice};

struct Shm(ptr::NonNull<*mut u8>);

impl Shm {
	fn create(name: &str, len: usize) -> io::Result<Self> {
		ptr::NonNull::new(
			unsafe { shm_shim_create(name.as_ptr(), name.len(), len) }
		).map(Self).ok_or_else(io::Error::last_os_error)
	}

	fn ptr(&self) -> *mut u8 {
		unsafe { self.0.read() }
	}
}

impl Drop for Shm {
	fn drop(&mut self) {
		unsafe { shm_shim_destroy(self.0.as_ptr()) };
	}
}

extern "C" {
	fn shm_shim_create(name: *const u8, name_len: usize, shm_len: usize) -> *mut *mut u8;
	fn shm_shim_destroy(map: *mut *mut u8);
}

fn main() {
	let mut args = std::env::args();

	let program = args.next().expect("program name should be present");

	let Some((path, shm_name)) = args.next().and_then(|path| Some((path, args.next()?))) else {
		println!("usage: {program} <file_path> <shm_name>");
		return;
	};

	let info = fs::metadata(&path).expect("file does not exist");

	let shm = Shm::create(&shm_name, info.len() as usize).expect("shm should be constructible");

	io::Read::read_exact(
		&mut fs::File::open(path).expect("file should be openable"),
		unsafe { slice::from_raw_parts_mut(shm.ptr(), info.len() as usize) }
	).expect("file should be readable");

	println!("Press enter to continue...");

	let _ = io::Read::read_exact(&mut io::stdin(), &mut [0]);
}
