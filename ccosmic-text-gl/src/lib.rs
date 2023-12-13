#![feature(array_chunks)]

use std::{
    ffi::{c_char, c_void, CStr, CString},
    path::PathBuf,
};

use cosmic_text::{Attrs, Metrics};
use glow::HasContext;

// static mut CPP_CWD: Option<PathBuf> = None;
// fn cpp_cwd() -> &'static Path {
//     unsafe { CPP_CWD.as_ref() }.unwrap().as_path()
// }

pub struct CCosmicTextRenderer {
    gl: glow::Context,
    font_system: cosmic_text::FontSystem,
    cache: cosmic_text::SwashCache,
    atlas: guillotiere::AtlasAllocator,
    hashmap: std::collections::HashMap<
        cosmic_text::CacheKey,
        (Option<guillotiere::Allocation>, cosmic_text::Placement),
    >,
    framebuffer: glow::NativeFramebuffer,
    texture: glow::NativeTexture,
}
impl Drop for CCosmicTextRenderer {
    fn drop(&mut self) {
        unsafe { self.gl.delete_texture(self.texture) };
        unsafe { self.gl.delete_framebuffer(self.framebuffer) };
    }
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

        let atlas = guillotiere::AtlasAllocator::new(guillotiere::Size::new(4096, 4096));
        dbg!(atlas.size());
        let texture = gl.create_framebuffer().unwrap();
        gl.bind_framebuffer(glow::FRAMEBUFFER, Some(texture));
        let tex = gl.create_texture().unwrap();
        gl.bind_texture(glow::TEXTURE_2D, Some(tex));
        gl.tex_image_2d(
            glow::TEXTURE_2D,
            0,
            glow::RGBA as i32,
            4096,
            4096,
            0,
            glow::RGBA,
            glow::UNSIGNED_BYTE,
            Some(&[0xFF; 4 * 4096 * 4096]),
        );
        gl.bind_texture(glow::TEXTURE_2D, None);
        gl.framebuffer_texture_2d(
            glow::FRAMEBUFFER,
            glow::COLOR_ATTACHMENT0,
            glow::TEXTURE_2D,
            Some(tex),
            0,
        );
        gl.bind_framebuffer(glow::FRAMEBUFFER, None);

        Self {
            texture: tex,
            framebuffer: texture,
            gl,
            font_system,
            cache,
            atlas,
            hashmap: std::collections::HashMap::new(),
        }
    }

    pub fn dbg(&self) -> String {
        format!("{:#?}", self.font_system.db())
    }
}

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct CtrBuffer(*mut c_void);
impl CtrBuffer {
    pub unsafe fn new(ctr: FfiCtr, value: cosmic_text::Buffer) -> Self {
        let boxed = Box::new(value);
        Self(Box::into_raw(boxed) as *mut c_void)
    }

    pub fn get(self) -> &'static mut cosmic_text::Buffer {
        unsafe { &mut *(self.0 as *mut _) }
    }
}

#[repr(C)]
pub struct CtrRect {
    min: [i32; 2],
    max: [i32; 2],
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
        unsafe { &mut *(self.0 as *mut _) }
    }
}

fn mulcol(a: u8, b: u8) -> u8 {
    ((a as f32 * b as f32) / (225.)) as u8
}

#[repr(i32)]
#[derive(Clone, Copy)]
pub enum CtrAlign {
    CENTER = 1,
    JUSTIFIED = 2,
    LEFT = 3,
    RIGHT = 4,
    END = 5,
}
impl From<CtrAlign> for cosmic_text::Align {
    fn from(value: CtrAlign) -> Self {
        match value {
            CtrAlign::CENTER => Self::Center,
            CtrAlign::JUSTIFIED => Self::Justified,
            CtrAlign::LEFT => Self::Left,
            CtrAlign::RIGHT => Self::Right,
            CtrAlign::END => Self::End,
        }
    }
}

impl FfiCtr {
    /// # Safety
    /// None of the pointers given will be borrowed for longer than the stack frame.
    /// All strings should be null-terminated
    #[no_mangle]
    #[export_name = "ctrInit"]
    pub unsafe extern "C" fn init(
        _cwd: *const c_char,
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

    #[no_mangle]
    #[export_name = "ctrCreateBuffer"]
    pub extern "C" fn create_buffer(self, font_size: f32, line_height: f32) -> CtrBuffer {
        unsafe {
            CtrBuffer::new(
                self,
                cosmic_text::Buffer::new(
                    &mut self.get().font_system,
                    Metrics::new(font_size, line_height),
                ),
            )
        }
    }

    /// # Safety
    /// text_ptr should be proper UTF-8
    #[no_mangle]
    #[export_name = "ctrSetBufferContents"]
    pub unsafe extern "C" fn set_buffer_contents(
        self,
        buf: CtrBuffer,
        text_ptr: *const u8,
        text_len: usize,
    ) {
        let str =
            std::str::from_utf8(unsafe { std::slice::from_raw_parts(text_ptr, text_len) }).unwrap();
        buf.get().set_text(
            &mut self.get().font_system,
            str,
            Attrs::new().family(cosmic_text::Family::Serif),
            cosmic_text::Shaping::Advanced,
        );
    }

    #[no_mangle]
    #[export_name = "ctrSetBufferSize"]
    pub extern "C" fn set_buffer_size(self, buf: CtrBuffer, x: f32, y: f32) {
        buf.get().set_size(&mut self.get().font_system, x, y)
    }

    #[no_mangle]
    #[export_name = "ctrShapeBuffer"]
    pub extern "C" fn shape_buffer(self, buf: CtrBuffer, align: CtrAlign) {
        for line in buf.get().lines.iter_mut() {
            line.set_align(Some(align.into()));
        }
        buf.get().shape_until_scroll(&mut self.get().font_system);
    }

    #[no_mangle]
    #[export_name = "ctrDrawBufferSoft"]
    pub extern "C" fn software_draw_buffer(
        self,
        buf: CtrBuffer,
        f_ctx: *mut c_void,
        f: extern "C" fn(*mut c_void, i32, i32, u32, u32, u32),
        color: u32,
    ) {
        buf.get().draw(
            &mut self.get().font_system,
            &mut self.get().cache,
            cosmic_text::Color(color),
            |x, y, w, h, c| f(f_ctx, x, y, w, h, c.0),
        );
    }

    /// # Safety
    /// A whole lotta gl
    #[no_mangle]
    #[export_name = "ctrDrawBuffer"]
    pub unsafe extern "C" fn draw_buffer(self, buf: CtrBuffer, color: u32, rect: CtrRect) {
        let gl = &self.get().gl;

        // dbg!("{:#?}", buf.get());

        for run in buf.get().layout_runs() {
            // dbg!("{run:?}");
            for glyph in run.glyphs.iter() {
                let physical_glyph = glyph.physical((0., 0.), 1.0);

                let al = match self.get().hashmap.get(&physical_glyph.cache_key) {
                    Some(alloc) => *alloc,
                    None => {
                        eprintln!("None: {:?}", physical_glyph.cache_key.glyph_id);
                        let glyph_color = match glyph.color_opt {
                            Some(some) => some,
                            None => cosmic_text::Color(color),
                        };

                        let img = self
                            .get()
                            .cache
                            .get_image_uncached(
                                &mut self.get().font_system,
                                physical_glyph.cache_key,
                            )
                            .unwrap();
                        let sz = guillotiere::size2(
                            img.placement.width as i32,
                            img.placement.height as i32,
                        );

                        if sz.area() == 0 {
                            self.get()
                                .hashmap
                                .insert(physical_glyph.cache_key, (None, img.placement));
                            continue;
                        }

                        let al = match self.get().atlas.allocate(sz) {
                            Some(al) => al,
                            None => {
                                panic!("Ran out of atlas space")
                            }
                        };

                        let temp_buf = gl.create_framebuffer().unwrap();
                        let temp_tex = gl.create_texture().unwrap();
                        gl.bind_framebuffer(glow::FRAMEBUFFER, Some(temp_buf));
                        gl.bind_texture(glow::TEXTURE_2D, Some(temp_tex));

                        match img.content {
                            cosmic_text::SwashContent::Mask => {
                                let pix: Vec<_> = img
                                    .data
                                    .chunks(sz.width as usize)
                                    .rev()
                                    .flat_map(|wor| {
                                        wor.iter().flat_map(|&alpha| {
                                            [
                                                mulcol(alpha, ((color & 0xFF000000) >> 24) as u8),
                                                mulcol(alpha, ((color & 0x00FF0000) >> 16) as u8),
                                                mulcol(alpha, ((color & 0x0000FF00) >> 8) as u8),
                                                mulcol(alpha, (color & 0x000000FF) as u8),
                                            ]
                                        })
                                    })
                                    .collect();
                                gl.tex_image_2d(
                                    glow::TEXTURE_2D,
                                    0,
                                    glow::RGBA as i32,
                                    sz.width,
                                    sz.height,
                                    0,
                                    glow::RGBA,
                                    glow::UNSIGNED_BYTE,
                                    Some(&pix),
                                )
                            }
                            cosmic_text::SwashContent::SubpixelMask => panic!("Subpixel mask"),
                            cosmic_text::SwashContent::Color => {
                                let pix: Vec<_> = img
                                    .data
                                    .chunks(sz.width as usize * 4)
                                    .map(|ch| {
                                        ch.array_chunks::<4>().flat_map(|ch| {
                                            [
                                                mulcol(ch[0], ch[3]),
                                                mulcol(ch[1], ch[3]),
                                                mulcol(ch[2], ch[3]),
                                                ch[3],
                                            ]
                                        })
                                    })
                                    .rev()
                                    .flatten()
                                    .collect();

                                gl.tex_image_2d(
                                    glow::TEXTURE_2D,
                                    0,
                                    glow::RGBA as i32,
                                    sz.width,
                                    sz.height,
                                    0,
                                    glow::RGBA,
                                    glow::UNSIGNED_BYTE,
                                    Some(&pix),
                                )
                            }
                        }
                        gl.framebuffer_texture_2d(
                            glow::FRAMEBUFFER,
                            glow::COLOR_ATTACHMENT0,
                            glow::TEXTURE_2D,
                            Some(temp_tex),
                            0,
                        );
                        gl.bind_texture(glow::TEXTURE_2D, None);
                        gl.bind_framebuffer(glow::FRAMEBUFFER, None);

                        gl.bind_framebuffer(glow::READ_FRAMEBUFFER, Some(temp_buf));
                        gl.bind_framebuffer(glow::DRAW_FRAMEBUFFER, Some(self.get().framebuffer));

                        gl.blit_framebuffer(
                            0,
                            0,
                            sz.width,
                            sz.height,
                            al.rectangle.min.x,
                            al.rectangle.min.y,
                            al.rectangle.max.x,
                            al.rectangle.max.y,
                            glow::COLOR_BUFFER_BIT,
                            glow::NEAREST,
                        );
                        gl.bind_framebuffer(glow::FRAMEBUFFER, None);

                        gl.delete_texture(temp_tex);
                        gl.delete_framebuffer(temp_buf);

                        self.get()
                            .hashmap
                            .insert(physical_glyph.cache_key, (Some(al), img.placement));

                        (Some(al), img.placement)
                    }
                };

                let (al, pl) = if let (Some(al), pl) = al {
                    (al, pl)
                } else {
                    continue;
                };

                let left = physical_glyph.x + pl.left;
                let bottom = physical_glyph.y - run.line_y as i32 + pl.top - pl.height as i32;
                let right = physical_glyph.x + pl.left + pl.width as i32;
                let top = physical_glyph.y - run.line_y as i32 + pl.top;

                // dbg!(left, bottom, right, top);

                gl.bind_framebuffer(glow::READ_FRAMEBUFFER, Some(self.get().framebuffer));

                gl.blit_framebuffer(
                    al.rectangle.min.x,
                    al.rectangle.min.y,
                    al.rectangle.max.x,
                    al.rectangle.max.y,
                    rect.min[0] + left,
                    rect.min[1] + bottom,
                    rect.min[0] + right,
                    rect.min[1] + top,
                    glow::COLOR_BUFFER_BIT,
                    glow::NEAREST,
                );

                gl.bind_framebuffer(glow::FRAMEBUFFER, None);
            }
        }

        // gl.bind_framebuffer(glow::READ_FRAMEBUFFER, Some(self.get().framebuffer));
        // gl.blit_framebuffer(0, 0, 4096, 4096, 0, 0, 3840, 2160, glow::COLOR_BUFFER_BIT, glow::NEAREST);
    }
}
