use core::ffi::c_void;

use roc_std::RocStr;

#[no_mangle]
pub unsafe extern "C" fn roc_alloc(size: usize, _alignment: u32) -> *mut c_void {
    libc::malloc(size)
}

#[no_mangle]
pub unsafe extern "C" fn roc_realloc(
    c_ptr: *mut c_void,
    new_size: usize,
    _old_size: usize,
    _alignment: u32,
) -> *mut c_void {
    libc::realloc(c_ptr, new_size)
}

#[no_mangle]
pub unsafe extern "C" fn roc_dealloc(c_ptr: *mut c_void, _alignment: u32) {
    libc::free(c_ptr);
}

#[no_mangle]
pub unsafe extern "C" fn roc_panic(msg: *mut RocStr, tag_id: u32) {
    match tag_id {
        0 => {
            eprintln!("Roc standard library hit a panic: {}", &*msg);
        }
        1 => {
            eprintln!("Application hit a panic: {}", &*msg);
        }
        _ => unreachable!(),
    }
    std::process::exit(1);
}

#[no_mangle]
pub unsafe extern "C" fn roc_dbg(loc: *mut RocStr, msg: *mut RocStr, src: *mut RocStr) {
    eprintln!("[{}] {} = {}", &*loc, &*src, &*msg);
}

#[no_mangle]
pub unsafe extern "C" fn roc_memset(dst: *mut c_void, c: i32, n: usize) -> *mut c_void {
    libc::memset(dst, c, n)
}

#[cfg(unix)]
#[no_mangle]
pub unsafe extern "C" fn roc_getppid() -> libc::pid_t {
    libc::getppid()
}

#[cfg(unix)]
#[no_mangle]
pub unsafe extern "C" fn roc_mmap(
    addr: *mut libc::c_void,
    len: libc::size_t,
    prot: libc::c_int,
    flags: libc::c_int,
    fd: libc::c_int,
    offset: libc::off_t,
) -> *mut libc::c_void {
    libc::mmap(addr, len, prot, flags, fd, offset)
}

#[cfg(unix)]
#[no_mangle]
pub unsafe extern "C" fn roc_shm_open(
    name: *const libc::c_char,
    oflag: libc::c_int,
    mode: libc::mode_t,
) -> libc::c_int {
    libc::shm_open(name, oflag, mode as libc::c_uint)
}

#[repr(C)]
pub struct Captures {
    _data: (),
    _marker: core::marker::PhantomData<(*mut u8, core::marker::PhantomPinned)>,
}

#[repr(C)]
pub struct Msg {
    _data: (),
    _marker: core::marker::PhantomData<(*mut u8, core::marker::PhantomPinned)>,
}

pub fn call_roc_setup_callback() -> *const Captures {
    extern "C" {
        #[link_name = "roc__setup_callback_for_host_1_exposed_generic"]
        fn caller(_: *mut Captures, _: u64);

        #[link_name = "roc__setup_callback_for_host_1_exposed_size"]
        fn size() -> usize;
    }

    unsafe {
        let captures = roc_alloc(size(), 0) as *mut Captures;
        caller(captures, 0);
        captures as *const Captures
    }
}

pub fn call_roc_callback(captures: *const Captures) -> *mut *mut Msg {
    extern "C" {
        #[link_name = "roc__setup_callback_for_host_0_caller"]
        fn caller(_: *const Captures, _: *const i32, _: *mut *mut Msg);

        #[link_name = "roc__setup_callback_for_host_0_result_size"]
        fn size() -> isize;
    }

    unsafe {
        let ret = roc_alloc(size() as usize, 0) as *mut *mut Msg;
        caller(captures, &0, ret);
        ret as *mut *mut Msg
    }
}

pub fn call_roc_handle_callback(msg: *mut *mut Msg) {
    extern "C" {
        #[link_name = "roc__handle_callback_for_host_1_exposed"]
        fn caller(_: *mut Msg);
    }

    unsafe { caller(*msg) };
}

// Effects
#[no_mangle]
pub unsafe extern "C" fn roc_fx_print(name: *const RocStr) {
    println!("{}", *name);
}
