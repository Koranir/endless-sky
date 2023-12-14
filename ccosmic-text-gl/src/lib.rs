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

macro_rules! extern_drop {
    ($what:ty, $base:ty, $name:ident) => {
        #[no_mangle]
        pub extern "C" fn $name(what: $what) {
            unsafe {
                std::mem::drop(Box::from_raw(what.0 as *mut $base));
            }
        }
    };
}

#[derive(Debug, Clone, Copy)]
pub struct Uniforms {
    source_pos: glow::UniformLocation,
    source_size: glow::UniformLocation,

    dest_pos: glow::UniformLocation,
    dest_size: glow::UniformLocation,

    tex: glow::UniformLocation,

    tex_col: glow::UniformLocation,
}

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
    shader: (
        glow::NativeProgram,
        Uniforms,
        (glow::NativeBuffer, glow::NativeVertexArray),
    ),
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
            None,
        );
        gl.tex_parameter_i32(
            glow::TEXTURE_2D,
            glow::TEXTURE_MIN_FILTER,
            glow::NEAREST as i32,
        );
        gl.tex_parameter_i32(
            glow::TEXTURE_2D,
            glow::TEXTURE_MAG_FILTER,
            glow::NEAREST as i32,
        );
        gl.framebuffer_texture_2d(
            glow::FRAMEBUFFER,
            glow::COLOR_ATTACHMENT0,
            glow::TEXTURE_2D,
            Some(tex),
            0,
        );

        gl.bind_texture(glow::TEXTURE_2D, None);

        gl.bind_framebuffer(glow::FRAMEBUFFER, None);

        let shader = gl.create_program().unwrap();
        let vertex = gl.create_shader(glow::VERTEX_SHADER).unwrap();
        let fragment = gl.create_shader(glow::FRAGMENT_SHADER).unwrap();

        gl.shader_source(vertex, VERTEX_GLSL);
        gl.shader_source(fragment, FRAGMENT_GLSL);

        gl.compile_shader(vertex);
        gl.compile_shader(fragment);

        assert!(
            gl.get_shader_compile_status(vertex),
            "Failed to compile vertex shader: {}",
            gl.get_shader_info_log(vertex)
        );
        assert!(
            gl.get_shader_compile_status(fragment),
            "Failed to link fragment shader: {}",
            gl.get_shader_info_log(fragment),
        );

        gl.attach_shader(shader, vertex);
        gl.attach_shader(shader, fragment);

        gl.link_program(shader);

        assert!(
            gl.get_program_link_status(shader),
            "Failed to link shader program: {}",
            gl.get_program_info_log(shader)
        );

        gl.delete_shader(vertex);
        gl.delete_shader(fragment);

        let uniforms = Uniforms {
            source_pos: gl.get_uniform_location(shader, "source_pos").unwrap(),
            source_size: gl.get_uniform_location(shader, "source_size").unwrap(),

            dest_pos: gl.get_uniform_location(shader, "dest_pos").unwrap(),
            dest_size: gl.get_uniform_location(shader, "dest_size").unwrap(),

            tex: gl.get_uniform_location(shader, "tex").unwrap(),

            tex_col: gl.get_uniform_location(shader, "tex_col").unwrap(),
        };

        let vao = gl.create_vertex_array().unwrap();
        gl.bind_vertex_array(Some(vao));
        let buf = gl.create_buffer().unwrap();
        gl.bind_buffer(glow::ARRAY_BUFFER, Some(buf));
        gl.buffer_data_u8_slice(
            glow::ARRAY_BUFFER,
            bytemuck::cast_slice(&[
                -0.5f32, -0.5f32, -0.5f32, 0.5f32, 0.5f32, -0.5f32, 0.5f32, 0.5f32,
            ]),
            glow::STATIC_DRAW,
        );

        let vert_id = gl.get_attrib_location(shader, "vert").unwrap();

        gl.vertex_attrib_pointer_f32(
            vert_id,
            2,
            glow::FLOAT,
            false,
            std::mem::size_of::<[f32; 2]>() as i32,
            0,
        );
        gl.enable_vertex_attrib_array(vert_id);

        gl.bind_vertex_array(None);
        gl.bind_buffer(glow::ARRAY_BUFFER, None);

        Self {
            texture: tex,
            framebuffer: texture,
            gl,
            font_system,
            cache,
            atlas,
            hashmap: std::collections::HashMap::new(),
            shader: (shader, uniforms, (buf, vao)),
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
    /// # Safety
    /// Must be dropped later
    #[must_use]
    pub unsafe fn new(value: cosmic_text::Buffer) -> Self {
        let boxed = Box::new(value);
        Self(Box::into_raw(boxed).cast::<c_void>())
    }

    #[must_use]
    pub fn get(self) -> &'static mut cosmic_text::Buffer {
        unsafe { &mut *self.0.cast() }
    }
}
extern_drop!(CtrBuffer, cosmic_text::Buffer, CtrBufferDrop);

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct CtrRect {
    min: [f32; 2],
    max: [f32; 2],
}

#[repr(transparent)]
#[derive(Debug, Copy, Clone)]
pub struct FfiCtr(*mut c_void);
impl From<CCosmicTextRenderer> for FfiCtr {
    fn from(value: CCosmicTextRenderer) -> Self {
        let boxed = Box::new(value);
        Self(Box::into_raw(boxed).cast::<c_void>())
    }
}
impl FfiCtr {
    #[must_use]
    pub fn get(self) -> &'static mut CCosmicTextRenderer {
        unsafe { &mut *self.0.cast() }
    }
}

fn mulcol(a: u8, b: u8) -> u8 {
    ((u32::from(a) * u32::from(b)) as f32 / 255.) as u8
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
        drop(Box::from_raw(self.0.cast::<CCosmicTextRenderer>()));
    }

    /// # Safety
    /// `self` will always be initialized
    /// Callee must deinitialize the resultant string
    #[no_mangle]
    #[export_name = "ctrDebug"]
    #[must_use]
    pub unsafe extern "C" fn debug(self) -> *mut c_char {
        CString::new(self.get().dbg()).unwrap().into_raw()
    }

    /// # Safety
    /// `str` should be created from ctrDebug
    #[no_mangle]
    #[export_name = "ctrDeinitDebug"]
    pub unsafe extern "C" fn deinit_debug(str: *mut c_char) {
        drop(CString::from_raw(str));
    }

    #[no_mangle]
    #[export_name = "ctrCreateBuffer"]
    #[must_use]
    pub extern "C" fn create_buffer(self, font_size: f32, line_height: f32) -> CtrBuffer {
        unsafe {
            CtrBuffer::new(cosmic_text::Buffer::new(
                &mut self.get().font_system,
                Metrics::new(font_size, line_height),
            ))
        }
    }

    /// # Safety
    /// `text_ptr` should be proper UTF-8
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
            Attrs::new().family(cosmic_text::Family::Name("ubuntu")),
            cosmic_text::Shaping::Advanced,
        );
    }

    #[no_mangle]
    #[export_name = "ctrSetBufferSize"]
    pub extern "C" fn set_buffer_size(self, buf: CtrBuffer, x: f32, y: f32) {
        buf.get().set_size(&mut self.get().font_system, x, y);
    }

    #[no_mangle]
    #[export_name = "ctrShapeBuffer"]
    pub extern "C" fn shape_buffer(self, buf: CtrBuffer, align: CtrAlign) {
        for line in &mut buf.get().lines {
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

    #[no_mangle]
    #[export_name = "ctrBufferLen"]
    pub extern "C" fn tex_buffer_len(
        self,
        buf: CtrBuffer,
        line: usize
    ) -> f32 {
        if let Some(lines) = buf.get().line_layout(&mut self.get().font_system, line) {
            lines[0].w
        } else {
            0.0
        }
    }

    /// # Safety
    /// Completely safe, just don't like gl being unsafe.
    /// 
    /// color is argb in the format AARRGGBB
    #[no_mangle]
    #[export_name = "ctrDrawBuffer"]
    pub unsafe extern "C" fn draw_buffer(
        self,
        buf: CtrBuffer,
        color: u32,
        rect: CtrRect,
        screen_width: f32,
        screen_height: f32,
    ) {
        let rend = self.get();
        let gl = &rend.gl;
        let uniforms = rend.shader.1;

        gl.use_program(Some(rend.shader.0));
        gl.bind_vertex_array(Some(rend.shader.2.1));
        gl.active_texture(glow::TEXTURE0);
        gl.bind_texture(glow::TEXTURE_2D, Some(rend.texture));
        gl.uniform_1_i32(Some(&uniforms.tex), 0);

        let mut prev_was_none = false;
        let mut col_changed;
        let coll = cosmic_text::Color(color);

        for run in buf.get().layout_runs() {
            for glyph in run.glyphs {
                let physical_glyph = glyph.physical((0., 0.), 1.0);

                let glyph_color = match glyph.color_opt {
                    Some(col) => {
                        prev_was_none = false;
                        col_changed = true;
                        col
                    },
                    None => {
                        col_changed = !prev_was_none;
                        prev_was_none = true;
                        coll
                    },
                };

                if col_changed {
                    gl.uniform_4_f32(Some(&uniforms.tex_col), 
                        glyph_color.r() as f32 / 255.0f32, 
                        glyph_color.g() as f32 / 255.0f32, 
                        glyph_color.b() as f32 / 255.0f32,
                        glyph_color.a() as f32 / 255.0f32
                    );
                }

                let al = if let Some(alloc) = self.get().hashmap.get(&physical_glyph.cache_key) {
                    *alloc
                } else if let Some(al) = self.glyph_into_atlas(
                    gl,
                    &physical_glyph
                ) {
                    al
                } else {
                    continue;
                };

                let (Some(al), pl) = al else {
                    continue;
                };

                // rect.min[1] + rect.max[1] - rect.max[0];

                gl.uniform_2_f32(
                    Some(&uniforms.dest_pos),
                    ((physical_glyph.x + pl.left) as f32 + (pl.width as f32 / 2.0f32) + rect.min[0])
                        / screen_width,
                    ((physical_glyph.y + pl.top) as f32 - (pl.height as f32 / 2.0f32) - rect.max[1] - run.line_y)
                        / screen_height,
                );
                gl.uniform_2_f32(
                    Some(&uniforms.dest_size),
                    pl.width as f32 / screen_width,
                    pl.height as f32 / screen_height,
                );

                gl.uniform_2_f32(
                    Some(&uniforms.source_pos),
                    al.rectangle.center().x as f32 / self.get().atlas.size().width as f32,
                    al.rectangle.center().y as f32 / self.get().atlas.size().height as f32,
                );
                gl.uniform_2_f32(
                    Some(&uniforms.source_size),
                    al.rectangle.size().width as f32 / self.get().atlas.size().width as f32,
                    al.rectangle.size().height as f32 / self.get().atlas.size().height as f32,
                );

                gl.draw_arrays(glow::TRIANGLE_STRIP, 0, 4);
            }
        }

        gl.use_program(None);
        gl.bind_vertex_array(None);
        gl.bind_texture(glow::TEXTURE_2D, None);
    }

    unsafe fn glyph_into_atlas(self, gl: &glow::Context, physical_glyph: &cosmic_text::PhysicalGlyph) -> Option<(Option<guillotiere::Allocation>, cosmic_text::Placement)> {
        let img = self
            .get()
            .cache
            .get_image_uncached(&mut self.get().font_system, physical_glyph.cache_key)
            .unwrap();
        let sz =
            guillotiere::size2(img.placement.width as i32, img.placement.height as i32);
    
        if sz.area() == 0 {
            self.get()
                .hashmap
                .insert(physical_glyph.cache_key, (None, img.placement));
            return None;
        }
    
        // let Some(mut al) = self.get().atlas.allocate(sz + guillotiere::size2(2, 2)) else {
        //     panic!("Ran out of atlas space")
        // };
        let mut al = self.get().atlas.allocate(sz + guillotiere::size2(2, 2)).expect("Ran out of atlas space");
        al.rectangle.max -= guillotiere::size2(1, 1);
        al.rectangle.min += guillotiere::size2(1, 1);

        let temp_buf = gl.create_framebuffer().unwrap();
        let temp_tex = gl.create_texture().unwrap();
        gl.bind_framebuffer(glow::FRAMEBUFFER, Some(temp_buf));
        gl.bind_texture(glow::TEXTURE_2D, Some(temp_tex));
    
        let rgba_pix: Vec<_> = match img.content {
            cosmic_text::SwashContent::Mask => {
                img
                    .data
                    .chunks(sz.width as usize)
                    .rev()
                    .flat_map(|wor| {
                        wor.iter().flat_map(|&alpha| {
                            [
                                255,
                                255,
                                255,
                                alpha,
                            ]
                        })
                    })
                    .collect()
            }
            cosmic_text::SwashContent::SubpixelMask => unimplemented!("subpixel mask"),
            cosmic_text::SwashContent::Color => {
                img
                    .data
                    .chunks(sz.width as usize * 4)
                    .rev()
                    .flatten()
                    .copied()
                    .collect()
            }
        };
        let premultiplied: Vec<_> = rgba_pix.chunks(4).flat_map(|ch|{
            [
                mulcol(ch[0], ch[3]),
                mulcol(ch[1], ch[3]),
                mulcol(ch[2], ch[3]),
                ch[3],
            ]
        }).collect();
        gl.tex_image_2d(
            glow::TEXTURE_2D,
            0,
            glow::RGBA as i32,
            sz.width,
            sz.height,
            0,
            glow::RGBA,
            glow::UNSIGNED_BYTE,
            Some(&premultiplied),
        );

        gl.framebuffer_texture_2d(
            glow::FRAMEBUFFER,
            glow::COLOR_ATTACHMENT0,
            glow::TEXTURE_2D,
            Some(temp_tex),
            0,
        );
        gl.bind_texture(glow::TEXTURE_2D, None);
    
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

        gl.bind_texture(glow::TEXTURE_2D, Some(self.get().texture));
    
        Some((Some(al), img.placement))
    }
}

// Am OpenGL Shader that blits a rect from a texture atlas to a rectangle on the screen.
const VERTEX_GLSL: &str = include_str!("blit.vert");
const FRAGMENT_GLSL: &str = include_str!("blit.frag");
