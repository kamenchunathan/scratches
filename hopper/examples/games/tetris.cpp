#include <algorithm>
#include <chrono>
#include <cmath>
#include <format>
#include <fstream>
#include <memory>
#include <random>
#include <vector>

#include <Eigen/Dense>

#include "application.hpp"
#include "color.hpp"
#include "common.hpp"
#include "ecs/query.hpp"
#include "ecs/system.hpp"
#include "ecs/world.hpp"
#include "input.hpp"
#include "log.hpp"
#include "log/layer.hpp"
#include "renderer.hpp"
#include "renderer/command.hpp"
#include "renderer/graphs/halfblock.hpp"
#include "renderer/resource.hpp"
#include "renderer/shader.hpp"
#include "renderer/types.hpp"
#include "time.hpp"
#include "transform.hpp"

#if PLATFORM_WASM
    #include "web/layer.hpp"
    #include "web/presenter.hpp"
#else
    #include "term/layer.hpp"
#endif

// ---------------------------------------------------------------------------
// Enumerations
// ---------------------------------------------------------------------------

enum class GameState {
    StartMenu,
    ModeSelect,
    DifficultySelect,
    Playing,
    Paused,
    GameOver,
    HighScoreInput,
    LeaderboardScreen,
    SettingsMenu,
    CreditsScreen
};

enum class GameMode : std::uint8_t {
    Classic      = 0,
    Sprint       = 1,
    Invisible    = 2,
    Garbage      = 3,
    HardDropOnly = 4,
    Count
};

enum class Difficulty : std::uint8_t { Standard = 0, Hard = 1, Bastard = 2, Count };

enum class ColorTheme : std::uint8_t { Catppuccin = 0, TokyoNight = 1, Kanagawa = 2, Count };

// ---------------------------------------------------------------------------
// Theme palette
// ---------------------------------------------------------------------------

struct ThemePalette {
    core::ColorRGBA32F background;
    core::ColorRGBA32F border;
    core::ColorRGBA32F empty_cell;
    core::ColorRGBA32F text_primary;
    core::ColorRGBA32F text_secondary;
    core::ColorRGBA32F text_accent;
    core::ColorRGBA32F ui_bg;
};

static ThemePalette make_palette(ColorTheme theme) {
    switch (theme) {
        case ColorTheme::TokyoNight:
            return {
                .background     = core::ColorRGBA32F::rgba(0.102f, 0.106f, 0.149f, 1.0f),
                .border         = core::ColorRGBA32F::rgba(0.161f, 0.180f, 0.259f, 1.0f),
                .empty_cell     = core::ColorRGBA32F::rgba(0.086f, 0.086f, 0.118f, 1.0f),
                .text_primary   = core::ColorRGBA32F::rgba(0.753f, 0.792f, 0.961f, 1.0f),
                .text_secondary = core::ColorRGBA32F::rgba(0.663f, 0.694f, 0.839f, 1.0f),
                .text_accent    = core::ColorRGBA32F::rgba(0.733f, 0.604f, 0.969f, 1.0f), // Magenta
                .ui_bg          = core::ColorRGBA32F::rgba(0.141f, 0.157f, 0.231f, 1.0f),
            };
        case ColorTheme::Kanagawa:
            return {
                .background     = core::ColorRGBA32F::rgba(0.122f, 0.122f, 0.157f, 1.0f),
                .border         = core::ColorRGBA32F::rgba(0.329f, 0.329f, 0.427f, 1.0f),
                .empty_cell     = core::ColorRGBA32F::rgba(0.086f, 0.086f, 0.114f, 1.0f),
                .text_primary   = core::ColorRGBA32F::rgba(0.863f, 0.843f, 0.729f, 1.0f),
                .text_secondary = core::ColorRGBA32F::rgba(0.576f, 0.541f, 0.663f, 1.0f),
                .text_accent = core::ColorRGBA32F::rgba(0.584f, 0.498f, 0.722f, 1.0f), // OniViolet
                .ui_bg       = core::ColorRGBA32F::rgba(0.133f, 0.196f, 0.286f, 1.0f),
            };
        default: // Catppuccin Macchiato
            return {
                .background     = core::ColorRGBA32F::rgba(0.141f, 0.153f, 0.227f, 1.0f),
                .border         = core::ColorRGBA32F::rgba(0.118f, 0.125f, 0.188f, 1.0f),
                .empty_cell     = core::ColorRGBA32F::rgba(0.094f, 0.094f, 0.145f, 1.0f),
                .text_primary   = core::ColorRGBA32F::rgba(0.792f, 0.827f, 0.961f, 1.0f),
                .text_secondary = core::ColorRGBA32F::rgba(0.647f, 0.678f, 0.808f, 1.0f),
                .text_accent    = core::ColorRGBA32F::rgba(0.961f, 0.663f, 0.498f, 1.0f), // Peach
                .ui_bg          = core::ColorRGBA32F::rgba(0.255f, 0.271f, 0.349f, 1.0f),
            };
    }
}

// ---------------------------------------------------------------------------
// Persistence helpers
// ---------------------------------------------------------------------------

static std::string get_data_path() {
#if PLATFORM_WASM
    return "highscores.dat";
#else
    const char* xdg = std::getenv("XDG_DATA_HOME");
    std::string dir;
    if (xdg) {
        dir = std::string(xdg) + "/hopper";
    } else if (const char* home = std::getenv("HOME")) {
        dir = std::string(home) + "/.local/share/hopper";
    } else {
        return "tetris_highscores.dat";
    }
    (void)std::system(("mkdir -p " + dir).c_str());
    return dir + "/tetris_highscores.dat";
#endif
}

// ---------------------------------------------------------------------------
// High-score types
// ---------------------------------------------------------------------------

struct HighScoreEntry {
    std::string name;
    std::int32_t score    = 0;
    Difficulty difficulty = Difficulty::Standard;
};

struct HighScores {
    static constexpr std::size_t MAX_PER_DIFF = 10;
    std::vector<HighScoreEntry> entries;
    std::string current_input;

    [[nodiscard]] std::vector<HighScoreEntry> for_difficulty(Difficulty d) const {
        std::vector<HighScoreEntry> out;
        for (const auto& e: entries) {
            if (e.difficulty == d)
                out.push_back(e);
        }
        return out;
    }

    [[nodiscard]] std::int32_t lowest_for(Difficulty d) const {
        auto v = for_difficulty(d);
        if (v.size() < MAX_PER_DIFF)
            return 0;
        return v.back().score;
    }

    [[nodiscard]] bool qualifies(std::int32_t score, Difficulty d) const {
        auto v = for_difficulty(d);
        return v.size() < MAX_PER_DIFF || score > lowest_for(d);
    }
};

struct Settings {
    ColorTheme theme   = ColorTheme::Catppuccin;
    bool ghost_enabled = true;
};

struct AppData {
    Settings settings;
    HighScores high_scores;
};

static void load_app_data(AppData& data) {
#if PLATFORM_WASM
#else

    std::ifstream file(get_data_path());
    if (!file.is_open())
        return;

    std::string type;
    while (file >> type) {
        if (type == "SET") {
            int theme;
            file >> theme >> data.settings.ghost_enabled;
            data.settings.theme
                = static_cast<ColorTheme>(std::clamp(theme, 0, (int)ColorTheme::Count - 1));
        } else if (type == "HS") {
            std::string name;
            std::int32_t score, diff;
            file >> name >> score >> diff;
            std::replace(name.begin(), name.end(), '_', ' ');
            data.high_scores.entries.push_back({name, score, static_cast<Difficulty>(diff)});
        }
    }

#endif
}

static void save_app_data(const AppData& data) {
#if PLATFORM_WASM
#else
    std::ofstream file(get_data_path());
    file << "SET " << (int)data.settings.theme << " " << data.settings.ghost_enabled << "\n";
    for (const auto& e: data.high_scores.entries) {
        std::string name = e.name.empty() ? "UNKNOWN" : e.name;
        std::replace(name.begin(), name.end(), ' ', '_');
        file << "HS " << name << " " << e.score << " " << (int)e.difficulty << "\n";
    }
#endif
}

static void insert_score(HighScores& hs, HighScoreEntry entry) {
    hs.entries.push_back(entry);
    std::stable_sort(
        hs.entries.begin(),
        hs.entries.end(),
        [](const HighScoreEntry& a, const HighScoreEntry& b) {
            if (a.difficulty != b.difficulty)
                return static_cast<int>(a.difficulty) < static_cast<int>(b.difficulty);
            return a.score > b.score;
        }
    );
    for (std::uint8_t d = 0; d < static_cast<std::uint8_t>(Difficulty::Count); ++d) {
        Difficulty diff   = static_cast<Difficulty>(d);
        std::size_t count = 0;
        hs.entries.erase(
            std::remove_if(
                hs.entries.begin(),
                hs.entries.end(),
                [&](const HighScoreEntry& e) {
                    if (e.difficulty != diff)
                        return false;
                    ++count;
                    return count > HighScores::MAX_PER_DIFF;
                }
            ),
            hs.entries.end()
        );
    }
}

// ---------------------------------------------------------------------------
// Tetromino definitions
// ---------------------------------------------------------------------------

struct Tetromino {
    Eigen::Vector2i blocks[4];
    core::ColorRGBA32F color;
};

static Tetromino TETROMINOS[8] = {
    {{{-1, 0}, {0, 0}, {1, 0}, {2, 0}},
     core::ColorRGBA32F::rgba(0.10f, 0.80f, 0.90f, 1.0f)}, // I - Cyan
    {{{0, 0}, {1, 0}, {0, 1}, {1, 1}},
     core::ColorRGBA32F::rgba(0.90f, 0.90f, 0.10f, 1.0f)}, // O - Yellow
    {{{-1, 0}, {0, 0}, {1, 0}, {0, -1}},
     core::ColorRGBA32F::rgba(0.80f, 0.10f, 0.80f, 1.0f)}, // T - Magenta
    {{{0, 0}, {1, 0}, {-1, 1}, {0, 1}},
     core::ColorRGBA32F::rgba(0.20f, 0.90f, 0.20f, 1.0f)}, // S - Green
    {{{-1, 0}, {0, 0}, {0, 1}, {1, 1}},
     core::ColorRGBA32F::rgba(0.90f, 0.20f, 0.20f, 1.0f)}, // Z - Red
    {{{-1, -1}, {-1, 0}, {0, 0}, {1, 0}},
     core::ColorRGBA32F::rgba(0.20f, 0.30f, 0.90f, 1.0f)}, // J - Blue
    {{{1, -1}, {-1, 0}, {0, 0}, {1, 0}},
     core::ColorRGBA32F::rgba(0.90f, 0.50f, 0.10f, 1.0f)}, // L - Orange
    {{{0, 0}, {0, 0}, {0, 0}, {0, 0}},
     core::ColorRGBA32F::rgba(0.50f, 0.50f, 0.50f, 1.0f)}, // Garbage - Gray
};

static int rng_uniform() {
    static std::mt19937 gen {std::random_device {}()};
    std::uniform_int_distribution<int> dist(0, 6);
    return dist(gen);
}

struct Board {
    static constexpr int WIDTH  = 10;
    static constexpr int HEIGHT = 20;
    int grid[HEIGHT][WIDTH]     = {};
};

float evaluate_board(const int grid[20][10]) {
    int heights[10] = {};
    int holes       = 0;
    for (int x = 0; x < 10; ++x) {
        bool found = false;
        for (int y = 0; y < 20; ++y) {
            if (grid[y][x]) {
                if (!found) {
                    heights[x] = 20 - y;
                    found      = true;
                }
            } else if (found) {
                ++holes;
            }
        }
    }
    int agg = 0;
    for (int h: heights)
        agg += h;
    int bump = 0;
    for (int i = 0; i < 9; ++i)
        bump += std::abs(heights[i] - heights[i + 1]);
    return 0.51f * float(agg) + 0.35f * float(holes) + 0.18f * float(bump);
}

struct ActivePiece {
    int type = 0, next_type = 0, hold_type = -1;
    bool hold_used = false;
    int x = 4, y = 1, rotation = 0;
    float fall_timer = 0.0f, fall_speed = 0.5f;
    bool soft_dropping = false;

    static Eigen::Vector2i block_pos(int type, int i, int rot, int ox, int oy) {
        int bx = TETROMINOS[type].blocks[i].x();
        int by = TETROMINOS[type].blocks[i].y();
        for (int r = 0; r < rot; ++r) {
            int t = bx;
            bx    = -by;
            by    = t;
        }
        return {ox + bx, oy + by};
    }

    Eigen::Vector2i get_block_pos(int i) const {
        return block_pos(type, i, rotation, x, y);
    }
};

int get_worst_piece_type(const Board& board) {
    float worst_best[7];
    for (int t = 0; t < 7; ++t) {
        float best = 1e10f;
        for (int rot = 0; rot < 4; ++rot) {
            int mn = 10, mx = -10;
            for (int i = 0; i < 4; ++i) {
                auto p = ActivePiece::block_pos(t, i, rot, 0, 0);
                mn     = std::min(mn, p.x());
                mx     = std::max(mx, p.x());
            }
            for (int bx = -mn; bx < 10 - mx; ++bx) {
                int by = -1;
                while (true) {
                    bool ok = true;
                    for (int i = 0; i < 4 && ok; ++i) {
                        auto p = ActivePiece::block_pos(t, i, rot, bx, by + 1);
                        if (p.y() >= 20 || (p.y() >= 0 && board.grid[p.y()][p.x()]))
                            ok = false;
                    }
                    if (!ok)
                        break;
                    ++by;
                }
                if (by < 0)
                    continue;

                int tmp[20][10];
                for (int r = 0; r < 20; ++r)
                    for (int c = 0; c < 10; ++c)
                        tmp[r][c] = board.grid[r][c];

                for (int i = 0; i < 4; ++i) {
                    auto p = ActivePiece::block_pos(t, i, rot, bx, by);
                    if (p.y() >= 0)
                        tmp[p.y()][p.x()] = t + 1;
                }
                int lines = 0;
                for (int r = 19; r >= 0; --r) {
                    bool full = true;
                    for (int c = 0; c < 10 && full; ++c)
                        full = tmp[r][c] != 0;
                    if (full) {
                        ++lines;
                        for (int sr = r; sr > 0; --sr)
                            for (int c = 0; c < 10; ++c)
                                tmp[sr][c] = tmp[sr - 1][c];
                        for (int c = 0; c < 10; ++c)
                            tmp[0][c] = 0;
                        ++r;
                    }
                }
                float s = evaluate_board(tmp) - 0.76f * float(lines);
                best    = std::min(best, s);
            }
        }
        worst_best[t] = best;
    }
    int worst  = 0;
    float maxv = -1e10f;
    for (int i = 0; i < 7; ++i) {
        if (worst_best[i] > maxv) {
            maxv  = worst_best[i];
            worst = i;
        }
    }
    return worst;
}

static int bag_next_piece() {
    static std::array<int, 7> bag = {0, 1, 2, 3, 4, 5, 6};
    static int pos                = 7;
    static std::mt19937 gen {std::random_device {}()};
    if (pos >= 7) {
        std::shuffle(bag.begin(), bag.end(), gen);
        pos = 0;
    }
    return bag[pos++];
}

int get_next_piece(const Board& board, Difficulty diff) {
    switch (diff) {
        case Difficulty::Standard:
            return bag_next_piece();
        case Difficulty::Hard:
            return rng_uniform();
        case Difficulty::Bastard:
            return get_worst_piece_type(board);
        default:
            return bag_next_piece();
    }
}

static bool is_valid(const Board& board, const ActivePiece& piece, int dx, int dy, int new_rot) {
    for (int i = 0; i < 4; ++i) {
        auto p = ActivePiece::block_pos(piece.type, i, new_rot, piece.x + dx, piece.y + dy);
        if (p.x() < 0 || p.x() >= Board::WIDTH || p.y() >= Board::HEIGHT)
            return false;
        if (p.y() >= 0 && board.grid[p.y()][p.x()])
            return false;
    }
    return true;
}

static int ghost_drop(const Board& board, const ActivePiece& piece) {
    int dy = 0;
    while (is_valid(board, piece, 0, dy + 1, piece.rotation))
        ++dy;
    return dy;
}

// ---------------------------------------------------------------------------
// Game resources
// ---------------------------------------------------------------------------

struct Vertex {
    Eigen::Vector2f position;
};
struct VOut {
    Eigen::Vector4f position;
    core::ColorRGBA32F color;
};

class QuadShader:
    public renderer::
        ShaderPrev<Vertex, VOut, core::ColorRGBA32F, Eigen::Matrix4f, core::ColorRGBA32F> {
public:
    VOut vertex(
        const Vertex& v,
        const Eigen::Matrix4f& model,
        const core::ColorRGBA32F& color
    ) override {
        return {model * Eigen::Vector4f(v.position.x(), v.position.y(), 0.0f, 1.0f), color};
    }
    core::ColorRGBA32F
    fragment(const VOut& v, const Eigen::Matrix4f&, const core::ColorRGBA32F&) override {
        return v.color;
    }
};

using QuadPipeline
    = renderer::Pipeline<Vertex, VOut, core::ColorRGBA32F, Eigen::Matrix4f, core::ColorRGBA32F>;

struct GameResources {
    renderer::BufferHandle<Vertex> quad_buffer;
    renderer::TextureHandle<core::ColorRGBA32F> color_texture;
    renderer::TextureHandle<renderer::CharacterPixel> char_texture;
    renderer::PipelineHandle<QuadPipeline> pipeline;
    std::uint32_t screen_width;
    std::uint32_t screen_height;
};

struct GameStatus {
    GameState state          = GameState::StartMenu;
    GameMode mode            = GameMode::Classic;
    Difficulty difficulty    = Difficulty::Standard;
    ColorTheme theme         = ColorTheme::Catppuccin;
    std::int32_t menu_cursor = 0, mode_cursor = 0, diff_cursor = 0, settings_cursor = 0,
                 leaderboard_tab = 0;
};

struct Score {
    std::int32_t value = 0, lines_cleared = 0, level = 1;
    float garbage_timer = 0.0f;
    std::vector<int> lines_to_clear;
    float clear_anim_timer   = 0.0f;
    float landing_anim_timer = 0.0f;
    std::vector<Eigen::Vector2i> landed_blocks;
};

struct PowerUpState {
    int garbage_clean_charges   = 1;
    int freeze_time_charges     = 1;
    bool freeze_time_active     = false;
    float freeze_time_remaining = 0.0f;
    float freeze_time_duration  = 3.0f;
};

struct SprintTimer {
    std::chrono::steady_clock::time_point start;
    double elapsed_ms = 0.0;
    bool running      = false;
};

struct Particle {
    float lifetime;
    Eigen::Vector2f velocity;
};
struct RenderColor {
    core::ColorRGBA32F color;
};

// ---------------------------------------------------------------------------
// Bitmap Fonts
// ---------------------------------------------------------------------------

#define C3X5(b1, b2, b3, b4, b5) ((b1 << 12) | (b2 << 9) | (b3 << 6) | (b4 << 3) | b5)

static const std::uint16_t FONT_3X5[] = {
    C3X5(0b010, 0b101, 0b111, 0b101, 0b101), // A
    C3X5(0b110, 0b101, 0b110, 0b101, 0b110), // B
    C3X5(0b011, 0b100, 0b100, 0b100, 0b011), // C
    C3X5(0b110, 0b101, 0b101, 0b101, 0b110), // D
    C3X5(0b111, 0b100, 0b110, 0b100, 0b111), // E
    C3X5(0b111, 0b100, 0b110, 0b100, 0b100), // F
    C3X5(0b011, 0b100, 0b101, 0b101, 0b011), // G
    C3X5(0b101, 0b101, 0b111, 0b101, 0b101), // H
    C3X5(0b010, 0b010, 0b010, 0b010, 0b010), // I
    C3X5(0b001, 0b001, 0b001, 0b101, 0b010), // J
    C3X5(0b101, 0b101, 0b110, 0b101, 0b101), // K
    C3X5(0b100, 0b100, 0b100, 0b100, 0b111), // L
    C3X5(0b101, 0b111, 0b101, 0b101, 0b101), // M
    C3X5(0b110, 0b101, 0b101, 0b101, 0b101), // N
    C3X5(0b010, 0b101, 0b101, 0b101, 0b010), // O
    C3X5(0b110, 0b101, 0b110, 0b100, 0b100), // P
    C3X5(0b010, 0b101, 0b101, 0b011, 0b001), // Q
    C3X5(0b110, 0b101, 0b110, 0b101, 0b101), // R
    C3X5(0b011, 0b100, 0b010, 0b001, 0b110), // S
    C3X5(0b111, 0b010, 0b010, 0b010, 0b010), // T
    C3X5(0b101, 0b101, 0b101, 0b101, 0b111), // U
    C3X5(0b101, 0b101, 0b101, 0b010, 0b010), // V
    C3X5(0b101, 0b101, 0b101, 0b111, 0b101), // W
    C3X5(0b101, 0b101, 0b010, 0b101, 0b101), // X
    C3X5(0b101, 0b101, 0b010, 0b010, 0b010), // Y
    C3X5(0b111, 0b001, 0b010, 0b100, 0b111), // Z
    C3X5(0b010, 0b101, 0b101, 0b101, 0b010), // 0
    C3X5(0b010, 0b110, 0b010, 0b010, 0b111), // 1
    C3X5(0b110, 0b001, 0b010, 0b100, 0b111), // 2
    C3X5(0b111, 0b001, 0b011, 0b001, 0b111), // 3
    C3X5(0b101, 0b101, 0b111, 0b001, 0b001), // 4
    C3X5(0b111, 0b100, 0b110, 0b001, 0b110), // 5
    C3X5(0b011, 0b100, 0b110, 0b101, 0b110), // 6
    C3X5(0b111, 0b001, 0b010, 0b010, 0b010), // 7
    C3X5(0b010, 0b101, 0b010, 0b101, 0b010), // 8
    C3X5(0b010, 0b101, 0b011, 0b001, 0b110), // 9
    C3X5(0b000, 0b010, 0b000, 0b010, 0b000), // :
    0, // space
    C3X5(0b000, 0b000, 0b111, 0b000, 0b000), // -
    C3X5(0b000, 0b000, 0b000, 0b000, 0b010), // .
    C3X5(0b100, 0b010, 0b001, 0b010, 0b100), // >
    C3X5(0b010, 0b101, 0b000, 0b000, 0b000), // v
    C3X5(0b111, 0b001, 0b111, 0b001, 0b111), // 3 (duplicate fallback)
};

static std::uint16_t get_3x5_char(char c) {
    if (c >= 'A' && c <= 'Z')
        return FONT_3X5[c - 'A'];
    if (c >= 'a' && c <= 'z')
        return FONT_3X5[c - 'a'];
    if (c >= '0' && c <= '9')
        return FONT_3X5[c - '0' + 26];
    if (c == ':')
        return FONT_3X5[36];
    if (c == ' ')
        return FONT_3X5[37];
    if (c == '-')
        return FONT_3X5[38];
    if (c == '.')
        return FONT_3X5[39];
    if (c == '>')
        return FONT_3X5[40];
    if (c == 'v')
        return FONT_3X5[41];
    return 0;
}

// ---------------------------------------------------------------------------
// Mixed Text Command Integration (Solid Block Elements & Braille Sub-text)
// ---------------------------------------------------------------------------
enum class TextAlign { Left, Center, Right };

struct TextDrawCall {
    std::string text;
    float x, y;
    core::ColorRGBA32F color;
    TextAlign align;
    bool use_braille;
};

static char32_t quadrant_to_char(bool tl, bool tr, bool bl, bool br) {
    int val = (tl ? 1 : 0) | (tr ? 2 : 0) | (bl ? 4 : 0) | (br ? 8 : 0);
    switch (val) {
        case 0:
            return U' ';
        case 1:
            return U'\u2598';
        case 2:
            return U'\u259D';
        case 3:
            return U'\u2580';
        case 4:
            return U'\u2596';
        case 5:
            return U'\u258C';
        case 6:
            return U'\u259E';
        case 7:
            return U'\u259B';
        case 8:
            return U'\u2597';
        case 9:
            return U'\u259A';
        case 10:
            return U'\u2590';
        case 11:
            return U'\u259C';
        case 12:
            return U'\u2584';
        case 13:
            return U'\u2599';
        case 14:
            return U'\u259F';
        case 15:
            return U'\u2588';
    }
    return U' ';
}

class MixedTextCommand: public renderer::RenderCommand {
public:
    MixedTextCommand(
        renderer::ResourceRegistry& registry,
        renderer::TextureHandle<renderer::CharacterPixel> char_texture,
        std::vector<TextDrawCall> calls
    ):
        registry_(registry),
        char_texture_(char_texture),
        calls_(std::move(calls)) {}

    const std::string target_pass() const override {
        return "text_pass";
    }

    void execute(renderer::RenderPassEncoder&) override {
        auto char_tex = registry_.get_texture_mut(char_texture_);
        if (!char_tex)
            return;
        auto* char_data      = char_tex.value();
        std::uint32_t char_w = char_data->width();
        std::uint32_t char_h = char_data->height();

        auto to_rgb8 = [](const core::ColorRGBA32F& c) {
            return core::ColorRGB8::rgb(
                static_cast<std::uint8_t>(std::clamp(c.r * 255.0f, 0.0f, 255.0f)),
                static_cast<std::uint8_t>(std::clamp(c.g * 255.0f, 0.0f, 255.0f)),
                static_cast<std::uint8_t>(std::clamp(c.b * 255.0f, 0.0f, 255.0f))
            );
        };

        struct SubPixel {
            bool active = false;
            core::ColorRGBA32F color;
        };
        std::vector<SubPixel> quad_grid(char_w * 2 * char_h * 2);

        for (const auto& call: calls_) {
            int char_w_dots = 3;
            int spacing     = 1;
            float total_w   = call.text.size() * (char_w_dots + spacing) - spacing;

            float start_x_sub = call.x * 2.0f;
            if (call.align == TextAlign::Center)
                start_x_sub -= total_w / 2.0f;
            else if (call.align == TextAlign::Right)
                start_x_sub -= total_w;

            if (call.use_braille) {
                int by_start = static_cast<int>(call.y * 2.0f);
                for (std::size_t c_idx = 0; c_idx < call.text.size(); ++c_idx) {
                    char c       = call.text[c_idx];
                    int bx_start = static_cast<int>(start_x_sub + c_idx * (char_w_dots + spacing));

                    std::uint16_t bits = get_3x5_char(c);
                    for (int i = 0; i < 15; ++i) {
                        if (bits & (1 << (14 - i))) {
                            int bx = bx_start + (i % 3);
                            int by = by_start + (i / 3);
                            int cx = bx / 2;
                            int cy = by / 4;
                            if (cx >= 0 && cx < static_cast<int>(char_w) && cy >= 0
                                && cy < static_cast<int>(char_h))
                            {
                                auto& cp = char_data->data_mut()[cy * char_w + cx];
                                if (cp.codepoint < U'\u2800' || cp.codepoint > U'\u28FF') {
                                    cp.codepoint = U'\u2800';
                                }
                                static constexpr std::uint32_t BRAILLE_MAP[4][2]
                                    = {{0x01, 0x08}, {0x02, 0x10}, {0x04, 0x20}, {0x40, 0x80}};
                                cp.codepoint |= BRAILLE_MAP[by % 4][bx % 2];
                                cp.fg_color = to_rgb8(call.color);
                            }
                        }
                    }
                }
            } else {
                int by_start = static_cast<int>(call.y * 1.0f);
                for (std::size_t c_idx = 0; c_idx < call.text.size(); ++c_idx) {
                    char c       = call.text[c_idx];
                    int bx_start = static_cast<int>(start_x_sub + c_idx * (char_w_dots + spacing));

                    std::uint16_t bits = get_3x5_char(c);
                    for (int i = 0; i < 15; ++i) {
                        if (bits & (1 << (14 - i))) {
                            int bx = bx_start + (i % 3);
                            int by = by_start + (i / 3);
                            if (bx >= 0 && bx < static_cast<int>(char_w * 2) && by >= 0
                                && by < static_cast<int>(char_h * 2))
                            {
                                quad_grid[by * (char_w * 2) + bx] = {true, call.color};
                            }
                        }
                    }
                }
            }
        }

        for (std::uint32_t cy = 0; cy < char_h; ++cy) {
            for (std::uint32_t cx = 0; cx < char_w; ++cx) {
                int bx  = cx * 2;
                int by  = cy * 2;
                bool tl = quad_grid[by * char_w * 2 + bx].active;
                bool tr = quad_grid[by * char_w * 2 + bx + 1].active;
                bool bl = quad_grid[(by + 1) * char_w * 2 + bx].active;
                bool br = quad_grid[(by + 1) * char_w * 2 + bx + 1].active;

                if (tl || tr || bl || br) {
                    char32_t q_char          = quadrant_to_char(tl, tr, bl, br);
                    core::ColorRGBA32F color = tl ? quad_grid[by * char_w * 2 + bx].color
                        : tr                      ? quad_grid[by * char_w * 2 + bx + 1].color
                        : bl                      ? quad_grid[(by + 1) * char_w * 2 + bx].color
                                                  : quad_grid[(by + 1) * char_w * 2 + bx + 1].color;

                    auto& cp     = char_data->data_mut()[cy * char_w + cx];
                    cp.codepoint = q_char;
                    cp.fg_color  = to_rgb8(color);
                }
            }
        }
    }

private:
    renderer::ResourceRegistry& registry_;
    renderer::TextureHandle<renderer::CharacterPixel> char_texture_;
    std::vector<TextDrawCall> calls_;
};

class QuadRenderCommand: public renderer::RenderCommand {
public:
    QuadRenderCommand(
        renderer::PipelineHandle<QuadPipeline> pipeline,
        renderer::FrameBuffer<renderer::Attachment<core::ColorRGBA32F>> target,
        renderer::BufferHandle<Vertex> vb,
        Eigen::Matrix4f model,
        core::ColorRGBA32F color
    ):
        pipeline_(pipeline),
        target_(target),
        vb_(vb),
        model_(model),
        color_(color) {}

    const std::string target_pass() const override {
        return renderer::halfblock::COLOR_PASS_FG;
    }
    void execute(renderer::RenderPassEncoder& enc) override {
        enc.draw(pipeline_, target_, vb_, std::make_tuple(model_, color_), 6);
    }

private:
    renderer::PipelineHandle<QuadPipeline> pipeline_;
    renderer::FrameBuffer<renderer::Attachment<core::ColorRGBA32F>> target_;
    renderer::BufferHandle<Vertex> vb_;
    Eigen::Matrix4f model_;
    core::ColorRGBA32F color_;
};

// ---------------------------------------------------------------------------
// Systems
// ---------------------------------------------------------------------------

void particle_system(ecs::World& world) {
    auto t_opt = world.get_resource<core::Time>();
    if (!t_opt)
        return;
    float dt = float(t_opt->get().delta_seconds);

    static std::mt19937 gen {std::random_device {}()};
    std::uniform_real_distribution<float> dx(0.0f, 160.0f);
    std::uniform_real_distribution<float> dv(10.0f, 40.0f);
    std::uniform_real_distribution<float> dl(1.5f, 6.5f);

    if (gen() % 10 == 0) {
        float x = dx(gen);
        // Do not spawn behind the central play area (which is at X = 60, width = 40)
        if (x < 55.0f || x > 105.0f) {
            world.spawn(
                core::Transform::from_pos({x, -5.0f, 0.0f}),
                Particle {dl(gen), {0.0f, dv(gen)}},
                RenderColor {core::ColorRGBA32F::rgba(0.4f, 0.4f, 0.7f, 0.4f)}
            );
        }
    }
    std::vector<ecs::Entity> dead;
    for (auto [e, t, p]: ecs::Query<core::Transform, Particle>(&world)) {
        t.translation.x() += p.velocity.x() * dt;
        t.translation.y() += p.velocity.y() * dt;
        p.lifetime -= dt;
        if (p.lifetime <= 0.0f || t.translation.y() > 95.0f || t.translation.y() < -10.0f)
            dead.push_back(e);
    }
    for (auto e: dead)
        world.destroy(e);
}

static char get_char_pressed(const core::input::InputState& input) {
    for (std::uint32_t i = std::uint32_t(core::input::KeyCode::A);
         i <= std::uint32_t(core::input::KeyCode::Z);
         ++i)
    {
        if (input.just_pressed(static_cast<core::input::KeyCode>(i)))
            return char('A' + (i - std::uint32_t(core::input::KeyCode::A)));
    }
    if (input.just_pressed(core::input::KeyCode::Space))
        return ' ';
    return 0;
}

static void spawn_piece(ActivePiece& piece, const Board& board, Difficulty diff) {
    piece.type       = piece.next_type;
    piece.next_type  = get_next_piece(board, diff);
    piece.x          = 4;
    piece.y          = 1;
    piece.rotation   = 0;
    piece.fall_timer = 0.0f;
    piece.hold_used  = false;
}

static void perform_hold(ActivePiece& piece, const Board& board, Difficulty diff) {
    if (piece.hold_used)
        return;
    piece.hold_used = true;

    if (piece.hold_type < 0) {
        piece.hold_type = piece.type;
        piece.type      = piece.next_type;
        piece.next_type = get_next_piece(board, diff);
    } else {
        std::swap(piece.type, piece.hold_type);
    }
    piece.x          = 4;
    piece.y          = 1;
    piece.rotation   = 0;
    piece.fall_timer = 0.0f;
}

static int find_lowest_partial_row(const Board& board) {
    for (int y = Board::HEIGHT - 1; y >= 0; --y) {
        bool any  = false;
        bool full = true;
        for (int x = 0; x < Board::WIDTH; ++x) {
            if (board.grid[y][x])
                any = true;
            else
                full = false;
        }
        if (any && !full)
            return y;
    }
    return -1;
}

static void perform_garbage_clean(Board& board, ActivePiece& piece, Score& score) {
    int row = find_lowest_partial_row(board);
    if (row < 0)
        return;

    for (int x = 0; x < Board::WIDTH; ++x)
        board.grid[row][x] = 0;

    for (int y = row - 1; y >= 0; --y) {
        for (int x = 0; x < Board::WIDTH; ++x)
            board.grid[y + 1][x] = board.grid[y][x];
    }
    for (int x = 0; x < Board::WIDTH; ++x)
        board.grid[0][x] = 0;

    score.value += 50 * score.level;

    while (!is_valid(board, piece, 0, 0, piece.rotation) && piece.y > -2)
        piece.y -= 1;
}

void tetris_input_system(ecs::World& world) {
    auto status_opt = world.get_resource<GameStatus>();
    auto input_opt  = world.get_resource<core::input::InputState>();
    if (!status_opt || !input_opt)
        return;

    auto& status = status_opt->get();
    auto& input  = input_opt->get();

    using K = core::input::KeyCode;

    if (status.state == GameState::StartMenu) {
        constexpr int MENU_ITEMS = 5;
        if (input.just_pressed(K::Down) || input.just_pressed(K::S))
            status.menu_cursor = (status.menu_cursor + 1) % MENU_ITEMS;
        if (input.just_pressed(K::Up) || input.just_pressed(K::W))
            status.menu_cursor = (status.menu_cursor + MENU_ITEMS - 1) % MENU_ITEMS;

        if (input.just_pressed(K::Enter) || input.just_pressed(K::Space)) {
            switch (status.menu_cursor) {
                case 0:
                    status.state = GameState::ModeSelect;
                    break;
                case 1:
                    status.state = GameState::LeaderboardScreen;
                    break;
                case 2:
                    status.state = GameState::SettingsMenu;
                    break;
                case 3:
                    status.state = GameState::CreditsScreen;
                    break;
                case 4:
                    if (auto app = world.get_resource<core::Application*>())
                        app->get()->set_should_exit(true);
                    break;
            }
        }
        if (input.just_pressed(K::T)) {
            status.theme = static_cast<ColorTheme>(
                (std::uint8_t(status.theme) + 1) % std::uint8_t(ColorTheme::Count)
            );
            if (auto set_opt = world.get_resource<Settings>()) {
                set_opt->get().theme = status.theme;
                if (auto ad_opt = world.get_resource<AppData>()) {
                    ad_opt->get().settings = set_opt->get();
                    save_app_data(ad_opt->get());
                }
            }
        }
        if (input.just_pressed(K::Escape) || input.just_pressed(K::Q)) {
            if (auto app = world.get_resource<core::Application*>())
                app->get()->set_should_exit(true);
        }
        return;
    }

    if (status.state == GameState::ModeSelect) {
        constexpr int MODE_COUNT = int(GameMode::Count);
        if (input.just_pressed(K::Down) || input.just_pressed(K::S))
            status.mode_cursor = (status.mode_cursor + 1) % MODE_COUNT;
        if (input.just_pressed(K::Up) || input.just_pressed(K::W))
            status.mode_cursor = (status.mode_cursor + MODE_COUNT - 1) % MODE_COUNT;
        if (input.just_pressed(K::Enter) || input.just_pressed(K::Space)) {
            status.mode  = static_cast<GameMode>(status.mode_cursor);
            status.state = GameState::DifficultySelect;
        }
        if (input.just_pressed(K::Escape) || input.just_pressed(K::Q))
            status.state = GameState::StartMenu;
        return;
    }

    if (status.state == GameState::DifficultySelect) {
        constexpr int DIFF_COUNT = int(Difficulty::Count);
        if (input.just_pressed(K::Down) || input.just_pressed(K::S))
            status.diff_cursor = (status.diff_cursor + 1) % DIFF_COUNT;
        if (input.just_pressed(K::Up) || input.just_pressed(K::W))
            status.diff_cursor = (status.diff_cursor + DIFF_COUNT - 1) % DIFF_COUNT;

        if (input.just_pressed(K::Enter) || input.just_pressed(K::Space)) {
            status.difficulty = static_cast<Difficulty>(status.diff_cursor);
            auto& sc          = world.get_resource<Score>()->get();
            sc                = Score {};
            auto& board       = world.get_resource<Board>()->get();
            for (int r = 0; r < Board::HEIGHT; ++r)
                for (int c = 0; c < Board::WIDTH; ++c)
                    board.grid[r][c] = 0;

            auto& piece     = world.get_resource<ActivePiece>()->get();
            piece.next_type = get_next_piece(board, status.difficulty);
            piece.hold_type = -1;
            piece.hold_used = false;
            spawn_piece(piece, board, status.difficulty);

            if (status.mode == GameMode::Sprint) {
                auto& st      = world.get_resource<SprintTimer>()->get();
                st.start      = std::chrono::steady_clock::now();
                st.elapsed_ms = 0.0;
                st.running    = true;
            }
            if (status.mode == GameMode::Garbage)
                sc.garbage_timer = 0.0f;
            if (auto pu_opt = world.get_resource<PowerUpState>())
                pu_opt->get() = PowerUpState {};
            status.state = GameState::Playing;
        }
        if (input.just_pressed(K::Escape) || input.just_pressed(K::Q))
            status.state = GameState::ModeSelect;
        return;
    }

    if (status.state == GameState::CreditsScreen) {
        if (input.just_pressed(K::Escape) || input.just_pressed(K::Q)
            || input.just_pressed(K::Space) || input.just_pressed(K::Enter))
        {
            status.state = GameState::StartMenu;
        }
        return;
    }

    if (status.state == GameState::Paused) {
        if (input.just_pressed(K::Q) || input.just_pressed(K::Escape)) {
            status.state = GameState::StartMenu;
        } else if (input.just_pressed(K::P) || input.just_pressed(K::Space)
                   || input.just_pressed(K::Enter))
        {
            status.state = GameState::Playing;
        }
        return;
    }

    if (status.state == GameState::GameOver) {
        if (input.just_pressed(K::Space) || input.just_pressed(K::Enter)) {
            auto& hs = world.get_resource<HighScores>()->get();
            auto val = world.get_resource<Score>()->get().value;
            if (hs.qualifies(val, status.difficulty)) {
                status.state = GameState::HighScoreInput;
                hs.current_input.clear();
            } else {
                status.state = GameState::StartMenu;
            }
        }
        return;
    }

    if (status.state == GameState::HighScoreInput) {
        auto& hs = world.get_resource<HighScores>()->get();
        char c   = get_char_pressed(input);
        if (c && hs.current_input.size() < 15)
            hs.current_input += c;
        if ((input.just_pressed(K::Backspace) || input.just_pressed(K::Delete))
            && !hs.current_input.empty())
            hs.current_input.pop_back();

        if (input.just_pressed(K::Enter) && !hs.current_input.empty()) {
            auto val = world.get_resource<Score>()->get().value;
            insert_score(hs, {hs.current_input, val, status.difficulty});
            if (auto ad_opt = world.get_resource<AppData>()) {
                ad_opt->get().high_scores = hs;
                save_app_data(ad_opt->get());
            }
            status.state           = GameState::LeaderboardScreen;
            status.leaderboard_tab = int(status.difficulty);
        }
        return;
    }

    if (status.state == GameState::SettingsMenu) {
        constexpr int SETTINGS_COUNT = 2;
        if (input.just_pressed(K::Down) || input.just_pressed(K::S))
            status.settings_cursor = (status.settings_cursor + 1) % SETTINGS_COUNT;
        if (input.just_pressed(K::Up) || input.just_pressed(K::W))
            status.settings_cursor = (status.settings_cursor + SETTINGS_COUNT - 1) % SETTINGS_COUNT;

        auto& settings = world.get_resource<Settings>()->get();
        if (input.just_pressed(K::Enter) || input.just_pressed(K::Space)
            || input.just_pressed(K::Left) || input.just_pressed(K::Right)
            || input.just_pressed(K::A) || input.just_pressed(K::D))
        {
            if (status.settings_cursor == 0) {
                settings.theme = static_cast<ColorTheme>(
                    (std::uint8_t(settings.theme) + 1) % std::uint8_t(ColorTheme::Count)
                );
                status.theme = settings.theme;
            } else if (status.settings_cursor == 1) {
                settings.ghost_enabled = !settings.ghost_enabled;
            }
            if (auto ad_opt = world.get_resource<AppData>()) {
                ad_opt->get().settings = settings;
                save_app_data(ad_opt->get());
            }
        }
        if (input.just_pressed(K::Escape) || input.just_pressed(K::Q)
            || input.just_pressed(K::Backspace))
            status.state = GameState::StartMenu;
        return;
    }

    if (status.state == GameState::LeaderboardScreen) {
        constexpr int TABS = int(Difficulty::Count);
        if (input.just_pressed(K::Right) || input.just_pressed(K::D))
            status.leaderboard_tab = (status.leaderboard_tab + 1) % TABS;
        if (input.just_pressed(K::Left) || input.just_pressed(K::A))
            status.leaderboard_tab = (status.leaderboard_tab + TABS - 1) % TABS;
        if (input.just_pressed(K::Escape) || input.just_pressed(K::Q)
            || input.just_pressed(K::Space) || input.just_pressed(K::Enter))
            status.state = GameState::StartMenu;
        return;
    }

    if (status.state != GameState::Playing)
        return;

    // Do not process movement inputs while lines are clearing!
    auto score_opt = world.get_resource<Score>();
    if (score_opt && score_opt->get().clear_anim_timer > 0.0f)
        return;

    auto piece_opt = world.get_resource<ActivePiece>();
    auto board_opt = world.get_resource<Board>();
    if (!piece_opt || !board_opt)
        return;
    auto& piece = piece_opt->get();
    auto& board = board_opt->get();

    if (input.just_pressed(K::P) || input.just_pressed(K::Escape)) {
        status.state = GameState::Paused;
        return;
    }

    if (input.just_pressed(K::Left) || input.just_pressed(K::A))
        if (is_valid(board, piece, -1, 0, piece.rotation))
            piece.x -= 1;
    if (input.just_pressed(K::Right) || input.just_pressed(K::D))
        if (is_valid(board, piece, 1, 0, piece.rotation))
            piece.x += 1;

    if (input.just_pressed(K::Up) || input.just_pressed(K::W)) {
        int nr = (piece.rotation + 1) % 4;
        if (is_valid(board, piece, 0, 0, nr))
            piece.rotation = nr;
    }
    if (input.just_pressed(K::Z)) {
        int nr = (piece.rotation + 3) % 4;
        if (is_valid(board, piece, 0, 0, nr))
            piece.rotation = nr;
    }

    if (status.mode != GameMode::HardDropOnly)
        piece.soft_dropping = input.is_button_down(K::Down) || input.is_button_down(K::S);
    else
        piece.soft_dropping = false;

    if (input.just_pressed(K::Space)) {
        int drop = ghost_drop(board, piece);
        piece.y += drop;
        piece.fall_timer = piece.fall_speed;
    }

    if (auto pu_opt = world.get_resource<PowerUpState>()) {
        auto& powerups = pu_opt->get();
        if (input.just_pressed(K::F) && powerups.garbage_clean_charges > 0 && score_opt
            && score_opt->get().clear_anim_timer <= 0.0f)
        {
            perform_garbage_clean(board, piece, score_opt->get());
            --powerups.garbage_clean_charges;
        }

        if (input.just_pressed(K::G) && powerups.freeze_time_charges > 0
            && !powerups.freeze_time_active && score_opt
            && score_opt->get().clear_anim_timer <= 0.0f)
        {
            powerups.freeze_time_active    = true;
            powerups.freeze_time_remaining = powerups.freeze_time_duration;
            --powerups.freeze_time_charges;
        }
    }

    if (input.just_pressed(K::C))
        perform_hold(piece, board, status.difficulty);
}

// ---------------------------------------------------------------------------
// Logic system
// ---------------------------------------------------------------------------

void tetris_logic_system(ecs::World& world) {
    auto status_opt = world.get_resource<GameStatus>();
    if (!status_opt || status_opt->get().state != GameState::Playing)
        return;

    auto piece_opt   = world.get_resource<ActivePiece>();
    auto board_opt   = world.get_resource<Board>();
    auto score_opt   = world.get_resource<Score>();
    auto powerup_opt = world.get_resource<PowerUpState>();
    auto time_opt    = world.get_resource<core::Time>();
    if (!piece_opt || !board_opt || !score_opt || !time_opt)
        return;

    auto& piece  = piece_opt->get();
    auto& board  = board_opt->get();
    auto& score  = score_opt->get();
    auto& status = status_opt->get();
    float dt     = float(time_opt->get().delta_seconds);

    if (status.mode == GameMode::Garbage) {
        score.garbage_timer += dt;
        float garbage_interval = std::max(2.0f, 10.0f - float(score.level));
        if (score.garbage_timer >= garbage_interval) {
            score.garbage_timer = 0.0f;
            for (int y = 0; y < Board::HEIGHT - 1; ++y) {
                for (int x = 0; x < Board::WIDTH; ++x)
                    board.grid[y][x] = board.grid[y + 1][x];
            }
            static std::mt19937 gen {std::random_device {}()};
            std::uniform_int_distribution<int> hole_dist(0, Board::WIDTH - 1);
            int hole = hole_dist(gen);
            for (int x = 0; x < Board::WIDTH; ++x)
                board.grid[Board::HEIGHT - 1][x] = (x == hole) ? 0 : 8;
            while (!is_valid(board, piece, 0, 0, piece.rotation) && piece.y > -2)
                piece.y -= 1;
        }
    }

    if (status.mode == GameMode::Sprint) {
        auto& st = world.get_resource<SprintTimer>()->get();
        if (st.running) {
            auto now      = std::chrono::steady_clock::now();
            st.elapsed_ms = std::chrono::duration<double, std::milli>(now - st.start).count();
            if (score.lines_cleared >= 40) {
                st.running   = false;
                status.state = GameState::GameOver;
            }
        }
    }

    if (score.clear_anim_timer > 0.0f) {
        score.clear_anim_timer -= dt;
        if (score.clear_anim_timer <= 0.0f) {
            // Allocate a new grid to prevent consecutive clears from modifying shifting index positions improperly
            int new_grid[Board::HEIGHT][Board::WIDTH] = {0};
            int dest_y                                = Board::HEIGHT - 1;

            for (int y = Board::HEIGHT - 1; y >= 0; --y) {
                if (std::find(score.lines_to_clear.begin(), score.lines_to_clear.end(), y)
                    == score.lines_to_clear.end())
                {
                    for (int x = 0; x < Board::WIDTH; ++x) {
                        new_grid[dest_y][x] = board.grid[y][x];
                    }
                    --dest_y;
                }
            }

            for (int y = 0; y < Board::HEIGHT; ++y) {
                for (int x = 0; x < Board::WIDTH; ++x) {
                    board.grid[y][x] = new_grid[y][x];
                }
            }

            score.lines_to_clear.clear();
            spawn_piece(piece, board, status.difficulty);
            if (!is_valid(board, piece, 0, 0, piece.rotation))
                status.state = GameState::GameOver;
        }
        return;
    }

    if (score.landing_anim_timer > 0.0f)
        score.landing_anim_timer -= dt;

    if (powerup_opt && powerup_opt->get().freeze_time_active) {
        auto& powerups = powerup_opt->get();
        powerups.freeze_time_remaining -= dt;
        if (powerups.freeze_time_remaining <= 0.0f)
            powerups.freeze_time_active = false;
        return;
    }

    float effective_speed = piece.soft_dropping ? piece.fall_speed / 2.0f : piece.fall_speed;
    piece.fall_timer += dt;

    if (piece.fall_timer < effective_speed)
        return;
    piece.fall_timer = 0.0f;

    if (is_valid(board, piece, 0, 1, piece.rotation)) {
        piece.y += 1;
        if (piece.soft_dropping)
            score.value += 1;
    } else {
        // Landing Block Flash Effect setup
        score.landing_anim_timer = 0.15f;
        score.landed_blocks.clear();

        for (int i = 0; i < 4; ++i) {
            auto p = piece.get_block_pos(i);
            score.landed_blocks.push_back(p);
            if (p.y() >= 0 && p.y() < Board::HEIGHT)
                board.grid[p.y()][p.x()] = piece.type + 1;
        }

        std::vector<int> full_rows;
        for (int y = Board::HEIGHT - 1; y >= 0; --y) {
            bool full = true;
            for (int x = 0; x < Board::WIDTH && full; ++x)
                full = board.grid[y][x] != 0;
            if (full)
                full_rows.push_back(y);
        }

        if (!full_rows.empty()) {
            static constexpr int LINE_SCORE[5] = {0, 100, 300, 500, 800};
            score.value += LINE_SCORE[std::min((int)full_rows.size(), 4)] * score.level;
            score.lines_cleared += full_rows.size();
            score.level            = score.lines_cleared / 10 + 1;
            piece.fall_speed       = std::max(0.05f, 0.5f - 0.04f * float(score.level - 1));
            score.lines_to_clear   = full_rows;
            score.clear_anim_timer = 0.3f;
        } else {
            spawn_piece(piece, board, status.difficulty);
            if (!is_valid(board, piece, 0, 0, piece.rotation))
                status.state = GameState::GameOver;
        }
    }
}

// ---------------------------------------------------------------------------
// Render system
// ---------------------------------------------------------------------------

void tetris_render_system(ecs::World& world) {
    auto renderer_ptr = world.get_resource<std::unique_ptr<renderer::Renderer>>();
    auto game_res_opt = world.get_resource<GameResources>();
    auto status_opt   = world.get_resource<GameStatus>();
    if (!renderer_ptr || !game_res_opt || !status_opt)
        return;

    auto& ren    = *renderer_ptr->get();
    auto& gr     = game_res_opt->get();
    auto& status = status_opt->get();
    auto state   = status.state;
    auto pal     = make_palette(status.theme);

    ren.submit(
        std::make_unique<renderer::halfblock::ClearColorCommand>(
            &ren.resource_registry,
            gr.color_texture,
            pal.background
        )
    );

    renderer::FrameBuffer<renderer::Attachment<core::ColorRGBA32F>> target {
        .attachments = {renderer::Attachment<core::ColorRGBA32F> {
            .view
            = {.texture = gr.color_texture,
               .x       = 0,
               .y       = 0,
               .width   = gr.screen_width,
               .height  = gr.screen_height * 2},
            .load  = renderer::LoadOp::Load,
            .store = renderer::StoreOp::Store,
        }}
    };

    Eigen::Matrix4f proj = Eigen::Matrix4f::Identity();
    proj(0, 0)           = 2.0f / 160.0f;
    proj(1, 1)           = -2.0f / 90.0f;
    proj(0, 3)           = -1.0f;
    proj(1, 3)           = 1.0f;

    auto draw_quad = [&](float x, float y, float w, float h, core::ColorRGBA32F color) {
        Eigen::Matrix4f model = core::Transform::from_pos_rot_scale(
                                    {x + w / 2.0f, y + h / 2.0f, 0.0f},
                                    Eigen::Quaternionf::Identity(),
                                    {w / 2.0f, h / 2.0f, 1.0f}
        )
                                    .to_mat4();
        ren.submit(
            std::make_unique<
                QuadRenderCommand>(gr.pipeline, target, gr.quad_buffer, proj * model, color)
        );
    };

    std::vector<TextDrawCall> text_calls;

    // Submits the 3x5 font request. Either uses Braille mapping (for subtext visual hierarchy) or Quadrant mapping (for primary text)
    auto draw_pixel_text = [&](const std::string& msg,
                               float x,
                               float y,
                               core::ColorRGBA32F color,
                               TextAlign align  = TextAlign::Left,
                               bool use_braille = false) {
        text_calls.push_back({msg, x, y, color, align, use_braille});
    };

    // Helper: Draws very large chunky text by submitting quads (which are then quantized to halfblocks)
    auto draw_large_text = [&](const std::string& msg,
                               float x,
                               float y,
                               float scale,
                               core::ColorRGBA32F color,
                               TextAlign align = TextAlign::Left) {
        float char_w  = 3.0f * scale;
        float spacing = 1.0f * scale;
        float total_w = msg.size() * (char_w + spacing) - spacing;
        float start_x = x;
        if (align == TextAlign::Center)
            start_x -= total_w / 2.0f;
        else if (align == TextAlign::Right)
            start_x -= total_w;

        for (std::size_t c_idx = 0; c_idx < msg.size(); ++c_idx) {
            std::uint16_t bits = get_3x5_char(msg[c_idx]);
            for (int b = 0; b < 15; ++b) {
                if (bits & (1 << (14 - b))) {
                    float bx = start_x + c_idx * (char_w + spacing) + (b % 3) * scale;
                    float by = y + (b / 3) * scale;
                    draw_quad(bx, by, scale, scale, color);
                }
            }
        }
    };

    auto draw_border_box = [&](float x, float y, float w, float h, const std::string& title) {
        draw_quad(x, y, w, 1.0f, pal.border); // Top
        draw_quad(x, y + h, w, 1.0f, pal.border); // Bottom
        draw_quad(x, y, 1.0f, h, pal.border); // Left
        draw_quad(x + w, y, 1.0f, h + 1.0f, pal.border); // Right
        draw_pixel_text(
            title,
            x + w / 2.0f,
            y - 4.0f,
            pal.text_secondary,
            TextAlign::Center,
            true
        ); // Braille Title
    };

    if (state == GameState::StartMenu) {
        float cx = 160.0f / 2.0f;
        draw_large_text("TETRIS", cx, 10.0f, 2.5f, pal.text_accent, TextAlign::Center);

        draw_pixel_text(
            "NEW GAME",
            cx,
            35,
            status.menu_cursor == 0 ? pal.text_accent : pal.text_primary,
            TextAlign::Center,
            false
        );
        draw_pixel_text(
            "LEADERBOARD",
            cx,
            42,
            status.menu_cursor == 1 ? pal.text_accent : pal.text_primary,
            TextAlign::Center,
            false
        );
        draw_pixel_text(
            "SETTINGS",
            cx,
            49,
            status.menu_cursor == 2 ? pal.text_accent : pal.text_primary,
            TextAlign::Center,
            false
        );
        draw_pixel_text(
            "CREDITS",
            cx,
            56,
            status.menu_cursor == 3 ? pal.text_accent : pal.text_primary,
            TextAlign::Center,
            false
        );
        draw_pixel_text(
            "QUIT",
            cx,
            63,
            status.menu_cursor == 4 ? pal.text_accent : pal.text_primary,
            TextAlign::Center,
            false
        );

        draw_pixel_text(
            "> SELECT WITH ARROWS, ENTER TO CONFIRM",
            cx,
            75,
            pal.text_secondary,
            TextAlign::Center,
            true
        );
        draw_pixel_text(
            "T: QUICK THEME CYCLE",
            cx,
            80,
            pal.text_secondary,
            TextAlign::Center,
            true
        );
    } else if (state == GameState::SettingsMenu) {
        float cx = 160.0f / 2.0f;
        draw_large_text("SETTINGS", cx, 10.0f, 2.0f, pal.text_accent, TextAlign::Center);

        auto& settings                              = world.get_resource<Settings>()->get();
        static constexpr const char* THEME_NAMES[3] = {"CATPPUCCIN", "TOKYONIGHT", "KANAGAWA"};
        auto theme_fg = (status.settings_cursor == 0) ? pal.text_accent : pal.text_primary;
        draw_pixel_text(
            std::format("THEME: {}", THEME_NAMES[int(settings.theme)]),
            cx,
            35,
            theme_fg,
            TextAlign::Center,
            false
        );

        auto ghost_fg = (status.settings_cursor == 1) ? pal.text_accent : pal.text_primary;
        draw_pixel_text(
            std::format("GHOST PIECE: {}", settings.ghost_enabled ? "ON" : "OFF"),
            cx,
            45,
            ghost_fg,
            TextAlign::Center,
            false
        );
        draw_pixel_text(
            "ARROWS TO CHANGE  Q/ESC TO BACK",
            cx,
            75,
            pal.text_secondary,
            TextAlign::Center,
            true
        );
    } else if (state == GameState::ModeSelect) {
        float cx = 160.0f / 2.0f;
        draw_large_text("MODE", cx, 10.0f, 2.0f, pal.text_accent, TextAlign::Center);

        static constexpr const char* MODE_NAMES[5]
            = {"CLASSIC", "SPRINT (40 LINES)", "INVISIBLE", "GARBAGE", "HARD DROP ONLY"};
        for (int i = 0; i < 5; ++i) {
            auto fg = (i == status.mode_cursor) ? pal.text_accent : pal.text_primary;
            draw_pixel_text(MODE_NAMES[i], cx, 35 + i * 8, fg, TextAlign::Center, false);
        }
        draw_pixel_text(
            "ENTER TO SELECT / Q TO BACK",
            cx,
            80,
            pal.text_secondary,
            TextAlign::Center,
            true
        );
    } else if (state == GameState::DifficultySelect) {
        float cx = 160.0f / 2.0f;
        draw_large_text("DIFFICULTY", cx, 10.0f, 2.0f, pal.text_accent, TextAlign::Center);

        static constexpr const char* DIFF_NAMES[3]
            = {"STANDARD (BAG)", "HARD (RANDOM)", "BASTARD (ADVERSARIAL)"};
        for (int i = 0; i < 3; ++i) {
            auto fg = (i == status.diff_cursor) ? pal.text_accent : pal.text_primary;
            draw_pixel_text(DIFF_NAMES[i], cx, 35 + i * 8, fg, TextAlign::Center, false);
        }
        draw_pixel_text(
            "ENTER TO START / Q TO BACK",
            cx,
            70,
            pal.text_secondary,
            TextAlign::Center,
            true
        );
    } else if (state == GameState::HighScoreInput) {
        float cx = 160.0f / 2.0f;
        auto val = world.get_resource<Score>()->get().value;
        draw_large_text(
            "NEW HIGHSCORE!",
            cx,
            10.0f,
            2.0f,
            core::ColorRGBA32F::rgba(0, 1, 0, 1),
            TextAlign::Center
        );

        draw_pixel_text(
            std::format("SCORE: {:>6}", val),
            cx,
            30,
            pal.text_accent,
            TextAlign::Center,
            false
        );
        draw_pixel_text("ENTER YOUR NAME:", cx, 45, pal.text_primary, TextAlign::Center, true);
        auto& hs = world.get_resource<HighScores>()->get();
        draw_pixel_text(hs.current_input + "_", cx, 52, pal.text_accent, TextAlign::Center, false);
        draw_pixel_text("ENTER TO SAVE", cx, 70, pal.text_secondary, TextAlign::Center, true);
    } else if (state == GameState::LeaderboardScreen) {
        float cx = 160.0f / 2.0f;
        draw_large_text("LEADERBOARD", cx, 10.0f, 2.0f, pal.text_accent, TextAlign::Center);

        static constexpr const char* TAB_NAMES[3] = {"STANDARD", "HARD", "BASTARD"};
        for (int t = 0; t < 3; ++t) {
            auto fg = (t == status.leaderboard_tab) ? pal.text_accent : pal.text_secondary;
            draw_pixel_text(TAB_NAMES[t], cx - 40 + t * 40, 30, fg, TextAlign::Center, false);
        }
        auto& hs     = world.get_resource<HighScores>()->get();
        auto entries = hs.for_difficulty(static_cast<Difficulty>(status.leaderboard_tab));
        for (std::size_t i = 0; i < std::min(entries.size(), HighScores::MAX_PER_DIFF); ++i) {
            auto line
                = std::format("{:>2}. {:<10.10} {:>6}", i + 1, entries[i].name, entries[i].score);

            draw_pixel_text(line, cx, 36 + i * 5, pal.text_primary, TextAlign::Center, true);
        }
        if (entries.empty())
            draw_pixel_text("NO SCORES YET.", cx, 50, pal.text_secondary, TextAlign::Center, true);
        draw_pixel_text(
            "A/D: CHANGE TAB  Q/SPACE: BACK",
            cx,
            85,
            pal.text_secondary,
            TextAlign::Center,
            true
        );
    } else if (state == GameState::CreditsScreen) {
        float cx = 160.0f / 2.0f;
        draw_large_text("CREDITS", cx, 10.0f, 2.0f, pal.text_accent, TextAlign::Center);

        draw_pixel_text("TETRIS v1.0", cx, 35, pal.text_primary, TextAlign::Center, false);
        draw_pixel_text(
            "CREATED BY NATHAN KAMENCHU",
            cx,
            45,
            pal.text_primary,
            TextAlign::Center,
            false
        );

        draw_pixel_text("POWERED BY", cx, 55, pal.text_secondary, TextAlign::Center, true);
        draw_pixel_text("HOPPER ENGINE", cx, 60, pal.text_accent, TextAlign::Center, false);

        draw_pixel_text("Q/ESC TO BACK", cx, 80, pal.text_secondary, TextAlign::Center, true);
    } else {
        // Draw particles behind everything
        for (auto [e, t, c]: ecs::Query<core::Transform, RenderColor>(&world).with<Particle>()) {
            draw_quad(t.translation.x(), t.translation.y(), 1.0f, 1.0f, c.color);
        }

        // Layout Constants - Play area is exactly centered
        static constexpr float CELL   = 4.0f;
        static constexpr float PLAY_X = 60.0f;
        static constexpr float PLAY_Y = 5.0f;

        draw_quad(PLAY_X - 1, PLAY_Y - 1, 40.0f + 2, 80.0f + 2, pal.border);
        draw_quad(PLAY_X, PLAY_Y, 40.0f, 80.0f, pal.empty_cell);

        if (auto bo = world.get_resource<Board>()) {
            auto& board = bo->get();
            auto& sc    = world.get_resource<Score>()->get();
            bool hide   = (status.mode == GameMode::Invisible && state == GameState::Playing);
            for (int row = 0; row < Board::HEIGHT; ++row) {
                bool is_clearing
                    = std::find(sc.lines_to_clear.begin(), sc.lines_to_clear.end(), row)
                    != sc.lines_to_clear.end();

                for (int col = 0; col < Board::WIDTH; ++col) {
                    int v = board.grid[row][col];
                    if (!v || (hide && v <= 7))
                        continue;
                    float bx   = PLAY_X + col * CELL;
                    float by   = PLAY_Y + row * CELL;
                    auto color = TETROMINOS[v - 1].color;
                    if (is_clearing) {
                        float flash = std::sin(sc.clear_anim_timer * 30.0f) * 0.5f + 0.5f;
                        color       = core::ColorRGBA32F::rgba(
                            color.r + flash * (1.0f - color.r),
                            color.g + flash * (1.0f - color.g),
                            color.b + flash * (1.0f - color.b),
                            1.0f
                        );
                    }
                    draw_quad(bx, by, CELL - 0.2f, CELL - 0.2f, color);
                }
            }
        }

        if (auto po = world.get_resource<ActivePiece>()) {
            auto& piece    = po->get();
            auto& board    = world.get_resource<Board>()->get();
            auto& settings = world.get_resource<Settings>()->get();

            if (state == GameState::Playing && settings.ghost_enabled) {
                int dy = ghost_drop(board, piece);
                if (dy > 0) {
                    auto ghost_color = TETROMINOS[piece.type].color;
                    ghost_color.r *= 0.35f;
                    ghost_color.g *= 0.35f;
                    ghost_color.b *= 0.35f;
                    ghost_color.a = 0.5f;
                    for (int i = 0; i < 4; ++i) {
                        auto p = ActivePiece::block_pos(
                            piece.type,
                            i,
                            piece.rotation,
                            piece.x,
                            piece.y + dy
                        );
                        if (p.y() < 0)
                            continue;
                        float bx = PLAY_X + p.x() * CELL;
                        float by = PLAY_Y + p.y() * CELL;
                        draw_quad(bx, by, CELL - 0.2f, CELL - 0.2f, ghost_color);
                    }
                }
            }

            for (int i = 0; i < 4; ++i) {
                auto p = piece.get_block_pos(i);
                if (p.y() < 0)
                    continue;
                float bx = PLAY_X + p.x() * CELL;
                float by = PLAY_Y + p.y() * CELL;
                draw_quad(bx, by, CELL - 0.2f, CELL - 0.2f, TETROMINOS[piece.type].color);
            }

            auto& sc = world.get_resource<Score>()->get();
            auto& st = status_opt->get();

            // Landing block per-block flash override
            if (sc.landing_anim_timer > 0.0f) {
                float alpha = (sc.landing_anim_timer / 0.15f) * 0.8f;
                for (const auto& p: sc.landed_blocks) {
                    if (p.y() < 0)
                        continue;
                    float bx = PLAY_X + p.x() * CELL;
                    float by = PLAY_Y + p.y() * CELL;
                    draw_quad(
                        bx,
                        by,
                        CELL - 0.2f,
                        CELL - 0.2f,
                        core::ColorRGBA32F::rgba(1.0f, 1.0f, 1.0f, alpha)
                    );
                }
            }

            // Symmetrical UI Positioning
            float left_pane_x  = 20.0f;
            float right_pane_x = 116.0f;
            float pane_y       = 10.0f;
            float box_w        = 24.0f;
            float box_h        = 20.0f;

            // ---- Left Panel ----
            draw_border_box(left_pane_x, pane_y, box_w, box_h, "HOLD");
            if (piece.hold_type >= 0) {
                float b_size    = 3.0f;
                float hx        = left_pane_x + 6.0f;
                float hy        = pane_y + 6.0f;
                auto hold_color = piece.hold_used ? core::ColorRGBA32F::rgba(0.3f, 0.3f, 0.3f, 1.0f)
                                                  : TETROMINOS[piece.hold_type].color;
                for (int i = 0; i < 4; ++i) {
                    auto hp = ActivePiece::block_pos(piece.hold_type, i, 0, 0, 0);
                    draw_quad(
                        hx + hp.x() * b_size,
                        hy + hp.y() * b_size,
                        b_size - 0.2f,
                        b_size - 0.2f,
                        hold_color
                    );
                }
            }

            draw_pixel_text(
                "SCORE",
                left_pane_x + box_w / 2.0f,
                40,
                pal.text_secondary,
                TextAlign::Center,
                true
            );
            draw_pixel_text(
                std::format("{}", sc.value),
                left_pane_x + box_w / 2.0f,
                46,
                pal.text_primary,
                TextAlign::Center,
                false
            );

            draw_pixel_text(
                "LEVEL",
                left_pane_x + box_w / 2.0f,
                56,
                pal.text_secondary,
                TextAlign::Center,
                true
            );
            draw_pixel_text(
                std::format("{}", sc.level),
                left_pane_x + box_w / 2.0f,
                62,
                pal.text_primary,
                TextAlign::Center,
                false
            );

            draw_pixel_text(
                "LINES",
                left_pane_x + box_w / 2.0f,
                72,
                pal.text_secondary,
                TextAlign::Center,
                true
            );
            draw_pixel_text(
                std::format("{}", sc.lines_cleared),
                left_pane_x + box_w / 2.0f,
                78,
                pal.text_primary,
                TextAlign::Center,
                false
            );

            // Instructions on sides
            draw_pixel_text(
                "P:PAUSE",
                left_pane_x + box_w / 2.0f,
                85,
                pal.text_secondary,
                TextAlign::Center,
                true
            );
            draw_pixel_text(
                "C:HOLD",
                right_pane_x + box_w / 2.0f,
                82,
                pal.text_secondary,
                TextAlign::Center,
                true
            );
            draw_pixel_text(
                "Z:ROTL",
                right_pane_x + box_w / 2.0f,
                86,
                pal.text_secondary,
                TextAlign::Center,
                true
            );
            if (auto pu_opt = world.get_resource<PowerUpState>()) {
                auto& powerups = pu_opt->get();
                draw_pixel_text(
                    std::format("F:GARB x{}", powerups.garbage_clean_charges),
                    right_pane_x + box_w / 2.0f,
                    88,
                    pal.text_secondary,
                    TextAlign::Center,
                    true
                );
                draw_pixel_text(
                    powerups.freeze_time_active
                        ? std::format("G:FZ {:.1f}s", powerups.freeze_time_remaining)
                        : std::format("G:FREE x{}", powerups.freeze_time_charges),
                    right_pane_x + box_w / 2.0f,
                    90,
                    pal.text_secondary,
                    TextAlign::Center,
                    true
                );
            }

            // ---- Right Panel ----
            draw_border_box(right_pane_x, pane_y, box_w, box_h, "NEXT");
            {
                float b_size = 3.0f;
                float nx     = right_pane_x + 6.0f;
                float ny     = pane_y + 6.0f;
                for (int i = 0; i < 4; ++i) {
                    auto np = ActivePiece::block_pos(piece.next_type, i, 0, 0, 0);
                    draw_quad(
                        nx + np.x() * b_size,
                        ny + np.y() * b_size,
                        b_size - 0.2f,
                        b_size - 0.2f,
                        TETROMINOS[piece.next_type].color
                    );
                }
            }

            static constexpr const char* MODE_NAMES_SHORT[5]
                = {"CLASSIC", "SPRINT", "INVISIBLE", "GARBAGE", "HARD DROP"};
            draw_pixel_text(
                MODE_NAMES_SHORT[int(st.mode)],
                right_pane_x + box_w / 2.0f,
                40,
                pal.text_primary,
                TextAlign::Center,
                true
            );

            if (status.mode == GameMode::Sprint) {
                auto& sst   = world.get_resource<SprintTimer>()->get();
                int total_s = int(sst.elapsed_ms / 1000.0);
                int ms      = int(sst.elapsed_ms) % 1000;
                draw_pixel_text(
                    std::format("{:02}:{:02}:{:03}", total_s / 60, total_s % 60, ms),
                    right_pane_x + box_w / 2.0f,
                    48,
                    pal.text_accent,
                    TextAlign::Center,
                    true
                );
            }
        }

        if (state == GameState::Paused) {
            draw_quad(0, 0, 160, 90, core::ColorRGBA32F::rgba(0, 0, 0, 0.6f));
            draw_large_text("PAUSED", 80, 30, 2.0f, pal.text_accent, TextAlign::Center);
            draw_pixel_text("SPACE TO RESUME", 80, 48, pal.text_primary, TextAlign::Center, false);
            draw_pixel_text("Q/ESC TO QUIT", 80, 58, pal.text_secondary, TextAlign::Center, true);
        }

        if (state == GameState::GameOver) {
            draw_quad(0, 0, 160, 90, core::ColorRGBA32F::rgba(0, 0, 0, 0.6f));
            draw_large_text(
                "GAME OVER",
                80,
                30,
                2.0f,
                core::ColorRGBA32F::rgba(1, 0, 0, 1),
                TextAlign::Center
            );
            draw_pixel_text(
                "SPACE TO CONTINUE",
                80,
                50,
                pal.text_secondary,
                TextAlign::Center,
                true
            );
        }
    }

    ren.submit(
        std::make_unique<renderer::halfblock::HalfBlockConversionCommand>(
            ren.resource_registry,
            gr.color_texture,
            std::get<0>(ren.render_target().attachments).view.texture
        )
    );

    ren.submit(
        std::make_unique<MixedTextCommand>(
            ren.resource_registry,
            std::get<0>(ren.render_target().attachments).view.texture,
            std::move(text_calls)
        )
    );

    ren.render_frame();
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main() {
    auto app = std::make_shared<core::Application>();
    app->add_layer(logging::LogLayer {});
    app->add_layer(core::input::InputLayer {});

    HOPPER_INFO("app", "Hello world");

    static constexpr std::uint32_t CHAR_W = 160;
    static constexpr std::uint32_t CHAR_H = 45; // 90 pixels high / 2

#if PLATFORM_WASM
    auto presenter = std::make_unique<web::BrowserPresenter>();
#else
    TerminalLayer term_layer(std::make_unique<Terminal>(), 60);
    app->world.insert_resource<TerminalLayer*>(&term_layer);
    app->add_layer(term_layer);
    auto presenter = term_layer.terminal->presenter();
#endif

    auto renderer  = std::make_unique<renderer::Renderer>(CHAR_W, CHAR_H, std::move(presenter));
    auto color_tex = renderer::halfblock::setup(*renderer, CHAR_W, CHAR_H);

    auto text_pass = std::make_unique<renderer::RenderPass>("text_pass");
    text_pass->add_dependency(renderer::halfblock::HALFBLOCK_PASS);
    renderer->render_graph.add_pass(std::move(text_pass));

    std::vector<Vertex> verts = {
        {{-1.f, -1.f}},
        {{-1.f, 1.f}},
        {{1.f, 1.f}},
        {{-1.f, -1.f}},
        {{1.f, 1.f}},
        {{1.f, -1.f}},
    };
    auto quad_buf = renderer->resource_registry.add_buffer<Vertex>(std::move(verts)).value();
    auto pipeline = renderer->resource_registry
                        .add_pipeline(
                            std::make_unique<QuadPipeline>(
                                renderer::PipelineDescriptor {},
                                std::make_unique<QuadShader>()
                            )
                        )
                        .value();

    app->world.insert_resource(
        GameResources {
            .quad_buffer   = quad_buf,
            .color_texture = color_tex,
            .char_texture  = std::get<0>(renderer->render_target().attachments).view.texture,
            .pipeline      = pipeline,
            .screen_width  = CHAR_W,
            .screen_height = CHAR_H,
        }
    );
    app->world.insert_resource(std::move(renderer));
    app->world.insert_resource(app.get());

    app->world.insert_resource(GameStatus {});
    app->world.insert_resource(Score {});
    app->world.insert_resource(Board {});
    app->world.insert_resource(SprintTimer {});
    app->world.insert_resource(PowerUpState {});

    AppData ad;
    load_app_data(ad);
    app->world.insert_resource(ad.settings);
    app->world.insert_resource(ad.high_scores);
    app->world.insert_resource(std::move(ad));

    {
        auto& status   = app->world.get_resource<GameStatus>()->get();
        auto& settings = app->world.get_resource<Settings>()->get();
        status.theme   = settings.theme;
    }

    {
        Board empty_board;
        ActivePiece piece;
        piece.next_type = get_next_piece(empty_board, Difficulty::Standard);
        piece.hold_type = -1;
        spawn_piece(piece, empty_board, Difficulty::Standard);
        app->world.insert_resource(piece);
    }

    // Systems Execution Setup
    app->scheduler.add_system(ecs::Stage::PreUpdate, tetris_input_system);
    app->scheduler.add_system(ecs::Stage::FixedUpdate, tetris_logic_system);
    app->scheduler.add_system(ecs::Stage::Update, particle_system);
    app->scheduler.add_system(ecs::Stage::PostUpdate, tetris_render_system);

    app->run();
    return 0;
}
