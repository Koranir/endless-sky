#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

#![feature(vec_into_raw_parts)]

use std::{ops::{DerefMut, Deref}, sync::Arc, str::FromStr, ffi::c_void};

macro_rules! c_fn_namespaced {
    ($for:ty: $( $(#[$($attrss:tt)*])* $name:ident ($($t:tt)*) $(-> $rest:ty)? { $($impl:tt)* } )*) => {paste::paste!{
        $(
            #[no_mangle]
            $(#[$($attrss)*])*
            pub extern "C" fn [< $for _ $name >] ($($t)*) $(-> $rest)? { $($impl)* }
        )*
    }};
}

macro_rules! c_opaque {
    ($ty:ty => $name:ident) => {paste::paste!{
        #[repr(C)]
        pub struct $name (*mut std::ffi::c_void);

        impl Deref for $name {
            type Target = $ty;
        
            fn deref(&self) -> &Self::Target {
                unsafe {
                    &*(self.0 as *mut $ty)
                }
            }
        }
        impl DerefMut for $name {
            fn deref_mut(&mut self) -> &mut Self::Target {
                unsafe {
                    &mut *(self.0 as *mut $ty)
                }
            }
        }
        impl From<$ty> for $name {
            fn from(other: $ty) -> Self {
                Self(Box::into_raw(Box::new(other)) as *mut std::ffi::c_void)
            }
        }
        impl From<$name> for Box<$ty> {
            fn from(other: $name) -> Self {
                unsafe {
                    Box::from_raw(other.0 as *mut $ty)
                }
            }
        }
        impl $name {
            pub fn into_boxed(self) -> Box<$ty> {
                Box::<$ty>::from(self)
            }
        }

        c_fn_namespaced!( $name:
            Drop(this: $name) {
                std::mem::drop(this.into_boxed());
            }
        );
    }};
}

macro_rules! c_tagged_union {
    ($name:ident {
        $($field:ident($const_name:ident): $ty:ty = $val:expr),*
    }) => {paste::paste!{
        #[repr(C)]
        #[derive(Clone, Copy)]
        pub union [< $name Data >] {
            $(pub $field: $ty),*
        }

        #[repr(C)]
        #[allow(non_snake_case)]
        #[derive(Clone, Copy)]
        pub struct $name {
            [< $name _Tag >]: i32,
            data: [< $name Data >],
        }

        $(
            pub const [< $name _Tag_ $const_name >]: i32 = $val;
        )*
    }};
}
macro_rules! c_enum {
    ($ty:ty => $name:ident {
        $($variant:ident($const_name:ident) = $val:expr),*
    }) => {paste::paste!{
        pub type $name = i32;
        $(
            pub const [< $name _ $const_name >]: $name = $val;
        )*

        pub fn [< $ty _from_ $name >] (val: $name) -> $ty {
            match val {
                $(
                    [< $name _ $const_name >] => $ty :: $variant,
                )*
                _ => unreachable!()
            }
        }

        pub fn [< $ty _to_ $name >] (val: $ty) -> $name {
            match val {
                $(
                    $ty :: $variant => [< $name _ $const_name >],
                )*
                #[allow(unreachable_patterns)]
                _ => unreachable!()
            }
        }
    }};
}

macro_rules! c_slice {
    ($ty:ty) => {paste::paste!{
        #[repr(C)]
        #[derive(Clone, Copy)]
        pub struct [< $ty s >] {
            ptr: *const $ty,
            len: usize,
        }
        impl [< $ty s >] {
            pub fn as_slice(&self) -> &'static [$ty] {
                unsafe { std::slice::from_raw_parts(self.ptr, self.len) }
            }
        }
    }};
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct ctSlice {
    ptr: *const u8,
    len: usize,
}
impl<T: AsRef<[u8]>> From<T> for ctSlice {
    fn from(value: T) -> Self {
        let value = value.as_ref();
        Self {
            ptr: value.as_ptr(),
            len: value.len(),
        }
    }
}
impl From<ctSlice> for &'static [u8] {
    fn from(value: ctSlice) -> Self {
        unsafe { std::slice::from_raw_parts(value.ptr, value.len) }
    }
}
impl From<ctSlice> for &'static str {
    fn from(value: ctSlice) -> Self {
        unsafe { std::str::from_utf8_unchecked(value.into()) }
    }
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct ctOwnedSlice {
    ptr: *mut u8,
    len: usize,
    cap: usize,
}
impl From<Vec<u8>> for ctOwnedSlice {
    fn from(value: Vec<u8>) -> Self {
        let (ptr, len, cap) = Vec::into_raw_parts(value);
        Self { ptr, len, cap }
    }
}
c_fn_namespaced!(ctOwnedSlice: 
    Drop(this: ctOwnedSlice) {
        let ctOwnedSlice { ptr, len, cap } = this;
        std::mem::drop(unsafe { Vec::from_raw_parts(ptr,len,cap) })
    }
);

c_tagged_union!(ctSource {
    binary(BINARY_DATA): ctSlice = 0,
    file(FILE_PATH): ctSlice = 1
});
c_slice!(ctSource);

c_opaque!(cosmic_text::FontSystem => ctFontSystem);
c_fn_namespaced!( ctFontSystem:
    /// The caller must drop the returned value
    New() -> ctFontSystem {
        cosmic_text::FontSystem::new().into()
    }

    /// The caller must drop the returned value
    NewWithFonts(fonts: ctSources) -> ctFontSystem {
        cosmic_text::FontSystem::new_with_fonts(
            unsafe { std::slice::from_raw_parts(fonts.ptr,fonts.len) }.iter().map(|source| {
                match source.ctSource_Tag {
                    ctSource_Tag_BINARY_DATA => {
                        cosmic_text::fontdb::Source::Binary(
                            Arc::new(unsafe { std::slice::from_raw_parts(source.data.binary.ptr,source.data.binary.len) })
                        )
                    }
                    ctSource_Tag_FILE_PATH => {
                        cosmic_text::fontdb::Source::File(std::path::PathBuf::from_str(
                            unsafe { std::str::from_utf8_unchecked(std::slice::from_raw_parts(source.data.file.ptr,source.data.file.len)) }
                        ).unwrap())
                    }
                    _ => unreachable!()
                }
            })
        ).into()
    }

    Locale(this: ctFontSystem) -> ctSlice {
        this.locale().as_bytes().into()
    }
);

c_opaque!{cosmic_text::SwashCache => ctSwashCache}
c_fn_namespaced!{ctSwashCache:
    New() -> ctSwashCache {
        cosmic_text::SwashCache::new().into()
    }

    /// ctSwashImage.content will be zero if the image does not exist
    /// The caller owns the returned value and must call drop
    GetImageUncached(mut this: ctSwashCache, mut fontSystem: ctFontSystem, cacheKey: ctCacheKey) -> ctSwashImage {
        this.get_image_uncached(fontSystem.deref_mut(), cacheKey.into()).map(|it| it.into()).unwrap_or(ctSwashImage { content: 0, placement: ctPlacement { left: 0, top: 0, width: 0, height: 0 }, data: ctOwnedSlice { ptr: std::ptr::null_mut(), len: 0, cap: 0 } })
    }

    /// ctSwashImage.content will be zero if the image does not exist
    GetImage(mut this: ctSwashCache, mut fontSystem: ctFontSystem, cacheKey: ctCacheKey) -> ctSwashImage {
        this.get_image(fontSystem.deref_mut(), cacheKey.into()).as_ref().map(|it| it.into()).unwrap_or(ctSwashImage { content: 0, placement: ctPlacement { left: 0, top: 0, width: 0, height: 0 }, data: ctOwnedSlice { ptr: std::ptr::null_mut(), len: 0, cap: 0 } })
    }
}

c_opaque!(cosmic_text::fontdb::ID => ctID);

#[repr(C)]
pub struct ctCacheKey {
    pub fontID: ctID,
    pub glyphID: u16,
    pub fontSizeBits: u32,
    pub xBin: ctSubpixelBin,
    pub yBin: ctSubpixelBin,
}
impl From<ctCacheKey> for cosmic_text::CacheKey {
    fn from(value: ctCacheKey) -> Self {
        let ctCacheKey { fontID, glyphID, fontSizeBits, xBin, yBin } = value;
        Self { font_id: *fontID, glyph_id: glyphID, font_size_bits: fontSizeBits, x_bin: SubpixelBin_from_ctSubpixelBin(xBin), y_bin: SubpixelBin_from_ctSubpixelBin(yBin) }
    }
}

use cosmic_text::SubpixelBin;
c_enum!(SubpixelBin => ctSubpixelBin {
    Zero(ZERO) = 0,
    One(ONE) = 1,
    Two(TWO) = 2,
    Three(THREE) = 3
});

#[repr(C)]
pub struct ctPlacement {
    pub left: i32,
    pub top: i32,
    pub width: u32,
    pub height: u32,
}
impl From<cosmic_text::Placement> for ctPlacement {
    fn from(value: cosmic_text::Placement) -> Self {
        Self {
            left: value.left,
            top: value.top,
            width: value.width,
            height: value.height,
        }
    }
}
impl From<ctPlacement> for cosmic_text::Placement {
    fn from(value: ctPlacement) -> Self {
        Self {
            left: value.left,
            top: value.top,
            width: value.width,
            height: value.height,
        }
    }
}

use cosmic_text::SwashContent;
c_enum!(SwashContent => ctSwashContent {
    Mask(MASK) = 1,
    SubpixelMask(SUBPIXEL_MASK) = 2,
    Color(COLOR) = 3
});

#[repr(C)]
pub struct ctSwashImage {
    pub content: ctSwashContent,
    pub placement: ctPlacement,
    pub data: ctOwnedSlice,
}
impl From<cosmic_text::SwashImage> for ctSwashImage {
    fn from(value: cosmic_text::SwashImage) -> Self {
        Self {
            content: SwashContent_to_ctSwashContent(value.content),
            placement: value.placement.into(),
            data: value.data.into(),
        }
    }
}
impl From<&cosmic_text::SwashImage> for ctSwashImage {
    fn from(value: &cosmic_text::SwashImage) -> Self {
        Self {
            content: SwashContent_to_ctSwashContent(value.content),
            placement: value.placement.into(),
            data: ctOwnedSlice { ptr: value.data.as_ptr() as *mut u8, len: value.data.len(), cap: 0 },
        }
    }
}
c_fn_namespaced!(ctSwashImage:
    Drop(this: ctSwashImage) {
        ctOwnedSlice_Drop(this.data)
    }
);

#[repr(C)]
#[derive(Clone, Copy)]
pub struct ctMetrics {
    pub fontSize: f32,
    pub lineHeight: f32,
}
impl From<cosmic_text::Metrics> for ctMetrics {
    fn from(value: cosmic_text::Metrics) -> Self {
        Self {
            fontSize: value.font_size,
            lineHeight: value.line_height,
        }
    }
}
impl From<ctMetrics> for cosmic_text::Metrics {
    fn from(value: ctMetrics) -> Self {
        Self {
            font_size: value.fontSize,
            line_height: value.lineHeight,
        }
    }
}
c_fn_namespaced!(ctMetrics:
    New(fontSize: f32, lineHeight: f32) -> ctMetrics {
        ctMetrics { fontSize, lineHeight }
    }

    Scale(this: ctMetrics, scale: f32) -> ctMetrics {
        cosmic_text::Metrics::from(this).scale(scale).into()
    }
);

c_opaque!(cosmic_text::Buffer => ctBuffer);
c_fn_namespaced!(ctBuffer:
    NewEmpty(metrics: ctMetrics) -> ctBuffer {
        cosmic_text::Buffer::new_empty(metrics.into()).into()
    }

    New(mut fontSystem: ctFontSystem, metrics: ctMetrics) -> ctBuffer {
        cosmic_text::Buffer::new(fontSystem.deref_mut(), metrics.into()).into()
    }

    ShapeUntil(mut this: ctBuffer, mut fontSystem: ctFontSystem, lines: i32) -> i32 {
        this.shape_until(fontSystem.deref_mut(), lines)
    }

    ShapeUntilScroll(mut this: ctBuffer, mut fontSystem: ctFontSystem) {
        this.shape_until_scroll(fontSystem.deref_mut())
    }

    Metrics(this: ctBuffer) -> ctMetrics {
        this.metrics().into()
    }

    SetMetrics(mut this: ctBuffer, mut fontSystem: ctFontSystem, metrics: ctMetrics) {
        this.set_metrics(fontSystem.deref_mut(), metrics.into())
    }

    Wrap(this: ctBuffer) -> ctWrap {
        Wrap_to_ctWrap(this.wrap())
    }

    SetWrap(mut this: ctBuffer, mut fontSystem: ctFontSystem, wrap: ctWrap) {
        this.set_wrap(fontSystem.deref_mut(), Wrap_from_ctWrap(wrap))
    }

    Size(this: ctBuffer) -> ctSize {
        this.size().into()
    }

    SetSize(mut this: ctBuffer, mut fontSystem: ctFontSystem, width: f32, height: f32) {
        this.set_size(fontSystem.deref_mut(), width, height)
    }

    Scroll(this: ctBuffer) -> i32 {
        this.scroll()
    }

    SetScroll(mut this: ctBuffer, scroll: i32) {
        this.set_scroll(scroll)
    }

    VisibleLines(this: ctBuffer) -> i32 {
        this.visible_lines()
    }

    SetText(mut this: ctBuffer, mut fontSystem: ctFontSystem, text: ctSlice, attrs: ctAttrs, shaping: ctShaping) {
        this.set_text(fontSystem.deref_mut(), text.into(), attrs.into(), Shaping_from_ctShaping(shaping))
    }

    SetRichText(mut this: ctBuffer, mut fontSystem: ctFontSystem, spans: ctRichSpans, shaping: ctShaping) {
        this.set_rich_text(fontSystem.deref_mut(), spans.as_slice().iter().map(|span| (span.text.into(), span.attr.into())), Shaping_from_ctShaping(shaping))
    }

    Redraw(this: ctBuffer) -> bool {
        this.redraw()
    }

    SetRedraw(mut this: ctBuffer, redraw: bool) {
        this.set_redraw(redraw)
    }

    Draw(this: ctBuffer, mut fontSystem: ctFontSystem, mut cache: ctSwashCache, color: ctColor, state: *mut c_void, f: extern "C" fn(*mut c_void, i32, i32, u32, u32, ctColor)) {
        this.draw(fontSystem.deref_mut(), cache.deref_mut(), color.into(), |x, y, w, h, col| {
            f(state, x, y, w, h, col.into())
        })
    }
);

use cosmic_text::Wrap;
c_enum!(Wrap => ctWrap {
    None(NONE) = 1,
    Glyph(GLYPH) = 2,
    Word(WORD) = 3
});

#[repr(C)]
pub struct ctSize(f32, f32);
impl From<(f32, f32)> for ctSize {
    fn from(value: (f32, f32)) -> Self {
        Self(value.0, value.1)
    }
}
impl From<ctSize> for (f32, f32) {
    fn from(value: ctSize) -> Self {
        (value.0, value.1)
    }
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct ctColor{r: u8, g: u8, b: u8, a: u8}
c_fn_namespaced!(ctColor:
    Rgb(r: u8, g: u8, b: u8) -> ctColor {
        ctColor { r, g, b, a: 0xFF }
    }

    Rgba(r: u8, g: u8, b: u8, a:u8) -> ctColor {
        ctColor { r, g, b, a }
    }
);
impl From<cosmic_text::Color> for ctColor {
    fn from(value: cosmic_text::Color) -> Self {
        unsafe { std::mem::transmute(value.0) }
    }
}
impl From<ctColor> for cosmic_text::Color {
    fn from(value: ctColor) -> Self {
        unsafe { std::mem::transmute(value) }
    }
}

c_tagged_union!(ctFamily {
    Name(NAME): ctSlice = 1,
    Serif(SERIF): () = 2,
    SansSerif(SANS_SERIF): () = 3,
    Cursive(CURSIVE): () = 4,
    Fantasy(FANTASY): () = 5,
    Monospace(MONOSPACE): () = 6
});

use cosmic_text::Stretch;
c_enum!(Stretch => ctStretch {
    UltraCondensed(ULTRA_CONDENSED) = 1,
    ExtraCondensed(EXTRA_CONDENSED) = 2,
    Condensed(CONDENSED) = 3,
    SemiCondensed(SEMI_CONDENSED) = 4,
    Normal(NORMAL) = 5,
    SemiExpanded(SEMI_EXPANDED) = 6,
    ExtraExpanded(EXTRA_EXPANDED) = 7,
    UltraExpanded(ULTRA_EXPANDED) = 8
});

use cosmic_text::Style;
c_enum!(Style => ctStyle {
    Normal(NORMAL) = 1,
    Italic(ITALIC) = 2,
    Oblique(OBLIQUE) = 3
});

pub type ctWeight = u16;
pub const ctWeight_THIN: ctWeight = 100;
pub const ctWeight_EXTRA_LIGHT: ctWeight = 200;
pub const ctWeight_LIGHT: ctWeight = 300;
pub const ctWeight_NORMAL: ctWeight = 400;
pub const ctWeight_MEDIUM: ctWeight = 500;
pub const ctWeight_SEMIBOLD: ctWeight = 600;
pub const ctWeight_BOLD: ctWeight = 700;
pub const ctWeight_EXTRA_BOLD: ctWeight = 800;
pub const ctWeight_BLACK: ctWeight = 900;

#[repr(C)]
#[derive(Clone, Copy)]
pub struct ctAttrs {
    pub use_color: bool,
    pub color: ctColor,
    pub family: ctFamily,
    pub stretch: ctStretch,
    pub style: ctStyle,
    pub weight: ctWeight,
    pub metadata: usize,
}
impl From<cosmic_text::Attrs<'_>> for ctAttrs {
    fn from(value: cosmic_text::Attrs) -> Self {
        let family = match value.family {
            cosmic_text::Family::Cursive => ctFamily { ctFamily_Tag: ctFamily_Tag_CURSIVE, data: ctFamilyData { Cursive: () } },
            cosmic_text::Family::Name(str) => ctFamily { ctFamily_Tag: ctFamily_Tag_NAME, data: ctFamilyData { Name: str.as_bytes().into() } },
            cosmic_text::Family::Serif => ctFamily { ctFamily_Tag: ctFamily_Tag_SERIF, data: ctFamilyData { Serif: () } },
            cosmic_text::Family::SansSerif => ctFamily { ctFamily_Tag: ctFamily_Tag_SANS_SERIF, data: ctFamilyData { SansSerif: () } },
            cosmic_text::Family::Fantasy => ctFamily { ctFamily_Tag: ctFamily_Tag_FANTASY, data: ctFamilyData { Fantasy: () } },
            cosmic_text::Family::Monospace => ctFamily { ctFamily_Tag: ctFamily_Tag_MONOSPACE, data: ctFamilyData { Monospace: () } },
        };

        Self {
            use_color: value.color_opt.is_some(),
            color: value.color_opt.unwrap_or(cosmic_text::Color(0)).into(),
            family,
            stretch: Stretch_to_ctStretch(value.stretch),
            style: Style_to_ctStyle(value.style),
            weight: value.weight.0,
            metadata: value.metadata,
        }
    }
}
impl From<ctAttrs> for cosmic_text::Attrs<'static> {
    fn from(value: ctAttrs) -> Self {
        use cosmic_text::Family::*;
        let family = match value.family.ctFamily_Tag {
            ctFamily_Tag_CURSIVE => Cursive,
            ctFamily_Tag_NAME => Name(unsafe { value.family.data.Name }.into()),
            ctFamily_Tag_SERIF => Serif,
            ctFamily_Tag_SANS_SERIF => SansSerif,
            ctFamily_Tag_FANTASY => Fantasy,
            ctFamily_Tag_MONOSPACE => Monospace,
            _ => unreachable!()
        };

        Self {
            color_opt: value.use_color.then_some(value.color.into()),
            family,
            stretch: Stretch_from_ctStretch(value.stretch),
            style: Style_from_ctStyle(value.style),
            weight: cosmic_text::Weight(value.weight),
            metadata: value.metadata,
        }
    }
}
c_fn_namespaced!(ctAttrs:
    New() -> ctAttrs {
        cosmic_text::Attrs::new().into()
    }
);

use cosmic_text::Shaping;
c_enum!(Shaping => ctShaping {
    Basic(BASIC) = 1,
    Advanced(ADVANCED) = 2
});

#[repr(C)]
pub struct ctRichSpan {
    text: ctSlice,
    attr: ctAttrs,
}
c_slice!(ctRichSpan);
