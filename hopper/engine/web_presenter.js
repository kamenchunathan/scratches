mergeInto(LibraryManager.library, {
  render_to_dom: function(buffer_ptr, width, height) {
    // 3 bytes 
    // struct ColorRGB8 {
    //   std::uint8_t r, g, b;
    // };

    /// Takes a u32 (Number) representation of ColorRGB8 and converts it into a string
    // css-style hexcode assuming little endian 
    function colorRGB8ToHex(repr){
      const r = (repr & 0xFF).toString(16).padStart(2, '0');
      const g = ((repr >> 8) & 0xFF) .toString(16).padStart(2, '0');
      const b = ((repr >> 16) &0xFF).toString(16).padStart(2, '0');
      return `#${r}${g}${b}`;
    }

    // 4 bytes x 3 (because of alignment and padding)
    // struct CharacterPixel {
    //   char32_t codepoint = U' ';
    //   core::ColorRGB8 fg_color;
    //   core::ColorRGB8 bg_color;
    // };
    const pixel_size = 12;

    // Create a view into wasm's linear memory where the bufer pointer is an offset in
    const view = new Uint32Array(
      Module.HEAPU8.buffer,
      buffer_ptr,
      height * width * 3 // Buffer size x (sizeof(CharacterPixel) / sizeof(u32))
    );

    let html = '';
    for (let y = 0; y < height; ++y)  {
      html += '<div>';
      for (let x = 0; x < width; ++x) {
        const pixel_base_idx = (y * width + x) * 3;
        const codepoint =  view[pixel_base_idx];
        const fg_color = colorRGB8ToHex(view[pixel_base_idx + 1]);
        const bg_color = colorRGB8ToHex(view[pixel_base_idx + 2]);
        
        let character =  String.fromCodePoint(codepoint);
        // TODO: Better escape handling
        if (character === '<') character = '&lt;';
        else if (character === '>') character = '&gt;';
        else if (character === '&') character = '&amp;';
        else if (character === ' ') character = '&nbsp;';
        
        // We create a span for every character as the colors / characters will be assumed to be
        // changing often
        html += `<span style="color: ${fg_color}; background-color: ${bg_color}">${character}</span>`;
      }
      html += '</div>'
    }

    if (Module.frameContainer) {
      Module.frameContainer.innerHTML = html;
    }
  }
});
