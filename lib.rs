use std::{io, ptr};

/// A shared memory map.
pub struct Shm(ptr::NonNull<*mut u8>);

impl Shm {
	/// Create a shared memory object with the specified name and size.
	pub fn create(name: &str, len: usize) -> io::Result<Self> {
		ptr::NonNull::new(unsafe { shm_shim_create(name.as_ptr(), name.len(), len) })
			.map(Self)
			.ok_or_else(io::Error::last_os_error)
	}

	/// Get a pointer to the base of the mapped shared memory object.
	pub fn ptr(&self) -> *mut u8 {
		unsafe { self.0.read() }
	}
}

impl Drop for Shm {
	fn drop(&mut self) {
		unsafe { shm_shim_destroy(self.0.as_ptr()) };
	}
}

unsafe extern "C" {
	fn shm_shim_create(name: *const u8, name_len: usize, shm_len: usize) -> *mut *mut u8;
	fn shm_shim_destroy(map: *mut *mut u8);
}
