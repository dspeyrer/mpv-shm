fn main() {
	cc::Build::new().file("shmshim.c").compile("shmshim");
}
