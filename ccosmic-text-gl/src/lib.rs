use std::{
    ffi::{c_char, c_void, CStr, CString},
    path::PathBuf,
};

use cosmic_text::fontdb::FaceInfo;

// static mut CPP_CWD: Option<PathBuf> = None;
// fn cpp_cwd() -> &'static Path {
//     unsafe { CPP_CWD.as_ref() }.unwrap().as_path()
// }

pub struct CCosmicTextRenderer {
    gl: glow::Context,
    font_system: cosmic_text::FontSystem,
    cache: cosmic_text::SwashCache,
}

impl CCosmicTextRenderer {
    /// # Safety
    /// It's not
    pub unsafe fn new<F: FnMut(&CStr) -> *const c_void>(
        loader_fn: F,
        fonts: impl Iterator<Item = cosmic_text::fontdb::Source>,
    ) -> Self {
        let gl = glow::Context::from_loader_function_cstr(loader_fn);
        let font_system = cosmic_text::FontSystem::new_with_fonts(fonts);
        let cache = cosmic_text::SwashCache::new();

        Self {
            gl,
            font_system,
            cache,
        }
    }

    pub fn dbg(&self) -> String {
        format!(
            "{:#?}",
            self.font_system.db()
        ) //, self.font_system.db())
    }
}

#[repr(transparent)]
#[derive(Debug, Copy, Clone)]
pub struct FfiCtr(*mut c_void);
impl From<CCosmicTextRenderer> for FfiCtr {
    fn from(value: CCosmicTextRenderer) -> Self {
        let boxed = Box::new(value);
        Self(Box::into_raw(boxed) as *mut c_void)
    }
}
impl FfiCtr {
    pub fn get(self) -> &'static mut CCosmicTextRenderer {
        unsafe { &mut *(self.0 as *mut CCosmicTextRenderer) }
    }
}

impl FfiCtr {
    /// # Safety
    /// None of the pointers given will be borrowed for longer than the stack frame.
    /// All strings should be null-terminated
    #[no_mangle]
    #[export_name = "ctrInit"]
    pub unsafe extern "C" fn init(
        cwd: *const c_char,
        loader_fn: extern "C" fn(*const c_char) -> *mut c_void,
        fonts: *const *const c_char,
        fonts_len: usize,
    ) -> Self {
        cew::init().unwrap();

        // unsafe { CPP_CWD = Some(PathBuf::from(CStr::from_ptr(cwd).to_str().unwrap())) }

        let fonts = std::slice::from_raw_parts(fonts, fonts_len)
            .iter()
            .map(|&str| {
                cosmic_text::fontdb::Source::File(PathBuf::from(
                    CStr::from_ptr(str).to_str().unwrap(),
                ))
            });
        CCosmicTextRenderer::new(|req| loader_fn(req.as_ptr()), fonts).into()
    }

    /// # Safety
    /// `self` will always be initialized
    #[no_mangle]
    #[export_name = "ctrDeinit"]
    pub unsafe extern "C" fn deinit(self) {
        drop(Box::from_raw(self.0 as *mut CCosmicTextRenderer))
    }

    /// # Safety
    /// `self` will always be initialized
    /// Callee must deinitialize the resultant string
    #[no_mangle]
    #[export_name = "ctrDebug"]
    pub unsafe extern "C" fn debug(self) -> *mut c_char {
        CString::new(self.get().dbg()).unwrap().into_raw()
    }

    /// # Safety
    /// `str` should be created from ctrDebug
    #[no_mangle]
    #[export_name = "ctrDeinitDebug"]
    pub unsafe extern "C" fn deinit_debug(str: *mut c_char) {
        drop(CString::from_raw(str))
    }
}
