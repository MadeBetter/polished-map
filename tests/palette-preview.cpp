// Run: make test-palette-preview GAME_ROOT=/path/to/polished-crystal
#include <cassert>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <FL/Fl_Shared_Image.H>
#include "config.h"
#include "metatileset.h"

int main(int argc, char **argv) {
    assert(argc == 3);
    std::string root = argv[1];
    if (root.back() != '/') { root += '/'; }
    fl_register_images();
    Config::allow_512_tiles(true);
    Config::arrange_0_before_1(false);
    Metatileset blocks;
    std::string palette = root + "maps/VioletCity.pal";
    const auto preview = Color::parse_palettes(palette.c_str());
    const auto runtime = Color::parse_palettes((root + "gfx/tilesets/violet_city_redplusplus.pal").c_str());
    const auto standard = Color::parse_palettes((root + "gfx/tilesets/bg_tiles.pal").c_str());
    assert(preview.size() == 24 && runtime.size() >= 21 && standard.size() >= 24);
    for (size_t time = 0; time < 3; time++) {
        for (size_t slot = 0; slot < 7; slot++) {
            assert(preview[time * 8 + slot] == runtime[time * 7 + slot]);
        }
        assert(preview[time * 8 + 7] == standard[time * 8 + 7]);
    }
    Color::read_palettes(palette.c_str(), Palettes::DAY);
    auto &tiles = blocks.tileset();
    assert(tiles.read_graphics((root + "gfx/tilesets/new_bark_cherrygrove.redplusplus_johto_common.png").c_str(),
        (root + "gfx/tilesets/redplusplus_johto_common.png").c_str(), NULL, Palettes::DAY) == Tileset::Result::GFX_OK);
    assert(blocks.read_metatiles((root + "data/tilesets/new_bark_cherrygrove_metatiles.bin").c_str()) == Metatileset::Result::META_OK);
    assert(blocks.read_attributes((root + "data/tilesets/new_bark_cherrygrove_attributes.bin").c_str()) == Metatileset::Result::META_OK);
    blocks.read_palette_swaps(root.c_str(), (root + "maps/VioletCity.asm").c_str());
    for (auto time : {Palettes::MORN, Palettes::DAY, Palettes::NITE}) {
        tiles.update_palettes(time);
        const auto *roof = blocks.preview_palette(Palette::ROOF, 18, 20);
        const auto *station = blocks.preview_palette(Palette::ROOF, 2, 28);
        const auto *pond = blocks.preview_palette(Palette::WATER, 18, 12);
        const auto *blue = blocks.preview_palette(Palette::WATER, 2, 28);
        assert(roof && station && pond && blue && *roof != *station && *pond != *blue);
        assert(*blocks.preview_palette(Palette::ROOF, 5, 39) == *station);
        assert(*blocks.preview_palette(Palette::ROOF, 6, 39) == *roof);
        assert(*blocks.preview_palette(Palette::ROOF, 5, 40) == *roof);
        assert(*blocks.preview_palette(Palette::WATER, 9, 39) == *blue);
        assert(*blocks.preview_palette(Palette::WATER, 10, 39) == *pond);
        assert(!blocks.preview_palette(Palette::GREEN, 2, 28));
        assert(!blocks.preview_palette(Palette::ROOF, -1, -1));
    }
    tiles.update_palettes(Palettes::CUSTOM);
    assert(!blocks.preview_palette(Palette::ROOF, 2, 28));
    tiles.update_palettes(Palettes::DAY);
    Map map; map.size(20, 20);
    assert(map.read_blocks((root + "maps/VioletCity.ablk").c_str()) == Map::Result::MAP_OK);
    std::unique_ptr<uchar[]> image(blocks.print_rgb(map));
    std::ofstream ppm(argv[2], std::ios::binary);
    ppm << "P6\n640 640\n255\n";
    ppm.write(reinterpret_cast<const char *>(image.get()), 640 * 640 * NUM_CHANNELS);
    ppm.close();
    assert(ppm.good());
    assert(!tiles.modified() && !blocks.modified() && !map.modified());
    blocks.read_palette_swaps(root.c_str(), (root + "maps/Route36.asm").c_str());
    assert(!blocks.preview_palette(Palette::ROOF, 2, 28));
    blocks.read_palette_swaps(root.c_str(), (root + "maps/VioletOutskirts.asm").c_str());
    assert(blocks.preview_palette(Palette::ROOF, 20, 10));
    assert(!blocks.preview_palette(Palette::ROOF, 255, 10)); // NULL outside keeps base colors.
    blocks.clear();
    assert(!blocks.preview_palette(Palette::ROOF, 20, 10));
    // A three-pair roof table must read group 10, not Cinnabar's evening row.
    assert(Color::read_roof_colors((root + "gfx/tilesets/roofs.pal").c_str(), 10, Roof_Palettes::ROOF_DAY_NITE));
    assert(Color::color(Palettes::DAY, Palette::ROOF, Hue::LIGHT)[0] == RGB5C(24));
    assert(Color::color(Palettes::DAY, Palette::ROOF, Hue::LIGHT)[1] == RGB5C(14));
    assert(Color::color(Palettes::NITE, Palette::ROOF, Hue::LIGHT)[0] == RGB5C(12));
    std::cout << "PASS: regional palettes, half-open rectangle edges, all three times, manual custom colors, map-switch clearing, correct roof-table stride, unchanged map data, and full-map PNG export.\n";
}
